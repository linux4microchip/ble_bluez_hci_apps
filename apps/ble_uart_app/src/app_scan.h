/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Timer Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_scan.h

  Summary:
    This file contains the Application scan functions for this project.

  Description:
    This file contains the Application scan functions for this project.
    Including the Set/Stop/Reset timer and timer expired handler.
 *******************************************************************************/

#ifndef APP_SCAN_H
#define APP_SCAN_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include "app_gap.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_SCAN_DEFAULT_FILTER_RSSI                0x7FFF //invalid, same as 
#define APP_SCAN_UUID_128_BASE_STR "00000000-0000-1000-8000-00805f9b34fb"



// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
typedef struct APP_SCAN_Filter_T {
    int rssi;
    char *p_pattern;
    char **pp_uuids;
    uint16_t manufId;
    int manufDataLen;
    uint8_t *p_manufData;
    char *p_srvUuid;
    bool isFilterManufData;
    bool isFilterSrvUuid;
    bool isConfigured;
} APP_SCAN_Filter_T;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
void APP_SCAN_Init(void);
void APP_SCAN_Start(void);
void APP_SCAN_Stop(void);
bool APP_SCAN_SetFilter(APP_SCAN_Filter_T *p_scanFilter);
APP_SCAN_Filter_T * APP_SCAN_GetFilter(void);
void APP_SCAN_ClearFilter(void);


#endif
