/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application Timer Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_timer.c

  Summary:
    This file contains the Application Timer functions for this project.

  Description:
    This file contains the Application Timer functions for this project.
    Including the Set/Stop/Reset timer and timer expired handler.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include <glib.h>

#include "shared/shell.h"

#include "application.h"
#include "app_timer.h"
#include "app_error_defs.h"
#include "app_hci_dfu.h"


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_TIMER_MAX_TMSG                    0x0F /**< Maximum of timer message. */
#define APP_TRP_TMR_ID_INST_MERGE(id, instance) ((((uint16_t)id) << 8) | instance)



// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
typedef struct APP_TIMER_MsgTmr_T
{
   uint8_t             state;
   APP_TIMER_TmrElem_T te;
} APP_TIMER_MsgTmr_T;





// *****************************************************************************
// *****************************************************************************
// Section: Local Variables
// *****************************************************************************
// *****************************************************************************
static APP_TIMER_MsgTmr_T        s_msgTimer[APP_TIMER_MAX_TMSG];


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

static bool app_timer_TimerEventHandler(APP_TIMER_TmrElem_T        * p_timer);
static bool app_timer_MsgTimerHandler(APP_TIMER_TmrElem_T        * p_timer);


static int APP_TIMER_TimerExpiredHandle(gpointer user_data)
{
    return app_timer_TimerEventHandler((APP_TIMER_TmrElem_T *)user_data);
}

void APP_TIMER_SetTimerElem(uint8_t timerId, uint8_t instance, void *p_tmrParam, APP_TIMER_TmrElem_T *p_tmrElem)
{
    p_tmrElem->tmrId = timerId;
    p_tmrElem->instance= instance;
    p_tmrElem->p_tmrParam= p_tmrParam;
}


uint16_t APP_TIMER_StopTimer(guint *timerHandler)
{
    if (timerHandler == NULL)
    {
        return APP_RES_INVALID_PARA;
    }

    if (g_source_remove(*timerHandler) == false)
    {
        return APP_RES_FAIL;
    }

    *timerHandler = 0;

    return APP_RES_SUCCESS;
}

uint16_t APP_TIMER_SetTimer(uint16_t idInstance, void *p_tmrParam, uint32_t timeout, 
                                        APP_TIMER_TmrElem_T *p_tmrElem)
{
    uint8_t tmrId;
    uint16_t result;

    if (p_tmrElem && p_tmrElem->tmrHandle)
    {
        g_source_remove(p_tmrElem->tmrHandle);
        p_tmrElem->tmrHandle = 0;
    }

    tmrId = (uint8_t)(idInstance >> 8);
    APP_TIMER_SetTimerElem(tmrId, (uint8_t)idInstance, (void *)p_tmrParam, p_tmrElem);

    p_tmrElem->tmrHandle = g_timeout_add(timeout, APP_TIMER_TimerExpiredHandle, p_tmrElem);

    if (p_tmrElem->tmrHandle)
        result = APP_RES_SUCCESS;
    else
        result = APP_RES_FAIL;

    return result;
}



#define APP_TIMER_TMSG_COMPOUND_ID(a,b) (((a & 0x0F) << 4) | (b & 0x0F))
#define APP_TIMER_TMSG_MSGID(a) ((a & 0xF0) >> 4)
#define APP_TIMER_TMSG_INST(a) (a & 0x0F)
#define APP_TIMER_ONESHOT_TIMER (0x01)
#define APP_TIMER_PERIODIC_TIMER (0x02)
#define APP_TIMER_EXPIRED_TIMER (0x03)
static uint8_t app_timer_FindFreeMsgIndex(uint8_t msgId, uint8_t instance)
{
    uint8_t i;
    
    for (i=0; i< APP_TIMER_MAX_TMSG; i++)
    {
        if (s_msgTimer[i].te.tmrHandle == 0)
        {
            return i;
        }
    }

    return APP_TIMER_MAX_TMSG;
}

static void app_timer_MsgExpired(uint8_t msgId, uint8_t instance)
{
    uint8_t i;
    uint8_t compId = APP_TIMER_TMSG_COMPOUND_ID(msgId, instance);
    
    for (i=0; i< APP_TIMER_MAX_TMSG; i++)
    {
        if (s_msgTimer[i].te.instance == compId &&
            s_msgTimer[i].state == APP_TIMER_ONESHOT_TIMER)
        {
            s_msgTimer[i].state = APP_TIMER_EXPIRED_TIMER;
        }
    }
}

static uint8_t app_timer_FindMsgIndex(uint8_t msgId, uint8_t instance, uint8_t state)
{
    uint8_t i;
    uint8_t compId = APP_TIMER_TMSG_COMPOUND_ID(msgId, instance);
    
    for (i=0; i< APP_TIMER_MAX_TMSG; i++)
    {
        if (s_msgTimer[i].te.instance == compId &&
            s_msgTimer[i].state == state)
        {
            return i;
        }
    }

    return APP_TIMER_MAX_TMSG;
}

static uint16_t app_timer_ClearTimerMsg(uint8_t msgId, uint8_t instance)
{
    uint8_t tmsgIdx;

    tmsgIdx = app_timer_FindMsgIndex(msgId, instance, APP_TIMER_EXPIRED_TIMER);
    if (tmsgIdx == APP_TIMER_MAX_TMSG)
        return APP_RES_FAIL;

    APP_TIMER_StopTimer(&(s_msgTimer[tmsgIdx].te.tmrHandle));
    memset(&s_msgTimer[tmsgIdx], 0, sizeof(APP_TIMER_MsgTmr_T));

    return APP_RES_SUCCESS;
}


static bool app_timer_MsgTimerHandler(APP_TIMER_TmrElem_T        * p_timer)
{
    bool timerPeriodic = false; 
    uint8_t mergeMsg = p_timer->instance;
    app_timer_MsgExpired(APP_TIMER_TMSG_MSGID(mergeMsg), APP_TIMER_TMSG_INST(mergeMsg));

    switch (APP_TIMER_TMSG_MSGID(mergeMsg))
    {
        default:
        break;
    }

    app_timer_ClearTimerMsg(APP_TIMER_TMSG_MSGID(mergeMsg), APP_TIMER_TMSG_INST(mergeMsg));
    return timerPeriodic;
}

uint16_t APP_TIMER_StopMsgTimer(APP_TIMER_MsgTmrId_T msgId, uint8_t instance)
{
    uint8_t tmsgIdx;
    uint16_t res;

    tmsgIdx = app_timer_FindMsgIndex(msgId, instance, APP_TIMER_ONESHOT_TIMER);
    if (tmsgIdx == APP_TIMER_MAX_TMSG)
    {
        tmsgIdx = app_timer_FindMsgIndex(msgId, instance, APP_TIMER_PERIODIC_TIMER);
        if (tmsgIdx == APP_TIMER_MAX_TMSG)
            return APP_RES_FAIL;
    }

    res = APP_TIMER_StopTimer(&(s_msgTimer[tmsgIdx].te.tmrHandle));
    if (res == APP_RES_SUCCESS)
    {
        memset(&s_msgTimer[tmsgIdx], 0, sizeof(APP_TIMER_MsgTmr_T));
    }
    return res;
}

uint16_t APP_TIMER_SetMsgTimer(APP_TIMER_MsgTmrId_T msgId, uint8_t instance, uint32_t timeout, void *p_param)

{
    uint16_t result = APP_RES_SUCCESS;
    uint8_t tmsgIdx;
    uint8_t compId = APP_TIMER_TMSG_COMPOUND_ID(msgId, instance);

    tmsgIdx = app_timer_FindFreeMsgIndex(msgId, instance);
    if (tmsgIdx == APP_TIMER_MAX_TMSG)
    {
        return APP_RES_FAIL;
    }

    s_msgTimer[tmsgIdx].state = APP_TIMER_ONESHOT_TIMER;
    result = APP_TIMER_SetTimer(APP_TRP_TMR_ID_INST_MERGE(APP_TIMER_MSG_TMR, compId), p_param, timeout, 
        &s_msgTimer[tmsgIdx].te);

    if (result != APP_RES_SUCCESS)
    {
        printf("MsgTimer err(%d)\n", result);
    }

    return result;
}

static bool app_timer_TimerEventHandler(APP_TIMER_TmrElem_T        * p_timer)
{
    bool timerPeriodic = false;
    
    switch(p_timer->tmrId)
    {
        case APP_TIMER_MSG_TMR:
        {
            timerPeriodic = app_timer_MsgTimerHandler(p_timer);
        }
        break;
    }

    return timerPeriodic;
}


