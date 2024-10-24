/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  TRSP Definition Header File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trsp_desf.h

  Summary:
    This file contains the TRSP definitions.

  Description:
    This file contains the TRSP definitions.
 *******************************************************************************/


#ifndef BLE_TRSP_DEFS_H
#define BLE_TRSP_DEFS_H


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

/** @addtogroup BLE_TRSP_DEFS
 *  @{ */
 
// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
/**@addtogroup BLE_TRSP_DEFS Defines
 * @{ */
 
/**@defgroup BLE_TRSP_ERR_CODE Error code definitions
 * @brief The definition of TRSP API result.
 * @{ */
#define TRSP_RES_SUCCESS                         (0x0000U)                                  /**< Execution successfully. */
#define TRSP_RES_FAIL                            (0x0001U)                                  /**< Execution fail. */
#define TRSP_RES_OOM                             (0x0002U)                                  /**< Out of memory. */
#define TRSP_RES_INVALID_PARA                    (0x0003U)                                  /**< Invalid parameters. */
#define TRSP_RES_NO_RESOURCE                     (0x0004U)                                  /**< No resource. */
#define TRSP_RES_BAD_STATE                       (0x0005U)                                  /**< Bad State. */
#define TRSP_RES_PENDING_DUE_TO_SECURITY         (0x0006U)                                  /**< Pending the request due to security process. */
#define TRSP_RES_BUSY                            (0x0007U)                                  /**< Execution fail due to stack is busy. */
/** @} */

/**@defgroup BLE_TRSP_HEADER_SIZE GATT Header size
 * @brief The definition of GATT header size.
 * @{ */
#define TRSP_ATT_HEADER_SIZE                          (3U)                                  /**< The ATT Header Size. */
/** @} */


/**@} */ //BLE_TRSP_DEFS_H

/** @} */

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif
