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
#include <errno.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"

#include "application.h"
#include "app_cmd.h"
#include "app_hci_dfu.h"
#include "app_hci_vnd.h"
#include "app_error_defs.h"
#include "app_timer.h"





// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
#define MAX_DFU_NAME_PATH 255



// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
APP_HCI_DfuParameters_T s_dfuParam;
gpointer s_dfuThread;


static APP_CMD_Menu_T s_mainMenu = {
    .name = "main",
    .entries = {
    { "dfu",          "...",      APP_CMD_DfuProcess, "DFU process, usage: dfu <file_name> [d] uart device [b] baud rate. e.g. dfu RNBD451_1.1.0.17.OTA.bin d /dev/ttyS1 b 115200" },
    { "aver",         "...",      APP_CMD_AppVer, "Query HCI controller application version, usage: aver [d] uart device [b] baud rate. e.g. aver d /dev/ttyS1 b 115200" },
    { } },
};

static void app_cmd_ResetDfuParam(void)
{
    if (s_dfuParam.p_dfuFilePath)
    {
        g_free(s_dfuParam.p_dfuFilePath);
        s_dfuParam.p_dfuFilePath = NULL;
    }

    if (s_dfuParam.p_ttyDevicePath)
    {
        g_free(s_dfuParam.p_ttyDevicePath);
        s_dfuParam.p_ttyDevicePath = NULL;
    }

    s_dfuParam.baudRate = 0;
}

static bool app_cmd_IsSupportBaudRate(int baudRate)
{
    switch(baudRate)
    {
        case BAUDRATE_9600:
        case BAUDRATE_19200:
        case BAUDRATE_38400:
        case BAUDRATE_57600:
        case BAUDRATE_115200:
        case BAUDRATE_230400:
        case BAUDRATE_460800:
        case BAUDRATE_921600:
            return true;
        default:
            return false;
    }
}


APP_CMD_Menu_T * APP_CMD_GetCmdMenu(void)
{
    return &s_mainMenu;
}

void APP_CMD_AppVer(int argc, char *argv[])
{
    int32_t hciDev, devDesc;
    uint32_t appVer;
    uint8_t paramIdx;
    int parseParam = 0;

    app_cmd_ResetDfuParam();

    if (argc > 1 && argc%2 == 1)
    {
        paramIdx = 1;
        while (paramIdx < argc)
        {
            parseParam = 0;
            switch(argv[paramIdx][0])
            {
                case 'b':
                    parseParam = 1;
                    break;
                case 'd':
                    parseParam = 2;
                    break;
                default:
            }
            if (parseParam == 1)
            {
                s_dfuParam.baudRate = atoi(argv[paramIdx+1]);
                if(s_dfuParam.baudRate <= 0)
                {
                    bt_shell_printf("invalid baudrate\n");
                    return;
                }
                else if(app_cmd_IsSupportBaudRate(s_dfuParam.baudRate) == false)
                {
                    bt_shell_printf("invalid baudrate\n");
                    return;
                }
            }
            else if (parseParam == 2)
            {
                s_dfuParam.p_ttyDevicePath = strdup(argv[paramIdx+1]);
            }
            paramIdx += 2;
        }
    }
    else if (argc != 1)
    {
        bt_shell_printf("invalid parameters\n");
        return;
    }

    if (s_dfuParam.p_ttyDevicePath && s_dfuParam.baudRate== 0)
    {
        bt_shell_printf("invalid parameters : no baud rate.\n");
        return;
    }

    if (s_dfuParam.p_ttyDevicePath == NULL)
    {
        hciDev = 0;
        devDesc = hci_open_dev(hciDev);
        if (devDesc < 0) {
            bt_shell_printf("Can't open device hci%d: %s (%d)\n", hciDev, strerror(errno), errno);
            goto failed;
        }
        APP_HCI_UseUartPath(false);
    }
    else
    {
        printf("HCI vendor command executed via UART device:%s\n", s_dfuParam.p_ttyDevicePath);
        devDesc = APP_HCI_OpenUartDev(s_dfuParam.p_ttyDevicePath);
        if (devDesc < 0)
            goto failed;
        
        if (APP_HCI_UartConfig(devDesc, s_dfuParam.baudRate, UART_CONFIG_FLCTRL, UART_CONFIG_DATA_BIT, UART_CONFIG_STOP_BIT, UART_CONFIG_PARITY) < 0 )
            goto failed;
        
        APP_HCI_UseUartPath(true);
    }


    if (APP_HCI_AppVersionInquiry(devDesc, &appVer, HCI_VND_CMD_TIMEOUT_DEFAULT) == APP_RES_SUCCESS)
    {
        bt_shell_printf("HCI controller application version=%d.%d.%d.%d\n", (appVer>>24 & 0xFF), (appVer>>16 & 0xFF), (appVer>>8 & 0xFF), (appVer & 0xFF));
        goto done;
    }

failed:
    bt_shell_printf("HCI vendor command executed fail\n");
done:
    hci_close_dev(devDesc);
}


void APP_CMD_DfuProcess(int argc, char *argv[])
{
    uint8_t paramIdx;
    int parseParam = 0;

    app_cmd_ResetDfuParam();

    if (argc < 2)
    {
        bt_shell_printf("invalid parameters: no OTAU image file.\n");
        return;
    }
    else if (strlen(argv[1]) > MAX_DFU_NAME_PATH)
    {
        bt_shell_printf("DFU file path too long\n");
        return;
    }

    
    if (argc > 2 && argc%2 == 0)
    {
        paramIdx = 2;
        while (paramIdx < argc)
        {
            parseParam = 0;
            switch(argv[paramIdx][0])
            {
                case 'b':
                    parseParam = 1;
                    break;
                case 'd':
                    parseParam = 2;
                    break;
                default:
            }
            if (parseParam == 1)
            {
                s_dfuParam.baudRate = atoi(argv[paramIdx+1]);
                if(s_dfuParam.baudRate <= 0)
                {
                    bt_shell_printf("invalid baudrate\n");
                    return;
                }
                else if(app_cmd_IsSupportBaudRate(s_dfuParam.baudRate) == false)
                {
                    bt_shell_printf("invalid baudrate\n");
                    return;
                }
            }
            else if (parseParam == 2)
            {
                s_dfuParam.p_ttyDevicePath = strdup(argv[paramIdx+1]);
            }
            paramIdx += 2;
        }
        

    }
    else if (argc != 2)
    {
        bt_shell_printf("invalid parameters: missing parameters.\n");
        return;
    }

    if (s_dfuParam.p_ttyDevicePath && s_dfuParam.baudRate== 0)
    {
        bt_shell_printf("invalid parameters : no baud rate.\n");
        return;
    }


    s_dfuParam.p_dfuFilePath = strdup(argv[1]);
    s_dfuThread = g_thread_new("dfuthread", APP_DFU_ProcessThread, &s_dfuParam);

    g_thread_unref(s_dfuThread);
}


/*******************************************************************************
 End of File
 */
