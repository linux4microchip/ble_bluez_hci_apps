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
void APP_CMD_Scan(int argc, char *argv[]);
void APP_CMD_ListDevices(int argc, char *argv[]);
void APP_CMD_Connect(int argc, char *argv[]);
void APP_CMD_CancelConnect(int argc, char *argv[]);
void APP_CMD_Disconnect(int argc, char *argv[]);
void APP_CMD_RemoveDevice(int argc, char *argv[]);
void APP_CMD_SendRawData(int argc, char *argv[]);
void APP_CMD_SendFileData(int argc, char *argv[]);
void APP_CMD_ReceiveFileData(int argc, char *argv[]);
void APP_CMD_AppVer(int argc, char *argv[]);
void APP_CMD_MgmtAdv(int argc, char *argv[]);
void APP_CMD_MgmtSetPhy(int argc, char *argv[]);
void APP_CMD_MgmtGetPhy(int argc, char *argv[]);
void APP_CMD_PairDevice(int argc, char *argv[]);
void APP_CMD_UnpairDevice(int argc, char *argv[]);
void APP_CMD_ModeSwitch(int argc, char *argv[]);
void APP_CMD_PatternSelect(int argc, char *argv[]);
void APP_CMD_BurstModeStart(int argc, char *argv[]);
void APP_CMD_BurstModeStartAll(int argc, char *argv[]);
#ifdef ENABLE_AUTO_RUN
void APP_CMD_SetExecRuns(int argc, char *argv[]);
#endif
void APP_CMD_SetIoCap(int argc, char *argv[]);
void APP_CMD_SetSecureConnection(int argc, char *argv[]);
void APP_CMD_ReadPairInfo(int argc, char *argv[]);


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_CMD_H */

/*******************************************************************************
 End of File
 */

