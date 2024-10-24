/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application DBus Proxy Handler Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dbp.c

  Summary:
    This file contains the Application DBus Proxy Handler for this project.

  Description:
    This file contains the Application DBus Proxy Handler for this project.
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
#include <string.h>
#include <sys/uio.h>
#include <stdbool.h>
#include <glib.h>
#include "gdbus/gdbus.h"
#include "shared/shell.h"
#include "shared/util.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/mgmt.h"
#include "bluetooth/uuid.h"

#include "application.h"
#include "app_dbp.h"
#include "app_log.h"
#include "app_timer.h"
#include "app_scan.h"
#include "app_sm.h"
#include "app_ble_handler.h"
#include "app_error_defs.h"
#include "app_mgmt.h"
#include "app_agent.h"
#include "ble_trsp/ble_trsps.h"
#include "ble_trsp/ble_trspc.h"
#include "ble_trsp/ble_trsp_defs.h"





// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define PROMPT_ON   COLOR_BLUE "[BLE UART]" COLOR_OFF "# "


#define DISTANCE_VAL_INVALID                0x7FFF
#define DEV_LIST_RSSI_THRESHOLD             (-70)


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
typedef struct APP_DBP_CtrlData_T
{
    GList  *  p_deviceList;
    GDBusProxy * p_controller;
    GDBusProxy * p_pairAgent;
} APP_DBP_CtrlData_T;

typedef struct APP_DBP_SetDiscoveryFilterArgs_T {
    char *p_transport;
    dbus_uint16_t rssi;
    dbus_int16_t pathloss;
    char **pp_uuids;
    size_t uuids_len;
    dbus_bool_t duplicate;
    char *p_pattern;
}APP_DBP_SetDiscoveryFilterArgs_T;



// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_DBP_CtrlData_T  s_dbpCtrl;
//static GattCharacteristic * sp_csfProxy; /*Client Supported Features*/
//static GattCharacteristic * sp_ssfProxy; /*Server Supported Features*/



// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static APP_DBP_BtDev_T * app_dbp_FindScanResultByAddress(const char * p_address);
static APP_DBP_BtDev_T * app_dbp_GetDeviceInfoByProxy(DeviceProxy * p_proxy);
static void app_dbp_SrvResolved(DeviceProxy * p_proxy, bool reportWhenResolved);
static void app_dbp_SendConnStatMsg(APP_DBP_BtDev_T * p_dev, bool isSuccess);
static void app_dbp_SortingDeviceList(void);



GDBusProxy * APP_DBP_GetDefaultAdapter(void)
{
    return s_dbpCtrl.p_controller;
}

GDBusProxy * APP_DBP_GetPairAgent(void)
{
    return s_dbpCtrl.p_pairAgent;
}


void APP_DBP_Init(void)
{
    memset(&s_dbpCtrl, 0, sizeof(s_dbpCtrl));
}

static int app_dbp_SortingDevListCompareFunc(const void* p_a, const void* p_b)
{
    APP_DBP_BtDev_T * p_devA = (APP_DBP_BtDev_T *) p_a;
    APP_DBP_BtDev_T * p_devB = (APP_DBP_BtDev_T *) p_b;

    if (p_devA->rssi == p_devB->rssi)
        return 0;
    else if (p_devA->rssi < p_devB->rssi)
        return 1;
    else 
        return -1;
}

static void app_dbp_FreeBtDev(APP_DBP_BtDev_T * p_dev)
{
    if (p_dev == NULL)
        return;
    
    if (p_dev->p_name)
        g_free(p_dev->p_name);
    if (p_dev->p_address)
        g_free(p_dev->p_address);
    if (p_dev->p_addressType)
        g_free(p_dev->p_addressType);
    g_free(p_dev);
}


void APP_DBP_PrintDeviceList(void)
{
    GList *p_l;
    bool isConnected;
    bool isPostFilter = false;
    char gapRole;
    APP_SCAN_Filter_T *p_scanFilter;
    uint16_t devCount = 0;
    APP_DBP_BtDev_T * p_dev;

    app_dbp_SortingDeviceList();
    p_scanFilter = APP_SCAN_GetFilter();

    if (p_scanFilter->isConfigured == false)
        isPostFilter = true;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        p_dev = p_l->data;
        if (p_dev->isValid == true)
            devCount++;
    }

    if (isPostFilter)
        printf("\nTotal devices = %d, only list devices which RSSI > -70\n", devCount);
    else
        printf("\nTotal devices =  %d\n", devCount);
    
    printf("Device List:\n");
    printf("[Index]\t[  Connected:? ][     Address     ][RSSI][Name]\n");
    printf("=================================================================================\n");
    
    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        p_dev = p_l->data;

        if (p_dev->isValid == false)
            continue;

        if ((isPostFilter == false) ||
            ((isPostFilter == true) && (p_dev->rssi > DEV_LIST_RSSI_THRESHOLD)))
        {
            isConnected = p_dev->isConnected;
            if (p_dev->role == BLE_GAP_ROLE_CENTRAL)
                gapRole = 'C';
            else if (p_dev->role == BLE_GAP_ROLE_PERIPHERAL)
                gapRole = 'P';
            else
                gapRole = 'N';
            
            if (p_dev->p_name)
                printf("dev#%2d\t[connected:%d(%c)][%s][ %3d][%s]\n", 
                    p_dev->index, isConnected, gapRole, p_dev->p_address, p_dev->rssi, p_dev->p_name);
            else
                printf("dev#%2d\t[connected:%d(%c)][%s][ %3d]\n", 
                    p_dev->index, isConnected, gapRole, p_dev->p_address, p_dev->rssi);
        }
    }
    bt_shell_printf("\n");
}


static void app_dbp_SortingDeviceList(void)
{
    GList *p_l = NULL;
    uint8_t scanListIndex = 0;
    APP_SCAN_Filter_T * p_scanFilter = NULL;
    p_scanFilter = APP_SCAN_GetFilter();

    if (p_scanFilter->isFilterSrvUuid == true)
    {
        p_l = s_dbpCtrl.p_deviceList;
        while (p_l)
        {
            APP_DBP_BtDev_T * p_dev = p_l->data;
            p_l = g_list_next(p_l);
            if (p_dev != NULL && 
                p_dev->isConnected != true && 
                p_dev->isCached != true && 
                p_dev->srvDataUuidMatch == false)
            {
                p_dev->isValid = false;
                p_dev->rssi = -0xFF; //move this device to tail when sorting
            }
        }
    }

    if (p_scanFilter->isFilterManufData == true)
    {
        p_l = s_dbpCtrl.p_deviceList;
        while (p_l)
        {
            APP_DBP_BtDev_T * p_dev = p_l->data;
            p_l = g_list_next(p_l);
            if (p_dev != NULL && 
                p_dev->isConnected != true && 
                p_dev->isCached != true && 
                p_dev->manufDataMatch == false)
            {
                p_dev->isValid = false;
                p_dev->rssi = -0xFF; //move this device to tail when sorting
            }
        }
    }


    //sorting device list by RSSI comparison
    s_dbpCtrl.p_deviceList = g_list_sort(s_dbpCtrl.p_deviceList, app_dbp_SortingDevListCompareFunc);

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T *p_dev = p_l->data;
        if (p_dev)
        {
            p_dev->index = scanListIndex++;
        }
    }

}

static void app_dbp_CheckServiceInCachedDevice(void)
{
    GList *p_l;
    
    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        
        if (p_dev->isCached == false) continue;

        app_dbp_SrvResolved(p_dev->p_devProxy, true);
    }

}

void APP_DBP_ConnectByIndex(int idx)
{
    GList *p_l;
    APP_BLE_ConnList_T *p_bleConn;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->index == idx)
        {
            if(APP_DBP_ConnectDevice(p_dev))
            {
                p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_SCANNING, APP_BLE_STATE_SCANNING);
                if (p_bleConn == NULL)
                {
                    p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_STANDBY, APP_BLE_STATE_STANDBY);

                }
                APP_SetBleStateByLink(p_bleConn, APP_BLE_STATE_CONNECTING);
                
                bt_shell_printf("connecting to device[%s]\n", p_dev->p_address);
            }
            else
            {
                bt_shell_printf("connecting to device[%s] failed\n", p_dev->p_address);
            }
            break;
        }
    }
}

void APP_DBP_DisconnectByIndex(int idx)
{
    GList *p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->index == idx)
        {
            if (APP_DBP_DisconnectDevice(p_dev))
            {
                bt_shell_printf("disconnecting to device[%s]\n", p_dev->p_address);
            }
            else
            {
                bt_shell_printf("disconnect to device[%s] failed\n", p_dev->p_address);
            }
            break;
        }
    }
}

void APP_DBP_DisconnectAll(void)
{
    GList *p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if(p_dev->isConnected == false)
            continue;
        
        if(APP_DBP_DisconnectDevice(p_dev))
        {
            printf("disconnecting to device[%s]\n", p_dev->p_address);
        }
        else
        {
            printf("disconnecting to device[%s] failed\n", p_dev->p_address);
        }
    }
}

static void app_dbp_ParseServiceData(APP_DBP_BtDev_T *p_scanDev, DeviceProxy *p_proxy)
{
    DBusMessageIter iter;
    DBusMessageIter entries;
    APP_SCAN_Filter_T * p_scanFilter = NULL;
    p_scanFilter = APP_SCAN_GetFilter();

    if (p_scanFilter->isFilterSrvUuid == false)
        return;
    
    if (g_dbus_proxy_get_property(p_proxy, "ServiceData", &iter) == FALSE)
        return;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        return;

    dbus_message_iter_recurse(&iter, &entries);

    while (dbus_message_iter_get_arg_type(&entries) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter value, entry, array;
        const char *uuid_str;
        bt_uuid_t uuid;
        uint8_t *service_data;
        int len;

        dbus_message_iter_recurse(&entries, &entry);
        dbus_message_iter_get_basic(&entry, &uuid_str);
        if (p_scanDev->srvDataUuidMatch == false &&
            !strcmp(uuid_str, p_scanFilter->p_srvUuid))
        {
            p_scanDev->srvDataUuidMatch = true;
            //printf("ServiceData matched::[%s]uuid_str=%s\n", p_scanDev->p_address, uuid_str);
        }

        if (bt_string_to_uuid(&uuid, uuid_str) < 0)
            break;

        dbus_message_iter_next(&entry);

        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
            break;

        dbus_message_iter_recurse(&entry, &value);

        if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_ARRAY)
            break;

        dbus_message_iter_recurse(&value, &array);

        if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_BYTE)
            break;

        dbus_message_iter_get_fixed_array(&array, &service_data, &len);

        dbus_message_iter_next(&entries);
    }
}

static void app_dbp_ParseManufacturerData(APP_DBP_BtDev_T *p_scanDev, DeviceProxy *p_proxy)
{
    DBusMessageIter iter;
    DBusMessageIter entries;

    APP_SCAN_Filter_T * p_scanFilter = NULL;
    p_scanFilter = APP_SCAN_GetFilter();

    if (p_scanFilter->isFilterManufData == false)
        return;
    
    if (g_dbus_proxy_get_property(p_proxy, "ManufacturerData", &iter) == FALSE)
        return;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
        return;

    dbus_message_iter_recurse(&iter, &entries);

    while (dbus_message_iter_get_arg_type(&entries) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter value, entry, array;
        uint16_t manuf_id;
        uint8_t *manuf_data;
        int len;

        dbus_message_iter_recurse(&entries, &entry);
        dbus_message_iter_get_basic(&entry, &manuf_id);

        dbus_message_iter_next(&entry);

        if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_VARIANT)
            return;

        dbus_message_iter_recurse(&entry, &value);

        if (dbus_message_iter_get_arg_type(&value) != DBUS_TYPE_ARRAY)
            return;

        dbus_message_iter_recurse(&value, &array);

        if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_BYTE)
            return;

        dbus_message_iter_get_fixed_array(&array, &manuf_data, &len);


                
        if ((p_scanDev->manufDataMatch == false) &&
            (manuf_id == p_scanFilter->manufId) &&
            (!memcmp(manuf_data, p_scanFilter->p_manufData, p_scanFilter->manufDataLen)))
        {
            p_scanDev->manufDataMatch = true;
#if 0
            printf("ManufacturerData for [%s] {%04x}(%d) = ", p_scanDev->p_address, manuf_id, len);
            for (uint8_t i=0; i<len; i++)
                printf("%02x", manuf_data[i]);
            printf("\n");
#endif

        }

        dbus_message_iter_next(&entries);
    }

    return;
}


static bool app_dbp_AddScanResult (DeviceProxy * p_proxy, bool isCached, const char *p_propName)
{
    DBusMessageIter iter;
    const char *p_address, *p_name, *p_addressType;
    dbus_bool_t connected;
    int16_t rssi;
    int16_t txpower;
    APP_DBP_BtDev_T *p_scanDev;
    bool isNewAdded = false;

    if (g_dbus_proxy_get_property(p_proxy, "Address", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_address);
    }

    if (g_dbus_proxy_get_property(p_proxy, "AddressType", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_addressType);
    }

    if (p_propName && 
        (!strcmp(p_propName, "Address") || 
        !strcmp(p_propName, "AddressType")))
    {
        p_scanDev = app_dbp_GetDeviceInfoByProxy(p_proxy);
        if (p_scanDev)
        {
            g_free(p_scanDev->p_address);
            g_free(p_scanDev->p_addressType);
            p_scanDev->p_address = g_strdup(p_address);
            p_scanDev->p_addressType = g_strdup(p_addressType);
        }
    }

    p_scanDev = app_dbp_FindScanResultByAddress(p_address);
    if (p_scanDev == NULL)
    {
        p_scanDev = g_new0(APP_DBP_BtDev_T, 1);
        if (p_scanDev == NULL) 
            return false;

        printf("found device[%s][%s]\n", p_address, p_addressType);
        p_scanDev->p_address = g_strdup(p_address);
        p_scanDev->p_addressType = g_strdup(p_addressType);
        p_scanDev->p_devProxy = p_proxy;
        p_scanDev->isCached = isCached;
        p_scanDev->p_name = NULL;
        p_scanDev->srvDataUuidMatch = false;
        p_scanDev->manufDataMatch = false;
        p_scanDev->attMtu = 0;
        p_scanDev->role = -1;
        p_scanDev->isValid = true;
        p_scanDev->p_charCheckMtu = NULL;

        s_dbpCtrl.p_deviceList = g_list_append(s_dbpCtrl.p_deviceList, p_scanDev);
        isNewAdded = true;
    }

    if (p_scanDev == NULL)
        return false;


    if (isNewAdded || !strcmp(p_propName, "Connected"))
    {
        if (g_dbus_proxy_get_property(p_proxy, "Connected", &iter)) {
            dbus_message_iter_get_basic(&iter, &connected);
            p_scanDev->isConnected = connected;
        }
    }

    if (isNewAdded || !strcmp(p_propName, "ServiceData"))
    {
        app_dbp_ParseServiceData(p_scanDev, p_proxy);
    }
    
    if (isNewAdded || !strcmp(p_propName, "ManufacturerData"))
    {
        app_dbp_ParseManufacturerData(p_scanDev, p_proxy);
    }

    if (isNewAdded || !strcmp(p_propName, "RSSI"))
    {
        if (g_dbus_proxy_get_property(p_proxy, "RSSI", &iter)) {
            dbus_message_iter_get_basic(&iter, &rssi);
            p_scanDev->rssi = rssi;
        }
    }

    if (isNewAdded || !strcmp(p_propName, "TxPower"))
    {
        if (g_dbus_proxy_get_property(p_proxy, "TxPower", &iter)) {
            dbus_message_iter_get_basic(&iter, &txpower);
            p_scanDev->txPower = txpower;
        }
    }

    if (isNewAdded || !strcmp(p_propName, "Name"))
    {
        if (g_dbus_proxy_get_property(p_proxy, "Name", &iter)) {
            dbus_message_iter_get_basic(&iter, &p_name);
            if (p_scanDev->p_name == NULL) {
                p_scanDev->p_name = g_strdup(p_name);
            }
        }
    }

    return connected;
}


static APP_DBP_BtDev_T * app_dbp_FindScanResultByAddress(const char * p_address)
{
    GList * p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (!strcmp(p_dev->p_address, p_address) )
            return p_dev;
    }
    return NULL;
}


static void app_dbp_RemoveDevice(DeviceProxy * p_proxy)
{
    GList * p_l;
    
    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        APP_DBP_BtDev_T *p_dev = p_l->data;
        if (p_dev != NULL && p_dev->p_devProxy == p_proxy)
        {
            //printf("remove dev[%s]\n", p_dev->p_address);
            s_dbpCtrl.p_deviceList = g_list_remove(s_dbpCtrl.p_deviceList, p_dev);
            app_dbp_FreeBtDev(p_dev);
            break;
        }
    }
}


static void app_dbp_StartDiscoveryReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);
    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to start discovery: %s\n",error.name);
        dbus_error_free(&error);
        return;
    }
    bt_shell_printf("Discovery started\n");
}

static void app_dbp_ClearDiscoveryFilter(DBusMessageIter *p_iter, void *p_userData)
{
    DBusMessageIter dict;

    dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY,
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                DBUS_TYPE_STRING_AS_STRING
                DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

    dbus_message_iter_close_container(p_iter, &dict);
}

static void app_dbp_DiscoveryFilterSetup(DBusMessageIter *p_iter, void *p_userData)
{
    APP_DBP_SetDiscoveryFilterArgs_T *p_args = p_userData;
    DBusMessageIter dict;

    dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY,
                DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                DBUS_TYPE_STRING_AS_STRING
                DBUS_TYPE_VARIANT_AS_STRING
                DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

    g_dbus_dict_append_array(&dict, "UUIDs", DBUS_TYPE_STRING,
                            &p_args->pp_uuids,
                            p_args->uuids_len);

    if (p_args->pathloss != DISTANCE_VAL_INVALID)
        g_dbus_dict_append_entry(&dict, "Pathloss", DBUS_TYPE_UINT16,
                        &p_args->pathloss);

    if (p_args->rssi != DISTANCE_VAL_INVALID)
        g_dbus_dict_append_entry(&dict, "RSSI", DBUS_TYPE_INT16,
                        &p_args->rssi);

    if (p_args->p_transport != NULL)
        g_dbus_dict_append_entry(&dict, "Transport", DBUS_TYPE_STRING,
                        &p_args->p_transport);
    if (p_args->duplicate)
        g_dbus_dict_append_entry(&dict, "DuplicateData",
                        DBUS_TYPE_BOOLEAN,
                        &p_args->duplicate);

    if (p_args->p_pattern != NULL)
        g_dbus_dict_append_entry(&dict, "Pattern", DBUS_TYPE_STRING,
                        &p_args->p_pattern);


    dbus_message_iter_close_container(p_iter, &dict);
}

static void app_dbp_SetDiscoveryFilterDestroy(void *p_userData)
{
    APP_DBP_SetDiscoveryFilterArgs_T *p_scanFilter = p_userData;

    if (p_scanFilter->p_pattern)
        g_free(p_scanFilter->p_pattern);
    if (p_scanFilter->p_transport)
        g_free(p_scanFilter->p_transport);
    if (p_scanFilter->pp_uuids)
        g_strfreev(p_scanFilter->pp_uuids);
    g_free(p_userData);
}

static void app_dbp_SetDiscoveryFilterReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    GDBusProxy * p_proxy = s_dbpCtrl.p_controller;

    dbus_error_init(&error);
    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("SetDiscoveryFilter failed: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }

    g_dbus_proxy_method_call(p_proxy, "StartDiscovery", NULL,
                    app_dbp_StartDiscoveryReply, NULL, NULL);
}

uint16_t APP_DBP_StartScan(GDBusProxy * p_ctrl)
{
    char *transportFilter = "le";
    APP_SCAN_Filter_T *p_scanFilter;
    APP_DBP_SetDiscoveryFilterArgs_T *p_scanFilterArgs;
    GDBusSetupFunction func;

    p_scanFilter = APP_SCAN_GetFilter();
    p_scanFilterArgs = g_malloc(sizeof(APP_DBP_SetDiscoveryFilterArgs_T));
    memset(p_scanFilterArgs, 0, sizeof(APP_DBP_SetDiscoveryFilterArgs_T));

    func = p_scanFilter->isConfigured ? app_dbp_DiscoveryFilterSetup : app_dbp_ClearDiscoveryFilter;

    p_scanFilterArgs->rssi = (uint16_t)p_scanFilter->rssi;
    p_scanFilterArgs->pathloss = DISTANCE_VAL_INVALID;
    p_scanFilterArgs->p_transport = g_strdup(transportFilter);

    if (p_scanFilter->pp_uuids){
        p_scanFilterArgs->pp_uuids = g_strdupv(p_scanFilter->pp_uuids);
        p_scanFilterArgs->uuids_len = g_strv_length(p_scanFilter->pp_uuids);
    }

    p_scanFilterArgs->duplicate = TRUE;
    if (p_scanFilter->p_pattern){
        p_scanFilterArgs->p_pattern = g_strdup(p_scanFilter->p_pattern);
    }else{
        p_scanFilterArgs->p_pattern = NULL;
    }

    if (g_dbus_proxy_method_call(p_ctrl, "SetDiscoveryFilter",
                func, app_dbp_SetDiscoveryFilterReply,
                p_scanFilterArgs, app_dbp_SetDiscoveryFilterDestroy) == FALSE){
        return APP_RES_FAIL;
    }

    return APP_RES_SUCCESS;
}

static void  app_dbp_StopDiscoveryReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);
    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to stop discovery: %s\n",error.name);
        dbus_error_free(&error);
        return;
    }
}

bool APP_DBP_StopScan(GDBusProxy * p_ctrl)
{
    if (g_dbus_proxy_method_call(p_ctrl, "StopDiscovery",
                NULL, app_dbp_StopDiscoveryReply,
                NULL, NULL) == FALSE)
    {
        return APP_RES_FAIL;
    }
    return APP_RES_SUCCESS;
}


#define PHY_SUPPORT (0x1E00)  //1M + 2M
static void app_dbp_PowerRely(const DBusError *p_error, void *p_userData)
{
    
    if (p_error!= NULL && dbus_error_is_set(p_error)) {
        bt_shell_printf("Failed to power on adapter: %s\n",p_error->name);
        return;
    }

    //traverse cached device and check if the service were already resolved.
    app_dbp_CheckServiceInCachedDevice();
    APP_MGMT_SetPhySupport(PHY_SUPPORT);
}

static bool app_dbp_PowerController(GDBusProxy * p_ctrl)
{
    dbus_bool_t powered = true;
    if (g_dbus_proxy_set_property_basic(s_dbpCtrl.p_controller, "Powered",
                    DBUS_TYPE_BOOLEAN, &powered,
                    app_dbp_PowerRely, "power",  NULL) == TRUE)
        return true;
    else
        return false;
}

static void app_dbp_DeviceConnectReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);

    if (!p_userData)
        return;
    
    APP_DBP_BtDev_T * p_dev = (APP_DBP_BtDev_T *)p_userData;
    if (p_dev)
        p_dev->isConnInitiadted = true;


    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to connect[%s]: %s\n", p_dev->p_address, error.name);
        dbus_error_free(&error);

        app_dbp_SendConnStatMsg(p_dev, false);

        return;
    }
}


static void app_dbp_DeviceDisconnectReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);

    APP_DBP_BtDev_T * p_dev = (APP_DBP_BtDev_T *)p_userData;
    if (p_dev)
    {
        p_dev->isConnInitiadted = false;
        p_dev->isCached = true;
        p_dev->role = -1;
    }


    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to disconnect: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }
}

static void app_dbp_DeviceRemoveReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);

    char * p_path = (char *)p_userData;

    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to remove[%s]: %s\n", p_path, error.name);
        dbus_error_free(&error);
        return;
    }

    /*device structure will be removed in APP_DBP_ProxyRemoved()*/

}


bool APP_DBP_DeviceIsPaired(APP_DBP_BtDev_T * p_dev)
{
    DBusMessageIter iter;
    if (g_dbus_proxy_get_property(p_dev->p_devProxy, "Paired", &iter)) {
            dbus_bool_t paired;
            dbus_message_iter_get_basic(&iter, &paired);
            return paired;
    }
    return false;
}

/*
bool APP_DBP_DeviceIsContected(APP_DBP_BtDev_T * dev)
{
    DBusMessageIter iter;
    if (g_dbus_proxy_get_property(dev->p_devProxy, "Connected", &iter)) {
        dbus_bool_t connected;
        dbus_message_iter_get_basic(&iter, &connected);
        return connected;
    }
    return false;
}
*/

bool APP_DBP_ConnectDevice(APP_DBP_BtDev_T * p_dev)
{
    if (!p_dev || !p_dev->p_devProxy)
        return false;

    if (p_dev->isConnected)
        return false;

    if (g_dbus_proxy_method_call(p_dev->p_devProxy, "Connect", NULL, app_dbp_DeviceConnectReply,
                            p_dev, NULL) == FALSE) {
        return false;
    }

    p_dev->isConnInitiadted = true;

    return true;
}

bool APP_DBP_DisconnectDevice(APP_DBP_BtDev_T * p_dev)
{
    if (!p_dev || !p_dev->p_devProxy)
        return false;

    //device is not connected and not in connecting
    if (!p_dev->isConnected && !p_dev->isConnInitiadted)
        return false;

    if (g_dbus_proxy_method_call(p_dev->p_devProxy, "Disconnect", NULL, app_dbp_DeviceDisconnectReply, 
                            p_dev, NULL) == FALSE) 
    {
        printf("Failed to disconnect\n");
        return false;
    }

    return true;
}

static void app_dbp_RemoveDeviceSetup(DBusMessageIter *p_iter, void *p_userData)
{
    char *p_path = (char *)p_userData;

    dbus_message_iter_append_basic(p_iter, DBUS_TYPE_OBJECT_PATH, &p_path);
}


bool APP_DBP_RemoveDevice(APP_DBP_BtDev_T * p_dev)
{
    char *p_path;

    if (!p_dev || !p_dev->p_devProxy)
        return false;

    p_path = g_strdup(g_dbus_proxy_get_path(p_dev->p_devProxy));

    if (g_dbus_proxy_method_call(s_dbpCtrl.p_controller, "RemoveDevice", 
                app_dbp_RemoveDeviceSetup, app_dbp_DeviceRemoveReply, 
                p_path, g_free) == FALSE) 
    {
        g_free(p_path);
        bt_shell_printf("Failed to remove device\n");
        return false;
    }

    return true;
}

void APP_DBP_RemoveDeviceList(bool includeConnectedDevices)
{
    GList *p_l;
    GList *nl = NULL;
    APP_DBP_BtDev_T * p_btDev = NULL;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        p_btDev = (APP_DBP_BtDev_T *)p_l->data;
        
        if (includeConnectedDevices == false && 
            (p_btDev->isConnected == true || p_btDev->isCached == true))
        {
            nl = g_list_append(nl, p_btDev);
        }
        else
        {
            app_dbp_FreeBtDev(p_btDev);
        }
    }

    g_list_free(s_dbpCtrl.p_deviceList);
    s_dbpCtrl.p_deviceList = NULL;

    if (nl != NULL)
    {
        s_dbpCtrl.p_deviceList = nl;
    }
}

static APP_DBP_BtDev_T * app_dbp_GetDeviceInfoByProxy(DeviceProxy * p_proxy)
{
    GList *p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->p_devProxy == p_proxy)
        {
            return p_dev;
        }
    }

    return NULL;
}

static void app_dbp_ConnStatChanged(DeviceProxy * p_proxy)
{
    DBusMessageIter iter;
    dbus_bool_t connected;
    APP_DBP_BtDev_T * p_dev;

    if (g_dbus_proxy_get_property(p_proxy, "Connected", &iter))
    {
        dbus_message_iter_get_basic(&iter, &connected);
    }

    p_dev = app_dbp_GetDeviceInfoByProxy(p_proxy);
    if (p_dev == NULL)
        return;

    
    if (p_dev->isConnInitiadted) //Central
    {
        if (connected == true)
        {
            p_dev->role = BLE_GAP_ROLE_CENTRAL;
            BLE_TRSPC_DevConnected(p_proxy);
        }
        else
        {
            p_dev->role = -1;
            BLE_TRSPC_DevDisconnected(p_proxy);
        }

    }
    else //Peripheral
    {
        if (connected == true)
        {
            p_dev->role = BLE_GAP_ROLE_PERIPHERAL;
            BLE_TRSPS_DevConnected(p_proxy);
        }
        else
        {
            p_dev->role = -1;
            BLE_TRSPS_DevDisconnected(p_proxy);
        }
    }


    p_dev->isConnected = connected;
    p_dev->rssi = 0;

    app_dbp_SendConnStatMsg(p_dev, true);
    
}

static void app_dbp_SendConnStatMsg(APP_DBP_BtDev_T * p_dev, bool isSuccess)
{
    APP_ConnectStateChanged_T connStatChanged;
    connStatChanged.p_proxy = p_dev->p_devProxy;
    connStatChanged.p_address = strdup(p_dev->p_address);
    connStatChanged.p_addressType = p_dev->p_addressType == NULL ? NULL : strdup(p_dev->p_addressType);
    connStatChanged.connState = p_dev->isConnected;
    connStatChanged.status = isSuccess;
    connStatChanged.role = p_dev->isConnInitiadted == true ? BLE_GAP_ROLE_CENTRAL : BLE_GAP_ROLE_PERIPHERAL;
    
    APP_PropertyChangedHandler(APP_CHANGED_TYPE_CONN_STAT, (void*)&connStatChanged);
    
    free(connStatChanged.p_address);
    free(connStatChanged.p_addressType);
}

static void app_dbp_SrvResolved(DeviceProxy * p_proxy, bool reportWhenResolved)
{
    DBusMessageIter iter;
    gboolean resolved;

    
    if (g_dbus_proxy_get_property(p_proxy, "ServicesResolved", &iter)) {
        dbus_message_iter_get_basic(&iter, &resolved);
    }

    if (reportWhenResolved && resolved)
    {
        APP_PropertyChangedHandler(APP_CHANGED_TYPE_SVC_RESOLVED, app_dbp_GetDeviceInfoByProxy(p_proxy));
    }
}

static void app_dbp_MtuUpdated(APP_DBP_BtDev_T * p_dev)
{
    APP_PropertyChangedHandler(APP_CHANGED_TYPE_MTU_UPDATED, p_dev);
}

static void app_dbp_GenericCb(const DBusError *error, void *p_userData)
{
    char *p_str = p_userData;

    if (dbus_error_is_set(error)) {
        bt_shell_printf("Failed to set %s: %s\n", p_str, error->name);
    }
}

void app_dbp_UpdateAdapterAlias(void)
{
    char adapterName[BLE_GAP_ADV_MAX_LENGTH];
    char *p_name;
    uint8_t len;
    
    if(APP_MGMT_GetLocalName(&len, (uint8_t*)&adapterName) == APP_RES_SUCCESS)
    {
        p_name = g_strdup(&adapterName[0]);
        if (g_dbus_proxy_set_property_basic(s_dbpCtrl.p_controller, "Alias", 
                DBUS_TYPE_STRING, &p_name, 
                app_dbp_GenericCb, p_name, g_free) == TRUE)
            return;
    
        g_free(p_name);
    }
}

static APP_DBP_BtDev_T * app_bdp_GetDeviceInfoByCharProxy(GattCharacteristic * p_charProxy)
{
    GList *p_l;
    const char * p_charProxyPath;
    APP_DBP_BtDev_T *p_dev = NULL;

    p_charProxyPath = g_dbus_proxy_get_path(p_charProxy);
    if (p_charProxyPath == NULL)
        return NULL;


    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        p_dev = p_l->data;
        if (g_str_has_prefix(p_charProxyPath, g_dbus_proxy_get_path(p_dev->p_devProxy)) == TRUE)
        {
            return p_dev;
        }
    }

    return NULL;
}

static int app_dbp_GetCharacteristicValue(DBusMessageIter *value, uint8_t *buf)
{
    DBusMessageIter array;
    uint8_t *data;
    int len;

    if (dbus_message_iter_get_arg_type(value) != DBUS_TYPE_ARRAY)
        return 0;

    dbus_message_iter_recurse(value, &array);

    if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_BYTE)
        return 0;

    dbus_message_iter_get_fixed_array(&array, &data, &len);
    memcpy(buf, data, len);

    return len;
}


#define UUID_SERVER_SUPPORTED_FEATURES "00002b3a-0000-1000-8000-00805f9b34fb"
static bool app_bdp_ParseSSF(GattCharacteristic * p_proxy)
{
    DBusMessageIter iter;
    APP_DBP_BtDev_T *p_dev = NULL;
    char *p_uuid;


    p_dev = app_bdp_GetDeviceInfoByCharProxy(p_proxy);
    if (p_dev == NULL || p_dev->ssf != 0 || p_dev->p_charCheckSsf != NULL)
    {
        return false;
    }

    if (g_dbus_proxy_get_property(p_proxy, "UUID", &iter) == TRUE)
    {
        dbus_message_iter_get_basic(&iter, &p_uuid);
        if (!strcmp(p_uuid, UUID_SERVER_SUPPORTED_FEATURES))
        {
            p_dev->p_charCheckSsf = p_proxy;
            return true;
        }
    }
    return false;
}

static void app_dbp_GetSSF(GattCharacteristic * p_proxy)
{
    DBusMessageIter iter;
    APP_DBP_BtDev_T *p_dev = NULL;
    uint8_t buf[16];
    int len;

    p_dev = app_bdp_GetDeviceInfoByCharProxy(p_proxy);
    if (p_dev == NULL)
        return;
    
    if (p_dev->p_charCheckSsf == p_proxy &&
        g_dbus_proxy_get_property(p_dev->p_charCheckSsf, "Value", &iter) == TRUE){
        len = app_dbp_GetCharacteristicValue(&iter, buf);
        if (len)
        {
            p_dev->ssf = buf[0];
        }
    }
}

static void app_bdp_ParseCharMtu(GattCharacteristic * p_proxy)
{
    DBusMessageIter iter;
    uint16_t mtu = 0;
    APP_DBP_BtDev_T *p_dev = NULL;


    p_dev = app_bdp_GetDeviceInfoByCharProxy(p_proxy);
    if (p_dev == NULL || p_dev->attMtu != 0 || p_dev->p_charCheckMtu != NULL)
    {
        return;
    }

    if (g_dbus_proxy_get_property(p_proxy, "MTU", &iter) == TRUE)
    {
        p_dev->p_charCheckMtu = NULL;
        dbus_message_iter_get_basic(&iter, &mtu);
    }
    else
    {
        //record the char proxy for triggering MTU property update later.
        p_dev->p_charCheckMtu = p_proxy;
    }

    if (p_dev->attMtu == 0 && mtu != 0)
    {
        p_dev->attMtu = mtu;
    }
}

APP_DBP_BtDev_T * APP_DBP_GetDevInfoByProxy(DeviceProxy *p_proxy)
{
    GList *p_l;

    if (p_proxy == NULL)
        return NULL;
    
    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->p_devProxy == p_proxy)
        {
            return p_dev;
        }
    }

    return NULL;
}

APP_DBP_BtDev_T * APP_DBP_GetDevInfoByIndex(int idx)
{
    GList *p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))  {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->index == idx)
        {
            return p_dev;
        }
    }

    return NULL;
}


uint16_t APP_DBP_GetAdapterAddr(BLE_GAP_Addr_T *p_addr)
{
    DBusMessageIter iter;
    const char *p_str;

    if (g_dbus_proxy_get_property(s_dbpCtrl.p_controller, "Address", &iter) == FALSE)
        return APP_RES_FAIL;

    dbus_message_iter_get_basic(&iter, &p_str);
    APP_Str2BtAddrBytes(p_str,p_addr->addr);
    
    if (g_dbus_proxy_get_property(s_dbpCtrl.p_controller, "AddressType", &iter) == FALSE)
        return APP_RES_FAIL;
    dbus_message_iter_get_basic(&iter, &p_str);
    
    if (!strcmp(p_str, "public"))
    {
        p_addr->addrType = BLE_GAP_ADDR_TYPE_PUBLIC;
    }
    else
    {
        p_addr->addrType = APP_DetermineRandomAddrType(p_addr->addr);
    }

    return APP_RES_SUCCESS;
}


void APP_DBP_RemoveByIndex(int idx)
{
    GList *p_l;

    for (p_l=s_dbpCtrl.p_deviceList; p_l; p_l=g_list_next(p_l))
    {
        APP_DBP_BtDev_T * p_dev = p_l->data;
        if (p_dev->index == idx)
        {
            if(APP_DBP_RemoveDevice(p_dev))
            {
                bt_shell_printf("remove device[%s]\n", p_dev->p_address);
            }
            else
            {
                bt_shell_printf("remove device[%s] failed\n", p_dev->p_address);
            }
            break;
        }
    }
}

static void app_dbp_DevicePairReply(DBusMessage *p_message, void *p_userData)
{
    DBusError error;
    dbus_error_init(&error);

    if (!p_userData)
        return;
    
    APP_DBP_BtDev_T * p_dev = (APP_DBP_BtDev_T *)p_userData;
    if (p_dev == NULL)
        return;


    if (dbus_set_error_from_message(&error, p_message) == TRUE) {
        bt_shell_printf("Failed to pair[%s]: %s\n", p_dev->p_address, error.name);
        dbus_error_free(&error);

        return;
    }
}

bool APP_DBP_Pair(APP_DBP_BtDev_T * p_dev)
{
    if (!p_dev || !p_dev->p_devProxy)
        return false;

    if (g_dbus_proxy_method_call(p_dev->p_devProxy, "Pair", NULL, app_dbp_DevicePairReply,
                            p_dev, NULL) == FALSE) {
        return false;
    }

    return true;
}

void APP_DBP_ProxyAdded(GDBusProxy *p_proxy, void *p_userData)
{
    const char * p_interface;
    bool isConnected = false;

    BLE_TRSPC_ProxyAddHandler(p_proxy);
        
    p_interface = g_dbus_proxy_get_interface(p_proxy);

    //printf("add proxy: %s\n", p_interface);
    if (!strcmp(p_interface, "org.bluez.Device1")) {
        if (APP_SM_GetSmState()==APP_SM_STATE_SCANNING || 
            APP_SM_GetSmState()==APP_SM_STATE_ADVERTISING) {
            isConnected = app_dbp_AddScanResult(p_proxy, false, NULL);
        }
        else {
            isConnected = app_dbp_AddScanResult(p_proxy, true, NULL);
        }

        if (isConnected)
        {
            app_dbp_ConnStatChanged(p_proxy);
            app_dbp_SortingDeviceList();
        }
        
    } else if (!strcmp(p_interface, "org.bluez.GattManager1") ) {
    
        uint16_t status = false;
        DBusConnection *p_conn = bt_shell_get_env("DBUS_CONNECTION");

        BLE_TRSPC_Init();
        status = BLE_TRSPS_Init(p_conn, p_proxy);
        if (status != TRSP_RES_SUCCESS)
            bt_shell_printf("BLE_TRSPS_Init error(%04x)\n", status);

    } else if (!strcmp(p_interface, "org.bluez.GattCharacteristic1")) {
        app_bdp_ParseCharMtu(p_proxy);
        app_bdp_ParseSSF(p_proxy);
    } else if (!strcmp(p_interface, "org.bluez.Adapter1")) {
        s_dbpCtrl.p_controller = p_proxy;
        app_dbp_PowerController(p_proxy);
        APP_UpdateLocalName(0, NULL);
        //change alias name to update device name in GAP sevice
        app_dbp_UpdateAdapterAlias();
        
    } else if (!strcmp(p_interface, "org.bluez.AgentManager1")) {
        s_dbpCtrl.p_pairAgent = p_proxy;
        APP_AGT_Register(p_proxy, DEFAULT_IO_CAPABILITY);
    }
}

void APP_DBP_ProxyRemoved(GDBusProxy *p_proxy, void *p_userData)
{
    const char * p_interface;

    BLE_TRSPC_ProxyRemoveHandler(p_proxy);
    
    p_interface = g_dbus_proxy_get_interface(p_proxy);
    //printf("remove proxy: %s\n", p_interface);
    if (!strcmp(p_interface, "org.bluez.Device1")) {
        app_dbp_RemoveDevice(p_proxy);
    }
}

void APP_DBP_PropertyChanged(GDBusProxy *p_proxy, const char *p_name, DBusMessageIter *p_iter, void *p_userData)
{
    const char *p_interface;

    BLE_TRSPC_PropertyHandler(p_proxy, p_name, p_iter);

    p_interface = g_dbus_proxy_get_interface(p_proxy);
    //printf("PROP changed:%s, name=%s\n", p_interface, p_name);
    
    if (!strcmp(p_interface, "org.bluez.Adapter1")) 
    {
        if (!strcmp(p_name, "Powered")) {
            dbus_bool_t powered;
            dbus_message_iter_get_basic(p_iter, &powered);
            if (powered && s_dbpCtrl.p_controller){
                APP_SM_Handler(APP_SM_EVENT_POWER_ON);
            }
        } else if (!strcmp(p_name, "Discovering") )  {
            dbus_bool_t discov;
            dbus_message_iter_get_basic(p_iter, &discov);
            if (s_dbpCtrl.p_controller)
            {
                if (discov){
                    APP_SM_Handler(APP_SM_EVENT_SCANNING_ON);
                }
                else if(!discov){
                    APP_SM_Handler(APP_SM_EVENT_SCANNING_OFF);
                }
            }
        }
    } 
    else if (!strcmp(p_interface, "org.bluez.Device1")) 
    {
        app_dbp_AddScanResult(p_proxy, false, p_name);
        
        if (!strcmp(p_name, "Connected") ) {
            app_dbp_ConnStatChanged(p_proxy);
        } 
        else if (!strcmp(p_name, "ServicesResolved") ) {

            gboolean resolved;
            dbus_message_iter_get_basic(p_iter, &resolved);

            if (resolved)
            {
                app_dbp_SrvResolved(p_proxy, true);

                /* If MTU was not retrieved from characteristic proxy added, it could be due to device record is in cached.
                   We trigger a property refresh here when device become connected, to force MTU property updated. */
                APP_DBP_BtDev_T * p_dev;
                p_dev = APP_DBP_GetDevInfoByProxy(p_proxy);
                if (p_dev && p_dev->attMtu == 0)
                {
                    g_dbus_proxy_refresh_property(p_dev->p_charCheckMtu, "MTU");
                    p_dev->p_charCheckMtu = NULL;
                }

                if (p_dev->p_charCheckSsf)
                {
                    g_dbus_proxy_refresh_property(p_dev->p_charCheckSsf, "Value");
                }
            }
        }
        else if (!strcmp(p_name, "Paired") ) {
            gboolean paired;
            dbus_message_iter_get_basic(p_iter, &paired);
            if(paired)
                bt_shell_printf("device paired\n");
            else
                bt_shell_printf("device unpaired\n");
        }
    }
    else if (!strcmp(p_name, "MTU")) 
    {
        APP_DBP_BtDev_T * p_dev = NULL;

        app_bdp_ParseCharMtu(p_proxy);
        p_dev = app_bdp_GetDeviceInfoByCharProxy(p_proxy);
        app_dbp_MtuUpdated(p_dev);
    }
    else if (!strcmp(p_interface, "org.bluez.GattCharacteristic1")) {
        app_dbp_GetSSF(p_proxy);
    }
}

void APP_DBP_DBusConnectHandler(DBusConnection *p_connection, void *p_userData)
{
    bt_shell_set_prompt(PROMPT_ON);
}

void APP_DBP_DBusDisconnectHandler(DBusConnection *p_connection, void *p_userData)
{
    bt_shell_detach();
}

void APP_DBP_DBusMessageHandler(DBusConnection *p_connection, DBusMessage *p_message, void *p_userData)
{
    LOG("[SIGNAL] %s.%s\n", dbus_message_get_interface(p_message), dbus_message_get_member(p_message));
}

void APP_DBP_ClientReady(GDBusClient *p_client, void *p_userData)
{
    bt_shell_attach(fileno(stdin));
}


