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


#define RUN_STATE_RUNNING	1234
#define RUN_STATE_DONE		-1234


static volatile int run_state = 0;
static char log_dirname[FILENAME_BUFLEN];

extern const char *svcagt_goal_state_str (bool state);

static int validate_directory (const char *directory, const char *msg)
{
	int err;
	struct stat stat_buf;

	if (NULL == directory) {
		svcagt_log (LEVEL_NO_LOGGER, 0, "svc_agt init NULL %s directory provided\n", msg);
		return -1;
	}

	if (strlen(directory) > SVCAGT_DIR_MAXLEN) {
		svcagt_log (LEVEL_NO_LOGGER, 0, "svc agt init %s directory name too long\n", msg);
		return -1;
	}

	err = stat (directory, &stat_buf);
	if (err != 0) {
		svcagt_log (LEVEL_NO_LOGGER, errno, "directory %s cannot be accessed\n", directory);
		return -1;
	}
	if (!S_ISDIR (stat_buf.st_mode)) {
		svcagt_log (LEVEL_NO_LOGGER, 0, "%s is not a directory\n", directory);
		return -1;
	}
	return 0;
}

/**
 * Initialize the service agent
 *
 * @param svcagt_directory directory where the service agent saves info
 * @param svcagt_ex_directory directory where extra input files are found
 * @param log_handler handler to receive all log notifications
 * @return 0 on success, valid errno otherwise.
 */
int svc_agt_init (const char *svcagt_directory, 
		const char *svcagt_ex_directory,
		svcagtLogHandler log_handler)
{
	int err = 0;

	if (validate_directory (svcagt_directory, "svcagt") != 0)
		return -1;

	if (NULL == svcagt_ex_directory)
		svcagt_ex_directory = svcagt_directory;
	else if (validate_directory (svcagt_ex_directory, "svcagt_ex") != 0)
		return -1;

	sprintf (log_dirname, "%s/logs", svcagt_directory);

	err = log_init (log_dirname, log_handler);
	if (err != 0) {
		svcagt_log (LEVEL_NO_LOGGER, 0, "Failed to init logger\n");
		return -1;
	}
	if (RUN_STATE_RUNNING == run_state) {
		svcagt_log (LEVEL_ERROR, 0, "SVCAGT: already running at init\n");
		return EALREADY;
	}
	if (0 != run_state) {
		svcagt_log (LEVEL_ERROR, 0, "SVCAGT: not idle at init\n");
		return EBUSY;
	}
	err = svcagt_files_open (svcagt_directory, svcagt_ex_directory);
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
		svcagt_log (LEVEL_NO_LOGGER, 0, "SVCAGT: not running at get all\n");
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
		svcagt_log (LEVEL_NO_LOGGER, 0, "SVCAGT: not running at get\n");
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
		svcagt_log (LEVEL_NO_LOGGER, 0, "SVCAGT: not running at set\n");
		return -1;
	}
	if (strcmp (new_state, svcagt_goal_state_str (true)) == 0)
		state_ = true;
	else if (strcmp (new_state, svcagt_goal_state_str (false)) == 0)
		state_ = false;
	else {
		svcagt_log (LEVEL_ERROR, 0, "Invalid goal state %s\n", new_state);
		return -1;
	}
	return svcagt_db_set (index, state_);
}

