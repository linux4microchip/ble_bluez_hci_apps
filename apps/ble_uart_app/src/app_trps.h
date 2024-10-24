/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Server Role Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trps.h

  Summary:
    This file contains the Application Transparent Server Role functions for this project.

  Description:
    This file contains the Application Transparent Server Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

#ifndef APP_TRPS_H
#define APP_TRPS_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "app_gap.h"
#include "app_trp_common.h"
#include "ble_trsp/ble_trsps.h"



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
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
void APP_TRPS_Init(void);
uint16_t APP_TRPS_LeTxData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data);
void APP_TRPS_EventHandler(BLE_TRSPS_Event_T *p_event);
void APP_TRPS_TxProc(APP_TRP_ConnList_T * p_trpConn);

#endif
