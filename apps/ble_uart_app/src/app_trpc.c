/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Client Role Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trpc.c

  Summary:
    This file contains the Application Transparent Client Role functions for this project.

  Description:
    This file contains the Application Transparent Client Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "application.h"
#include "app_trpc.h"
#include "app_timer.h"
#include "app_error_defs.h"
#include "app_ble_handler.h"
#include "app_scan.h"
#include "app_log.h"
#include "ble_trsp/ble_trsp_defs.h"
#include "shared/shell.h"


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define APP_TRPC_EVENT_NULL             0x00
#define APP_TRPC_EVENT_CHECK_SUM        0x01
#define APP_TRPC_EVENT_TRX_END          0x02
#define APP_TRPC_EVENT_LAST_NUMBER      0x04
#define APP_TRPC_EVENT_RX_LE_DATA       0x08
#define APP_TRPC_EVENT_TX_LE_DATA       0x10

/**@brief Enumeration type of check sum state. */
enum APP_TRPC_CS_STATE_T
{
    TRPC_CS_STATE_NULL = 0x00,      /**< The null state of check sum state machine. */
    TRPC_CS_STATE_ENABLE_MODE,      /**< The enable mode state of check sum state machine. */
    TRPC_CS_STATE_SEND_LENGTH,      /**< The send length state of check sum state machine. */
    TRPC_CS_STATE_SEND_TYPE,        /**< The send type state of check sum state machine. */
    TRPC_CS_STATE_START_TX,         /**< The start transmit state of check sum state machine. */
    TRPC_CS_STATE_TX,               /**< The transmit state of check sum state machine. */
    TRPC_CS_STATE_WAIT_CS,          /**< The wait check sum value state of check sum state machine. */
    TRPC_CS_STATE_STOP_TX,          /**< The stop transmit state of check sum state machine. */
    TRPC_CS_STATE_SEND_CS,          /**< The send check sum state of check sum state machine. */
    
    TRPC_CS_STATE_END               /**< The end state of check sum state machine. */
};

/**@brief Enumeration type of fixed pattern state. */
enum APP_TRPC_FP_STATE_T
{
    TRPC_FP_STATE_NULL = 0x00,          /**< The null state of fixed pattern state machine. */
    TRPC_FP_STATE_ENABLE_MODE,          /**< The enable mode state of fixed pattern state machine. */
    TRPC_FP_STATE_SEND_TYPE,            /**< The send type state of fixed pattern state machine. */
    TRPC_FP_STATE_RX,                   /**< The recieved state of fixed pattern state machine. */
    // TRPC_FP_STATE_WAIT_STOP_RX,         /**< The wait stop state of fixed pattern state machine. */
    TRPC_FP_STATE_WAIT_LAST_NUMBER,     /**< The wait last unmber state of fixed pattern state machine. */
    TRPC_FP_STATE_SEND_LAST_NUMBER,     /**< The send last unmber state of fixed pattern state machine. */
    
    TRPC_FP_STATE_END               /**< The end state of fixed pattern state machine. */
};

/**@brief Enumeration type of loopback state. */
enum APP_TRPC_LB_STATE_T
{
    TRPC_LB_STATE_NULL = 0x00,      /**< The null state of loopback state machine. */
    TRPC_LB_STATE_ENABLE_MODE,      /**< The enable mode state of loopback state machine. */
    TRPC_LB_STATE_SEND_TYPE,        /**< The send type state of loopback state machine. */
    TRPC_LB_STATE_START_TX,         /**< The start Tx state of loopback state machine. */
    TRPC_LB_STATE_TRX,              /**< The TRx state of loopback state machine. */
    TRPC_LB_STATE_WAIT_STOP_TX,     /**< The wait stop state of loopback state machine. */
    TRPC_LB_STATE_SEND_STOP_TX,     /**< The send stop state of loopback state machine. */
    
    TRPC_LB_STATE_END               /**< The end state of loopback state machine. */
};

/**@brief Enumeration type of UART state. */
enum APP_TRPC_UART_STATE_T
{
    TRPC_UART_STATE_NULL = 0x00,        /**< The null state of UART state machine. */
    TRPC_UART_STATE_ENABLE_MODE,        /**< The enable mode state of UART state machine. */
    TRPC_UART_STATE_SEND_TYPE,          /**< The send type state of UART state machine. */
    TRPC_UART_STATE_RELAY_DATA,         /**< The relay state of UART state machine. */
    TRPC_UART_STATE_DISABLE_MODE,       /**< The disable mode state of UART state machine. */
    TRPC_UART_STATE_END                 /**< The end state of UART state machine. */
};

/**@brief Enumeration type of reverse loopback state. */
enum APP_TRPC_REV_LB_STATE_T
{
    TRPC_REV_LB_STATE_NULL = 0x00,      /**< The null state of reverse loopback state machine. */
    TRPC_REV_LB_STATE_ENABLE_MODE,      /**< The enable mode state of reverse loopback state machine. */
    TRPC_REV_LB_STATE_SEND_TYPE,        /**< The send type state of reverse loopback state machine. */
    TRPC_REV_LB_STATE_START_TX,         /**< The start Tx state of reverse loopback state machine. */
    TRPC_REV_LB_STATE_TRX,              /**< The TRx state of reverse loopback state machine. */
    TRPC_REV_LB_STATE_SEND_STOP_TX,     /**< The send stop state of reverse loopback state machine. */
    
    TRPC_REV_LB_STATE_END               /**< The end state of loopback state machine. */
};


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_TRP_ConnList_T       *sp_trpcCurrentLink = NULL;
APP_TRP_TrafficPriority_T       s_trpcTrafficPriority;


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static void app_trpc_LeRxProc(APP_TRP_ConnList_T *p_trpConn);


static void app_trpc_UartStateMachine(uint8_t event, APP_TRP_ConnList_T *p_trpConn)
{
    switch(p_trpConn->trpState)
    {
        case TRPC_UART_STATE_NULL:
        {
            p_trpConn->trpState = TRPC_UART_STATE_ENABLE_MODE;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_UART, APP_TRP_WMODE_UART_ENABLE);
            APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);
        }
        break;

        case TRPC_UART_STATE_ENABLE_MODE:
        {
            p_trpConn->trpState = TRPC_UART_STATE_SEND_TYPE;
            APP_TRP_COMMON_SendTypeCommand(p_trpConn);
            APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);
        }
        break;

        case TRPC_UART_STATE_SEND_TYPE:
        {
            p_trpConn->trpState = TRPC_UART_STATE_RELAY_DATA;
            p_trpConn->workModeEn = true;
            APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
        }
        break;

        case TRPC_UART_STATE_RELAY_DATA:
        {
            if (event & APP_TRPC_EVENT_RX_LE_DATA) // Send Le data to UART
                APP_TRP_COMMON_SendTrpProfileDataToUART(p_trpConn);
            
            if (event & APP_TRPC_EVENT_TX_LE_DATA) // Send UART data to Le
            {
                //printf("UART-SM(%d)\n", p_trpConn->uartCircQueue.usedNum);
                if (p_trpConn->uartCircQueue.usedNum > 0)
                {
                    APP_TRP_COMMON_SendLeDataUartCircQueue(p_trpConn);
                }
            }
            
            if (event & APP_TRPC_EVENT_TRX_END)
                p_trpConn->trpState = TRPC_UART_STATE_DISABLE_MODE;
        }
        break;

        case TRPC_UART_STATE_DISABLE_MODE:
        {
        }
        break;
        
        default:
            break;
    }
}

static void app_trpc_LoopbackRxDataCheck(APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t result = APP_RES_FAIL;
    
    result = APP_TRP_COMMON_CheckFixPatternData(p_trpConn);
    if (result != APP_RES_SUCCESS)
    {
        APP_LOG_ERROR("Loopback content error !\n");
        result = APP_TRP_COMMON_SendErrorRsp(p_trpConn, TRP_GRPID_LOOPBACK);
        if (result == APP_RES_SUCCESS)
        {
            p_trpConn->trpState = TRPC_LB_STATE_SEND_STOP_TX;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
        }
        else
        {
            p_trpConn->trpState = TRPC_LB_STATE_WAIT_STOP_TX;
            p_trpConn->workModeEn = false;
        }
    }
    else if (p_trpConn->rxLastNunber == p_trpConn->lastNumber)
    {
        p_trpConn->trpState = TRPC_LB_STATE_WAIT_STOP_TX;
        p_trpConn->workModeEn = false;
        APP_LOG_INFO("Loopback is successful !\n");
    }
}

static void app_trpc_LoopbackStateMachine(uint8_t event, APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t result = APP_RES_FAIL;

    switch(p_trpConn->trpState)
    {
        case TRPC_LB_STATE_NULL:
        {
            p_trpConn->trpState = TRPC_LB_STATE_ENABLE_MODE;
            result = APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_LOOPBACK, APP_TRP_WMODE_LOOPBACK_ENABLE);
            if (result == APP_RES_SUCCESS)
            {
                APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);
            }
        }
        break;
        
        case TRPC_LB_STATE_ENABLE_MODE:
        {
            p_trpConn->trpState = TRPC_LB_STATE_SEND_TYPE;
            APP_TRP_COMMON_SendTypeCommand(p_trpConn);
        }
        break;

        case TRPC_LB_STATE_SEND_TYPE:
        {
            p_trpConn->trpState = TRPC_LB_STATE_START_TX;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_START);
        }
        break;
        
        case TRPC_LB_STATE_START_TX:
        {
            p_trpConn->trpState = TRPC_LB_STATE_TRX;
            p_trpConn->workModeEn = true;
            result = APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
            if (result != APP_RES_SUCCESS)
            {
                APP_LOG_ERROR("APP_TIMER_PROTOCOL_RSP stop error ! result=%d\n", result);
            }
        }
        break;

        case TRPC_LB_STATE_TRX:
        {
            if (event & APP_TRPC_EVENT_RX_LE_DATA) // Send Le data to UART
                APP_TRP_COMMON_SendTrpProfileDataToUART(p_trpConn);
            
            if (event & APP_TRPC_EVENT_TX_LE_DATA) // Send UART data to Le
            {
                //printf("usedNum=%d\n", p_trpConn->uartCircQueue.usedNum);
                //printf("LB-SM(%d)\n", p_trpConn->uartCircQueue.usedNum);
                if (p_trpConn->uartCircQueue.usedNum > 0)
                {
                    APP_TRP_COMMON_SendLeDataUartCircQueue(p_trpConn);
                }
            }
        }
        break;
        
        case TRPC_LB_STATE_WAIT_STOP_TX:
        {
            if (event & APP_TRPC_EVENT_RX_LE_DATA)
            {
                app_trpc_LoopbackRxDataCheck(p_trpConn);
            }
            
            if (p_trpConn->workModeEn == false)
            {
                p_trpConn->trpState = TRPC_LB_STATE_SEND_STOP_TX;
                APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
            }
        }
        break;

        case TRPC_LB_STATE_SEND_STOP_TX:
        {
            p_trpConn->trpState = TRPC_LB_STATE_NULL;
            result = APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
            if (result != APP_RES_SUCCESS)
            {
                APP_LOG_ERROR("APP_TIMER_PROTOCOL_RSP stop error ! result=%d\n", result);
            }
            //app_trpc_WmodeStateMachine(p_connList_t);
        }
        break;

        default:
            break;
    }
}

static void app_trpc_FixPatternStateMachine(uint8_t event, APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t result = APP_RES_FAIL;
    
    APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);

        
    switch(p_trpConn->trpState)
    {
        case TRPC_FP_STATE_NULL:
        {
            p_trpConn->trpState = TRPC_FP_STATE_ENABLE_MODE;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_FIX_PATTERN, APP_TRP_WMODE_FIX_PATTERN_ENABLE);
        }
            break;

        case TRPC_FP_STATE_ENABLE_MODE:
        {
            p_trpConn->trpState = TRPC_FP_STATE_SEND_TYPE;
            APP_TRP_COMMON_SendTypeCommand(p_trpConn);
            APP_TRP_COMMON_StartLog(p_trpConn);
        }
            break;

        case TRPC_FP_STATE_SEND_TYPE:
        {
            p_trpConn->trpState = TRPC_FP_STATE_RX;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_START);
            p_trpConn->workModeEn = true;
            p_trpConn->rxLastNunber = 0;
            p_trpConn->checkSum = 0;
            p_trpConn->rxAccuLeng = 0;
        }
            break;

        case TRPC_FP_STATE_RX:
        {
            if (event & APP_TRPC_EVENT_RX_LE_DATA)
            {
                result = APP_TRP_COMMON_CheckFixPatternData(p_trpConn);
                if (result != APP_RES_SUCCESS)
                {
                    APP_LOG_ERROR("Fix pattern content error(%d) !\n", result);
                    APP_TRP_COMMON_DelAllLeCircData(&(p_trpConn->leCircQueue));
                    p_trpConn->trpState = TRPC_FP_STATE_SEND_LAST_NUMBER;
                    p_trpConn->lastNumber = p_trpConn->rxLastNunber;
                    APP_TRP_COMMON_SendLastNumber(p_trpConn);
                    result = APP_TRP_COMMON_SendErrorRsp(p_trpConn, TRP_GRPID_FIX_PATTERN);
                }
                APP_TRP_COMMON_ProgressingLog(p_trpConn);
            }
            if (event & APP_TRPC_EVENT_TRX_END)
            {
                p_trpConn->trpState = TRPC_FP_STATE_WAIT_LAST_NUMBER;
                (p_trpConn->rxLastNunber)--;
            }
        }
            break;

        case TRPC_FP_STATE_WAIT_LAST_NUMBER:
        {
            if (event & APP_TRPC_EVENT_LAST_NUMBER)
            {
                p_trpConn->trpState = TRPC_FP_STATE_SEND_LAST_NUMBER;
                p_trpConn->lastNumber = p_trpConn->rxLastNunber;
                result = APP_TRP_COMMON_SendLastNumber(p_trpConn);
            }
        }
            break;

        case TRPC_FP_STATE_SEND_LAST_NUMBER:
        {
            p_trpConn->workModeEn = false;
            p_trpConn->trpState = TRPC_FP_STATE_NULL;
            
            APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
            //APP_TRPC_WmodeStateMachine(p_connList_t);

            APP_TRP_COMMON_FinishLog(p_trpConn);
        }
            break;

        default:
            break;
    }

}

static void app_trpc_CheckSumStateMachine(uint8_t event, APP_TRP_ConnList_T *p_trpConn)
{
    uint16_t result = APP_RES_FAIL;
    
    APP_TIMER_SetTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn), (void *)p_trpConn, APP_TIMER_3S);

    
    switch(p_trpConn->trpState)
    {
        case TRPC_CS_STATE_NULL:
        {
            p_trpConn->trpState = TRPC_CS_STATE_ENABLE_MODE;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_CHECK_SUM, APP_TRP_WMODE_CHECK_SUM_ENABLE);
        }
            break;

        case TRPC_CS_STATE_ENABLE_MODE:
        {
            p_trpConn->trpState = TRPC_CS_STATE_SEND_LENGTH;
            APP_TRP_COMMON_SendLengthCommand(p_trpConn, APP_TRP_WMODE_TX_MAX_SIZE);
        }
            break;

        case TRPC_CS_STATE_SEND_LENGTH:
        {
            p_trpConn->trpState = TRPC_CS_STATE_SEND_TYPE;
            APP_TRP_COMMON_SendTypeCommand(p_trpConn);
        }
            break;

        case TRPC_CS_STATE_SEND_TYPE:
        {
            p_trpConn->trpState = TRPC_CS_STATE_START_TX;
            APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_START);
            p_trpConn->workModeEn = true;
        }
            break;

        case TRPC_CS_STATE_START_TX:
        {
            // Send the first packet
            APP_TRP_COMMON_InitFixPatternParam(p_trpConn);
            APP_TRP_COMMON_SendFixPattern(p_trpConn);
            APP_TRP_COMMON_StartLog(p_trpConn);
            APP_TRP_COMMON_ProgressingLog(p_trpConn);
            p_trpConn->trpState = TRPC_CS_STATE_TX;
        }
            break;

        case TRPC_CS_STATE_TX:
        {
            APP_TRP_COMMON_ProgressingLog(p_trpConn);
            
            if(event & APP_TRPC_EVENT_TX_LE_DATA)
            {
                result = APP_TRP_COMMON_SendFixPattern(p_trpConn);
                if (result & APP_RES_COMPLETE)
                {
                    p_trpConn->trpState = TRPC_CS_STATE_WAIT_CS;
                }
            }
            // Wait for check sum from server role.
            if (event & APP_TRPC_EVENT_CHECK_SUM)
            {
                p_trpConn->trpState = TRPC_CS_STATE_STOP_TX;
                APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
            }
        }
            break;
        
        case TRPC_CS_STATE_WAIT_CS:
        {
            // Wait for check sum from server role.
            if (event & APP_TRPC_EVENT_CHECK_SUM)
            {
                p_trpConn->trpState = TRPC_CS_STATE_STOP_TX;
                APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, APP_TRP_WMODE_TX_DATA_END);
            }
        }
            break;

        case TRPC_CS_STATE_STOP_TX:
        {
            p_trpConn->workModeEn = false;
            p_trpConn->trpState = TRPC_CS_STATE_SEND_CS;
            APP_TRP_COMMON_SendCheckSumCommand(p_trpConn);
        }
            break;

        case TRPC_CS_STATE_SEND_CS:
        {
            p_trpConn->trpState = TRPC_CS_STATE_NULL;
            APP_TIMER_StopTimer(APP_TIMER_PROTOCOL_RSP, APP_TRP_COMMON_GetConnIndex(p_trpConn));
            //APP_TRPC_WmodeStateMachine(p_connList_t);

            APP_TRP_COMMON_FinishLog(p_trpConn);
        }
            break;

        default:
            break;
    }

}


static void app_trpc_VendorCmdProc(APP_TRP_ConnList_T *p_trpConn, uint8_t length, uint8_t *p_cmd)
{
    uint8_t idx, groupId, commandId;
    bool    sendErrCommandFg = false;
    uint8_t grpId[] = {TRP_GRPID_NULL, TRP_GRPID_CHECK_SUM, TRP_GRPID_LOOPBACK, TRP_GRPID_FIX_PATTERN,
        TRP_GRPID_UART, TRP_GRPID_REV_LOOPBACK};
    uint16_t lastNumberServer;

    idx = 1;
    groupId = p_cmd[idx++];
    commandId = p_cmd[idx++];
    //printf("(W=%d)Group ID = %d, Command ID = %d \n", p_trpConn->workMode, groupId, commandId);


    switch(p_trpConn->workMode)
    {
        case TRP_WMODE_CHECK_SUM:
        {
            if (groupId == TRP_GRPID_CHECK_SUM)
            {
                if (commandId == APP_TRP_WMODE_CHECK_SUM)
                {
                    if (((uint8_t)(p_trpConn->checkSum)) == p_cmd[idx])
                    {
                        p_trpConn->testStage = APP_TEST_PASSED;
                        //bt_shell_printf("Check sum is successful !\n");
                    }
                    else
                    {
                        p_trpConn->testStage = APP_TEST_FAILED;
                        //bt_shell_printf("Check sum is error. CS_C:%d,CS_S:%d\n", (uint8_t)(p_trpConn->checkSum), p_cmd[idx]);
                    }
                    
                    if ((p_trpConn->trpState == TRPC_CS_STATE_TX) || (p_trpConn->trpState == TRPC_CS_STATE_WAIT_CS))
                        app_trpc_CheckSumStateMachine(APP_TRPC_EVENT_CHECK_SUM, p_trpConn);
                    else
                        sendErrCommandFg = true;
                }
                else if (commandId == APP_TRP_WMODE_ERROR_RSP)
                {
                    bt_shell_printf("Check sum procedure is error!\n");
                }
            }
        }
        break;

        case TRP_WMODE_FIX_PATTERN:
        {
            if (groupId == TRP_GRPID_FIX_PATTERN)
            {
                if (commandId == APP_TRP_WMODE_TX_LAST_NUMBER)
                {
                    lastNumberServer = p_cmd[idx++];
                    lastNumberServer = (lastNumberServer << 8) | p_cmd[idx];
                    if (p_trpConn->rxLastNunber == lastNumberServer)
                    {
                        p_trpConn->testStage = APP_TEST_PASSED;
                        //bt_shell_printf("Fixed Pattern is successful !\n");
                    }
                    else
                    {
                        p_trpConn->testStage = APP_TEST_FAILED;
                        //bt_shell_printf("Fixed Pattern is error. FP_C:%d,FP_S:%d", p_trpConn->rxLastNunber, 
                        //    lastNumberServer);
                    }
                    if (p_trpConn->trpState == TRPC_FP_STATE_WAIT_LAST_NUMBER)
                        app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_LAST_NUMBER, p_trpConn);
                    else
                        sendErrCommandFg = true;
                }
                else if (commandId == APP_TRP_WMODE_ERROR_RSP)
                {
                    bt_shell_printf("Fixed Pattern procedure is error!\n");
                }
            }
            else if (groupId == TRP_GRPID_TRANSMIT)
            {
                if ((commandId == APP_TRP_WMODE_TX_DATA_END) && (p_trpConn->trpState == TRPC_FP_STATE_RX))
                    app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_TRX_END, p_trpConn);
                else
                    sendErrCommandFg = true;
            }
        }
        break;
            
        case TRP_WMODE_LOOPBACK:
        {
            if ((groupId == TRP_GRPID_LOOPBACK) && (commandId == APP_TRP_WMODE_ERROR_RSP))
            {
                bt_shell_printf("Loopback procedure is error!\n");
            }
        }
        break;

        case TRP_WMODE_UART:
        {
            if (groupId == TRP_GRPID_UART)
            {
                if ((commandId == APP_TRP_WMODE_UART_DISABLE) && (p_trpConn->trpState == TRPC_UART_STATE_RELAY_DATA))
                    app_trpc_UartStateMachine(APP_TRPC_EVENT_TRX_END, p_trpConn);
            }
        }
        break;

        default:
            break;
    }

    /*
    if (groupId == TRP_GRPID_UPDATE_CONN_PARA)
    {
        sendErrCommandFg = true;
    }
    */


    if (sendErrCommandFg)
    {
        //printf("VendorCmdProc, SendErrorRsp\n");
        APP_TRP_COMMON_SendErrorRsp(p_trpConn, grpId[p_trpConn->workMode]);
    }

}

static void app_trpc_VendorCmdRspProc(APP_TRP_ConnList_T *p_trpConn)
{
    //printf("app_trpc_VendorCmdRspProc(w=%d)\n", p_trpConn->workMode);
    switch(p_trpConn->workMode)
    {
        case TRP_WMODE_CHECK_SUM:
            app_trpc_CheckSumStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
            break;

        case TRP_WMODE_FIX_PATTERN:
            app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
            break;
            
        case TRP_WMODE_LOOPBACK:
            app_trpc_LoopbackStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
            break;

        case TRP_WMODE_UART:
            app_trpc_UartStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
            break;

        default:
            break;
    }
}


void APP_TRPC_Init(void)
{
    memset((uint8_t *) &s_trpcTrafficPriority, 0, sizeof(APP_TRP_TrafficPriority_T));
    s_trpcTrafficPriority.validNumber = APP_TRP_MAX_TRANSMIT_NUM;
}

void APP_TRPC_EventHandler(BLE_TRSPC_Event_T *p_event)
{
    APP_TRP_ConnList_T *p_trpcConnLink = NULL;
    uint16_t status;

    switch(p_event->eventId)
    {
        case BLE_TRSPC_EVT_UL_STATUS:
        {
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onUplinkStatus.p_dev);
            
            if (p_trpcConnLink != NULL)
            {
                p_trpcConnLink->workMode = TRP_WMODE_NULL;
                p_trpcConnLink->trpState = 0;
                if (p_event->eventField.onUplinkStatus.status == BLE_TRSPC_UL_STATUS_CBFCENABLED)
                {
                    bt_shell_printf("TRP up link is established\n");
                    //app_trpc_WmodeStateMachine(p_trpcConnLink);
                    p_trpcConnLink->workMode = TRP_WMODE_UART;
                    p_trpcConnLink->trpState = TRPC_UART_STATE_NULL;
                    app_trpc_UartStateMachine(APP_TRPC_EVENT_NULL, p_trpcConnLink);

                }
            }
        }
        break;

        case BLE_TRSPC_EVT_DL_STATUS:
        {
            APP_TRP_TYPE_T trpLinkType;
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onDownlinkStatus.p_dev);
            
            if (p_trpcConnLink != NULL )
            {
                trpLinkType = p_trpcConnLink->type;
                    
                if (p_event->eventField.onDownlinkStatus.status == BLE_TRSPC_DL_STATUS_DISABLED)
                {
                    p_trpcConnLink->type = APP_TRP_TYPE_UNKNOWN;
                }
                else
                {
                    p_trpcConnLink->type = APP_TRP_TYPE_LEGACY;
                    
                    /*if (p_event->eventField.onDownlinkStatus.status == BLE_TRSPC_DL_STATUS_CBFCENABLED)
                    {
                        p_trpcConnLink->isDlCreditBased = true;
                    }
                    else
                    {
                        p_trpcConnLink->isDlCreditBased = false;
                    }*/
                    
                    if (trpLinkType == APP_TRP_TYPE_UNKNOWN && p_trpcConnLink->type != trpLinkType)
                    {
                        bt_shell_printf("TRP down link is established\n");
                    }
                    
                    if (p_event->eventField.onDownlinkStatus.currentCreditNumber)
                    {
                        uint8_t trpIdx = APP_TRP_COMMON_GetConnIndex(p_trpcConnLink);
                        //uint8_t transIdx =  APP_GetFileTransIndex(p_event->eventField.onDownlinkStatus.p_dev);
                        //printf("credit(%d=%d)\n", transIdx, p_event->eventField.onDownlinkStatus.currentCreditNumber);
                        //APP_TRPC_TxProc(p_trpcConnLink);
                        APP_TIMER_SetTimer(APP_TIMER_TRPC_RCV_CREDIT, trpIdx, (void *)p_trpcConnLink, APP_TIMER_1MS);
                    }
                    
                }
            }
        }
        break;

        case BLE_TRSPC_EVT_RECEIVE_DATA:
        {
            uint8_t grpId[] = {TRP_GRPID_NULL, TRP_GRPID_CHECK_SUM, TRP_GRPID_LOOPBACK, TRP_GRPID_FIX_PATTERN,
                TRP_GRPID_UART, TRP_GRPID_REV_LOOPBACK};
                
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onReceiveData.p_dev);
            
            if (p_trpcConnLink != NULL )
            {
                if ((p_trpcConnLink->workMode == TRP_WMODE_NULL) 
                    || (p_trpcConnLink->workMode == TRP_WMODE_CHECK_SUM) 
                    || (p_trpcConnLink->workModeEn == false))
                {
                    status = APP_TRP_COMMON_FreeLeData(p_trpcConnLink);
                    
                    if (status == APP_RES_OOM)
                    {
                        APP_LOG_ERROR("LE_RX: APP_RES_OOM !\n");
                    }
                    APP_TRP_COMMON_SendErrorRsp(p_trpcConnLink, grpId[p_trpcConnLink->workMode]);
                    break;
                }
                app_trpc_LeRxProc(p_trpcConnLink);
            }
        }
        break;

        case BLE_TRSPC_EVT_VENDOR_CMD:
        {
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onVendorCmd.p_dev);
            
            if (p_trpcConnLink == NULL)
                break;
            if (p_event->eventField.onVendorCmd.p_payLoad[0] == APP_TRP_VENDOR_OPCODE_BLE_UART)
                app_trpc_VendorCmdProc(p_trpcConnLink, p_event->eventField.onVendorCmd.payloadLength, p_event->eventField.onVendorCmd.p_payLoad);
        }
        break;

        case BLE_TRSPC_EVT_VENDOR_CMD_RSP:
        {
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onVendorCmdRsp.p_dev);
            
            if (p_trpcConnLink != NULL)
            {
                uint8_t trpIdx = APP_TRP_COMMON_GetConnIndex(p_trpcConnLink);
                //printf("VENDOR_RSP(%d)\n", p_event->eventField.onDataRsp.result);
                if (p_event->eventField.onDataRsp.result == BLE_TRSPC_SEND_RESULT_SUCCESS)
                {
                    //continue
                    p_trpcConnLink->gattcRspWait = 0;
                    app_trpc_VendorCmdRspProc(p_trpcConnLink);
                }
                else
                {
                    //retry
                    APP_TIMER_SetTimer(APP_TIMER_TRP_VND_RETRY, trpIdx, (void *)p_trpcConnLink, APP_TIMER_1MS);
                }
            }
        }
        break;

        case BLE_TRSPC_EVT_DATA_RSP:
        {
            p_trpcConnLink = APP_TRP_COMMON_GetConnListByDevProxy(p_event->eventField.onDataRsp.p_dev);

            if(p_trpcConnLink != NULL)
            {
                uint8_t transIndex = APP_GetFileTransIndex(p_trpcConnLink->p_deviceProxy);
                
                if(p_event->eventField.onDataRsp.result == BLE_TRSPC_SEND_RESULT_SUCCESS)
                {
                    //printf("DRSP(Q=%d,I=%d)\n", p_trpcConnLink->uartCircQueue.usedNum, transIndex);
                    //clear waiting
                    p_trpcConnLink->gattcRspWait = 0;

                    if (p_trpcConnLink->workMode == TRP_WMODE_LOOPBACK && p_trpcConnLink->workModeEn == true)
                    {
                        //Fetch pattern data into queue
                        APP_TIMER_SetTimer(APP_TIMER_FILE_FETCH, transIndex, (void*)p_trpcConnLink->p_deviceProxy, APP_TIMER_10MS);
                    }
                    else if (p_trpcConnLink->workMode == TRP_WMODE_UART && p_trpcConnLink->workModeEn == true)
                    {
                        //Fetch console data into queue
                        APP_TIMER_SetTimer(APP_TIMER_RAW_DATA_FETCH, transIndex, (void*)p_trpcConnLink->p_deviceProxy, APP_TIMER_1MS);
                    }
                    else
                    {
                        APP_TRPC_TxProc(p_trpcConnLink);
                    }
                }
                else
                {
                    //retry
                    APP_TIMER_SetTimer(APP_TIMER_TRP_DAT_RETRY, transIndex, (void *)p_trpcConnLink, APP_TIMER_1MS);
                }
            }
        }
        break;

        default:
            break;
    }
}


void APP_TRPC_TxProc(APP_TRP_ConnList_T * p_trpConn)
{
    APP_TRP_ConnList_T  *p_trpcConnLink = NULL;
    uint8_t checkListFg = false;

    if (p_trpConn == NULL)
    {
        sp_trpcCurrentLink = APP_TRP_COMMON_GetConnListByIndex(s_trpcTrafficPriority.txToken);
    }
    else
    {
        //change token
        APP_TRP_COMMON_AssignToken(p_trpConn, APP_TRP_LINK_TYPE_TX, &s_trpcTrafficPriority);
        sp_trpcCurrentLink = p_trpConn;
    }

    p_trpcConnLink = sp_trpcCurrentLink;
    s_trpcTrafficPriority.validNumber = APP_TRP_MAX_TRANSMIT_NUM;


    while (s_trpcTrafficPriority.validNumber > 0)
    {
        switch(sp_trpcCurrentLink->workMode)
        {
            case TRP_WMODE_CHECK_SUM:
            {
                if ((sp_trpcCurrentLink->trpState == TRPC_CS_STATE_TX) && (sp_trpcCurrentLink->workModeEn == true))
                {
                    app_trpc_CheckSumStateMachine(APP_TRPC_EVENT_TX_LE_DATA, sp_trpcCurrentLink);
                }

                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;
                
            case TRP_WMODE_LOOPBACK:
            {
                if ((sp_trpcCurrentLink->trpState == TRPC_LB_STATE_TRX) && (sp_trpcCurrentLink->workModeEn == true))
                {
                    app_trpc_LoopbackStateMachine(APP_TRPC_EVENT_TX_LE_DATA, sp_trpcCurrentLink);
                }

                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;

            case TRP_WMODE_UART:
            {                
                if (sp_trpcCurrentLink->trpState == TRPC_UART_STATE_RELAY_DATA)
                    app_trpc_UartStateMachine(APP_TRPC_EVENT_TX_LE_DATA, sp_trpcCurrentLink);
                
                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;

            default:
            {
                //Change link
                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;
        }

        //Tx available = 0 need to change link.
        if (sp_trpcCurrentLink->maxAvailTxNumber == 0)
        {
            sp_trpcCurrentLink->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
            sp_trpcCurrentLink = APP_TRP_COMMON_ChangeNextLink(APP_TRP_CLIENT_ROLE, APP_TRP_LINK_TYPE_TX, &s_trpcTrafficPriority);
            checkListFg = true;
        }

        if (checkListFg)
        {
            checkListFg = false;
            if ((p_trpcConnLink == sp_trpcCurrentLink) || (sp_trpcCurrentLink == NULL) || (p_trpcConnLink == NULL))
            {
                break;
            }
        }
    }

}


static void app_trpc_LeRxProc(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t checkListFg = false;
    uint16_t status = APP_RES_SUCCESS;


    
    sp_trpcCurrentLink = p_trpConn;
    
    if (sp_trpcCurrentLink == NULL)
        sp_trpcCurrentLink = APP_TRP_COMMON_ChangeNextLink(APP_TRP_CLIENT_ROLE, APP_TRP_LINK_TYPE_RX, &s_trpcTrafficPriority);
    
    if(sp_trpcCurrentLink == NULL)
        return;


    s_trpcTrafficPriority.validNumber = APP_TRP_MAX_TRANSMIT_NUM;



    while (s_trpcTrafficPriority.validNumber > 0)
    {
        switch (sp_trpcCurrentLink->workMode)
        {
            case TRP_WMODE_FIX_PATTERN:
            {
                app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_RX_LE_DATA, sp_trpcCurrentLink);
                //Break the while loop
                s_trpcTrafficPriority.validNumber = 0;
            }
            break;

            case TRP_WMODE_LOOPBACK:
            {
                app_trpc_LoopbackStateMachine(APP_TRPC_EVENT_RX_LE_DATA, sp_trpcCurrentLink);
                s_trpcTrafficPriority.validNumber = 0;
            }
            break;

            case TRP_WMODE_UART:
            {
                app_trpc_UartStateMachine(APP_TRPC_EVENT_RX_LE_DATA, sp_trpcCurrentLink);
                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;

            default:
            {
                //Change link.
                sp_trpcCurrentLink->maxAvailTxNumber = 0;
            }
            break;
        }

        if ((status == APP_RES_OOM) || (status == APP_RES_INVALID_PARA))
        {
            break;
        }

        // Change to the next link if there is no resource for the dedicated link.
        if (status == APP_RES_NO_RESOURCE)
            sp_trpcCurrentLink->maxAvailTxNumber = 0;

        if (sp_trpcCurrentLink->maxAvailTxNumber == 0)
        {
            sp_trpcCurrentLink->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
            sp_trpcCurrentLink = APP_TRP_COMMON_ChangeNextLink(APP_TRP_CLIENT_ROLE, APP_TRP_LINK_TYPE_RX, &s_trpcTrafficPriority);
            checkListFg = true;
        }
        
        if (checkListFg)
        {
            checkListFg = false;
            if ((sp_trpcCurrentLink == p_trpConn) || (sp_trpcCurrentLink == NULL) || (p_trpConn == NULL))
                break;
        }
    }

}

uint16_t APP_TRPC_LeTxData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data)
{
    uint16_t status = TRSP_RES_SUCCESS;

    if (p_trpConn == NULL || p_data == NULL || len == 0)
        return APP_RES_FAIL;

    if(p_trpConn->gattcRspWait)
    {
        return APP_RES_BUSY;
    }

    status = BLE_TRSPC_SendData(p_trpConn->p_deviceProxy, len, p_data);
    if (status != TRSP_RES_SUCCESS)
        return status;

    p_trpConn->gattcRspWait = APP_TRP_SEND_DATA_FAIL;

    return APP_RES_SUCCESS;
}


void APP_TRPC_ProtocolErrRsp(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t grpId[] = {TRP_GRPID_NULL, TRP_GRPID_CHECK_SUM, TRP_GRPID_LOOPBACK, TRP_GRPID_FIX_PATTERN, 
        TRP_GRPID_UART, TRP_GRPID_REV_LOOPBACK};


    APP_TRP_COMMON_SendErrorRsp(p_trpConn, grpId[p_trpConn->workMode]);
    p_trpConn->workModeEn = false;
    
    switch(p_trpConn->workMode)
    {
        case TRP_WMODE_CHECK_SUM:
        {
            p_trpConn->trpState = TRPC_CS_STATE_SEND_CS;
            app_trpc_CheckSumStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
        }
        break;

        case TRP_WMODE_FIX_PATTERN:
        {
            p_trpConn->trpState = TRPC_FP_STATE_SEND_LAST_NUMBER;
            app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
        }
        break;

        case TRP_WMODE_LOOPBACK:
        {
            p_trpConn->trpState = TRPC_LB_STATE_SEND_STOP_TX;
            app_trpc_LoopbackStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
        }
        break;

        case TRP_WMODE_UART:
        {
            p_trpConn->trpState = TRPC_UART_STATE_DISABLE_MODE;
            app_trpc_UartStateMachine(APP_TRPC_EVENT_NULL, p_trpConn);
        }
        break;

        default:
            break;
    }
}


void APP_TRPC_TransmitModeSwitch(uint8_t mode, APP_TRP_ConnList_T *p_trpConn)
{
    APP_TRP_ConnList_T * p_usedLink = NULL;

    if (p_trpConn != NULL)
    {
        p_usedLink = p_trpConn;
    }
    else
    {
        APP_BLE_ConnList_T * p_bleLink;
        p_bleLink = APP_GetLastOneConnectedBleLink();
        p_usedLink = APP_TRP_COMMON_GetConnListByDevProxy(p_bleLink->p_deviceProxy);
    }

    if (p_usedLink == NULL)
    {
        printf("link not exist\n");
        return;
    }

    switch(mode)
    {
        case TRP_WMODE_CHECK_SUM:
        {
            p_usedLink->workMode = mode;
            p_usedLink->trpState = TRPC_CS_STATE_NULL;
            app_trpc_CheckSumStateMachine(APP_TRPC_EVENT_NULL, p_usedLink);
        }
        break;
        case TRP_WMODE_FIX_PATTERN:
        {
            p_usedLink->workMode = mode;
            p_usedLink->trpState = TRPC_FP_STATE_NULL;
            app_trpc_FixPatternStateMachine(APP_TRPC_EVENT_NULL, p_usedLink);
        }
        break;
        case TRP_WMODE_UART:
        {
            p_usedLink->workMode = mode;
            p_usedLink->trpState = TRPC_UART_STATE_NULL;
            app_trpc_UartStateMachine(APP_TRPC_EVENT_NULL, p_usedLink);
        }
        break;
        case TRP_WMODE_LOOPBACK:
        {
            p_usedLink->workMode = mode;
            p_usedLink->trpState = TRPC_LB_STATE_NULL;
            app_trpc_LoopbackStateMachine(APP_TRPC_EVENT_NULL, p_usedLink);
        }
        break;
        default:
        {
            bt_shell_printf("TransmitMode switch error\n");
        }
        break;
    }
}


void APP_TRPC_RetryVendorCmd(APP_TRP_ConnList_T *p_trpConn)
{
    uint8_t grpId[] = {TRP_GRPID_NULL, TRP_GRPID_CHECK_SUM, TRP_GRPID_LOOPBACK, TRP_GRPID_FIX_PATTERN,
        TRP_GRPID_UART, TRP_GRPID_REV_LOOPBACK};
    uint16_t prevGattcRspWait;

    if (p_trpConn != NULL)
    {
        //printf("RetryVendorCmd, workMode=%d, gattcRspWait=%d\n", p_trpConn->workMode, p_trpConn->gattcRspWait);
        
        prevGattcRspWait = p_trpConn->gattcRspWait;
        p_trpConn->gattcRspWait = 0;

        switch(p_trpConn->workMode)
        {
            case TRP_WMODE_CHECK_SUM:
            {
                if (prevGattcRspWait == APP_TRP_SEND_GID_CS_FAIL)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_CHECK_SUM, 
                        APP_TRP_WMODE_CHECK_SUM_ENABLE);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_LENGTH_FAIL)
                {
                    APP_TRP_COMMON_SendLengthCommand(p_trpConn, APP_TRP_WMODE_TX_MAX_SIZE);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_TYPE_FAIL)
                {
                    APP_TRP_COMMON_SendTypeCommand(p_trpConn);
                }
                else if ((prevGattcRspWait == APP_TRP_SEND_GID_TX_FAIL) 
                    && (p_trpConn->trpState == TRPC_CS_STATE_START_TX))
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, 
                        APP_TRP_WMODE_TX_DATA_START);
                }
                else if ((prevGattcRspWait == APP_TRP_SEND_GID_TX_FAIL) 
                    && (p_trpConn->trpState == TRPC_CS_STATE_STOP_TX))
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, 
                        APP_TRP_WMODE_TX_DATA_END);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_CHECK_SUM_FAIL)
                {
                    APP_TRP_COMMON_SendCheckSumCommand(p_trpConn);
                }
            }
            break;

            case TRP_WMODE_FIX_PATTERN:
            {
                if (prevGattcRspWait == APP_TRP_SEND_GID_FP_FAIL)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_FIX_PATTERN, 
                        APP_TRP_WMODE_FIX_PATTERN_ENABLE);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_TYPE_FAIL)
                {
                    APP_TRP_COMMON_SendTypeCommand(p_trpConn);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_GID_TX_FAIL)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, 
                        APP_TRP_WMODE_TX_DATA_START);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_LAST_NUMBER_FAIL)
                {
                    APP_TRP_COMMON_SendLastNumber(p_trpConn);
                }
            }
            break;
                
            case TRP_WMODE_LOOPBACK:
            {
                if (prevGattcRspWait == APP_TRP_SEND_GID_LB_FAIL)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_LOOPBACK, 
                        APP_TRP_WMODE_LOOPBACK_ENABLE);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_TYPE_FAIL)
                {
                    APP_TRP_COMMON_SendTypeCommand(p_trpConn);
                }
                else if ((prevGattcRspWait == APP_TRP_SEND_GID_TX_FAIL)
                    && (p_trpConn->trpState == TRPC_LB_STATE_START_TX))
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, 
                        APP_TRP_WMODE_TX_DATA_START);
                }
                else if ((prevGattcRspWait == APP_TRP_SEND_GID_TX_FAIL)
                    && (p_trpConn->trpState == TRPC_LB_STATE_SEND_STOP_TX))
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_TRANSMIT, 
                        APP_TRP_WMODE_TX_DATA_END);
                }
            }
            break;

            case TRP_WMODE_UART:
            {
                if (prevGattcRspWait == APP_TRP_SEND_GID_UART_FAIL)
                {
                    APP_TRP_COMMON_SendModeCommand(p_trpConn, TRP_GRPID_UART, APP_TRP_WMODE_UART_ENABLE);
                }
                else if (prevGattcRspWait == APP_TRP_SEND_TYPE_FAIL)
                {
                    APP_TRP_COMMON_SendTypeCommand(p_trpConn);
                }
            }
            break;

            default:
                break;
        }

        if (prevGattcRspWait == APP_TRP_SEND_ERROR_RSP_FAIL)
        {
            APP_TRP_COMMON_SendErrorRsp(p_trpConn, grpId[p_trpConn->workMode]);
        }
    }
}

void APP_TRPC_RetryData(APP_TRP_ConnList_T *p_trpConn)
{
    APP_TRPC_TxProc(p_trpConn);
}


