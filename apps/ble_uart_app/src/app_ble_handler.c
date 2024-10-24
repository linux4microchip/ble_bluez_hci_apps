/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application BLE Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble_handler.c

  Summary:
    This file contains the Application BLE functions for this project.

  Description:
    This file contains the Application BLE functions for this project.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/mgmt.h"
#include "shared/shell.h"

#include "application.h"
#include "app_ble_handler.h"
#include "app_utility.h"
#include "app_timer.h"
#include "app_sm.h"
#include "app_error_defs.h"
#include "app_scan.h"
#include "app_dbp.h"
#include "app_mgmt.h"
#include "app_trps.h"



// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define APP_BLE_NUM_ADDR_IN_DEV_NAME    2    /**< The number of bytes of device address included in the device name. */



/**@brief Definition for random address determination */
#define BLE_GAP_RANDOM_SUB_TYPE_MASK           0xC0     /**< Random Address Mask: Bit 7, Bit6. Bit 7 is the Most significant bit */
#define BLE_GAP_RESOLVABLE_ADDR                0x40     /**< (bit7:bit6) of BLE address is 01 then it is resolvable private address */
#define BLE_GAP_NON_RESOLVABLE_ADDR            0x00     /**< (bit7:bit6) of BLE address is 00 then it is non-resolvable private address */
#define BLE_GAP_STATIC_ADDR                    0xC0     /**< (bit7:bit6) of BLE address is 11 then it is static private address */

#define BLE_GAP_IS_STATIC_ADDR(x)              ((x[0] & BLE_GAP_RANDOM_SUB_TYPE_MASK) ==BLE_GAP_STATIC_ADDR) /**< Returns non-zero value if x is a resolvable address */
#define BLE_GAP_IS_RESOLVABLE_ADDR(x)          ((x[0] & BLE_GAP_RANDOM_SUB_TYPE_MASK) ==BLE_GAP_RESOLVABLE_ADDR) /**< Returns non-zero value if x is a resolvable address */
#define BLE_GAP_IS_NON_RESOLVABLE_ADDR(x)      ((x[0] & BLE_GAP_RANDOM_SUB_TYPE_MASK) ==BLE_GAP_NON_RESOLVABLE_ADDR) /**< Returns non-zero value if x is a resolvable address */

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Global Variabless
// *****************************************************************************
// *****************************************************************************
uint8_t                         g_bleConnLinkNum;

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_BLE_ConnList_T       s_bleConnList[APP_BLE_MAX_LINK_NUMBER];
static APP_BLE_ConnList_T       *sp_currentBleLink = NULL; /**< This pointer means the last one connected BLE link. */


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static void app_ClearConnListByDevProxy(DeviceProxy *p_devProxy)
{
    uint8_t i;

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].p_deviceProxy == p_devProxy)
        {
            memset((uint8_t *)(&s_bleConnList[i]), 0, sizeof(APP_BLE_ConnList_T));
            s_bleConnList[i].linkState = APP_BLE_STATE_STANDBY;
        }
    }
}

static APP_BLE_ConnList_T *app_GetScanConnList(void)
{
    uint8_t i;

    //First find the state of APP_BLE_STATE_CONNECTING
    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].linkState == APP_BLE_STATE_CONNECTING)
        {
            return (&s_bleConnList[i]);
        }
    }

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].linkState == APP_BLE_STATE_STANDBY)
        {
            return (&s_bleConnList[i]);
        }
    }
    
    return NULL;
}

uint8_t APP_GetConnLinkNum(void)
{
    uint8_t index, num = 0;

    for (index = 0; index < APP_BLE_MAX_LINK_NUMBER; index++)
    {
        if (s_bleConnList[index].linkState == APP_BLE_STATE_CONNECTED)
        {
            num+=1;
        }
    }

    return num;
}

APP_BLE_ConnList_T *APP_GetFreeConnList(void)
{
    uint8_t i;

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].linkState == APP_BLE_STATE_STANDBY)
        {
            return (&s_bleConnList[i]);
        }
    }
    return NULL;
}

APP_BLE_ConnList_T *APP_GetConnInfoByDevProxy(DeviceProxy *p_devProxy)
{
    uint8_t i;

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].p_deviceProxy == p_devProxy)
        {
            return (&s_bleConnList[i]);
        }
    }
    return NULL;
}

uint8_t APP_GetRoleNumber(uint8_t role) 
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        if (s_bleConnList[i].connData.role == role && s_bleConnList[i].linkState == APP_BLE_STATE_CONNECTED)
        {
            count++;
        }
    }
    return count;
}


void APP_PropertyChangedHandler(APP_PtyChanged_T ptyChangedType, void *p_param)
{
    APP_ConnectStateChanged_T *p_connStaChanged = NULL;
    APP_BLE_ConnList_T *p_bleConn = NULL;
    bdaddr_t bdaddr;
    uint8_t addrType;

    switch(ptyChangedType)
    {
        case APP_CHANGED_TYPE_CONN_STAT:
        {
            if (p_param == NULL)
                return;

            p_connStaChanged = (APP_ConnectStateChanged_T *)p_param;

            APP_Str2BtAddr(p_connStaChanged->p_address, &bdaddr);


            if (p_connStaChanged->status == false)
            {                
                p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_CONNECTING, APP_BLE_STATE_CONNECTING);

                if (p_bleConn != NULL)
                {
                    memset(p_bleConn, 0, sizeof(APP_BLE_ConnList_T));
                    p_bleConn->linkState = APP_BLE_STATE_STANDBY;
                }
            }
            else //p_connStaChanged->status == true
            {
                if (p_connStaChanged->connState == true) //connected
                {
                    g_bleConnLinkNum++;

                    addrType = BLE_GAP_ADDR_TYPE_PUBLIC;
                    if (!memcmp(p_connStaChanged->p_addressType, "public", 6)){
                        addrType = BLE_GAP_ADDR_TYPE_PUBLIC;
                    } else if (!memcmp(p_connStaChanged->p_addressType, "random", 6)) {
                        addrType = APP_DetermineRandomAddrType(bdaddr.b);
                    }

                    APP_TRP_COMMON_ConnEvtProc(p_connStaChanged->p_proxy, p_connStaChanged->role);
                    
                    if (p_connStaChanged->role == BLE_GAP_ROLE_CENTRAL)
                    {
                        p_bleConn = app_GetScanConnList();
                    }
                    else
                    {
                        p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_ADVERTISING, APP_BLE_STATE_ADVERTISING);
                        if (p_bleConn == NULL) //for cached devices
                        {
                            p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_STANDBY, APP_BLE_STATE_STANDBY);
                        }
                    }
                    
                    if (p_bleConn)
                    {
                        /* Update the connection parameter */
                        p_bleConn->linkState                        = APP_BLE_STATE_CONNECTED;
                        p_bleConn->p_deviceProxy                    = p_connStaChanged->p_proxy;
                        p_bleConn->connData.role                    = p_connStaChanged->role;        // 0x00: Central, 0x01:Peripheral
                    
                        /* Save Remote Device Address */
                        p_bleConn->connData.remoteAddr.addrType = addrType;
                        memcpy((uint8_t *)p_bleConn->connData.remoteAddr.addr, (uint8_t *)bdaddr.b, GAP_MAX_BD_ADDRESS_LEN);
                    
                        sp_currentBleLink = p_bleConn;
                    }
                    
                    APP_SM_Handler(APP_SM_EVENT_CONNECTED);
                    APP_DeviceConnected(p_bleConn->p_deviceProxy);
                    bt_shell_printf("device [%s] connected\n", p_connStaChanged->p_address);
                    //printf("device [%s] connected, role=%d\n", p_connStaChanged->p_address, p_connStaChanged->role);
                }//p_connStaChanged->connState == true
                else //disconnected
                {
                    APP_DeviceDisconnected(p_connStaChanged->p_proxy);
                    
                    //Clear connection list
                    app_ClearConnListByDevProxy(p_connStaChanged->p_proxy);
                    
                    if (g_bleConnLinkNum > 0)
                        g_bleConnLinkNum--; //Don't move it because the two functions below need to reference.

                    APP_TRP_COMMON_DiscEvtProc(p_connStaChanged->p_proxy);

                    APP_SM_Handler(APP_SM_EVENT_DISCONNECTED);
                    
                    bt_shell_printf("device [%s] disconnected\n", p_connStaChanged->p_address);

                }
            }
        }
        break;
        case APP_CHANGED_TYPE_SVC_RESOLVED:
        {
            if (p_param == NULL)
                break;
            
            APP_DBP_BtDev_T *p_dev = (APP_DBP_BtDev_T *)p_param;
            APP_TRP_COMMON_UpdateMtu(p_dev->p_devProxy, p_dev->attMtu);
        }
        break;
        case APP_CHANGED_TYPE_MTU_UPDATED:
        {
            if (p_param == NULL)
                break;
            
            APP_DBP_BtDev_T *p_dev = (APP_DBP_BtDev_T *)p_param;
            APP_TRP_COMMON_UpdateMtu(p_dev->p_devProxy, p_dev->attMtu);
        }
        break;
        default:
        break;
    }
}


void APP_UpdateLocalName(uint8_t devNameLen, uint8_t *p_devName)
{
    uint8_t localName[GAP_MAX_DEVICE_NAME_LEN] = {0};
    uint8_t localNameLen = 0;

    if (p_devName == NULL || devNameLen == 0)
    {
        BLE_GAP_Addr_T addrPara;
        uint8_t digitNum = APP_BLE_NUM_ADDR_IN_DEV_NAME * 2;

        localName[localNameLen++] = 'B';
        localName[localNameLen++] = 'L';
        localName[localNameLen++] = 'U';
        localName[localNameLen++] = 'E';
        localName[localNameLen++] = 'Z';
        localName[localNameLen++] = '_';

        APP_DBP_GetAdapterAddr(&addrPara);

        sprintf((char *)&localName[localNameLen], "%02X%02X", addrPara.addr[4], addrPara.addr[5]);

        localNameLen += digitNum;

        APP_MGMT_SetLocalName(localNameLen, localName);
    }
    else
    {
        APP_MGMT_SetLocalName(devNameLen, p_devName);
    }
}

APP_BLE_LinkState_T APP_GetBleStateByLink(APP_BLE_ConnList_T *p_bleConn)
{
    if (p_bleConn == NULL)
        return (sp_currentBleLink->linkState);
    else
        return (p_bleConn->linkState);
}

void APP_SetBleStateByLink(APP_BLE_ConnList_T *p_bleConn, APP_BLE_LinkState_T state)
{
    if (p_bleConn == NULL)
        sp_currentBleLink->linkState = state;
    else
        p_bleConn->linkState = state;
}

APP_BLE_ConnList_T *APP_GetLastOneConnectedBleLink(void)
{
    return sp_currentBleLink;
}

APP_BLE_ConnList_T *APP_GetBleLinkByStates(APP_BLE_LinkState_T start, APP_BLE_LinkState_T end)
{
    uint8_t i;

    while (end >= start)
    {
        i = 0;
        
        for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
        {
            if (s_bleConnList[i].linkState == start)
                return (&s_bleConnList[i]);
        }
        
        start+=1;
    }
    
    return NULL;
}

int8_t APP_Str2BtAddr(const char *p_str, bdaddr_t *p_ba)
{
    bdaddr_t bdaddrTmp;

    if(str2ba(p_str, &bdaddrTmp)< 0)
    {
        return -1;
    }

    baswap(p_ba, &bdaddrTmp);

    return 0;
}

int8_t APP_Str2BtAddrBytes(const char *p_str, uint8_t *p_ba)
{
    bdaddr_t bdaddrTmp;
    bdaddr_t bdaddr;

    if(str2ba(p_str, &bdaddrTmp) < 0)
    {
        return -1;
    }
    
    baswap(&bdaddr, &bdaddrTmp);
    memcpy(p_ba, bdaddr.b, 6);

    return 0;
}

uint8_t APP_DetermineRandomAddrType(uint8_t * p_addr)
{
    if (BLE_GAP_IS_RESOLVABLE_ADDR(p_addr))
        return BLE_GAP_ADDR_TYPE_RANDOM_RESOLVABLE;
    else if (BLE_GAP_IS_NON_RESOLVABLE_ADDR(p_addr))
        return BLE_GAP_ADDR_TYPE_RANDOM_NON_RESOLVABLE;
    else
        return BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
}

void APP_InitConnList(void)
{
    uint8_t i;

    sp_currentBleLink = &s_bleConnList[0];
    g_bleConnLinkNum = 0;
    //s_connHandleIndex = LINK_HANDLE_INIT;

    for (i = 0; i < APP_BLE_MAX_LINK_NUMBER; i++)
    {
        memset((uint8_t *)(&s_bleConnList[i]), 0, sizeof(APP_BLE_ConnList_T));
        s_bleConnList[i].linkState = APP_BLE_STATE_STANDBY;
    }
}


