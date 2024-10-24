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
    app_scan.c

  Summary:
    This file contains the Application scan functions for this project.

  Description:
    This file contains the Application scan functions for this project.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>

#include "application.h"
#include "app_ble_handler.h"
#include "app_scan.h"
#include "app_timer.h"
#include "app_error_defs.h"
#include "app_sm.h"
#include "app_dbp.h"


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
APP_SCAN_Filter_T s_scanFilter;


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

uint16_t APP_SCAN_Ctrl(uint8_t enable)
{
    uint16_t result = APP_RES_BAD_STATE;

    APP_BLE_ConnList_T *p_bleConn = NULL;
    p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_SCANNING, APP_BLE_STATE_SCANNING);

    if (enable)
    {
        if (p_bleConn == NULL)
        {
            p_bleConn = APP_GetFreeConnList();
        
            if (p_bleConn == NULL)
                return APP_RES_NO_RESOURCE;

            result = APP_DBP_StartScan(APP_DBP_GetDefaultAdapter());
            if (result == APP_RES_SUCCESS)
            {
                APP_SetBleStateByLink(p_bleConn, APP_BLE_STATE_SCANNING);
                APP_SM_Handler(APP_SM_EVENT_SCANNING_ON);
                APP_TIMER_SetTimer(APP_TIMER_SCAN, 0, NULL, APP_TIMER_5S);
            }
        }
    }
    else
    {
        if (p_bleConn != NULL)
        {
            APP_TIMER_StopTimer(APP_TIMER_SCAN, 0);
            result = APP_DBP_StopScan(APP_DBP_GetDefaultAdapter());
            if(result == APP_RES_SUCCESS)
            {
                APP_SetBleStateByLink(p_bleConn, APP_BLE_STATE_STANDBY);
                APP_SM_Handler(APP_SM_EVENT_SCANNING_OFF);
            }
        }
    }

    return result;
}

void APP_SCAN_Start(void)
{
    APP_SCAN_Ctrl(true);
}

void APP_SCAN_Stop(void)
{
    APP_SCAN_Ctrl(false);
}

bool APP_SCAN_SetFilter(APP_SCAN_Filter_T *p_scanFilter)
{
    APP_SCAN_ClearFilter();

    memcpy(&s_scanFilter, p_scanFilter, sizeof(APP_SCAN_Filter_T));
    
    if (p_scanFilter->p_pattern)
    {
        s_scanFilter.p_pattern = strdup(p_scanFilter->p_pattern);
        if (s_scanFilter.p_pattern == NULL)
            return false;
    }
    if (p_scanFilter->pp_uuids)
    {
        s_scanFilter.pp_uuids = g_strdupv(p_scanFilter->pp_uuids);
        if (s_scanFilter.pp_uuids == NULL)
            return false;
    }
    if (p_scanFilter->manufDataLen)
    {
        s_scanFilter.p_manufData = malloc(p_scanFilter->manufDataLen);
        if (s_scanFilter.p_manufData == NULL)
            return false;
        memcpy(s_scanFilter.p_manufData, p_scanFilter->p_manufData, p_scanFilter->manufDataLen);
    }
    if (p_scanFilter->p_srvUuid)
    {
        s_scanFilter.p_srvUuid = strdup(p_scanFilter->p_srvUuid);
        if (s_scanFilter.p_srvUuid == NULL)
            return false;
    }
    
    s_scanFilter.isConfigured = true;
    
    return true;
}

APP_SCAN_Filter_T * APP_SCAN_GetFilter(void)
{
    return &s_scanFilter;
}


void APP_SCAN_ClearFilter(void)
{
    if (s_scanFilter.p_pattern)
        free(s_scanFilter.p_pattern);
    if (s_scanFilter.pp_uuids)
        g_strfreev(s_scanFilter.pp_uuids);
    if (s_scanFilter.p_manufData)
        free(s_scanFilter.p_manufData);
    if (s_scanFilter.p_srvUuid)
        free(s_scanFilter.p_srvUuid);
        
    APP_SCAN_Init();
}

void APP_SCAN_Init(void)
{
    memset(&s_scanFilter, 0, sizeof(APP_SCAN_Filter_T));
    s_scanFilter.rssi = APP_SCAN_DEFAULT_FILTER_RSSI;
}


