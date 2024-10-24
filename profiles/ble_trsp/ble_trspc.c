/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BLE Transparent Client Profile Source File

  Company:
    Microchip Technology Inc.

  File Name:
    ble_trspc.c

  Summary:
    This file contains the BLE Transparent Client functions for application user.

  Description:
    This file contains the BLE Transparent Client functions for application user.
 *******************************************************************************/


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/uio.h>

#include "shared/att-types.h"
#include "shared/util.h"


#include "ble_trspc.h"
#include "ble_trsp_defs.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

/**@defgroup BLE_TRSPC_CBFC_OPCODE BLE_TRSPC_CBFC_OPCODE
 * @brief The definition of BLE transparent credit based flow control
 * @{ */
#define BLE_TRSPC_CBFC_OPCODE_SUCCESS           (0x00U)    /**< Definition of response for successful operation. */
#define BLE_TRSPC_CBFC_OPCODE_DL_ENABLED        (0x14U)    /**< Definition of Op Code for Credit Based Flow Control Protocol, Enable CBFC downlink. */
#define BLE_TRSPC_CBFC_OPCODE_UL_ENABLED        (0x15U)    /**< Definition of Op Code for Credit Based Flow Control Protocol: Enable CBFC uplink. */
/** @} */

/**@defgroup BLE_TRSPC_VENDOR_OPCODE BLE_TRSPC_VENDOR_OPCODE
 * @brief The definition of BLE transparent vendor opcodes
 * @{ */
#define BLE_TRSPC_VENDOR_OPCODE_MIN             (0x20U)    /**< Definition of Op Code range in TRS vendor commands. */
#define BLE_TRSPC_VENDOR_OPCODE_MAX             (0xFFU)    /**< Definition of Op Code range in TRS vendor commands. */
/** @} */

/**@defgroup BLE_TRSPC_INIT_CREDIT BLE_TRSPC_INIT_CREDIT
 * @brief The definition of initial credit value.
 * @{ */
#define BLE_TRSPC_INIT_CREDIT                   (0x10U)     /**< Definition of initial credit */
/** @} */

/**@defgroup BLE_TRSPC_MAX_RETURN_CREDIT BLE_TRSPC_MAX_RETURN_CREDIT
 * @brief The definition of maximum return credit number.
 * @{ */
#define BLE_TRSPC_MAX_RETURN_CREDIT             (13U)     /**< Maximum return credit number */
/** @} */

/**@defgroup BLE_TRSPC_CBFC_PROC BLE_TRSPC_CBFC_PROC
 * @brief The definition of CBFC procedure in connect/disconnect process.
 * @{ */
#define CBFC_PROC_IDLE                          (0x00U)    /**< CBFC procdure: Idle. */
#define CBFC_PROC_ENABLE_SESSION                (0x01U)    /**< CBFC procdure: Enable Control Point CCCD. */
#define CBFC_PROC_ENABLE_TCP_CCCD               (0x02U)    /**< CBFC procdure: Enable TCP CCCD. */
#define CBFC_PROC_ENABLE_TDD_CBFC               (0x03U)    /**< CBFC procdure: Enable TDD CBFC. */
#define CBFC_PROC_ENABLE_TUD_CCCD               (0x04U)    /**< CBFC procdure: Enable TUD CCCD. */
#define CBFC_PROC_DISABLE_TUD_CCCD              (0x05U)    /**< CBFC procdure: Disable TUD CCCD. */
/** @} */

/**@defgroup BLE_TRSPC_VENCOM_PROC BLE_TRSPC_VENCOM_PROC
 * @brief The definition of vendor command response procedure.
 * @{ */
#define VENCOM_PROC_IDLE                        (0x00U)    /**< Vendor command response procdure: Idle. */
#define VENCOM_PROC_ENABLE                      (0x01U)    /**< Vendor command response procdure: Enable. */
/** @} */

/**@defgroup BLE_TRSPC_CBFC_CONFIG BLE_TRSPC_CBFC_CONFIG
 * @brief The definition of credit base flow control configuration.
 * @{ */
#define BLE_TRSPC_CBFC_DISABLED                 (0x00U)    /**< Definition of ble transparent service credit based downlink/uplink disable. */
#define BLE_TRSPC_CBFC_DL_ENABLED               (0x01U)    /**< Definition of ble transparent service credit based downlink enable. */
#define BLE_TRSPC_CBFC_UL_ENABLED               (0x02U)    /**< Definition of ble transparent service credit based uplink enable. */
/** @} */

/**@defgroup BLE_TRSPC_CP_STATUS BLE_TRSPC_CP_STATUS
 * @brief The definition of BLE transparent service control point status.
 * @{ */
#define BLE_TRSPC_CP_STATUS_DISABLED            (0x00U)    /**< Transparent control point CCCD is disabled. */
#define BLE_TRSPC_CP_STATUS_ENABLED             (0x80U)    /**< Transparent control point CCCD is enabled (Notify). */
/** @} */

/**@defgroup BLE_TRSPC_UUID BLE_TRSPC_UUID
 * @brief The definition of UUID in TRS
 * @{ */
#define UUID_MCHP_TRANS_SVC                     "49535343-fe7d-4ae5-8fa9-9fafd205e455"      /**< Definition of MCHP proprietary service UUID. */
#define UUID_MCHP_CHAR_TUD                      "49535343-1e4d-4bd9-ba61-23c647249616"      /**< Definition of MCHP Transparent TUD characteristic UUID. */
#define UUID_MCHP_CHAR_TDD                      "49535343-8841-43f4-a8d4-ecbe34729bb3"      /**< Definition of MCHP Transparent TDD characteristic UUID. */
#define UUID_MCHP_CHAR_TCP                      "49535343-4c8a-39b3-2f49-511cff073b7e"      /**< Definition of MCHP Transparent TCP characteristic UUID. */
/** @} */

/**@defgroup BLE_TRSPC_MD_CALLER BLE_TRSPC_MD_CALLER
 * @brief The definition of method callers.
 * @{ */
#define BLE_TRSPC_MD_CALLER_NONE                          (0x00U)    /**< Not assigned. */
#define BLE_TRSPC_MD_CALLER_CBFC                          (0x01U)    /**< Profile CBFC procedure. */
#define BLE_TRSPC_MD_CALLER_CREDIT                        (0x02U)    /**< Profile client return credit. */
#define BLE_TRSPC_MD_CALLER_VERDOR_CMD                    (0x03U)    /**< Application send vendor command. */
#define BLE_TRSPC_MD_CALLER_DATA                          (0x04U)    /**< Application send data. */
/** @} */


/**@brief Enumeration type of BLE transparent profile characteristics. */
typedef enum BLE_TRSPC_CharIndex_T
{
    TRSPC_INDEX_CHARTUD = 0x00U,        /**< Index of transparent transparent Uplink Data Characteristic. */
    TRSPC_INDEX_CHARTDD,                /**< Index of transparent transparent Downlink Data Characteristic. */
    TRSPC_INDEX_CHARTCP,                /**< Index of transparent Control Point characteristic. */
    TRSPC_CHAR_NUM                      /**< Total number of TRS characteristics. */
}BLE_TRSPC_CharIndex_T;

/**@defgroup BLE_TRSPC_STATE TRSPC state
 * @brief The definition of BLE TRSPC connection state
 * @{ */
typedef enum BLE_TRSPC_State_T
{
    BLE_TRSPC_STATE_IDLE = 0x00,        /**< Default state (Disconnected). */
    BLE_TRSPC_STATE_CONNECTED           /**< Connected. */
} BLE_TRSPC_State_T;
/** @} */

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

/**@brief The structure contains information about BLE transparent profile packetIn. */
typedef struct BLE_TRSPC_PacketList_T
{
    uint16_t                   length;                  /**< Data length. */
    uint8_t                    *p_packet;               /**< Pointer to the TX/RX data buffer */
} BLE_TRSPC_PacketList_T;

/**@brief The structure contains information about packet input queue format of BLE transparent profile. */
typedef struct BLE_TRSPC_QueueIn_T
{
    uint8_t                    usedNum;                    /**< The number of data list of packetIn buffer. */
    uint8_t                    writeIndex;                 /**< The Index of data, written in packet buffer. */
    uint8_t                    readIndex;                  /**< The Index of data, read in packet buffer. */
    BLE_TRSPC_PacketList_T     packetList[BLE_TRSPC_INIT_CREDIT];  /**< Written in packet buffer. @ref BLE_TRSPC_PacketList_T.*/  
} BLE_TRSPC_QueueIn_T;

/**@brief The structure contains information about BLE transparent profile connection parameters for recording connection information. */
typedef struct BLE_TRSPC_ConnList_T
{
    uint8_t                     trspState;              /**< BLE transparent profile current state. @ref BLE_TRSPC_STATUS.*/
    GDBusProxy                  *p_dev;                 /**< Proxy to org.bluez.device interface. */
    GDBusProxy                  *chrc[TRSPC_CHAR_NUM];  /**< Proxies to org.bluez.charateristic1 interface. */
    uint16_t                    attMtu;                 /**< Record the current connection MTU size. */
    uint8_t                     cbfcConfig;             /**< Credit based flow control usage of uplink/downlink. */
    uint8_t                     cbfcProcedure;          /**< Record ongoing credit based flow control configuration procedure. */
    uint8_t                     localCredit;            /**< Credit number of TRS downlink (Client to Server). */
    uint8_t                     peerCredit;             /**< Credit number of TRS uplink (Server to Client). */
    uint8_t                     vendorCmdProc;          /**< Vendor command reponse procedure. */
    BLE_TRSPC_QueueIn_T         inputQueue;             /**< Input queue to store Rx packets. */
    uint8_t                     cbfcRetryProcedure;     /**< Record credit based flow control configuration procedure for retry used. */
    BLE_TRSPC_State_T           state;                  /**< Connection state. */
    uint8_t                     retryCnt;               /**< Retry counter. */
    uint8_t                     updatingPeerCredit;     /**< The updating peer credit. If updatingPeerCredit > 0, the client return credit procedure is in progress */
} BLE_TRSPC_ConnList_T;

typedef struct BLE_TRSPC_MethodData_T
{
    struct iovec                iov;                    /**< Write value data. */
    char                        *p_type;                /**< Write command type. */
    BLE_TRSPC_ConnList_T        *p_conn;                /**< Connection associated with this method call. */
    uint8_t                     caller;                 /**< Which one call this method. */
}BLE_TRSPC_MethodData_T;

typedef struct BLE_TRSPC_RetryData_T
{
    BLE_TRSPC_ConnList_T        *p_conn;                /**< Connection associated with this method call. */
    uint8_t                     caller;                 /**< Which one call this method. */
}BLE_TRSPC_RetryData_T;


typedef struct BLE_TRSPC_ProxyCache_T
{
    GList                    *p_svc;                    /**< Proxy list of org.bluez.service1 interface. */
    GList                    *chrc[TRSPC_CHAR_NUM];     /**< Proxy list of org.bluez.charateristic1 interface. */
}BLE_TRSPC_ProxyCache_T;


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static BLE_TRSPC_EventCb_T      bleTrspcProcess;
static BLE_TRSPC_ConnList_T     s_trspcConnList[BLE_TRSPC_MAX_CONN_NBR];

static BLE_TRSPC_ProxyCache_T   s_trspcCache;

static void ble_trspc_WriteReply(DBusMessage *p_message, void *p_userData);
static void ble_trspc_WriteSetup(DBusMessageIter *p_iter, void *p_userData);

static void ble_trspc_ConveyErrEvt(BLE_TRSPC_EventId_T evtId)
{
    if (bleTrspcProcess != NULL)
    {
        BLE_TRSPC_Event_T evtPara;
    
        evtPara.eventId = evtId;
        bleTrspcProcess(&evtPara);
    }
}

static void ble_trspc_ProcVendorCmdReply(BLE_TRSPC_ConnList_T *p_conn, uint8_t result)
{
    if (p_conn->vendorCmdProc != VENCOM_PROC_IDLE)
    {
        p_conn->vendorCmdProc = VENCOM_PROC_IDLE;
        if (bleTrspcProcess != NULL)
        {
            BLE_TRSPC_Event_T evtPara;
        
            evtPara.eventId = BLE_TRSPC_EVT_VENDOR_CMD_RSP;
            evtPara.eventField.onVendorCmdRsp.p_dev = p_conn->p_dev;
            evtPara.eventField.onVendorCmdRsp.result = result;
            bleTrspcProcess(&evtPara);
        }
    }
}

static void ble_trspc_ProcDataReply(BLE_TRSPC_ConnList_T *p_conn, uint8_t result)
{
    if (result != BLE_TRSPC_SEND_RESULT_SUCCESS 
        && (p_conn->trspState & BLE_TRSPC_DL_STATUS_CBFCENABLED) != 0U)
    {
        p_conn->localCredit++;
    }

    if (bleTrspcProcess != NULL)
    {
        BLE_TRSPC_Event_T evtPara;
    
        evtPara.eventId = BLE_TRSPC_EVT_DATA_RSP;
        evtPara.eventField.onDataRsp.p_dev = p_conn->p_dev;
        evtPara.eventField.onDataRsp.result = result;
        bleTrspcProcess(&evtPara);
    }
}


static void ble_trspc_InitConnList(BLE_TRSPC_ConnList_T *p_conn)
{
    (void)memset((uint8_t *)p_conn, 0, sizeof(BLE_TRSPC_ConnList_T));
    p_conn->attMtu= BT_ATT_DEFAULT_LE_MTU;
}

static BLE_TRSPC_ConnList_T *ble_trspc_GetConnListByProxy(GDBusProxy *p_dev)
{
    uint8_t i;

    for(i=0; i<BLE_TRSPC_MAX_CONN_NBR; i++)
    {
        if ((s_trspcConnList[i].state == BLE_TRSPC_STATE_CONNECTED) && (s_trspcConnList[i].p_dev == p_dev))
        {
            return &s_trspcConnList[i];
        }
    }

    return NULL;
}

static BLE_TRSPC_ConnList_T *ble_trspc_GetFreeConnList(void)
{
    uint8_t i;

    for(i=0; i<BLE_TRSPC_MAX_CONN_NBR; i++)
    {
        if (s_trspcConnList[i].state == BLE_TRSPC_STATE_IDLE)
        {
            s_trspcConnList[i].state = BLE_TRSPC_STATE_CONNECTED;
            return &s_trspcConnList[i];
        }
    }

    return NULL;
}




static void ble_trspc_EnableDownlinkCreditBaseFlowControl(BLE_TRSPC_ConnList_T *p_conn)
{
    BLE_TRSPC_MethodData_T *p_data;
    char charValue[1];
    
	p_data = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_data == NULL)
	{
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_NO_MEM);
		return;
	}

    
    charValue[0] = BLE_TRSPC_CBFC_OPCODE_DL_ENABLED;
    
    p_data->p_conn = p_conn;
    p_data->iov.iov_base = charValue;
    p_data->iov.iov_len = 1;
    p_data->p_type = "request";
    p_data->caller = BLE_TRSPC_MD_CALLER_CBFC;
    
    if (g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTCP], "WriteValue", ble_trspc_WriteSetup, ble_trspc_WriteReply, p_data, NULL))
    {
        p_conn->cbfcProcedure = CBFC_PROC_ENABLE_TDD_CBFC;
        p_conn->cbfcRetryProcedure = CBFC_PROC_ENABLE_TCP_CCCD;
    }
    else
    {
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
    }
}

static void ble_trspc_EnableControlPointCccd(BLE_TRSPC_ConnList_T *p_conn)
{
    BLE_TRSPC_MethodData_T *p_data;
    
	p_data = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_data == NULL)
	{
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_NO_MEM);
		return;
	}

    
    p_data->p_conn = p_conn;
    p_data->caller = BLE_TRSPC_MD_CALLER_CBFC;
    
    if (g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTCP], "StartNotify", NULL, ble_trspc_WriteReply, p_data, NULL))
    {
        p_conn->cbfcProcedure = CBFC_PROC_ENABLE_TCP_CCCD;
        p_conn->cbfcRetryProcedure = CBFC_PROC_ENABLE_SESSION;
    }
    else
    {
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
    }

}

static void ble_trspc_ClientReturnCredit(BLE_TRSPC_ConnList_T *p_conn)
{
    BLE_TRSPC_MethodData_T *p_data;
    char charValue[2];

    if (p_conn->updatingPeerCredit > 0)
    {
        return;
    }


	p_data = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_data == NULL)
	{
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_NO_MEM);
		return;
	}

    
    p_data->p_conn = p_conn;

    
    charValue[0] = BLE_TRSPC_CBFC_OPCODE_UL_ENABLED;
    charValue[1] = p_conn->peerCredit;
    p_data->p_conn = p_conn;
    p_data->iov.iov_base = charValue;
    p_data->iov.iov_len = 2;
    p_data->p_type = "request";
    p_data->caller = BLE_TRSPC_MD_CALLER_CREDIT;
    
    if (!g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTCP], "WriteValue", ble_trspc_WriteSetup, ble_trspc_WriteReply, p_data, NULL))
    {
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
    }
    else
    {
        p_conn->updatingPeerCredit = p_conn->peerCredit;
    }
}

static void ble_trspc_ConfigureUplinkDataCccd(BLE_TRSPC_ConnList_T *p_conn, bool enable)
{

    BLE_TRSPC_MethodData_T *p_data;
    const char *p_method;

	if (enable == true)
	{
		p_method = "StartNotify";
	}
	else
	{
		p_method = "StopNotify";
	}

	p_data = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_data == NULL)
	{
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_NO_MEM);
		return;
	}

    p_data->p_conn = p_conn;
    p_data->caller = BLE_TRSPC_MD_CALLER_CBFC;

    if (g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTUD], p_method, NULL, ble_trspc_WriteReply, p_data, NULL))
    {
        if (enable)
        {
            p_conn->cbfcProcedure = CBFC_PROC_ENABLE_TUD_CCCD;
        }
        else
        {
            p_conn->cbfcProcedure = CBFC_PROC_DISABLE_TUD_CCCD;
        }

        p_conn->cbfcRetryProcedure = CBFC_PROC_ENABLE_TDD_CBFC;
    }
    else
    {
        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
    }
}

static void ble_trspc_RcvData(BLE_TRSPC_ConnList_T *p_conn, uint16_t receivedLen, uint8_t *p_receivedValue)
{
    if (p_conn->inputQueue.usedNum < (uint8_t)BLE_TRSPC_INIT_CREDIT)
    {
        BLE_TRSPC_Event_T evtPara;
        uint8_t *p_buffer = NULL;

        (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));
        p_buffer = g_malloc(receivedLen);

        if (p_buffer == NULL)
        {
            evtPara.eventId = BLE_TRSPC_EVT_ERR_NO_MEM;
            if (bleTrspcProcess != NULL)
            {
                bleTrspcProcess(&evtPara);
            }
            return;
        }

        (void)memcpy(p_buffer, p_receivedValue, receivedLen);
        p_conn->inputQueue.packetList[p_conn->inputQueue.writeIndex].length = receivedLen;
        p_conn->inputQueue.packetList[p_conn->inputQueue.writeIndex].p_packet = p_buffer;
        p_conn->inputQueue.writeIndex++;
        if (p_conn->inputQueue.writeIndex >= BLE_TRSPC_INIT_CREDIT)
        {
            p_conn->inputQueue.writeIndex = 0;
        }

        p_conn->inputQueue.usedNum++;

        evtPara.eventId = BLE_TRSPC_EVT_RECEIVE_DATA;
        evtPara.eventField.onReceiveData.p_dev = p_conn->p_dev;
        if (bleTrspcProcess != NULL)
        {
            bleTrspcProcess(&evtPara);
        }
    }
}

static void ble_trspc_RcvCtrlData(BLE_TRSPC_ConnList_T *p_conn, uint16_t len, uint8_t *p_value)
{
    BLE_TRSPC_Event_T evtPara;

    /* Opcode: response */
    if (p_value[0] == BLE_TRSPC_CBFC_OPCODE_SUCCESS)
    {
        /* Request opcode of response */
        if (p_value[1] == BLE_TRSPC_CBFC_OPCODE_DL_ENABLED)
        {
            p_conn->trspState |= BLE_TRSPC_DL_STATUS_CBFCENABLED;
            p_conn->attMtu = get_be16(&p_value[2]);
            p_conn->localCredit += p_value[4];

            if (bleTrspcProcess != NULL)
            {
                evtPara.eventId = BLE_TRSPC_EVT_DL_STATUS;
                evtPara.eventField.onDownlinkStatus.p_dev = p_conn->p_dev;
                evtPara.eventField.onDownlinkStatus.status = BLE_TRSPC_DL_STATUS_CBFCENABLED;
                evtPara.eventField.onDownlinkStatus.currentCreditNumber = p_conn->localCredit;
                bleTrspcProcess(&evtPara);
            }
        }
    }
    else if (p_value[0]>=BLE_TRSPC_VENDOR_OPCODE_MIN)
    {
        if (bleTrspcProcess != NULL)
        {
            evtPara.eventId = BLE_TRSPC_EVT_VENDOR_CMD;
            evtPara.eventField.onVendorCmd.p_dev = p_conn->p_dev;
            evtPara.eventField.onVendorCmd.payloadLength = (uint8_t)len;
            evtPara.eventField.onVendorCmd.p_payLoad = p_value;

            bleTrspcProcess(&evtPara);
        }
    }
    else
    {
        //Shall not enter here
    }
}


static void ble_trspc_ProcCbfcReply(BLE_TRSPC_ConnList_T *p_conn)
{
    BLE_TRSPC_Event_T evtPara;

    (void)memset((uint8_t *) &evtPara, 0, sizeof(evtPara));

    p_conn->retryCnt = 0;

    switch (p_conn->cbfcProcedure)
    {
        case CBFC_PROC_ENABLE_SESSION:
        {
            ble_trspc_EnableControlPointCccd(p_conn);
        }
        break;
        case CBFC_PROC_ENABLE_TCP_CCCD:
        {
            p_conn->cbfcProcedure = CBFC_PROC_IDLE;
            p_conn->cbfcRetryProcedure = CBFC_PROC_IDLE;
            p_conn->trspState |= BLE_TRSPC_CP_STATUS_ENABLED;
            ble_trspc_EnableDownlinkCreditBaseFlowControl(p_conn);
        }
        break;

        case CBFC_PROC_ENABLE_TDD_CBFC:
        {
            p_conn->cbfcProcedure = CBFC_PROC_IDLE;
            p_conn->cbfcRetryProcedure = CBFC_PROC_IDLE;
            if ((p_conn->cbfcConfig&BLE_TRSPC_CBFC_UL_ENABLED) != 0U)
            {
                ble_trspc_ConfigureUplinkDataCccd(p_conn, true);
            }
        }
        break;

        case CBFC_PROC_ENABLE_TUD_CCCD:
        {
            p_conn->cbfcProcedure = CBFC_PROC_IDLE;
            p_conn->cbfcRetryProcedure = CBFC_PROC_IDLE;
            p_conn->trspState |= BLE_TRSPC_UL_STATUS_CBFCENABLED;
            /* Initialize and give credits after connected */
            p_conn->peerCredit = BLE_TRSPC_INIT_CREDIT;
            ble_trspc_ClientReturnCredit(p_conn);

            if (bleTrspcProcess != NULL)
            {
                evtPara.eventId = BLE_TRSPC_EVT_UL_STATUS;
                evtPara.eventField.onUplinkStatus.p_dev = p_conn->p_dev;
                evtPara.eventField.onUplinkStatus.status = BLE_TRSPC_UL_STATUS_CBFCENABLED;
                bleTrspcProcess(&evtPara);
            }
        }
        break;

        case CBFC_PROC_DISABLE_TUD_CCCD:
        {
            p_conn->cbfcProcedure = CBFC_PROC_IDLE;
            p_conn->trspState &= (uint8_t)(~BLE_TRSPC_UL_STATUS_CBFCENABLED);
            if (bleTrspcProcess != NULL)
            {
                evtPara.eventId = BLE_TRSPC_EVT_UL_STATUS;
                evtPara.eventField.onUplinkStatus.p_dev = p_conn->p_dev;
                evtPara.eventField.onUplinkStatus.status = BLE_TRSPC_UL_STATUS_DISABLED;
                bleTrspcProcess(&evtPara);
            }

            /* Reset credit */
            p_conn->peerCredit = 0;
        }
        break;
        default:
        {
        }
        break;
    }
}


static void ble_trspc_ProcGattWriteResp(BLE_TRSPC_ConnList_T *p_conn, uint8_t caller)
{
    switch (caller)
    {
        case BLE_TRSPC_MD_CALLER_CBFC:
        {
            ble_trspc_ProcCbfcReply(p_conn);
        }
        break;
        case BLE_TRSPC_MD_CALLER_CREDIT:
        {
            p_conn->retryCnt = 0;

            if (p_conn->updatingPeerCredit <= p_conn->peerCredit)
            {
                p_conn->peerCredit -= p_conn->updatingPeerCredit;
                p_conn->updatingPeerCredit = 0;

                if (p_conn->peerCredit >= BLE_TRSPC_MAX_RETURN_CREDIT)
                {
                    ble_trspc_ClientReturnCredit(p_conn);
                }
            }
            else /* should not be here */
            {
                ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
            }
        }
        break;
        case BLE_TRSPC_MD_CALLER_VERDOR_CMD:
        {
            ble_trspc_ProcVendorCmdReply(p_conn, BLE_TRSPC_SEND_RESULT_SUCCESS);
        }
        break;
        case BLE_TRSPC_MD_CALLER_DATA:
        {
            ble_trspc_ProcDataReply(p_conn, BLE_TRSPC_SEND_RESULT_SUCCESS);
        }
        break;
        default:
        {
        }
        break;
    }
}

static void ble_trspc_WriteSetup(DBusMessageIter *p_iter, void *p_userData)
{
	BLE_TRSPC_MethodData_T *p_data = p_userData;
	DBusMessageIter array, dict;

	dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY, "y", &array);
	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&p_data->iov.iov_base,
						p_data->iov.iov_len);
	dbus_message_iter_close_container(p_iter, &array);

	dbus_message_iter_open_container(p_iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&dict);

	if (p_data->p_type)
	{
		g_dbus_dict_append_entry(&dict, "type", DBUS_TYPE_STRING,
								&p_data->p_type);
	}

	dbus_message_iter_close_container(p_iter, &dict);

}

static gboolean ble_trspc_RetryTmr(gpointer arg)
{
    BLE_TRSPC_RetryData_T *p_retry;

    
    p_retry = (BLE_TRSPC_RetryData_T *) arg;
    if (p_retry->p_conn->state == BLE_TRSPC_STATE_CONNECTED) 
    {
        if (p_retry->p_conn->cbfcRetryProcedure!=CBFC_PROC_IDLE)
        {
            p_retry->p_conn->cbfcProcedure = p_retry->p_conn->cbfcRetryProcedure;
            p_retry->p_conn->cbfcRetryProcedure = CBFC_PROC_IDLE;
            ble_trspc_ProcGattWriteResp(p_retry->p_conn, p_retry->caller);
        }
        else
        {
            p_retry->p_conn->updatingPeerCredit = 0;
            ble_trspc_ClientReturnCredit(p_retry->p_conn);
        }
    }

    g_free(p_retry);
    
    return FALSE;
}


static void ble_trspc_WriteReply(DBusMessage *p_message, void *p_userData)
{
    BLE_TRSPC_MethodData_T *p_mdData = p_userData;
	DBusError error;
    BLE_TRSPC_ConnList_T *p_conn;
    uint8_t mdCaller;
    
    p_conn = p_mdData->p_conn;
    mdCaller = p_mdData->caller;
    g_free(p_mdData);

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, p_message) == TRUE) 
    {
        if (strcmp(error.name, "org.bluez.Error.InProgress") == 0)
        {
            switch(mdCaller)
            {
                case BLE_TRSPC_MD_CALLER_CBFC:
                case BLE_TRSPC_MD_CALLER_CREDIT:
                {
                    if (p_conn->retryCnt++ < BLE_TRSPC_RETRY_MAX_NUMBER)
                    {
                        BLE_TRSPC_RetryData_T *p_retry = g_new(BLE_TRSPC_RetryData_T, 1);
                        if (p_retry)
                        {
                            p_retry->p_conn = p_conn;
                            p_retry->caller = mdCaller;
                            
                            g_timeout_add(BLE_TRSPC_RETRY_TIMEOUT, ble_trspc_RetryTmr, p_retry);
                        }
                        else
                        {
                            ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_NO_MEM);
                        }
                    }
                    else
                    {
                        ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
                    }
                }
                break;
                case BLE_TRSPC_MD_CALLER_VERDOR_CMD:
                {
                    ble_trspc_ProcVendorCmdReply(p_conn, BLE_TRSPC_SEND_RESULT_BUSY);
                }
                break;
                case BLE_TRSPC_MD_CALLER_DATA:
                {
                    ble_trspc_ProcDataReply(p_conn, BLE_TRSPC_SEND_RESULT_BUSY);
                }
                break;
                default:
                {
                    // should not be here
                }
                break;
            }
        }
        else
        {
            if (mdCaller == BLE_TRSPC_MD_CALLER_VERDOR_CMD)
            {
                ble_trspc_ProcVendorCmdReply(p_conn, BLE_TRSPC_SEND_RESULT_FAILED);
            }
            else if (mdCaller == BLE_TRSPC_MD_CALLER_DATA)
            {
                ble_trspc_ProcDataReply(p_conn, BLE_TRSPC_SEND_RESULT_FAILED);
            }
            else
            {
                p_conn->cbfcRetryProcedure = CBFC_PROC_IDLE;
                p_conn->retryCnt = 0;
                p_conn->updatingPeerCredit = 0;

                ble_trspc_ConveyErrEvt(BLE_TRSPC_EVT_ERR_UNSPECIFIED);
            }
        }

		dbus_error_free(&error);
		return;
	}

    ble_trspc_ProcGattWriteResp(p_conn, mdCaller);
}



static void ble_trspc_EnableDataSession(BLE_TRSPC_ConnList_T *p_conn, uint8_t cbfcConfig)
{
    if (p_conn != NULL)
    {
        p_conn->cbfcConfig |= cbfcConfig;

        if ((p_conn->cbfcConfig&BLE_TRSPC_CBFC_DL_ENABLED)!= 0U)
        {
            ble_trspc_EnableControlPointCccd(p_conn);
        }
        else
        {
            p_conn->trspState |= BLE_TRSPC_DL_STATUS_NONCBFCENABLED;
            if ((p_conn->cbfcConfig&BLE_TRSPC_CBFC_UL_ENABLED)!= 0U)
            {
                ble_trspc_ConfigureUplinkDataCccd(p_conn, true);
            }
        }
    }
}

static void ble_trspc_GetChrcProxyBySvcPath(BLE_TRSPC_ConnList_T *p_conn, const char *p_targetPath)
{
    uint8_t idx;
    GList *p_list;
    DBusMessageIter iter;
    const char *p_svc;

    for (idx = TRSPC_INDEX_CHARTUD; idx < TRSPC_CHAR_NUM; idx++)
    {
        for (p_list = g_list_first(s_trspcCache.chrc[idx]); p_list; p_list = g_list_next(p_list)) 
        {
            if (g_dbus_proxy_get_property(p_list->data, "Service", &iter) == FALSE)
                continue;
            
            dbus_message_iter_get_basic(&iter, &p_svc);

            if (g_str_equal(p_svc, p_targetPath) == TRUE)
            {
                p_conn->chrc[idx] = p_list->data;
                break;
            }

        }

        if (p_conn->chrc[idx] == NULL)
        {
            return;
        }
    }
}


static bool ble_trspc_GetProxyCache(BLE_TRSPC_ConnList_T *p_conn)
{
    GList *p_list;
    DBusMessageIter iter;
    const char *p_devPath;

	for (p_list = g_list_first(s_trspcCache.p_svc); p_list; p_list = g_list_next(p_list)) 
    {
        if (g_dbus_proxy_get_property(p_list->data, "Device", &iter) == FALSE)
            continue;
        
        dbus_message_iter_get_basic(&iter, &p_devPath);

        if (g_str_equal(p_devPath, g_dbus_proxy_get_path(p_conn->p_dev)) == TRUE)
        {

            ble_trspc_GetChrcProxyBySvcPath(p_conn, g_dbus_proxy_get_path(p_list->data));

            if (p_conn->chrc[TRSPC_CHAR_NUM - 1] != NULL)
            {
                return true;
            }
        }
	}

    return false;
}

void BLE_TRSPC_EventRegister(BLE_TRSPC_EventCb_T bleTranCliHandler)
{
    bleTrspcProcess = bleTranCliHandler;
}

void BLE_TRSPC_Init(void)
{
    uint8_t i;

    /* Reset connection information */
    for (i = 0; i < BLE_TRSPC_MAX_CONN_NBR; i++)
    {
        ble_trspc_InitConnList(&s_trspcConnList[i]);
    }

    (void)memset(&s_trspcCache, 0x00, sizeof(s_trspcCache));
}

void BLE_TRSPC_DevConnected(GDBusProxy *p_proxyDev)
{
    BLE_TRSPC_ConnList_T    *p_conn;

    p_conn=ble_trspc_GetFreeConnList();
    if(p_conn!=NULL)
    {
        p_conn->p_dev=p_proxyDev;
    }
}

void BLE_TRSPC_DevDisconnected(GDBusProxy *p_proxyDev)
{
    BLE_TRSPC_ConnList_T    *p_conn;

    p_conn = ble_trspc_GetConnListByProxy(p_proxyDev);

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
            if (p_conn->inputQueue.readIndex >= BLE_TRSPC_INIT_CREDIT)
            {
                p_conn->inputQueue.readIndex = 0;
            }
        
            p_conn->inputQueue.usedNum--;
        }
        ble_trspc_InitConnList(p_conn);
    }

}


void BLE_TRSPC_ProxyAddHandler(GDBusProxy *p_proxy)
{
    const char *p_interface;
    DBusMessageIter iter;
    char *p_objPath, *p_uuid;
    uint8_t idx;

    p_interface = g_dbus_proxy_get_interface(p_proxy);

    if (!strcmp(p_interface, "org.bluez.GattService1")) 
    {
        if (g_dbus_proxy_get_property(p_proxy, "UUID", &iter) == FALSE)
        {
            return;
        }
        
        dbus_message_iter_get_basic(&iter, &p_uuid);

        if (!strcmp(p_uuid, UUID_MCHP_TRANS_SVC))
        {
            s_trspcCache.p_svc = g_list_append(s_trspcCache.p_svc, p_proxy);
        }
    } 
    else if (!strcmp(p_interface, "org.bluez.GattCharacteristic1")) 
    {
        if (g_dbus_proxy_get_property(p_proxy, "Service", &iter) == FALSE)
        {
            return;
        }
        
        dbus_message_iter_get_basic(&iter, &p_objPath);

        if (g_dbus_proxy_lookup(s_trspcCache.p_svc, NULL, p_objPath, "org.bluez.GattService1") == NULL)
        {
            return;
        }
        
        if (g_dbus_proxy_get_property(p_proxy, "UUID", &iter) == FALSE)
        {
            return;
        }
    
        dbus_message_iter_get_basic(&iter, &p_uuid);
    
        if (!strcmp(p_uuid, UUID_MCHP_CHAR_TUD))
        {
            idx = TRSPC_INDEX_CHARTUD;
        }
        else if (!strcmp(p_uuid, UUID_MCHP_CHAR_TDD))
        {
            idx = TRSPC_INDEX_CHARTDD;
        }
        else if (!strcmp(p_uuid, UUID_MCHP_CHAR_TCP))
        {
            idx = TRSPC_INDEX_CHARTCP;
        }
        else
        {
            return;
        }

        s_trspcCache.chrc[idx] = g_list_append(s_trspcCache.chrc[idx], p_proxy);
    } 
}

void BLE_TRSPC_ProxyRemoveHandler(GDBusProxy *p_proxy)
{
    const char *p_interface;

    p_interface = g_dbus_proxy_get_interface(p_proxy);

    if (!strcmp(p_interface, "org.bluez.GattService1")) 
    {
        s_trspcCache.p_svc = g_list_remove(s_trspcCache.p_svc, p_proxy);
    }
    else if (!strcmp(p_interface, "org.bluez.GattCharacteristic1")) 
    {
        uint8_t idx;
        GList *p_list;

        for (idx = TRSPC_INDEX_CHARTUD; idx < TRSPC_CHAR_NUM; idx++)
        {
            p_list = g_list_find(s_trspcCache.chrc[idx], p_proxy);
            if (p_list != NULL)
            {
                s_trspcCache.chrc[idx] = g_list_delete_link(s_trspcCache.chrc[idx], p_list);
                break;
            }
        }
    }

}


void BLE_TRSPC_PropertyHandler(GDBusProxy *p_proxy, const char *p_name, DBusMessageIter *p_iter)
{
    if (strcmp(p_name, "ServicesResolved") == 0) 
    {
        gboolean resolved;
    
        dbus_message_iter_get_basic(p_iter, &resolved);

        if (resolved)
        {
            BLE_TRSPC_ConnList_T * p_conn;
        
            p_conn = ble_trspc_GetConnListByProxy(p_proxy);

            if (p_conn != NULL)
            {
                if (ble_trspc_GetProxyCache(p_conn) == false)
                {
                        return;
                }
                
                if (bleTrspcProcess != NULL)
                {
                    BLE_TRSPC_Event_T evtPara;
            
                    evtPara.eventId = BLE_TRSPC_EVT_DISC_COMPLETE;
                    evtPara.eventField.onUplinkStatus.p_dev = p_conn->p_dev;
                    bleTrspcProcess(&evtPara);
                }

                ble_trspc_EnableDataSession(p_conn, (BLE_TRSPC_CBFC_DL_ENABLED|BLE_TRSPC_CBFC_UL_ENABLED));
            }
        }
    }
    else if (strcmp(p_name, "Value") == 0)
    {
        if (strcmp(g_dbus_proxy_get_interface(p_proxy), "org.bluez.GattCharacteristic1") == 0)
        {
            uint8_t i;               
            DBusMessageIter  array;
            uint8_t *p_value;
            int len;
            dbus_message_iter_recurse(p_iter, &array);
            dbus_message_iter_get_fixed_array(&array, &p_value, &len);

            for(i=0; i<BLE_TRSPC_MAX_CONN_NBR; i++)
            {
                if (s_trspcConnList[i].state == BLE_TRSPC_STATE_CONNECTED)
                {
                   if (s_trspcConnList[i].chrc[TRSPC_INDEX_CHARTUD] == p_proxy)
                   {
                        ble_trspc_RcvData(&s_trspcConnList[i], len, p_value);
                   }
                   else if (s_trspcConnList[i].chrc[TRSPC_INDEX_CHARTCP] == p_proxy)
                   {
                        ble_trspc_RcvCtrlData(&s_trspcConnList[i], len, p_value);
                   }
               }
           }
       }
   }
}

uint16_t BLE_TRSPC_SendVendorCommand(GDBusProxy *p_proxyDev, uint8_t commandID, uint8_t commandLength, uint8_t *p_commandPayload)
{
    BLE_TRSPC_ConnList_T *p_conn;
    BLE_TRSPC_MethodData_T *p_data;
    char * p_charValue;


    p_conn = ble_trspc_GetConnListByProxy(p_proxyDev);
    if (p_conn == NULL)
    {
        return TRSP_RES_FAIL;
    }

    if ((p_conn->trspState&BLE_TRSPC_CP_STATUS_ENABLED)==0U)
    {
        return TRSP_RES_FAIL;
    }

    if (commandID < BLE_TRSPC_VENDOR_OPCODE_MIN)
    {
        return TRSP_RES_INVALID_PARA;
    }

    if (commandLength > (p_conn->attMtu-TRSP_ATT_HEADER_SIZE-1U))
    {
        return TRSP_RES_INVALID_PARA;
    }



	p_data = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_data == NULL)
	{
		return TRSP_RES_OOM;
	}

    p_charValue = g_malloc(commandLength+1U);
    if (p_charValue == NULL)
    {
        g_free(p_data);
        return TRSP_RES_OOM;
    }
    
    p_charValue[0] = commandID;
    (void)memcpy(p_charValue + 1, p_commandPayload, commandLength);
    
    p_data->p_conn = p_conn;
    p_data->iov.iov_base = p_charValue;
    p_data->iov.iov_len = (uint16_t)commandLength+1U;
    p_data->p_type = "request";   
    p_data->caller = BLE_TRSPC_MD_CALLER_VERDOR_CMD;

    

    if (g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTCP], "WriteValue", ble_trspc_WriteSetup, ble_trspc_WriteReply, p_data, NULL))
    {
        p_conn->vendorCmdProc = VENCOM_PROC_ENABLE;

        g_free(p_charValue);
        
        return TRSP_RES_SUCCESS;
    }
    else
    {
        g_free(p_data);
        g_free(p_charValue);
        return TRSP_RES_FAIL;
    }

}



uint16_t BLE_TRSPC_SendData(GDBusProxy *p_proxyDev, uint16_t len, uint8_t *p_data)
{
    BLE_TRSPC_ConnList_T *p_conn;
    BLE_TRSPC_MethodData_T *p_mdData;


    p_conn = ble_trspc_GetConnListByProxy(p_proxyDev);
    if (p_conn == NULL)
    {
        return TRSP_RES_FAIL;
    }

    if (((p_conn->trspState&BLE_TRSPC_DL_STATUS_CBFCENABLED) != 0U) && (p_conn->localCredit == 0U))
    {
        return TRSP_RES_NO_RESOURCE;
    }

    if (len > (p_conn->attMtu - TRSP_ATT_HEADER_SIZE))
    {
        return TRSP_RES_FAIL;
    }

	p_mdData = g_new0(BLE_TRSPC_MethodData_T, 1);
	if (p_mdData == NULL)
	{
		return TRSP_RES_OOM;
	}

     
    if ((p_conn->trspState & BLE_TRSPC_DL_STATUS_CBFCENABLED) != 0U)
    {
        p_mdData->p_type = "command";
    }
    else if ((p_conn->trspState & BLE_TRSPC_DL_STATUS_NONCBFCENABLED) != 0U)
    {
        p_mdData->p_type = "request";
    }
    else
    {
        g_free(p_mdData);
        return TRSP_RES_BAD_STATE;
    }    

    p_mdData->p_conn = p_conn;
    p_mdData->iov.iov_base = p_data;
    p_mdData->iov.iov_len = len;
    p_mdData->caller = BLE_TRSPC_MD_CALLER_DATA;


    if (g_dbus_proxy_method_call(p_conn->chrc[TRSPC_INDEX_CHARTDD], "WriteValue", ble_trspc_WriteSetup, ble_trspc_WriteReply, p_mdData, NULL))
    {
        if ((p_conn->trspState & BLE_TRSPC_DL_STATUS_CBFCENABLED) != 0U)
        {
            p_conn->localCredit--;
        }

        return TRSP_RES_SUCCESS;
    }
    else
    {
        g_free(p_mdData);
        return TRSP_RES_FAIL;
    }
}

void BLE_TRSPC_GetDataLength(GDBusProxy *p_proxyDev, uint16_t *p_dataLength)
{
    BLE_TRSPC_ConnList_T *p_conn = NULL;

    p_conn = ble_trspc_GetConnListByProxy(p_proxyDev);
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

uint16_t BLE_TRSPC_GetData(GDBusProxy *p_proxyDev, uint8_t *p_data)
{
    BLE_TRSPC_ConnList_T *p_conn = NULL;

    p_conn = ble_trspc_GetConnListByProxy(p_proxyDev);
    if (p_conn != NULL)
    {
        if ((p_conn->inputQueue.usedNum) > 0U)
        {
            if (p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet != NULL)
            {
                (void)memcpy(p_data, p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet, 
                    p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].length);
                g_free(p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet);
                p_conn->inputQueue.packetList[p_conn->inputQueue.readIndex].p_packet = NULL;
            }

            p_conn->inputQueue.readIndex++;
            if (p_conn->inputQueue.readIndex >= BLE_TRSPC_INIT_CREDIT)
            {
                p_conn->inputQueue.readIndex = 0;
            }

            p_conn->inputQueue.usedNum --;

            if ((p_conn->trspState & BLE_TRSPC_UL_STATUS_CBFCENABLED) != 0U)
            {
                p_conn->peerCredit++;
                if (p_conn->peerCredit >= BLE_TRSPC_MAX_RETURN_CREDIT)
                {
                    ble_trspc_ClientReturnCredit(p_conn);
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

