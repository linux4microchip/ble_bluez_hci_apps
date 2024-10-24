/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Client Role Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trpc.h

  Summary:
    This file contains the Application Transparent Client Role functions for this project.

  Description:
    This file contains the Application Transparent Client Role functions for this project.
    The implementation of demo mode is included in this file.
 *******************************************************************************/

#ifndef APP_TRPC_H
#define APP_TRPC_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "app_trp_common.h"
#include "ble_trsp/ble_trspc.h"

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
void APP_TRPC_Init(void);
void APP_TRPC_EventHandler(BLE_TRSPC_Event_T *p_event);
uint16_t APP_TRPC_LeTxData(APP_TRP_ConnList_T *p_trpConn, uint16_t len, uint8_t *p_data);
void APP_TRPC_ProtocolErrRsp(APP_TRP_ConnList_T *p_trpConn);
void APP_TRPC_RetryVendorCmd(APP_TRP_ConnList_T *p_trpConn);
void APP_TRPC_RetryData(APP_TRP_ConnList_T *p_trpConn);
void APP_TRPC_TransmitModeSwitch(uint8_t mode, APP_TRP_ConnList_T *p_trpConn);
void APP_TRPC_TxProc(APP_TRP_ConnList_T *p_trpConn);



#endif

