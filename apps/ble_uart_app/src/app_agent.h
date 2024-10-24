/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Pairing Agent Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_timer.h

  Summary:
    This file contains the Application pairing agent function.

  Description:
    This file contains the Application pairing agent function.
 *******************************************************************************/

#ifndef _APP_AGENT_H
#define _APP_AGENT_H

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



#define BLE_SMP_IO_DISPLAYONLY      (0x00)
#define BLE_SMP_IO_DISPLAYYESNO     (0x01)
#define BLE_SMP_IO_KEYBOARDONLY     (0x02)
#define BLE_SMP_IO_NOINPUTNOOUTPUT  (0x03)
#define BLE_SMP_IO_KEYBOARDDISPLAY  (0x04)

#define BLE_SMP_SC_DISABLED         (0x00)
#define BLE_SMP_SC_ENABLED          (0x01)
#define BLE_SMP_SC_ONLY             (0x02)

#define DEFAULT_IO_CAPABILITY       BLE_SMP_IO_NOINPUTNOOUTPUT







// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

void APP_AGT_Register(GDBusProxy *p_manager, uint8_t ioCap);
void APP_AGT_Unregister(GDBusProxy *p_manager);
void APP_AGT_ChangeIoCap(GDBusProxy *p_manager, uint8_t ioCap);
const char * APP_AGT_GetIoCap(uint8_t *p_ioCap);


/**@} */ //_APP_AGENT_H


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END


#endif

/** @} */

/** @} */

/**
  @}
 */


/*******************************************************************************
 End of File
 */



