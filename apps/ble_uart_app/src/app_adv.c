/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Advertising Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_adv.c

  Summary:
    This file contains the Application advertising functions for this project.

  Description:
    This file contains the Application advertising functions for this project.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdlib.h>
#include <string.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"
#include "bluetooth/mgmt.h"
#include "shared/shell.h"

#include "application.h"
#include "app_ble_handler.h"
#include "app_adv.h"
#include "app_error_defs.h"
#include "app_sm.h"
#include "app_mgmt.h"



// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

typedef struct APP_ADV_DataParams_T
{
    uint8_t                 advLen;
    uint8_t                 advData[BLE_GAP_ADV_MAX_LENGTH];
} APP_ADV_DataParams_T;


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_ADV_DataParams_T              s_bleAdvData;
static APP_ADV_DataParams_T              s_bleScanRspData;


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static uint8_t APP_ADV_CalculateDataLength(uint8_t *p_advData)
{
    /* Caculate Total Length of Adv Data /Scan Response Data Elements  */
    uint8_t len = 0, i = 0;

    while (1)
    {
        if (p_advData[i] != 0x00)   // Check the length is Zero or not
        {
            len++;               // Add Length field size
            len += p_advData[i];   // Add this Element Data Size

            if (len >= BLE_GAP_ADV_MAX_LENGTH)
            {
                len = BLE_GAP_ADV_MAX_LENGTH;
                break;
            }
            else
            {
                i = len;
            }
        }
        else
        {
            break;
        }
    }
    return len;
}

static uint16_t APP_ADV_StartExtAdv(void)
{
    uint16_t ret;

    ret = APP_MGMT_SetExtAdvEnable(true);
    if (ret != APP_RES_SUCCESS)
        return APP_RES_FAIL;
    else
        return APP_RES_SUCCESS;
}

static uint16_t APP_ADV_StopExtAdv(void)
{
    uint16_t ret;
    
    ret = APP_MGMT_SetExtAdvEnable(false);
    if (ret != APP_RES_SUCCESS)
        return APP_RES_FAIL;
    else
        return APP_RES_SUCCESS;
}

static uint16_t APP_ADV_SetExtAdvParams(void)
{
    uint16_t ret;

    struct mgmt_cp_add_ext_adv_params extAdvParam;

    memset(&extAdvParam, 0, sizeof(struct mgmt_cp_add_ext_adv_params));
    
    extAdvParam.instance = APP_ADV_HANDLE1;
    extAdvParam.min_interval = APP_ADV_DEFAULT_INTERVAL;
    extAdvParam.max_interval = APP_ADV_DEFAULT_INTERVAL;
    extAdvParam.tx_power = APP_ADV_TX_POWER_LEVEL;
    extAdvParam.flags |= MGMT_ADV_PARAM_INTERVALS;
    extAdvParam.flags |= MGMT_ADV_PARAM_TX_POWER;
    extAdvParam.flags |= MGMT_ADV_FLAG_CONNECTABLE;
    
    /* Existed secPhy flag make evt_properties set as LE_EXT_ADV_CONN_IND 
       (ref: hci_setup_ext_adv_instance_sync() in hci_sync.c)
       This result in legacy type adv have a invalid HCI parameter error when setting scan response data.*/
#ifdef APP_ADV_TYPE_EXT
    extAdvParam.flags |= MGMT_ADV_FLAG_SEC_2M;
#endif


    ret = APP_MGMT_SetExtAdvParams(&extAdvParam);
    if (ret != APP_RES_SUCCESS)
        return APP_RES_FAIL;
    else
        return APP_RES_SUCCESS;
}

static uint16_t APP_ADV_SetExtAdvData(void)
{
    uint16_t ret;
    uint16_t paramLen;
    struct mgmt_cp_add_ext_adv_data *p_advScanRspData;

    paramLen = sizeof(struct mgmt_cp_add_ext_adv_data) + s_bleAdvData.advLen + s_bleScanRspData.advLen;
    p_advScanRspData = malloc(paramLen);
    
    memset(p_advScanRspData, 0, paramLen);
    
    p_advScanRspData->instance = APP_ADV_HANDLE1;

#ifdef APP_ADV_TYPE_LEGACY
    p_advScanRspData->adv_data_len = s_bleAdvData.advLen;
    p_advScanRspData->scan_rsp_len = s_bleScanRspData.advLen;
#else //APP_ADV_TYPE_EXT
    p_advScanRspData->adv_data_len = s_bleAdvData.advLen + s_bleScanRspData.advLen;
    p_advScanRspData->scan_rsp_len = 0;
#endif
    memcpy(p_advScanRspData->data, &s_bleAdvData.advData, s_bleAdvData.advLen);
    if (s_bleScanRspData.advLen > 0)
    {
        memcpy(p_advScanRspData->data+s_bleAdvData.advLen, &s_bleScanRspData.advData, s_bleScanRspData.advLen);
    }

    ret = APP_MGMT_SetExtAdvData(p_advScanRspData);
    free(p_advScanRspData);
    
    if (ret != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    return APP_RES_SUCCESS;
}


void APP_ADV_UpdateAdvData(void)
{
    uint8_t idx = 0;

    memset(&s_bleAdvData.advData[0], 0x00, BLE_GAP_ADV_MAX_LENGTH);

    //Flags
    s_bleAdvData.advData[idx++] = (APP_ADV_TYPE_LEN + 1); //length
    s_bleAdvData.advData[idx++] = APP_ADV_TYPE_FLAGS;     //AD Type: flags
    s_bleAdvData.advData[idx++] = APP_ADV_FLAG_LE_GEN_DISCOV | APP_ADV_FLAG_BREDR_NOT_SUPPORTED;

    //Service Data
    s_bleAdvData.advData[idx++] = (APP_ADV_TYPE_LEN + APP_ADV_SRV_DATA_LEN); //length
    s_bleAdvData.advData[idx++] = APP_ADV_TYPE_SRV_DATA_16BIT_UUID;              //AD Type: Service Data
    s_bleAdvData.advData[idx++] = (uint8_t)APP_ADV_SERVICE_UUID_MCHP;
    s_bleAdvData.advData[idx++] = (uint8_t)(APP_ADV_SERVICE_UUID_MCHP >> 8);
    s_bleAdvData.advData[idx++] = APP_ADV_ADD_DATA_CLASS_BYTE;
    s_bleAdvData.advData[idx++] = APP_ADV_PROD_TYPE_BLE_UART;

    s_bleAdvData.advLen = APP_ADV_CalculateDataLength(&s_bleAdvData.advData[0]);
}

void APP_ADV_UpdateScanRspData(void)
{
    uint8_t devNameLen;

    memset(&s_bleScanRspData.advData[0], 0x00, BLE_GAP_ADV_MAX_LENGTH);

    APP_MGMT_GetLocalName(&devNameLen, &s_bleScanRspData.advData[2]);

    //Local Name
    s_bleScanRspData.advData[0] = (APP_ADV_TYPE_LEN + devNameLen); //length
    s_bleScanRspData.advData[1] = APP_ADV_TYPE_COMPLETE_LOCAL_NAME;

    s_bleScanRspData.advLen = APP_ADV_CalculateDataLength(&s_bleScanRspData.advData[0]);
}

uint16_t APP_ADV_Ctrl(uint8_t enable)
{
    uint16_t result = APP_RES_BAD_STATE;
    APP_BLE_ConnList_T *p_bleConn = NULL;

    if (enable)
    {
        p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_ADVERTISING, APP_BLE_STATE_ADVERTISING);
        
        if (p_bleConn == NULL)
        {
            p_bleConn = APP_GetFreeConnList();
            
            if (p_bleConn == NULL)
                return APP_RES_NO_RESOURCE;

            result = APP_ADV_StartExtAdv();
            if (result == APP_RES_SUCCESS)
            {
                APP_SetBleStateByLink(p_bleConn, APP_BLE_STATE_ADVERTISING);
            }
        }
    }
    else
    {
        p_bleConn = APP_GetBleLinkByStates(APP_BLE_STATE_ADVERTISING, APP_BLE_STATE_ADVERTISING);
        //1. There is a ADV link, so go to stop ADV.
        //2. When BLE_GAP_EVT_CONNECTED is coming, FW will go to stop ADV.
        if ((p_bleConn != NULL) || (APP_GetBleStateByLink(NULL) == APP_BLE_STATE_CONNECTED))
        {
            result = APP_ADV_StopExtAdv();
            if ((result == APP_RES_SUCCESS) && (APP_GetBleStateByLink(p_bleConn) == APP_BLE_STATE_ADVERTISING))
            {
                APP_SetBleStateByLink(p_bleConn, APP_BLE_STATE_STANDBY);
            }
        }
    }

    return result;
}

void APP_ADV_Start(void)
{
    uint16_t result = APP_RES_SUCCESS;
    
    APP_ADV_SetExtAdvParams();
    APP_ADV_UpdateAdvData();
    APP_ADV_UpdateScanRspData();
    APP_ADV_SetExtAdvData();
    
    result = APP_ADV_Ctrl(true);
    if (result == APP_RES_SUCCESS)
    {
        bt_shell_printf("advertising is on\n");
    }
}

void APP_ADV_Stop(void)
{
    uint16_t result = APP_RES_SUCCESS;
    
    result = APP_ADV_Ctrl(false);
    if (result == APP_RES_SUCCESS)
    {
        bt_shell_printf("advertising is off\n");
    }

}

void APP_ADV_Init(void)
{
}
