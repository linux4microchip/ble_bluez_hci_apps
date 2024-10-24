/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application HCI Vendor Command Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.h

  Summary:
    This header file provides prototypes and definitions for the HCI Vendor Command handler.

  Description:
    This header file provides prototypes and definitions for the HCI Vendor Command handler.

*******************************************************************************/

#ifndef _APP_HCI_VND_H
#define _APP_HCI_VND_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END




// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

#define APP_HCI_DFU_MAX_TX_PAYLOAD_SIZE (0xFE)
#define HCI_HWERR_EVT_DFU_TIMEOUT (0x01)
#define APP_VENDOR_CMD_OCF (0x0000)
#define HCI_EVENT_HDR_CHECK_SIZE (HCI_EVENT_HDR_SIZE + 1) /*include 2nd byte for payload size*/
#define HCI_VND_CMD_TIMEOUT_DEFAULT (1000)

#define SUBOP_HCI_VND_DFU_ENABLE    0x01
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuEnableCmd_T;

#define HCI_VND_DFU_ENABLE_CP_SIZE 1

#define SUBOP_HCI_VND_DFU_REQUEST    0x02
typedef struct {
    uint8_t     subOp;
    uint32_t    imgSize;
    uint8_t     imgEnc;
    uint32_t    imgId;
} __attribute__ ((packed)) APP_HCI_VndDfuRequestCmd_T;
#define HCI_VND_DFU_REQUEST_CP_SIZE 10

#define SUBOP_HCI_VND_DFU_START    0x03
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuStartCmd_T;
#define HCI_VND_DFU_START_CP_SIZE 1

#define SUBOP_HCI_VND_DFU_DIST    0x04
typedef struct {
    uint8_t subOp;
    uint8_t payload[APP_HCI_DFU_MAX_TX_PAYLOAD_SIZE];
} __attribute__ ((packed)) APP_HCI_VndDfuDistCmd_T;
#define HCI_VND_DFU_DIST_CP_SIZE 1

#define SUBOP_HCI_VND_DFU_COMPL    0x05
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuComplCmd_T;
#define HCI_VND_DFU_COMPL_CP_SIZE 1

#define SUBOP_HCI_VND_DFU_EXIT    0x06
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuExitCmd_T;
#define HCI_VND_DFU_EXIT_CP_SIZE 1

////

#define SUBOP_HCI_VND_SLEEP_ENABLE    0x00
typedef struct {
    uint8_t subOp;
    uint8_t sleepEnable;
} __attribute__ ((packed)) APP_HCI_VndSleepEnableCmd_T;
#define HCI_VND_SLEEP_ENABLE_CP_SIZE 2

#define SUBOP_HCI_VND_APP_VERSION    0x07
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndAppVersionCmd_T;
#define HCI_VND_APP_VERSION_CP_SIZE 1

#define SUBOP_HCI_VND_MODE_CLEAR    0x08
typedef struct {
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndModeClearCmd_T;
#define HCI_VND_MODE_CLEAR_CP_SIZE 1

#define SUBOP_HCI_VND_UART_PARAM_CONFIG    0x09
typedef struct {
    uint8_t subOp;
    uint8_t baudRate;
    uint8_t stopBit;
    uint8_t parityMode;
    uint8_t parityCheck;
    uint8_t flowControl;
} __attribute__ ((packed)) APP_HCI_VndUartParamConfigCmd_T;
#define HCI_VND_UART_PARAM_CONFIG_SIMPLE_CP_SIZE 2
#define HCI_VND_UART_PARAM_CONFIG_COMPLETE_CP_SIZE 6

#define SUBOP_HCI_VND_PTA_ENABLE    0x0A
typedef struct {
    uint8_t subOp;
    uint8_t ptaEnable;
} __attribute__ ((packed)) APP_HCI_VndPtaEnableCmd_T;
#define HCI_VND_PTA_ENABLE_CP_SIZE 2




//////////////////////////////////////////////////////////////


typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuEnableRsp_T;
#define HCI_VND_DFU_ENABLE_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
    uint8_t imgFragSize;
} __attribute__ ((packed)) APP_HCI_VndDfuRequestRsp_T;
#define HCI_VND_DFU_REQUEST_RP_SIZE 3

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuStartRsp_T;
#define HCI_VND_DFU_START_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndDfuDistRsp_T;
#define HCI_VND_DFU_DIST_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
    uint8_t validationResult;
} __attribute__ ((packed)) APP_HCI_VndDfuComplRsp_T;
#define HCI_VND_DFU_COMPL_RP_SIZE 3

typedef struct {
    uint8_t status;
    uint8_t subOp;
    uint8_t exitStatus;
} __attribute__ ((packed)) APP_HCI_VndDfuExitRsp_T;
#define HCI_VND_DFU_EXIT_RP_SIZE 3

////

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndSleepEnableRsp_T;
#define HCI_VND_SLEEP_ENABLE_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
    uint8_t appVersion[4];
} __attribute__ ((packed)) APP_HCI_VndAppVersionRsp_T;
#define HCI_VND_APP_VERSION_RP_SIZE 6

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndModeClearRsp_T;
#define HCI_VND_MODE_CLEAR_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndUartParamConfigRsp_T;
#define HCI_VND_UART_PARAM_CONFIG_RP_SIZE 2

typedef struct {
    uint8_t status;
    uint8_t subOp;
} __attribute__ ((packed)) APP_HCI_VndPtaEnableRsp_T;
#define HCI_VND_PTA_ENABLE_RP_SIZE 2


////

typedef struct {
    char* p_dfuFilePath;
    char* p_ttyDevicePath;
    int   baudRate;
} __attribute__ ((packed)) APP_HCI_DfuParameters_T;




uint16_t APP_HCI_DfuEnable(int32_t dd, uint32_t to);
uint16_t APP_HCI_DfuRequest(int32_t dd, APP_HCI_VndDfuRequestCmd_T *p_cp, APP_HCI_VndDfuRequestRsp_T *p_rp, uint32_t to);
uint16_t APP_HCI_DfuStart(int32_t dd, uint32_t to);
uint16_t APP_HCI_DfuDistribution(int32_t dd, APP_HCI_VndDfuDistCmd_T *p_cp, uint32_t payloadLen, uint32_t to);
uint16_t APP_HCI_DfuComplete(int32_t dd, APP_HCI_VndDfuComplRsp_T *p_rp, uint32_t to);
uint16_t APP_HCI_DfuExit(int32_t dd, APP_HCI_VndDfuExitRsp_T *p_rp, uint32_t to);

uint16_t APP_HCI_SleepEnable(int32_t dd, bool isEnable, uint32_t to);
uint16_t APP_HCI_AppVersionInquiry(int32_t dd, uint32_t *p_appVersion, uint32_t to);
uint16_t APP_HCI_ModeClear(int32_t dd, uint32_t to);
uint16_t APP_HCI_UartParamConfig(int32_t dd, APP_HCI_VndUartParamConfigCmd_T *p_rp, bool isFullyConfig, uint32_t to);
uint16_t APP_HCI_PtaEnable(int32_t dd, bool isEnable, uint32_t to);

void APP_HCI_UseUartPath(bool enable);
int APP_HCI_OpenUartDev(char *p_portName);
void APP_HCI_CloseUartDev(int32_t fd);
int APP_HCI_UartConfig(int fd, int speed, int flowCtrl, int dataBits, int stopBits, int parity);






//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_HCI_VND_H */

/*******************************************************************************
 End of File
 */

