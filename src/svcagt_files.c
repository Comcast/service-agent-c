/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "svcagt_files.h"
#include "svcagt_log.h"
#include "svc_agt.h"

#ifdef SVCAGT_KEEP_FILES_OPEN
static FILE *exclude_fp = NULL;
static FILE *goal_states_fp = NULL;
#endif

// File name buffers.
static char exclude_fname[FILENAME_BUFLEN];
static char goal_states_fname[FILENAME_BUFLEN];

int seek_file_pos (FILE *goal_states_fp, long pos)
{
	if (fseek (goal_states_fp, pos, SEEK_SET) < 0) {
		svcagt_log (LEVEL_ERROR, errno, "Error seeking pos %ld in file %s\n", 
			pos, goal_states_fname);
		
		return errno;
	}
	return 0;
}

int seek_file_end (FILE *goal_states_fp)
{
	if (fseek (goal_states_fp, 0, SEEK_END) < 0) {
		svcagt_log (LEVEL_ERROR, errno, "Error seeking end of file %s\n", 
			goal_states_fname);
		return errno;
	}
	return 0;
}

int tell_file_pos (FILE *goal_states_fp, long *pos)
{
	long p = ftell (goal_states_fp);
	if (p < 0) {
		svcagt_log (LEVEL_ERROR, errno, "Error getting file pos of goal states file\n"); 
		return errno;
	}
	*pos = p;
	return 0;
}

// must be called after svcagt_files_open
static void exclude_file_open__ (FILE **fp)
{
#ifndef SVCAGT_KEEP_FILES_OPEN
  FILE *exclude_fp;
#endif

	exclude_fp = fopen (exclude_fname, "r");
	if (NULL == exclude_fp)
		svcagt_log (LEVEL_DEBUG, 0, "File %s not opened\n", exclude_fname);
	else
		svcagt_log (LEVEL_INFO, 0, "Opened file %s\n", exclude_fname);
  *fp = exclude_fp;
}

// must be called after svcagt_files_open
static int goal_state_file_open__ (FILE **fp)
{
	int fd;
#ifndef SVCAGT_KEEP_FILES_OPEN
  FILE *goal_states_fp;
#endif

	// Open the goal states file for read/write/append. Create if it doesn't exist.
	fd = open (goal_states_fname, O_CREAT | O_RDWR | O_SYNC, 0666);
	if (fd < 0) {
		svcagt_log (LEVEL_ERROR, errno, "Error(1) opening file %s\n", goal_states_fname);
		return errno;
	}
	goal_states_fp = fdopen (fd, "w+");
	if (goal_states_fp == NULL) {
		svcagt_log (LEVEL_ERROR, errno, "Error(2) opening file %s\n", goal_states_fname);
		return errno;
	}
  *fp = goal_states_fp;
	svcagt_log (LEVEL_INFO, 0, "Opened file %s\n", goal_states_fname);
  return 0;
}

int svcagt_exclude_file_open (FILE **fp)
{
#ifdef SVCAGT_KEEP_FILES_OPEN
  // it's already open
	*fp = exclude_fp;
#else
	exclude_file_open__ (fp);
#endif
	return 0;
}

int svcagt_goal_state_file_open (FILE **fp)
{
#ifdef SVCAGT_KEEP_FILES_OPEN
  // it's already open
	*fp = goal_states_fp;
	return 0;
#else
	return goal_state_file_open__ (fp);
#endif
}

static int file_close__ (FILE **fp)
{
	fclose (*fp);
	*fp = NULL;
  return 0;
}

#ifdef SVCAGT_KEEP_FILES_OPEN
int svcagt_file_close (FILE **fp __attribute__((unused)))
{
  // it already was closed
	return 0;
}
#else
int svcagt_file_close (FILE **fp)
{
	return file_close__ (fp);
}
#endif

int svcagt_files_open (const char *svcagt_directory, const char *svcagt_ex_directory)
{
#ifdef SVCAGT_KEEP_FILES_OPEN
	int err;
	FILE *exclude_fp;
	FILE *goal_states_fp;
#endif

	sprintf (exclude_fname, "%s/svcagt_exclude_services.txt", svcagt_ex_directory);
	sprintf (goal_states_fname, "%s/svcagt_goal_states.txt", svcagt_directory);

#ifdef SVCAGT_KEEP_FILES_OPEN
	exclude_file_open__ (&exclude_fp);
  err = goal_state_file_open__ (&goal_states_fp);
  if (err != 0)
    return err;
	return seek_file_pos (goal_states_fp, 0);
#else
	return 0;
#endif
}

int svcagt_goal_state_file_rewind (FILE *fp)
{
	return seek_file_pos (fp, 0);
}
	

void svcagt_files_close (void)
{
#ifdef SVCAGT_KEEP_FILES_OPEN
	file_close__ (&exclude_fp);
	file_close__ (&goal_states_fp);
#endif
}

int svcagt_exclude_file_read (FILE *fp, char *svc_name)
{
	char *pos;

	if (NULL == fp)
		return EOF;
	if (NULL == fgets (svc_name, SVCAGT_SVC_NAME_BUFLEN, fp))
		return EOF;
	pos = strchr (svc_name, '\n');
	if (pos != NULL)
		*pos = 0;
	return 0;	
}

// Read one line of the goal states file. Also retrieve the file position
// where you read.
int svcagt_goal_state_file_read (FILE *fp, char *svc_name, 
	bool *state, long *file_pos)
{
	int err, nfields, state_;

	err = tell_file_pos (fp, file_pos);
	if (err != 0)
		return -1;

	nfields = fscanf (fp, "%d %s\n", &state_, svc_name);
	if (nfields == EOF) {
		return 1;
	}
	if (nfields != 2) {
		svcagt_log (LEVEL_ERROR, 0, "Format error in goal states file\n");
		return -1;
	}
	if (state_ == (int)true)
		*state = true;
	else if (state_ == (int)false)
		*state = false;
	else {
		svcagt_log (LEVEL_ERROR, 0, "Invalid state %d in goal_states file\n", state_);
		return -1;
	}
	return 0;
}

// Append one line to the goal states file. Retrieve the file position
// where you wrote.
int svcagt_goal_state_file_append (FILE *fp, const char *svc_name, 
	bool state, long *file_pos)
{
	int err = seek_file_end (fp);
	if (err == 0)
		err = tell_file_pos (fp, file_pos);
	if (err != 0)
		return err;
	svcagt_log (LEVEL_DEBUG, 0, "Appending %s, pos %ld to goal state file\n", 
		svc_name, *file_pos);
	fprintf (fp, "%d %s\n", (int)state, svc_name);
	fflush (fp);
	return 0;
}

// Overwrite the goal states file at the specified position. 
int svcagt_goal_state_file_update (FILE *fp, long file_pos, bool state)
{
	int err = seek_file_pos (fp, file_pos);
	if (err != 0)
		return err;
	fprintf (fp, "%d", (int)state);
	fflush (fp);
	return 0;
}

