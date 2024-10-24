/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Transparent Common Function Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trp_common.h

  Summary:
    This file contains the Application Transparent common functions for this project.

  Description:
    This file contains the Application Transparent common functions for this project.
 *******************************************************************************/

#ifndef APP_TRP_COMMON_H
#define APP_TRP_COMMON_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "app_utility.h"
#include "app_timer.h"
#include "app_dbp.h"
#include <sys/time.h>

#include "gdbus/gdbus.h"


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

#define APP_TRP_CTRL_CHAN_ENABLE            0x01
#define APP_TRP_CTRL_CHAN_DISABLE           (~APP_TRP_CTRL_CHAN_ENABLE)
#define APP_TRP_DATA_CHAN_ENABLE            0x02
#define APP_TRP_DATA_CHAN_DISABLE           (~APP_TRP_DATA_CHAN_ENABLE)
#define APP_TRCBP_CTRL_CHAN_ENABLE          0x04
#define APP_TRCBP_CTRL_CHAN_DISABLE         (~APP_TRCBP_CTRL_CHAN_ENABLE)
#define APP_TRCBP_DATA_CHAN_ENABLE          0x08
#define APP_TRCBP_DATA_CHAN_DISABLE         (~APP_TRCBP_DATA_CHAN_ENABLE)
#define APP_TRP_MAX_TX_AVAILABLE_TIMES      0x0D
#define APP_TRP_MAX_TRANSMIT_NUM            0x0F

#define APP_TRP_VENDOR_OPCODE_BLE_UART      0x80    /**< Opcode for BLE UART */

#define APP_TRP_WMODE_TX_MAX_SIZE           (500 * 0x400) // 500k bytes
#define APP_TRP_WMODE_ERROR_RSP             0x03


/* vendor command retry*/
#define APP_TRP_SEND_GID_CS_FAIL            0x01
#define APP_TRP_SEND_GID_FP_FAIL            0x02
#define APP_TRP_SEND_GID_LB_FAIL            0x04
#define APP_TRP_SEND_GID_UART_FAIL          0x08
#define APP_TRP_SEND_GID_TX_FAIL            0x10
#define APP_TRP_SEND_ERROR_RSP_FAIL         0x20
#define APP_TRP_SEND_CHECK_SUM_FAIL         0x40
#define APP_TRP_SEND_LAST_NUMBER_FAIL       0x80
#define APP_TRP_SEND_LENGTH_FAIL            0x100
#define APP_TRP_SEND_TYPE_FAIL              0x200
#define APP_TRP_SEND_STATUS_FLAG            0x400
#define APP_TRP_SEND_GID_REV_LB_FAIL        0x800
#define APP_TRP_SEND_DATA_FAIL              0x1000

#define APP_TRP_SERVER_UART                 0x01
#define APP_TRP_CLIENT_UART                 0x02

#define APP_TRP_LE_MAX_QUEUE_NUM            0x02
#define APP_TRP_ML_MAX_QUEUE_NUM            0x04


/**@brief Enumeration type of BLE transparent type. */
typedef enum APP_TRP_TYPE_T
{
    APP_TRP_TYPE_UNKNOWN = 0x00,                     /**< Unknown Type. */
    APP_TRP_TYPE_LEGACY,                             /**< Legacy Transparent Profile. */
    APP_TRP_TYPE_TRCBP,                              /**< Transparent Credit Based Profile. */

    APP_TRP_TYPE_TOTAL
} APP_TRP_TYPE_T;

enum APP_TRP_GRPID_T
{
    TRP_GRPID_NULL = 0x00,
    TRP_GRPID_CHECK_SUM,
    TRP_GRPID_LOOPBACK,
    TRP_GRPID_FIX_PATTERN,
    TRP_GRPID_UART,
    TRP_GRPID_TRANSMIT,
    TRP_GRPID_UPDATE_CONN_PARA,
    TRP_GRPID_WMODE_SELECTION,
    TRP_GRPID_REV_LOOPBACK,
    TRP_GRPID_END
};

typedef enum APP_TRP_WMODE_T
{
    TRP_WMODE_NULL              = TRP_GRPID_NULL,
    TRP_WMODE_CHECK_SUM         = TRP_GRPID_CHECK_SUM,
    TRP_WMODE_LOOPBACK          = TRP_GRPID_LOOPBACK,
    TRP_WMODE_FIX_PATTERN       = TRP_GRPID_FIX_PATTERN,
    TRP_WMODE_UART              = TRP_GRPID_UART,
    //TRP_WMODE_REV_LOOPBACK      = TRP_GRPID_REV_LOOPBACK,
    
    TRP_WMODE_END
} APP_TRP_WMODE_T;

//TRP_GRPID_CHECK_SUM
enum
{
    APP_TRP_WMODE_CHECK_SUM_DISABLE   = 0x00,
    APP_TRP_WMODE_CHECK_SUM_ENABLE    = 0x01,
    APP_TRP_WMODE_CHECK_SUM           = 0x02,
    //APP_TRP_WMODE_ERROR_RSP           0x03
};

//TRP_GRPID_LOOPBACK
enum
{
    APP_TRP_WMODE_LOOPBACK_DISABLE    = 0x00,
    APP_TRP_WMODE_LOOPBACK_ENABLE     = 0x01,
    //APP_TRP_WMODE_ERROR_RSP           0x03
};

//TRP_GRPID_FIX_PATTERN
enum
{
    APP_TRP_WMODE_FIX_PATTERN_DISABLE = 0x00,
    APP_TRP_WMODE_FIX_PATTERN_ENABLE  = 0x01,
    APP_TRP_WMODE_TX_LAST_NUMBER      = 0x02,
    //APP_TRP_WMODE_ERROR_RSP           0x03
};

//TRP_GRPID_UART
enum
{
    APP_TRP_WMODE_UART_DISABLE        = 0x00,
    APP_TRP_WMODE_UART_ENABLE         = 0x01
};

//TRP_GRPID_TRANSMIT
enum
{
    APP_TRP_WMODE_TX_DATA_END         = 0x00,
    APP_TRP_WMODE_TX_DATA_START       = 0x01,
    APP_TRP_WMODE_TX_DATA_LENGTH      = 0x02,
    //APP_TRP_WMODE_ERROR_RSP           0x03
    APP_TRP_WMODE_TX_TYPE             = 0x04
};

//TRP_GRPID_UPDATE_CONN_PARA
enum
{
    APP_TRP_WMODE_SNED_UP_CONN_STATUS = 0x00,
    APP_TRP_WMODE_UPDATE_CONN_PARA    = 0x01
};

//TRP_GRPID_REV_LOOPBACK
enum
{
    APP_TRP_WMODE_REV_LOOPBACK_DISABLE= 0x00,
    APP_TRP_WMODE_REV_LOOPBACK_ENABLE = 0x01
    //APP_TRP_WMODE_ERROR_RSP           0x03
};






// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************



typedef enum APP_TRP_TestStage_T
{
    APP_TEST_IDLE,
    APP_TEST_PROGRESS,
    APP_TEST_PASSED,
    APP_TEST_FAILED
} APP_TRP_TestStage_T;


typedef enum APP_TRP_State_T
{
    APP_TRP_STATE_IDLE = 0x00,        /**< Default state (Disconnected). */
    APP_TRP_STATE_CONNECTED,          /**< Connected. */
} APP_TRP_State_T;

typedef enum APP_TRP_LINK_TYPE_T
{
    APP_TRP_LINK_TYPE_TX,       /**< Tx link. */
    APP_TRP_LINK_TYPE_RX        /**< Rx link. */
} APP_TRP_LINK_TYPE_T;

typedef enum APP_TRP_Role_T
{
    APP_TRP_UNKNOWN_ROLE = 0x00,
    APP_TRP_SERVER_ROLE  = 0x01,
    APP_TRP_CLIENT_ROLE  = 0x02
} APP_TRP_Role_T;


/**@brief The structure contains information about APP transparent connection parameters for recording connection information. */
typedef struct APP_TRP_ConnList_T
{
    APP_TRP_State_T         connState;          /**< Connection state. */
    APP_TRP_Role_T          trpRole;            /**< Transparent Role for APP_TRP_SERVER_ROLE or APP_TRP_CLIENT_ROLE */
    uint8_t                 channelEn;          /**< Channel enable for control channel and data channel. */
    uint16_t                exchangedMTU;       /**< Exchange MTU size */
    APP_TRP_WMODE_T         workMode;           /**< Work active mode */
    uint8_t                 workModeEn;         /**< Enable work mode procedure */
    uint8_t                 trpState;           /**< Transparent state */
    uint32_t                checkSum;           /**< Check sum value for check sum mode */
    uint16_t                lastNumber;         /**< The last number value for fix pattern mode */
    uint16_t                rxLastNunber;       /**< The received last number value for fix pattern check */
    uint16_t                txMTU;              /**< The Tx MTU value to transmit data by GATT */
    uint32_t                fixPattMaxSize;     /**< The total pattern length for fix pattern mode */
    uint16_t                fixPattTrcbpMtu;    /**< The fix pattern MTU value for fix pattern mode over L2CAP CoC. */
    APP_TRP_TYPE_T          type;               /**< Transparent type. See @ref APP_TRP_TYPE_T. */
    uint16_t                gattcRspWait;       /**< Wait for GATT client write response*/
    uint32_t                txTotalLeng;        /**< The transmission total length */
    uint16_t                lePktLeng;          /**< The LE packet length and it could be TRP or TRCBP packet size. */
    uint8_t                 maxAvailTxNumber;   /**< The maximum available number of transmission packets. */
    APP_UTILITY_CircQueue_T leCircQueue;        /**< The circular queue to store LE data */
    APP_UTILITY_CircQueue_T uartCircQueue;      /**< The circular queue to store UART data */
    DeviceProxy            *p_deviceProxy;     /**< DBus device proxy */
    APP_TRP_TestStage_T     testStage;          /**< Test Stage in Burst Mode*/
    GTimer                 *p_transTimer;      /**< Data Transmission timer used in Burst Mode for elapsed time calculation. */
    uint32_t                rxAccuLeng;
    uint16_t                progress;
} APP_TRP_ConnList_T;

/**@brief The structure contains the information about general data format. */
typedef struct APP_TRP_GenData_T
{
    uint16_t    rxLeng;         /**< Received length. */
    uint16_t    srcOffset;      /**< Source data offset. */
    uint8_t     *p_srcData;     /**< Source data pointer. */
} APP_TRP_GenData_T;

/**@brief The structure contains the information about traffic priority format. */
typedef struct APP_TRP_TrafficPriority_T
{
    uint8_t     rxToken;        /**< Rx token. */
    uint8_t     txToken;        /**< Tx token. */
    uint8_t     validNumber;    /**< Valid number. */  
} APP_TRP_TrafficPriority_T;



// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
extern const char * APP_TRP_TestStageStr[];

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
void APP_TRP_COMMON_Init(void);
void APP_TRP_COMMON_UpdateMtu(DeviceProxy *p_devProxy, uint16_t exchangedMTU);
void APP_TRP_COMMON_ConnEvtProc(DeviceProxy *p_devProxy, uint8_t gapRole);
void APP_TRP_COMMON_DiscEvtProc(DeviceProxy *p_devProxy);
uint16_t APP_TRP_COMMON_SendModeCommand(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId, uint8_t commandId);
uint16_t APP_TRP_COMMON_SendTypeCommand(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendErrorRsp(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId);
bool APP_TRP_COMMON_CheckValidTopology(uint8_t trpRole);
uint8_t APP_TRP_COMMON_GetConnIndex(APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_CtrlChOpenProc(bool isOpen);
void APP_TRP_COMMON_TxChOpenProc(bool isOpen);
uint16_t APP_TRP_COMMON_SendLeDataUartCircQueue(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendLastNumber(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendErrorRsp(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId);
uint16_t APP_TRP_COMMON_SendUpConnParaStatus(APP_TRP_ConnList_T *p_trpConn, uint8_t grpId, uint8_t commandId, uint8_t upParaStatus);
uint16_t APP_TRP_COMMON_GetTrpDataLength(APP_TRP_ConnList_T *p_trpConn, uint16_t *p_dataLeng);
uint16_t APP_TRP_COMMON_GetTrpData(APP_TRP_ConnList_T *p_trpConn, uint8_t *p_data);
uint16_t APP_TRP_COMMON_FreeLeData(APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_DelAllCircData(APP_UTILITY_CircQueue_T *p_circQueue);
void APP_TRP_COMMON_DelAllLeCircData(APP_UTILITY_CircQueue_T *p_circQueue);
uint32_t APP_TRP_COMMON_CalculateCheckSum(uint32_t checkSum, uint32_t *p_dataLeng, APP_TRP_ConnList_T *p_trpConn);
uint8_t * APP_TRP_COMMON_GenFixPattern(uint16_t *p_startSeqNum, uint16_t *p_patternLeng, uint32_t *p_pattMaxSize, uint32_t *p_checkSum);
uint16_t APP_TRP_COMMON_UpdateFixPatternLen(APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_InitFixPatternParam(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendFixPatternFirstPkt(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendFixPattern(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendMultiLinkFixPattern(APP_TRP_TrafficPriority_T *p_connToken, APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_CheckFixPatternData(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendLeDataToFile(APP_TRP_ConnList_T *p_trpConn, uint16_t dataLeng, uint8_t *p_rxBuf);
void APP_TRP_COMMON_SendTrpProfileDataToUART(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_InsertUartDataToCircQueue(APP_TRP_ConnList_T *p_trpConn,  APP_TRP_GenData_T *p_rxData);
uint16_t APP_TRP_COMMON_CopyUartRxData(APP_TRP_ConnList_T *p_trpConn, APP_TRP_GenData_T *p_rxData);
uint16_t APP_TRP_COMMON_SendLeDataUartCircQueue(APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_SendMultiLinkLeDataTrpProfile(APP_TRP_TrafficPriority_T *p_connToken, APP_TRP_ConnList_T *p_trpConn);
uint16_t APP_TRP_COMMON_UartRxData(APP_TRP_GenData_T *p_rxData, APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_FetchTxData(DeviceProxy *p_devProxy, uint16_t dataLeng);
APP_TRP_ConnList_T *APP_TRP_COMMON_GetConnListByIndex(uint8_t index);
APP_TRP_ConnList_T *APP_TRP_COMMON_GetConnListByDevProxy(DeviceProxy *p_devProxy);
APP_TRP_ConnList_T *APP_TRP_COMMON_ChangeNextLink(uint8_t trpRole, APP_TRP_LINK_TYPE_T linkType, APP_TRP_TrafficPriority_T *p_connToken);
bool APP_TRP_COMMON_IsWorkModeExist(uint8_t trpRole, APP_TRP_WMODE_T workMode);
uint16_t APP_TRP_COMMON_SendLengthCommand(APP_TRP_ConnList_T *p_trpConn, uint32_t length);
uint16_t APP_TRP_COMMON_SendCheckSumCommand(APP_TRP_ConnList_T *p_trpConn);
uint8_t APP_TRP_COMMON_GetRoleNum(uint8_t gapRole);
void APP_TRP_COMMON_AssignToken(APP_TRP_ConnList_T *p_trpConn, APP_TRP_LINK_TYPE_T linkType, APP_TRP_TrafficPriority_T *p_connToken);

void APP_TRP_COMMON_StartLog(APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_ProgressingLog(APP_TRP_ConnList_T *p_trpConn);
void APP_TRP_COMMON_FinishLog(APP_TRP_ConnList_T *p_trpConn);


#endif

