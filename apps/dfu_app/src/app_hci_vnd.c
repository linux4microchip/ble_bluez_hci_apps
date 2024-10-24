/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
  Application HCI Vendor Command Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_cmd.c

  Summary:
    This file contains application HCI vendor commands handler.

  Description:
    This file contains application HCI vendor commands handler.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/uio.h>
#include <termios.h>

#include "bluetooth/bluetooth.h"
#include "bluetooth/hci.h"
#include "bluetooth/hci_lib.h"

#include "application.h"
#include "app_error_defs.h"
#include "app_hci_vnd.h"





// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************




// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
static int app_hci_UartSendReq(int dd, struct hci_request *p_req, int to);

static bool s_useUartPath = false;


static uint16_t app_hci_SendReq(int dd, struct hci_request *rq, int to)
{
    if(!s_useUartPath)
    {
        if (hci_send_req(dd, rq, to) < 0)
            return APP_RES_FAIL;
    }
    else
    {
        if (app_hci_UartSendReq(dd, rq, to) < 0)
            return APP_RES_FAIL;
    }
    return APP_RES_SUCCESS;
}

void APP_HCI_UseUartPath(bool enable)
{
    if (enable)
        s_useUartPath = true;
    else
        s_useUartPath = false;
}


uint16_t APP_HCI_DfuEnable(int32_t dd, uint32_t to)
{
    APP_HCI_VndDfuEnableCmd_T cp;
    APP_HCI_VndDfuEnableRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_DFU_ENABLE;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_DFU_ENABLE_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_DFU_ENABLE_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_DFU_ENABLE)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_DfuRequest(int32_t dd, APP_HCI_VndDfuRequestCmd_T *p_cp, APP_HCI_VndDfuRequestRsp_T *p_rp, uint32_t to)
{
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    p_cp->subOp = SUBOP_HCI_VND_DFU_REQUEST;
        
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = p_cp;
    rq.clen   = HCI_VND_DFU_REQUEST_CP_SIZE;
    rq.rparam = p_rp;
    rq.rlen   = HCI_VND_DFU_REQUEST_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (p_rp->status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (p_rp->subOp == SUBOP_HCI_VND_DFU_REQUEST)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_DfuStart(int32_t dd, uint32_t to)
{
    APP_HCI_VndDfuStartCmd_T cp;
    APP_HCI_VndDfuStartRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_DFU_START;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_DFU_START_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_DFU_START_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_DFU_START)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_DfuDistribution(int32_t dd, APP_HCI_VndDfuDistCmd_T *p_cp, uint32_t payloadLen, uint32_t to)
{
    APP_HCI_VndDfuDistRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    p_cp->subOp = SUBOP_HCI_VND_DFU_DIST;

    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = p_cp;
    rq.clen   = HCI_VND_DFU_DIST_CP_SIZE + payloadLen;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_DFU_DIST_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_DFU_DIST)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_DfuComplete(int32_t dd, APP_HCI_VndDfuComplRsp_T *p_rp, uint32_t to)
{
    APP_HCI_VndDfuComplCmd_T cp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_DFU_COMPL;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_DFU_COMPL_CP_SIZE;
    rq.rparam = p_rp;
    rq.rlen   = HCI_VND_DFU_COMPL_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (p_rp->status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (p_rp->subOp == SUBOP_HCI_VND_DFU_COMPL)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_DfuExit(int32_t dd, APP_HCI_VndDfuExitRsp_T *p_rp, uint32_t to)
{
    APP_HCI_VndDfuExitCmd_T cp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_DFU_EXIT;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_DFU_EXIT_CP_SIZE;
    rq.rparam = p_rp;
    rq.rlen   = HCI_VND_DFU_EXIT_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (p_rp->status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (p_rp->subOp == SUBOP_HCI_VND_DFU_EXIT)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_SleepEnable(int32_t dd, bool isEnable, uint32_t to)
{
    APP_HCI_VndSleepEnableCmd_T cp;
    APP_HCI_VndSleepEnableRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_SLEEP_ENABLE;
    cp.sleepEnable = isEnable;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_SLEEP_ENABLE_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_SLEEP_ENABLE_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_SLEEP_ENABLE)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_AppVersionInquiry(int32_t dd, uint32_t *p_appVersion, uint32_t to)
{
    APP_HCI_VndAppVersionCmd_T cp;
    APP_HCI_VndAppVersionRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_APP_VERSION;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_APP_VERSION_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_APP_VERSION_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_APP_VERSION)
    {
        if (p_appVersion)
        {
            uint8_t * ptr = rp.appVersion;
            *p_appVersion = bt_get_le32(ptr);
            return APP_RES_SUCCESS;
        }
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_ModeClear(int32_t dd, uint32_t to)
{
    APP_HCI_VndModeClearCmd_T cp;
    APP_HCI_VndModeClearRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_MODE_CLEAR;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_MODE_CLEAR_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_MODE_CLEAR_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_MODE_CLEAR)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_UartParamConfig(int32_t dd, APP_HCI_VndUartParamConfigCmd_T *p_cp, bool isFullyConfig, uint32_t to)
{
    APP_HCI_VndUartParamConfigRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    p_cp->subOp = SUBOP_HCI_VND_UART_PARAM_CONFIG;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = p_cp;
    if (isFullyConfig)
        rq.clen   = HCI_VND_UART_PARAM_CONFIG_COMPLETE_CP_SIZE;
    else
        rq.clen   = HCI_VND_UART_PARAM_CONFIG_SIMPLE_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_UART_PARAM_CONFIG_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_UART_PARAM_CONFIG)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}

uint16_t APP_HCI_PtaEnable(int32_t dd, bool isEnable, uint32_t to)
{
    APP_HCI_VndPtaEnableCmd_T cp;
    APP_HCI_VndPtaEnableRsp_T rp;
    struct hci_request rq;

    memset(&rq, 0, sizeof(rq));
    cp.subOp = SUBOP_HCI_VND_PTA_ENABLE;
    cp.ptaEnable = isEnable;
    
    rq.ogf    = OGF_VENDOR_CMD;
    rq.ocf    = APP_VENDOR_CMD_OCF;
    rq.cparam = &cp;
    rq.clen   = HCI_VND_PTA_ENABLE_CP_SIZE;
    rq.rparam = &rp;
    rq.rlen   = HCI_VND_PTA_ENABLE_RP_SIZE;

    if (app_hci_SendReq(dd, &rq, to) != APP_RES_SUCCESS)
        return APP_RES_FAIL;

    if (rp.status) {
        errno = EIO;
        return APP_RES_FAIL;
    }

    if (rp.subOp == SUBOP_HCI_VND_PTA_ENABLE)
    {
        return APP_RES_SUCCESS;
    }

    return APP_RES_FAIL;
}


static int app_hci_UartSendReq(int dd, struct hci_request *p_req, int to)
{
    unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
    uint16_t opcode = htobs(cmd_opcode_pack(p_req->ogf, p_req->ocf));
    hci_event_hdr *hdr;
    int try;

    if (hci_send_cmd(dd, p_req->ogf, p_req->ocf, p_req->clen, p_req->cparam) < 0)
        goto failed;

    try = 10;
    while (try--) {
        evt_cmd_complete *cc;
        evt_cmd_status *cs;
        evt_le_meta_event *me;
        int len;

        if (to) {
            struct pollfd p;
            int n;

            p.fd = dd; p.events = POLLIN;
            while ((n = poll(&p, 1, to)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;
                goto failed;
            }

            if (!n) {
                errno = ETIMEDOUT;
                goto failed;
            }

            to -= 10;
            if (to < 0)
                to = 0;
        }



        //read packet header
        len = 0;
        while (len < (HCI_EVENT_HDR_CHECK_SIZE))
        {
            while ((len += read(dd, buf+len, (HCI_EVENT_HDR_CHECK_SIZE-len))) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                goto failed;
            }

            if (len >= HCI_EVENT_HDR_CHECK_SIZE) break;
        }

        //read packet payload
        while (len < (buf[2]+HCI_EVENT_HDR_CHECK_SIZE))
        {
            while ((len += read(dd, buf+len, buf[2]+HCI_EVENT_HDR_CHECK_SIZE-len)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                goto failed;
            }
        }

        hdr = (void *) (buf + 1);
        ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
        len -= (1 + HCI_EVENT_HDR_SIZE);

        switch (hdr->evt) {
        case EVT_CMD_STATUS:
            cs = (void *) ptr;

            if (cs->opcode != opcode)
                continue;

            if (p_req->event != EVT_CMD_STATUS) {
                if (cs->status) {
                    errno = EIO;
                    goto failed;
                }
                break;
            }

            p_req->rlen = MIN(len, p_req->rlen);
            memcpy(p_req->rparam, ptr, p_req->rlen);
            goto done;

        case EVT_CMD_COMPLETE:
            cc = (void *) ptr;

            if (cc->opcode != opcode)
                continue;

            ptr += EVT_CMD_COMPLETE_SIZE;
            len -= EVT_CMD_COMPLETE_SIZE;

            p_req->rlen = MIN(len, p_req->rlen);
            memcpy(p_req->rparam, ptr, p_req->rlen);
            goto done;

        case EVT_LE_META_EVENT:
            me = (void *) ptr;

            if (me->subevent != p_req->event)
                continue;

            len -= 1;
            p_req->rlen = MIN(len, p_req->rlen);
            memcpy(p_req->rparam, me->data, p_req->rlen);
            goto done;

        default:
            if (hdr->evt != p_req->event)
                break;

            p_req->rlen = MIN(len, p_req->rlen);
            memcpy(p_req->rparam, ptr, p_req->rlen);
            goto done;
        }
    }
    errno = ETIMEDOUT;

failed:
    return -1;

done:
    return 0;
}

int APP_HCI_UartConfig(int fd, int speed, int flowCtrl, int dataBits, int stopBits, int parity)
{
    int baudRate[] = {B9600, B19200, B38400, B57600, 
                      B115200, B230400, B460800, B921600};
    int baudRateName[] = {BAUDRATE_9600, BAUDRATE_19200, BAUDRATE_38400, BAUDRATE_57600, 
                          BAUDRATE_115200, BAUDRATE_230400, BAUDRATE_460800, BAUDRATE_921600};
    struct termios options;
    
    if(tcgetattr(fd, &options)!=0)
    {
        perror("Setup UART fail\n");
        return -1;
    }

    for(uint8_t i=0; i < sizeof(baudRate)/sizeof(int); i++)
    {
        if(speed == baudRateName[i])
        {
            cfsetispeed(&options, baudRate[i]);
            cfsetospeed(&options, baudRate[i]);
            break;
        }
    }

    /* enable the receiver and set local mode */
    options.c_cflag |= (CLOCAL|CREAD);
    
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    switch(flowCtrl)
    {
        case 0: // No flow control is used
            options.c_cflag &= ~CRTSCTS;
            break;
        case 1: // Hardware flow control
            options.c_cflag |= CRTSCTS;
            break;
        case 2: // Software flow control
            options.c_cflag |= IXON | IXOFF | IXANY;
            break;
    }

    options.c_cflag &= ~CSIZE;	// Mask other flag bits
    switch (dataBits)
    {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS6;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:    
            options.c_cflag |= CS8;
            break;  
        default:   
            fprintf(stderr, "Unsupported data size\n");
            return -1;
    }

    switch(parity)
    {  
        case 'n':
        case 'N':	//No Parity bit
            options.c_cflag &= ~PARENB; 
            options.c_iflag &= ~INPCK;  
            break; 
        case 'o':  
        case 'O':	//Odd parity check
            options.c_cflag |= (PARODD | PARENB); 
            options.c_iflag |= INPCK;             
            break; 
        case 'e': 
        case 'E':	//Even parity check
            options.c_cflag |= PARENB;       
            options.c_cflag &= ~PARODD;       
            options.c_iflag |= INPCK;       
            break;
        case 's':
        case 'S':	//Space
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break; 
        default:  
            fprintf(stderr, "Unsupported parity\n");   
            return -1; 
    }

    //Set stop bit
    switch (stopBits)
    {  
        case 1:   
            options.c_cflag &= ~CSTOPB; 
            break; 
        case 2:   
            options.c_cflag |= CSTOPB; 
            break;
        default:   
            fprintf(stderr,"Unsupported stop bits\n"); 
        return -1;
    }

    //Modify output mode, RAW data mode
    options.c_oflag &= ~OPOST;

    //set the minimum waiting time and minimum receiving bytes before unblocking
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    tcflush(fd,TCIFLUSH);
    //active the configuration
    if(tcsetattr(fd, TCSANOW, &options) != 0)
    {
        return -1; 
    }
    return 0;
}

int32_t APP_HCI_OpenUartDev(char *p_portName)
{
    int32_t fd;
    
    fd = open(p_portName, O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd == false)
    {
        perror("Can't open the serial port\n");
        return(FALSE);
    }

    if (fcntl(fd, F_SETFL, 0) < 0)
    {
        return(FALSE);
    }

    // Flush away any bytes previously read or written.
    tcflush(fd, TCIOFLUSH);

    return fd;
}

void APP_HCI_CloseUartDev(int32_t fd)
{
    close(fd);
}


/*******************************************************************************
 End of File
 */
