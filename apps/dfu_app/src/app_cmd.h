/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application command handler Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.h

  Summary:
    This header file provides prototypes and definitions for the application command handler.

  Description:
    This header file provides prototypes and definitions for the application command handler.

*******************************************************************************/

#ifndef _APP_CMD_H
#define _APP_CMD_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "glib.h"
#include "shared/shell.h"


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


// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef struct bt_shell_menu APP_CMD_Menu_T;

APP_CMD_Menu_T * APP_CMD_GetCmdMenu(void);
void APP_CMD_DfuProcess(int argc, char *argv[]);
void APP_CMD_AppVer(int argc, char *argv[]);


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_CMD_H */

/*******************************************************************************
 End of File
 */

