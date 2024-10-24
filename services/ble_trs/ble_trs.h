/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BLE Transparent Service Header File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trs.h

  Summary:
    This file contains the BLE Transparent Service functions for application user.

  Description:
    This file contains the BLE Transparent Service functions for application user.
 *******************************************************************************/


/**
 * @addtogroup BLE_TRS BLE TRS
 * @{
 * @brief Header file for the BLE Transparent Service.
 * @note Definitions and prototypes for the BLE Transparent Service stack layer application programming interface.
 */
#ifndef BLE_TRS_H
#define BLE_TRS_H

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
/**@defgroup UUID UUID
 * @brief The definition of UUID
 * @{ */
#define UUID_MCHP_PROPRIETARY_SERVICE_16             "49535343-fe7d-4ae5-8fa9-9fafd205e455"
#define UUID_MCHP_TRANS_TX_16                        "49535343-1e4d-4bd9-ba61-23c647249616"
#define UUID_MCHP_TRANS_RX_16                        "49535343-8841-43f4-a8d4-ecbe34729bb3"
#define UUID_MCHP_TRANS_CTRL_16                      "49535343-4c8a-39b3-2f49-511cff073b7e"
/** @} */


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

/**
 *@brief Initialize BLE Transparent Service callback function.
 *
 * @param[in] p_dbusConn                     The connection to D-Bus.
 * @param[in] p_proxyGattMgr                 Proxy associated with the org.bluez.GattManager interface on BlueZ
 *
 * @return true                              Successfully register BLE transparent service.
 * @return false                             Fail to register service.
 *
 */
bool BLE_TRS_Add(DBusConnection *p_dbusConn, GDBusProxy * p_proxyGattMgr);

/**
 *@brief Update Ctrl characteristic value to BlueZ.
 *
 * @param[in] p_data                         Pointer to the updated data.
 * @param[in] len                            Data length.
 *
 */
void BLE_TRS_UpdateValueCtrl(uint8_t *p_value, uint16_t len);

/**
 *@brief Update Tx characteristic value to BlueZ.
 *
 * @param[in] p_data                         Pointer to the updated data.
 * @param[in] len                            Data length.
 *
 */
void BLE_TRS_UpdateValueTx(uint8_t *p_value, uint16_t len);

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif

/**
  @}
 */
