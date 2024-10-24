/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble_handler.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

#ifndef _APP_BLE_HANDLER_H
#define _APP_BLE_HANDLER_H

// **********************n*******************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <string.h>
#include "app_gap.h"
#include "gdbus/gdbus.h"
#include "bluetooth/bluetooth.h"


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

//#define APP_TRP_MAX_LINK_NUMBER        ((BLE_TRSPC_MAX_CONN_NBR > BLE_TRSPS_MAX_CONN_NBR) ? BLE_TRSPC_MAX_CONN_NBR : BLE_TRSPS_MAX_CONN_NBR)
#define APP_TRP_MAX_LINK_NUMBER         BLE_GAP_MAX_LINK_NBR


/**@brief Maximum device in peripheral role. */
#define APP_TRPS_MAX_LINK_NUMBER        APP_TRP_MAX_LINK_NUMBER

/**@brief Maximum device in central role. */
#define APP_TRPC_MAX_LINK_NUMBER        APP_TRP_MAX_LINK_NUMBER




/**@defgroup APP_BLE_MAX_LINK_NUMBER APP_BLE_MAX_LINK_NUMBER
 * @brief The definition of maximum BLE links that can exist
 * @{ */
#define APP_BLE_MAX_LINK_NUMBER         BLE_GAP_MAX_LINK_NBR
/** @} */

//#define APP_INVALID_CONN_HANDLE         0xFFFF


/**@brief Enumeration type of BLE state. */
typedef enum APP_BLE_LinkState_T
{
    APP_BLE_STATE_STANDBY,                                                /**< Standby state. i.e. not advertising */
    APP_BLE_STATE_ADVERTISING,                                            /**< BLE is advertising */
    APP_BLE_STATE_SCANNING,                                               /**< BLE is scanning (only for BLE_GAP_ROLE_CENTRAL) */
    APP_BLE_STATE_CONNECTING,                                             /**< BLE is connecting */
    APP_BLE_STATE_CONNECTED,                                              /**< BLE is connected */
    APP_BLE_STATE_TOTAL
} APP_BLE_LinkState_T;

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

/**@brief The structure contains the BLE Connection parameters. */
typedef struct APP_BLE_ConnData_T
{
    uint8_t                role;                                           /**< GAP role, see @ref BLE_GAP_ROLE. */
    //uint16_t               handle;                                         /**< Connection handle associated with this connection. */
    BLE_GAP_Addr_T         remoteAddr;                                     /**< See @ref BLE_GAP_Addr_T. */
    //uint16_t               connInterval;                                   /**< Connection interval used on this connection. Range should be @ref BLE_GAP_CP_RANGE. */
    //uint16_t               connLatency;                                    /**< Slave latency for the connection in terms of number of connection events, see @ref BLE_GAP_CP_RANGE. */
    //uint16_t               supervisionTimeout;                             /**< Supervision timeout for the LE Link, see @ref BLE_GAP_CP_RANGE. */
    //uint8_t                txPhy;                                          /**< TX PHY. See @ref BLE_GAP_PHY_TYPE. */
    //uint8_t                rxPhy;                                          /**< RX PHY. See @ref BLE_GAP_PHY_TYPE. */
} APP_BLE_ConnData_T;


/**@brief The structure contains the BLE link related information maintained by the application Layer */
typedef struct APP_BLE_ConnList_T
{
    APP_BLE_LinkState_T         linkState;                                              /**< BLE link state. see @ref APP_BLE_LinkState_T */
    APP_BLE_ConnData_T          connData;                                               /**< BLE connection information. See @ref APP_BLE_ConnData_T */
    DeviceProxy                 *p_deviceProxy;
} APP_BLE_ConnList_T;


typedef enum APP_PtyChanged_T
{
    APP_CHANGED_TYPE_CONN_STAT,
    APP_CHANGED_TYPE_SVC_RESOLVED,
    APP_CHANGED_TYPE_MTU_UPDATED,
} APP_PtyChanged_T;


typedef struct APP_ConnectStateChanged_T
{
  DeviceProxy *p_proxy;
  char * p_address;
  char * p_addressType;
  bool status;
  bool connState;
  uint8_t role;
} APP_ConnectStateChanged_T;


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
extern uint8_t              g_bleConnLinkNum;

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************




APP_BLE_ConnList_T *APP_GetFreeConnList(void);
void APP_InitConnList(void);
void APP_UpdateLocalName(uint8_t devNameLen, uint8_t *p_devName);
uint8_t APP_GetConnLinkNum(void);
APP_BLE_LinkState_T APP_GetBleStateByLink(APP_BLE_ConnList_T *p_bleConn);
void APP_SetBleStateByLink(APP_BLE_ConnList_T *p_bleConn, APP_BLE_LinkState_T state);
APP_BLE_ConnList_T *APP_GetLastOneConnectedBleLink(void);
APP_BLE_ConnList_T *APP_GetBleLinkByStates(APP_BLE_LinkState_T start, APP_BLE_LinkState_T end);
void APP_PropertyChangedHandler(APP_PtyChanged_T ptyChangedType, void *p_param);
int8_t APP_Str2BtAddr(const char *p_str, bdaddr_t *p_ba);
int8_t APP_Str2BtAddrBytes(const char *p_str, uint8_t *p_ba);
uint8_t APP_DetermineRandomAddrType(uint8_t * p_addr);
uint8_t APP_GetRoleNumber(uint8_t role) ;




#endif /* _APP_BLE_HANDLER_H */

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

/*******************************************************************************
 End of File
 */

