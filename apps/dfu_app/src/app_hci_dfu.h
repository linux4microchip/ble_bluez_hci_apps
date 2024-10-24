/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application HCI DFU Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.h

  Summary:
    This header file provides prototypes and definitions for the HCI DFU handler.

  Description:
    This header file provides prototypes and definitions for the HCI DFU handler.

*******************************************************************************/

#ifndef _APP_HCI_DFU_H
#define _APP_HCI_DFU_H

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
#include <glib.h>


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



#define APP_OTAU_HEADER_SIZE_V1 (0x10)
#define APP_OTAU_HEADER_SIZE_V2 (0x20)

#define APP_DFU_VALIDATION_STATUS_SUCCESS (0x01)
#define APP_DFU_VALIDATION_STATUS_FAIL    (0x02)

#define APP_DFU_EXIT_WITH_REBOOT    (0x00)
#define APP_DFU_EXIT_ONLY           (0x01)




typedef struct APP_DFU_OtauInfo_T
{
    uint8_t     headerVer;
    uint8_t     imgEnc;
    uint16_t    checkSum;
    uint32_t    imgId;
    uint32_t    imgRev;
    uint8_t     fileType;
    uint16_t    crc16;
    uint32_t    imgSize;
    uint8_t     *p_img;
} __attribute__ ((packed)) APP_DFU_OtauInfo_T;


typedef enum APP_DFU_TestMode_T 
{
    APP_DFU_TEST_MODE_NA = 0,
    APP_DFU_TEST_MODE_TIMEOUT = 1,
    APP_DFU_TEST_MODE_ABORT = 2,
} APP_DFU_TestMode_T;


gpointer APP_DFU_ProcessThread(gpointer data);


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_HCI_DFU_H */

/*******************************************************************************
 End of File
 */

