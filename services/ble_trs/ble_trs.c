/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BLE Transparent Service Source File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trs.c

  Summary:
    This file contains the BLE Transparent Service functions for application user.

  Description:
    This file contains the BLE Transparent Service functions for application user.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ble_trs/ble_trs.h"
#include "ble_trsp/ble_trsps.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define BLUEZ_SERVICE_INTERFACE "org.bluez.GattService1"
#define BLUEZ_CHRC_INTERFACE "org.bluez.GattCharacteristic1"


#define TRS_SERVICE_OBJ_PATH    "/org/mchp/trs"

#define TRS_CHRC_TX_OBJ_PATH    "/org/mchp/trs/tx"
#define TRS_CHRC_RX_OBJ_PATH    "/org/mchp/trs/rx"
#define TRS_CHRC_CTRL_OBJ_PATH  "/org/mchp/trs/ctrl"


#define TRS_CHRC_NUM            (3)
// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
typedef struct _BLE_TRS_Characteristic_T
{
	const char *p_path;
	const char *p_uuid;
	const char **pp_flags;
    const GDBusMethodTable *p_methods;
}_BLE_TRS_Characteristic_T;


// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
static gboolean ble_trs_SvcGetUuid(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);
static gboolean ble_trs_SvcGetPrimary(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);
static gboolean ble_trs_ChrcGetUuid(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);
static gboolean ble_trs_ChrcGetSvc(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);
static gboolean ble_trs_ChrcGetValue(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);
static gboolean ble_trs_ChrcGetFlags(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data);


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
/* Service Properties table*/
static const GDBusPropertyTable s_trsSvcProp[] = {
	{ "UUID", "s", ble_trs_SvcGetUuid },
	{ "Primary", "b", ble_trs_SvcGetPrimary },
	{ }
};

/* Characteristic Properties table*/
static const GDBusPropertyTable s_trsChrcProp[] = {
	{ "UUID", "s", ble_trs_ChrcGetUuid, NULL, NULL },
	{ "Service", "o", ble_trs_ChrcGetSvc, NULL, NULL },
	{ "Value", "ay", ble_trs_ChrcGetValue, NULL, NULL },
	{ "Flags", "as", ble_trs_ChrcGetFlags, NULL, NULL },
	{ }
};

/* Characteristic value for notification */
static uint8_t *sp_trsChrcValue = NULL;
static uint16_t s_trsChrcValueLen = 0;


/* Characteristic Flags */
static const char *s_trsChrcFlagTx[] = {"notify", NULL};
static const char *s_trsChrcFlagRx[] = {"write","write-without-response", NULL};
static const char *s_trsChrcFlagCtrl[] = {"write","write-without-response","notify", NULL};

/* Characteristic methods table*/
static const GDBusMethodTable s_trsMethodsTx[] = {
	{ GDBUS_ASYNC_METHOD("StartNotify", NULL, NULL, BLE_TRSPS_ChrcStartNotifyTx) },
	{ GDBUS_METHOD("StopNotify", NULL, NULL, BLE_TRSPS_ChrcStopNotifyTx) },
	{ }
};

static const GDBusMethodTable s_trsMethodsRx[] = {
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, BLE_TRSPS_ChrcWriteValueRx) },
	{ }
};

static GDBusMethodTable s_trsMethodsCtrl[] = {
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, BLE_TRSPS_ChrcWriteValueCtrl) },
	{ GDBUS_ASYNC_METHOD("StartNotify", NULL, NULL, BLE_TRSPS_ChrcStartNotifyCtrl) },
	{ GDBUS_METHOD("StopNotify", NULL, NULL, BLE_TRSPS_ChrcStopNotifyCtrl) },
	{ }
};


/* Characteristic table */
_BLE_TRS_Characteristic_T s_trsChrc[TRS_CHRC_NUM]=
{
    {
        TRS_CHRC_CTRL_OBJ_PATH,
        UUID_MCHP_TRANS_CTRL_16, 
        s_trsChrcFlagCtrl,
        s_trsMethodsCtrl
    },
    {
        TRS_CHRC_RX_OBJ_PATH,
        UUID_MCHP_TRANS_RX_16,
        s_trsChrcFlagRx,
        s_trsMethodsRx
    },
    {
        TRS_CHRC_TX_OBJ_PATH,
        UUID_MCHP_TRANS_TX_16,
        s_trsChrcFlagTx,
        s_trsMethodsTx
    }
};


static DBusConnection *sp_trsDbusConn;

// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************


static gboolean ble_trs_SvcGetUuid(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
	char uuid[] = {UUID_MCHP_PROPRIETARY_SERVICE_16};
	const char *p_ptr = uuid;

	dbus_message_iter_append_basic(p_iter, DBUS_TYPE_STRING, &p_ptr);

	return TRUE;
}

static gboolean ble_trs_SvcGetPrimary(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
	dbus_bool_t primary;

	primary = TRUE;

	dbus_message_iter_append_basic(p_iter, DBUS_TYPE_BOOLEAN, &primary);

	return TRUE;
}



static bool ble_trs_RegisterSvc(DBusConnection *p_dbusConn)
{
	if (g_dbus_register_interface(p_dbusConn, TRS_SERVICE_OBJ_PATH,
					BLUEZ_SERVICE_INTERFACE, NULL, NULL,
					s_trsSvcProp, NULL,
					NULL) == FALSE) {
        return false;
	}

    return true;
}

static gboolean ble_trs_ChrcGetUuid(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
	_BLE_TRS_Characteristic_T *p_chrc = p_data;

	dbus_message_iter_append_basic(p_iter, DBUS_TYPE_STRING, &p_chrc->p_uuid);

	return TRUE;
}

static gboolean ble_trs_ChrcGetSvc(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
    const char *p_str = TRS_SERVICE_OBJ_PATH;

	dbus_message_iter_append_basic(p_iter, DBUS_TYPE_OBJECT_PATH,
						&p_str);

	return TRUE;
}

static gboolean ble_trs_ChrcGetValue(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
	DBusMessageIter array;

	dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY, "y", &array);

	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&sp_trsChrcValue, s_trsChrcValueLen);

	dbus_message_iter_close_container(p_iter, &array);

	return TRUE;
}

static gboolean ble_trs_ChrcGetFlags(const GDBusPropertyTable *p_property,
					DBusMessageIter *p_iter, void *p_data)
{
	_BLE_TRS_Characteristic_T *p_chrc = p_data;
	int i;
	DBusMessageIter array;

	dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY, "s", &array);

	for (i = 0; p_chrc->pp_flags[i]; i++)
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING,
							&p_chrc->pp_flags[i]);

	dbus_message_iter_close_container(p_iter, &array);

	return TRUE;
}

static bool ble_trs_RegisterChrc(DBusConnection *p_dbusConn, _BLE_TRS_Characteristic_T *p_chrc)
{
	if (g_dbus_register_interface(p_dbusConn, p_chrc->p_path, BLUEZ_CHRC_INTERFACE,
					p_chrc->p_methods, NULL, s_trsChrcProp,
					p_chrc, NULL) == FALSE) {
		return false;
	}

	return true;
}

static void ble_trs_RegisterAppSetup(DBusMessageIter *p_iter, void *p_userData)
{
	DBusMessageIter opt;
	const char *p_path = "/";

	dbus_message_iter_append_basic(p_iter, DBUS_TYPE_OBJECT_PATH, &p_path);

	dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&opt);
	dbus_message_iter_close_container(p_iter, &opt);

}

static void ble_trs_RegisterAppReply(DBusMessage *p_message, void *p_userData)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, p_message) == TRUE) {
		dbus_error_free(&error);
		return ;
	}

	return ;
}


static bool ble_trs_RegisterApp(GDBusProxy *p_proxyGattMgr)
{
	if (g_dbus_proxy_method_call(p_proxyGattMgr, "RegisterApplication",
						ble_trs_RegisterAppSetup,
						ble_trs_RegisterAppReply, NULL,
						NULL) == FALSE) {
		return false;
	}

    return true;
}


bool BLE_TRS_Add(DBusConnection *p_dbusConn, GDBusProxy * p_proxyGattMgr) 
{
    uint8_t i;

    if (ble_trs_RegisterSvc(p_dbusConn) == false)
    {
        return false;
    }

    for (i = 0; i < TRS_CHRC_NUM; i++)
    {
        if (ble_trs_RegisterChrc(p_dbusConn, &s_trsChrc[i]) == false)
        {
            return false;
        }
    }
   
    if (ble_trs_RegisterApp(p_proxyGattMgr) == false)
    {
        return false;
    }

    sp_trsDbusConn = p_dbusConn;

    return true;
}

void BLE_TRS_UpdateValueCtrl(uint8_t *p_value, uint16_t len)
{
    sp_trsChrcValue = p_value;
    s_trsChrcValueLen = len;

    g_dbus_emit_property_changed_full(sp_trsDbusConn, TRS_CHRC_CTRL_OBJ_PATH, BLUEZ_CHRC_INTERFACE, "Value", G_DBUS_PROPERTY_CHANGED_FLAG_FLUSH);
}

void BLE_TRS_UpdateValueTx(uint8_t *p_value, uint16_t len)
{
    sp_trsChrcValue = p_value;
    s_trsChrcValueLen = len;

    g_dbus_emit_property_changed_full(sp_trsDbusConn, TRS_CHRC_TX_OBJ_PATH, BLUEZ_CHRC_INTERFACE, "Value", G_DBUS_PROPERTY_CHANGED_FLAG_FLUSH);
}


