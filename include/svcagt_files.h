/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _SVCAGT_FILES_H
#define  _SVCAGT_FILES_H

#include <stdbool.h>

#define FILENAME_BUFLEN 120
#define SVCAGT_DIR_MAXLEN  70

#define SVCAGT_SVC_NAME_BUFLEN 128

/**
 * Open all files.
 *
 */
int svcagt_files_open (const char *svcagt_directory);

/**
 * Close all files.
 *
 */
int svcagt_files_close (void);

/**
 * Read one service name from the exclude file
 *
 * @param svc_name buffer (SVCAGT_SVC_NAME_BUFLEN long) to receive the name
 * @return 0 on success, EOF at end
 */
int svcagt_exclude_file_read (char *svc_name);

/**
 * Read one line from the goal states file
 *
 * @param svc_name buffer (SVCAGT_SVC_NAME_BUFLEN long) to receive the name
 * @param state variable to receive the goal state
 * @param file_pos variable to receive the file position of this line.
 * @return 0 on success, 1 at end, -1 on error
 */
int svcagt_goal_state_file_read (char *svc_name, bool *state, long *file_pos);

/**
 * Append one line to the goal states file
 *
 * @param svc_name 
 * @param state 
 * @param file_pos variable to receive the file position of this line.
 * @return 0 on success, 1 at end, -1 on error
 */
int svcagt_goal_state_file_append (const char *svc_name, bool state, long *file_pos);

/**
 * Update the goal states file at specified position
 *
 * @param state 
 * @param file_pos position at which to write the new state value
 * @return 0 on success, 1 at end, -1 on error
 */
int svcagt_goal_state_file_update (long file_pos, bool state);

#endif

