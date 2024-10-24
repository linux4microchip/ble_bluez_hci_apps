/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  BT Mgmt wrapper function Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_mgmt.h

  Summary:
    This header file provides prototypes and definitions for wrapper function of BT Mgmt API of BlueZ.

  Description:
    This header file provides prototypes and definitions for wrapper function of BT Mgmt API of BlueZ.

*******************************************************************************/

#ifndef _APP_MGMT_H
#define _APP_MGMT_H

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

#include "app_gap.h"



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

void APP_MGMT_Init(void);
void APP_MGMT_Deinit(void);
uint16_t APP_MGMT_SetLocalName(uint8_t len, uint8_t  *p_devName);
uint16_t APP_MGMT_GetLocalName(uint8_t *p_len, uint8_t  *p_devName);
uint16_t APP_MGMT_SetExtAdvParams(struct mgmt_cp_add_ext_adv_params *p_extAdvParams);
uint16_t APP_MGMT_SetExtAdvData(struct mgmt_cp_add_ext_adv_data *p_advDataParam);
uint16_t APP_MGMT_SetExtAdvEnable(bool enable);
uint16_t APP_MGMT_SetPhySupport(uint32_t phySupport);
uint16_t APP_MGMT_GetPhySupport(void);
uint16_t APP_MGMT_RemoveBonding(const char *p_bdaddr, const char * p_bdaddrType);
uint16_t APP_MGMT_SetSecureConnection(uint8_t mode);
uint16_t APP_MGMT_ReadControllerSetting(void);




//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_MGMT_H */

/*******************************************************************************
 End of File
 */

