/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <stdio.h>
#include "svcagt_db.h"
#include "svcagt_log.h"
#include "svcagt_files.h"
#include "svcagt_systemctl.h"
#include "svcagt_startup_states.h"
#include <uthash.h>
#include <utlist.h>
#include "pthread.h"

pthread_mutex_t svcagt_mutex=PTHREAD_MUTEX_INITIALIZER;

const char *running_str = "Running";
const char *stopped_str = "Stopped";

// used for debugging
bool svcagt_suppress_init_states = false;

struct my_service_struct {
	const char *name;
	bool goal_state;
	long state_file_pos;
	UT_hash_handle hh;
};

struct my_service_struct *service_db = NULL;

struct my_service_struct **service_index = NULL;

static const char *states_fname;

static unsigned service_count = 0;


const char *svcagt_goal_state_str (bool state)
{
	if (state)
		return running_str;
	else
		return stopped_str;
}

// Comparison function to be used for the sort
static int sort_cmp (void *a, void *b)
{
	struct my_service_struct *astruct = (struct my_service_struct *) a;
	struct my_service_struct *bstruct = (struct my_service_struct *) b;
	return strcmp (astruct->name, bstruct->name);
}

// Create the service index, to allow queries by index number.
int svcagt_db_init_index (void)
{
	int i;
	struct my_service_struct *s;

	service_count = HASH_COUNT (service_db);

	log_dbg ("Creating service index\n");

	if (service_count == 0) {
		service_index = NULL;
		return 0;
	}

	service_index = (struct my_service_struct **) 
		malloc (service_count * sizeof (struct my_service_struct *));
	if (service_index == NULL) {
		log_error ("Unable to allocate mem for service index\n");
		return -1;
	}

	HASH_SORT (service_db, sort_cmp);
	for (i=0,s=service_db; s != NULL; i++,s=s->hh.next)
		service_index[i] = s;
	return 0;
}

// Loop through the database.  For every service that has not
// gotten an initial state, invoke systemd to get the state,
// and record the state in the database.
int get_remaining_states_from_system ()
{
	struct my_service_struct *s;
	int state;
	unsigned count = 0;

	for (s=service_db; s != NULL; s=s->hh.next) {
		if (s->state_file_pos != -1)
			continue;
		state = svcagt_get_service_state (s->name);
		if (state < 0)
			continue;
		s->goal_state = (bool) state;
		count++;
	}
	log_info ("Got %u states from system\n", count);
	return 0;
}

static void remove_service_node (struct my_service_struct *s)
{
	HASH_DEL (service_db, s);
	free ((void*)s->name);
	free (s);
}

void svcagt_db_shutdown (void)
{
	struct my_service_struct *s, *tmp;
	HASH_ITER (hh, service_db, s, tmp) {
		remove_service_node (s);
	}
	if (service_index != NULL) {
		free (service_index);
		service_index = NULL;
	}
}

int svcagt_db_init(void)
{
	int err;

	service_count = 0;
	if (svcagt_suppress_init_states) // for debugging
		return 0;
	// Find all services, create the database, and set states from
	// the goal states file.
	err = svcagt_startup_states_init ();
	if (err == 0)
		err = get_remaining_states_from_system ();
	if (err == 0)
		err = svcagt_db_init_index ();
	if (err != 0)
		svcagt_db_shutdown ();
	return err;
}

// called during initialization
int svcagt_db_add (const char *name)
{
	size_t name_len;
	char *new_name;
	struct my_service_struct *db_node = NULL;

	if (service_index != NULL)
		return EPERM;

	HASH_FIND_STR (service_db, name, db_node);
	if (db_node != NULL)
		return EEXIST;

	db_node = (struct my_service_struct*) malloc (sizeof (struct my_service_struct));
	if (db_node == NULL) {
		log_error ("Unable to allocate mem for new service db node\n");
		return ENOMEM;
	}
	name_len = strlen (name);
	new_name = (char*) malloc (name_len+1);
	if (new_name == NULL) {
		log_error ("Unable to allocate mem for new service name\n");
		return ENOMEM;
	}
	strcpy (new_name, name);
	db_node->name = new_name;
	db_node->goal_state = false;
	db_node->state_file_pos = -1;
	HASH_ADD_KEYPTR (hh, service_db, db_node->name, name_len, db_node);
	return 0;
}

// called during initialization
int svcagt_db_remove (const char *name)
{
	size_t name_len;

	struct my_service_struct *db_node = NULL;
	if (service_index != NULL)
		return EPERM;

	HASH_FIND_STR (service_db, name, db_node);
	if (db_node == NULL)
		return ENOENT;
	remove_service_node (db_node);
	return 0;
}

static int update_node_state (struct my_service_struct *db_node)
{
	int svc_state = svcagt_get_service_state (db_node->name);
	if (svc_state >= 0) {
		db_node->goal_state = (bool) svc_state;
		return 0;
	}
	return -1;
}

int svcagt_db_get (unsigned index, const char **name, bool *state, bool db_query)
{
	int err = 0;
	struct my_service_struct *db_node;

	if (index >= service_count) {
		log_dbg ("Invalid index %u provided to svcagt_db_get\n", index);
		*name = NULL;
		*state = -1;
		return EINVAL;
	}

	pthread_mutex_lock (&svcagt_mutex);
	db_node = service_index[index];
	if (!db_query) {
		log_dbg ("Updating node state\n");
		err = update_node_state (db_node);
	}
	*name = db_node->name;
	*state = db_node->goal_state;			
	pthread_mutex_unlock (&svcagt_mutex);
	return err;
}

// called during initialization
int svcagt_set_by_name (const char *name, bool state, long state_file_pos)
{
	int err;
	struct my_service_struct *db_node;

	HASH_FIND_STR (service_db, name, db_node);
	if (db_node == NULL) {
		log_dbg ("set by name %s not found\n", name);
		return EINVAL;
	}
	db_node->goal_state = state;
	if (state_file_pos != -1) {
		db_node->state_file_pos = state_file_pos;
		svcagt_set_service_state (db_node->name, state);
	}
	return 0;
}

int svcagt_db_set (unsigned index, bool state)
{
	int err;
	bool new_value = state;
	struct my_service_struct *db_node;

	if (index >= service_count) {
		log_dbg ("Invalid index %u provided to svcagt_db_set\n", index);
		return EINVAL;
	}

	pthread_mutex_lock (&svcagt_mutex);
	db_node = service_index[index];
	if (db_node->state_file_pos == -1) {
		long file_pos;
		svcagt_set_service_state (db_node->name, new_value);
		log_dbg ("Appending %s to goal state file\n", db_node->name);
		err = svcagt_goal_state_file_append (db_node->name, new_value, &file_pos);
		if (err == 0)
			db_node->state_file_pos = file_pos;
	} else if (new_value != db_node->goal_state) {
		svcagt_set_service_state (db_node->name, new_value);
		log_dbg ("Updating %s:%d in goal state file at pos %ld\n", 
			db_node->name, new_value, db_node->state_file_pos);
		err = svcagt_goal_state_file_update (db_node->state_file_pos, new_value);
	}
	db_node->goal_state = new_value;
	pthread_mutex_unlock (&svcagt_mutex);
	return 0;
}

int svcagt_db_get_all (service_list_item_t **service_list, bool db_query)
{
	int i, err;
	struct my_service_struct *db_node;
	service_list_item_t *item_array;
	service_list_item_t *item_list = NULL;

	if (service_index == NULL) {
		*service_list = NULL;
		return 0;
	}
	item_array = (service_list_item_t *) malloc (service_count * sizeof (service_list_item_t));
	if (item_array == NULL) {
		log_error ("Unable to allocate mem for service list\n");
		return -1;
	}
	pthread_mutex_lock (&svcagt_mutex);
	for (i=0; i<service_count; i++) {
		item_array[i].index = i;
		db_node = service_index[i];
		if (!db_query) {
			update_node_state (db_node);
		}
		item_array[i].svc_info.svc_name = db_node->name;
		item_array[i].svc_info.goal_state = 
			svcagt_goal_state_str (db_node->goal_state);
		DL_APPEND (item_list, &item_array[i]);
	}
	*service_list = item_list;
	pthread_mutex_unlock (&svcagt_mutex);
	return service_count;
}

void svcagt_db_remove_service_list (service_list_item_t *service_list)
{
	free (service_list);
}

// For testing
void svcagt_show_service_db (void)
{
	struct my_service_struct *s;
	printf ("Service DB List\n");
	for (s=service_db; s != NULL; s=s->hh.next)
		printf ("  %s goal: %d, pos: %ld\n",
			s->name, s->goal_state, s->state_file_pos);
	printf ("End of Service DB List\n");
}

// For testing
unsigned svcagt_show_service_index (void)
{
	int i;
	struct my_service_struct *s;
	printf ("Service DB Index\n");
	for (i=0; i<service_count; i++) {
		s = service_index[i];
		printf ("  %s goal: %d, pos: %ld\n",
			s->name, s->goal_state, s->state_file_pos);
	}
	printf ("End of Service DB Index\n");
	return service_count;
}

// For testing
void svcagt_show_service_list (service_list_item_t *service_list)
{
	service_list_item_t *current;
	printf ("Service List\n");
	DL_FOREACH (service_list, current) {
		printf ("%u %s goal: %s\n", current->index,
			current->svc_info.svc_name, 
			current->svc_info.goal_state);
	}
	printf ("End of Service List\n");
}
