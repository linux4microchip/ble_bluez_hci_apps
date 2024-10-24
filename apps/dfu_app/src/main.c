/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <glib.h>

#include "gdbus/gdbus.h"
#include "shared/util.h"

#include "application.h"
#include "app_log.h"
#include "app_cmd.h"


// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
#define PROMPT_ON   COLOR_BLUE "[DFU-App]" COLOR_OFF "# "


int main(int argc, char *argv[])
{
    APP_LOG_INIT("DFU");
    
    bt_shell_init(argc, argv, NULL);
    bt_shell_set_menu(APP_CMD_GetCmdMenu());
    
    APP_Initialize();

    bt_shell_set_prompt(PROMPT_ON);
    bt_shell_attach(fileno(stdin));
    /*running in a loop*/
    bt_shell_run();
    
    APP_Deinitialize();
    
    return 0;
}

/*******************************************************************************
 End of File
*/


