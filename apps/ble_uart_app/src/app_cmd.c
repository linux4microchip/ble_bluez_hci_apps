/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  application command Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.c

  Summary:
    This file contains application commands handler.

  Description:
    This file contains application commands handler.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "bluetooth/bluetooth.h"
#include "bluetooth/mgmt.h"
#include "bluetooth/uuid.h"

#include "application.h"
#include "app_cmd.h"
#include "app_sm.h"
#include "app_scan.h"
#include "app_dbp.h"
#include "app_mgmt.h"
#include "app_agent.h"
#include "app_ble_handler.h"
#include "app_error_defs.h"




// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************




// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

static APP_CMD_Menu_T s_mainMenu = {
    .name = "main",
    .entries = {
    { "scan",         "[...]",    APP_CMD_Scan, "Scan devices. usage: scan [r <rssi>][s <uuid>][p <pattern>][u <128bit uuid>][m <manufactor id><data>]" }, 
    { "dev",          NULL,       APP_CMD_ListDevices, "List available devices, inlude cached devices" }, 
    { "remove",       "<index>",  APP_CMD_RemoveDevice, "Remove devices, inlude cached devices" }, 
    { "conn",         "<index>",  APP_CMD_Connect, "Connect to selected device which listed in dev command" }, 
    { "cc",           "<index>",  APP_CMD_CancelConnect, "Cancel connect command" }, 
    { "disc",         "<index>",  APP_CMD_Disconnect, "Disconnect to selected device which listed in dev command" }, 
    { "adv",          "<0-1>",    APP_CMD_MgmtAdv, "Advertisement switch" }, 
    // TRP command
    { "raw",          "...",      APP_CMD_SendRawData, "Send raw data to remote peer manually. usage: raw <index> <text>" }, 
    { "txf",          "...",      APP_CMD_SendFileData, "Send file to remote peer. usage: txf <index> <file-path>" }, 
    { "rxf",          "...",      APP_CMD_ReceiveFileData, "Receive file from remote peer. usage: rxf <index> [file-path]" }, 
    { "sw",           "<1-3>",    APP_CMD_ModeSwitch, "Transmission mode switch (1=checksum, 2=loopback, 3=fixed-pattern)" }, 
    { "pt",           "<0-6>",    APP_CMD_PatternSelect, "Select data pattern for transmission(0=1K, 1=5K, 2=10K, 3=50K, 4=100K, 5=200L, 6=500K)" }, 
    { "b",            "<index>",  APP_CMD_BurstModeStart, "Start Burst Mode data transmission on selected device" }, 
    { "ba",           NULL,       APP_CMD_BurstModeStartAll, "Start Burst Mode data transmission on all connected devices" }, 
#ifdef ENABLE_AUTO_RUN
    { "r",            "<1-65536>",APP_CMD_SetExecRuns, "[Test used only] Execute Burst Mode data transmission for configured times" }, 
#endif
    //pairing command
    { "pair",         "<index>",  APP_CMD_PairDevice, "Pair with selected device" }, 
    { "unpair",       "<index>",  APP_CMD_UnpairDevice, "Unpair with selected device" }, 
    { "io",           "<0-4>",    APP_CMD_SetIoCap, "IO capability setting (0=DispalyOnly, 1=DisplayYesNo, 2=KeyboardOnly, 3=NoInputNoOutput, 4=KeyboardDisplay)" }, 
    { "sc",           "<0-1>",    APP_CMD_SetSecureConnection, "Secure connection setting(0=disable, 1=enable)" }, 
    { "pinfo",        NULL,       APP_CMD_ReadPairInfo, "Print current IO capability and secure connection setting" }, 
    { } },
};


static bool app_cmd_BeU8AsciiToHex(uint8_t *p_str, uint8_t *p_value)
{
    unsigned int hexValue;

    if (p_str == NULL || !isxdigit(p_str[0]) || !isxdigit(p_str[1]))
        return false;

    sscanf((const char *)&p_str[0], "%02x", &hexValue);
    *p_value = (uint8_t)hexValue;
    return true;
}

static bool app_cmd_BeU16AsciiToHex(uint8_t *p_str, uint16_t *p_value)
{
    unsigned int hexValue;

    if (p_str == NULL || !isxdigit(p_str[0]) || !isxdigit(p_str[1]) || !isxdigit(p_str[2]) || !isxdigit(p_str[3]))
        return false;

    sscanf((const char *)&p_str[0], "%04x", &hexValue);
    *p_value = (uint16_t)hexValue;
    return true;
}


static bool app_cmd_BeStringToHex(uint8_t *p_str, uint8_t *p_hex, uint8_t hexLen)
{
    uint8_t i;

    for(i = 0; i < hexLen; i++)
    {
        if (app_cmd_BeU8AsciiToHex(&p_str[2*i], (p_hex+i)) != true)
            return false;
    }

    return true;
}

static inline int app_cmd_is_uuid128(const char *p_string)
{
    return (strlen(p_string) == 36 &&
            p_string[8] == '-' &&
            p_string[13] == '-' &&
            p_string[18] == '-' &&
            p_string[23] == '-');
}

static char* app_cmd_strdupUuid(char *p_string)
{
    char * p_uuid128Str;
    p_uuid128Str = (char *)malloc(MAX_LEN_UUID_STR);

    if (p_uuid128Str == NULL)
        return NULL;

    if (strlen(p_string) >= (MAX_LEN_UUID_STR))
        return NULL;

    if (strlen(p_string)%2 != 0)
        return NULL;
    
    if (app_cmd_is_uuid128(p_string))
    {
        strcpy(p_uuid128Str, p_string);
    }
    else
    {
        strcpy(p_uuid128Str, APP_SCAN_UUID_128_BASE_STR);
        if(strlen(p_string) == 8) //uuid32
        {
            strncpy(&p_uuid128Str[0], p_string, 8);
        }
        else if(strlen(p_string) == 4) //uuid16
        {
            strncpy(&p_uuid128Str[4], p_string, 4);
        }
        else
        {
            return NULL;
        }
    }

    return p_uuid128Str;
}

static void app_cmd_toLowerCase(char *p_string)
{
    for (int i=0; p_string[i]; i++) {
        p_string[i] = tolower(p_string[i]);
    }
}


APP_CMD_Menu_T * APP_CMD_GetCmdMenu(void)
{
    return &s_mainMenu;
}

void APP_CMD_Scan(int argc, char *argv[])
{
    uint8_t i = 1;
    APP_SCAN_Filter_T scanFilter;

    memset(&scanFilter, 0, sizeof(APP_SCAN_Filter_T));
    scanFilter.rssi = APP_SCAN_DEFAULT_FILTER_RSSI;
    
    //command example: scan r -60 p RNBD s 1
    if (argc > 2)
    {
        //parse scan filter
        do {
            switch(argv[i][0])
            {
                case 'R':
                case 'r':
                    scanFilter.rssi = atoi(argv[i+1]);
                    if (scanFilter.rssi == 0){
                        scanFilter.rssi = APP_SCAN_DEFAULT_FILTER_RSSI;
                    }
                    break;
                case 'P':
                case 'p':
                    scanFilter.p_pattern = strdup(argv[i+1]);
                    break;
                case 'S':
                case 's':
                    scanFilter.p_srvUuid = app_cmd_strdupUuid(argv[i+1]);
                    if (scanFilter.p_srvUuid == NULL)
                    {
                        bt_shell_printf("invalid parameters\n");
                        goto done;
                    }
                    app_cmd_toLowerCase(scanFilter.p_srvUuid);
                    scanFilter.isFilterSrvUuid = true;
                    break;
                case 'U':
                case 'u':
                    scanFilter.pp_uuids = g_strdupv(&argv[i+1]);
                    break;
                case 'M':
                case 'm':
                    if (!app_cmd_BeU16AsciiToHex((uint8_t*)argv[i+1], &scanFilter.manufId))
                    {
                        bt_shell_printf("invalid parameters\n");
                        goto done;
                    }
                    scanFilter.manufDataLen = strlen(argv[i+2])/2;
                    scanFilter.p_manufData = malloc(scanFilter.manufDataLen);
                    if (!app_cmd_BeStringToHex((uint8_t*)argv[i+2], scanFilter.p_manufData, scanFilter.manufDataLen))
                    {
                        bt_shell_printf("invalid parameters\n");
                        goto done;
                    }
                    scanFilter.isFilterManufData = true;
                    i++;
                    break;
                default:
                    bt_shell_printf("invalid parameters\n");
                    goto done;
            }
            i += 2;
        } while(i< argc);

        APP_SCAN_ClearFilter();
        
        if (APP_SCAN_SetFilter(&scanFilter) == false)
            goto done;
    }
    else
    {
        APP_SCAN_ClearFilter();
    }

    APP_DBP_RemoveDeviceList(false);
    APP_SCAN_Start();

done:
    if (scanFilter.p_pattern)
        free(scanFilter.p_pattern);
    if (scanFilter.pp_uuids)
        g_strfreev(scanFilter.pp_uuids);
    if (scanFilter.p_manufData)
        free(scanFilter.p_manufData);
    if (scanFilter.p_srvUuid)
        free(scanFilter.p_srvUuid);

}

void APP_CMD_ListDevices(int argc, char *argv[])
{
    APP_DBP_PrintDeviceList();
}

void APP_CMD_RemoveDevice(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        APP_DBP_BtDev_T * p_dev;
            
        selectIdx = atoi(argv[1]);
        p_dev = APP_DBP_GetDevInfoByIndex(selectIdx);
        if (p_dev == NULL)
        {
            bt_shell_printf("device is not existed\n");
        }
        else if(p_dev->isConnected)
        {
            bt_shell_printf("device is connected, please disconnect first\n");
        }
        else
        {
            APP_DBP_RemoveByIndex(selectIdx);
        }
    }
    else
    {
        bt_shell_printf("please set device index\n");
    }
}


void APP_CMD_Connect(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        selectIdx = atoi(argv[1]);

        if(APP_GetRoleNumber(BLE_GAP_ROLE_PERIPHERAL))
        {
            bt_shell_printf("Warning: Reject multo-role topology\n");
            return;
        }
        
        APP_DBP_ConnectByIndex(selectIdx);
    }
    else
    {
        bt_shell_printf("please set device index\n");
    }
}

void APP_CMD_CancelConnect(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        selectIdx = atoi(argv[1]);
        APP_DBP_DisconnectByIndex(selectIdx);
    }
    else
    {
        bt_shell_printf("invalid parameters\n");
    }
}

void APP_CMD_Disconnect(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        selectIdx = atoi(argv[1]);
        APP_DBP_DisconnectByIndex(selectIdx);
    }
    else
    {
        bt_shell_printf("please set device index\n");
    }
}

void APP_CMD_MgmtAdv(int argc, char *argv[])
{
    uint8_t enable = 0;

    if (argc==2)
    {
        enable = atoi(argv[1]);
    }

    if(APP_GetRoleNumber(BLE_GAP_ROLE_CENTRAL))
    {
        bt_shell_printf("Warning: Reject multo-role topology\n");
        return;
    }


    if (enable == 0)
    {
        APP_SM_Handler(APP_SM_EVENT_ADV_OFF);
    }
    else if (enable == 1)
    {
        APP_SM_Handler(APP_SM_EVENT_ADV_ON);
    }
    else
    {
        bt_shell_printf("adv parameter error\n");
    }
}

void APP_CMD_SendRawData(int argc, char *argv[])
{
    uint8_t devIndex = 0xFF;
    APP_DBP_BtDev_T *p_dev;
    APP_TRP_ConnList_T *p_trpConn;
    
    if (argc == 3)
    {
        devIndex = atoi(argv[1]);
        
        p_dev = APP_DBP_GetDevInfoByIndex(devIndex);
        if (p_dev == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }
        
        p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_dev->p_devProxy);
        if (p_trpConn == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }

        APP_SendRawData(p_trpConn, argv[2]);
    }
}

void APP_CMD_SendFileData(int argc, char *argv[])
{
    uint8_t devIndex = 0xFF;
    APP_DBP_BtDev_T *p_dev;
    APP_TRP_ConnList_T *p_trpConn;
    
    if (argc == 3)
    {
        devIndex = atoi(argv[1]);
        
        p_dev = APP_DBP_GetDevInfoByIndex(devIndex);
        if (p_dev == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }
        
        p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_dev->p_devProxy);
        if (p_trpConn == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }

        APP_SendRawDataFromFile(p_trpConn, argv[2]);
    }
}

void APP_CMD_ReceiveFileData(int argc, char *argv[])
{
    uint8_t devIndex = 0xFF;
    APP_DBP_BtDev_T *p_dev;
    APP_TRP_ConnList_T *p_trpConn;
    
    if (argc == 2 || argc == 3)
    {
        devIndex = atoi(argv[1]);
        
        p_dev = APP_DBP_GetDevInfoByIndex(devIndex);
        if (p_dev == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }
        
        p_trpConn = APP_TRP_COMMON_GetConnListByDevProxy(p_dev->p_devProxy);
        if (p_trpConn == NULL)
        {
            bt_shell_printf("invalid parameter\n");
            return;
        }
    }

    if (argc == 2)
        APP_ReceiveRawDataToFile(p_trpConn, NULL);
    else if (argc == 3)
        APP_ReceiveRawDataToFile(p_trpConn, argv[2]);

}

void APP_CMD_PairDevice(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        bool isPaired = false;
        APP_DBP_BtDev_T * p_dev;
        
        selectIdx = atoi(argv[1]);
        p_dev = APP_DBP_GetDevInfoByIndex(selectIdx);
        if (p_dev)
        {
            isPaired = APP_DBP_DeviceIsPaired(p_dev);
            if (isPaired)
            {
                bt_shell_printf("device is already bonded\n");
                return;
            }
            else
            {
                bt_shell_printf("pair device [%s]\n", p_dev->p_address);
                APP_DBP_Pair(p_dev);
            }
        }
    }
    else
    {
        bt_shell_printf("please set device index\n");
    }
}

void APP_CMD_UnpairDevice(int argc, char *argv[])
{
    if (argc == 2)
    {
        uint8_t selectIdx;
        uint16_t result;
        bool isPaired = false;
        APP_DBP_BtDev_T * p_dev;
        
        selectIdx = atoi(argv[1]);
        p_dev = APP_DBP_GetDevInfoByIndex(selectIdx);
        if (p_dev)
        {
            isPaired = APP_DBP_DeviceIsPaired(p_dev);
            if (!isPaired)
            {
                bt_shell_printf("device is not bonded\n");
                return;
            }
            
            result = APP_MGMT_RemoveBonding(p_dev->p_address, p_dev->p_addressType);
            if (result == APP_RES_FAIL)
            {
                bt_shell_printf("remove bonding failed\n");
            }
            else
            {
                bt_shell_printf("remove bonding success\n");
            }
        }
    }
    else
    {
        bt_shell_printf("please set device index\n");
    }
}

void APP_CMD_ModeSwitch(int argc, char *argv[])
{
    uint8_t mode = TRP_WMODE_NULL;

    if (argc==2)
    {
        mode = atoi(argv[1]);
        if (mode >= TRP_WMODE_CHECK_SUM && mode <= TRP_WMODE_FIX_PATTERN)
        {
            return APP_SetWorkMode(mode);
        }
        bt_shell_printf("set mode error\n");
    }
    else
    {
        bt_shell_printf("please set mode\n");
    }
    
}

void APP_CMD_PatternSelect(int argc, char *argv[])
{
    uint8_t fileType = APP_PATTERN_FILE_TYPE_50K;

    if (argc==2)
    {
        fileType = atoi(argv[1]);
        if (fileType >= APP_PATTERN_FILE_TYPE_MAX)
        {
            printf("file type error, must < %d\n", APP_PATTERN_FILE_TYPE_MAX);
            return;
        }
    }
    
    APP_PreparePatternData(fileType);
}


void APP_CMD_BurstModeStart(int argc, char *argv[])
{
    uint8_t devIndex = 0xFF;
    
    if (argc == 2)
    {
        devIndex = atoi(argv[1]);
    }

    APP_BurstModeStart(devIndex);
}

void APP_CMD_BurstModeStartAll(int argc, char *argv[])
{
    APP_BurstModeStartAll();
}

#ifdef ENABLE_AUTO_RUN
void APP_CMD_SetExecRuns(int argc, char *argv[])
{
    uint16_t execRuns = 0;
    
    if (argc == 2)
    {
        execRuns = atoi(argv[1]);
    }

    if (execRuns > 0)
        APP_SetExecIterations(execRuns);
}
#endif

void APP_CMD_SetIoCap(int argc, char *argv[])
{
    uint8_t iocap = 0xFF;

    if (argc == 1)
    {
        return;
    }
    else if (argc != 2)
    {
        bt_shell_printf("parameter error\n");
    }

    iocap = atoi(argv[1]);
    APP_IoCapSetting(iocap);
}

void APP_CMD_SetSecureConnection(int argc, char *argv[])
{
    uint8_t sc = 0xFF;

    if (argc == 1)
    {
        return;
    }
    else if (argc != 2)
    {
        bt_shell_printf("parameter error\n");
    }

    sc = atoi(argv[1]);
    APP_ScSetting(sc);
}

void APP_CMD_ReadPairInfo(int argc, char *argv[])
{
    bt_shell_printf("IO Capability = %s\n", APP_AGT_GetIoCap(NULL));
    APP_MGMT_ReadControllerSetting();
}




/*******************************************************************************
 End of File
 */
