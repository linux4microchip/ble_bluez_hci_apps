/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Advertising Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_adv.h

  Summary:
    This file contains the Application Advertising functions for this project.

  Description:
    This file contains the Application Advertising functions for this project.
 *******************************************************************************/

#ifndef APP_ADV_H
#define APP_ADV_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>



// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
/**@defgroup APP_ADV_DEFAULT_INTERVAL APP_ADV_DEFAULT_INTERVAL
 * @brief The default value for the BLE Advertising interval. Unit: 0.625 ms. Default: 0x0200 (320 milliseconds)
 * @{ */
#define APP_ADV_DEFAULT_INTERVAL                                        0x0200
/** @} */

/**@defgroup APP_ADV_TYPE APP_ADV_TYPE
* @brief The definition of the advertising type
* @{ */
#define APP_ADV_TYPE_FLAGS                                              0x01       /**< Flags. */
#define APP_ADV_TYPE_INCOMPLETE_16BIT_SRV_UUID                          0x02       /**< Incomplete List of 16-bit Service Class UUIDs. */
#define APP_ADV_TYPE_COMPLETE_16BIT_SRV_UUID                            0x03       /**< Complete List of 16-bit Service Class UUIDs. */
#define APP_ADV_TYPE_SHORTENED_LOCAL_NAME                               0x08       /**< Shortened Local Name. */
#define APP_ADV_TYPE_COMPLETE_LOCAL_NAME                                0x09       /**< Complete Local Name. */
#define APP_ADV_TYPE_TX_POWER                                           0x0A       /**< Tx Power Level. */
#define APP_ADV_TYPE_SRV_DATA_16BIT_UUID                                0x16       /**< Service Data - 16-bit UUID. */
#define APP_ADV_TYPE_MANU_DATA                                          0xFF       /**< Manufacture Specific Data. */
/** @} */

/** @brief Advertising data size. */
#define APP_ADV_TYPE_LEN                                                0x01       /**< Length of advertising data type. */
#define APP_ADV_SRV_DATA_LEN                                            0x04       /**< Length of service data. */
#define APP_ADV_SRV_UUID_LEN                                            0x02       /**< Length of service UUID. */

/**@defgroup APP_ADV_FLAG APP_ADV_FLAG
* @brief The definition of the mask setting for the advertising type
* @{ */
#define APP_ADV_FLAG_LE_LIMITED_DISCOV                                              (1 << 0)       /**< LE Limited Discoverable Mpde. */
#define APP_ADV_FLAG_LE_GEN_DISCOV                                                  (1 << 1)       /**< LE General Discoverable Mpde. */
#define APP_ADV_FLAG_BREDR_NOT_SUPPORTED                                            (1 << 2)       /**< BR/EDR Not Supported. */
#define APP_ADV_FLAG_SIMULTANEOUS_LE_BREDR_TO_SAME_DEVICE_CAP_CONTROLLER            (1 << 3)       /**< Simultaneous LE and BR/EDR to Same Device Capable (Controller). */
#define APP_ADV_FLAG_SIMULTANEOUS_LE_BREDR_TO_SAME_DEVICE_CAP_HOST                  (1 << 4)       /**< Simultaneous LE and BR/EDR to Same Device Capable (Host). */
/** @} */

#define APP_ADV_COMPANY_ID_MCHP                                         0x00CD
#define APP_ADV_SERVICE_UUID_MCHP                                       0xFEDA
#define APP_ADV_ADD_DATA_CLASS_BYTE                                     0xFF

/**@defgroup APP_ADV_PROD_TYPE APP_ADV_PROD_TYPE
* @brief The definition of the product type in the advertising data
* @{ */
#define APP_ADV_PROD_TYPE_BLE_UART                                                  0x01           /**< Product Type: BLE UART */
/** @} */

/**@defgroup APP_ADV_FEATURE_SET APP_ADV_FEATURE_SET
* @brief The definition of the mask setting for the feature set in the advertising data
* @{ */
#define APP_ADV_FEATURE_SET_FEATURE1                                                (1 << 0)       /**< Feature 1. */
#define APP_ADV_FEATURE_SET_FEATURE2                                                (1 << 1)       /**< Feature 2. */
/** @} */


/**@brief The definition of the advertising set number. (Only for extended advertising)
* @{ */
#if defined(APP_ADV_TYPE_LEGACY) && defined(APP_ADV_TYPE_EXT)
#define APP_ADV_SET_NUM                                                  0x02                     /**< Number of advertising set. */
#else
#define APP_ADV_SET_NUM                                                  0x01                     /**< Number of advertising set. */
#endif
/** @} */

/**@brief The definition of the advertising handle. (Only for extended advertising)
* @{ */
#define APP_ADV_HANDLE1                                                  0x01                     /**< Advertising handle of advertising set 1. */
#define APP_ADV_HANDLE2                                                  0x02                     /**< Advertising handle of advertising set 2. */
/** @} */

/**@brief The definition of the value to be transmitted in the advertising SID subfield of the ADI field of the Extended Header. (Only for extended advertising)
* @{ */
#define APP_ADV_SID1                                                     0x00                     /**< SID of advertising set 1. */
#define APP_ADV_SID2                                                     0x01                     /**< SID of advertising set 2. */
/** @} */


#define APP_ADV_TX_POWER_LEVEL                                           11       /**< Advertising TX power level: 11dB */


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

/**@brief The structure contains the BLE Advertising parameters to be set by the application. */
typedef struct APP_BLE_AdvParams_T
{
    uint16_t             intervalMin;                             /**< Minimum advertising interval, see @ref BLE_GAP_ADV_INTERVAL. Unit: 0.625ms */
    uint16_t             intervalMax;                             /**< Maximum advertising interval, see @ref BLE_GAP_ADV_INTERVAL. Unit: 0.625ms */
    uint8_t              advType;                                 /**< advertising type, see @ref BLE_GAP_ADV_TYPE. */
    uint8_t              filterPolicy;                            /**< Advertising filter policy. See @ref BLE_GAP_ADV_FILTER_POLICY */
} APP_BLE_AdvParams_T;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
void APP_ADV_Init(void);
void APP_ADV_Start(void);
void APP_ADV_Stop(void);

#endif
