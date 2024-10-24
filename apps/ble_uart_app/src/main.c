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
#include "app_dbp.h"
#include "app_sm.h"
#include "app_scan.h"
#include "app_cmd.h"


static DBusConnection * sp_dbusConn;


// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************


int main(int argc, char *argv[])
{
    GDBusClient *p_dbusClient;

    APP_LOG_INIT("BLE_UART");
    
    bt_shell_init(argc, argv, NULL);
    bt_shell_set_menu(APP_CMD_GetCmdMenu());
    
    APP_Initialize();

    /*set up the dbus connection*/
    sp_dbusConn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
    g_dbus_attach_object_manager(sp_dbusConn);

    bt_shell_set_env("DBUS_CONNECTION", sp_dbusConn);
    /*create a bluez client for dbus connection object*/
    p_dbusClient = g_dbus_client_new(sp_dbusConn, "org.bluez", "/org/bluez");

    /* set connect/disconnect/signal handler function*/
    g_dbus_client_set_connect_watch(p_dbusClient, APP_DBP_DBusConnectHandler, NULL);
    g_dbus_client_set_disconnect_watch(p_dbusClient, APP_DBP_DBusDisconnectHandler, NULL);
    g_dbus_client_set_signal_watch(p_dbusClient, APP_DBP_DBusMessageHandler, NULL);
    /* set proxy handlers*/
    g_dbus_client_set_proxy_handlers(p_dbusClient, APP_DBP_ProxyAdded, APP_DBP_ProxyRemoved, APP_DBP_PropertyChanged, NULL);
    /* set ready */
    g_dbus_client_set_ready_watch(p_dbusClient, APP_DBP_ClientReady,  NULL);

    /*running in a loop*/
    bt_shell_run();
    
    APP_Deinitialize();
    
    /*release the resources*/
    g_dbus_client_unref(p_dbusClient);
    dbus_connection_unref(sp_dbusConn);

    return 0;
}

/*******************************************************************************
 End of File
*/


