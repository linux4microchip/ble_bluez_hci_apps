/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Timer Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_timer.h

  Summary:
    This file contains the Application Timer functions for this project.

  Description:
    This file contains the Application Timer functions for this project.
    Including the Set/Stop/Reset timer and timer expired handler.
 *******************************************************************************/

#ifndef APP_TIMER_H
#define APP_TIMER_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

/**@brief The definition of Timer ID. */
typedef enum APP_TIMER_TimerId_T
{
    APP_TIMER_MSG_TMR,                      /**< The timer for BLE UART Application Messages. */
    APP_TIMER_RESERVED_02,                  /**< Reserved. */
    APP_TIMER_RESERVED_03,                  /**< Reserved. */
    APP_TIMER_RESERVED_04,                  /**< Reserved. */
    APP_TIMER_RESERVED_05,                  /**< Reserved. */
    APP_TIMER_RESERVED_06,                  /**< Reserved. */
    APP_TIMER_RESERVED_07,                  /**< Reserved. */
    APP_TIMER_RESERVED_08,                  /**< Reserved. */
    APP_TIMER_RESERVED_09,                  /**< Reserved. */

    APP_TIMER_TOTAL = 0x0A
} APP_TIMER_TimerId_T;


/**@brief The definition of Application Message Timer. */
typedef enum APP_TIMER_MsgTmrId_T
{
    APP_TIMER_MSG_END = 0x10
} APP_TIMER_MsgTmrId_T;


/**@defgroup APP_TIMER_TIMEOUT APP_TIMER_TIMEOUT
 * @brief The definition of the timeout value.
 * @{ */
#define APP_TIMER_10MS                                 0x0A     /**< 10ms timer. */
#define APP_TIMER_12MS                                 0x0C     /**< 12ms timer. */
#define APP_TIMER_18MS                                 0x12     /**< 18ms timer. */
#define APP_TIMER_20MS                                 0x14     /**< 20ms timer. */
#define APP_TIMER_30MS                                 0x1E     /**< 30ms timer. */
#define APP_TIMER_50MS                                 0x32     /**< 50ms timer. */
#define APP_TIMER_100MS                                0x64     /**< 100ms timer. */
#define APP_TIMER_500MS                                0x1F4    /**< 500ms timer. */
#define APP_TIMER_1S                                   0x3E8    /**< 1s timer. */
#define APP_TIMER_2S                                   0x7D0    /**< 2s timer. */
#define APP_TIMER_3S                                   0xBB8    /**< 3s timer. */
#define APP_TIMER_5S                                   0x1388   /**< 5s timer. */
#define APP_TIMER_30S                                  0x7530   /**< 30s timer. */
/** @} */

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
/**@brief The structure contains the information about Timer element structure. */
typedef struct APP_TIMER_TmrElem_T
{
    uint8_t        tmrId;           /**< Dedicated timer Id */
    uint8_t        instance;        /**< Dedicated timer instance */ 
    guint           tmrHandle;      /**< Dedicated timer handler */ 
    void            *p_tmrParam;    /**< Dedicated timer parameter */
} APP_TIMER_TmrElem_T;

typedef struct APP_TIMER_Msg_T
{
    uint8_t        msgId;
    void            *p_param;
} APP_TIMER_Msg_T;



// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************


uint16_t APP_TIMER_StopTimer(guint *timerHandler);

uint16_t APP_TIMER_SetTimer(uint16_t idInstance, void *p_tmrParam, uint32_t timeout, APP_TIMER_TmrElem_T *p_tmrElem);

uint16_t APP_TIMER_SetMsgTimer(APP_TIMER_MsgTmrId_T msgId, uint8_t instance, uint32_t timeout, void *p_param);

uint16_t APP_TIMER_StopMsgTimer(APP_TIMER_MsgTmrId_T msgId, uint8_t instance);


#endif
