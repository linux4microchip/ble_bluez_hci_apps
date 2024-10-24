/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BLE Transparent Server Profile Header File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trsps.h

  Summary:
    This file contains the BLE Transparent Server functions for application user.

  Description:
    This file contains the BLE Transparent Server functions for application user.
 *******************************************************************************/

/** @addtogroup BLE_PROFILE BLE Profile
 *  @{ */

/** @addtogroup BLE_TRP Transparent Profile
 *  @{ */

/**
 * @defgroup BLE_TRPS Transparent Profile Server Role (TRPS)
 * @brief Transparent Profile Server Role (TRPS)
 * @{
 * @brief Header file for the BLE Transparent Profile library.
 * @note Definitions and prototypes for the BLE Transparent profile stack layer application programming interface.
 */
#ifndef BLE_TRSPS_H
#define BLE_TRSPS_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include "gdbus/gdbus.h"


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
/**@addtogroup BLE_TRPS_DEFINES Defines
 * @{ */

/**@defgroup BLE_TRS_MAX_CONN_NBR Maximum connection number
 * @brief The definition of Memory size.
 * @{ */
#define BLE_TRSPS_MAX_CONN_NBR                  (0x06U)    /**< Maximum allowing Conncetion Numbers for MBADK. */
/** @} */

/**@defgroup BLE_TRSPS_STATUS TRSPS Status
 * @brief The definition of BLE transparent service status.
 * @{ */
#define BLE_TRSPS_STATUS_CTRL_DISABLED          (0x00U)    /**< Local ble transparent service control characteristic CCCD is closed. */
#define BLE_TRSPS_STATUS_CTRL_OPENED            (0x01U)    /**< Local ble transparent service control characteristic CCCD is enable. */
#define BLE_TRSPS_STATUS_TX_DISABLED            (0x00U)    /**< Local ble transparent service TX characteristic CCCD is closed. */
#define BLE_TRSPS_STATUS_TX_OPENED              (0x01U)    /**< Local ble transparent service TX characteristic CCCD is enable. */
/** @} */

/**@} */ //BLE_TRPS_DEFINES


/**@addtogroup BLE_TRPS_ENUMS Enumerations
 * @{ */

/**@brief Enumeration type of BLE transparent profile callback events. */
typedef enum BLE_TRSPS_EventId_T
{
    BLE_TRSPS_EVT_NULL = 0x00U,
    BLE_TRSPS_EVT_CTRL_STATUS,                          /**< Transparent Profile Control Channel status update event. See @ref BLE_TRSPS_EvtCtrlStatus_T for event details. */
    BLE_TRSPS_EVT_TX_STATUS,                            /**< Transparent Profile Data Channel transmit status event. See @ref BLE_TRSPS_EvtTxStatus_T for event details. */
    BLE_TRSPS_EVT_CBFC_ENABLED,                         /**< Transparent Profile Credit based flow control enable notification event. See @ref BLE_TRSPS_EvtCbfcEnabled_T for event details. */
    BLE_TRSPS_EVT_CBFC_CREDIT,                          /**< Transparent Profile Credit based flow control credit update event. See @ref BLE_TRSPS_EvtCbfcEnabled_T for event details. */
    BLE_TRSPS_EVT_RECEIVE_DATA,                         /**< Transparent Profile Data Channel received notification event. See @ref BLE_TRSPS_EvtReceiveData_T for event details. */
    BLE_TRSPS_EVT_VENDOR_CMD,                           /**< Transparent Profile vendor command received notification event. See @ref BLE_TRSPS_EvtVendorCmd_T for event details. */
    BLE_TRSPS_EVT_ERR_UNSPECIFIED,                      /**< Profile internal unspecified error occurs. */
    BLE_TRSPS_EVT_ERR_NO_MEM,                           /**< Profile internal error occurs due to insufficient heap memory. */
    BLE_TRSPS_EVT_END
}BLE_TRSPS_EventId_T;

/**@} */ //BLE_TRPS_ENUMS

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
/**@addtogroup BLE_TRPS_STRUCTS Structures
 * @{ */

/**@brief Data structure for @ref BLE_TRSPS_EVT_CTRL_STATUS event. */
typedef struct BLE_TRSPS_EvtCtrlStatus_T
{
    uint8_t          status;                                                /**< Connection status. See @ref BLE_TRSPS_STATUS.*/
}   BLE_TRSPS_EvtCtrlStatus_T;

/**@brief Data structure for @ref BLE_TRSPS_EVT_TX_STATUS event. */
typedef struct BLE_TRSPS_EvtTxStatus_T
{
    uint8_t          status;                                                /**< Connection status. See @ref BLE_TRSPS_STATUS.*/
}   BLE_TRSPS_EvtTxStatus_T;

/**@brief Data structure for @ref BLE_TRSPS_EVT_CBFC_ENABLED event. */
typedef struct BLE_TRSPS_EvtCbfcEnabled_T
{
    GDBusProxy       *p_dev;                                                /**< Proxy associated with this remote device interface. */
}BLE_TRSPS_EvtCbfcEnabled_T;

/**@brief Data structure for @ref BLE_TRSPS_EVT_RECEIVE_DATA event. */
typedef struct BLE_TRSPS_EvtReceiveData_T
{
    GDBusProxy       *p_dev;                                                /**< Proxy associated with this remote device interface. */
}BLE_TRSPS_EvtReceiveData_T;

/**@brief Data structure for @ref BLE_TRSPS_EVT_VENDOR_CMD event. */
typedef struct BLE_TRSPS_EvtVendorCmd_T
{
    GDBusProxy       *p_dev;                                                /**< Proxy associated with this remote device interface. */
    uint16_t         length;                                                /**< Vendor command payload length. */
    uint8_t          *p_payLoad;                                            /**< Vendor command payload pointer. */
}BLE_TRSPS_EvtVendorCmd_T;

/**@brief The union of BLE Transparent profile server event types. */
typedef union
{
    BLE_TRSPS_EvtCtrlStatus_T        onCtrlStatus;            /**< Handle @ref BLE_TRSPS_EVT_CTRL_STATUS. */
    BLE_TRSPS_EvtTxStatus_T          onTxStatus;              /**< Handle @ref BLE_TRSPS_EVT_TX_STATUS. */
    BLE_TRSPS_EvtCbfcEnabled_T       onCbfcEnabled;           /**< Handle @ref BLE_TRSPS_EVT_CBFC_ENABLED. */
    BLE_TRSPS_EvtReceiveData_T       onReceiveData;           /**< Handle @ref BLE_TRSPS_EVT_RECEIVE_DATA. */
    BLE_TRSPS_EvtVendorCmd_T         onVendorCmd;             /**< Handle @ref BLE_TRSPS_EVT_VENDOR_CMD. */
} BLE_TRSPS_EventField_T;

/**@brief BLE Transparent profile server callback event. */
typedef struct  BLE_TRSPS_Event_T
{
    BLE_TRSPS_EventId_T       eventId;                        /**< Event ID.*/
    BLE_TRSPS_EventField_T    eventField;                     /**< Event field. */
} BLE_TRSPS_Event_T;

/**@brief The structure contains information about change UUID function parameters. */
typedef struct  BLE_TRSPS_Uuids_T
{
    uint8_t *p_primaryService;                      /**< The user-defined 128-bit primary service UUID.*/
    uint8_t *p_transTx;                             /**< The user-defined 128-bit trans tx characteristic UUID. */
    uint8_t *p_transRx;                             /**< The user-defined 128-bit trans rx characteristic UUID. */
    uint8_t *p_transCtrl;                           /**< The user-defined 128-bit trans ctrl characteristic UUID. */
} BLE_TRSPS_Uuids_T;

/**@brief BLE Transparent profile server callback type. This callback function sends BLE Transparent profile server events to the application. */
typedef void(*BLE_TRSPS_EventCb_T)(BLE_TRSPS_Event_T *p_event);

/**@} */ //BLE_TRPS_STRUCTS

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
/**@addtogroup BLE_TRPS_FUNS Functions
 * @{ */

/**
 *@brief Register BLE Transparent profile server callback.
 *
 * @param[in] bleTranServHandler          	Client callback function.
 *
 */
void BLE_TRSPS_EventRegister(BLE_TRSPS_EventCb_T bleTranServHandler);

/**
 *@brief Initialize BLE Transparent Profile.
 *
 * @param[in] p_dbusConn                     The connection to D-Bus.
 * @param[in] p_proxyGattMgr                 Proxy associated with the org.bluez.GattManager interface on BlueZ
 *
 * @retval TRSP_RES_SUCCESS                  Success to add a service to the service table. 
 * @retval TRSP_RES_FAIL                     Fail to add a service to the service table.
 *
 */
uint16_t BLE_TRSPS_Init(DBusConnection *p_dbusConn, GDBusProxy * p_proxyGattMgr);


/**@brief Send vendor command.
 *
 * @param[in] p_proxyDev                    Proxy associated with the remote device interface
 * @param[in] commandID                     Command id of the vendor command
 * @param[in] commandLength                 Length of payload in vendor commnad
 * @param[in] p_commandPayload              Pointer to the payload of vendor command
 *
 * @retval TRSP_RES_SUCCESS                 Successfully issue a send vendor command.
 * @retval TRSP_RES_FAIL                    Invalid connection.
 * @retval TRSP_RES_OOM                     No available memory.
 * @retval TRSP_RES_INVALID_PARA            Error commandID usage or commandLength invalid or the CCCD of TCP is not enabled.
 *
 */
uint16_t BLE_TRSPS_SendVendorCommand(GDBusProxy *p_proxyDev, uint8_t commandID, uint8_t commandLength, uint8_t *p_commandPayload);


/**@brief Send transparent data.
 *
 * @param[in] p_proxyDev                    Proxy associated with this remote device interface.
 * @param[in] len                           Data length.
 * @param[in] p_data                        Pointer to the transparent data.
 *
 * @retval TRSP_RES_SUCCESS                 Successfully issue a send data.
 * @retval TRSP_RES_OOM                     No available memory.
 * @retval TRSP_RES_INVALID_PARA            Parameter does not meet the spec.
 *
 */
uint16_t BLE_TRSPS_SendData(GDBusProxy *p_proxyDev, uint16_t len, uint8_t *p_data);

/**@brief Get queued data length.
 *
 * @param[in] p_proxyDev                    Device proxy associated with the queued data
 * @param[out] p_dataLength                 Pointer to the data length
 *
 *
 */
void BLE_TRSPS_GetDataLength(GDBusProxy *p_proxyDev, uint16_t *p_dataLength);


/**@brief Get queued data.
 *
 * @param[in] p_proxyDev                    Device proxy associated with the queued data
 * @param[out] p_data                       Pointer to the data buffer
 *
 * @retval TRSP_RES_SUCCESS                 Successfully issue a flow ctrl stop.
 * @retval TRSP_RES_FAIL                    No data in the input queue or can not find the link.
 *
 */
uint16_t BLE_TRSPS_GetData(GDBusProxy *p_proxyDev, uint8_t *p_data);

/**@brief Notify profile one device is connected.
 *
 * @param[in] p_proxyDev                    Device proxy associated with connected device
 *
 */
void BLE_TRSPS_DevConnected(GDBusProxy *p_proxyDev);

/**@brief Notify profile one device is disconnected.
 *
 * @param[in] p_proxyDev                    Device proxy associated with disconnected device
 *
 */
void BLE_TRSPS_DevDisconnected(GDBusProxy *p_proxyDev);

/**@brief Handle BlueZ StartNotify message for Tx characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcStartNotifyTx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@brief Handle BlueZ StopNotify message for Tx characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcStopNotifyTx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@brief Handle BlueZ WriteValue message for Rx characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcWriteValueRx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@brief Handle BlueZ StartNotify message for Ctrl characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcStartNotifyCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@brief Handle BlueZ StopNotify message for Ctrl characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcStopNotifyCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@brief Handle BlueZ WriteValue message for Ctrl characteristic.
 *       This API should be called by D-Bus framework.
 *
 * @param[in] p_dbusConn                    The connection to D-Bus.
 * @param[in] p_msg                         The received message.
 * @param[in] p_userData                    The user data associated with this charateristic.
 *
 * @retval DBusMessage                      The message reply to BlueZ.
 *
 */
DBusMessage *BLE_TRSPS_ChrcWriteValueCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData);

/**@} */ //BLE_TRPS_FUNS


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif

/** @} */

/** @} */

/**
  @}
 */
