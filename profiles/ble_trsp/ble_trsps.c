/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>

#include "shared/att-types.h"
#include "shared/util.h"



#include "ble_trs/ble_trs.h"
#include "ble_trsp/ble_trsps.h"
#include "ble_trsp/ble_trsp_defs.h"




// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

/**@defgroup BLE_TRSPS_INIT_CREDIT BLE_TRSPS_INIT_CREDIT
 * @brief The definition of initial credit value.
 * @{ */
#define BLE_TRSPS_INIT_CREDIT                   (0x0A)//0x10    /**< Definition of initial credit */
/** @} */

/**@defgroup BLE_TRSPS_MAX_BUF BLE_TRSPS_MAX_BUF
 * @brief The definition of maximum buffer list.
 * @{ */
#define BLE_TRSPS_MAX_BUF_IN                    (BLE_TRSPS_INIT_CREDIT*BLE_TRSPS_MAX_CONN_NBR)     /**< Maximum incoming queue number */
/** @} */

/**@defgroup BLE_TRSPS_MAX_RETURN_CREDIT BLE_TRSPS_MAX_RETURN_CREDIT
 * @brief The definition of maximum return credit number.
 * @{ */
#define BLE_TRSPS_MAX_RETURN_CREDIT              (0x07)//(13)   /**< Maximum return credit number */
/** @} */

/**@defgroup BLE_TRSPS_CBFC BLE_TRSPS_CBFC
 * @brief The definition of credit base flow control.
 * @{ */
#define BLE_TRSPS_CBFC_TX_ENABLED               1              /**< Definition of ble transparent service credit based transmit enable. */
#define BLE_TRSPS_CBFC_RX_ENABLED               (1<<1)         /**< Definition of ble transparent service credit based receive enable. */
/** @} */

/**@defgroup BLE_TRSPS_CBFC_OPCODE BLE_TRSPS_CBFC_OPCODE
 * @brief The definition of BLE transparent credit based flow control
 * @{ */
#define BLE_TRSPS_CBFC_OPCODE_SERVER_ENABLED    0x14    /**< Definition of Op Code for Credit Based Flow Control Protocol, sending by Server. */
#define BLE_TRSPS_CBFC_OPCODE_GIVE_CREDIT       0x15    /**< Definition of Op Code for Credit Based Flow Control Protocol, giving credit. */
#define BLE_TRSPS_CBFC_OPCODE_SUCCESS           0       /**< Definition of response for successful operation. */
/** @} */

/**@defgroup BLE_TRSPS_VENDOR_OPCODE BLE_TRSPS_VENDOR_OPCODE
 * @brief The definition of BLE transparent vendor opcodes
 * @{ */
#define BLE_TRSPS_VENDOR_OPCODE_MIN             0x20    /**< Definition of Op Code range in TRS vendor commands. */
#define BLE_TRSPS_VENDOR_OPCODE_MAX             0xFF    /**< Definition of Op Code range in TRS vendor commands. */
/** @} */

/**@defgroup BLE_TRSPS_STATE TRSPS state
 * @brief The definition of BLE TRSPS connection state
 * @{ */
typedef enum BLE_TRSPS_State_T
{
    BLE_TRSPS_STATE_IDLE = 0x00,        /**< Default state (Disconnected). */
    BLE_TRSPS_STATE_CONNECTED           /**< Connected. */
} BLE_TRSPS_State_T;
/** @} */

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

/**@brief The structure contains information about BLE transparent profile packetIn. */
typedef struct BLE_TRSPS_PacketList_T
{
    uint16_t                   length;                  /**< Data length. */
    uint8_t                    *p_packet;               /**< Pointer to the TX/RX data buffer */
} BLE_TRSPS_PacketList_T;

/**@brief The structure contains information about packet input queue format of BLE transparent profile. */
typedef struct BLE_TRSPS_QueueIn_T
{
    uint8_t                    usedNum;                    /**< The number of data list of packetIn buffer. */
    uint8_t                    writeIndex;                 /**< The Index of data, written in packet buffer. */
    uint8_t                    readIndex;                  /**< The Index of data, read in packet buffer. */
    BLE_TRSPS_PacketList_T     packetList[BLE_TRSPS_INIT_CREDIT];  /**< Written in packet buffer. @ref BLE_TRSPS_PacketBufferIn_T.*/
} BLE_TRSPS_QueueIn_T;

/**@brief The structure contains information about BLE transparent profile connection parameters for recording connection information. */
typedef struct BLE_TRSPS_ConnList_T
{
    GDBusProxy                 *p_dev;                  /**< Proxy to org.bluez.device interface. */
    uint8_t                    state;                   /**< Connection state. */
    uint16_t                   attMtu;                  /**< Record the current connection MTU size. */
    uint8_t                    cbfcEnable;              /**< Credit based flow enable. @ref BLE_TRSPS_CREDIT_BASED_FLOW_CONTROL. */
    uint8_t                    peerCredit;              /**< Credit number from Central to Peripheral. */
    uint8_t                    localCredit;             /**< Credit number from Peripheral to Central. */
    BLE_TRSPS_QueueIn_T        inputQueue;              /**< Input queue to store Rx packets. */
} BLE_TRSPS_ConnList_T;

// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************

static BLE_TRSPS_EventCb_T      bleTrspsProcess;
static BLE_TRSPS_ConnList_T     s_trsConnList[BLE_TRSPS_MAX_CONN_NBR];
static uint8_t                  s_trsState;                /**< BLE transparent service current state. @ref BLE_TRSPS_STATUS.*/


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static void ble_trsps_InitConnList(BLE_TRSPS_ConnList_T *p_conn)
{
    memset((uint8_t *)p_conn, 0, sizeof(BLE_TRSPS_ConnList_T));
    p_conn->attMtu= BT_ATT_DEFAULT_LE_MTU;
}

static BLE_TRSPS_ConnList_T * ble_trsps_GetConnListByProxy(GDBusProxy *p_dev)
{
    uint8_t i;

    for(i=0; i<BLE_TRSPS_MAX_CONN_NBR;i++)
    {
        if ((s_trsConnList[i].state == BLE_TRSPS_STATE_CONNECTED) && (s_trsConnList[i].p_dev == p_dev))
        {
            return &s_trsConnList[i];
        }
    }

    return NULL;
}

static BLE_TRSPS_ConnList_T * ble_trsps_GetConnListByObjPath(char *p_path)
{
    uint8_t i;

    for(i=0; i<BLE_TRSPS_MAX_CONN_NBR;i++)
    {
        if ((s_trsConnList[i].state == BLE_TRSPS_STATE_CONNECTED) && (strcmp(g_dbus_proxy_get_path(s_trsConnList[i].p_dev), p_path) == 0))
        {
            return &s_trsConnList[i];
        }
    }

    return NULL;
}


static BLE_TRSPS_ConnList_T *ble_trsps_GetFreeConnList(void)
{
    uint8_t i;

    for(i=0; i<BLE_TRSPS_MAX_CONN_NBR;i++)
    {
        if (s_trsConnList[i].state == BLE_TRSPS_STATE_IDLE)
        {
            s_trsConnList[i].state = BLE_TRSPS_STATE_CONNECTED;
            return &s_trsConnList[i];
        }
    }

    return NULL;
}

static void ble_trsps_ServerReturnCredit(BLE_TRSPS_ConnList_T *p_conn)
{
    uint8_t buf[5];
    memset(buf, 0x00, sizeof(buf));

    if (p_conn->peerCredit == 0U)
    {
        return;
    }

    buf[0] = BLE_TRSPS_CBFC_OPCODE_SUCCESS;
    buf[1] = BLE_TRSPS_CBFC_OPCODE_SERVER_ENABLED;
    put_be16(p_conn->attMtu, &buf[2]);
    buf[4] = p_conn->peerCredit;
        
    BLE_TRS_UpdateValueCtrl(buf, sizeof(buf));

    p_conn->peerCredit = 0;
}


void BLE_TRSPS_EventRegister(BLE_TRSPS_EventCb_T bleTranServHandler)
{
    bleTrspsProcess = bleTranServHandler;
}

uint16_t BLE_TRSPS_Init(DBusConnection *p_dbusConn, GDBusProxy * p_proxyGattMgr)
{
    uint8_t i;

    for (i = 0; i < BLE_TRSPS_MAX_CONN_NBR; i++)
    {
        ble_trsps_InitConnList(&s_trsConnList[i]);
    }

    if (BLE_TRS_Add(p_dbusConn, p_proxyGattMgr) == true)
    {
        return TRSP_RES_SUCCESS;
    }
    else
    {
        return TRSP_RES_FAIL;
    }
}

uint16_t BLE_TRSPS_SendVendorCommand(GDBusProxy *p_proxyDev, uint8_t commandID, uint8_t commandLength, uint8_t *p_commandPayload)
{
    BLE_TRSPS_ConnList_T *p_conn;
    uint8_t *p_buf;

    p_conn = ble_trsps_GetConnListByProxy(p_proxyDev);
    if (p_conn == NULL)
    {
        return TRSP_RES_FAIL;
    }

    if (commandID < BLE_TRSPS_VENDOR_OPCODE_MIN)
    {
        return TRSP_RES_INVALID_PARA;
    }

    if (commandLength > (p_conn->attMtu-TRSP_ATT_HEADER_SIZE-1U))
    {
        return TRSP_RES_INVALID_PARA;
    }

    
    p_buf = malloc(1+commandLength);
    if (p_buf != NULL)
    {
        p_buf[0] = commandID;
        memcpy(&p_buf[1], p_commandPayload, commandLength);
        
        BLE_TRS_UpdateValueCtrl(p_buf, commandLength + 1);
        free(p_buf);

        return TRSP_RES_SUCCESS;
    }

    return TRSP_RES_OOM;
}

uint16_t BLE_TRSPS_SendData(GDBusProxy *p_proxyDev, uint16_t len, uint8_t *p_data)
{
    BLE_TRSPS_ConnList_T *p_conn;

    p_conn = ble_trsps_GetConnListByProxy(p_proxyDev);
    if (p_conn == NULL)
    {
        return TRSP_RES_FAIL;
    }

    if (s_trsState != BLE_TRSPS_STATUS_TX_OPENED)
    {
        return TRSP_RES_BAD_STATE;
    }

    if (((p_conn->cbfcEnable&BLE_TRSPS_CBFC_TX_ENABLED)!=0U) && (p_conn->localCredit == 0U))
    {
        return TRSP_RES_NO_RESOURCE;
    }

    if (len > (p_conn->attMtu - TRSP_ATT_HEADER_SIZE))
    {
        return TRSP_RES_FAIL;
    }

    BLE_TRS_UpdateValueTx(p_data, len);

    if ((p_conn->cbfcEnable&BLE_TRSPS_CBFC_TX_ENABLED) != 0U)
    {
        p_conn->localCredit--;
    }


    return TRSP_RES_SUCCESS;
}

void BLE_TRSPS_GetDataLength(GDBusProxy *p_proxyDev, uint16_t *p_dataLength)
{
    BLE_TRSPS_ConnList_T *p_conn = NULL;

    p_conn = ble_trsps_GetConnListByProxy(p_proxyDev);
    if (p_conn != NULL)
    {
        if ((p_conn->inputQueue.usedNum) > 0U)
        {
            *p_dataLength = p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].length;
        }
        else
        {
            *p_dataLength = 0;
        }
    }
    else
    {
        *p_dataLength = 0;
    }
}

uint16_t BLE_TRSPS_GetData(GDBusProxy *p_proxyDev, uint8_t *p_data)
{
    BLE_TRSPS_ConnList_T *p_conn = NULL;

    p_conn = ble_trsps_GetConnListByProxy(p_proxyDev);
    if (p_conn!= NULL)
    {
        if ((p_conn->inputQueue.usedNum) > 0U)
        {
            if (p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet != NULL)
            {
                (void)memcpy(p_data, p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet, 
                    p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].length);
                free(p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet);
                p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet = NULL;
            }
            
            p_conn->inputQueue.readIndex++;
            if (p_conn->inputQueue.readIndex >= BLE_TRSPS_INIT_CREDIT)
            {
                p_conn->inputQueue.readIndex = 0;
            }

            p_conn->inputQueue.usedNum --;
            
            if ((p_conn->cbfcEnable&BLE_TRSPS_CBFC_RX_ENABLED)!=0U)
            {
                p_conn->peerCredit++;
                if (p_conn->peerCredit >= BLE_TRSPS_MAX_RETURN_CREDIT)
                {
                    ble_trsps_ServerReturnCredit(p_conn);
                }
            }

            return TRSP_RES_SUCCESS;
        }
        else
        {
            return TRSP_RES_FAIL;
        }
    }
    else
    {
        return TRSP_RES_FAIL;
    }

}


static int ble_trsps_ParseValueArg(DBusMessageIter *p_iter, uint8_t **pp_value, int *p_len)
{
	DBusMessageIter array;

	if (dbus_message_iter_get_arg_type(p_iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	dbus_message_iter_recurse(p_iter, &array);
	dbus_message_iter_get_fixed_array(&array, pp_value, p_len);

	return 0;
}


static int ble_trsps_ParseOptions(DBusMessageIter *p_iter, uint16_t *p_offset, uint16_t *p_mtu,
						char **p_device, char **p_link,
						bool *p_prepAuthorize)
{
	DBusMessageIter dict;

	if (dbus_message_iter_get_arg_type(p_iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	dbus_message_iter_recurse(p_iter, &dict);

	while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY) {
		const char *p_key;
		DBusMessageIter value, entry;
		int var;

		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &p_key);

		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &value);

		var = dbus_message_iter_get_arg_type(&value);
		if (strcasecmp(p_key, "offset") == 0) {
			if (var != DBUS_TYPE_UINT16)
				return -EINVAL;
			if (p_offset)
				dbus_message_iter_get_basic(&value, p_offset);
		} else if (strcasecmp(p_key, "MTU") == 0) {
			if (var != DBUS_TYPE_UINT16)
				return -EINVAL;
			if (p_mtu)
				dbus_message_iter_get_basic(&value, p_mtu);
		} else if (strcasecmp(p_key, "device") == 0) {
			if (var != DBUS_TYPE_OBJECT_PATH)
				return -EINVAL;
			if (p_device)
				dbus_message_iter_get_basic(&value, p_device);
		} else if (strcasecmp(p_key, "link") == 0) {
			if (var != DBUS_TYPE_STRING)
				return -EINVAL;
			if (p_link)
				dbus_message_iter_get_basic(&value, p_link);
		} else if (strcasecmp(p_key, "prepare-authorize") == 0) {
			if (var != DBUS_TYPE_BOOLEAN)
				return -EINVAL;
			if (p_prepAuthorize) {
				int tmp;

				dbus_message_iter_get_basic(&value, &tmp);
				*p_prepAuthorize = !!tmp;
			}
		}

		dbus_message_iter_next(&dict);
	}

	return 0;
}


DBusMessage *BLE_TRSPS_ChrcStartNotifyTx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_Event_T evtPara;

    (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));
    s_trsState = BLE_TRSPS_STATUS_TX_OPENED;

    evtPara.eventId=BLE_TRSPS_EVT_TX_STATUS;
    evtPara.eventField.onTxStatus.status = s_trsState;
    if (bleTrspsProcess != NULL)
    {
        bleTrspsProcess(&evtPara);
    }

	return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);
}

DBusMessage *BLE_TRSPS_ChrcStopNotifyTx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_Event_T evtPara;

    (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));
    s_trsState = BLE_TRSPS_STATUS_TX_DISABLED;

    evtPara.eventId=BLE_TRSPS_EVT_TX_STATUS;
    evtPara.eventField.onTxStatus.status = s_trsState;
    if (bleTrspsProcess != NULL)
    {
        bleTrspsProcess(&evtPara);
    }

	return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);

}

static void ble_trsps_RxValue(BLE_TRSPS_ConnList_T *p_conn, uint16_t receivedLen, uint8_t *p_receivedValue)
{
    if (p_conn->inputQueue.usedNum < BLE_TRSPS_INIT_CREDIT)
    {
        BLE_TRSPS_Event_T evtPara;
        uint8_t *p_buffer = NULL;


        (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));
        p_buffer = malloc(receivedLen);
        
        if (p_buffer == NULL)
        {
            evtPara.eventId = BLE_TRSPS_EVT_ERR_NO_MEM;
            if (bleTrspsProcess != NULL)
            {
                bleTrspsProcess(&evtPara);
            }
            return;
        }

        (void)memcpy(p_buffer, p_receivedValue, receivedLen);
        p_conn->inputQueue.packetList[p_conn->inputQueue.writeIndex].length = receivedLen;
        p_conn->inputQueue.packetList[p_conn->inputQueue.writeIndex].p_packet = p_buffer;
        p_conn->inputQueue.writeIndex++;
        if (p_conn->inputQueue.writeIndex >= BLE_TRSPS_INIT_CREDIT)
        {
            p_conn->inputQueue.writeIndex = 0;
        }

        p_conn->inputQueue.usedNum++;

        evtPara.eventId=BLE_TRSPS_EVT_RECEIVE_DATA;
        evtPara.eventField.onReceiveData.p_dev = p_conn->p_dev;
        if (bleTrspsProcess != NULL)
        {
            bleTrspsProcess(&evtPara);
        }
    }

}


DBusMessage *BLE_TRSPS_ChrcWriteValueRx(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_ConnList_T *p_conn;

    char *p_device = NULL, *p_link = NULL;
    DBusMessageIter iter;
    int valueLen;
    uint8_t *p_value;
    uint16_t mtu;
    
    
    dbus_message_iter_init(p_msg, &iter);
    
    if (ble_trsps_ParseValueArg(&iter, &p_value, &valueLen))
        return g_dbus_create_error(p_msg,
                "org.bluez.Error.InvalidArguments", NULL);
    
    dbus_message_iter_next(&iter);
    if (ble_trsps_ParseOptions(&iter, NULL, &mtu, &p_device, &p_link,
                        NULL))
        return g_dbus_create_error(p_msg,
                "org.bluez.Error.InvalidArguments", NULL);
    
    p_conn = ble_trsps_GetConnListByObjPath(p_device);
    
    if (p_conn == NULL)
    {
        return g_dbus_create_error(p_msg, "org.bluez.Error.Failed", "0x80");
    }

    ble_trsps_RxValue(p_conn, valueLen, p_value);

	return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);
}

static void ble_trsps_CtrlValue(BLE_TRSPS_ConnList_T *p_conn, uint16_t length, uint8_t *p_value)
{
    BLE_TRSPS_Event_T evtPara;

    (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));

    switch (p_value[0])
    {
        case BLE_TRSPS_CBFC_OPCODE_SERVER_ENABLED:
        {
            p_conn->cbfcEnable |= BLE_TRSPS_CBFC_RX_ENABLED;
            p_conn->peerCredit = BLE_TRSPS_INIT_CREDIT;
            ble_trsps_ServerReturnCredit(p_conn);
            
            if (bleTrspsProcess != NULL)
            {
                evtPara.eventId=BLE_TRSPS_EVT_CBFC_ENABLED;
                evtPara.eventField.onCbfcEnabled.p_dev = p_conn->p_dev;
                bleTrspsProcess(&evtPara);
            }
        }
        break;

        case BLE_TRSPS_CBFC_OPCODE_GIVE_CREDIT:
        {
            p_conn->cbfcEnable |= BLE_TRSPS_CBFC_TX_ENABLED;
            p_conn->localCredit += p_value[1];
            
            if (bleTrspsProcess != NULL)
            {
                evtPara.eventId = BLE_TRSPS_EVT_CBFC_CREDIT;
                evtPara.eventField.onCbfcEnabled.p_dev = p_conn->p_dev;
                bleTrspsProcess(&evtPara);
            }
        }
        break;
        
        default:
        {
            if ((p_value[0] >= BLE_TRSPS_VENDOR_OPCODE_MIN) && (bleTrspsProcess!= NULL))
            {
                evtPara.eventId = BLE_TRSPS_EVT_VENDOR_CMD;
                evtPara.eventField.onVendorCmd.p_dev = p_conn->p_dev;
                evtPara.eventField.onVendorCmd.length = length;
                evtPara.eventField.onVendorCmd.p_payLoad = p_value;
                
                bleTrspsProcess(&evtPara);
            }
        }
        break;
    }
}


DBusMessage *BLE_TRSPS_ChrcWriteValueCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_ConnList_T *p_conn;

    char *p_device = NULL, *p_link = NULL;
    DBusMessageIter iter;
    int valueLen;
    uint8_t *p_value;
    uint16_t mtu;
    
    
    dbus_message_iter_init(p_msg, &iter);
    
    if (ble_trsps_ParseValueArg(&iter, &p_value, &valueLen))
        return g_dbus_create_error(p_msg,
                "org.bluez.Error.InvalidArguments", NULL);
    
    dbus_message_iter_next(&iter);
    if (ble_trsps_ParseOptions(&iter, NULL, &mtu, &p_device, &p_link,
                        NULL))
        return g_dbus_create_error(p_msg,
                "org.bluez.Error.InvalidArguments", NULL);


    p_conn = ble_trsps_GetConnListByObjPath(p_device);

    if (p_conn == NULL)
    {
        return g_dbus_create_error(p_msg, "org.bluez.Error.Failed", "0x80");
    }


    p_conn->attMtu = mtu;
    ble_trsps_CtrlValue(p_conn, valueLen, p_value);

	return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);
}

DBusMessage *BLE_TRSPS_ChrcStartNotifyCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_Event_T evtPara;


    if (bleTrspsProcess != NULL)
    {
        (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));

        evtPara.eventId=BLE_TRSPS_EVT_CTRL_STATUS;
        evtPara.eventField.onCtrlStatus.status = BLE_TRSPS_STATUS_CTRL_OPENED;

        bleTrspsProcess(&evtPara);
    }

    return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);
}

DBusMessage *BLE_TRSPS_ChrcStopNotifyCtrl(DBusConnection *p_dbusConn, DBusMessage *p_msg,
							void *p_userData)
{
    BLE_TRSPS_Event_T evtPara;


    if (bleTrspsProcess != NULL)
    {
        (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));

        evtPara.eventId=BLE_TRSPS_EVT_CTRL_STATUS;
        evtPara.eventField.onCtrlStatus.status = BLE_TRSPS_STATUS_CTRL_DISABLED;

        bleTrspsProcess(&evtPara);
    }

    return g_dbus_create_reply(p_msg, DBUS_TYPE_INVALID);
}


void BLE_TRSPS_DevConnected(GDBusProxy *p_proxyDev)
{
    BLE_TRSPS_ConnList_T    *p_conn;
    
    p_conn=ble_trsps_GetFreeConnList();
    if(p_conn==NULL)
    {
        BLE_TRSPS_Event_T evtPara;
        evtPara.eventId = BLE_TRSPS_EVT_ERR_UNSPECIFIED;
        if (bleTrspsProcess)
        {
            bleTrspsProcess(&evtPara);
        }
        return;
    }

    p_conn->p_dev=p_proxyDev;

}

void BLE_TRSPS_DevDisconnected(GDBusProxy *p_proxyDev)
{
    BLE_TRSPS_ConnList_T    *p_conn;

    p_conn = ble_trsps_GetConnListByProxy(p_proxyDev);

    if (p_conn != NULL)
    {
        // Flush all queued data.
        while (p_conn->inputQueue.usedNum > 0U)
        {
            if (p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet != NULL)
            {
                free(p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet);
                p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet = NULL;
            }
    
            p_conn->inputQueue.readIndex++;
            if (p_conn->inputQueue.readIndex >= BLE_TRSPS_INIT_CREDIT)
            {
                p_conn->inputQueue.readIndex = 0;
            }
    
            p_conn->inputQueue.usedNum --;
        }

        ble_trsps_InitConnList(p_conn);
    }

}

