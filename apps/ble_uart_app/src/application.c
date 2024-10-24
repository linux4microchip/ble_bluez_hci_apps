/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    application.c

  Summary:
    This file contains BlueZ BLE UART application.

  Description:
    This file contains BlueZ BLE UART application.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/mgmt.h"
#include "shared/shell.h"
#include "shared/timeout.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"
#include "shared/mainloop.h"
#include <poll.h>


#include "application.h"
#include "app_gap.h"
#include "app_error_defs.h"
#include "app_sm.h"
#include "app_scan.h"
#include "app_adv.h"
#include "app_ble_handler.h"
#include "app_dbp.h"
#include "app_mgmt.h"
#include "app_trps.h"
#include "app_trpc.h"
#include "app_agent.h"



// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
#define LOOPBACK_DIFF_RESULT ".compare.result"
#define RAWDATA_DIFF_RESULT ".rawdata.result"
#define ANONYMOUS_OUTPUT_FILE_NAME ".out.file"
#define RAW_DATA_BUFFER_SIZE     (102400) //100K Bytes
//#define RAW_DATA_BUFFER_SIZE     (10240) //10K Bytes
//#define RAW_DATA_BUFFER_SIZE     (2440) //intent to check chunk corner case





// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
GThread *sp_shutdownThread;
GThread *sp_prevThread;
GThread *sp_currThread;

static unsigned char * sp_patternData;
static unsigned int s_patternDataSize;
static uint8_t      s_bleWorkMode;
static unsigned int s_patternFileIndex;
#ifdef ENABLE_AUTO_RUN
static uint16_t      s_expectExecRuns;
static uint16_t      s_currentExecRuns;
static uint8_t       s_joinLinks;
#endif


static const char * s_appWorkModeDesc[] = {
    "null",
    "Checksum",
    "Loopback",
    "Fixed-pattern",
    "raw",
    "Reverse-loopback",
};

static const char * s_appPatternFile[] = {
    "./pattern/1k.txt",
    "./pattern/5k.txt",
    "./pattern/10k.txt",
    "./pattern/50k.txt",
    "./pattern/100k.txt",
    "./pattern/200k.txt",
    "./pattern/500k.txt",
};

static const char * s_appPatternTypeStr[] = {
    "1K",
    "5K",
    "10K",
    "50K",
    "100K",
    "200K",
    "500K",
    "N/A",
};


typedef struct APP_FileTransList_T
{
    DeviceProxy         *p_deviceProxy;
    uint16_t             attMtu;
    unsigned int         txOffset;
    unsigned int         rxOffset;
    char                *p_dataBuf;   //data buffer for data received from loopback of burst mode or raw mode file send/received.
    unsigned int         rawDataSize; //raw mode tx file size
    unsigned int         chunkSize;     //raw mode used in chunk buffer
    unsigned int         chunkNumber;   //raw mode used in chunk buffer
    unsigned int         rwChunkIndex;  //raw mode used in chunk buffer
    char                *p_rawDataFileName;   //raw mode tx/rx data file name
    GTimer              *p_lbTimer;
    APP_TRP_TestStage_T  testStage;
} APP_FileTransList_T;


static APP_FileTransList_T s_appFileTransList[BLE_GAP_MAX_LINK_NBR];




// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************
static APP_FileTransList_T * app_GetFileTransList(DeviceProxy * p_devProxy);
#ifdef ENABLE_DATA_BUFFER_OVERFLOW_MONITOR
static void app_HciEvtMonitor(void);
#endif
static void app_ClearFileTransRecord(APP_FileTransList_T        * p_fileTrans, uint32_t rxDataSize);



static void app_LoopbackProgressingLog(DeviceProxy * p_devProxy)
{
    uint8_t i;
    APP_DBP_BtDev_T *p_dev;


    printf("\rProgressing: ");
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL && s_appFileTransList[i].testStage >= APP_TEST_PROGRESS)
        {
            p_dev = APP_DBP_GetDevInfoByProxy(s_appFileTransList[i].p_deviceProxy);
            if (p_dev == NULL)
                continue;
            
            printf("[%s: %3d%%]", p_dev->p_name, s_appFileTransList[i].rxOffset*100/s_patternDataSize);
        }
    }
    
    fflush(stdout);
}

static void app_LoopbackFinishLog(void)
{
    uint8_t i;
    uint8_t countPass = 0;
    gdouble elapseTime;
    APP_DBP_BtDev_T *p_dev;


    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL)
        {
            if (s_appFileTransList[i].testStage == APP_TEST_PROGRESS)
                return;
            if  (s_appFileTransList[i].testStage == APP_TEST_PASSED)
                countPass++;
        }
    }

#ifdef ENABLE_AUTO_RUN
    printf("\nLoopback (%s) test result(%d runs):\n", s_appPatternTypeStr[s_patternFileIndex], APP_GetPassedRun());
#else
    printf("\nLoopback (%s) test result:\n", s_appPatternTypeStr[s_patternFileIndex]);
#endif
    printf("[Index]	[     Address     ][    Name    ][Result][Time Elapsed]\n");
    printf("=================================================================================\n");

    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].testStage == APP_TEST_IDLE)
            continue;
            
        p_dev = APP_DBP_GetDevInfoByProxy(s_appFileTransList[i].p_deviceProxy);
        if (p_dev == NULL)
            continue;
        
        if (s_appFileTransList[i].testStage == APP_TEST_PASSED)
        {
            elapseTime = g_timer_elapsed(s_appFileTransList[i].p_lbTimer, NULL);
        }
        
        printf("dev#%2d\t[%s][%s][%s][%f s]\n", p_dev->index, p_dev->p_address, p_dev->p_name, 
            APP_TRP_TestStageStr[s_appFileTransList[i].testStage], elapseTime);
    }

    bt_shell_printf("\n");

#ifdef ENABLE_AUTO_RUN
    APP_GoNextRun(countPass);
#endif
}

static void app_RawDataProgressingLog(DeviceProxy * p_devProxy)
{
    APP_FileTransList_T * p_fileTrans;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
        return;

    if (p_fileTrans->rxOffset)
    {
        printf("\rProgressing(Rx:%d)", p_fileTrans->rxOffset);
    }
    else if (p_fileTrans->txOffset)
    {
        printf("\rProgressing(Tx:%d)", p_fileTrans->txOffset);
    }


    fflush(stdout);
}

static APP_FileTransList_T * app_GetFileTransList(DeviceProxy * p_devProxy)
{
    uint8_t i;
    APP_DBP_BtDev_T *p_Dev;

    if (p_devProxy == NULL)
        return NULL;
    
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy == p_devProxy)
            return &s_appFileTransList[i];
    }

    p_Dev = APP_DBP_GetDevInfoByProxy(p_devProxy);
    if (p_Dev == NULL || p_Dev->isConnected == false)
        return NULL;

    //allocate free one if not found
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy == NULL)
        {
            s_appFileTransList[i].p_deviceProxy = p_devProxy;
            s_appFileTransList[i].p_lbTimer = g_timer_new();
            return &s_appFileTransList[i];
        }
    }

    return NULL;
}

static void app_FreeFileTransList(DeviceProxy * p_devProxy)
{
    uint8_t i;
    
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy == p_devProxy)
        {
            if (s_appFileTransList[i].p_lbTimer)
            {
                g_timer_destroy(s_appFileTransList[i].p_lbTimer);
            }
            memset(&s_appFileTransList[i], 0, sizeof(APP_FileTransList_T));
            break;
        }
    }
}


static void app_OpenPatternFile(const char * path)
{
    struct stat st;
    ssize_t len;
    int fd;


    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open pattern file %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Failed to get pattern file size\n");
        close(fd);
        return;
    }

    if (sp_patternData)
    {
        free(sp_patternData);
    }

    sp_patternData = malloc(st.st_size);
    if (!sp_patternData) {
        fprintf(stderr, "Failed to allocate pattern file buffer\n");
        close(fd);
        return;
    }

    len = read(fd, sp_patternData, st.st_size);
    if (len < 0) {
        fprintf(stderr, "Failed to read pattern file\n");
        close(fd);
        return;
    }

    s_patternDataSize = st.st_size;

    close(fd);
}

static void app_OpenRawDataByChunk(APP_FileTransList_T * p_fileTrans, const char * p_filePath, uint32_t chunkSize)
{
    struct stat st;
    ssize_t len;
    int fd;
    unsigned int readSize;

    if (p_fileTrans->p_dataBuf)
    {
        free(p_fileTrans->p_dataBuf);
    }
    
    fd = open(p_filePath, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open file %s\n", p_filePath);
        return;
    }
    
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Failed to get file size\n");
        close(fd);
        return;
    }

    p_fileTrans->rawDataSize = st.st_size;
    p_fileTrans->p_rawDataFileName = strdup(p_filePath);
    p_fileTrans->p_dataBuf = malloc(chunkSize);
    if (!p_fileTrans->p_dataBuf) {
        fprintf(stderr, "Failed to allocate file buffer\n");
        close(fd);
        return;
    }

    if (p_fileTrans->rawDataSize < chunkSize)
        readSize = p_fileTrans->rawDataSize;
    else
        readSize = chunkSize;
    
    len = read(fd, p_fileTrans->p_dataBuf, readSize);
    if (len < 0) {
        fprintf(stderr, "Failed to read file\n");
        close(fd);
        return;
    }
    //printf("open read(%d,%ld)\n", readSize, len);
    
    close(fd);
    
    p_fileTrans->rwChunkIndex = 0;
    p_fileTrans->chunkSize = chunkSize;

    p_fileTrans->chunkNumber = p_fileTrans->rawDataSize / p_fileTrans->chunkSize;
    if (p_fileTrans->rawDataSize % p_fileTrans->chunkSize)
        p_fileTrans->chunkNumber += 1;
    
}


static bool app_LoadRawDataNextChunk(APP_FileTransList_T * p_fileTrans)
{
    ssize_t len;
    int fd;
    unsigned int readSize;
    unsigned int offset;

    if (p_fileTrans->p_dataBuf == NULL)
        return false;
    if ((p_fileTrans->rwChunkIndex+1) == p_fileTrans->chunkNumber)
    {
        fprintf(stderr, "Failed to change chunk: last chunk\n");
        return false;
    }
    
    fd = open(p_fileTrans->p_rawDataFileName, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open file %s\n", p_fileTrans->p_rawDataFileName);
        return false;
    }

    offset = p_fileTrans->chunkSize*(p_fileTrans->rwChunkIndex+1);
    
    if (lseek(fd, offset, SEEK_SET) >= 0)
    {
        if (p_fileTrans->rawDataSize - offset > p_fileTrans->chunkSize) {
            readSize = p_fileTrans->chunkSize;
        }else{
            readSize = p_fileTrans->rawDataSize - offset;
        }

        len = read(fd, p_fileTrans->p_dataBuf, readSize);
        if (len < 0) {
            fprintf(stderr, "Failed to read file\n");
            close(fd);
            return false;
        }
        p_fileTrans->rwChunkIndex++;
    }
    else
    {
        fprintf(stderr, "Failed to seek file\n");
    }
    
    close(fd);

    return true;
}

void app_SaveRawDataByChunk(DeviceProxy * p_devProxy, bool rxFinish) {
    int fd;
    int wsize;
    int devIdx = 1000;
    int oFlag = 0;
    APP_DBP_BtDev_T *p_dev;
    APP_FileTransList_T * p_fileTrans;
    struct stat st;
    char diffCmd[512];
    char rmDiffResultCmd[256];
    char diffResultFileName[128];


    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }

    p_dev = APP_DBP_GetDevInfoByProxy(p_fileTrans->p_deviceProxy);

    if (p_fileTrans->p_rawDataFileName == NULL)
    {
        return;
    }

    if (!p_dev)
        bt_shell_printf("<Text Mode> Received(%d bytes).\n", p_fileTrans->rxOffset);
    else {
        devIdx = p_dev->index;
        bt_shell_printf("<Text Mode> Received(%d bytes) from[%s].\n", p_fileTrans->rxOffset, p_dev->p_address);
    }

    if (p_fileTrans->rwChunkIndex == 0)
        oFlag |= O_TRUNC | O_CREAT;

    oFlag |= O_RDWR;

    umask(0);

    fd = open(p_fileTrans->p_rawDataFileName, oFlag, 0755);
    if (fd < 0) {
        fprintf(stderr, "Failed to open output file: %s (%s)\n", p_fileTrans->p_rawDataFileName, strerror(errno));
        return;
    }
    
    if (p_fileTrans->rwChunkIndex > 0)
    {
        if (lseek(fd, 0, SEEK_END) < 0) {
            fprintf(stderr, "Failed to append output file(%d): %s [%s]\n", errno, p_fileTrans->p_rawDataFileName, strerror(errno));
            return;
        }
    }

    
    if (p_fileTrans->rxOffset % p_fileTrans->chunkSize > 0){
        wsize = p_fileTrans->rxOffset % p_fileTrans->chunkSize;
    }else{
        wsize = p_fileTrans->chunkSize;
    }
    
    if (write(fd, p_fileTrans->p_dataBuf, wsize) < 0) {
        fprintf(stderr, "Failed to write output file: %s\n", p_fileTrans->p_rawDataFileName);
        close(fd);
        return;
    }

    p_fileTrans->rwChunkIndex++;
    
    close(fd);

    if (rxFinish)
    {
        if(s_patternFileIndex < APP_PATTERN_FILE_TYPE_MAX)
        {
            sprintf(diffResultFileName, "%s%d", RAWDATA_DIFF_RESULT, devIdx);
            sprintf(diffCmd, "diff %s %s > %s", p_fileTrans->p_rawDataFileName, s_appPatternFile[s_patternFileIndex], diffResultFileName);
            sprintf(rmDiffResultCmd, "rm -rf %s", diffResultFileName);

            system(rmDiffResultCmd);
            system(diffCmd);
            
            if (stat(diffResultFileName, &st) == 0)
            {
                if (st.st_size == 0) {
                    bt_shell_printf("Raw data compare [%s] successfully.\n", s_appPatternTypeStr[s_patternFileIndex]);
                } else {
                    bt_shell_printf("Raw data compare [%s] failed.\n", s_appPatternTypeStr[s_patternFileIndex]);
                }
            } else {
                bt_shell_printf("Raw data compare [%s] failed.\n", s_appPatternTypeStr[s_patternFileIndex]);
            }
        }
        else
        {
            bt_shell_printf("No comparison due to no pattern selected.\n");
        }
    }
        
}

gpointer app_SaveFileThread(gpointer data) {
    int fd;
    int wlen;
    int devIdx = 1000;
    struct stat st;
    unsigned int overSize;
    char fileNameFull[256];
    char filenameSuffix[16];
    char diffCmd[512];
    char rmDiffResultCmd[256];
    char diffResultFileName[128];
    time_t now;
    struct tm * p_timeStruct;
    APP_DBP_BtDev_T *p_dev;
    APP_FileTransList_T * p_fileTrans;


    if(data == NULL)
    {
        bt_shell_printf("user data is NULL\n");
        return NULL;
    }

    p_fileTrans = (APP_FileTransList_T * )data;
    p_dev = APP_DBP_GetDevInfoByProxy(p_fileTrans->p_deviceProxy);

    now = time(0);
    p_timeStruct = localtime(&now);
    strftime(filenameSuffix, 16, "%Y%m%d_%H%M%S", p_timeStruct);
    
    if (!p_dev)
        sprintf(fileNameFull, "Raw-%s", filenameSuffix);
    else if (p_dev->p_name)
    {
        devIdx = p_dev->index;
        sprintf(fileNameFull, "%s-Raw-%s", p_dev->p_name, filenameSuffix);
    }
    else
    {
        devIdx = p_dev->index;
        sprintf(fileNameFull, "%s-Raw-%s", p_dev->p_address, filenameSuffix);
    }

    if (p_fileTrans->rxOffset > s_patternDataSize)
        overSize = p_fileTrans->rxOffset - s_patternDataSize;
    else
        overSize = 0;

    //bt_shell_printf("Dump received data(%d/%d) to file=%s\n", p_fileTrans->rxOffset, s_patternDataSize, fileNameFull);
    fd = open(fileNameFull, O_CREAT | O_RDWR, 0755);
    if (fd < 0) {
        fprintf(stderr, "Failed to open dump pattern file\n");
        return NULL;
    }

    wlen = write(fd, p_fileTrans->p_dataBuf, p_fileTrans->rxOffset - overSize);
    if (wlen < 0) {
        fprintf(stderr, "Failed to dump pattern file\n");
        close(fd);
        return NULL;
    }

    close(fd);


    sprintf(diffResultFileName, "%s%d", LOOPBACK_DIFF_RESULT, devIdx);
    sprintf(diffCmd, "diff %s %s > %s", fileNameFull, s_appPatternFile[s_patternFileIndex], diffResultFileName);
    sprintf(rmDiffResultCmd, "rm -rf %s", diffResultFileName);

    system(rmDiffResultCmd);
    system(diffCmd);
    
    if (stat(diffResultFileName, &st) == 0)
    {
        if (st.st_size == 0) {
            p_fileTrans->testStage = APP_TEST_PASSED;
        } else {
            p_fileTrans->testStage = APP_TEST_FAILED;
        }
    } else {
        p_fileTrans->testStage = APP_TEST_FAILED;
    }

    app_LoopbackFinishLog();

    return NULL;
}

static void app_SaveLoopbackDataToFile(DeviceProxy * p_devProxy)
{
    APP_FileTransList_T * p_fileTrans;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }

    if (p_fileTrans->p_lbTimer)
        g_timer_stop(p_fileTrans->p_lbTimer);

    if (sp_prevThread)
        g_thread_unref(sp_prevThread);
    
    sp_currThread = g_thread_new("lpthread", app_SaveFileThread, p_fileTrans);
    sp_prevThread = sp_currThread;
}


static void app_SaveLoopbackDataToRam(DeviceProxy * p_devProxy, const unsigned char * value, int len)
{
    APP_FileTransList_T * p_fileTrans;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }
        
    if (len)
    {
        memcpy(&p_fileTrans->p_dataBuf[p_fileTrans->rxOffset], value, len);
        p_fileTrans->rxOffset += len;
    }
    

    app_LoopbackProgressingLog(p_devProxy);

    if (p_fileTrans->rxOffset >= s_patternDataSize)
    {
        uint8_t transIndex = APP_GetFileTransIndex(p_devProxy);

        APP_TIMER_StopTimer(APP_TIMER_FILE_FETCH, transIndex);
        APP_TIMER_StopTimer(APP_TIMER_LOOPBACK_RX_CHECK, transIndex);

        app_SaveLoopbackDataToFile(p_devProxy);
    }
}

static void app_SaveRawDataToRam(DeviceProxy * p_devProxy, const unsigned char * value, int len)
{
    APP_FileTransList_T * p_fileTrans;
    unsigned int chunkOffset;
    unsigned int seg = 0;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }

    if (p_fileTrans->p_dataBuf == NULL)
    {
        printf("output buffer is NULL\n");
        return;
    }

    if (len <= 0)
    {
        printf("write length error(%d)\n", len);
        return;
    }

    chunkOffset = p_fileTrans->rxOffset % p_fileTrans->chunkSize;

    //Chunk is full or data will overflow the chunk.
    if ( (chunkOffset == 0 && p_fileTrans->rxOffset != 0) ||
        (chunkOffset + len) > RAW_DATA_BUFFER_SIZE)
    {
        //copy front part to fill chunk fully.
        seg = RAW_DATA_BUFFER_SIZE - chunkOffset;
        if (seg != RAW_DATA_BUFFER_SIZE)
        {
            memcpy(&p_fileTrans->p_dataBuf[chunkOffset], value, seg);
            p_fileTrans->rxOffset += seg;
            chunkOffset = p_fileTrans->rxOffset % p_fileTrans->chunkSize;
            len -= seg;
        }
        else {
            seg = 0;
        }
        //save full chunk
        app_SaveRawDataByChunk(p_devProxy, false);
    }

    if (len && (chunkOffset + len) <= RAW_DATA_BUFFER_SIZE)
    {
        memcpy(&p_fileTrans->p_dataBuf[chunkOffset], value + seg, len);
        p_fileTrans->rxOffset += len;
    }

    app_RawDataProgressingLog(p_devProxy);
}

static uint16_t app_GetPatternDataLength(APP_FileTransList_T * p_fileTrans)
{
    uint16_t copyLen;
    //APP_DBP_BtDev_T *p_dev;
    APP_TRP_ConnList_T *p_trpConn;
    uint8_t qNum = 0;
    
    if (sp_patternData == NULL)
    {
        printf("%s:pattern file is NULL\n", __FUNCTION__);
        return 0;
    }

    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_fileTrans->p_deviceProxy);
    if (p_trpConn)
        qNum = APP_UTILITY_GetAvailCircQueueNum(&(p_trpConn->uartCircQueue));

    if (qNum == 0)
        return 0;

#if 0
    p_dev = APP_DBP_GetDevInfoByProxy(p_fileTrans->p_deviceProxy);
    if (p_dev && p_dev->ssf == 0){
        copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE;
    }
    else{   //for Multi-Event-Notify enabled
        copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE - 2;
    }
#else
    //for Multi-Event-Notify consideration
    copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE - ATT_MULTI_EVENT_NOTIFY_SINGLE_VALUE_PAIR;
#endif

    if (s_patternDataSize - p_fileTrans->txOffset < copyLen)
    {
        copyLen = s_patternDataSize - p_fileTrans->txOffset;
    }

    return copyLen;
}

static uint16_t app_GetFileDataLength(APP_FileTransList_T * p_fileTrans)
{
    uint16_t copyLen;
    //APP_DBP_BtDev_T *p_dev;
    APP_TRP_ConnList_T *p_trpConn;
    uint8_t qNum = 0;
    

    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_fileTrans->p_deviceProxy);
    if (p_trpConn)
        qNum = APP_UTILITY_GetAvailCircQueueNum(&(p_trpConn->uartCircQueue));

    if (qNum == 0)
        return 0;

#if 0
    p_dev = APP_DBP_GetDevInfoByProxy(p_fileTrans->p_deviceProxy);
    if (p_dev && p_dev->ssf == 0){
        copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE;
    }
    else{   //for Multi-Event-Notify enabled
        copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE - 2;
    }
#else
    //for Multi-Event-Notify consideration
    copyLen = p_fileTrans->attMtu - ATT_WRITE_HEADER_SIZE - ATT_MULTI_EVENT_NOTIFY_SINGLE_VALUE_PAIR;
#endif

    if (p_fileTrans->rawDataSize - p_fileTrans->txOffset < copyLen)
    {
        copyLen = p_fileTrans->rawDataSize - p_fileTrans->txOffset;
    }

    return copyLen;
}



#ifdef ENABLE_DATA_BUFFER_OVERFLOW_MONITOR
#define HCI_MAX_EVENT_SIZE  260

static void app_PollEvt(int fd)
{
    unsigned char buf[HCI_MAX_EVENT_SIZE];
    unsigned char control[64];
    struct msghdr msg;
    struct iovec iov;
    int to = 1000;

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    while (1) {
        ssize_t len;

        if (to) {
            struct pollfd p;
            int n;

            p.fd = fd; p.events = POLLIN;
            while ((n = poll(&p, 1, to)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;
            }

            if (!n) {
                errno = ETIMEDOUT;
                continue;
            }

            to -= 10;
            if (to < 0)
                to = 0;
        }

        len = recvmsg(fd, &msg, MSG_DONTWAIT);
        if (len < 0)
            continue;


        switch (buf[0]) {
        case HCI_EVENT_PKT:
            APP_TIMER_SetTimer(APP_TIMER_DATA_BUFFER_OVERFLOW_EVT, 0, NULL, APP_TIMER_1MS);
            break;
        }
    }
}


static int app_OpenHciDev(uint16_t index)
{
    struct sockaddr_hci addr;
    struct hci_filter flt;
    int fd;

    fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (fd < 0) {
        perror("Failed to open channel");
        return -1;
    }

    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT,  &flt);
    //hci_filter_set_event(EVT_NUM_COMP_PKTS, &flt);
    hci_filter_set_event(EVT_DATA_BUFFER_OVERFLOW, &flt);

    if (setsockopt(fd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        perror("Failed to set HCI filter");
        close(fd);
        return -1;
    }


    memset(&addr, 0, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = index;
    addr.hci_channel = HCI_CHANNEL_RAW;
    
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Failed to bind channel");
        close(fd);
        return -1;
    }

    return fd;
}


gpointer app_HciEvtMonitorThread(gpointer data)
{
    int fd;

    fd = app_OpenHciDev(0);
    if (fd < 0) {
        printf("open_hci_dev fail, create HciEvtMonitorThread failed\n");
        return NULL;
    }

    app_PollEvt(fd);

    return NULL;
}


static void app_HciEvtMonitor(void)
{
    g_thread_new("hciMonthread", app_HciEvtMonitorThread, NULL);
}
#endif


uint16_t APP_FileRead(DeviceProxy * p_devProxy, uint8_t *p_buffer, uint16_t len)
{
    uint16_t copyLen;
    APP_FileTransList_T * p_fileTrans;
    
    if (sp_patternData == NULL)
    {
        printf("%s:pattern file is NULL\n", __FUNCTION__);
        return 0;
    }

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return 0;
    }

    p_fileTrans->testStage = APP_TEST_PROGRESS;

    if (s_patternDataSize - p_fileTrans->txOffset > len)
    {
        copyLen = len;
    }
    else
    {
        copyLen = s_patternDataSize - p_fileTrans->txOffset;
    }

    memcpy(p_buffer, &sp_patternData[p_fileTrans->txOffset], copyLen);
    p_fileTrans->txOffset += copyLen;


    if (p_fileTrans->txOffset == copyLen)
    {
        g_timer_start(p_fileTrans->p_lbTimer);
    }

    app_LoopbackProgressingLog(p_devProxy);
    
    return copyLen;
}


uint16_t APP_ConsoleRead(DeviceProxy * p_devProxy, uint8_t *p_buffer, uint16_t len)
{
    uint16_t copyLen;
    uint16_t remainLen; //remain data in next chunk
    APP_FileTransList_T * p_fileTrans;
    APP_TRP_ConnList_T * p_trpConn;
    APP_DBP_BtDev_T * p_dev;
    bool changeChunk = false;

    
    p_fileTrans = app_GetFileTransList(p_devProxy);
    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return 0;
    }

    if (p_fileTrans->rawDataSize - p_fileTrans->txOffset > len)
    {
        copyLen = len;
    }
    else
    {
        copyLen = p_fileTrans->rawDataSize - p_fileTrans->txOffset;
    }

    if (((p_fileTrans->txOffset % p_fileTrans->chunkSize) + copyLen) >= p_fileTrans->chunkSize)
    {
        remainLen = copyLen;
        copyLen = p_fileTrans->chunkSize - (p_fileTrans->txOffset % p_fileTrans->chunkSize);
        remainLen -= copyLen;

        if (p_fileTrans->chunkNumber > 1) {
            changeChunk = true;
        }
    }


    memcpy(p_buffer, p_fileTrans->p_dataBuf + (p_fileTrans->txOffset % p_fileTrans->chunkSize), copyLen);
    p_fileTrans->txOffset += copyLen;

    app_RawDataProgressingLog(p_devProxy);

    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    p_dev = APP_DBP_GetDevInfoByProxy(p_devProxy);
    if (p_fileTrans->txOffset == p_fileTrans->rawDataSize)
    {
        if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
            bt_shell_printf("<Text Mode> Notify(%d bytes) to all clients successed\n", p_fileTrans->rawDataSize);
        else if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE && p_dev != NULL)
            bt_shell_printf("<Text Mode> Sent(%d bytes) to peer[%s] successed\n", p_fileTrans->rawDataSize, p_dev->p_address);
    }

    if(changeChunk)
    {
        if(app_LoadRawDataNextChunk(p_fileTrans))
        {
            memcpy((p_buffer + copyLen), p_fileTrans->p_dataBuf + (p_fileTrans->txOffset % p_fileTrans->chunkSize), remainLen);
            p_fileTrans->txOffset += remainLen;

            copyLen += remainLen;
        }
    }

    return copyLen;
}

void APP_FileWriteTimeout(void *p_param)
{
    uint8_t transIndex;
    DeviceProxy  *p_devProxy = (DeviceProxy *)p_param;

    transIndex = APP_GetFileTransIndex(p_devProxy);

    APP_TIMER_StopTimer(APP_TIMER_FILE_FETCH, transIndex);
    APP_TIMER_StopTimer(APP_TIMER_LOOPBACK_RX_CHECK, transIndex);
        
    app_SaveLoopbackDataToFile(p_devProxy);
}

void APP_RawDataFileWriteTimeout(void *p_param)
{
    app_SaveRawDataByChunk((DeviceProxy *)p_param, true);
    app_ClearFileTransRecord(app_GetFileTransList((DeviceProxy *)p_param), RAW_DATA_BUFFER_SIZE);
}

uint16_t APP_FileWrite(DeviceProxy * p_devProxy, uint16_t length, uint8_t *p_buffer)
{
    APP_FileTransList_T * p_fileTrans;
    APP_TRP_ConnList_T * p_trpConn;
    uint8_t transIndex;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    transIndex = APP_GetFileTransIndex(p_devProxy);

    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return APP_RES_FAIL;
    }

    if (s_bleWorkMode == TRP_WMODE_NULL && p_trpConn->workMode == TRP_WMODE_NULL)
    {
        //ignore this output
        printf("ignore received data in work mode unspecified.\n");
        return APP_RES_SUCCESS;
    }
    else if (s_bleWorkMode == TRP_WMODE_LOOPBACK)
    {
        APP_TIMER_SetTimer(APP_TIMER_LOOPBACK_RX_CHECK, transIndex, (void*)p_devProxy, APP_TIMER_3S);
        app_SaveLoopbackDataToRam(p_devProxy, p_buffer, length);
        return APP_RES_SUCCESS;
    }
    else
    {
        return APP_RES_FAIL;
    }

    return APP_RES_FAIL;
}

uint16_t APP_ConsoleWrite(DeviceProxy * p_devProxy, uint16_t length, uint8_t *p_buffer)
{
    APP_FileTransList_T * p_fileTrans;
    APP_TRP_ConnList_T * p_trpConn;
    APP_DBP_BtDev_T * p_dev;
    uint8_t transIndex;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    transIndex = APP_GetFileTransIndex(p_devProxy);

    if(p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return APP_RES_FAIL;
    }

    if (s_bleWorkMode == TRP_WMODE_NULL && p_trpConn->workMode == TRP_WMODE_NULL)
    {
        //ignore this output
        printf("ignore received data in work mode unspecified.\n");
        return APP_RES_SUCCESS;
    }
    else if (s_bleWorkMode == TRP_WMODE_UART && p_fileTrans->p_rawDataFileName != NULL)
    {
        APP_TIMER_SetTimer(APP_TIMER_RAW_DATA_RX_CHECK, transIndex, (void*)p_devProxy, APP_TIMER_3S);
        app_SaveRawDataToRam(p_devProxy, p_buffer, length);
        
        return APP_RES_SUCCESS;
    }
    else if (s_bleWorkMode == TRP_WMODE_UART || p_trpConn->workMode == TRP_WMODE_UART)
    {
        
        char * p_outStr;
        p_outStr = malloc(length + 1); //append string termination symbol
        if (p_outStr == NULL)
        {
            return APP_RES_FAIL;
        }

        p_dev = APP_DBP_GetDevInfoByProxy(p_trpConn->p_deviceProxy);
        
        memcpy(p_outStr, p_buffer, length);
        p_outStr[length] = '\0';
        
        bt_shell_printf("<Text Mode> Received(%d bytes) from[%s]:%s\n", length, p_dev->p_address , p_outStr);

        free(p_outStr);
        
        return APP_RES_SUCCESS;
    }
    else
    {
        return APP_RES_FAIL;
    }

    return APP_RES_FAIL;
}


void APP_FetchTxDataFromPatternFile(DeviceProxy *p_deviceProxy)
{
    //uint8_t idx;
    uint16_t dataLeng;
    APP_FileTransList_T *p_fileTrans;

    if (p_deviceProxy == NULL)
        return;
    
    p_fileTrans = app_GetFileTransList(p_deviceProxy);
    if (p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }
    
    dataLeng = app_GetPatternDataLength(p_fileTrans);

    
    //if (dataLeng > 0)
    {
        APP_TRP_COMMON_FetchTxData(p_deviceProxy, dataLeng);
    }
}

void APP_FetchTxDataFromRawDataFile(DeviceProxy *p_deviceProxy)
{
    uint16_t dataLeng;
    APP_FileTransList_T *p_fileTrans;

    if (p_deviceProxy == NULL)
        return;
    
    p_fileTrans = app_GetFileTransList(p_deviceProxy);
    if (p_fileTrans == NULL)
    {
        printf("p_fileTrans is NULL\n");
        return;
    }
    
    dataLeng = app_GetFileDataLength(p_fileTrans);

    //if (dataLeng)
    //    printf("APP_FetchTxDataFromRawDataFile, dataLeng=%d\n", dataLeng);
    
    if (dataLeng > 0)
    {
        APP_TRP_COMMON_FetchTxData(p_deviceProxy, dataLeng);
    }
}


void APP_FetchTxDataAll(void)
{
    uint8_t i;

    bt_shell_printf("loopback start for all\n");
    
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL)
        {
            APP_TIMER_SetTimer(APP_TIMER_FILE_FETCH, i, (void*)s_appFileTransList[i].p_deviceProxy, APP_TIMER_50MS*i);
        }
    }
}

void APP_PreparePatternData(uint8_t patternFileType)
{
    app_OpenPatternFile(s_appPatternFile[patternFileType]);
    bt_shell_printf("pattern file selection: %s\n", s_appPatternFile[patternFileType]);
    s_patternFileIndex = patternFileType;
}

static void app_ClearFileTransRecord(APP_FileTransList_T        * p_fileTrans, uint32_t rxDataSize)
{
    APP_DBP_BtDev_T * p_dev;

    p_dev = APP_DBP_GetDevInfoByProxy(p_fileTrans->p_deviceProxy);
    if (p_dev == NULL)
    {
        p_fileTrans->attMtu = BLE_ATT_DEFAULT_MTU_LEN;
    }
    else
    {
        p_fileTrans->attMtu = p_dev->attMtu;
    }
    
    p_fileTrans->txOffset = 0;
    p_fileTrans->rxOffset = 0;
    p_fileTrans->chunkSize = rxDataSize;
    p_fileTrans->chunkNumber = 1;
    p_fileTrans->rwChunkIndex = 0;

    p_fileTrans->testStage = APP_TEST_IDLE;
    if (p_fileTrans->p_dataBuf)
    {
        free(p_fileTrans->p_dataBuf);
    }
    if (p_fileTrans->p_lbTimer)
    {
        g_timer_stop(p_fileTrans->p_lbTimer);
    }

    if (rxDataSize)
        p_fileTrans->p_dataBuf = malloc(rxDataSize);
    else
        p_fileTrans->p_dataBuf = NULL;
}

void APP_LoopbackStart(uint8_t devIndex)
{
    APP_DBP_BtDev_T * p_dev;
    APP_FileTransList_T * p_fileTrans;
    APP_TRP_ConnList_T * p_trpConn;
    uint8_t i;
    uint8_t transIndex;
    
    p_dev = APP_DBP_GetDevInfoByIndex(devIndex);
    if (p_dev == NULL || p_dev->isConnected == false)
    {
        return;
    }
    
    p_fileTrans = app_GetFileTransList(p_dev->p_devProxy);
    if (p_fileTrans == NULL)
    {
        bt_shell_printf("p_fileTrans is NULL\n");
        return;
    }

    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL)
        {
            app_ClearFileTransRecord(&s_appFileTransList[i], s_patternDataSize);
        }
    }

    transIndex = APP_GetFileTransIndex(p_dev->p_devProxy);
    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_dev->p_devProxy);
    APP_TRPC_TransmitModeSwitch(s_bleWorkMode, p_trpConn);
    
    APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, transIndex, (void*)p_dev->p_devProxy, APP_TIMER_500MS);
}


void APP_UnidirectionModeStart(uint8_t devIndex)
{
    APP_DBP_BtDev_T * p_dev;
    APP_TRP_ConnList_T * p_trpConn;
    
    p_dev = APP_DBP_GetDevInfoByIndex(devIndex);
    if (p_dev == NULL)
        return;
    
    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_dev->p_devProxy);

    APP_TRPC_TransmitModeSwitch(s_bleWorkMode, p_trpConn);
    bt_shell_printf("%s start\n", s_appWorkModeDesc[s_bleWorkMode]);
}


void APP_BurstModeStart(uint8_t devIndex)
{
    APP_DBP_BtDev_T * p_dev;

    p_dev = APP_DBP_GetDevInfoByIndex(devIndex);

    if (p_dev == NULL)
    {
        bt_shell_printf("unknown device\n");
        return;
    }
    else if (p_dev->isConnected == false)
    {
        bt_shell_printf("device is not connected\n");
        return;
    }

    if (APP_GetWorkMode()==TRP_WMODE_LOOPBACK)
    {
        APP_LoopbackStart(devIndex);
    }
    else
    {
        APP_UnidirectionModeStart(devIndex);
    }
}


void APP_BurstModeStartAll(void)
{
    uint8_t i;
    APP_TRP_ConnList_T * p_trpConn;

#ifdef ENABLE_AUTO_RUN
    s_joinLinks = 0;
#endif

    printf("%s start\n", s_appWorkModeDesc[s_bleWorkMode]);

    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL)
        {
            if (s_bleWorkMode == TRP_WMODE_LOOPBACK)
            {
                app_ClearFileTransRecord(&s_appFileTransList[i], s_patternDataSize);
            }
                
            p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(s_appFileTransList[i].p_deviceProxy);
            APP_TRPC_TransmitModeSwitch(s_bleWorkMode, p_trpConn);
#ifdef ENABLE_AUTO_RUN
            s_joinLinks++;
#endif
        }
    }

    if (s_bleWorkMode == TRP_WMODE_LOOPBACK)
    {
        APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE_FOR_ALL, 0, NULL, APP_TIMER_500MS);
    }
}



void APP_SetWorkMode(uint8_t mode)
{
    if (mode >= TRP_WMODE_CHECK_SUM && mode <= TRP_WMODE_UART)
        s_bleWorkMode = mode;

    bt_shell_printf("set work mode = %s mode\n", s_appWorkModeDesc[s_bleWorkMode]);
}

uint8_t APP_GetWorkMode(void)
{
    return s_bleWorkMode;
}

bool APP_ConfirmWorkMode(DeviceProxy *p_devProxy)
{
    APP_TRP_ConnList_T * p_trpConn;
    
    p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    if (p_trpConn == NULL)
        return false;

    return (s_bleWorkMode == p_trpConn->workMode);
}

bool APP_ConfirmWorkModeAll(void)
{
    APP_TRP_ConnList_T * p_trpConn;
    uint8_t i;
    
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy != NULL)
        {

            p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(s_appFileTransList[i].p_deviceProxy);
            if (p_trpConn == NULL || 
                s_bleWorkMode != p_trpConn->workMode)
            {
                return false;
            }
        }
    }

    return true;
}


void APP_DeviceDisconnected(DeviceProxy * p_devProxy)
{
    APP_FileTransList_T * p_fileTrans;
    uint8_t transIndex;

    p_fileTrans = app_GetFileTransList(p_devProxy);
    if (p_fileTrans == NULL)
    {
        bt_shell_printf("p_fileTrans is NULL\n");
        return;
    }
        
    if (p_fileTrans->testStage == APP_TEST_PROGRESS)
    {
        bt_shell_printf("device disconnected while file was being transferred(Tx:%d/Rx:%d)\n", p_fileTrans->txOffset, p_fileTrans->rxOffset);
        
        transIndex = APP_GetFileTransIndex(p_devProxy);
        
        APP_TIMER_StopTimer(APP_TIMER_FILE_FETCH, transIndex);
        APP_TIMER_StopTimer(APP_TIMER_LOOPBACK_RX_CHECK, transIndex);

        app_SaveLoopbackDataToFile(p_devProxy);
        p_fileTrans->txOffset = 0;
        p_fileTrans->rxOffset = 0;
    }
    


    app_FreeFileTransList(p_devProxy);
}

void APP_DeviceConnected(DeviceProxy * p_devProxy)
{
    app_GetFileTransList(p_devProxy); //allocate
}

uint8_t APP_GetFileTransIndex(DeviceProxy * p_devProxy)
{
    uint8_t i;
    
    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_appFileTransList[i].p_deviceProxy == p_devProxy)
        {
            return i;
        }
    }

    return BLE_GAP_MAX_LINK_NBR;
}

void APP_SendRawData(APP_TRP_ConnList_T *p_trpConn, char * p_data)
{
    APP_FileTransList_T * p_fileTrans;
    uint8_t transIndex;
    
    if (p_trpConn == NULL || p_data == NULL)
    {
        printf("p_trpConn is NULL\n");
        return;
    }


    if (p_trpConn->trpRole != APP_TRP_UNKNOWN_ROLE)
    {
        p_fileTrans = app_GetFileTransList(p_trpConn->p_deviceProxy);
        if (p_fileTrans == NULL)
        {
            bt_shell_printf("p_fileTrans is NULL\n");
            return;
        }

        app_ClearFileTransRecord(p_fileTrans, strlen(p_data));
        p_fileTrans->rawDataSize = strlen(p_data);
        strcpy(p_fileTrans->p_dataBuf, p_data);
        
        APP_SetWorkMode(TRP_WMODE_UART);

        transIndex = APP_GetFileTransIndex(p_trpConn->p_deviceProxy);
        if (p_trpConn->workMode != TRP_WMODE_UART)
        {
            APP_TRPC_TransmitModeSwitch(TRP_WMODE_UART, p_trpConn);
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_500MS);
        }
        else
        {
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_1MS);
        }
    }
    else
    {
        bt_shell_printf("TRP is not established\n");
        return;
    }
}


void APP_SendRawDataFromFile(APP_TRP_ConnList_T *p_trpConn, char * p_filePath)
{
    APP_FileTransList_T * p_fileTrans;
    uint8_t transIndex;
    
    if (p_trpConn == NULL || p_filePath == NULL)
    {
        printf("p_trpConn is NULL\n");
        return;
    }

    if (p_trpConn->trpRole != APP_TRP_UNKNOWN_ROLE)
    {
        p_fileTrans = app_GetFileTransList(p_trpConn->p_deviceProxy);
        if (p_fileTrans == NULL)
        {
            bt_shell_printf("p_fileTrans is NULL\n");
            return;
        }

        app_ClearFileTransRecord(p_fileTrans, 0);
        app_OpenRawDataByChunk(p_fileTrans, p_filePath, RAW_DATA_BUFFER_SIZE);
        

        APP_SetWorkMode(TRP_WMODE_UART);

        transIndex = APP_GetFileTransIndex(p_trpConn->p_deviceProxy);
        if (p_trpConn->workMode != TRP_WMODE_UART)
        {
            APP_TRPC_TransmitModeSwitch(TRP_WMODE_UART, p_trpConn);
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_500MS);
        }
        else
        {
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_1MS);
        }
    }
    else
    {
        bt_shell_printf("TRP is not established\n");
        return;
    }
}

void APP_ReceiveRawDataToFile(APP_TRP_ConnList_T *p_trpConn, char * p_filePath)
{
    APP_FileTransList_T * p_fileTrans;
    uint8_t transIndex;

    
    if (p_trpConn == NULL)
    {
        bt_shell_printf("p_trpConn is NULL\n");
        return;
    }

    if (p_filePath == NULL && s_patternFileIndex == APP_PATTERN_FILE_TYPE_MAX)
    {
        bt_shell_printf("Please select pattern or specific output file name.");
        return;
    }

    if (p_trpConn->trpRole != APP_TRP_UNKNOWN_ROLE)
    {
        p_fileTrans = app_GetFileTransList(p_trpConn->p_deviceProxy);
        if (p_fileTrans == NULL)
        {
            bt_shell_printf("p_fileTrans is NULL\n");
            return;
        }

        app_ClearFileTransRecord(p_fileTrans, RAW_DATA_BUFFER_SIZE);

        if (p_fileTrans->p_rawDataFileName)
            free(p_fileTrans->p_rawDataFileName);

        if (p_filePath)
            p_fileTrans->p_rawDataFileName = strdup(p_filePath);
        else
            p_fileTrans->p_rawDataFileName = strdup(ANONYMOUS_OUTPUT_FILE_NAME);
        

        APP_SetWorkMode(TRP_WMODE_UART);

        transIndex = APP_GetFileTransIndex(p_trpConn->p_deviceProxy);
        if (p_trpConn->workMode != TRP_WMODE_UART)
        {
            APP_TRPC_TransmitModeSwitch(TRP_WMODE_UART, p_trpConn);
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE_ONLY, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_500MS);
        }
        else
        {
            APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE_ONLY, transIndex, p_trpConn->p_deviceProxy, APP_TIMER_1MS);
        }
    }
    else
    {
        bt_shell_printf("TRP is not established\n");
        return;
    }
}

uint32_t APP_RawDataRemaining(APP_TRP_ConnList_T *p_trpConn)
{
    APP_FileTransList_T * p_fileTrans;

    if (p_trpConn == NULL)
    {
        bt_shell_printf("p_trpConn is NULL\n");
        return 0;
    }

    p_fileTrans = app_GetFileTransList(p_trpConn->p_deviceProxy);
    if (p_fileTrans == NULL)
    {
        bt_shell_printf("p_fileTrans is NULL\n");
        return 0;
    }

    return p_fileTrans->rawDataSize - p_fileTrans->txOffset;
}


#ifdef ENABLE_AUTO_RUN
void APP_SetExecIterations(uint16_t runs)
{
    s_expectExecRuns = runs;
    s_currentExecRuns = 1;
}

void APP_GoNextRun(uint8_t countPass)
{
    if(s_currentExecRuns < s_expectExecRuns && countPass == s_joinLinks)
    {
        s_currentExecRuns++;
        APP_TIMER_SetTimer(APP_TIMER_AUTO_NEXT_RUN, 0, NULL, APP_TIMER_500MS);
    }
}

uint16_t APP_GetPassedRun(void)
{
    return s_currentExecRuns;
}

#endif

void APP_IoCapSetting(uint8_t ioCap)
{
    if (ioCap > BLE_SMP_IO_KEYBOARDDISPLAY)
    {
        bt_shell_printf("IO Capability parameter error");
        return;
    }
    
    APP_AGT_ChangeIoCap(APP_DBP_GetPairAgent(), ioCap);
}

void APP_ScSetting(uint8_t sc)
{
    if (sc > BLE_SMP_SC_ONLY)
    {
        bt_shell_printf("Secure Connection parameter error");
        return;
    }

    APP_MGMT_SetSecureConnection(sc);
}



void APP_Initialize(void)
{
    sp_patternData = NULL;
    s_patternDataSize = 0;
    s_bleWorkMode = TRP_WMODE_NULL;
    s_patternFileIndex = APP_PATTERN_FILE_TYPE_MAX;
    sp_currThread = NULL;
    sp_prevThread = NULL;

    APP_DBP_Init();
    APP_SM_Init();
    APP_SM_Handler(APP_SM_EVENT_POWER_ON);
    APP_InitConnList();
    APP_ADV_Init();
    APP_SCAN_Init();
    APP_MGMT_Init();
    BLE_TRSPS_EventRegister(APP_TRPS_EventHandler);
    BLE_TRSPC_EventRegister(APP_TRPC_EventHandler);

    APP_TRP_COMMON_Init();
    APP_TRPS_Init();
    APP_TRPC_Init();
#ifdef ENABLE_DATA_BUFFER_OVERFLOW_MONITOR
    app_HciEvtMonitor();
#endif
}


#define SHUTDOWN_SLEEP_1S  (1000000)
#define SHUTDOWN_TIMEOUT   (3)
static gpointer app_ShutdownThread(gpointer data)
{
    uint8_t timeout = SHUTDOWN_TIMEOUT;

    APP_SM_Handler(APP_SM_EVENT_POWER_OFF);
    APP_SM_Handler(APP_SM_EVENT_DISCONNECTED); //in case there is no connected device.

    while (APP_SM_GetSmState() != APP_SM_STATE_OFF)
    {
        printf("shutdown waiting(%d)\n", timeout);
        
        g_usleep (SHUTDOWN_SLEEP_1S);
        if (--timeout == 0)
            break;
    }

    g_thread_exit(NULL);

    return NULL;
}

void APP_Deinitialize(void)
{
    sp_shutdownThread = g_thread_new("shutdownThread", app_ShutdownThread, NULL);

    g_thread_join(sp_shutdownThread);

    APP_AGT_Unregister(APP_DBP_GetPairAgent());
    APP_MGMT_Deinit();
}


/*******************************************************************************
 End of File
 */
