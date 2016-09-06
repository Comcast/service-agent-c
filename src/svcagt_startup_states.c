/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "svc_agt.h"
#include "svcagt_startup_states.h"
#include "svcagt_log.h"
#include "svcagt_files.h"
#include "svcagt_db.h"

#define SERVICES_DIR1 "/etc/systemd/system"
#define SERVICES_DIR2 "/lib/systemd/system"

const char *services_dir1 = SERVICES_DIR1;
const char *services_dir2 = SERVICES_DIR2;

// used for debugging
void set_test_services (const char *test_services_dir1, const char *test_services_dir2)
{
	services_dir1 = test_services_dir1;
	services_dir2 = test_services_dir2;
}

// Extract service name from directory entry.
// Return false if it's not a service file name
static bool get_dirent_svc_name (const char *name, char *svc_name)
{
	size_t len;
	char *dot_pos = strrchr (name, '.');
	if (NULL == dot_pos)
		return false;
	if (strcmp (dot_pos, ".service") != 0)
		return false;
	len = dot_pos - name;
	if ((len > 0) && (name[len-1] == '@'))
		return false;
	strncpy (svc_name, name, len);
	svc_name[len] = '\0';
	return true;
}

// Find all services in specified directory, and add to database.
int find_services_ (const char *services_dir)
{
	int err, service_count;
	struct dirent *dent;
	DIR *dirp = opendir (services_dir);
	char svc_name[SVCAGT_SVC_NAME_BUFLEN];

	if (dirp == NULL) {
		svcagt_log (LEVEL_ERROR, errno, "Could not open systemd directory %s\n", 
			services_dir);
		return -1;
	}

	svcagt_log (LEVEL_INFO, 0, "Finding services in %s\n", services_dir);
	service_count = 0;
	while (1) {
		dent = readdir (dirp);
		if (dent == NULL)
			break;
		if (dent->d_type != DT_REG)
			continue;
		if (get_dirent_svc_name (dent->d_name, svc_name)) {
			err = svcagt_db_add (svc_name);
			if (err == EEXIST)
				continue;
			if (err != 0)
				return err;
			service_count++;
		}
	}

	closedir (dirp);
	svcagt_log (LEVEL_INFO, 0, "Found %d services in %s\n", service_count, services_dir);
	return 0;
}

// Find all the services in either of the service directories,
// and add to the database.
int find_services (void)
{
	int err = find_services_ (services_dir1);
	if (err == 0)
		err = find_services_ (services_dir2);
	return err;
}

static const char *extract_svc_name (char *svc_name)
{
	char *start_pos = strrchr (svc_name, '/');
	char *dot_pos;
	int len;

	if (NULL == start_pos)
		start_pos = svc_name;
	else
		start_pos++;
	dot_pos = strrchr (start_pos, '.');
	if (NULL != dot_pos) {
		if (strcmp (dot_pos, ".service") != 0)
			return NULL;
		len = dot_pos - start_pos;
		if ((len > 0) && (start_pos[len-1] == '@'))
			return NULL;
		*dot_pos = '\0';
	}
	return (const char *) start_pos;
}

// Read the exclusion file and remove from the database all services listed.
int remove_excluded_services (void)
{
	int err, count;
	const char *svc_name;
	char name_buf[SVCAGT_SVC_NAME_BUFLEN];

	count = 0;
	while (1) {
		err = svcagt_exclude_file_read (name_buf);
		if (err != 0)
			break;
		svc_name = extract_svc_name (name_buf);
		if (svc_name != NULL) {
			err = svcagt_db_remove (svc_name);
			if (err == 0)
				count++;
		}
	}
	svcagt_log (LEVEL_INFO, 0, "Excluded %d services\n", count);
	return 0;
}

// Read the goal states file, and start or stop each service listed,
// as specified in the file.
int set_states_from_goals_file (void)
{
	int err = 0;
	int count = 0;
	bool state;
	long file_pos;
	char name_buf[SVCAGT_SVC_NAME_BUFLEN];

	svcagt_log (LEVEL_INFO, 0, "Restoring service states from goals file\n");
	while (1) {
		err = svcagt_goal_state_file_read (name_buf, &state, &file_pos);
		if (err != 0)
			break;
		err = svcagt_set_by_name (name_buf, state, file_pos);
		if (err == 0)
			count++;
	}
	svcagt_log (LEVEL_INFO, 0, "Restored %d service states\n", count);
	if (err == 1) // EOF
		return 0;
	return err;
}

int svcagt_startup_states_init (void)
{
	int err;
	// read /lib/systemd/system and do adds
	// read exclude list and remove
	// read goals file and set states by name
	err = find_services ();
	if (err == 0)
		err = remove_excluded_services ();
	if (err == 0)
		err = set_states_from_goals_file ();
	return err;
}

int svcagt_startup_states_shutdown (void)
{
	return 0;
}
