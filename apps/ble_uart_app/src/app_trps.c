/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Server Role Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trps.c

  Summary:
    This file contains the Application Transparent Server Role functions for this project.

  Description:
    This file contains the Application Transparent Server Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "application.h"
#include "app_trps.h"
#include "app_timer.h"
#include "app_error_defs.h"
#include "app_ble_handler.h"
#include "app_log.h"
#include "ble_trsp/ble_trsp_defs.h"
#include "shared/shell.h"
#include "shared/util.h"



// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_TRP_WM_LOOPBACK_STR         "Loopback"
#define APP_TRP_WM_CHECKSUM_STR         "Checksum"
#define APP_TRP_WM_FIXPATTERN_STR       "Fixed-pattern"
#define APP_TRP_WM_PROGRESS_STR         "progressing"
#define APP_TRP_WM_START_STR            "start"


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
APP_TRP_TrafficPriority_T       s_trpsTrafficPriority;


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************


static void APP_TRPS_FlushRxDataInAllQueue(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t status = APP_RES_SUCCESS, dataLeng = 0;
    uint8_t *p_data = NULL;


    APP_TRP_COMMON_DelAllLeCircData(&(p_trpConn->leCircQueue));
    status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
    while (dataLeng)
    {
        p_data = malloc(dataLeng);
        
        if (p_data != NULL)
        {
            APP_TRP_COMMON_GetTrpData(p_trpConn, p_data);
            free(p_data);
            p_data = NULL;
            dataLeng = 0;
        }

        status = APP_TRP_COMMON_GetTrpDataLength(p_trpConn, &dataLeng);
        if (status != APP_RES_SUCCESS)
            dataLeng = 0;
    }
}

static void app_trps_VendorCmdProc(APP_TRP_ConnList_T *p_trpConn, uint8_t *p_cmd)
{
    uint16_t lastNumber, idx;
    uint8_t groupId, commandId;

    idx = 1;
    groupId = p_cmd[idx++];
    commandId = p_cmd[idx++];
    //printf("VendorCmdProc::Group ID = %d, Command ID = %d \n", groupId, commandId);

    switch (groupId)
    {
        case TRP_GRPID_CHECK_SUM:
        {
            if (commandId == APP_TRP_WMODE_CHECK_SUM_DISABLE)
            {
                APP_TRP_COMMON_SendCheckSumCommand(p_trpConn);
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
            else if (commandId == APP_TRP_WMODE_CHECK_SUM_ENABLE)
            {
                p_trpConn->workMode = TRP_WMODE_CHECK_SUM;
                p_trpConn->workModeEn = false;
                p_trpConn->checkSum = 0;
            }
            else if (commandId == APP_TRP_WMODE_CHECK_SUM)
            {
                if (p_cmd[idx] == (p_trpConn->checkSum & 0xFF))
                {
                    bt_shell_printf("\nCheck sum = %x. Check sum is correct. \n", p_cmd[idx]);
                }
                else
                {
                    bt_shell_printf("\nCheck sum = %x. Check sum is wrong. \n", p_cmd[idx]);
                }
            }
        }
        break;

        case TRP_GRPID_LOOPBACK:
        {
            if (commandId == APP_TRP_WMODE_LOOPBACK_DISABLE)
            {
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
            else if (commandId == APP_TRP_WMODE_LOOPBACK_ENABLE)
            {
                p_trpConn->workMode = TRP_WMODE_LOOPBACK;
                p_trpConn->workModeEn = false;
            }
        }
        break;

        case TRP_GRPID_FIX_PATTERN:
        {
            if (commandId == APP_TRP_WMODE_FIX_PATTERN_DISABLE)
            {
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
            else if (commandId == APP_TRP_WMODE_FIX_PATTERN_ENABLE)
            {
                p_trpConn->workMode = TRP_WMODE_FIX_PATTERN;
                p_trpConn->workModeEn = false;
                p_trpConn->lastNumber = 0;
            }
            else if (commandId == APP_TRP_WMODE_TX_LAST_NUMBER)
            {
                lastNumber = get_be16(&(p_cmd[idx]));
                if (lastNumber == (p_trpConn->lastNumber - 1))
                {
                    bt_shell_printf("\nThe last number = %x. The last number check is successful !\n", lastNumber);
                }
                else
                {
                    bt_shell_printf("\nThe last number = %x. The last number check is fail !\n", lastNumber);
                }
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
            else if (commandId == APP_TRP_WMODE_ERROR_RSP)
            {
                bt_shell_printf("Fixed pattern error response! \n");
            }
        }
        break;

        case TRP_GRPID_UART:
        {
            if (commandId == APP_TRP_WMODE_UART_DISABLE)
            {
                APP_TRP_COMMON_DelAllCircData(&(p_trpConn->uartCircQueue));
                APP_TRP_COMMON_DelAllLeCircData(&(p_trpConn->leCircQueue));
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
            else if (commandId == APP_TRP_WMODE_UART_ENABLE)
            {
                p_trpConn->workMode = TRP_WMODE_UART;
            }
        }
        break;

        case TRP_GRPID_TRANSMIT:
        {
            if (commandId == APP_TRP_WMODE_TX_DATA_END)
            {
                if (p_trpConn->workMode == TRP_WMODE_FIX_PATTERN)
                {
                    APP_TRP_COMMON_SendLastNumber(p_trpConn);
                }
                p_trpConn->workMode = TRP_WMODE_NULL;
                p_trpConn->workModeEn = false;
                APP_TRP_COMMON_DelAllLeCircData(&(p_trpConn->leCircQueue));
                APP_TRPS_FlushRxDataInAllQueue(p_trpConn);
            }
            else if (commandId == APP_TRP_WMODE_TX_DATA_START)
            {
                p_trpConn->workModeEn = true;
                
                if (p_trpConn->workMode == TRP_WMODE_FIX_PATTERN)
                {
                    APP_TRPS_FlushRxDataInAllQueue(p_trpConn);
                    // Send the first packet
                    APP_TRP_COMMON_InitFixPatternParam(p_trpConn);
                    APP_TRP_COMMON_SendFixPatternFirstPkt(p_trpConn);

                    p_trpConn->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
                    APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpConn, APP_TIMER_1MS);
                }
                if (p_trpConn->workMode == TRP_WMODE_LOOPBACK) 
                {
                    APP_TIMER_SetTimer(APP_TIMER_TRPS_PROGRESS_CHECK, 0, NULL, APP_TIMER_3S);
                }

                APP_TRP_COMMON_StartLog(p_trpConn);
                
                break;
            }
            else if (commandId == APP_TRP_WMODE_TX_DATA_LENGTH)
            {
                //APP_UTILITY_BUF_BE_TO_U32(&(p_trpConn->txTotalLeng), &(p_cmd[idx]));
                p_trpConn->txTotalLeng = get_be32(&p_cmd[idx]);
                APP_LOG_DEBUG("Total data length to be transmitted = %d \n", p_trpConn->txTotalLeng);
            }
            else if (commandId == APP_TRP_WMODE_TX_TYPE)
            {
                p_trpConn->type = p_cmd[idx];
                
                if (p_trpConn->type == APP_TRP_TYPE_LEGACY)
                {
                    APP_DBP_BtDev_T *p_dev = APP_DBP_GetDevInfoByProxy(p_trpConn->p_deviceProxy);
                    if (p_dev && p_dev->ssf == 0x01)
                        p_trpConn->lePktLeng = p_trpConn->txMTU-2;
                    else
                        p_trpConn->lePktLeng = p_trpConn->txMTU;
                }
                else if (p_trpConn->channelEn & APP_TRCBP_DATA_CHAN_ENABLE)
                    p_trpConn->lePktLeng = p_trpConn->fixPattTrcbpMtu;
            }
        }
        break;

        case TRP_GRPID_UPDATE_CONN_PARA: 
        break;

        case TRP_GRPID_WMODE_SELECTION:
        {
            if((commandId >= TRP_GRPID_NULL) && (commandId < TRP_GRPID_TRANSMIT))
            {
                p_trpConn->workMode = commandId;
            }
        }
        break;

        case TRP_GRPID_REV_LOOPBACK:
        {
            if ((commandId == APP_TRP_WMODE_REV_LOOPBACK_DISABLE) || (commandId == APP_TRP_WMODE_ERROR_RSP))
            {
                p_trpConn->workMode = TRP_WMODE_NULL;
            }
        }
        break;
        
        default:
            break;
    }
}


uint16_t APP_TRPS_LeTxData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data)
{
    uint16_t status = TRSP_RES_SUCCESS;

    if (p_trpConn == NULL || p_data == NULL || len == 0)
        return APP_RES_FAIL;

#if 0
    if (!APP_TRP_COMMON_CheckValidTopology(APP_TRP_SERVER_ROLE))
    {
        return APP_RES_BAD_STATE;
    }
#endif

    if (len > p_trpConn->txMTU)
    {
        return APP_RES_OOM;
    }
    
    status = BLE_TRSPS_SendData(p_trpConn->p_deviceProxy, len, p_data);
    if (status == TRSP_RES_NO_RESOURCE)
        return APP_RES_NO_RESOURCE;
    else if (status != TRSP_RES_SUCCESS)
        return APP_RES_FAIL;

    return APP_RES_SUCCESS;
}

static void app_trps_LeRxProc(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t checkListFg = false;
    uint16_t status = APP_RES_FAIL;
    APP_TRP_ConnList_T *p_trpCurrConn, *p_trpNextConn;

    p_trpCurrConn = APP_TRP_COMMON_GetConnListByIndex(s_trpsTrafficPriority.rxToken);
    if (p_trpCurrConn == NULL)
    {
        p_trpCurrConn = APP_TRP_COMMON_ChangeNextLink(APP_TRP_SERVER_ROLE, APP_TRP_LINK_TYPE_RX, &s_trpsTrafficPriority);
        if(p_trpCurrConn == NULL)
            return;
    }
    
    s_trpsTrafficPriority.validNumber = APP_TRP_MAX_TRANSMIT_NUM;

    //printf("TRPS_RxBuf start(%d, %d, %d),w=%d\n", 
    //    s_trpsTrafficPriority.txToken, s_trpsTrafficPriority.validNumber, 
    //    p_trpCurrConn->maxAvailTxNumber, p_trpCurrConn->workMode);

    while (s_trpsTrafficPriority.validNumber > 0)
    {        
        switch (p_trpConn->workMode)
        {
            case TRP_WMODE_CHECK_SUM:
            {
                p_trpConn->checkSum = APP_TRP_COMMON_CalculateCheckSum(p_trpConn->checkSum, &(p_trpConn->txTotalLeng), p_trpConn);
                
                if (p_trpConn->txTotalLeng == 0)
                {
                    status = APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
                    
                    if (status != APP_RES_SUCCESS)
                        APP_LOG_ERROR("APP_TIMER_PROTOCOL_RSP stop error ! result=%d\n", status);
                    
                    APP_TRP_COMMON_SendCheckSumCommand(p_trpConn);
                }
                p_trpConn->maxAvailTxNumber = 0;

                APP_TRP_COMMON_ProgressingLog(p_trpConn);
            }
            break;

            case TRP_WMODE_LOOPBACK:
            {
                status = APP_TRP_COMMON_SendMultiLinkLeDataTrpProfile(&s_trpsTrafficPriority, p_trpConn);

                APP_TRP_COMMON_ProgressingLog(p_trpConn);
            }
            break;

            case TRP_WMODE_UART:
            {
                APP_TRP_COMMON_SendTrpProfileDataToUART(p_trpConn);
                p_trpConn->maxAvailTxNumber = 0;
            }
            break;

            default:
                p_trpConn->maxAvailTxNumber = 0;
            break;
        }

        if ((status == APP_RES_OOM) || (status == APP_RES_INVALID_PARA))
            break;

        // Change to the next link if there is no resource for the dedicated link.
        if (status == APP_RES_NO_RESOURCE)
            p_trpConn->maxAvailTxNumber = 0;

        if (p_trpConn->maxAvailTxNumber == 0)
        {
            p_trpConn->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
            p_trpNextConn = APP_TRP_COMMON_ChangeNextLink(APP_TRP_SERVER_ROLE, APP_TRP_LINK_TYPE_TX, &s_trpsTrafficPriority);
            checkListFg = true;
        }
        
        if (checkListFg)
        {
            checkListFg = false;
            if ((p_trpNextConn == p_trpCurrConn) || (p_trpCurrConn == NULL) || (p_trpNextConn == NULL))
                break;
        }
    }

}


void APP_TRPS_EventHandler(BLE_TRSPS_Event_T *p_event)
{
    APP_TRP_ConnList_T *p_trpsConnLink = NULL;

    switch(p_event->eventId)
    {
        case BLE_TRSPS_EVT_CTRL_STATUS:
        {
            printf("BLE_TRSPS_EVT_CTRL_STATUS\n");
            if (p_event->eventField.onCtrlStatus.status == BLE_TRSPS_STATUS_CTRL_OPENED)
            {
                APP_TRP_COMMON_CtrlChOpenProc(true);
            }
            else
            {
                APP_TRP_COMMON_CtrlChOpenProc(false);
            }
        }
        break;
            
        case BLE_TRSPS_EVT_TX_STATUS:
        {
            printf("BLE_TRSPS_EVT_TX_STATUS\n");
            if (p_event->eventField.onTxStatus.status == BLE_TRSPS_STATUS_TX_OPENED)
            {
                APP_TRP_COMMON_TxChOpenProc(true);
            }
            else
            {
                APP_TRP_COMMON_TxChOpenProc(false);
            }
        }
        break;

        case BLE_TRSPS_EVT_RECEIVE_DATA:
        {
            //printf("BLE_TRSPS_EVT_RECEIVE_DATA\n");
            p_trpsConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onReceiveData.p_dev);
            
            if (p_trpsConnLink != NULL)
            {
                if (p_trpsConnLink->workMode == TRP_WMODE_NULL)
                {
                    APP_TRP_COMMON_FreeLeData(p_trpsConnLink);
                    break;
                }
                if (p_trpsConnLink->workMode != TRP_WMODE_UART)
                {
                    APP_TIMER_SetTimer(APP_TIMER_TRPS_PROGRESS_CHECK, 0, NULL, APP_TIMER_3S);
                }

                app_trps_LeRxProc(p_trpsConnLink);
            }
        }
        break;

        case BLE_TRSPS_EVT_CBFC_ENABLED:
        {
            printf("BLE_TRSPS_EVT_CBFC_ENABLED\n");
        }
        break;

        case BLE_TRSPS_EVT_CBFC_CREDIT:
        {
            uint16_t status = APP_RES_SUCCESS;
            APP_TRP_ConnList_T       *p_trpsCurrentLink = NULL;
            APP_TRP_ConnList_T       *p_creditReturnLink = NULL;

            //printf("BLE_TRSPS_EVT_CBFC_CREDIT\n");

            p_creditReturnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onCbfcEnabled.p_dev);
            p_trpsCurrentLink = APP_TRP_COMMON_GetConnListByIndex(s_trpsTrafficPriority.txToken);

            if ((p_creditReturnLink == p_trpsCurrentLink) &&
                (p_trpsCurrentLink->workMode == TRP_WMODE_FIX_PATTERN) && 
                (p_trpsCurrentLink->workModeEn == true))
            {
                if (p_trpsCurrentLink->maxAvailTxNumber > 0)
                {
                    status = APP_TRP_COMMON_SendMultiLinkFixPattern(&s_trpsTrafficPriority, p_trpsCurrentLink);
                    
                    if (status & APP_RES_COMPLETE)
                    {
                        APP_TRP_COMMON_SendModeCommand(p_trpsCurrentLink, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
                        APP_TRP_COMMON_SendLastNumber(p_trpsCurrentLink);
                        p_trpsCurrentLink->workModeEn = false;
                        break;
                    }
    
                    if (status != APP_RES_SUCCESS)
                    {
                      p_trpsCurrentLink->maxAvailTxNumber = 0;
                    }
                }
            }
            else if ((p_creditReturnLink == p_trpsCurrentLink) &&
                (p_trpsCurrentLink->workModeEn == true))
            {
                p_trpsCurrentLink->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
                APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpsCurrentLink, APP_TIMER_1MS);
            }


        }
        break;
        
        case BLE_TRSPS_EVT_VENDOR_CMD:
        {
            p_trpsConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onVendorCmd.p_dev);
            
            if ((p_trpsConnLink != NULL) && (p_event->eventField.onVendorCmd.p_payLoad[0] == APP_TRP_VENDOR_OPCODE_BLE_UART))
            {
                app_trps_VendorCmdProc(p_trpsConnLink, p_event->eventField.onVendorCmd.p_payLoad);
            }
        }
        break;

        default:
            break;
    }
}

void APP_TRPS_Init(void)
{
    memset((uint8_t *) &s_trpsTrafficPriority, 0, sizeof(APP_TRP_TrafficPriority_T));
}


#define APP_TIMER_275MS 0x113
void APP_TRPS_TxProc(APP_TRP_ConnList_T * p_trpConn)
{
    APP_TRP_ConnList_T  *p_trpsTxLeLink = NULL;
    uint16_t status = APP_RES_SUCCESS;

    if (p_trpConn == NULL)
    {
        p_trpsTxLeLink = APP_TRP_COMMON_GetConnListByIndex(s_trpsTrafficPriority.txToken);
    }
    else
    {
        //change token
        APP_TRP_COMMON_AssignToken(p_trpConn, APP_TRP_LINK_TYPE_TX, &s_trpsTrafficPriority);
        p_trpsTxLeLink = p_trpConn;
    }


    s_trpsTrafficPriority.validNumber = APP_TRP_MAX_TRANSMIT_NUM;

    APP_TIMER_SetTimer(APP_TIMER_TRPS_PROGRESS_CHECK, 0, NULL, APP_TIMER_3S);

    while (1)
    {
        if ((p_trpsTxLeLink->workMode == TRP_WMODE_LOOPBACK) && (p_trpsTxLeLink->workModeEn))
        {
            if (p_trpsTxLeLink->maxAvailTxNumber > 0)
            {
                status = APP_TRP_COMMON_SendMultiLinkLeDataTrpProfile(&s_trpsTrafficPriority, p_trpsTxLeLink);
                if (status == APP_RES_OOM)
                {
                    break;
                }
                else if ((status == APP_RES_INVALID_PARA) || (status == APP_RES_NO_RESOURCE))
                {
                    p_trpsTxLeLink->maxAvailTxNumber = 0;
                }
                
                APP_TRP_COMMON_ProgressingLog(p_trpsTxLeLink);
            }
        }
        else if ((p_trpsTxLeLink->workMode == TRP_WMODE_FIX_PATTERN) && (p_trpsTxLeLink->workModeEn == true))
        {
            if (p_trpsTxLeLink->maxAvailTxNumber > 0)
            {
                status = APP_TRP_COMMON_SendMultiLinkFixPattern(&s_trpsTrafficPriority, p_trpsTxLeLink);
                if (status & APP_RES_COMPLETE)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpsTxLeLink, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
                    APP_TRP_COMMON_SendLastNumber(p_trpsTxLeLink);
                    p_trpsTxLeLink->workModeEn = false;
                    bt_shell_printf("\rSend Fixed-Pattern last number\n");
                    break;
                }

                if (status != APP_RES_SUCCESS)
                {
                    bt_shell_printf("\rSend Fixed-Pattern fail(%d)\n", status);
                    p_trpsTxLeLink->maxAvailTxNumber = 0;
                }
                
                APP_TRP_COMMON_ProgressingLog(p_trpsTxLeLink);
            }
        }
        else if ((p_trpsTxLeLink->workMode == TRP_WMODE_UART) && (p_trpsTxLeLink->workModeEn == true))
        {
            // Send data to transparent profile
            status = APP_TRP_COMMON_SendLeDataUartCircQueue(p_trpsTxLeLink);
            if (status == APP_RES_NO_RESOURCE)
            {
                p_trpsTxLeLink->maxAvailTxNumber = 0;
                break;
            }

            uint32_t remain = APP_RawDataRemaining(p_trpsTxLeLink);

            if (p_trpsTxLeLink->maxAvailTxNumber > 0)
            {
                p_trpsTxLeLink->maxAvailTxNumber--;
                if (remain > 0)
                {
                    APP_TIMER_SetTimer(APP_TIMER_RAW_DATA_FETCH, p_trpsTxLeLink->maxAvailTxNumber, p_trpsTxLeLink->p_deviceProxy, APP_TIMER_1MS);
                }
                else
                {
                    p_trpsTxLeLink->maxAvailTxNumber = 0;
                }
                
                APP_TRP_COMMON_ProgressingLog(p_trpsTxLeLink);
            }
        }
        else
        {
            p_trpsTxLeLink->maxAvailTxNumber = 0;
        }
        

        if (p_trpsTxLeLink->maxAvailTxNumber == 0)
        {
            if (p_trpsTxLeLink->workMode == TRP_WMODE_FIX_PATTERN)
            {
                //uint8_t peripheralNum;
                //peripheralNum = APP_TRP_COMMON_GetRoleNum(BLE_GAP_ROLE_PERIPHERAL);
                p_trpsTxLeLink->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
                //if (peripheralNum)
                //{
                //    APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpsTxLeLink, APP_TIMER_275MS/peripheralNum);
                //}
                APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpsTxLeLink, APP_TIMER_275MS);
            }
            else if (p_trpsTxLeLink->workMode == TRP_WMODE_UART)
            {
                //uint8_t peripheralNum;
                uint32_t remain = APP_RawDataRemaining(p_trpsTxLeLink);
                //peripheralNum = APP_TRP_COMMON_GetRoleNum(BLE_GAP_ROLE_PERIPHERAL);
                if (remain || p_trpsTxLeLink->uartCircQueue.usedNum)
                {
                    p_trpsTxLeLink->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
                    APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpsTxLeLink, APP_TIMER_275MS);
                }
            }
            break;
        }
    }
}

