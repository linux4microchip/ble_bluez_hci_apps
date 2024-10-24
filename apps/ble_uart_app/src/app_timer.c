/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Timer Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_timer.c

  Summary:
    This file contains the Application Timer functions for this project.

  Description:
    This file contains the Application Timer functions for this project.
    Including the Set/Stop timer and timer expired handler.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <glib.h>

#include "shared/shell.h"

#include "application.h"
#include "app_timer.h"
#include "app_error_defs.h"
#include "app_scan.h"
#include "app_log.h"
#include "app_dbp.h"
#include "app_trpc.h"
#include "app_trps.h"



// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_TMR_ID_INST(id, inst) ((((uint16_t)id) << 8) | inst)
#define APP_TMR_ID(inst) (inst >> 8)
#define APP_TMR_INST(inst) (inst & 0xFF)



// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef struct APP_TIMER_Elem_T
{
    uint16_t        tmrIdInst;           /**< timer Id of compound message with instance */
    guint           tmrHandle;           /**< timer handle */ 
    void            *p_tmrParam;         /**< timer parameter */
} APP_TIMER_Elem_T;




// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static GList  *sp_timerList;
static GMutex  s_mutex;



// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static void app_timer_RemoveTimer(APP_TIMER_TimerId_T tmrId, uint8_t instance)
{
    GList *p_l;
    APP_TIMER_Elem_T *p_tmr;
    uint16_t idInst = APP_TMR_ID_INST(tmrId, instance);

    g_mutex_lock(&s_mutex);
    for (p_l=sp_timerList; p_l; p_l=g_list_next(p_l))  {
        p_tmr = p_l->data;
        if (p_tmr->tmrIdInst == idInst)
        {
            sp_timerList = g_list_remove(sp_timerList, p_tmr);
            break;
        }
    }

    g_mutex_unlock(&s_mutex);
}


static int APP_TIMER_TimeoutHandle(gpointer userData)
{
    APP_TIMER_Elem_T *p_tmr;

    p_tmr = (APP_TIMER_Elem_T *)userData;

    if (APP_TMR_ID(p_tmr->tmrIdInst) < APP_TIMER_PERIODIC_START)
        app_timer_RemoveTimer(APP_TMR_ID(p_tmr->tmrIdInst), APP_TMR_INST(p_tmr->tmrIdInst));

    switch(APP_TMR_ID(p_tmr->tmrIdInst))
    {
        case APP_TIMER_PROTOCOL_RSP:
        {
            APP_TRP_ConnList_T *p_trpConn = p_tmr->p_tmrParam;
            
            if ((p_trpConn != NULL) && (p_trpConn->trpRole == APP_TRP_CLIENT_ROLE))
            {
                APP_TRPC_ProtocolErrRsp(p_trpConn);
            }
        }
        break;

        case APP_TIMER_TRP_VND_RETRY:
        {
            APP_TRP_ConnList_T *p_trpConn = p_tmr->p_tmrParam;
            if (p_trpConn != NULL)
            {
                APP_TRPC_RetryVendorCmd(p_trpConn);
            }
        }
        break;

        case APP_TIMER_TRP_DAT_RETRY:
        {
            APP_TRP_ConnList_T *p_trpConn = p_tmr->p_tmrParam;
            if (p_trpConn != NULL)
            {
                APP_TRPC_RetryData(p_trpConn);
            }
        }
        break;


        case APP_TIMER_UART_SEND:
        {
            APP_TRP_ConnList_T *p_trpConn = p_tmr->p_tmrParam;
            if (p_trpConn != NULL)
                APP_TRP_COMMON_SendTrpProfileDataToUART(p_trpConn);
        }
        break;


        case APP_TIMER_CHECK_MODE:
        {
            if(APP_ConfirmWorkMode(p_tmr->p_tmrParam) == false)
            {
                APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, APP_TMR_ID(p_tmr->tmrIdInst), p_tmr->p_tmrParam, APP_TIMER_500MS);
            }
            else
            {
                if (APP_GetWorkMode() == TRP_WMODE_LOOPBACK)
                {
                    bt_shell_printf("loopback start\n");
                    APP_TIMER_SetTimer(APP_TIMER_FILE_FETCH, APP_TMR_ID(p_tmr->tmrIdInst), p_tmr->p_tmrParam, APP_TIMER_1MS);
                }
                else if (APP_GetWorkMode() == TRP_WMODE_UART)
                {
                    APP_TRP_ConnList_T *p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy((DeviceProxy *)p_tmr->p_tmrParam);

                    bt_shell_printf("Sending data to remote peer.\n");
                    APP_TIMER_SetTimer(APP_TIMER_RAW_DATA_FETCH, APP_TMR_ID(p_tmr->tmrIdInst), p_tmr->p_tmrParam, APP_TIMER_1MS);
                    if (p_trpConn != NULL && p_trpConn->trpRole == APP_TRP_SERVER_ROLE)
                    {
                        p_trpConn->workModeEn = true;
                        p_trpConn->maxAvailTxNumber = APP_TRP_MAX_TX_AVAILABLE_TIMES;
                        APP_TIMER_SetTimer(APP_TIMER_TRPS_RCV_CREDIT, 0, p_trpConn, APP_TIMER_1MS);
                    }
                }
            }
        }
        break;

        case APP_TIMER_CHECK_MODE_ONLY:
        {
            if(APP_ConfirmWorkMode(p_tmr->p_tmrParam) == false)
            {
                APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE, APP_TMR_ID(p_tmr->tmrIdInst), p_tmr->p_tmrParam, APP_TIMER_500MS);
            }
            else
            {
                if (APP_GetWorkMode()==TRP_WMODE_UART)
                {
                    bt_shell_printf("waiting for data sending from remote peer.\n");
                }
            }
        }

        case APP_TIMER_CHECK_MODE_FOR_ALL:
        {
            if(APP_ConfirmWorkModeAll() == false)
            {
                APP_TIMER_SetTimer(APP_TIMER_CHECK_MODE_FOR_ALL, 0, NULL, APP_TIMER_500MS);
            }
            else
            {
                if (APP_GetWorkMode() == TRP_WMODE_LOOPBACK)
                {
                    APP_FetchTxDataAll();
                }
            }
        }
        break;

        case APP_TIMER_FILE_FETCH:
        {
            APP_FetchTxDataFromPatternFile(p_tmr->p_tmrParam);
        }
        break;

        case APP_TIMER_RAW_DATA_FETCH:
        {
            APP_FetchTxDataFromRawDataFile(p_tmr->p_tmrParam);
        }
        break;

        case APP_TIMER_LOOPBACK_RX_CHECK:
        {
            bt_shell_printf("Loopback Rx timeout\n");
            APP_FileWriteTimeout(p_tmr->p_tmrParam);
        }
        break;

        case APP_TIMER_RAW_DATA_RX_CHECK:
        {
            bt_shell_printf("\nRaw Data Rx finished\n");
            APP_RawDataFileWriteTimeout(p_tmr->p_tmrParam);
        }
        break;
        
        case APP_TIMER_SCAN:
        {
            bt_shell_printf("Discovery stopped\n");
            APP_SCAN_Stop();
            APP_DBP_PrintDeviceList();
        }
        break;

        case APP_TIMER_TRPS_RCV_CREDIT:
        {
            APP_TRPS_TxProc(p_tmr->p_tmrParam);
        }
        break;

        case APP_TIMER_TRPS_PROGRESS_CHECK:
        {
            bt_shell_printf("TRPS transmission done\n");
        }
        break;

        case APP_TIMER_TRPC_RCV_CREDIT:
        {
            APP_TRPC_TxProc(p_tmr->p_tmrParam);
        }
        break;

        case APP_TIMER_DATA_BUFFER_OVERFLOW_EVT:
        {
            bt_shell_printf("Data buffer overflow detected.\n");
        }
        break;

        case APP_TIMER_AUTO_NEXT_RUN:
        {
            APP_BurstModeStartAll();
        }
        break;
        
        default:
        break;

    }

    if (APP_TMR_ID(p_tmr->tmrIdInst) >= APP_TIMER_PERIODIC_START)
        return true;

    if (p_tmr->tmrHandle)
        g_source_remove(p_tmr->tmrHandle);
    g_free(p_tmr);
        
    return false;

}

uint16_t APP_TIMER_StopTimer(APP_TIMER_TimerId_T tmrId, uint8_t instance)
{
    GList *p_l;
    bool isFound = false;
    APP_TIMER_Elem_T *p_tmr;
    uint16_t idInst = APP_TMR_ID_INST(tmrId, instance);


    g_mutex_lock(&s_mutex);
    for (p_l=sp_timerList; p_l; p_l=g_list_next(p_l))  {
        p_tmr = p_l->data;
        if (p_tmr->tmrIdInst == idInst)
        {
            if (p_tmr->tmrHandle)
                g_source_remove(p_tmr->tmrHandle);
            
            sp_timerList = g_list_remove(sp_timerList, p_tmr);
            g_free(p_tmr);
            isFound = true;
            break;
        }
    }

    g_mutex_unlock(&s_mutex);

    if (isFound)
        return APP_RES_SUCCESS;

    return APP_RES_FAIL;
}

uint16_t APP_TIMER_SetTimer(APP_TIMER_TimerId_T tmrId, uint8_t instance, void *p_tmrParam, uint32_t timeout)
{
    guint tmrHandle;
    APP_TIMER_Elem_T *p_tmrNew;

    //Stop and remove the timer if it already exists.
    APP_TIMER_StopTimer(tmrId, instance);

    //Append the new timer to the list
    p_tmrNew = g_new0(APP_TIMER_Elem_T, 1);
    if (p_tmrNew == NULL)
        return APP_RES_OOM;

    p_tmrNew->tmrIdInst = APP_TMR_ID_INST(tmrId, instance);
    p_tmrNew->p_tmrParam = p_tmrParam;

    tmrHandle = g_timeout_add(timeout, APP_TIMER_TimeoutHandle, p_tmrNew);
    if (tmrHandle == 0)
    {
        g_free(p_tmrNew);
        return APP_RES_FAIL;
    }

    p_tmrNew->tmrHandle = tmrHandle;
    g_mutex_lock(&s_mutex);
    sp_timerList = g_list_append(sp_timerList, p_tmrNew);
    g_mutex_unlock(&s_mutex);


    return APP_RES_SUCCESS;
}

