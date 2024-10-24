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
    Including the Set/Stop timer and timer expired handler.
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
    APP_TIMER_PROTOCOL_RSP,                 /**< The timer to check TRP protocol response. */
    APP_TIMER_FETCH_TRP_RX_DATA,            /**< The timer to fetch the TRP RX data. */
    APP_TIMER_UART_SEND,                    /**< The timer to send data to UART. */
    APP_TIMER_TRP_VND_RETRY,                /**< The timer for TRP vendor command retry. */
    APP_TIMER_TRP_DAT_RETRY,                /**< The timer for TRP Data send to DBus retry. */
    APP_TIMER_FILE_FETCH,                   /**< The timer to fetch pattern file data. */
    APP_TIMER_RAW_DATA_FETCH,               /**< The timer to fetch raw data. */
    APP_TIMER_SCAN,                         /**< The timer to trigger scan activity. */
    APP_TIMER_CHECK_MODE,                   /**< The timer to check TRP work mode. */
    APP_TIMER_CHECK_MODE_FOR_ALL,           /**< The timer to check TRP work mode for all links. Instance can be ignored.*/
    APP_TIMER_CHECK_MODE_ONLY,              /**< The timer to check TRP work mode, no fetch tx data. */
    APP_TIMER_LOOPBACK_RX_CHECK,            /**< The timer to check whether Loopback Rx activity is finished. */
    APP_TIMER_RAW_DATA_RX_CHECK,            /**< The timer to check whether Raw data Rx activity is finished. */
    APP_TIMER_TRPS_RCV_CREDIT,              /**< The timer triggered by TRP server when credit has received. */
    APP_TIMER_TRPS_PROGRESS_CHECK,          /**< The timer to check TRP burst mode activity is inprogress. */
    APP_TIMER_TRPC_RCV_CREDIT,              /**< The timer triggered by TRP client when credit has received. */
    APP_TIMER_AUTO_NEXT_RUN,
    APP_TIMER_DATA_BUFFER_OVERFLOW_EVT,

    APP_TIMER_PERIODIC_START = 0xA0,
    //periodic timer define here
    APP_TIMER_MAX = 0xFF                    /**< The timer ID shluld not be grater than 0xFF. */
} APP_TIMER_TimerId_T;


/**@defgroup APP_TIMER_TIMEOUT APP_TIMER_TIMEOUT
 * @brief The definition of the timeout value.
 * @{ */
#define APP_TIMER_1MS                                  0x01     /**<  1ms timer. */
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

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************

/**@brief The function is used to set and start a timer. 
          Callers can use the same Timer ID with different Timer Instances to produce distinguishable timers.
          When trying to stop a specific timer, use the Timer ID plus the correct Timer Instance.
          Note that if you set a same Timer ID with same Timer Instance, it will stop the previous one and then create a new one.
 *@param[in] tmrId                            Timer ID. See @ref APP_TIMER_TimerId_T.
 *@param[in] instance                         Timer Instance.
 *@param[in] p_tmrParam                       User data.
 *@param[in] timeout                          Timeout value (unit: ms)
 *
 * @retval APP_RES_SUCCESS                    Set and start a timer successfully.
 * @retval APP_RES_FAIL                       Failed to start the timer.
 * @retval APP_RES_OOM                        No available memory.
 *
 */
uint16_t APP_TIMER_SetTimer(APP_TIMER_TimerId_T tmrId, uint8_t instance, void *p_tmrParam, uint32_t timeout);

/**@brief The function is used to stop a timer.
 *@param[in] tmrId                            Timer ID. See @ref APP_TIMER_TimerId_T.
 *@param[in] instance                         Timer Instance. 
 *
 * @retval APP_RES_SUCCESS                    Stop a timer successfully.
 * @retval APP_RES_FAIL                       Failed to stop the timer.
 * @retval APP_RES_INVALID_PARA               The timerId doesn't exist.
 *
 */
uint16_t APP_TIMER_StopTimer(APP_TIMER_TimerId_T tmrId, uint8_t instance);

#endif

