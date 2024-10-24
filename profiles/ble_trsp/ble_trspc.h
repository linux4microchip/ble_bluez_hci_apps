/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BLE Transparent Client Profile Source File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trspc.h

  Summary:
    This file contains the BLE Transparent Client functions for application user.

  Description:
    This file contains the BLE Transparent Client functions for application user.
 *******************************************************************************/
/** @addtogroup BLE_PROFILE BLE Profile
 *  @{ */

/** @addtogroup BLE_TRP Transparent Profile
 *  @{ */

/**
 * @defgroup BLE_TRPC Transparent Profile Client Role (TRPC)
 * @brief Transparent Profile Client Role (TRPC)
 * @{
 * @brief Header file for the BLE Transparent Profile Client role library.
 * @note Definitions and prototypes for the BLE Transparent profile stack layer application programming interface.
 */
#ifndef BLE_TRSPC_H
#define BLE_TRSPC_H

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
/**@addtogroup BLE_TRPC_DEFINES Defines
 * @{ */

/**@defgroup BLE_TRSPC_MAX_CONN_NBR Maximum connection number
 * @brief The definition of Memory size.
 * @{ */
#define BLE_TRSPC_MAX_CONN_NBR                  (0x06U)    /**< Maximum allowing Conncetion Numbers for the device. */
/** @} */


/**@defgroup BLE_TRSPC_UL_STATUS Definition of uplink status
 * @brief The definition of BLE transparent service uplink status.
 * @{ */
#define BLE_TRSPC_UL_STATUS_DISABLED            (0x00U)    /**< Transparent uplink is disabled. */
#define BLE_TRSPC_UL_STATUS_CBFCENABLED         (0x01U)    /**< Transparent uplink is enabled with credit based flow control. */
/** @} */

/**@defgroup BLE_TRSPC_DL_STATUS Definition of downlink status
 * @brief The definition of BLE transparent service downlink status.
 * @{ */
#define BLE_TRSPC_DL_STATUS_DISABLED            (0x00U)    /**< Transparent downlink is disabled. */
#define BLE_TRSPC_DL_STATUS_NONCBFCENABLED      (0x10U)    /**< Transparent downlink is enabled without credit based flow control. */
#define BLE_TRSPC_DL_STATUS_CBFCENABLED         (0x20U)    /**< Transparent downlink is enabled with credit based flow control. */
/** @} */

/**@defgroup BLE_TRSPC_SEND_RESULT Definition of send vendor command and send data result
 * @brief The definition of send vendor command and send data result.
 * @{ */
#define BLE_TRSPC_SEND_RESULT_SUCCESS           (0x00U)    /**< Send successfully. */
#define BLE_TRSPC_SEND_RESULT_BUSY              (0x01U)    /**< BlueZ is busy, application must retry later. */
#define BLE_TRSPC_SEND_RESULT_FAILED            (0x02U)    /**< Send failed. */
/** @} */


/**@defgroup BLE_TRSPC_RETRY BLE_TRSPC_RETRY
 * @brief If the profile message transmission conflicts with other processes, profile will resend message.
 * @      If retry still failed, application will receive BLE_TRSPC_EVT_ERR_UNSPECIFIED event.
 * @      Application can adjust this configuration according to its environment.
 * @{ */
#define BLE_TRSPC_RETRY_TIMEOUT                           (45U)        /**< Retry timeout in ms. */
#define BLE_TRSPC_RETRY_MAX_NUMBER                        (5U)         /**< Max retry number. */
/** @} */


/**@} */ //BLE_TRPC_DEFINES


/**@addtogroup BLE_TRPC_ENUMS Enumerations
 * @{ */

/**@brief Enumeration type of BLE transparent profile callback events. */
typedef enum BLE_TRSPC_EventId_T
{
    BLE_TRSPC_EVT_NULL = 0x00U,
    BLE_TRSPC_EVT_UL_STATUS,                            /**< Transparent Profile Uplink status update event. See @ref BLE_TRSPC_EvtUplinkStatus_T for event details. */
    BLE_TRSPC_EVT_DL_STATUS,                            /**< Transparent Profile Downlink status update event. See @ref BLE_TRSPC_EvtDownlinkStatus_T for event details. */
    BLE_TRSPC_EVT_RECEIVE_DATA,                         /**< Transparent Profile Data Channel received notification event. See @ref BLE_TRSPC_EvtReceiveData_T for event details. */
    BLE_TRSPC_EVT_VENDOR_CMD,                           /**< Transparent Profile vendor command received notification event. See @ref BLE_TRSPC_EvtVendorCmd_T for event details. */
    BLE_TRSPC_EVT_VENDOR_CMD_RSP,                       /**< Transparent Profile Vendor command response received notification event. See @ref BLE_TRSPC_EvtVendorCmdRsp_T for event details. */
    BLE_TRSPC_EVT_DATA_RSP,                             /**< Transparent Profile Data response received notification event. See @ref BLE_TRSPC_EvtDataRsp_T for event details. */
    BLE_TRSPC_EVT_DISC_COMPLETE,                        /**< Transparent Profile discovery complete event. See @ref BLE_TRSPC_EvtDiscComplete_T for event details. */
    BLE_TRSPC_EVT_ERR_NO_MEM,                           /**< Profile internal error occurs due to insufficient heap memory. */
    BLE_TRSPC_EVT_ERR_UNSPECIFIED,                      /**< Profile internal unspecified error occurs. */
    BLE_TRSPC_EVT_END
}BLE_TRSPC_EventId_T;

/**@} */ //BLE_TRPC_ENUMS


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
/**@addtogroup BLE_TRPC_STRUCTS Structures
 * @{ */

/**@brief Data structure for @ref BLE_TRSPC_EVT_UL_STATUS event. */
typedef struct BLE_TRSPC_EvtUplinkStatus_T
{
    GDBusProxy       *p_dev;                            /**< Proxy associated with this remote device interface. */
    uint8_t          status;                            /**< Connection status. See @ref BLE_TRSPC_UL_STATUS.*/
}   BLE_TRSPC_EvtUplinkStatus_T;

/**@brief Data structure for @ref BLE_TRSPC_EVT_DL_STATUS event. */
typedef struct BLE_TRSPC_EvtDownlinkStatus_T
{
    GDBusProxy      *p_dev;                             /**< Proxy associated with this remote device interface. */
    uint8_t         status;                             /**< Connection status. See @ref BLE_TRSPC_DL_STATUS.*/
    uint8_t         currentCreditNumber;                /**< The current available credit number of the downlink in this connection. */
}   BLE_TRSPC_EvtDownlinkStatus_T;

/**@brief Data structure for @ref BLE_TRSPC_EVT_RECEIVE_DATA event. */
typedef struct BLE_TRSPC_EvtReceiveData_T
{
    GDBusProxy       *p_dev;                            /**< Proxy associated with this remote device interface. */
}   BLE_TRSPC_EvtReceiveData_T;

/**@brief Data structure for @ref BLE_TRSPC_EVT_VENDOR_CMD event. */
typedef struct BLE_TRSPC_EvtVendorCmd_T
{
    GDBusProxy      *p_dev;                             /**< Proxy associated with this remote device interface. */
    uint8_t         payloadLength;                      /**< Length of payload. */
    uint8_t         *p_payLoad;                         /**< Vendor command payload pointer. */
}   BLE_TRSPC_EvtVendorCmd_T;

/**@brief Data structure for @ref BLE_TRSPC_EVT_VENDOR_CMD_RSP event. */
typedef struct BLE_TRSPC_EvtVendorCmdRsp_T
{
    GDBusProxy       *p_dev;                            /**< Proxy associated with this remote device interface. */
    uint8_t          result;                            /**< The result of @ref BLE_TRSPC_SendVendorCommand. See @ref BLE_TRSPC_SEND_RESULT. */
}   BLE_TRSPC_EvtVendorCmdRsp_T;

/**@brief Data structure for @ref BLE_TRSPC_EVT_DATA_RSP event. */
typedef struct BLE_TRSPC_EvtDataRsp_T
{
    GDBusProxy       *p_dev;                            /**< Proxy associated with this remote device interface. */
    uint8_t          result;                            /**< The result of @ref BLE_TRSPC_SendData. See @ref BLE_TRSPC_SEND_RESULT. */
}   BLE_TRSPC_EvtDataRsp_T;


/**@brief Data structure for @ref BLE_TRSPC_EVT_DISC_COMPLETE event. */
typedef struct BLE_TRSPC_EvtDiscComplete_T
{
    GDBusProxy       *p_dev;                            /**< Proxy associated with this remote device interface. */
}   BLE_TRSPC_EvtDiscComplete_T;

/**@brief The union of BLE Transparent profile client event types. */
typedef union
{
    BLE_TRSPC_EvtUplinkStatus_T     onUplinkStatus;     /**< Handle @ref BLE_TRSPC_EVT_UL_STATUS. */
    BLE_TRSPC_EvtDownlinkStatus_T   onDownlinkStatus;   /**< Handle @ref BLE_TRSPC_EVT_DL_STATUS. */
    BLE_TRSPC_EvtReceiveData_T      onReceiveData;      /**< Handle @ref BLE_TRSPC_EVT_RECEIVE_DATA. */
    BLE_TRSPC_EvtVendorCmd_T        onVendorCmd;        /**< Handle @ref BLE_TRSPC_EVT_VENDOR_CMD. */
    BLE_TRSPC_EvtVendorCmdRsp_T     onVendorCmdRsp;     /**< Handle @ref BLE_TRSPC_EVT_VENDOR_CMD_RSP. */
    BLE_TRSPC_EvtDataRsp_T          onDataRsp;          /**< Handle @ref BLE_TRSPC_EVT_DATA_RSP. */
    BLE_TRSPC_EvtDiscComplete_T     onDiscComplete;     /**< Handle @ref BLE_TRSPC_EVT_DISC_COMPLETE. */
} BLE_TRSPC_EventField_T;


/**@brief BLE Transparent profile client callback event. */
typedef struct  BLE_TRSPC_Event_T
{
    BLE_TRSPC_EventId_T         eventId;                /**< Event ID.*/
    BLE_TRSPC_EventField_T      eventField;             /**< Event field. */
} BLE_TRSPC_Event_T;

/**@brief BLE Transparent profile cliet callback type. This callback function sends BLE Transparent profile client events to the application. */
typedef void(*BLE_TRSPC_EventCb_T)(BLE_TRSPC_Event_T *p_event);

/**@} */ //BLE_TRPC_STRUCTS


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
/**@addtogroup BLE_TRPC_FUNS Functions
 * @{ */

/**@brief Initialize TRS client profile.
 *
 */
void BLE_TRSPC_Init(void);

/**@brief Register BLE Transparent profile client callback. 
 *
 * @param[in] bleTranCliHandler             Client callback function.
 *
 */
void BLE_TRSPC_EventRegister(BLE_TRSPC_EventCb_T bleTranCliHandler);

/**@brief Send vendor command. 
 * Application must wait @ref BLE_TRSPC_EVT_VENDOR_CMD_RSP event to get send result.
 *
 * @param[in] p_proxyDev                    Proxy associated with the remote device interface
 * @param[in] commandID                     Command id of the vendor command.
 * @param[in] commandLength                 Length of payload in vendor commnad.
 * @param[in] p_commandPayload              Pointer to the payload of vendor command.
 *
 * @retval TRSP_RES_SUCCESS                  Successfully issue a send vendor command.
 * @retval TRSP_RES_FAIL                     Invalid connection or the CCCD of TCP is not enabled.
 * @retval TRSP_RES_OOM                      No available memory.
 * @retval TRSP_RES_INVALID_PARA             Error commandID usage or commandLength invalid.
 *
 */
uint16_t BLE_TRSPC_SendVendorCommand(GDBusProxy *p_proxyDev, uint8_t commandID, uint8_t commandLength, uint8_t *p_commandPayload);

/**@brief Send transparent data.
 * Application must wait @ref BLE_TRSPC_EVT_DATA_RSP event to get send result.
 *
 * @param[in] p_proxyDev                    Proxy associated with the remote device interface
 * @param[in] len                           Data length.
 * @param[in] p_data                        Pointer to the transparent data.
 *
 * @retval TRSP_RES_SUCCESS                  Successfully issue a send data.
 * @retval TRSP_RES_OOM                      No available memory.
 * @retval TRSP_RES_INVALID_PARA             Parameter does not meet the spec.
 *
 */
uint16_t BLE_TRSPC_SendData(GDBusProxy *p_proxyDev, uint16_t len, uint8_t *p_data);

/**@brief Get queued data length.
 *
 * @param[in] p_proxyDev                    Device proxy associated with the queued data
 * @param[out] p_dataLength                 Data length.
 *
 */
void BLE_TRSPC_GetDataLength(GDBusProxy *p_proxyDev, uint16_t *p_dataLength);

/**@brief Get queued data.
 *
 * @param[in] p_proxyDev                    Device proxy associated with the queued data
 * @param[out] p_data                       Pointer to the data buffer.
 *
 * @retval TRSP_RES_SUCCESS                  Successfully issue a flow ctrl stop.
 * @retval TRSP_RES_FAIL                     No data in the input queue.
 *
 */
uint16_t BLE_TRSPC_GetData(GDBusProxy *p_proxyDev, uint8_t *p_data);

/**@brief Notify profile one device is connected.
 *
 * @param[in] p_proxyDev                    Device proxy associated with connected device
 *
 */
void BLE_TRSPC_DevConnected(GDBusProxy *p_proxyDev);

/**@brief Notify profile one device is disconnected.
 *
 * @param[in] p_proxyDev                    Device proxy associated with disconnected device
 *
 */
void BLE_TRSPC_DevDisconnected(GDBusProxy *p_proxyDev);

/**@brief Handle an new proxy is added. Application must call this API to notify profile an new proxy to BlueZ is added.
 *
 * @param[in] p_proxy                       The new proxy.
 *
 */
void BLE_TRSPC_ProxyAddHandler(GDBusProxy *p_proxy);

/**@brief Handle a proxy is removed. Application must call this API to notify profile a proxy to BlueZ is removed.
 *
 * @param[in] p_proxy                       The removed proxy.
 *
 */
void BLE_TRSPC_ProxyRemoveHandler(GDBusProxy *p_proxy);

/**@brief Handle a property is changed. Application must call this API to notify profile a property on BlueZ is changed.
 *
 * @param[in] p_proxy                       The proxy associated with the property.
 * @param[in] p_name                        The propery name
 * @param[in] p_iter                        The message iterator
 *
 */
void BLE_TRSPC_PropertyHandler(GDBusProxy *p_proxy, const char *p_name, DBusMessageIter *p_iter);


/**@} */ //BLE_TRPC_FUNS


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

