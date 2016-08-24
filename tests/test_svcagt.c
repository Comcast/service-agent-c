// test_svcagt.c
//
//
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <svc_agt.h>
#include <svcagt_db.h>
#include <svcagt_files.h>
#include <svcagt_time.h>
#include <utlist.h>

extern int svcagt_db_init_index (void);
extern void svcagt_show_service_db (void);
extern unsigned svcagt_show_service_index (void);
extern void svcagt_show_service_list (service_list_item_t *service_list, const char *title);
extern const char *svcagt_goal_state_str (bool state);
extern bool svcagt_suppress_init_states;
extern void set_test_services (const char *test_services_dir1, const char *test_services_dir2);
extern const char *svcagt_systemctl_cmd;
extern void dbg (const char *fmt, ...);
extern void dbg_err (int err, const char *fmt, ...);

static const char *running_str = "Running";
static const char *stopped_str = "Stopped";

#define MOCK_PID_FILE_NAME "mock_systemctl_pid.txt"

int check_mock_systemctl_running (void)
{
	int err;
	char *pos;
	FILE *mock_pid_file;
	const char *home_dir;
	char name_buf[128];
	char pid_buf[32];
	struct stat stat_buf;

	home_dir = getenv ("HOME");
	if (NULL == home_dir) {
		printf ("No $HOME defined\n");
		return ENOENT;
	}

	sprintf (name_buf, "%s/%s", home_dir, MOCK_PID_FILE_NAME);

	mock_pid_file = fopen (name_buf, "r");
	if (NULL == mock_pid_file) {
		printf ("mock_systemctl should be started\n");
		printf ("file %s could not be opened\n", name_buf);
		return -1;
	}

	if (NULL == fgets (pid_buf, 32, mock_pid_file)) {
		printf ("mock_systemctl should be started\n");
		printf ("file %s is invalid\n", name_buf);
		return -1;
	}
	pos = strchr (pid_buf, '\n');
	if (pos != NULL)
		*pos = 0;

	pid_buf[31] = 0;
	fclose (mock_pid_file);

	sprintf (name_buf, "/proc/%s", pid_buf);

	err = stat (name_buf, &stat_buf);
	if (err != 0) {
		printf ("mock_systemctl should be started\n");
		dbg_err (errno, "unable to stat %s\n", name_buf);
		return -1;
	}
	if (!S_ISDIR (stat_buf.st_mode)) {
		printf ("mock_systemctl should be started\n");
		printf ("%s should be a directory\n", name_buf);
		return -1;
	}

	return 0;
}


int load_goal_states_list (service_list_item_t **service_list)
{
	int err;
	int index;
	bool goal_state;
	long file_pos;
	//int fd, nfields, state_;
	//FILE *fp;
	service_list_item_t *item_list = NULL;
	service_list_item_t *item;
	char svc_name_buf[128];
	char *svc_name;

	err = svcagt_goal_state_file_rewind ();
	if (err != 0)
		return err;
#if 0
	fd = open ("./svcagt_goal_states.txt", O_RDONLY, 0666);
	if (fd < 0) {
		printf ("Error(1) opening goal states file\n");
		return -1;
	}
	fp = fdopen (fd, "r");
	if (fp == NULL) {
		printf ("Error(2) opening goal states file\n");
		return -1;
	}
#endif

	index = 0;
	while (true) {
		err = svcagt_goal_state_file_read (svc_name_buf, &goal_state, &file_pos);
		if (err == 1)
			break;
		if (err != 0)
			return -1;
#if 0
		nfields = fscanf (fp, "%d %s\n", &state_, svc_name_buf);
		if (nfields == EOF) {
			break;
		}
		if (nfields != 2) {
			printf ("Format error in goal states file\n");
			return -1;
		}
		if (state_ == (int)true)
			goal_state = true;
		else if (state_ == (int)false)
			goal_state = false;
		else {
			printf ("Invalid state %d in goal_states file\n", state_);
			return -1;
		}
#endif
		svc_name = (char *) malloc (strlen(svc_name_buf) + 1);
		if (svc_name == NULL) {
			printf ("Unable to allocate mem for service name\n");
			return -1;
		}
		strcpy (svc_name, svc_name_buf);
		item = (service_list_item_t *) malloc (sizeof (service_list_item_t));
		if (item == NULL) {
			printf ("Unable to allocate mem for service list item\n");
			return -1;
		}
		item->index = index;
		item->svc_info.svc_name = svc_name;
		item->svc_info.goal_state = svcagt_goal_state_str (goal_state);
		DL_APPEND (item_list, item);
		index++;
	}

	//fclose (fp);
	*service_list = item_list;
	return 0;
}

int vmake_svc_list (service_list_item_t **service_list, va_list ap)
{
	int err, index;
	bool goal_state;
	service_list_item_t *item_list = NULL;
	service_list_item_t *item;
	char svc_name_buf[128];
	char *svc_name, *arg;

	index = 0;
	while (true) {
		arg = va_arg (ap, char *);
		if (arg == NULL)
			break;
		if (arg[0] == '1')
			goal_state = true;
		else if (arg[0] == '0')
			goal_state = false;
		else {
			printf ("Invalid state %c in goal_states file\n", arg[0]);
			return -1;
		}
		svc_name = (char *) malloc (strlen(arg));
		if (svc_name == NULL) {
			printf ("Unable to allocate mem for service name\n");
			return -1;
		}
		strcpy (svc_name, arg+1);
		item = (service_list_item_t *) malloc (sizeof (service_list_item_t));
		if (item == NULL) {
			printf ("Unable to allocate mem for service list item\n");
			return -1;
		}
		//printf ("Adding %s to list\n", svc_name);
		item->index = index;
		item->svc_info.svc_name = svc_name;
		item->svc_info.goal_state = svcagt_goal_state_str (goal_state);
		DL_APPEND (item_list, item);
		index++;
	}

	*service_list = item_list;
	return 0;
}

int make_svc_list (service_list_item_t **service_list, ...)
{
	int result;
	va_list ap;
	va_start (ap, service_list);
	result = vmake_svc_list (service_list, ap);
	va_end (ap);
	return result;
}

void remove_service_list_item (service_list_item_t **service_list, 
	service_list_item_t *item)
{
	free ((void*)item->svc_info.svc_name);
	DL_DELETE (*service_list, item);
	free (item);
}

void remove_service_list (service_list_item_t *service_list)
{
	service_list_item_t *item, *tmp;

	DL_FOREACH_SAFE (service_list, item, tmp) {
		remove_service_list_item (&service_list, item);
	}
}


int make_index_list (service_list_item_t **service_list, ...)
{
	int result;
	unsigned index;
	bool state;
	va_list ap;
	service_list_item_t *item_list = NULL;
	service_list_item_t *item, *tmp;

	va_start (ap, service_list);
	result = vmake_svc_list (&item_list, ap);
	va_end (ap);
	if (result != 0)
		return result;
	DL_FOREACH_SAFE (item_list, item, tmp) {
		result = svcagt_get_index (item->svc_info.svc_name, &index, &state);
		if (result != 0) {
			remove_service_list_item (&item_list, item);
			continue;
		}
		if (state) {
			remove_service_list_item (&item_list, item);
			continue;
		}
		item->index = index;
		item->svc_info.goal_state = svcagt_goal_state_str (state);
	}
	*service_list = item_list;
	return 0;
}

int get_index (const char *name)
{
	int result;
	unsigned index;
	bool state;

	result = svcagt_get_index (name, &index, &state);
	if (result != 0)
		return -1;
	if (state)
		return -1;
	printf ("Index for %s is %u\n", name, index);
	return (int) index;	
}

bool lists_equal (service_list_item_t *list1, service_list_item_t *list2)
{
	while (true) {
		if ((list1 == NULL) && (list2 == NULL))
			return true;
		if (list1 == NULL)
			return false;
		if (list2 == NULL)
			return false;
		if (strcmp (list1->svc_info.svc_name, list2->svc_info.svc_name) != 0)
			return false;
		if (strcmp (list1->svc_info.goal_state, list2->svc_info.goal_state) != 0)
			return false;
		list1 = list1->next;
		list2 = list2->next;
	}
}

int vlist_equals (service_list_item_t *service_list, va_list ap)
{
	int result;
	service_list_item_t *my_list;
	result = vmake_svc_list (&my_list, ap);
	if (result != 0)
		return -1;
	result = (int) lists_equal (service_list, my_list);
	remove_service_list (my_list);
	return result;
}

int list_equals (service_list_item_t *service_list, ...)
{
	int result;
	va_list ap;
	va_start (ap, service_list);
	result = vlist_equals (service_list, ap);
	va_end (ap);
	return result;
}

int cmp_svc_info (service_list_item_t *a, service_list_item_t *b)
{
	int	result = strcmp (a->svc_info.svc_name, b->svc_info.svc_name);
	if (result == 0)
		result = strcmp (a->svc_info.goal_state, b->svc_info.goal_state);
	return result;
}

int sort_cmp (void *a, void *b)
{
	service_list_item_t *svc_item_a = (service_list_item_t *) a;
	service_list_item_t *svc_item_b = (service_list_item_t *) b;
	return cmp_svc_info (svc_item_a, svc_item_b);
}

int get_all_eq_test (bool expected, ...)
{
	int err, result;
	va_list ap;
	service_list_item_t *service_list;

	err = svc_agt_get_all (&service_list, false);
	if (err < 0) {
		printf ("FAIL: svc_agt_get_all error\n");
		return -1;
	}
	DL_SORT (service_list, sort_cmp);
	//svcagt_show_service_list (service_list, "Goal States Eq Test");
	va_start (ap, expected);
	result = vlist_equals (service_list, ap);
	va_end (ap);
	svc_agt_remove_service_list (service_list);
	if (expected) {
		if (result <= 0) {
			printf ("FAIL: svc_agt get_all list does not match, result %d\n", result);
			return -1;
		}
		printf ("SUCCESS: svc_agt get_all list matches\n");
	} else {
		if (result <= 0) {
			printf ("SUCCESS: svc agt get_all list does not match, as it should not\n");
		} else {
			printf ("FAIL: svc agt get_all list matches (but should not)\n");
			return -1;
		}
	}
	return 0;
}

int goal_states_file_eq_test (bool expected, ...)
{
	int err, result;
	va_list ap;
	service_list_item_t *service_list;

	err = load_goal_states_list (&service_list);
	if (err != 0)
		return -1;
	//svcagt_show_service_list (service_list, "Goal States Eq Test");
	va_start (ap, expected);
	result = vlist_equals (service_list, ap);
	va_end (ap);
	remove_service_list (service_list);
	if (expected) {
		if (result <= 0) {
			printf ("FAIL: goal states file does not match, result %d\n", result);
			return -1;
		}
		printf ("SUCCESS: goal states file matches\n");
	} else {
		if (result <= 0) {
			printf ("SUCCESS: goal states file does not match, as it should not\n");
		} else {
			printf ("FAIL: goal states file matches (but should not)\n");
			return -1;
		}
	}
	return 0;
}

bool list1_in_list2 (service_list_item_t *list1, service_list_item_t *list2)
// assumes the lists are sorted
{
	int result;
	while (true) {
		if (list1 == NULL)
			return true;
		if (list2 == NULL)
			return false;
		result = cmp_svc_info (list1, list2);
		if (result < 0)
			return false;
		if (result > 0) {
			list2 = list2->next;
			continue;
		}
		list1 = list1->next;
		list2 = list2->next;
	}
}

bool list_contains_dups (service_list_item_t *service_list)
// assumes the list is sorted
{
	service_list_item_t *item;
	service_list_item_t *prev = NULL;

	DL_FOREACH (service_list, item) {
		if (prev != NULL) {
			if (strcmp (item->svc_info.svc_name, prev->svc_info.svc_name) == 0)
				return true;
		}
		prev = item;
	}
	return false;
}

int vlist_contains (service_list_item_t *service_list, va_list ap)
// assumes the service list is sorted
{
	int result;
	service_list_item_t *my_list;
	result = vmake_svc_list (&my_list, ap);
	if (result != 0)
		return -1;
	DL_SORT (my_list, sort_cmp);
	result = (int) list1_in_list2 (my_list, service_list);
	remove_service_list (my_list);
	return result;
}

int list_contains (service_list_item_t *service_list, ...)
// assumes the service list is sorted
{
	int result;
	va_list ap;
	va_start (ap, service_list);
	result = vlist_contains (service_list, ap);
	va_end (ap);
	return result;
}

int goal_states_file_contains_dups_test (bool dups_expected)
{
	int err;
	bool result;
	service_list_item_t *service_list;

	err = load_goal_states_list (&service_list);
	if (err != 0)
		return -1;
	//svcagt_show_service_list (service_list, "Goal States Contains Test");
	DL_SORT (service_list, sort_cmp);
	result = list_contains_dups (service_list);
	remove_service_list (service_list);
	if (!dups_expected) {
		if (result) {
			printf ("FAIL: goal_states file contains dups\n");
			return -1;
		}
		printf ("SUCCESS: goal states file contains no dups\n");
	} else {
		if (result)
			printf ("SUCCESS: goal_states file contains dups, as expected\n");
		else {
			printf ("FAIL: goal states file should have dups\n");
			return -1;
		}
	}
	return 0;
}

int get_all_contains_test (bool expected, ...)
{
	int err, result;
	va_list ap;
	service_list_item_t *service_list;

	err = svc_agt_get_all (&service_list, false);
	if (err < 0) {
		printf ("FAIL: svc_agt_get_all error\n");
		return -1;
	}
	DL_SORT (service_list, sort_cmp);
	//svcagt_show_service_list (service_list, "Goal States Eq Test");
	va_start (ap, expected);
	result = vlist_contains (service_list, ap);
	va_end (ap);
	svc_agt_remove_service_list (service_list);
	if (expected) {
		if (result <= 0) {
			printf ("FAIL: svc_agt get_all list does not contain expected values, result %d\n", 
				result);
			return -1;
		}
		printf ("SUCCESS: svc_agt get_all list contains expected values\n");
	} else {
		if (result <= 0) {
			printf ("SUCCESS: svc agt get_all list does not contain values it should not\n");
		} else {
			printf ("FAIL: svc agt get_all list contains values (but should not)\n");
			return -1;
		}
	}
	return 0;
}

int goal_states_file_contains_test (bool expected, ...)
{
	int err, result;
	va_list ap;
	service_list_item_t *service_list;

	err = load_goal_states_list (&service_list);
	if (err != 0)
		return -1;
	//svcagt_show_service_list (service_list, "Goal States Contains Test");
	DL_SORT (service_list, sort_cmp);
	va_start (ap, expected);
	result = vlist_contains (service_list, ap);
	va_end (ap);
	remove_service_list (service_list);
	if (expected) {
		if (result <= 0) {
			printf ("FAIL: goal states file does not contain expected values, result %d\n", 
				result);
			return -1;
		}
		printf ("SUCCESS: goal states file contains expected values\n");
	} else {
		if (result <= 0) {
			printf ("SUCCESS: goal states file does not contain values that it should not\n");
		} else {
			printf ("FAIL: goal states file contains values that it should not\n");
			return -1;
		}
	}
	return 0;
}

int get_test (unsigned index, const char *name, bool expected)
{
	int err;
	service_info_t info;
	const char *expected_str;

	err = svc_agt_get (index, &info, false);
	if (err != 0)
		return err;
	if (strcmp (info.svc_name, name) != 0) {
		printf ("FAIL: get test expected name %s but got %s\n",
			name, info.svc_name);
		return -1;
	}
	expected_str = svcagt_goal_state_str (expected);
	if (strcmp (info.goal_state, expected_str) != 0) {
		printf ("FAIL: get test expected state %s but got %s\n",
			expected_str, info.goal_state);
		return -1;
	}
	printf ("SUCCESS: expected and got %s %s\n", info.svc_name, info.goal_state);
	return 0;
}


int show_goal_states_file (void)
{
	int err;
	service_list_item_t *service_list;

	err = load_goal_states_list (&service_list);
	if (err != 0)
		return -1;
	svcagt_show_service_list (service_list, "Goal States File");
	remove_service_list (service_list);
	return 0;
}

void delay (unsigned long delay_secs)
{
	unsigned long secs;
	while (delay_secs > 0) {
		if (delay_secs > 5)
			secs = 5;  
		else
			secs = (int) delay_secs;
		printf ("Counting down %lu\n", delay_secs);
		sleep ((int)secs);
		delay_secs -= secs;
	}
}

int pass_fail_tests (void)
{
	int err;
	const char *name;
	bool state;
	service_list_item_t *service_list, *list2;
	int sendmail_index = -1;
	int winbind_index = -1;
	int upower_index = -1;
	int sshd_index = -1;
	int sheepdog_index = -1;
	int sm_client_index = -1;


	err = make_svc_list (&service_list, "1service1", "0sajwt1", "0winbind",
		"1utlist", NULL);
	if (err == 0) {
		svcagt_show_service_list (service_list, NULL);
	}
	err = make_svc_list (&list2, "1service1", "0sajwt1", "0winbind",
		"1utlist", NULL);
	if (err == 0) {
		svcagt_show_service_list (list2, NULL);
		if (lists_equal (service_list, list2))
			printf ("SUCCESS: Lists are equal\n");
		else
			printf ("FAIL: Lists are not equal\n");
		remove_service_list (service_list);
		err = make_svc_list (&service_list, "1service1", "0sajwt1", "0winbind",
			"1utlist", "1zzlist2", NULL);
	}
	if (err == 0) {
		if (lists_equal (service_list, list2))
			printf ("FAIL: Lists are equal, but should not be\n");
		else
			printf ("SUCCESS: lists are not equal and should not be\n");
		err = list_equals (list2, "1service1", "0sajwt1", "0winbind",
			"1utlist", NULL);
		printf ("List equals result %d (should be 1)\n", err);
		err = list_equals (list2, "1service2", "0sajwt1", "0winbind",
			"1utlist", NULL);
		printf ("List equals result %d (should be 0)\n", err);
		DL_SORT (service_list, sort_cmp);
		DL_SORT (list2, sort_cmp);
		if (list1_in_list2 (list2, service_list))
			printf ("SUCCESS: list2 is contained in service list as it should be)\n");
		else
			printf ("FAIL: list2 not contained in service list\n");
		err = list_contains (service_list, "0sajwt1", "1service1", "1utlist", NULL);
		printf ("List contains result %d (should be 1)\n", err);
		err = list_contains (service_list, "0sajwt1", "1service1", "1utlist", "0zzlist2", NULL);
		printf ("List contains result %d (should be 0)\n", err);
		remove_service_list (service_list);
		remove_service_list (list2);
	}

	err = svc_agt_init ("nosuch");
	if (err == 0) {
		printf ("FAIL: svc_agt_init of non-existent directory should fail\n");
		svc_agt_shutdown ();
	} else
		printf ("SUCCESS: svc_sgt_init of non-existent directory failed, as it should\n"); 

	svcagt_suppress_init_states = true;
	err = svc_agt_init (".");
	if (err != 0)
		return 0;


	#if 0
	// not using namebuf
		for (i=0; i<40; i++) {
			sprintf (test_name, "TestFileNameWhichIsPrettyLong%d", i);
			ptr = svcagt_add_name (test_name);
			printf ("Added name is %s\n", ptr);
		}
	#endif
	
		svcagt_db_add ("service4");
		svcagt_db_add ("service3");
		svcagt_db_add ("service2");
		svcagt_db_add ("service1");
		svcagt_show_service_db ();

		svcagt_db_remove ("service1");
		svcagt_db_remove ("nosuch");
		svcagt_db_init_index ();
		svcagt_show_service_index ();
	
		err = svcagt_db_get (0, &name, &state, true);
		if (err == 0) {
			printf ("SUCCESS: Get 0: %s goal: %d\n", name,	state);
		} else {
			printf ("FAIL: Get 0 (service2) failed\n");
		}
		err = svcagt_set_by_name ("service4", true, 22L);
		err = svcagt_db_get (2, &name, &state, true);
		if (err == 0) {
			printf ("SUCCESS: Get 2: %s goal: %d\n", name,	state);
		} else {
			printf ("FAIL: Get 2 (service4) failed\n");
		}
	
		err = svcagt_db_get (3, &name, &state, true);
		if (err == 0) {
			printf ("FAIL: Should not get 3: %s goal: %d\n", name,	state);
		} else {
			printf ("SUCCESS: Get 3 (service1) failed (expected)\n");
		}

		err = svcagt_db_set (0, true);
		if (err == 0)
			err = svcagt_db_get (0, &name, &state, true);
		if (err == 0) {
			printf ("Set/Get 0: %s goal: %d\n", name,	state);
			if (state)
				printf ("SUCCESS: Set/Get 0 (service2) true\n");
			else
				printf ("FAIL: Set/Get 0 (service2) should be true\n"); 
		} else {
			printf ("FAIL: Set/Get 0 (service2) failed\n");
		}
			
		err = svcagt_db_set (1, false);
		if (err == 0)
			err = svcagt_db_get (1, &name, &state, true);
		if (err == 0) {
			printf ("Set/Get 1: %s goal: %d\n", name,	state);
			if (!state)
				printf ("SUCCESS: Set/Get 1 (service3) false\n");
			else
				printf ("FAIL: Set/Get 1 (service3) should be false\n"); 
		} else {
			printf ("FAIL: Set/Get 1 failed\n");
		}

		err = get_all_eq_test (true, "1service2", "0service3", "1service4", NULL);
		svcagt_show_service_db ();


	if (err == 0) {
		err = svc_agt_get_all (&service_list, true);
		if (err >= 0) {
			svcagt_show_service_list (service_list, "Gets/Sets");
			svc_agt_remove_service_list (service_list);
		}
	} else {
		printf ("Error %d on svcagt_db_get_all\n", err);
	}
	svc_agt_shutdown ();

	err = remove ("./svcagt_goal_states.txt");
	if ((err != 0) && (err != ENOENT)) {
		dbg_err (errno, "Error removing goal states file\n");
		return -1;
	}

	svcagt_suppress_init_states = false;
	set_test_services ("./mock_systemd_libs/etc", "./mock_systemd_libs/usr");

	err = svc_agt_init (".");
	if (err == 0)
		err = show_goal_states_file ();

	sendmail_index = get_index ("sendmail");
	winbind_index = get_index ("winbind");
	upower_index = get_index ("upower");
	sshd_index = get_index ("sshd");
	sheepdog_index = get_index ("sheepdog");
	sm_client_index = get_index ("sm-client");

	if ((err == 0) && (sendmail_index >= 0)) {
		err = svc_agt_set ((unsigned)sendmail_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_eq_test (true, "1sendmail", NULL);
		if (err == 0)
			err = get_test ((unsigned)sendmail_index, "sendmail", true);
		if (err == 0)
			err = get_all_contains_test (true, "1sendmail", NULL);
		svcagt_show_service_db ();
	}
	if ((err == 0) && (winbind_index >= 0)) {
		err = svc_agt_set ((unsigned)winbind_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_contains_dups_test (false);
		if (err == 0)
			err = goal_states_file_eq_test (true, "1sendmail", "1winbind", NULL);
		if (err == 0)
			err = get_test ((unsigned)winbind_index, "winbind", true);
		if (err == 0)
			err = svc_agt_set ((unsigned)winbind_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_contains_dups_test (false);
		if (err == 0)
			err = goal_states_file_eq_test (true, "1sendmail", "1winbind", NULL);
	}
	if ((err == 0) && (sshd_index >= 0)) {
		err = svc_agt_set ((unsigned)sshd_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_contains_dups_test (false);
		if (err == 0)
			err = goal_states_file_eq_test (true, "1sendmail", "1winbind", "1sshd", NULL);
		if (err == 0)
			err = get_all_contains_test (true, "1sendmail", "1sshd", "1winbind", NULL);
		if (err == 0)
			err = get_test ((unsigned)sshd_index, "sshd", true);
		if (err == 0)
			err = goal_states_file_eq_test (false, "1sendmail", "1winbind", "0sshd", NULL);
		if (err == 0)
			err = goal_states_file_eq_test (false, "1sendmail", "1winbind", NULL);
	}
	if ((err == 0) && (sm_client_index >= 0)) {
		err = svc_agt_set ((unsigned)sm_client_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_contains_dups_test (false);
		if (err == 0)
			err = goal_states_file_eq_test 
				(true, "1sendmail", "1winbind", "1sshd", "1sm-client", NULL);
		if (err == 0)
			err = goal_states_file_contains_test
				(true, "1sendmail", "1winbind", "1sshd", NULL);
		if (err == 0)
			err = goal_states_file_contains_test
				(false, "1sendmail", "1winbindd", "1sshd", NULL);
		if (err == 0)
			err = goal_states_file_contains_test
				(false, "1sendmail", "1winbind", "1sshd", "1upower", NULL);
		if (err == 0)
			err = svc_agt_set ((unsigned)sm_client_index, svcagt_goal_state_str(true));
		if (err == 0)
			err = goal_states_file_contains_dups_test (false);
		if (err == 0)
			err = svc_agt_set ((unsigned)sm_client_index, svcagt_goal_state_str(false));
		if (err == 0)
			err = goal_states_file_eq_test 
				(true, "1sendmail", "1winbind", "1sshd", "0sm-client", NULL);
		if (err == 0)
			err = get_all_contains_test (true, "1sendmail", "0sm-client", "1sshd", "1winbind", NULL);
		if (err == 0)
			err = get_test ((unsigned)sm_client_index, "sm-client", false);
	}
	err = show_goal_states_file ();
				
	svc_agt_shutdown ();
	return err;

}

int main(int argc, char *argv[])
{
	int i, err;
	char timestamp[20];
	char test_name[100];
	const char *name;
	bool state;
	long file_pos;
	const char *arg;
	const char *ptr;
	service_info_t svc_info;
	service_list_item_t *service_list;
	int sajwt1_index = -1;
	int sajwt2_index = -1;
	int sajwt3_index = -1;

	err = make_current_timestamp (timestamp);
	if (err == 0)
		printf ("Current time is %s\n", timestamp);
	else
		printf ("make timestamp error %d\n", err);

	svcagt_systemctl_cmd = "./mock_systemctl";

	if (check_mock_systemctl_running() != 0)
		return 4;

	if (argc <= 1)
		return pass_fail_tests ();

	arg = argv[1];

	if (arg[0] == 's') {	
		unsigned service_count;
		unsigned long delay_secs = 0;

		if (argc >= 3) {
			delay_secs = strtoul (argv[2], NULL, 10);
			if (delay_secs > 60)
				delay_secs = 60;
		}
		if (delay_secs == 0)
			delay_secs = 15;
		svcagt_suppress_init_states = false;
		err = svc_agt_init (".");
		if (err != 0)
			return 0;
		sajwt1_index = get_index ("sajwt1");
		if (sajwt1_index == -1) {
			printf ("sajwt1_index not found\n");
			return 4;
		}
		sajwt2_index = get_index ("sajwt2");
		if (sajwt2_index == -1) {
			printf ("sajwt2_index not found\n");
			return 4;
		}
		sajwt3_index = get_index ("sajwt3");
		if (sajwt3_index == -1) {
			printf ("sajwt3_index not found\n");
			return 4;
		}

		service_count = svcagt_show_service_index ();
		if (service_count > 0) {
			err = svc_agt_set (sajwt1_index, running_str);
			err = svc_agt_get (sajwt1_index, &svc_info, false);
			if (err == 0) {
				printf ("Get 1: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
				if (strcmp (svc_info.goal_state, running_str) == 0)
					printf ("SUCCESS: Get 1 %s\n", running_str);
				else
					printf ("FAIL: Get 1 should be %s\n", running_str);
			} else {
				printf ("FAIL: Get 1 (sajwt1) failed\n");
			}
			delay (delay_secs);
			err = svc_agt_set (sajwt2_index, running_str);
			err = svc_agt_get (sajwt2_index, &svc_info, false);
			if (err == 0) {
				printf ("Get 2: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
				if (strcmp (svc_info.goal_state, running_str) == 0)
					printf ("SUCCESS: Get 2 %s\n", running_str);
				else
					printf ("FAIL: Get 2 should be %s\n", running_str);
			} else {
				printf ("FAIL: Get 2 (sajwt2) failed\n");
			}
			err = svc_agt_set (sajwt1_index, stopped_str);
			delay (delay_secs);
			err = svc_agt_set (sajwt3_index, running_str);
			err = svc_agt_set (sajwt2_index, stopped_str);
			delay (delay_secs);
			err = svc_agt_set (sajwt3_index, stopped_str);

			err = svc_agt_get (sajwt1_index, &svc_info, false);
			if (err == 0) {
				printf ("Get 1: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
				if (strcmp (svc_info.goal_state, stopped_str) == 0)
					printf ("SUCCESS: Get 1 %s\n", stopped_str);
				else
					printf ("FAIL: Get 1 should be %s\n", stopped_str);
			} else {
				printf ("FAIL: Get 1 (sajwt1) failed\n");
			}

			err = svc_agt_get (sajwt2_index, &svc_info, false);
			if (err == 0) {
				printf ("Get 2: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
				if (strcmp (svc_info.goal_state, stopped_str) == 0)
					printf ("SUCCESS: Get 2 %s\n", stopped_str);
				else
					printf ("FAIL: Get 2 should be %s\n", stopped_str);
			} else {
				printf ("FAIL: Get 2 (sajwt2) failed\n");
			}

			err = svc_agt_get (sajwt3_index, &svc_info, false);
			if (err == 0) {
				printf ("Get 3: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
				if (strcmp (svc_info.goal_state, stopped_str) == 0)
					printf ("SUCCESS: Get 3 %s\n", stopped_str);
				else
					printf ("FAIL: Get 3 should be %s\n", stopped_str);
			} else {
				printf ("FAIL: Get 3 (sajwt3) failed\n");
			}

		}
		svcagt_show_service_db ();
		err = svc_agt_get_all (&service_list, false);
		
		if (err >= 0) {
			svcagt_show_service_list (service_list, "Gets/Sets");
			svc_agt_remove_service_list (service_list);
		} else {
			printf ("Error %d on svcagt_db_get_all\n", err);
		}
		svc_agt_shutdown ();

	} else {

		printf ("Invalid argument %s\n", arg);
	}



	printf ("Test done!\n");
}
