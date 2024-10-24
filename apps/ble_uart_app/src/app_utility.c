/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Utility Function Source File

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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdlib.h>
#include <string.h>
#include "app_utility.h"
#include "application.h"
#include "app_error_defs.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************


uint8_t APP_UTILITY_GetAvailCircQueueNum(APP_UTILITY_CircQueue_T *p_circQ)
{
    if (p_circQ == NULL)
        return 0;
    
    return (p_circQ->size - p_circQ->usedNum);
}

uint8_t APP_UTILITY_GetValidCircQueueNum(APP_UTILITY_CircQueue_T *p_circQ)
{
    uint8_t validNum = 0;
    
    if (p_circQ != NULL)
    {
        if (p_circQ->usedNum < p_circQ->size)
            validNum = p_circQ->size - p_circQ->usedNum;
    }
    
    return validNum;
}

uint16_t APP_UTILITY_InsertDataToCircQueue(uint16_t dataLeng, uint8_t *p_data, 
    APP_UTILITY_CircQueue_T *p_circQ)
{
    uint16_t status = APP_RES_SUCCESS;

    if ((dataLeng > 0) && (p_data != NULL) && (p_circQ != NULL))
    {
        if (APP_UTILITY_GetValidCircQueueNum(p_circQ) > 0)
        {
            p_circQ->p_queueElem[p_circQ->writeIdx].dataLeng = dataLeng;
            p_circQ->p_queueElem[p_circQ->writeIdx].p_data = p_data;
            p_circQ->usedNum++;
            p_circQ->writeIdx++;
            if (p_circQ->writeIdx >= p_circQ->size)
                p_circQ->writeIdx = 0;
        }
        else
            return APP_RES_NO_RESOURCE;
    }
    else
        return APP_RES_INVALID_PARA;
    
    return status;
}

APP_UTILITY_QueueElem_T * APP_UTILITY_GetElemCircQueue(APP_UTILITY_CircQueue_T *p_circQ)
{
    APP_UTILITY_QueueElem_T *p_queueElem;

    if (p_circQ != NULL)
    {
        if (p_circQ->usedNum > 0)
        {
            p_queueElem = (APP_UTILITY_QueueElem_T *) &(p_circQ->p_queueElem[p_circQ->readIdx].dataLeng);
        }
        else
            return NULL;
    }
    else
        return NULL;
    
    return p_queueElem;
}

void APP_UTILITY_FreeElemCircQueue(APP_UTILITY_CircQueue_T *p_circQ)
{
    if (p_circQ != NULL)
    {
        p_circQ->p_queueElem[p_circQ->readIdx].dataLeng = 0;
        if (p_circQ->p_queueElem[p_circQ->readIdx].p_data != NULL)
            free(p_circQ->p_queueElem[p_circQ->readIdx].p_data);
        if (p_circQ->usedNum > 0)
            p_circQ->usedNum--;
        p_circQ->readIdx++;
        if (p_circQ->readIdx >= p_circQ->size)
            p_circQ->readIdx = 0;
    }
}

uint16_t APP_UTILITY_InitCircQueue(APP_UTILITY_CircQueue_T *p_circQ, uint8_t size)
{
    uint8_t i;

    if (p_circQ == NULL || size == 0)
        return APP_RES_FAIL;

    if (p_circQ->p_queueElem)
        free(p_circQ->p_queueElem);
    
    memset(p_circQ, 0, sizeof(APP_UTILITY_CircQueue_T));
    
    p_circQ->p_queueElem = malloc(size*sizeof(APP_UTILITY_QueueElem_T));
    if (p_circQ->p_queueElem == NULL)
    {
        return APP_RES_OOM;
    }

    p_circQ->size = size;

    for(i=0; i<size; i++)
    {
        p_circQ->p_queueElem[i].dataLeng = 0;
        p_circQ->p_queueElem[i].p_data = NULL;
    }

    return APP_RES_SUCCESS;
}

