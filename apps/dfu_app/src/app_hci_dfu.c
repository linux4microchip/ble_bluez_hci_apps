/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application HCI DFU Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.c

  Summary:
    This file contains application HCI DFU handler.

  Description:
    This file contains application HCI DFU handler.
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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/time.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"
#include "shared/shell.h"
#include "shared/timeout.h"

#include "application.h"
#include "app_error_defs.h"
#include "app_hci_dfu.h"
#include "app_hci_vnd.h"
#include "app_timer.h"





// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

static APP_DFU_OtauInfo_T s_otauInfo;


static const char *s_validationStatusStr[] = {
    "UNKNOWN",
    "Validation Success",
    "Validation Failed",
};

static const char *s_ExitStatusStr[] = {
    "DFU exited with controller reboot",
    "DFU exited only",
};




static uint16_t app_dfu_OpenOTAU(char *p_path)
{
    struct stat st;
    ssize_t len;
    int fd;


    fd = open((char*)p_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open OTAU file %s\n", p_path);
        return APP_RES_FAIL;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Failed to get OTAU file size\n");
        close(fd);
        return APP_RES_FAIL;
    }

    if (s_otauInfo.p_img)
    {
        free(s_otauInfo.p_img);
    }

    s_otauInfo.p_img = malloc(st.st_size);
    if (!s_otauInfo.p_img) {
        fprintf(stderr, "Failed to allocate OTAU file buffer\n");
        close(fd);
        return APP_RES_FAIL;
    }

    len = read(fd, s_otauInfo.p_img, st.st_size);
    if (len < 0) {
        fprintf(stderr, "Failed to read OTAU file\n");
        close(fd);
        return APP_RES_FAIL;
    }

    s_otauInfo.imgSize = st.st_size;
    s_otauInfo.headerVer = s_otauInfo.p_img[0];
    s_otauInfo.imgEnc = s_otauInfo.p_img[1];
    s_otauInfo.checkSum = s_otauInfo.p_img[2]<<8 | s_otauInfo.p_img[3];
    s_otauInfo.imgId = s_otauInfo.p_img[7]<<24 | s_otauInfo.p_img[6]<<16 | s_otauInfo.p_img[5]<<8 | s_otauInfo.p_img[4];
    s_otauInfo.imgRev = s_otauInfo.p_img[11]<<24 | s_otauInfo.p_img[10]<<16 | s_otauInfo.p_img[9]<<8 | s_otauInfo.p_img[8];
    s_otauInfo.fileType = s_otauInfo.p_img[12];
    s_otauInfo.crc16 = s_otauInfo.p_img[15]<<8 | s_otauInfo.p_img[14];
    

    close(fd);

    return APP_RES_SUCCESS;
}

static void app_dfu_freeOTAU(void)
{
    free(s_otauInfo.p_img);
    memset(&s_otauInfo, 0, sizeof(APP_DFU_OtauInfo_T));
}

gpointer APP_DFU_ProcessThread(gpointer data)
{
    int32_t hciDev, devDesc;
    APP_HCI_VndDfuRequestCmd_T dfuReqCp;
    APP_HCI_VndDfuRequestRsp_T dfuReqRp;
    APP_HCI_VndDfuDistCmd_T dfuDistCp;
    APP_HCI_VndDfuComplRsp_T dfuCplRp;
    APP_HCI_VndDfuExitRsp_T dfuExitRp;
    uint32_t remainLen = 0;
    uint8_t fragSize;
    uint32_t txOffset = 0;
    static GTimer *dfuTimer;
    gdouble elapseTime;
    bool useUartPath = false;
    APP_HCI_DfuParameters_T *p_dfuParam;

    p_dfuParam = (APP_HCI_DfuParameters_T *)data;

    if (p_dfuParam->p_ttyDevicePath == NULL)
    {
        hciDev = 0;
        devDesc = hci_open_dev(hciDev);
        if (devDesc < 0) {
            printf("Can't open device hci%d: %s (%d)\n", hciDev, strerror(errno), errno);
            goto failed;
        }
        else
        {
            APP_HCI_UseUartPath(false);
        }

    }
    else
    {
        printf("DFU via UART device:%s\n", p_dfuParam->p_ttyDevicePath);
        devDesc = APP_HCI_OpenUartDev(p_dfuParam->p_ttyDevicePath);
        if (devDesc < 0)
            goto failed;
        
        if (APP_HCI_UartConfig(devDesc, p_dfuParam->baudRate, UART_CONFIG_FLCTRL, UART_CONFIG_DATA_BIT, UART_CONFIG_STOP_BIT, UART_CONFIG_PARITY) < 0 )
            goto failed;
        
        APP_HCI_UseUartPath(true);
        useUartPath = true;
    }


    if(app_dfu_OpenOTAU(p_dfuParam->p_dfuFilePath) != APP_RES_SUCCESS)
        goto failed;

    dfuTimer = g_timer_new();
    g_timer_start(dfuTimer);

    if (APP_HCI_DfuEnable(devDesc, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
    {
        goto failed;
    }

    dfuReqCp.imgSize = s_otauInfo.imgSize;
    dfuReqCp.imgEnc = s_otauInfo.imgEnc;
    dfuReqCp.imgId = s_otauInfo.imgId;
    if (s_otauInfo.headerVer == 1)
    {
        dfuReqCp.imgSize -= APP_OTAU_HEADER_SIZE_V1;
        txOffset = APP_OTAU_HEADER_SIZE_V1;
    }
    else
    {
        dfuReqCp.imgSize -= APP_OTAU_HEADER_SIZE_V2;
        txOffset = APP_OTAU_HEADER_SIZE_V2;
    }

    remainLen = dfuReqCp.imgSize;
    
    if (APP_HCI_DfuRequest(devDesc, &dfuReqCp, &dfuReqRp, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
    {
        bt_shell_printf("DFU request failed\n");
        goto failed;
    }

    if (APP_HCI_DfuStart(devDesc, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
    {
        bt_shell_printf("DFU start failed\n");
        goto failed;
    }


    while(remainLen)
    {
        if (remainLen > dfuReqRp.imgFragSize)
            fragSize = dfuReqRp.imgFragSize;
        else
            fragSize = remainLen;


        memcpy(&dfuDistCp.payload[0], &s_otauInfo.p_img[txOffset], fragSize);
        if (APP_HCI_DfuDistribution(devDesc, &dfuDistCp, fragSize, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
        {
            bt_shell_printf("DFU distribution failed\n");
            goto failed;
        }

        remainLen -= fragSize;
        txOffset += fragSize;
        
        printf("Tx(%d/R:%d/T:%d)\n", fragSize, remainLen, dfuReqCp.imgSize);
    }

    
    if (APP_HCI_DfuComplete(devDesc, &dfuCplRp, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
        goto failed;

    bt_shell_printf("Validation result=%s\n", s_validationStatusStr[dfuCplRp.validationResult]);

    if (APP_HCI_DfuExit(devDesc, &dfuExitRp, HCI_VND_CMD_TIMEOUT_DEFAULT) != APP_RES_SUCCESS)
        goto failed;

    bt_shell_printf("Exited with status=%s\n", s_ExitStatusStr[dfuExitRp.exitStatus]);

    g_timer_stop(dfuTimer);
    elapseTime = g_timer_elapsed(dfuTimer, NULL);
    bt_shell_printf("Time elapsed: %f seconds\n", elapseTime);

    
    goto done;

failed:
    bt_shell_printf("DFU fail: %s (%d)\n", strerror(errno), errno);
done:
    if (useUartPath == false)
        hci_close_dev(devDesc);
    else
        APP_HCI_CloseUartDev(devDesc);

    app_dfu_freeOTAU();

    g_timer_destroy(dfuTimer);
    g_thread_exit(NULL);
    
    return NULL;

}




/*******************************************************************************
 End of File
 */
