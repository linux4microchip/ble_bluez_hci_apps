/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application State Machine Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_key.c

  Summary:
    This file implement the Application State Machine.

  Description:
    This file implement the Application State Machine.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "app_sm.h"
#include "app_adv.h"
#include "app_scan.h"
#include "app_ble_handler.h"
#include "app_log.h"
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

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_SM_STATE_T s_appSmState;

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************

typedef APP_SM_STATE_T (*APP_SM_StateProc) (APP_SM_Event_T evt);

static APP_SM_STATE_T APP_SM_StateProcOff(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcStandby(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcAdvertising(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcScanning(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcInitiating(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcConnection(APP_SM_Event_T evt);
static APP_SM_STATE_T APP_SM_StateProcShutDown(APP_SM_Event_T evt);

static const APP_SM_StateProc s_appSmStateProc[APP_SM_STATE_MAX]=
{ 
    APP_SM_StateProcOff,
    APP_SM_StateProcStandby,
    APP_SM_StateProcAdvertising,
    APP_SM_StateProcScanning,
    APP_SM_StateProcInitiating,
    APP_SM_StateProcConnection,
    APP_SM_StateProcShutDown
};


static APP_SM_STATE_T APP_SM_StateProcOff(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            break;
        case APP_SM_EVENT_EXIT_STATE:
            break;
        case APP_SM_EVENT_POWER_ON:
            changedState = APP_SM_STATE_STANDBY;
            break;
        default:
            break;
    }
    
    return changedState;
}

static APP_SM_STATE_T APP_SM_StateProcStandby(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            break;
        case APP_SM_EVENT_EXIT_STATE:
            break;
        case APP_SM_EVENT_ADV_ON:
            {
                changedState = APP_SM_STATE_ADVERTISING;
            }
            break;
        case APP_SM_EVENT_SCANNING_ON:
            {
                changedState = APP_SM_STATE_SCANNING;
            }
            break;
        case APP_SM_EVENT_CONNECTED: //controller has already connected with peer and then host bring up
            changedState = APP_SM_STATE_CONNECTION;
            break;
        case APP_SM_EVENT_POWER_OFF:
            changedState = APP_SM_STATE_SHUTDOWN;
        default:
            break;
    }
    
    return changedState;
}


static APP_SM_STATE_T APP_SM_StateProcAdvertising(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            APP_ADV_Start();
            break;
        case APP_SM_EVENT_EXIT_STATE:
            APP_ADV_Stop();
            break;
        case APP_SM_EVENT_CONNECTED:
            changedState = APP_SM_STATE_CONNECTION;
            break;
        case APP_SM_EVENT_SCANNING_ON:
            changedState = APP_SM_STATE_SCANNING;
            break;
        case APP_SM_EVENT_DISCONNECTED:
            {
                if (APP_GetConnLinkNum())
                    changedState = APP_SM_STATE_CONNECTION;
                else
                    changedState = APP_SM_STATE_STANDBY;
            }
            break;
        case APP_SM_EVENT_ADV_OFF:
            changedState = APP_SM_STATE_STANDBY;
            break;
        case APP_SM_EVENT_POWER_OFF:
            changedState = APP_SM_STATE_SHUTDOWN;
        default:
            break;
    }
    
    return changedState;
}

static APP_SM_STATE_T APP_SM_StateProcScanning(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            APP_SCAN_Start();
            break;
        case APP_SM_EVENT_EXIT_STATE:
            APP_SCAN_Stop();
            break;
        case APP_SM_EVENT_ADV_ON:
            {
                if (APP_GetConnLinkNum() < APP_BLE_MAX_LINK_NUMBER)
                    changedState = APP_SM_STATE_ADVERTISING;
            }
            break;
        case APP_SM_EVENT_SCANNING_OFF:
            {
                if (APP_GetConnLinkNum())
                    changedState = APP_SM_STATE_CONNECTION;
                else
                    changedState = APP_SM_STATE_STANDBY;
            }
            break;
        case APP_SM_EVENT_DISCONNECTED:
            {
                if (APP_GetConnLinkNum())
                    changedState = APP_SM_STATE_CONNECTION;
                else
                    changedState = APP_SM_STATE_STANDBY;
            }
            break;
        case APP_SM_EVENT_CONNECTING_START:
            changedState = APP_SM_STATE_INITIATING;
            break;
        default:
            break;
    }
    
    return changedState;
}

static APP_SM_STATE_T APP_SM_StateProcInitiating(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            break;
        case APP_SM_EVENT_EXIT_STATE:
            APP_SCAN_Stop();
            break;
        case APP_SM_EVENT_CONNECTING_STOP:
        case APP_SM_EVENT_DISCONNECTED:
            {
                if (APP_GetConnLinkNum())
                    changedState = APP_SM_STATE_CONNECTION;
                else
                    changedState = APP_SM_STATE_STANDBY;
            }
            break;
        case APP_SM_EVENT_CONNECTED:
            changedState = APP_SM_STATE_CONNECTION;
            break;
        case APP_SM_EVENT_POWER_OFF:
            APP_DBP_DisconnectAll(); //cancel connecting
            changedState = APP_SM_STATE_SHUTDOWN;
            break;
        default:
            break;
    }
    
    return changedState;
}

static APP_SM_STATE_T APP_SM_StateProcConnection(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            break;
        case APP_SM_EVENT_EXIT_STATE:
            break;
        case APP_SM_EVENT_ADV_ON:
            if (APP_GetConnLinkNum() < APP_BLE_MAX_LINK_NUMBER)
                changedState = APP_SM_STATE_ADVERTISING;
            break;
        case APP_SM_EVENT_SCANNING_ON:
            changedState = APP_SM_STATE_SCANNING;
            break;
        case APP_SM_EVENT_DISCONNECTED:
            {
                uint8_t num = APP_GetConnLinkNum();
                if (num == 0)
                {
                    changedState = APP_SM_STATE_STANDBY;
                }
            }
            break;
        case APP_SM_EVENT_POWER_OFF:
            APP_DBP_DisconnectAll();
            changedState = APP_SM_STATE_SHUTDOWN;
            break;
        default:
            break;
    }
    
    return changedState;
}

static APP_SM_STATE_T APP_SM_StateProcShutDown(APP_SM_Event_T evt)
{
    APP_SM_STATE_T changedState = s_appSmState;
        
    switch (evt)
    {
        case APP_SM_EVENT_ENTER_STATE:
            break;
        case APP_SM_EVENT_EXIT_STATE:
            break;
        case APP_SM_EVENT_DISCONNECTED:
            if (APP_GetConnLinkNum() == 0)
            {
                changedState = APP_SM_STATE_OFF;
            }
        break;
        default:
            break;
    }
    
    return changedState;
}


APP_SM_STATE_T APP_SM_GetSmState(void)
{
    return s_appSmState;
}


void APP_SM_Handler(APP_SM_Event_T evt)
{
    APP_SM_STATE_T currState = s_appSmState;
    APP_SM_STATE_T newState = s_appSmState;


    newState = s_appSmStateProc[currState](evt);

    //state transition
    if (newState != currState) 
    {
        s_appSmStateProc[currState](APP_SM_EVENT_EXIT_STATE);

        s_appSmState = newState;

        s_appSmStateProc[newState](APP_SM_EVENT_ENTER_STATE);
        
        //LOG("SM state %d -> %d\n", currState, newState);
    }
    
}



void APP_SM_Init(void)
{
    s_appSmState = APP_SM_STATE_OFF;
}



