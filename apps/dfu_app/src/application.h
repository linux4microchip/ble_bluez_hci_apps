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

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

#define APP_VERSION VERSION

#define BAUDRATE_9600 9600
#define BAUDRATE_19200 19200
#define BAUDRATE_38400 38400
#define BAUDRATE_57600 57600
#define BAUDRATE_115200 115200
#define BAUDRATE_230400 230400
#define BAUDRATE_460800 460800
#define BAUDRATE_921600 921600

#define UART_CONFIG_DATA_BIT (8)
#define UART_CONFIG_STOP_BIT (1)
#define UART_CONFIG_PARITY   'N'
#define UART_CONFIG_FLCTRL   (0)


// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************


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


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APPLICATION_H */

/*******************************************************************************
 End of File
 */

