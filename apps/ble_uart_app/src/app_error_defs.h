/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Error Code Definition Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_error_desf.h

  Summary:
    This file contains the definitions of error codes for application.

  Description:
    This file contains the definitions of error codes for application.
 *******************************************************************************/

#ifndef APP_ERROR_DEFS_H
#define APP_ERROR_DEFS_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define APP_RES_SUCCESS                         0x0000              /**< Execution successfully. */
#define APP_RES_FAIL                            0x0001              /**< Execution fail. */
#define APP_RES_OOM                             0x0002              /**< Out of memory. */
#define APP_RES_INVALID_PARA                    0x0003              /**< Invalid parameters. */
#define APP_RES_NO_RESOURCE                     0x0004              /**< No resource. */
#define APP_RES_BAD_STATE                       0x0005              /**< Bad State. */
#define APP_RES_PENDING_DUE_TO_SECURITY         0x0006              /**< Pending the request due to security process. */
#define APP_RES_BUSY                            0x0007              /**< Execution fail due to system is busy. */
#define APP_RES_COMPLETE                        0x1000              /**< Some procedure is complete to distinguish execution successfully */

#endif
