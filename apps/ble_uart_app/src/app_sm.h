/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application State Machine Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_key.h

  Summary:
    This file contains the Application State Machine API.

  Description:
    This file contains the Application State Machine API.
 *******************************************************************************/

#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************


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
/**@brief The structure contains the Application State in State Machine. */

typedef enum _APP_SM_STATE_T
{
    APP_SM_STATE_OFF,
    APP_SM_STATE_STANDBY,
    APP_SM_STATE_ADVERTISING,
    APP_SM_STATE_SCANNING,
    APP_SM_STATE_INITIATING,
    APP_SM_STATE_CONNECTION,
    APP_SM_STATE_SHUTDOWN,
    APP_SM_STATE_MAX
}APP_SM_STATE_T;

typedef enum _APP_SM_EvtMsg_T
{
    APP_SM_EVENT_ENTER_STATE,            //reserved
    APP_SM_EVENT_EXIT_STATE,             //reserved
    APP_SM_EVENT_POWER_ON,
    APP_SM_EVENT_POWER_OFF,
    APP_SM_EVENT_ADV_ON,
    APP_SM_EVENT_ADV_OFF,
    APP_SM_EVENT_SCANNING_ON,
    APP_SM_EVENT_SCANNING_OFF,
    APP_SM_EVENT_CONNECTING_START,
    APP_SM_EVENT_CONNECTING_STOP,
    APP_SM_EVENT_CONNECTED,
    APP_SM_EVENT_DISCONNECTED,
}APP_SM_Event_T;

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

APP_SM_STATE_T APP_SM_GetSmState(void);
void APP_SM_Handler(APP_SM_Event_T evt);
void APP_SM_Init(void);


#endif
/*******************************************************************************
 End of File
 */


