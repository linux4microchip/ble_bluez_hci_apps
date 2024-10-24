/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  APP GAP Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_gap.h

  Summary:
    This file contains the Application GAP definitions.

  Description:
 *******************************************************************************/

#ifndef APP_GAP_H
#define APP_GAP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

/**@defgroup GAP_MAX_ADDRESS_LEN Address length
 * @brief The definition of maximum Bluetooth address length.
 * @{ */
#define GAP_MAX_BD_ADDRESS_LEN                               0x06        /**< Maximum length of BD address . */
/** @} */

/**@defgroup GAP_MAX_DEVICE_NAME_LEN Maximum device name length
 * @brief The definition of maximum length of device name
 * @{ */
#define GAP_MAX_DEVICE_NAME_LEN                              0x20        /**< Maximum length of device name. */
/** @} */

/**@defgroup BLE_GAP_MAX_LINK_NBR Maximum connection number
 * @brief The definition of maximum allowed link number of GAP connections
 * @{ */
#define BLE_GAP_MAX_LINK_NBR                                    0x06        /**< Maximum allowed BLE GAP connections */
/** @} */

/**@defgroup BLE_GAP_ADV_DATA_LEN Maximum advertising data length
 * @brief The definition of maximum advertising data length
 * @{ */
#define BLE_GAP_ADV_MAX_LENGTH                                  0x1F        /**< Maximum length of advertising data in bytes. */
/** @} */

/**@defgroup BLE_GAP_ADDR_TYPE Address type
 * @brief The definition of address types
 * @{ */
#define BLE_GAP_ADDR_TYPE_PUBLIC                                0x00        /**< Public device address (default). */
#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC                         0x01        /**< Static random device address. */
#define BLE_GAP_ADDR_TYPE_RANDOM_RESOLVABLE                     0x02        /**< Private resolvable random device address. */
#define BLE_GAP_ADDR_TYPE_RANDOM_NON_RESOLVABLE                 0x03        /**< Private non resolvable random device address. */
/** @} */

/**@defgroup BLE_GAP_ROLE Connection roles
 * @brief The definition of GAP role in connection state.
 * @{ */
#define BLE_GAP_ROLE_CENTRAL                                    0x00        /**< MBA plays Center role in connection state . */
#define BLE_GAP_ROLE_PERIPHERAL                                 0x01        /**< MBA plays Peripheral role in connection state . */
/** @} */


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

/**@brief Bluetooth address. */
typedef struct BLE_GAP_Addr_T
{
    uint8_t                 addrType;                               /**< See @ref BLE_GAP_ADDR_TYPE. */
    uint8_t                 addr[GAP_MAX_BD_ADDRESS_LEN];           /**< Bluetooth address, LSB format. */
} BLE_GAP_Addr_T;



/**@} */
#endif

/**
  @}
*/
