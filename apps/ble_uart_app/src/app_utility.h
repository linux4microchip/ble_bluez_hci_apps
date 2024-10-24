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
    app_utility.c

  Summary:
    This file contains the Application utility functions for this project.

  Description:
    This file contains the Application utility functions for this project.
    Including the FIFO control (Linked list and circular queue).
 *******************************************************************************/

#ifndef APP_UTILITY_H
#define APP_UTILITY_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdint.h>


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_UTILITY_MAX_QUEUE_NUM      16


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


/**@brief The structure contains information about circular queue element. */
typedef struct APP_UTILITY_QueueElem_T
{
    uint16_t                   dataLeng;    /**< Data length. */
    uint8_t                    *p_data;     /**< Pointer to the data buffer */
} APP_UTILITY_QueueElem_T;

/**@brief The structure contains information about circular queue format. */
typedef struct APP_UTILITY_CircQueue_T
{
    uint8_t                     size;
    uint8_t                     usedNum;                                /**< The number of data list in circular queue. */
    uint8_t                     writeIdx;                               /**< The Index of data, written in circular queue. */
    uint8_t                     readIdx;                                /**< The Index of data, read in circular queue. */
    APP_UTILITY_QueueElem_T     *p_queueElem;   /**< The circular data queue. @ref APP_UTILITY_QueueElem_T.*/
} APP_UTILITY_CircQueue_T;


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************


/**@brief The function is to get the valid number stored in a circular queue.
 *
 * *@param[in] p_circQueue_t     Point to the circular queue of insert data. See @ref APP_UTILITY_CircQueue_T.
 *
 * @return A valid number stored in a circular queue.
 */
uint8_t APP_UTILITY_GetValidCircQueueNum(APP_UTILITY_CircQueue_T *p_circQ);

/**@brief The function is to insert data to a circular queue.
 *
 * *@param[in] dataLeng         Data length
 * *@param[in] p_data           Pointer to the data buffer
 * *@param[out] p_circQueue_t   Point to the circular queue of insert data. See @ref APP_UTILITY_CircQueue_T.
 *
 * @return A status.
 */
uint16_t APP_UTILITY_InsertDataToCircQueue(uint16_t dataLeng, uint8_t *p_data, 
    APP_UTILITY_CircQueue_T *p_circQ);

/**@brief The function is to get an element from a circular queue.
 *
 * *@param[in] p_circQueue_t     Point to the circular queue of insert data. See @ref APP_UTILITY_CircQueue_T.
 *
 * @return A data queue pointer. See @ref APP_UTILITY_QueueElem_T.
 */
APP_UTILITY_QueueElem_T * APP_UTILITY_GetElemCircQueue(APP_UTILITY_CircQueue_T *p_circQ);

/**@brief The function is to free an element from a circular queue.
 *
 * *@param[in] p_circQueue_t     Point to the circular queue of insert data. See @ref APP_UTILITY_CircQueue_T.
 *
 */
void APP_UTILITY_FreeElemCircQueue(APP_UTILITY_CircQueue_T *p_circQ);

/**@brief The function is to initialize a circular queue.
 *
 * *@param[in] p_circQueue_t     Point to the circular queue of insert data. See @ref APP_UTILITY_CircQueue_T.
 * *@param[in] size              Size of circular data queue.
 *
 */
uint16_t APP_UTILITY_InitCircQueue(APP_UTILITY_CircQueue_T *p_circQ, uint8_t size);

uint8_t APP_UTILITY_GetAvailCircQueueNum(APP_UTILITY_CircQueue_T *p_circQ);


#endif
