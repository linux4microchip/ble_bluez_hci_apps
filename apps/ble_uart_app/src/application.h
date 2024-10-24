/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    application.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides prototypes and definitions for the application.

*******************************************************************************/

#ifndef _APPLICATION_H
#define _APPLICATION_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "app_trp_common.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

#define APP_VERSION VERSION

//#define APP_ADV_TYPE_EXT
#define APP_ADV_TYPE_LEGACY

/*
To enable Debug feature in BlueZ, there are two levels enabling. (This method may only be effective in normal Linux environment but not in Sam4Linux kernel)
1. Enabling BlueZ kernel part debug log. To build kernel may needed, you should turn on "CONFIG_BT_FEATURE_DEBUG" in kernel menu config. 
   And then enable following "ENABLE_BLUEZ_DEBUG" to send MGMT_OP_SET_EXP_FEATURE op.
2. Enabling BlueZ userspace part debug log. You should have root privilege and modify service starting config file : "/lib/systemd/system/bluetooth.service".
   Append "-d -E" option at tail of the line of "bluetoothd" exists.
*/
//#define ENABLE_BLUEZ_DEBUG
#define ENABLE_AUTO_RUN
#define ENABLE_DATA_BUFFER_OVERFLOW_MONITOR
//#define ENABLE_EXP_MULTI_ROLE



#define BLE_ATT_DEFAULT_MTU_LEN                             (23U)                  /**< ATT default MTU length. */
#define BLE_ATT_MAX_MTU_LEN                                 (247U)                 /**< The Maximum supported MTU length of ATT stack. */
#define ATT_HANDLE_VALUE_HEADER_SIZE                        (3U)                   /**< The ATT Handle Value Notification/Indication Header Size. */
#define ATT_WRITE_HEADER_SIZE                               (3U)                   /**< The ATT Write Request/Command Header Size. */
#define ATT_MULTI_EVENT_NOTIFY_SINGLE_VALUE_PAIR            (2U)

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

enum APP_PATTERN_FILE
{
    APP_PATTERN_FILE_TYPE_1K,
    APP_PATTERN_FILE_TYPE_5K,
    APP_PATTERN_FILE_TYPE_10K,
    APP_PATTERN_FILE_TYPE_50K,
    APP_PATTERN_FILE_TYPE_100K,
    APP_PATTERN_FILE_TYPE_200K,
    APP_PATTERN_FILE_TYPE_500K,
    APP_PATTERN_FILE_TYPE_MAX,
};



// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************
/* These routines are called by drivers when certain events occur.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************


void APP_Initialize ( void );
void APP_Deinitialize(void);
void APP_SendRawData(APP_TRP_ConnList_T *p_trpConn, char * p_data);
void APP_SendRawDataFromFile(APP_TRP_ConnList_T *p_trpConn, char * p_filePath);
void APP_ReceiveRawDataToFile(APP_TRP_ConnList_T *p_trpConn, char * p_filePath);
uint32_t APP_RawDataRemaining(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_OutputWrite(APP_TRP_ConnList_T *p_trpConn, uint16_t length, uint8_t *p_buffer);
void APP_SetWorkMode(uint8_t mode);
void APP_FetchTxDataFromPatternFile(DeviceProxy * p_devProxy);
void APP_FetchTxDataFromRawDataFile(DeviceProxy *p_deviceProxy);
void APP_FetchTxDataAll(void);
uint16_t APP_FileRead(DeviceProxy * p_devProxy, uint8_t *p_buffer, uint16_t len);
uint16_t APP_FileWrite(DeviceProxy * p_devProxy, uint16_t length, uint8_t *p_buffer);
uint16_t APP_ConsoleRead(DeviceProxy * p_devProxy, uint8_t *p_buffer, uint16_t len);
uint16_t APP_ConsoleWrite(DeviceProxy * p_devProxy, uint16_t length, uint8_t *p_buffer);
void APP_FileWriteTimeout(void *p_param);
void APP_RawDataFileWriteTimeout(void *p_param);
void APP_PreparePatternData(uint8_t patternFileType);
void APP_BurstModeStart(uint8_t devIndex);
void APP_BurstModeStartAll(void);
uint8_t APP_GetWorkMode(void);
bool APP_ConfirmWorkMode(DeviceProxy * p_devProxy);
bool APP_ConfirmWorkModeAll(void);
void APP_DeviceConnected(DeviceProxy * p_devProxy);
void APP_DeviceDisconnected(DeviceProxy * p_devProxy);
uint8_t APP_GetFileTransIndex(DeviceProxy * p_devProxy);
#ifdef ENABLE_AUTO_RUN
void APP_SetExecIterations(uint16_t runs);
void APP_GoNextRun(uint8_t countPass);
uint16_t APP_GetPassedRun(void);
#endif
void APP_IoCapSetting(uint8_t iocap);
void APP_ScSetting(uint8_t sc);



//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APPLICATION_H */

/*******************************************************************************
 End of File
 */

