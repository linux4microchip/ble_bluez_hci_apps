/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Common Function Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trp_common.c

  Summary:
    This file contains the Application Transparent common functions for this project.

  Description:
    This file contains the Application Transparent common functions for this project.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "application.h"
#include "app_ble_handler.h"
#include "app_error_defs.h"
#include "app_trps.h"
#include "app_trpc.h"
#include "app_log.h"
#include "app_trp_common.h"
#include "app_timer.h"

#include "shared/util.h"
#include "shared/shell.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************



const char * APP_TRP_TestStageStr[] = {
    "N/A",
    "Progress",
    "Passed",
    "Failed"
};


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_TRP_GenData_T        s_trpInputData[BLE_GAP_MAX_LINK_NBR];
static APP_TRP_ConnList_T       s_trpConnList[APP_TRP_MAX_LINK_NUMBER];
static uint8_t s_trpsChannelEn;
static APP_TRP_TYPE_T s_trpsType;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
static void app_trp_common_LinkClear(APP_TRP_ConnList_T *p_trpConn);
static uint16_t app_trp_common_SendLeData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data);


void APP_TRP_COMMON_Init(void)
{
    uint8_t i;

    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        app_trp_common_LinkClear(&s_trpConnList[i]);
    }

    memset((uint8_t *) &s_trpInputData, 0, BLE_GAP_MAX_LINK_NBR*sizeof(APP_TRP_GenData_T));
    s_trpsChannelEn = 0;
    s_trpsType = APP_TRP_TYPE_UNKNOWN;

}

bool APP_TRP_COMMON_CheckValidTopology(uint8_t trpRole)
{
    uint8_t i;

    if (trpRole == APP_TRP_CLIENT_ROLE)
        return true;

    if (trpRole == APP_TRP_UNKNOWN_ROLE)
        return false;
    
    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].connState == APP_TRP_STATE_CONNECTED &&
            s_trpConnList[i].workMode != TRP_WMODE_UART)
        {
            return false;
        }
    }

    return true;
}

APP_TRP_ConnList_T *APP_TRP_COMMON_GetConnListByDevProxy(DeviceProxy *p_devProxy)
{
    uint8_t index;

    for (index = 0; index < APP_TRP_MAX_LINK_NUMBER; index++)
    {
        if (s_trpConnList[index].p_deviceProxy == p_devProxy)
        {
            return &s_trpConnList[index];
        }
    }

    return NULL;
}

uint8_t APP_TRP_COMMON_GetConnIndex(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t index;

    for (index = 0; index < APP_TRP_MAX_LINK_NUMBER; index++)
    {
        if (&s_trpConnList[index] == p_trpConn)
        {
            return index;
        }
    }

    return APP_TRP_MAX_LINK_NUMBER;
}


static uint16_t app_trp_common_CheckCtrlChannel(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t result = APP_RES_FAIL;

    if ((p_trpConn->trpRole == APP_TRP_CLIENT_ROLE) || 
        ((p_trpConn->trpRole == APP_TRP_SERVER_ROLE) && (p_trpConn->channelEn & APP_TRP_CTRL_CHAN_ENABLE)))
        result = APP_RES_SUCCESS;

    return result;
}


void APP_TRP_COMMON_ConnEvtProc(DeviceProxy *p_devProxy, uint8_t gapRole)
{
    uint8_t i;

    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].connState == APP_TRP_STATE_IDLE)
        {
            s_trpConnList[i].connState = APP_TRP_STATE_CONNECTED;
            s_trpConnList[i].p_deviceProxy = p_devProxy;
            s_trpConnList[i].trpRole = (gapRole == BLE_GAP_ROLE_CENTRAL ? APP_TRP_CLIENT_ROLE : APP_TRP_SERVER_ROLE);
            if (gapRole == BLE_GAP_ROLE_PERIPHERAL)
            {
                s_trpConnList[i].workMode = TRP_WMODE_UART; //default mode is UART.
                s_trpConnList[i].channelEn = s_trpsChannelEn;
                s_trpConnList[i].type = s_trpsType;
            }
            break;
        }
    }
}

void APP_TRP_COMMON_DiscEvtProc(DeviceProxy *p_devProxy)
{
    APP_TRP_ConnList_T *p_trpConnLink = NULL;
    
    p_trpConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    app_trp_common_LinkClear(p_trpConnLink);
}

void APP_TRP_COMMON_UpdateMtu(DeviceProxy *p_devProxy, uint16_t exchangedMTU)
{
    APP_TRP_ConnList_T *p_trpConnLink = NULL;
    
    p_trpConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    
    if(p_trpConnLink != NULL)
    {
        p_trpConnLink->exchangedMTU = exchangedMTU;
        p_trpConnLink->txMTU = p_trpConnLink->exchangedMTU - ATT_HANDLE_VALUE_HEADER_SIZE;
    }
}

//All sessions share the same Control channel status in BlueZ host
void APP_TRP_COMMON_CtrlChOpenProc(bool isOpen)
{
    uint8_t i;

    if (isOpen)
    {
        s_trpsChannelEn |= APP_TRP_CTRL_CHAN_ENABLE;
    }
    else
    {
        s_trpsChannelEn &= APP_TRP_CTRL_CHAN_DISABLE;
    }

    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].trpRole == APP_TRP_SERVER_ROLE)
        {
            s_trpConnList[i].channelEn = s_trpsChannelEn;
        }
    }


}

//All sessions share the same Tx channel status in BlueZ host
void APP_TRP_COMMON_TxChOpenProc(bool isOpen)
{
    uint8_t i;

    if (isOpen)
    {
        s_trpsChannelEn |= APP_TRP_DATA_CHAN_ENABLE;
        s_trpsType = APP_TRP_TYPE_LEGACY;
    }
    else
    {
        s_trpsChannelEn &= APP_TRP_DATA_CHAN_DISABLE;
        s_trpsType = APP_TRP_TYPE_UNKNOWN;
    }

    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].trpRole == APP_TRP_SERVER_ROLE)
        {
            s_trpConnList[i].channelEn = s_trpsChannelEn;
            s_trpConnList[i].type = s_trpsType;
        }
    }
}

static uint16_t app_trp_common_SendLeData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data)
{
    uint16_t status = APP_RES_FAIL;

    if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
    {
        if ((p_trpConn->channelEn & APP_TRP_DATA_CHAN_ENABLE))
        {
            if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
            {
                status = APP_TRPS_LeTxData(p_trpConn, len, p_data);
            }
        }
    }
    else if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)  //Legacy TRP
        {
            status = APP_TRPC_LeTxData(p_trpConn, len, p_data);
        }
    }

    return status;
}

static void app_trp_common_LinkClear(APP_TRP_ConnList_T *p_trpConn)
{
    if (p_trpConn == NULL) return;

    if (p_trpConn->p_transTimer)
    {
        g_timer_destroy(p_trpConn->p_transTimer);
    }

    memset(p_trpConn, 0, sizeof(APP_TRP_ConnList_T));
    p_trpConn->exchangedMTU = BLE_ATT_DEFAULT_MTU_LEN;
    p_trpConn->txMTU = BLE_ATT_DEFAULT_MTU_LEN - ATT_HANDLE_VALUE_HEADER_SIZE;
    p_trpConn->p_transTimer = g_timer_new();

    APP_UTILITY_InitCircQueue(&(p_trpConn->uartCircQueue), APP_UTILITY_MAX_QUEUE_NUM);
    APP_UTILITY_InitCircQueue(&(p_trpConn->leCircQueue), APP_TRP_LE_MAX_QUEUE_NUM);
}


static APP_TRP_GenData_T * app_trp_common_GetInputData(DeviceProxy *p_devProxy)
{
    uint8_t bleLinkIdx;

    bleLinkIdx = APP_GetFileTransIndex(p_devProxy);
    if (bleLinkIdx == APP_BLE_MAX_LINK_NUMBER)
    {
        printf("ERROR, link index not found\n");
        return NULL;
    }
    
    return &s_trpInputData[bleLinkIdx];
}


static uint16_t app_trp_common_CheckCtrlRspFg(APP_TRP_ConnList_T *p_trpConn)
{
    if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE && p_trpConn->gattcRspWait)
        return APP_RES_FAIL;

    return APP_RES_SUCCESS;
}


void app_trp_common_SetCtrlRspFg(APP_TRP_ConnList_T *p_trpConn, uint16_t result, uint16_t flag)
{
    if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE && result == APP_RES_SUCCESS)
    {
        p_trpConn->gattcRspWait = flag;
    }
}


uint16_t app_trp_common_SendVendorCmd(APP_TRP_ConnList_T *p_trpConn, uint16_t length, uint8_t *p_payload)
{
    uint16_t result = APP_RES_FAIL;

    if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
    {
        if (p_trpConn->channelEn & APP_TRP_CTRL_CHAN_ENABLE)   //Legacy TRP
        {
            result = BLE_TRSPS_SendVendorCommand(p_trpConn->p_deviceProxy, APP_TRP_VENDOR_OPCODE_BLE_UART, length, p_payload);
        }
    }
    else if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)  //Legacy TRP
        {
            result = BLE_TRSPC_SendVendorCommand(p_trpConn->p_deviceProxy, APP_TRP_VENDOR_OPCODE_BLE_UART, length, p_payload);
        }
    }


    return result;
}

#define APP_TRP_WMODE_TX_MODE_PL_NUM        0x02
uint16_t APP_TRP_COMMON_SendModeCommand(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId, uint8_t commandId)
{
    uint8_t payload[APP_TRP_WMODE_TX_MODE_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL, rspFlag = APP_TRP_SEND_GID_UART_FAIL;
    
    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    
    if (result == APP_RES_SUCCESS)
    {
        if(app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;
        
        idx = 0;
        payload[idx++] = grpId;
        payload[idx] = commandId;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_TX_MODE_PL_NUM, payload);
        
        switch(grpId)
        {
            case TRP_GRPID_CHECK_SUM:
                rspFlag = APP_TRP_SEND_GID_CS_FAIL;
                break;

            case TRP_GRPID_FIX_PATTERN:
                rspFlag = APP_TRP_SEND_GID_FP_FAIL;
                break;

            case TRP_GRPID_LOOPBACK:
                rspFlag = APP_TRP_SEND_GID_LB_FAIL;
                break;

            case TRP_GRPID_UART:
                rspFlag = APP_TRP_SEND_GID_UART_FAIL;
                break;

            case TRP_GRPID_TRANSMIT:
                rspFlag = APP_TRP_SEND_GID_TX_FAIL;
                break;

            case TRP_GRPID_REV_LOOPBACK:
                rspFlag = APP_TRP_SEND_GID_REV_LB_FAIL;
                break;
            default:
                break;
        }
        app_trp_common_SetCtrlRspFg(p_trpConn, result, rspFlag);
    }
    
    return result;
}

#define APP_TRP_WMODE_TX_LENGTH_PL_NUM      0x06
uint16_t APP_TRP_COMMON_SendLengthCommand(APP_TRP_ConnList_T *p_trpConn, uint32_t length)
{
    uint8_t payload[APP_TRP_WMODE_TX_LENGTH_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL;

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    if (result == APP_RES_SUCCESS)
    {
        if(app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;

        idx = 0;
        payload[idx++] = TRP_GRPID_TRANSMIT;
        payload[idx++] = APP_TRP_WMODE_TX_DATA_LENGTH;
        payload[idx++] = (uint8_t)(length >> 24);
        payload[idx++] = (uint8_t)(length >> 16);
        payload[idx++] = (uint8_t)(length >> 8);
        payload[idx] = (uint8_t)length;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_TX_LENGTH_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_LENGTH_FAIL);
    }

    return result;
}

#define APP_TRP_WMODE_TX_TYPE_PL_NUM        0x03
uint16_t APP_TRP_COMMON_SendTypeCommand(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t payload[APP_TRP_WMODE_TX_TYPE_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL;

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    
    if (result == APP_RES_SUCCESS)
    {
        if (app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;
            
        idx = 0;
        payload[idx++] = TRP_GRPID_TRANSMIT;
        payload[idx++] = APP_TRP_WMODE_TX_TYPE;
        payload[idx] = p_trpConn->type;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_TX_TYPE_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_TYPE_FAIL);
    }

    return result;
}

#define APP_TRP_WMODE_CHECK_SUM_PL_NUM      0x03
uint16_t APP_TRP_COMMON_SendCheckSumCommand(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t payload[APP_TRP_WMODE_CHECK_SUM_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL;

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    if (result == APP_RES_SUCCESS)
    {
        if(app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;

        idx = 0;
        payload[idx++] = TRP_GRPID_CHECK_SUM;
        payload[idx++] = APP_TRP_WMODE_CHECK_SUM;
        payload[idx] = (uint8_t) p_trpConn->checkSum;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_CHECK_SUM_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_CHECK_SUM_FAIL);
    }

    return result;
}

#define APP_TRP_WMODE_FIX_PATTERN_PL_NUM    0x04
uint16_t APP_TRP_COMMON_SendLastNumber(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t payload[APP_TRP_WMODE_FIX_PATTERN_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL, lastNumber = 0;

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    if (result == APP_RES_SUCCESS)
    {
        if(app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;

        idx = 0;
        if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
            lastNumber = p_trpConn->lastNumber - 1;
        else
            lastNumber = p_trpConn->lastNumber;
        payload[idx++] = TRP_GRPID_FIX_PATTERN;
        payload[idx++] = APP_TRP_WMODE_TX_LAST_NUMBER;
        payload[idx++] = (uint8_t)(lastNumber >> 8);
        payload[idx] = (uint8_t)lastNumber;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_FIX_PATTERN_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_LAST_NUMBER_FAIL);
    }
    
    return result;
}

#define APP_TRP_WMODE_ERROR_PL_NUM          0x02
uint16_t APP_TRP_COMMON_SendErrorRsp(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId)
{
    uint8_t payload[APP_TRP_WMODE_ERROR_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL;

    printf("SendErrorRsp\n");

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    if (result == APP_RES_SUCCESS)
    {
        if (app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;
        
        idx = 0;
        payload[idx++] = grpId;
        payload[idx] = APP_TRP_WMODE_ERROR_RSP;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_ERROR_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_ERROR_RSP_FAIL);
    }
    
    return result;
}

#define APP_TRP_WMODE_NOTIFY_PL_NUM         0x03
uint16_t APP_TRP_COMMON_SendUpConnParaStatus(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId, uint8_t commandId, uint8_t upParaStatus)
{
    uint8_t payload[APP_TRP_WMODE_NOTIFY_PL_NUM], idx;
    uint16_t result = APP_RES_FAIL;

    result = app_trp_common_CheckCtrlChannel(p_trpConn);
    
    if (result == APP_RES_SUCCESS)
    {
        if(app_trp_common_CheckCtrlRspFg(p_trpConn))
            return APP_RES_FAIL;

        idx = 0;
        payload[idx++] = grpId;
        payload[idx++] = commandId;
        payload[idx] = upParaStatus;

        result = app_trp_common_SendVendorCmd(p_trpConn, APP_TRP_WMODE_NOTIFY_PL_NUM, payload);
        app_trp_common_SetCtrlRspFg(p_trpConn, result, APP_TRP_SEND_STATUS_FLAG);
    }
    
    return result;
}
    
uint16_t APP_TRP_COMMON_GetTrpDataLength(APP_TRP_ConnList_T *p_trpConn, uint16_t *p_dataLeng)
{
    uint16_t status = APP_RES_INVALID_PARA;
    
    if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
        {
            BLE_TRSPS_GetDataLength(p_trpConn->p_deviceProxy, p_dataLeng);
            status = APP_RES_SUCCESS;
        }
    }
    else if(p_trpConn->trpRole == APP_TRP_CLIENT_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
        {
            BLE_TRSPC_GetDataLength(p_trpConn->p_deviceProxy, p_dataLeng);
            status = APP_RES_SUCCESS;
        }
    }

    return status;
}

uint16_t APP_TRP_COMMON_GetTrpData(APP_TRP_ConnList_T *p_trpConn, uint8_t *p_data)
{
    uint16_t status = APP_RES_FAIL;

    if (p_data == NULL)
        return status;
    
    if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
        {
            status = BLE_TRSPS_GetData(p_trpConn->p_deviceProxy, p_data);
        }
    }
    else if(p_trpConn->trpRole == APP_TRP_CLIENT_ROLE)
    {
        if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
        {
            status = BLE_TRSPC_GetData(p_trpConn->p_deviceProxy, p_data);
        }
    }
    
    return status;
}

uint16_t APP_TRP_COMMON_FreeLeData(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t dataLeng = 0, status = APP_RES_SUCCESS;
    uint8_t *p_data;

    status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
    if (status != APP_RES_SUCCESS)
        return status;
    if (dataLeng > 0)
    {
        p_data = malloc(dataLeng);
        if (p_data != NULL)
        {
            status = APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
            free(p_data);
        }
        else
        {
            status = APP_RES_OOM;
        }
    }

    return status;
}

void APP_TRP_COMMON_DelAllCircData(APP_UTILITY_CircQueue_T *p_circQueue)
{
    uint8_t i;

    if (p_circQueue->usedNum > 0)
    {
        for (i = 0; i < p_circQueue->usedNum; i++)
            APP_UTILITY_FreeElemCircQueue(p_circQueue);
    }
}

void APP_TRP_COMMON_DelAllLeCircData(APP_UTILITY_CircQueue_T *p_circQueue)
{
    uint8_t i;

    if (p_circQueue->usedNum > 0)
    {
        for (i = 0; i < p_circQueue->usedNum; i++)
            //APP_TRP_COMMON_FreeElemLeCircQueue(p_circQueue);
            APP_UTILITY_FreeElemCircQueue(p_circQueue);
    }
}

uint32_t APP_TRP_COMMON_CalculateCheckSum(uint32_t checkSum, uint32_t *p_dataLeng, APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t tmpLeng, status;
    uint32_t i;
    uint8_t *p_data = NULL;

    if (((*p_dataLeng) == 0) || (p_trpConn == NULL))
        return checkSum;

    status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &tmpLeng);
    if (status != APP_RES_SUCCESS)
        return checkSum;

    while (tmpLeng > 0)
    {
        p_data = malloc(tmpLeng);
        if (p_data != NULL)
        {
            status = APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
            if (status == APP_RES_SUCCESS)
            {
                for (i = 0; i < tmpLeng; i++)
                    checkSum += p_data[i];
                if ((*p_dataLeng) > tmpLeng)
                    *p_dataLeng -= tmpLeng;
                else
                    *p_dataLeng = 0;
            }
            free(p_data);
            if ((*p_dataLeng) == 0)
                break;
            status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &tmpLeng);
            if (status != APP_RES_SUCCESS)
                return checkSum;
        }
        else
            return checkSum;
    }

    //Start a timer then reset the timer every time the device receives the data
    //If timeout occurs, send out current checksum directly.
    if (*p_dataLeng != 0)
    {
        APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);
    }

    return checkSum;
}

uint8_t * APP_TRP_COMMON_GenFixPattern(uint16_t *p_startSeqNum, uint16_t *p_patternLeng,
    uint32_t *p_pattMaxSize, uint32_t *p_checkSum)
{
    uint8_t *p_data, *p_buf;
    uint16_t i;
    
    if ((*p_patternLeng == 0) || (*p_patternLeng == 1) || (*p_pattMaxSize == 0))
        return NULL;
    
    *p_patternLeng = *p_patternLeng & ~0x01;  // even length
    p_data = malloc(*p_patternLeng);
    
    if (p_data == NULL)
        return p_data;
    
    p_buf = p_data;
    i = 0;
    
    while (i < *p_patternLeng)
    {
        //U16_TO_STREAM_BE(&p_buf, *startSeqNum);
        put_be16(*p_startSeqNum, p_buf);
        (*p_startSeqNum)++;
        i += 2;
        p_buf += 2;
    }
    
    if (*p_pattMaxSize > *p_patternLeng)
        *p_pattMaxSize -= *p_patternLeng;
    else
        *p_pattMaxSize = 0;
    
    for (i = 0; i < *p_patternLeng; i++)
        *p_checkSum += p_data[i];

    return p_data;
}

uint16_t APP_TRP_COMMON_UpdateFixPatternLen(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t patternLeng = 0;

    if (p_trpConn->type == APP_TRP_TYPE_LEGACY)   //Legacy TRPS
    {
        if (p_trpConn->fixPattMaxSize > p_trpConn->txMTU)
        {
            patternLeng = p_trpConn->txMTU;
        }
        else
        {
            patternLeng = p_trpConn->fixPattMaxSize;
        }
    }
    else if (p_trpConn->channelEn & APP_TRCBP_DATA_CHAN_ENABLE)       //TRCBPS
    {
        if (p_trpConn->fixPattMaxSize > p_trpConn->fixPattTrcbpMtu)
        {
            patternLeng = p_trpConn->fixPattTrcbpMtu;
        }
        else
        {
            patternLeng = p_trpConn->fixPattMaxSize;
        }
    }

    return patternLeng;
}

void APP_TRP_COMMON_InitFixPatternParam(APP_TRP_ConnList_T *p_trpConn)
{
    p_trpConn->fixPattMaxSize = APP_TRP_WMODE_TX_MAX_SIZE;
    p_trpConn->lastNumber = 0;
    p_trpConn->rxLastNunber = 0;
    p_trpConn->checkSum = 0;
}

uint16_t APP_TRP_COMMON_SendFixPatternFirstPkt(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t *p_trsBuf = NULL;
    uint16_t status = APP_RES_SUCCESS, dataLength = 0;
                    
    if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
    {
        p_trsBuf = APP_TRP_COMMON_GenFixPattern(&(p_trpConn->lastNumber), &(p_trpConn->txMTU), 
            &(p_trpConn->fixPattMaxSize), &(p_trpConn->checkSum));
    }
    else if (p_trpConn->channelEn & APP_TRCBP_DATA_CHAN_ENABLE)
    {
        p_trsBuf = APP_TRP_COMMON_GenFixPattern(&(p_trpConn->lastNumber), &(p_trpConn->fixPattTrcbpMtu), 
            &(p_trpConn->fixPattMaxSize), &(p_trpConn->checkSum));
    }

    if (p_trsBuf == NULL)
        return APP_RES_OOM;

    if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
    {
        dataLength = p_trpConn->txMTU;
    }
    else if (p_trpConn->channelEn & APP_TRCBP_DATA_CHAN_ENABLE)
    {
        dataLength = p_trpConn->fixPattTrcbpMtu;
    }
    
    status = app_trp_common_SendLeData(p_trpConn, dataLength, p_trsBuf);
    
    if (status != APP_RES_SUCCESS)
    {
        p_trpConn->lastNumber = 0;
        p_trpConn->fixPattMaxSize = APP_TRP_WMODE_TX_MAX_SIZE;
    }

    free(p_trsBuf);

    return status;
}

uint16_t APP_TRP_COMMON_SendFixPattern(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t *p_data, validNum = APP_TRP_MAX_TRANSMIT_NUM;
    uint16_t status = APP_RES_FAIL, patternLeng;
    uint16_t lastNum;
    uint32_t leftSize, lastCheckSum;

    patternLeng = APP_TRP_COMMON_UpdateFixPatternLen(p_trpConn);

    lastNum = p_trpConn->lastNumber;
    leftSize = p_trpConn->fixPattMaxSize;
    lastCheckSum = p_trpConn->checkSum;

    p_data = APP_TRP_COMMON_GenFixPattern(&(p_trpConn->lastNumber), &(patternLeng), &(p_trpConn->fixPattMaxSize),
        &(p_trpConn->checkSum));

    if (p_data == NULL)
        return APP_RES_OOM;

    while (p_data != NULL)
    {
        status = app_trp_common_SendLeData(p_trpConn, patternLeng, p_data);

        free(p_data);

        if (status == APP_RES_SUCCESS)
        {
            validNum--;
            if (p_trpConn->fixPattMaxSize == 0)
            {
                status |= APP_RES_COMPLETE;
                return status;
            }
            else
            {
                if (validNum > 0) // Limit transmit number
                {
                    patternLeng = APP_TRP_COMMON_UpdateFixPatternLen(p_trpConn);

                    lastNum = p_trpConn->lastNumber;
                    leftSize = p_trpConn->fixPattMaxSize;
                    lastCheckSum = p_trpConn->checkSum;

                    p_data = APP_TRP_COMMON_GenFixPattern(&(p_trpConn->lastNumber), &(patternLeng), 
                        &(p_trpConn->fixPattMaxSize), &(p_trpConn->checkSum));

                    if (p_data == NULL)
                        return APP_RES_OOM;
                }
                else
                {
                    status = APP_RES_SUCCESS;
                    break;
                }
            }
        }
        else
        {
            p_trpConn->lastNumber = lastNum;
            p_trpConn->fixPattMaxSize = leftSize;
            p_trpConn->checkSum = lastCheckSum;

            break;
        }
    }
    
    return status;
}

uint16_t APP_TRP_COMMON_SendMultiLinkFixPattern(APP_TRP_TrafficPriority_T *p_connToken, APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t *p_data;
    uint16_t status = APP_RES_FAIL, patternLeng;
    uint16_t lastNum;
    uint32_t leftSize, lastCheckSum;
    
    if ((p_connToken == NULL) || (p_trpConn == NULL))
        return APP_RES_INVALID_PARA;
    
    patternLeng = APP_TRP_COMMON_UpdateFixPatternLen(p_trpConn);
    lastNum = p_trpConn->lastNumber;
    leftSize = p_trpConn->fixPattMaxSize;
    lastCheckSum = p_trpConn->checkSum;

    p_data = APP_TRP_COMMON_GenFixPattern(&(p_trpConn->lastNumber), &(patternLeng), &(p_trpConn->fixPattMaxSize), &(p_trpConn->checkSum));

    if (p_data != NULL)
    {
        status = app_trp_common_SendLeData(p_trpConn, patternLeng, p_data);

        free(p_data);

        if (status == APP_RES_SUCCESS)
        {
            p_connToken->validNumber--;
            p_trpConn->maxAvailTxNumber--;
            
            if (p_trpConn->fixPattMaxSize == 0)
            {
                status |= APP_RES_COMPLETE;
                return status;
            }
        }
        else
        {
            p_trpConn->lastNumber = lastNum;
            p_trpConn->fixPattMaxSize = leftSize;
            p_trpConn->checkSum = lastCheckSum;
        }
    }
    else
        return APP_RES_OOM;
    
    return status;
}

uint16_t APP_TRP_COMMON_CheckFixPatternData(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t tmpLeng, status = APP_RES_SUCCESS, i, fixPatternData;
    uint8_t *p_data = NULL;

    if (p_trpConn == NULL)
        return APP_RES_INVALID_PARA;

    status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &tmpLeng);
    
    if (status != APP_RES_SUCCESS)
        return status;

    while (tmpLeng > 0)
    {
        p_trpConn->rxAccuLeng += tmpLeng;
        p_data = malloc(tmpLeng);
        
        if (p_data != NULL)
        {
            status = APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
            
            if (status == APP_RES_SUCCESS)
            {
                for (i = 0; i < tmpLeng; i += 2)
                {
                    fixPatternData = p_data[i];
                    fixPatternData = (fixPatternData << 8) | p_data[i+1];
                    
                    if (p_trpConn->rxLastNunber != fixPatternData)
                    {
                        printf("number mismatch[%04x, %04x]\n", p_trpConn->rxLastNunber, fixPatternData);
                        status = APP_RES_FAIL;
                        break;
                    }
                    else
                        (p_trpConn->rxLastNunber)++;
                }
            }
            
            free(p_data);

            if (status != APP_RES_SUCCESS)
                break;
            
            status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &tmpLeng);
            
            if (status != APP_RES_SUCCESS)
                return status;
        }
        else
            return APP_RES_OOM;
    }

    return status;
}


uint16_t APP_TRP_COMMON_SendLeDataToFile(APP_TRP_ConnList_T *p_trpConn, uint16_t dataLeng, uint8_t *p_rxBuf)
{
    uint16_t status = APP_RES_INVALID_PARA;

    if ((dataLeng > 0) && (p_rxBuf != NULL))
    {
        if (p_trpConn->workMode == TRP_WMODE_LOOPBACK)
        {
            status = APP_FileWrite(p_trpConn->p_deviceProxy, dataLeng, p_rxBuf);
        }
        else if (p_trpConn->workMode == TRP_WMODE_UART)
        {
            status = APP_ConsoleWrite(p_trpConn->p_deviceProxy, dataLeng, p_rxBuf);
        }
    }

    return status;
}


void APP_TRP_COMMON_SendTrpProfileDataToUART(APP_TRP_ConnList_T *p_trpConn)
{
    APP_UTILITY_CircQueue_T *p_leCircQueue = &(p_trpConn->leCircQueue);
    APP_UTILITY_QueueElem_T *p_queueElem = NULL;
    uint16_t status = APP_RES_SUCCESS, dataLeng = 0;
    uint8_t validNum = APP_TRP_MAX_TRANSMIT_NUM, *p_data = NULL;
    uint8_t trpIdx;

    if (p_trpConn == NULL)
        return;

    trpIdx = APP_TRP_COMMON_GetConnIndex(p_trpConn);
        
    while(validNum > 0)
    {
        p_queueElem = APP_UTILITY_GetElemCircQueue(p_leCircQueue);
        
        if (p_queueElem != NULL)
        {
            if ((p_queueElem->dataLeng != 0) && (p_queueElem->p_data != NULL))
            {
                status = APP_TRP_COMMON_SendLeDataToFile(p_trpConn, p_queueElem->dataLeng, p_queueElem->p_data);
                if (status == APP_RES_SUCCESS)
                {
                    APP_UTILITY_FreeElemCircQueue(p_leCircQueue);
                    validNum--;
                }
                else
                {
                    // Set a timer to retransmit it.
                    APP_TIMER_SetTimer(APP_TIMER_UART_SEND, trpIdx, (void *)p_trpConn, APP_TIMER_1MS);

                    return;
                }
            }
            else
            {
                APP_UTILITY_FreeElemCircQueue(p_leCircQueue);
                validNum--;
            }
        }
        else
        {
            status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
            
            if (status == APP_RES_SUCCESS)
            {
                if (dataLeng)
                {
                    p_data = malloc(dataLeng);
                    
                    if (p_data != NULL)
                    {
                        status = APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
                        
                        if (status == APP_RES_SUCCESS)
                        {
                            status = APP_TRP_COMMON_SendLeDataToFile(p_trpConn, dataLeng, p_data);

                            if (status == APP_RES_SUCCESS)
                            {
                                free(p_data);
                                validNum--;
                            }
                            else
                            {
                                if (status == APP_RES_FAIL)
                                {
                                    if (APP_UTILITY_GetValidCircQueueNum(p_leCircQueue) > 0)
                                    {
                                        APP_UTILITY_InsertDataToCircQueue(dataLeng, p_data, p_leCircQueue);
                                        status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
                                        if (status == APP_RES_SUCCESS)
                                        {
                                            if (dataLeng)
                                            {
                                                // Set a timer to retransmit.
                                                APP_TIMER_SetTimer(APP_TIMER_UART_SEND, trpIdx, (void *)p_trpConn, APP_TIMER_18MS);
                                            }
                                            else
                                            {
                                                APP_TIMER_SetTimer(APP_TIMER_FETCH_TRP_RX_DATA, trpIdx, (void *)p_trpConn, APP_TIMER_18MS);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        free(p_data);
                                    }
                                }
                                else
                                {
                                    free(p_data);
                                }
                                return;
                            }
                        }
                        else
                        {
                            free(p_data);
                            validNum--;
                        }
                    }
                    else
                        return;
                }
                else
                    return;
            }
            else
                return;
        }
    }
    status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
    if ((APP_UTILITY_GetValidCircQueueNum(p_leCircQueue) > 0) || ((status == APP_RES_SUCCESS) 
        && (dataLeng > 0)))
    {
        // Set a timer to retransmit.
        APP_TIMER_SetTimer(APP_TIMER_UART_SEND, trpIdx, (void *)p_trpConn, APP_TIMER_18MS);
    }
}

uint16_t APP_TRP_COMMON_InsertUartDataToCircQueue(APP_TRP_ConnList_T *p_trpConn, 
    APP_TRP_GenData_T *p_rxData)
{
    uint16_t status = APP_RES_SUCCESS;

    status = APP_UTILITY_InsertDataToCircQueue(p_rxData->srcOffset, p_rxData->p_srcData, &(p_trpConn->uartCircQueue));
    if (status == APP_RES_NO_RESOURCE)
    {
        return APP_RES_NO_RESOURCE;
    }
    else
    {
        if ((status == APP_RES_INVALID_PARA) && (p_rxData->p_srcData != NULL))
            free(p_rxData->p_srcData);
        
        p_rxData->p_srcData = NULL;
        p_rxData->srcOffset = 0;
    }

    return status;
}

uint16_t APP_TRP_COMMON_CopyUartRxData(APP_TRP_ConnList_T *p_trpConn, APP_TRP_GenData_T *p_rxData)
{ 
    uint16_t status = APP_RES_SUCCESS, readLeng, dataLeng;
    bool insertDataFlag;

    //printf("CopyUartRxData(%d,%d,%d)\n", p_rxData->srcOffset, p_rxData->rxLeng, p_trpConn->lePktLeng);
    
    if ((p_rxData->srcOffset + p_rxData->rxLeng) >= p_trpConn->lePktLeng)
    {
        insertDataFlag = true;
        dataLeng = p_trpConn->lePktLeng - p_rxData->srcOffset;
    }
    else if (p_rxData->rxLeng > 0)
    {
        insertDataFlag = true;
        dataLeng = p_rxData->rxLeng;
    }
    else
    {
        insertDataFlag = false;
    }
    
    if (p_trpConn->workMode == TRP_WMODE_LOOPBACK)
    {
        readLeng = APP_FileRead(p_trpConn->p_deviceProxy, p_rxData->p_srcData + p_rxData->srcOffset, dataLeng);
    }
    else if (p_trpConn->workMode == TRP_WMODE_UART)
    {
        readLeng = APP_ConsoleRead(p_trpConn->p_deviceProxy, p_rxData->p_srcData + p_rxData->srcOffset, dataLeng);
    }
    else
    {
        return APP_RES_FAIL;
    }
    
    p_rxData->rxLeng -= readLeng;
    p_rxData->srcOffset += readLeng;

    if (insertDataFlag)
    {
        status = APP_TRP_COMMON_InsertUartDataToCircQueue(p_trpConn, p_rxData);
    }

    return status;
}

uint16_t APP_TRP_COMMON_SendLeDataUartCircQueue(APP_TRP_ConnList_T *p_trpConn)
{
    APP_UTILITY_CircQueue_T *p_circQueue;
    APP_UTILITY_QueueElem_T *p_queueElem;
    uint16_t status = APP_RES_SUCCESS;
    uint8_t validNum = APP_TRP_MAX_TRANSMIT_NUM;

    p_circQueue = &(p_trpConn->uartCircQueue);
    
    p_queueElem = APP_UTILITY_GetElemCircQueue(p_circQueue);
    while (p_queueElem != NULL)
    {
        status = app_trp_common_SendLeData(p_trpConn, p_queueElem->dataLeng, 
            p_queueElem->p_data);

        if (status == APP_RES_SUCCESS)
        {
            validNum--;
            APP_UTILITY_FreeElemCircQueue(p_circQueue);
            if (validNum > 0)   // limit transmit number.
                p_queueElem = APP_UTILITY_GetElemCircQueue(p_circQueue);
            else
            {
                status = APP_RES_SUCCESS;
                break;
            }
        }
        else
        {
            if ((status == APP_RES_NO_RESOURCE) || (status == APP_RES_OOM) || (status == APP_RES_BUSY))
            {
                //APP_LOG_ERROR("LE Tx error: status=%d\n", status);
                break;
            }
            else
            {
                APP_UTILITY_FreeElemCircQueue(p_circQueue);
                validNum--;
                if (validNum > 0)   // limit transmit number.
                    p_queueElem = APP_UTILITY_GetElemCircQueue(p_circQueue);
                else
                {
                    status = APP_RES_SUCCESS;
                    break;
                }
            }
        }
    }
            
    return status;
}

uint16_t APP_TRP_COMMON_SendMultiLinkLeDataTrpProfile(APP_TRP_TrafficPriority_T *p_connToken, APP_TRP_ConnList_T *p_trpConn)
{
    APP_UTILITY_CircQueue_T *p_leCircQueue;
    APP_UTILITY_QueueElem_T *p_queueElem;
    uint16_t status = APP_RES_SUCCESS, dataLeng = 0;
    uint8_t *p_data = NULL;
    bool toTree = false;


    if ((p_connToken == NULL) || (p_trpConn == NULL))
        return APP_RES_INVALID_PARA;
    
    p_leCircQueue = &(p_trpConn->leCircQueue);
    p_queueElem = APP_UTILITY_GetElemCircQueue(p_leCircQueue);
    
    if (p_queueElem != NULL)
    {
        status = app_trp_common_SendLeData(p_trpConn, p_queueElem->dataLeng, p_queueElem->p_data);

        if (status == APP_RES_SUCCESS)
        {
            p_connToken->validNumber--;
            p_trpConn->maxAvailTxNumber--;
            APP_UTILITY_FreeElemCircQueue(p_leCircQueue);
        }
        else
        {
            if ((status == APP_RES_NO_RESOURCE) || (status == APP_RES_OOM) || (status == APP_RES_BUSY))
            {
                APP_LOG_ERROR("LE Tx err1(%x)\n", status);
            }
            else
            {
                p_connToken->validNumber--;
                p_trpConn->maxAvailTxNumber--;
                APP_UTILITY_FreeElemCircQueue(p_leCircQueue);
                APP_LOG_ERROR("LE Tx err1(%x,1)\n", status);
            }
        }
    }
    else
    {
        status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
        
        if (dataLeng)
        {
            p_data = malloc(dataLeng);
            
            if (p_data != NULL)
            {
                status = APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
                
                if (status == APP_RES_SUCCESS)
                {
                    status = app_trp_common_SendLeData(p_trpConn, dataLeng, p_data);

                    if (status == APP_RES_SUCCESS)
                    {
                        p_connToken->validNumber--;
                        p_trpConn->maxAvailTxNumber--;
                        free(p_data);
                    }
                    else
                    {            
                        if ((status == APP_RES_NO_RESOURCE) || (status == APP_RES_OOM) || (status == APP_RES_BUSY))
                        { 
                            if (APP_UTILITY_GetValidCircQueueNum(p_leCircQueue) > 0)
                            {
                                APP_UTILITY_InsertDataToCircQueue(dataLeng, p_data, p_leCircQueue);
                                toTree = false;
                            }
                            else
                            {
                                p_connToken->validNumber--;
                                p_trpConn->maxAvailTxNumber--;
                                free(p_data);
                                toTree = true;
                            }
                        }
                        else
                        {
                            p_connToken->validNumber--;
                            p_trpConn->maxAvailTxNumber--;
                            free(p_data);
                            toTree = true;
                        }
                        APP_LOG_ERROR("LE Tx err2(0x%x,%d)\n", status, toTree);
                    }
                }
                else
                {
                    //can't find current link or there is no data in the queue.
                    free(p_data);
                    p_trpConn->maxAvailTxNumber = 0; //change link
                }
            }
            else
            {
                //can't allocate memory.
                return APP_RES_OOM;
            }
        }
        else
        {
            //get data length = 0.
            //p_trpConn->maxAvailTxNumber = 0; //change link
            status = APP_RES_NO_RESOURCE;
        }
    }
    
    return status;
}

uint16_t APP_TRP_COMMON_UartRxData(APP_TRP_GenData_T *p_rxData, APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t status = APP_RES_SUCCESS;
    uint8_t validNum = APP_TRP_MAX_TRANSMIT_NUM;

    // Send data to transparent profile
    if (p_trpConn->uartCircQueue.usedNum > 0)
    {
        APP_TRP_COMMON_SendLeDataUartCircQueue(p_trpConn);
    }
    
    if (p_trpConn->lePktLeng == 0)
    {
        if (p_trpConn->txMTU > 0)
            p_trpConn->lePktLeng = p_trpConn->txMTU;
        else
            p_trpConn->lePktLeng = BLE_ATT_MAX_MTU_LEN - ATT_HANDLE_VALUE_HEADER_SIZE;
    }
    
    while (p_rxData->rxLeng > 0)
    {
        if (p_rxData->p_srcData != NULL)
        {
            status = APP_TRP_COMMON_CopyUartRxData(p_trpConn, p_rxData);
            if (status != APP_RES_SUCCESS)
            {
                break;
            }
        }
        else
        {
            validNum--;
            if (validNum > 0)   // Limit transmission number
            {
                // Get Rx buffer.
                p_rxData->p_srcData = malloc(p_trpConn->lePktLeng);
                if (p_rxData->p_srcData == NULL)
                {
                    status = APP_RES_OOM;
                    break;
                } 
                p_rxData->srcOffset = 0;
                status = APP_TRP_COMMON_CopyUartRxData(p_trpConn, p_rxData);
                if (status != APP_RES_SUCCESS)
                {
                    break;
                }
            }
            else
            {
                 status = APP_RES_SUCCESS;
                 break;
            }
        }
    }
    
    // Send data to transparent profile
    if (p_trpConn->uartCircQueue.usedNum > 0)
    {
        status = APP_TRP_COMMON_SendLeDataUartCircQueue(p_trpConn);
    }

    return status;
}


void APP_TRP_COMMON_FetchTxData(DeviceProxy *p_devProxy, uint16_t dataLeng)
{
    APP_TRP_ConnList_T *p_connList = NULL;
    uint16_t status;
    APP_TRP_GenData_T *p_genData = NULL;
    
    if (p_devProxy == NULL)
        return;

    p_genData = app_trp_common_GetInputData(p_devProxy);
    if (p_genData == NULL)
        return;

    p_connList = APP_TRP_COMMON_GetConnListByDevProxy(p_devProxy);
    
    p_genData->rxLeng = dataLeng;
    //p_connList->noUartRxCnt = 0;
    status = APP_TRP_COMMON_UartRxData(p_genData, p_connList);
    
    if (status != APP_RES_SUCCESS)
    {
        //printf("Fetch(%d)\n", status);
    }
}


APP_TRP_ConnList_T *APP_TRP_COMMON_GetConnListByIndex(uint8_t index)
{
    if (index < APP_TRPC_MAX_LINK_NUMBER)
    {
        if (s_trpConnList[index].connState != APP_TRP_STATE_IDLE)
            return &s_trpConnList[index];
    }
    
    return NULL;
}


void APP_TRP_COMMON_AssignToken(APP_TRP_ConnList_T *p_trpConn, APP_TRP_LINK_TYPE_T linkType, APP_TRP_TrafficPriority_T *p_connToken)
{
    uint8_t i;
    
    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (p_trpConn == &s_trpConnList[i])
        {
            if (linkType == APP_TRP_LINK_TYPE_TX)
            {
                p_connToken->txToken = i;
            }
            else if (linkType == APP_TRP_LINK_TYPE_RX)
            {
                p_connToken->rxToken = i;
            }
            return;
        }
    }
}

APP_TRP_ConnList_T *APP_TRP_COMMON_ChangeNextLink(uint8_t trpRole, APP_TRP_LINK_TYPE_T linkType, APP_TRP_TrafficPriority_T *p_connToken)
{
    uint8_t i = 0, index = 0;
    
    if (p_connToken == NULL)
        return NULL;
    
    if (linkType == APP_TRP_LINK_TYPE_TX)
    {
        index = p_connToken->txToken;
        
        for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
        {
            index++;
            
            if (index > (APP_TRP_MAX_LINK_NUMBER - 1))
                index = 0;
            
            if (s_trpConnList[index].connState != APP_TRP_STATE_IDLE && s_trpConnList[index].trpRole == trpRole)
                break;
        }
        
        p_connToken->txToken = index;

        if (s_trpConnList[index].connState == APP_TRP_STATE_IDLE)
        {
            return NULL;
        }
        else
        {
            return &(s_trpConnList[index]);
        }
        
    }
    else if (linkType == APP_TRP_LINK_TYPE_RX)
    {
        index = p_connToken->rxToken;
        
        for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
        {
            index++;
            
            if (index > (APP_TRP_MAX_LINK_NUMBER - 1))
                index = 0;
            
            if (s_trpConnList[index].connState != APP_TRP_STATE_IDLE)
                break;
        }
        
        p_connToken->rxToken = index;

        if (s_trpConnList[index].connState == APP_TRP_STATE_IDLE && s_trpConnList[index].trpRole == trpRole)
        {
            return NULL;
        }
        else
        {
            return &(s_trpConnList[index]);
        }
        
    }

    return NULL;
}

bool APP_TRP_COMMON_IsWorkModeExist(uint8_t trpRole, APP_TRP_WMODE_T workMode)
{
    uint8_t i;
    
    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].workMode == workMode && s_trpConnList[i].trpRole == trpRole)
            return true;
    }

    return false;
}


uint8_t APP_TRP_COMMON_GetRoleNum(uint8_t gapRole)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < APP_TRP_MAX_LINK_NUMBER; i++)
    {
        if (s_trpConnList[i].connState == APP_TRP_STATE_CONNECTED)
        {
            if (s_trpConnList[i].trpRole == gapRole)
            {
                count++;
            }
        }
    }

    return count;
}

#define APP_TRP_WM_LOOPBACK_STR         "Loopback"
#define APP_TRP_WM_CHECKSUM_STR         "Checksum"
#define APP_TRP_WM_FIXPATTERN_STR       "Fixed-pattern"
#define APP_TRP_WM_PROGRESS_STR         "progressing"
#define APP_TRP_WM_START_STR            "start"

void APP_TRP_COMMON_StartLog(APP_TRP_ConnList_T *p_trpConn)
{
    if (p_trpConn == NULL)
        return;

    p_trpConn->progress = 0;
    p_trpConn->testStage = APP_TEST_PROGRESS;

    if (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE)
    {
        g_timer_start(p_trpConn->p_transTimer);
    }
    else
    {
        switch (p_trpConn->workMode)
        {
            case TRP_WMODE_FIX_PATTERN:
            {
                bt_shell_printf("%s %s\n", APP_TRP_WM_FIXPATTERN_STR, APP_TRP_WM_START_STR);
            }
            break;
            case TRP_WMODE_LOOPBACK:
            {
                bt_shell_printf("%s %s\n", APP_TRP_WM_LOOPBACK_STR, APP_TRP_WM_START_STR);
            }
            break;
            case TRP_WMODE_CHECK_SUM:
            {
                bt_shell_printf("%s %s\n", APP_TRP_WM_CHECKSUM_STR, APP_TRP_WM_START_STR);
            }
            break;
            default:
            break;
        }
    }
}

void APP_TRP_COMMON_ProgressingLog(APP_TRP_ConnList_T *p_trpConn)
{

    uint8_t i;
    uint16_t progress;
    uint32_t patternRemainSize;
    APP_DBP_BtDev_T *p_dev;


    if (p_trpConn == NULL)
        return;

    if (p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
    {
        progress = p_trpConn->progress + 1;
        
        switch (p_trpConn->workMode)
        {
            case TRP_WMODE_FIX_PATTERN:
            {
                if(progress >= 100)
                    progress = 0;
                else if(progress == 50)
                    printf("\r%s %s \\", APP_TRP_WM_FIXPATTERN_STR, APP_TRP_WM_PROGRESS_STR);
                else if(progress == 1)
                    printf("\r%s %s /", APP_TRP_WM_FIXPATTERN_STR, APP_TRP_WM_PROGRESS_STR);
            }
            break;
            case TRP_WMODE_LOOPBACK:
            {
                if(progress >= 100)
                    progress = 0;
                else if(progress == 50)
                    printf("\r%s %s \\", APP_TRP_WM_LOOPBACK_STR, APP_TRP_WM_PROGRESS_STR);
                else if(progress == 1)
                    printf("\r%s %s /", APP_TRP_WM_LOOPBACK_STR, APP_TRP_WM_PROGRESS_STR);
            }
            break;
            case TRP_WMODE_CHECK_SUM:
            {
                if(progress >= 100)
                    progress = 0;
                else if(progress == 50)
                    printf("\r%s %s \\", APP_TRP_WM_CHECKSUM_STR, APP_TRP_WM_PROGRESS_STR);
                else if(progress == 1)
                    printf("\r%s %s /", APP_TRP_WM_CHECKSUM_STR, APP_TRP_WM_PROGRESS_STR);
            }
            break;
            default:
            break;
        }

        p_trpConn->progress = progress;

    }
    else
    {
        switch (p_trpConn->workMode)
        {
            case TRP_WMODE_FIX_PATTERN:
            {
                printf("\rProgressing: ");
                for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
                {
                    if (s_trpConnList[i].p_deviceProxy != NULL && s_trpConnList[i].testStage >= APP_TEST_PROGRESS)
                    {
                        p_dev = APP_DBP_GetDevInfoByProxy(s_trpConnList[i].p_deviceProxy);
                        if (p_dev == NULL)
                            continue;
                        
                        printf("[%s: %3d%%]", p_dev->p_name, s_trpConnList[i].rxAccuLeng*100/APP_TRP_WMODE_TX_MAX_SIZE);
                    }
                }
            }
            break;
            case TRP_WMODE_CHECK_SUM:
            {
                printf("\rProgressing: ");
                for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
                {
                    if (s_trpConnList[i].p_deviceProxy != NULL && s_trpConnList[i].testStage >= APP_TEST_PROGRESS)
                    {
                        p_dev = APP_DBP_GetDevInfoByProxy(s_trpConnList[i].p_deviceProxy);
                        if (p_dev == NULL)
                            continue;

                        patternRemainSize = APP_TRP_WMODE_TX_MAX_SIZE - s_trpConnList[i].fixPattMaxSize;
                        printf("[%s: %3d%%]", p_dev->p_name, patternRemainSize*100/APP_TRP_WMODE_TX_MAX_SIZE);
                    }
                }

            }
            break;
            default:
            break;
        }
    }
    
    fflush(stdout);
}

void APP_TRP_COMMON_FinishLog(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t i;
    uint8_t countPass = 0;
    gdouble elapseTime;
    APP_DBP_BtDev_T *p_dev;

    g_timer_stop(p_trpConn->p_transTimer);

    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_trpConnList[i].p_deviceProxy != NULL)
        {
            if (s_trpConnList[i].testStage == APP_TEST_PROGRESS)
                return;
            //the final step is return to NULL state
            if (s_trpConnList[i].trpState != 0)
                return;
            if (s_trpConnList[i].testStage == APP_TEST_PASSED)
                countPass++;
        }
    }

#ifdef ENABLE_AUTO_RUN
    printf("\nTest result(%d runs):\n", APP_GetPassedRun());
#else
    printf("\nTest result:\n");
#endif

    printf("[Index]	[     Address     ][    Name    ][Result][Time Elapsed]\n");
    printf("=================================================================================\n");

    for (i=0; i<BLE_GAP_MAX_LINK_NBR; i++)
    {
        if (s_trpConnList[i].testStage == APP_TEST_IDLE)
            continue;
            
        p_dev = APP_DBP_GetDevInfoByProxy(s_trpConnList[i].p_deviceProxy);
        elapseTime = g_timer_elapsed(s_trpConnList[i].p_transTimer, NULL);
        
        printf("dev#%2d\t[%s][%s][%s][%f s]\n", p_dev->index, p_dev->p_address, p_dev->p_name, 
            APP_TRP_TestStageStr[s_trpConnList[i].testStage], elapseTime);

        s_trpConnList[i].testStage = APP_TEST_IDLE;
    }

    bt_shell_printf("\n");
    
#ifdef ENABLE_AUTO_RUN
    APP_GoNextRun(countPass);
#endif

}




