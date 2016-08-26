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

#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2


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

//#define svcagt_log(level, message, ...)
/**
 * @brief Handler used by svcagt_log_set_handler to receive all log
 * notifications produced by the library on this function.
 *
 * @param level The log level
 *
 * @param log_msg The actual log message reported.
 *
 */
typedef void (*svcagtLogHandler) (int level, const char * log_msg);


void  svcagt_log_set_handler (svcagtLogHandler handler);


void svcagt_log (  int level, const char *fmt, ...);
 

#endif


