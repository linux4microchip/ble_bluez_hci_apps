/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application DBus Proxy Handler Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dbp.h

  Summary:
    This file contains the Application DBus Proxy Handler function for this project.

  Description:
    This file contains the Application DBus Proxy Handler function for this project.
 *******************************************************************************/

#ifndef APP_DBP_H
#define APP_DBP_H

#include <stdint.h>
#include "app_gap.h"
#include "gdbus/gdbus.h"



typedef GDBusProxy GattCharacteristic;
typedef GDBusProxy DeviceProxy;


typedef  struct APP_DBP_BtDev_T
{
    char * p_address;
    char * p_addressType;
    char * p_name;
    bool srvDataUuidMatch;
    bool manufDataMatch;
    bool isCached;
    bool isConnected;
    bool isConnInitiadted;
    bool isValid;
    int8_t role;
    int8_t index;
    int16_t rssi;
    int16_t txPower;
    int16_t attMtu;
    int8_t ssf; /*Server Supported Features*/
    DeviceProxy * p_devProxy;
    GattCharacteristic * p_charCheckMtu;
    GattCharacteristic * p_charCheckSsf; /*for Server Supported Features check*/
} APP_DBP_BtDev_T;



void APP_DBP_Init();
bool APP_DBP_DeviceIsPaired(APP_DBP_BtDev_T * p_dev);
bool APP_DBP_ConnectDevice(APP_DBP_BtDev_T * p_dev);
bool APP_DBP_DisconnectDevice(APP_DBP_BtDev_T * p_dev);
void APP_DBP_DisconnectAll(void);
uint16_t APP_DBP_StartScan(GDBusProxy * p_ctrl);
bool APP_DBP_StopScan(GDBusProxy * p_ctrl);
void APP_DBP_PrintDeviceList (void);
GDBusProxy * APP_DBP_GetDefaultAdapter(void);
GDBusProxy * APP_DBP_GetPairAgent(void);
void APP_DBP_ConnectByIndex(int idx);
void APP_DBP_DisconnectByIndex(int idx);
void APP_DBP_RemoveByIndex(int idx);
void APP_DBP_RemoveDeviceList(bool includeConnectedDevices);
APP_DBP_BtDev_T * APP_DBP_GetDevInfoByProxy(DeviceProxy *p_proxy);
APP_DBP_BtDev_T * APP_DBP_GetDevInfoByIndex(int idx);
uint16_t APP_DBP_GetAdapterAddr(BLE_GAP_Addr_T *p_addr);
void APP_DBP_ProxyAdded(GDBusProxy *p_proxy, void *p_userData);
void APP_DBP_ProxyRemoved(GDBusProxy *p_proxy, void *p_userData);
void APP_DBP_PropertyChanged(GDBusProxy *p_proxy, const char *p_name, DBusMessageIter *p_iter, void *p_userData);
void APP_DBP_DBusConnectHandler(DBusConnection *p_connection, void *p_userData);
void APP_DBP_DBusDisconnectHandler(DBusConnection *p_connection, void *p_userData);
void APP_DBP_DBusMessageHandler(DBusConnection *p_connection, DBusMessage *p_message, void *p_userData);
void APP_DBP_ClientReady(GDBusClient *p_client, void *p_userData);
bool APP_DBP_Pair(APP_DBP_BtDev_T * p_dev);


#endif //APP_DBP_H

