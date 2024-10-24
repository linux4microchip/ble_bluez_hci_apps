/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#ifndef APP_LOG_H
#define APP_LOG_H

#include <syslog.h>

#define  APP_LOG_INIT(tag) \
	do 			\
	{			\
		openlog(tag, LOG_PERROR | LOG_PID, LOG_DAEMON);	\
	} while(0) ;



#define LOG(fmt, args...)  syslog(LOG_DEBUG, fmt, ##args)
#define APP_LOG_ERROR(...) printf(__VA_ARGS__);
#define APP_LOG_INFO(...) printf(__VA_ARGS__);
#define APP_LOG_DEBUG(...) printf(__VA_ARGS__);

#endif