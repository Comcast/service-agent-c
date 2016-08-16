/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "svc_agt.h"
#include "svcagt_log.h"
#include "svcagt_db.h"
#include "svcagt_files.h"

extern void dbg (const char *fmt, ...);
extern void dbg_err (int err, const char *fmt, ...);

#define RUN_STATE_RUNNING	1234
#define RUN_STATE_DONE		-1234


static volatile int run_state = 0;
static char log_dirname[FILENAME_BUFLEN];

extern const char *svcagt_goal_state_str (bool state);

/**
 * Initialize the service agent
 *
 * @param svcagt_directory directory where the service agent saves info
 * @param service_list Pointer to a linked list of service_list_item's
 * @return 0 on success, valid errno otherwise.
 */
int svc_agt_init (const char *svcagt_directory)
{
	int err;
	struct stat stat_buf;

	if (NULL == svcagt_directory) {
		dbg ("svc_agt init NULL svcagt directory provided\n");
		return -1;
	}

	if (strlen(svcagt_directory) > SVCAGT_DIR_MAXLEN) {
		dbg ("svc agt init svcagt directory name too long\n");
		return -1;
	}

	err = stat (svcagt_directory, &stat_buf);
	if (err != 0) {
		dbg_err (errno, "directory %s cannot be accessed\n", svcagt_directory);
		return -1;
	}
	if (!S_ISDIR (stat_buf.st_mode)) {
		dbg ("%s is not a directory\n", svcagt_directory);
		return -1;
	}

	sprintf (log_dirname, "%s/logs", svcagt_directory);

	err = log_init (log_dirname);
	if (err != 0) {
		dbg ("Failed to init logger\n");
		return -1;
	}
	if (RUN_STATE_RUNNING == run_state) {
		log_error ("SVCAGT: already running at init\n");
		return EALREADY;
	}
	if (0 != run_state) {
		log_error ("SVCAGT: not idle at init\n");
		return EBUSY;
	}
	err = svcagt_files_open (svcagt_directory);
	if (err == 0) {
		err = svcagt_db_init ();
		if (err == 0)
			run_state = RUN_STATE_RUNNING;
  }
	return err;
}

int svc_agt_shutdown (void)
{
	run_state = RUN_STATE_DONE;
	svcagt_db_shutdown ();
	svcagt_files_close ();
	log_shutdown ();
	run_state = 0;
	return 0;
}

/**
 * Get the state of all services
 *
 * @param service_list Pointer to a linked list of service_list_item's
 * @return # items on success, -1 otherwise.
 */
int svc_agt_get_all (service_list_item_t **service_list, bool db_query)
{
	if (RUN_STATE_RUNNING != run_state) {
		dbg ("SVCAGT: not running at get all\n");
		return -1;
	}
	return svcagt_db_get_all (service_list, db_query);
}

void svc_agt_remove_service_list (service_list_item_t *service_list)
{
	svcagt_db_remove_service_list (service_list);
}

/**
 * Get the state of a specified service
 *
 * @param index index of the service to be queried
 * @param service_info Pointer to structure containing the info.
 * @return 0 on success, -1 otherwise.
 */
int svc_agt_get (unsigned index, service_info_t *service_info, bool db_query)
{
	const char *name;
	bool state;
	int err;

	if (RUN_STATE_RUNNING != run_state) {
		dbg ("SVCAGT: not running at get\n");
		return -1;
	}
	err = svcagt_db_get (index, &name, &state, db_query);
	if (err == 0) {
		service_info->svc_name = name;
		service_info->goal_state = svcagt_goal_state_str (state);
	}
	return 0;
}

/**
 * Set the state of a specified service
 *
 * @param index index of the service to be updated
 * @param new_state new state of the service
 * @return 0 on success, -1 otherwise.
 */
int svc_agt_set (unsigned index, const char *new_state)
{
	bool state_;

	if (RUN_STATE_RUNNING != run_state) {
		dbg ("SVCAGT: not running at set\n");
		return -1;
	}
	if (strcmp (new_state, svcagt_goal_state_str (true)) == 0)
		state_ = true;
	else if (strcmp (new_state, svcagt_goal_state_str (false)) == 0)
		state_ = false;
	else {
		log_error ("Invalid goal state %s\n", new_state);
		return -1;
	}
	return svcagt_db_set (index, state_);
}

