/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _SVCAGT_LOG_H
#define  _SVCAGT_LOG_H

#include <stdarg.h>
#include <stdbool.h>

// Messages are always written to log files, but when TEST_ENVIRONMENT 
// is defined, messages are also displayed on the terminal screen.
//#define TEST_ENVIRONMENT 1

int log_init (const char *log_directory);

int log_shutdown (void);

void log_dbg (const char *fmt, ...);

void log_info (const char *fmt, ...);

void log_error (const char *fmt, ...);

void log_errno (int err, const char *fmt, ...);

void log_not (const char *fmt, ...);

bool log_level_is_debug (void);

#endif


