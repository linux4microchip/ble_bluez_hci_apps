/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Pairing Agent Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_agent.c

  Summary:
    This file contains the Application Pairing Agent function.

  Description:
    This file contains the Application pairing Agent function.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include "shared/shell.h"
#include "gdbus/gdbus.h"
#include "app_agent.h"

#define AGENT_PATH "/org/bluez/agent"
#define AGENT_INTERFACE "org.bluez.Agent1"

#define AGENT_PROMPT	COLOR_RED "[agent]" COLOR_OFF " "

#define IO_CAPABILITY_DISPLAYONLY       "DisplayOnly"
#define IO_CAPABILITY_DISPLAYYESNO      "DisplayYesNo"
#define IO_CAPABILITY_KEYBOARDONLY      "KeyboardOnly"
#define IO_CAPABILITY_NOINPUTNOOUTPUT   "NoInputNoOutput"
#define IO_CAPABILITY_KEYBOARDDISPLAY   "KeyboardDisplay"

const char * sp_ioCapStr[] = {
    IO_CAPABILITY_DISPLAYONLY,
    IO_CAPABILITY_DISPLAYYESNO,
    IO_CAPABILITY_KEYBOARDONLY,
    IO_CAPABILITY_NOINPUTNOOUTPUT,
    IO_CAPABILITY_KEYBOARDDISPLAY,
};

static gboolean s_agentRegistered = FALSE;
static const char *sp_agentCapability = NULL;
static DBusMessage *sp_pendingMessage = NULL;
GDBusProxy *sp_agentManager = NULL; 
static uint8_t s_newIoCap = 0xFF;
static uint8_t s_ioCap;




static void app_agt_ReleasePrompt(void)
{
    if (!sp_pendingMessage)
        return;

    bt_shell_release_prompt("");
}

static void app_agt_PincodeResponse(const char *p_input, void *p_userData)
{
    DBusConnection *p_conn = p_userData;

    g_dbus_send_reply(p_conn, sp_pendingMessage, DBUS_TYPE_STRING, &p_input, DBUS_TYPE_INVALID);
}

static void app_agt_PasskeyResponse(const char *p_input, void *p_userData)
{
    DBusConnection *p_conn = p_userData;
    dbus_uint32_t passkey;

    if (sscanf(p_input, "%u", &passkey) == 1)
        g_dbus_send_reply(p_conn, sp_pendingMessage, DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);
    else if (!strcmp(p_input, "no"))
        g_dbus_send_error(p_conn, sp_pendingMessage, "org.bluez.Error.Rejected", NULL);
	else
        g_dbus_send_error(p_conn, sp_pendingMessage, "org.bluez.Error.Canceled", NULL);
}

static void app_agt_ConfirmResponse(const char *p_input, void *p_userData)
{
    DBusConnection *p_conn = p_userData;

    if (!strcmp(p_input, "yes"))
        g_dbus_send_reply(p_conn, sp_pendingMessage, DBUS_TYPE_INVALID);
    else if (!strcmp(p_input, "no"))
        g_dbus_send_error(p_conn, sp_pendingMessage, "org.bluez.Error.Rejected", NULL);
    else
        g_dbus_send_error(p_conn, sp_pendingMessage, "org.bluez.Error.Canceled", NULL);
}

static void app_agt_Release(DBusConnection *p_conn)
{
    s_agentRegistered = FALSE;
    sp_agentCapability = NULL;

    if (sp_pendingMessage) {
        dbus_message_unref(sp_pendingMessage);
        sp_pendingMessage = NULL;
    }

    app_agt_ReleasePrompt();

    g_dbus_unregister_interface(p_conn, AGENT_PATH, AGENT_INTERFACE);
}

static DBusMessage *app_agt_DbusReleaseAgent(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    bt_shell_printf("Agent released\n");

    app_agt_Release(p_conn);

    return dbus_message_new_method_return(p_msg);
}

static DBusMessage *app_agt_DbusRequestPincode(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;

    bt_shell_printf("Request PIN code\n");

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_INVALID);

    bt_shell_prompt_input("agent", "Enter PIN code:", app_agt_PincodeResponse, p_conn);

    sp_pendingMessage = dbus_message_ref(p_msg);

    return NULL;
}

static DBusMessage *app_agt_DbusDisplayPincode(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;
    const char *pincode;

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_STRING, &pincode, DBUS_TYPE_INVALID);

    bt_shell_printf(AGENT_PROMPT "PIN code: %s\n", pincode);

    return dbus_message_new_method_return(p_msg);
}

static DBusMessage *app_agt_DbusRequestPasskey(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;

    bt_shell_printf("Request passkey\n");

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_INVALID);

    bt_shell_prompt_input("agent", "Enter passkey (number in 0-999999):", app_agt_PasskeyResponse, p_conn);

    sp_pendingMessage = dbus_message_ref(p_msg);

    return NULL;
}

static DBusMessage *app_agt_DbusDisplayPasskey(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;
    dbus_uint32_t passkey;
    dbus_uint16_t entered;
    char passkey_full[7];

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device,
            DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_UINT16, &entered, DBUS_TYPE_INVALID);

    snprintf(passkey_full, sizeof(passkey_full), "%.6u", passkey);
    passkey_full[6] = '\0';

    if (entered > strlen(passkey_full))
        entered = strlen(passkey_full);

    bt_shell_printf(AGENT_PROMPT "Passkey: "
            COLOR_BOLDGRAY "%.*s" COLOR_BOLDWHITE "%s\n" COLOR_OFF,
            entered, passkey_full, passkey_full + entered);

    return dbus_message_new_method_return(p_msg);
}

static DBusMessage *app_agt_DbusRequestConfirm(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;
    dbus_uint32_t passkey;
    char *str;

    bt_shell_printf("Request confirmation\n");

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);

    str = g_strdup_printf("Confirm passkey %06u (yes/no):", passkey);
    bt_shell_prompt_input("agent", str, app_agt_ConfirmResponse, p_conn);
    g_free(str);

    sp_pendingMessage = dbus_message_ref(p_msg);

    return NULL;
}

static DBusMessage *app_agt_DbusRequestAuth(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device;

    bt_shell_printf("Request authorization\n");

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_INVALID);

    if (s_ioCap != BLE_SMP_IO_NOINPUTNOOUTPUT)
    {
        bt_shell_prompt_input("agent", "Accept pairing (yes/no):", app_agt_ConfirmResponse, p_conn);
    }

    sp_pendingMessage = dbus_message_ref(p_msg);

    if (s_ioCap == BLE_SMP_IO_NOINPUTNOOUTPUT)
    {
        bt_shell_printf("auto-accept pairing (IO Capability=%s)\n", sp_ioCapStr[s_ioCap]);
        g_dbus_send_reply(p_conn, sp_pendingMessage, DBUS_TYPE_INVALID);
    }


    return NULL;
}

static DBusMessage *app_agt_DbusAuthService(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    const char *p_device, *p_uuid;
    char *p_str;

    bt_shell_printf("Authorize service\n");

    dbus_message_get_args(p_msg, NULL, DBUS_TYPE_OBJECT_PATH, &p_device, DBUS_TYPE_STRING, &p_uuid, DBUS_TYPE_INVALID);

    p_str = g_strdup_printf("Authorize service %s (yes/no):", p_uuid);
    bt_shell_prompt_input("agent", p_str, app_agt_ConfirmResponse, p_conn);
    g_free(p_str);

    sp_pendingMessage = dbus_message_ref(p_msg);

    return NULL;
}

static DBusMessage *app_agt_DbusCancelRequest(DBusConnection *p_conn, DBusMessage *p_msg, void *p_userData)
{
    bt_shell_printf("Request canceled\n");

    app_agt_ReleasePrompt();
    dbus_message_unref(sp_pendingMessage);
    sp_pendingMessage = NULL;

    return dbus_message_new_method_return(p_msg);
}

static const GDBusMethodTable agentMethods[] = {
    { GDBUS_METHOD("Release", NULL, NULL, app_agt_DbusReleaseAgent) },
    { GDBUS_ASYNC_METHOD("RequestPinCode",
            GDBUS_ARGS({ "device", "o" }),
            GDBUS_ARGS({ "pincode", "s" }), app_agt_DbusRequestPincode) },
    { GDBUS_METHOD("DisplayPinCode",
            GDBUS_ARGS({ "device", "o" }, { "pincode", "s" }),
            NULL, app_agt_DbusDisplayPincode) },
    { GDBUS_ASYNC_METHOD("RequestPasskey",
            GDBUS_ARGS({ "device", "o" }),
            GDBUS_ARGS({ "passkey", "u" }), app_agt_DbusRequestPasskey) },
    { GDBUS_METHOD("DisplayPasskey",
            GDBUS_ARGS({ "device", "o" }, { "passkey", "u" },
                            { "entered", "q" }),
            NULL, app_agt_DbusDisplayPasskey) },
    { GDBUS_ASYNC_METHOD("RequestConfirmation",
            GDBUS_ARGS({ "device", "o" }, { "passkey", "u" }),
            NULL, app_agt_DbusRequestConfirm) },
    { GDBUS_ASYNC_METHOD("RequestAuthorization",
            GDBUS_ARGS({ "device", "o" }),
            NULL, app_agt_DbusRequestAuth) },
    { GDBUS_ASYNC_METHOD("AuthorizeService",
            GDBUS_ARGS({ "device", "o" }, { "uuid", "s" }),
            NULL,  app_agt_DbusAuthService) },
    { GDBUS_METHOD("Cancel", NULL, NULL, app_agt_DbusCancelRequest) },
    { }
};

static void app_agt_RegAgentSetup(DBusMessageIter *p_iter, void *p_userData)
{
    const char *path = AGENT_PATH;
    const char *p_capability = sp_agentCapability;

    dbus_message_iter_append_basic(p_iter, DBUS_TYPE_OBJECT_PATH, &path);
    dbus_message_iter_append_basic(p_iter, DBUS_TYPE_STRING, &p_capability);
}

static void app_agt_RegAgentReply(DBusMessage *p_message, void *p_userData)
{
    DBusConnection *p_dbusConn = p_userData;
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, p_message) == FALSE) {
        s_agentRegistered = TRUE;
        bt_shell_printf("Agent registered (IO Capability=%s)\n", sp_ioCapStr[s_ioCap]);
    } else {
        bt_shell_printf("Failed to register agent(reply): %s\n", error.name);
        dbus_error_free(&error);

        if (g_dbus_unregister_interface(p_dbusConn, AGENT_PATH, AGENT_INTERFACE) == FALSE)
            bt_shell_printf("Failed to unregister agent object\n");
    }
}


void APP_AGT_Register(GDBusProxy *p_manager, uint8_t ioCap)
{
    DBusConnection *p_dbusConn = bt_shell_get_env("DBUS_CONNECTION");

    if (s_agentRegistered == TRUE) {
        bt_shell_printf("Agent is already registered\n");
        return;
    }

    s_ioCap = ioCap;
    sp_agentCapability = sp_ioCapStr[ioCap];

    if (g_dbus_register_interface(p_dbusConn, AGENT_PATH, AGENT_INTERFACE, agentMethods, 
            NULL, NULL, NULL, NULL) == FALSE) {
        bt_shell_printf("Failed to register agent object\n");
        return;
    }

	if (g_dbus_proxy_method_call(p_manager, "RegisterAgent", app_agt_RegAgentSetup, app_agt_RegAgentReply,
            p_dbusConn, NULL) == FALSE) {
        bt_shell_printf("Failed to call register agent method\n");
        return;
    }

    sp_agentCapability = NULL;
}

static void app_agt_UnregAagentSetup(DBusMessageIter *p_iter, void *p_userData)
{
    const char *p_path = AGENT_PATH;

    dbus_message_iter_append_basic(p_iter, DBUS_TYPE_OBJECT_PATH, &p_path);
}

static void app_agt_unregAgentReply(DBusMessage *p_message, void *p_userData)
{
    DBusConnection *p_dbusConn = p_userData;
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, p_message) == FALSE) {
        bt_shell_printf("Agent unregistered\n");
        app_agt_Release(p_dbusConn);
        if (sp_agentManager && s_newIoCap != 0xFF)
        {
            APP_AGT_Register(sp_agentManager, s_newIoCap);
            sp_agentManager = NULL;
            s_newIoCap = 0xFF;
        }
    } else {
        bt_shell_printf("Failed to unregister agent: %s\n", error.name);
        dbus_error_free(&error);
    }
}

void APP_AGT_Unregister(GDBusProxy *p_manager)
{
    DBusConnection *p_dbusConn = bt_shell_get_env("DBUS_CONNECTION");

    if (s_agentRegistered == FALSE) {
        bt_shell_printf("No agent is registered\n");
        return;
    }

    if (!p_manager) {
        bt_shell_printf("Agent unregistered\n");
        app_agt_Release(p_dbusConn);
        return;
    }

    if (g_dbus_proxy_method_call(p_manager, "UnregisterAgent", app_agt_UnregAagentSetup, app_agt_unregAgentReply,
            p_dbusConn, NULL) == FALSE) {
        bt_shell_printf("Failed to call unregister agent method\n");
        return;
    }
}

void APP_AGT_ChangeIoCap(GDBusProxy *p_manager, uint8_t ioCap)
{
    APP_AGT_Unregister(p_manager);

    //pending the new IO Capability until Agent unregister successfully.
    sp_agentManager = p_manager;
    s_newIoCap = ioCap;
}

const char * APP_AGT_GetIoCap(uint8_t *p_ioCap)
{
    if (p_ioCap)
        *p_ioCap = s_ioCap;
    
    return sp_ioCapStr[s_ioCap];
}



