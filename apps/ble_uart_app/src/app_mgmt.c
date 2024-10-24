/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BT Mgmt wrapper function Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_mgmt.c

  Summary:
    This file contains BT Mgmt wrapper function.

  Description:
    This file contains BT Mgmt wrapper function.
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
#include <stdbool.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/time.h>

#include "gdbus/gdbus.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"
#include "bluetooth/mgmt.h"
#include "shared/shell.h"
#include "shared/mgmt.h"
#include "shared/util.h"

#include "application.h"
#include "app_error_defs.h"
#include "app_adv.h"
#include "app_mgmt.h"
#include "app_log.h"
#include "app_dbp.h"




// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

static struct mgmt *sp_mgmtPrimary = NULL;
static uint16_t s_mgmtIndex = 0;
static uint8_t s_mgmtVersion = 0;
static uint8_t s_mgmtRevision = 0;
static char s_localNameComplete[MGMT_MAX_NAME_LENGTH];
static char s_localNameShort[MGMT_MAX_SHORT_NAME_LENGTH];


#ifdef ENABLE_BLUEZ_DEBUG
static void app_mgmt_BluezDebug(void)
{
    struct mgmt_cp_set_exp_feature cp;
    struct mgmt_exp_uuid {
        uint8_t val[16];
        const char *str;
    };

    /* d4992530-b9ec-469f-ab01-6c481c47da1c */
    static const struct mgmt_exp_uuid bluez_debug_uuid = {
        .val = { 0x1c, 0xda, 0x47, 0x1c, 0x48, 0x6c, 0x01, 0xab,
                 0x9f, 0x46, 0xec, 0xb9, 0x30, 0x25, 0x99, 0xd4 },
        .str = "d4992530-b9ec-469f-ab01-6c481c47da1c"
    };


    memset(&cp, 0, sizeof(cp));
    memcpy(cp.uuid, bluez_debug_uuid.val, 16);
    cp.action = 1;

    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_SET_EXP_FEATURE, MGMT_INDEX_NONE, sizeof(cp), &cp, NULL, NULL, NULL) > 0)
        return;
}
#endif

#ifdef ENABLE_EXP_MULTI_ROLE
static void app_mgmt_SimultCentralPeripheral(void)
{
    struct mgmt_cp_set_exp_feature cp;
    struct mgmt_exp_uuid {
        uint8_t val[16];
        const char *str;
    };

    /* d4992530-b9ec-469f-ab01-6c481c47da1c */
    static const struct mgmt_exp_uuid le_simult_central_peripheral_uuid = {
        .val = { 0xd6, 0x49, 0xb0, 0xd1, 0x28, 0xeb, 0x27, 0x92,
                 0x96, 0x46, 0xc0, 0x42, 0xb5, 0x10, 0x1b, 0x67 },
        .str = "671b10b5-42c0-4696-9227-eb28d1b049d6"
    };


    memset(&cp, 0, sizeof(cp));
    memcpy(cp.uuid, le_simult_central_peripheral_uuid.val, 16);
    cp.action = 1;

    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_SET_EXP_FEATURE, s_mgmtIndex, sizeof(cp), &cp, NULL, NULL, NULL) > 0)
        return;
}
#endif


static void app_mgmt_ReadVersionCompleteCb(uint8_t status, uint16_t length,
                    const void *p_param, void *p_userData)
{
    const struct mgmt_rp_read_version *p_rp = p_param;

    if (status != MGMT_STATUS_SUCCESS) {
        printf("Failed to read version information: %s (0x%02x)", mgmt_errstr(status), status);
        return;
    }

    if (length < sizeof(*p_rp)) {
        printf("Wrong size of read version response");
        return;
    }

    s_mgmtVersion = p_rp->version;
    s_mgmtRevision = btohs(p_rp->revision);
}

static void app_mgmt_DevConnCb(uint16_t index, uint16_t len, const void *p_param, void *p_userData)
{
    const struct mgmt_ev_device_connected *p_ev = p_param;
    char addr[18];

    if (p_param == NULL)
    {
        printf("param is NULL\n");
        return;
    }

    if (len < sizeof(*p_ev)) {
        printf("Invalid connected event length (%u bytes)\n", len);
        return;
    }

    ba2str(&p_ev->addr.bdaddr, addr);
    //printf("hci%u %s connected, flags=%x\n", index, addr, ev->flags);
    //ev->flags == 0x08 is centrol role
    //ev->flags == 0x02 Legacy Pairing
}

/*
Possible values for the Reason parameter:
    0   Unspecified
    1   Connection timeout
    2   Connection terminated by local host
    3   Connection terminated by remote host
    4   Connection terminated due to authentication failure
    5   Connection terminated by local host for suspend
*/
static void app_mgmt_DevDisconnCb(uint16_t index, uint16_t length, const void *p_param, void *p_userData)
{
    const struct mgmt_ev_device_disconnected *p_ev = p_param;
    uint8_t reason;

    if (length < sizeof(struct mgmt_addr_info)) {
        printf("Too small device disconnected event\n");
        return;
    }

    if (length < sizeof(*p_ev))
        reason = MGMT_DEV_DISCONN_UNKNOWN;
    else
        reason = p_ev->reason;

    printf("disconnect reason=%d\n", reason);
}


static void app_mgmt_AddExtAdvParamsCb(uint8_t status, uint16_t length,
                    const void *p_param, void *p_userData)
{
    const struct mgmt_rp_add_ext_adv_params *p_rp = p_param;

    if (status)
        goto fail;

    if (!p_param || length < sizeof(*p_rp)) {
        status = MGMT_STATUS_FAILED;
        goto fail;
    }

    //printf("tx_power=%d, adv_len=%d, scanRsp_len=%d\n", rp->tx_power, rp->max_adv_data_len, rp->max_scan_rsp_len);

    return;
    
fail:
    LOG("MGMT_OP_ADD_EXT_ADV_PARAMS response error\n");
}

static void app_mgmt_AdvRemoved(uint16_t index, uint16_t len, const void *p_param, void *p_userData)
{
    const struct mgmt_ev_advertising_removed *p_ev = p_param;

    if (len < sizeof(*p_ev)) {
        printf("Too small (%u bytes) advertising_removed event\n", len);
        return;
    }

    printf("hci%u advertising_removed: instance %u\n", index, p_ev->instance);
}

static void app_mgmt_AdvAdded(uint16_t index, uint16_t len, const void *p_param, void *p_userData)
{
    const struct mgmt_ev_advertising_added *p_ev = p_param;

    if (len < sizeof(*p_ev)) {
        printf("Too small (%u bytes) advertising_added event\n", len);
        return;
    }

    printf("hci%u advertising_added: instance %u\n", index, p_ev->instance);
}

static void app_mgmt_ReadIndexRsp(uint8_t status, uint16_t len, const void *param, void *user_data)
{
    const struct mgmt_rp_read_index_list *rp = param;
    uint16_t count;

    if (status != 0) {
        printf("Reading index list failed with status 0x%02x (%s)\n", status, mgmt_errstr(status));
        return;
    }

    if (len < sizeof(*rp)) {
        printf("Too small index list reply (%u bytes)\n", len);
        return;
    }

    count = le16_to_cpu(rp->num_controllers);

    if (len < sizeof(*rp) + count * sizeof(uint16_t)) {
        printf("Index count (%u) doesn't match reply length (%u)\n", count, len);
        return;
    }

    if (count == 0)
    {
        printf("(ERROR) No HCI device attached.\n");
    }
}

static void app_mgmt_ReadIndexList(void)
{
    if (!mgmt_send(sp_mgmtPrimary, MGMT_OP_READ_INDEX_LIST, MGMT_INDEX_NONE, 0, NULL,
                    app_mgmt_ReadIndexRsp, sp_mgmtPrimary, NULL)) {
        printf("Unable to send index_list cmd\n");
    }
}


void APP_MGMT_Init(void)
{
    sp_mgmtPrimary = mgmt_new_default();
    if (sp_mgmtPrimary == NULL) {
        printf("Failed to access management interface\n");
        return;
    }

    s_mgmtIndex = 0;

    app_mgmt_ReadIndexList();
    
    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_READ_VERSION,
                MGMT_INDEX_NONE, 0, NULL,
                app_mgmt_ReadVersionCompleteCb, NULL, NULL) <= 0)
    {
        printf("Failed to read management version information\n");
    }


    mgmt_register(sp_mgmtPrimary, MGMT_EV_DEVICE_CONNECTED, s_mgmtIndex, app_mgmt_DevConnCb, NULL, NULL);
    mgmt_register(sp_mgmtPrimary, MGMT_EV_DEVICE_DISCONNECTED, s_mgmtIndex, app_mgmt_DevDisconnCb, NULL, NULL);
    mgmt_register(sp_mgmtPrimary, MGMT_EV_ADVERTISING_ADDED, s_mgmtIndex, app_mgmt_AdvAdded, NULL, NULL);
    mgmt_register(sp_mgmtPrimary, MGMT_EV_ADVERTISING_REMOVED, s_mgmtIndex, app_mgmt_AdvRemoved, NULL, NULL);

#ifdef ENABLE_BLUEZ_DEBUG
    app_mgmt_BluezDebug();
#endif
#ifdef ENABLE_EXP_MULTI_ROLE
    app_mgmt_SimultCentralPeripheral();
#endif
}

void APP_MGMT_Deinit(void)
{
    mgmt_cancel_all(sp_mgmtPrimary);
    mgmt_unregister_all(sp_mgmtPrimary);
    mgmt_unref(sp_mgmtPrimary);
}

static void app_mgmt_SetLocalNameCpl(uint8_t status, uint16_t length, const void *p_param, void *p_userData)
{

    if (status != MGMT_STATUS_SUCCESS) {
        printf("Failed to set local name: %s (0x%02x)", mgmt_errstr(status), status);
        return;
    }
}

uint16_t APP_MGMT_SetLocalName(uint8_t len, uint8_t  *p_devName)
{
    struct mgmt_cp_set_local_name setLocalName;

    if (sp_mgmtPrimary == NULL)
    {
        return APP_RES_FAIL;
    }

    memset(s_localNameComplete, 0, MGMT_MAX_NAME_LENGTH);
    memset(s_localNameShort, 0, MGMT_MAX_SHORT_NAME_LENGTH);
    
    memcpy(s_localNameComplete, p_devName, len);
    if (len >= MGMT_MAX_SHORT_NAME_LENGTH)
        memcpy(s_localNameShort, p_devName, MGMT_MAX_SHORT_NAME_LENGTH-1);
    else
        memcpy(s_localNameShort, p_devName, len);
    
    memcpy(setLocalName.name, s_localNameComplete, MGMT_MAX_NAME_LENGTH);
    memcpy(setLocalName.short_name, s_localNameShort, MGMT_MAX_SHORT_NAME_LENGTH);
        
    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_SET_LOCAL_NAME,
                s_mgmtIndex, sizeof(setLocalName), &setLocalName,
                app_mgmt_SetLocalNameCpl, NULL, NULL) <= 0)
    {
        LOG("MGMT_OP_SET_LOCAL_NAME failed\n");
        return APP_RES_FAIL;
    }
    
    return APP_RES_SUCCESS;
}

uint16_t APP_MGMT_GetLocalName(uint8_t *p_len, uint8_t  *p_devName)
{
    uint8_t lenNameCompl;
    uint8_t lenNameShort;
    
    if (p_len == NULL || p_devName == NULL)
    {
        return APP_RES_INVALID_PARA;
    }

    lenNameCompl = strlen(s_localNameComplete);
    lenNameShort = strlen(s_localNameShort);
    if(lenNameCompl > BLE_GAP_ADV_MAX_LENGTH)
    {
        *p_len = lenNameShort;
        memcpy(p_devName, s_localNameShort, lenNameShort);
        p_devName[lenNameShort] = 0;
    }
    else
    {
        *p_len = lenNameCompl;
        memcpy(p_devName, s_localNameComplete, lenNameCompl);
        p_devName[lenNameCompl] = 0;
    }

    return APP_RES_SUCCESS;
}

void app_mgmt_SetExtAdvParamsDestroy(void *p_userData)
{
    if (p_userData != NULL)
    {
        free(p_userData);
    }
}


/*
flag defined in BlueZ
0   Switch into Connectable mode
1   Advertise as Discoverable
2   Advertise as Limited Discoverable
3   Add Flags field to Adv_Data
4   Add TX Power field to Adv_Data
5   Add Appearance field to Scan_Rsp
6   Add Local Name in Scan_Rsp
7   Secondary Channel with LE 1M
8   Secondary Channel with LE 2M
9   Secondary Channel with LE Coded
12  The Duration parameter should be used
13  The Timeout parameter should be used
14  The Interval parameters should be used
15  The Tx Power parameter should be used
16  The advertisement will contain a scan response
*/
uint16_t APP_MGMT_SetExtAdvParams(struct mgmt_cp_add_ext_adv_params *p_extAdvParams)
{
    uint16_t paramLen;
    struct mgmt_cp_add_ext_adv_params *p_cp;
    
    if (sp_mgmtPrimary == NULL)
    {
        return APP_RES_FAIL;
    }

    paramLen = sizeof(struct mgmt_cp_add_ext_adv_params);
    p_cp = malloc(paramLen);
    memcpy(p_cp, p_extAdvParams, paramLen);

    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_ADD_EXT_ADV_PARAMS,
                s_mgmtIndex, paramLen, p_cp,
                app_mgmt_AddExtAdvParamsCb, p_cp, app_mgmt_SetExtAdvParamsDestroy) <= 0)
    {
        bt_shell_printf("MGMT_OP_ADD_EXT_ADV_PARAMS failed\n");
        return APP_RES_FAIL;
    }

    return APP_RES_SUCCESS;
}

void app_mgmt_SetExtAdvDataDestroy(void *p_userData)
{
    if (p_userData != NULL)
    {
        free(p_userData);
    }
}

uint16_t APP_MGMT_SetExtAdvData(struct mgmt_cp_add_ext_adv_data *p_extAdvData)
{
    uint16_t paramLen;
    struct mgmt_cp_add_ext_adv_data *p_cp;
    
    if (sp_mgmtPrimary == NULL)
    {
        return APP_RES_FAIL;
    }

    paramLen = sizeof(struct mgmt_cp_add_ext_adv_data) + p_extAdvData->adv_data_len + p_extAdvData->scan_rsp_len;
    p_cp = malloc(paramLen);
    memcpy(p_cp, p_extAdvData, paramLen);

    /* Submit request to update instance data */
    if(mgmt_send(sp_mgmtPrimary, MGMT_OP_ADD_EXT_ADV_DATA,
                 s_mgmtIndex, paramLen, p_cp,
                 NULL, p_cp, app_mgmt_SetExtAdvDataDestroy) <= 0)
    {
        bt_shell_printf("APP_MGMT_SetExtAdvData failed\n");
        return APP_RES_FAIL;
    }

    return APP_RES_SUCCESS;
}


/*
The value 0x00 disables advertising, the value 0x01 enables
advertising with considering of connectable setting and the
value 0x02 enables advertising in connectable mode.
*/
uint16_t APP_MGMT_SetExtAdvEnable(bool enable)
{
    struct mgmt_cp_remove_advertising cp;

    if (enable)
    {
        //"HCI Command: LE Set Extended Advertising Enable" will be auto executed 
        //after MGMT_OP_ADD_EXT_ADV_DATA has been called.
        bt_shell_printf("advertising name=%s\n", s_localNameComplete);
        return APP_RES_SUCCESS;
    }
    else
    {
        cp.instance = 0; //clear all advertising sets
        if (mgmt_send(sp_mgmtPrimary, MGMT_OP_REMOVE_ADVERTISING, 
                      s_mgmtIndex, sizeof(cp), &cp,
                      NULL, NULL, NULL) <= 0)
        {
            bt_shell_printf("MGMT_OP_REMOVE_ADVERTISING(%d) failed\n", enable);
            return APP_RES_FAIL;
        }
    }

    return APP_RES_SUCCESS;
}

static void app_mgmt_SePhyRsp(uint8_t status, uint16_t len, const void *p_param, void *p_userData)
{
    if (status != 0) {
        bt_shell_printf("Could not set PHY Configuration with status 0x%02x (%s)\n", status, mgmt_errstr(status));
    }
    else
    {
        APP_MGMT_GetPhySupport();
    }
}

static const char *sp_physStr[] = {
    "BR1M1SLOT",
    "BR1M3SLOT",
    "BR1M5SLOT",
    "EDR2M1SLOT",
    "EDR2M3SLOT",
    "EDR2M5SLOT",
    "EDR3M1SLOT",
    "EDR3M3SLOT",
    "EDR3M5SLOT",
    "LE1MTX",
    "LE1MRX",
    "LE2MTX",
    "LE2MRX",
    "LECODEDTX",
    "LECODEDRX",
};

static const char *phys2str(uint32_t phys)
{
    static char str[256];
    unsigned int i;
    int off;

    off = 0;
    str[0] = '\0';

    for (i = 0; i < NELEM(sp_physStr); i++) {
        if ((phys & (1 << i)) != 0)
            off += snprintf(str + off, sizeof(str) - off, "%s ", sp_physStr[i]);
    }
    
    return str;
}

static void app_mgmt_GetPhyRsp(uint8_t status, uint16_t len, const void *p_param, void *p_userData)
{
    const struct mgmt_rp_get_phy_confguration *p_rp = p_param;
    uint32_t selected_phys;

    if (status != 0) {
        bt_shell_printf("Get PHY Configuration failed with status 0x%02x (%s)\n", status, mgmt_errstr(status));
        return;
    }

    if (len < sizeof(*p_rp)) {
        bt_shell_printf("Too small get-phy reply (%u bytes)\n", len);
        return;
    }

    selected_phys = get_le32(&p_rp->selected_phys);
    bt_shell_printf("Selected phys: %s\n", phys2str(selected_phys));
}


/*
The PHYs parameters are a bitmask with currently the
following available bits:

    0   BR 1M 1-Slot
    1   BR 1M 3-Slot
    2   BR 1M 5-Slot
    3   EDR 2M 1-Slot
    4   EDR 2M 3-Slot
    5   EDR 2M 5-Slot
    6   EDR 3M 1-Slot
    7   EDR 3M 3-Slot
    8   EDR 3M 5-Slot
    9   LE 1M TX
    10  LE 1M RX
    11  LE 2M TX
    12  LE 2M RX
    13  LE Coded TX
    14  LE Coded RX
*/
uint16_t APP_MGMT_SetPhySupport(uint32_t phySupport)
{
    struct mgmt_cp_set_phy_confguration cp;
    cp.selected_phys = cpu_to_le32(phySupport);

    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_SET_PHY_CONFIGURATION, s_mgmtIndex, sizeof(cp), 
                  &cp, app_mgmt_SePhyRsp, NULL, NULL) <= 0) {
        bt_shell_printf("Unable to send Set PHY cmd\n");
        return APP_RES_FAIL;
    }

    return APP_RES_SUCCESS;
}

uint16_t APP_MGMT_GetPhySupport(void)
{
    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_GET_PHY_CONFIGURATION, s_mgmtIndex, 0, NULL,
                  app_mgmt_GetPhyRsp, NULL, NULL) <= 0) {
        bt_shell_printf("Unable to send Get PHY cmd\n");
        return APP_RES_FAIL;
    }
    return APP_RES_SUCCESS;
}

uint16_t APP_MGMT_RemoveBonding(const char *p_bdaddr, const char * p_bdaddrType)
{
    struct mgmt_cp_unpair_device cp;
    uint8_t bdaddrType;

    if (!strcmp(p_bdaddrType, "random"))
    {
        bdaddrType = BDADDR_LE_RANDOM;
    }
    else
    {
        bdaddrType = BDADDR_LE_PUBLIC;
    }
    memset(&cp, 0, sizeof(cp));
    str2ba(p_bdaddr, &cp.addr.bdaddr);
    cp.addr.type = bdaddrType;
    cp.disconnect = 1;
    printf("device [%s]type=%d unpairing\n", p_bdaddr, bdaddrType);

    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_UNPAIR_DEVICE,
                s_mgmtIndex, sizeof(cp), &cp,
                NULL, NULL, NULL) <= 0)
        return APP_RES_FAIL;

    return APP_RES_SUCCESS;
}

static void app_mgmt_SetSecureConnectionCpl(uint8_t status, uint16_t length,
                    const void *p_param, void *p_userData)
{
    if (status != MGMT_STATUS_SUCCESS) {
        printf("Failed to set mode: %s (0x%02x)", mgmt_errstr(status), status);
        return;
    }

    bt_shell_printf("Secure Connection mode setup complete\n");
}

uint16_t APP_MGMT_SetSecureConnection(uint8_t mode)
{
    struct mgmt_mode cp;
    
    memset(&cp, 0, sizeof(cp));
    cp.val = mode;
    
    if (mgmt_send(sp_mgmtPrimary, MGMT_OP_SET_SECURE_CONN,
                s_mgmtIndex, sizeof(cp), &cp,
                app_mgmt_SetSecureConnectionCpl, NULL, NULL) > 0)
        return APP_RES_SUCCESS;
    
    printf("Failed to set Secure Connection mode\n");
    
    return APP_RES_FAIL;
}

/*
Current_Settings and Supported_Settings is a bitmask with
currently the following available bits:

    0   Powered
    1   Connectable
    2   Fast Connectable
    3   Discoverable
    4   Bondable
    5   Link Level Security (Sec. mode 3)
    6   Secure Simple Pairing
    7   Basic Rate/Enhanced Data Rate
    8   High Speed
    9   Low Energy
    10  Advertising
    11  Secure Connections
    12  Debug Keys
    13  Privacy
    14  Controller Configuration
    15  Static Address
    16  PHY Configuration
    17  Wideband Speech
*/
#define MGMT_SETTING_SEC_CONN (0x800)

static void app_mgmt_ReadControllerInfoRsp(uint8_t status, uint16_t len, const void *p_param,
                            void *p_userData)
{
    const struct mgmt_rp_read_info *p_rp = p_param;
    uint16_t index = PTR_TO_UINT(p_userData);
    uint32_t currSettings;

    if (status != 0) {
        printf("Reading hci%u info failed with status 0x%02x (%s)\n",
                    index, status, mgmt_errstr(status));
        return;
    }

    if (len < sizeof(*p_rp)) {
        printf("Too small info reply (%u bytes)\n", len);
        return;
    }

    currSettings = le32_to_cpu(p_rp->current_settings);

    if (currSettings & MGMT_SETTING_SEC_CONN) {
        bt_shell_printf("secure connection = enabled\n");
    } else {
        bt_shell_printf("secure connection = disabled\n");
    }
    return;
}


uint16_t APP_MGMT_ReadControllerSetting(void)
{
    if (!mgmt_send(sp_mgmtPrimary, MGMT_OP_READ_INFO, s_mgmtIndex, 0, NULL, app_mgmt_ReadControllerInfoRsp,
                    UINT_TO_PTR(s_mgmtIndex), NULL)) {
        printf("Unable to send read_info cmd\n");
        return APP_RES_FAIL;
    }

    return APP_RES_SUCCESS;
}


/*******************************************************************************
 End of File
 */
