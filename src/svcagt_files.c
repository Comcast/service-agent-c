/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "svcagt_files.h"
#include "svcagt_log.h"
#include "svc_agt.h"

static FILE *exclude_fp = NULL;
static FILE *goal_states_fp = NULL;

// File name buffers.
static char exclude_fname[FILENAME_BUFLEN];
static char goal_states_fname[FILENAME_BUFLEN];

int seek_file_pos (long pos)
{
	if (fseek (goal_states_fp, pos, SEEK_SET) < 0) {
		svcagt_log (LEVEL_ERROR,"Error seeking pos %ld in file %s: ,errno:%d(%s)\n", pos, goal_states_fname, errno, strerror(errno));
		
		return errno;
	}
	return 0;
}

int seek_file_end (void)
{
	if (fseek (goal_states_fp, 0, SEEK_END) < 0) {
		svcagt_log (LEVEL_ERROR,"Error seeking end of file %s: ,errno:%d(%s)\n", goal_states_fname, errno, strerror(errno));
		return errno;
	}
	return 0;
}

int tell_file_pos (long *pos)
{
	long p = ftell (goal_states_fp);
	if (p < 0) {
		svcagt_log (LEVEL_ERROR,"Error getting file pos of goal states file:errno:%d(%s)\n", errno, strerror(errno));
		return errno;
	}
	*pos = p;
	return 0;
}

int svcagt_files_open (const char *svcagt_directory)
{
	int fd;

	sprintf (exclude_fname, "%s/svcagt_exclude_services.txt", svcagt_directory);
	sprintf (goal_states_fname, "%s/svcagt_goal_states.txt", svcagt_directory);

	// Open the exclude file for read only
	exclude_fp = fopen (exclude_fname, "r");
	if (NULL == exclude_fp)
		
		svcagt_log (LEVEL_DEBUG, "File %s not opened\n", exclude_fname);
	else
		svcagt_log (LEVEL_INFO, "Opened file %s\n", exclude_fname);


	// Open the goal states file for read/write/append. Create if it doesn't exist.
	fd = open (goal_states_fname, O_CREAT | O_RDWR | O_SYNC, 0666);
	if (fd < 0) {
		svcagt_log (LEVEL_ERROR,"Error(1) opening file %s: ,errno:%d(%s)\n", goal_states_fname, errno, strerror(errno));
		return errno;
	}
	goal_states_fp = fdopen (fd, "w+");
	if (goal_states_fp == NULL) {
		svcagt_log (LEVEL_ERROR, "Error(2) opening file %s:errno:%d(%s)\n", goal_states_fname, errno, strerror(errno));
		return errno;
	}
	svcagt_log (LEVEL_INFO, "Opened file %s\n", goal_states_fname);

	return seek_file_pos (0);
}

int svcagt_goal_state_file_rewind (void)
{
	return seek_file_pos (0);
}
	

void svcagt_files_close (void)
{
	if (NULL != exclude_fp)
		fclose (exclude_fp);
	exclude_fp = NULL;
	fclose (goal_states_fp);
	goal_states_fp = NULL;
}

int svcagt_exclude_file_read (char *svc_name)
{
	char *pos;

	if (NULL == exclude_fp)
		return EOF;
	if (NULL == fgets (svc_name, SVCAGT_SVC_NAME_BUFLEN, exclude_fp))
		return EOF;
	pos = strchr (svc_name, '\n');
	if (pos != NULL)
		*pos = 0;
	return 0;	
}

// Read one line of the goal states file. Also retrieve the file position
// where you read.
int svcagt_goal_state_file_read (char *svc_name, bool *state, long *file_pos)
{
	int err, nfields, state_;

	err = tell_file_pos (file_pos);
	if (err != 0)
		return -1;

	nfields = fscanf (goal_states_fp, "%d %s\n", &state_, svc_name);
	if (nfields == EOF) {
		return 1;
	}
	if (nfields != 2) {
		svcagt_log (LEVEL_ERROR,"Format error in goal states file\n");
		return -1;
	}
	if (state_ == (int)true)
		*state = true;
	else if (state_ == (int)false)
		*state = false;
	else {
		svcagt_log (LEVEL_ERROR,"Invalid state %d in goal_states file\n", state_);
		return -1;
	}
	return 0;
}

// Append one line to the goal states file. Retrieve the file position
// where you wrote.
int svcagt_goal_state_file_append (const char *svc_name, bool state, long *file_pos)
{
	int err = seek_file_end ();
	if (err == 0)
		err = tell_file_pos (file_pos);
	if (err != 0)
		return err;
	svcagt_log (LEVEL_DEBUG,"Appending %s, pos %ld to goal state file\n", svc_name, *file_pos);
	fprintf (goal_states_fp, "%d %s\n", (int)state, svc_name);
	return 0;
}

// Overwrite the goal states file at the specified position. 
int svcagt_goal_state_file_update (long file_pos, bool state)
{
	int err = seek_file_pos (file_pos);
	if (err != 0)
		return err;
	fprintf (goal_states_fp, "%d", (int)state);
	return 0;
}




