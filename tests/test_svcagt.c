// test_svcagt.c
//
//
//

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <svc_agt.h>
#include <svcagt_db.h>
#include <svcagt_time.h>
#include <utlist.h>

extern int svcagt_db_init_index (void);
extern void svcagt_show_service_db (void);
extern unsigned svcagt_show_service_index (void);
extern void svcagt_show_service_list (service_list_item_t *service_list);
extern const char *svcagt_goal_state_str (bool state);
extern bool svcagt_suppress_init_states;
extern const char *svcagt_systemctl_cmd;

static const char *running_str = "Running";
static const char *stopped_str = "Stopped";

int load_goal_states_list (service_list_item_t **service_list)
{
	int err, fd, nfields, state_;
	int index;
	bool goal_state;
	FILE *fp;
	service_list_item_t *item_list = NULL;
	service_list_item_t *item;
	char svc_name_buf[128];
	char *svc_name;

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

	index = 0;
	while (true) {
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

	fclose (fp);
	*service_list = item_list;
	return 0;
}

int make_svc_list (service_list_item_t **service_list, ...)
{
	va_list ap;
	int err, index;
	bool goal_state;
	service_list_item_t *item_list = NULL;
	service_list_item_t *item;
	char svc_name_buf[128];
	char *svc_name, *arg;

	va_start (ap, service_list);

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
			va_end (ap);
			return -1;
		}
		svc_name = (char *) malloc (strlen(arg));
		if (svc_name == NULL) {
			printf ("Unable to allocate mem for service name\n");
			va_end (ap);
			return -1;
		}
		strcpy (svc_name, arg+1);
		item = (service_list_item_t *) malloc (sizeof (service_list_item_t));
		if (item == NULL) {
			printf ("Unable to allocate mem for service list item\n");
			va_end (ap);
			return -1;
		}
		item->index = index;
		item->svc_info.svc_name = svc_name;
		item->svc_info.goal_state = svcagt_goal_state_str (goal_state);
		DL_APPEND (item_list, item);
		index++;
	}

	va_end (ap);
	*service_list = item_list;
	return 0;
}

void remove_service_list (service_list_item_t *service_list)
{
	service_list_item_t *item, *tmp;

	DL_FOREACH_SAFE (service_list, item, tmp) {
		free ((void*)item->svc_info.svc_name);
		DL_DELETE (service_list, item);
		free (item);
	}
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


int show_goal_states_file (void)
{
	int err;
	service_list_item_t *service_list;

	err = load_goal_states_list (&service_list);
	if (err != 0)
		return -1;
	svcagt_show_service_list (service_list);
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
	service_list_item_t *service_list;

	err = make_svc_list (&service_list, "1service1", "0sajwt1", "0winbind",
		"1utlist", NULL);
	if (err == 0) {
		svcagt_show_service_list (service_list);
		remove_service_list (service_list);
	}

	svcagt_suppress_init_states = false;
	err = svc_agt_init (".");
	if (err == 0)
		err = show_goal_states_file ();
	if (err == 0)
		err = svcagt_db_get (1, &name, &state, false);
	if (err == 0) {
		state = !state;
		err = svc_agt_set (1, svcagt_goal_state_str (state));
	}
	if (err == 0)
		err = show_goal_states_file ();
	if (err == 0) {
		state = !state;
		err = svc_agt_set (1, svcagt_goal_state_str (state));
	}
	if (err == 0)
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

	err = make_current_timestamp (timestamp);
	if (err == 0)
		printf ("Current time is %s\n", timestamp);
	else
		printf ("make timestamp error %d\n", err);

	svcagt_systemctl_cmd = "./mock_systemctl";

	if (argc <= 1)
		return pass_fail_tests ();

	arg = argv[1];

	if (arg[0] == 's') {	
		unsigned service_count;
		unsigned long delay_secs = 0;

		if (strcmp (arg, "baddir") == 0) {
			err = svc_agt_init ("nosuch");
			return 0;
		}
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
		service_count = svcagt_show_service_index ();
		if (service_count > 0) {
			err = svc_agt_set (6, stopped_str); //sheepdog
			err = svc_agt_get (6, &svc_info, true);
			if (err == 0) {
				printf ("Get 6: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
			} else {
				printf ("Get 6 failed\n");
			}
			err = svc_agt_set (1, running_str); //sajwt1
			err = svc_agt_get (1, &svc_info, false);
			if (err == 0) {
				printf ("Get 1: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
			} else {
				printf ("Get 1 failed\n");
			}
			delay (delay_secs);
			err = svc_agt_set (2, running_str); //sajwt2
			err = svc_agt_get (2, &svc_info, false);
			if (err == 0) {
				printf ("Get 2: %s goal: %s\n", svc_info.svc_name,	svc_info.goal_state);
			} else {
				printf ("Get 2 failed\n");
			}
			err = svc_agt_set (1, stopped_str); //sajwt1
			delay (delay_secs);
			err = svc_agt_set (3, running_str); //sajwt3
			err = svc_agt_set (2, stopped_str); //sajwt2
			delay (delay_secs);
			err = svc_agt_set (3, stopped_str); //sajwt3
		}
		svcagt_show_service_db ();
		err = svc_agt_get_all (&service_list, false);
		
	} else {

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
			printf ("Get 0: %s goal: %d\n", name,	state);
		} else {
			printf ("Get 0 failed\n");
		}
		err = svcagt_db_get (3, &name, &state, true);
		if (err == 0) {
			printf ("Get 3: %s goal: %d\n", name,	state);
		} else {
			printf ("Get 3 failed (expected)\n");
		}
		err = svcagt_set_by_name ("service4", true, 22L);
		err = svcagt_db_get (2, &name, &state, true);
		if (err == 0) {
			printf ("Get 2: %s goal: %d\n", name,	state);
		} else {
			printf ("Get 2 failed\n");
		}
	
		err = svcagt_db_set (1, true);
		if (err == 0)
			err = svcagt_db_get (1, &name, &state, true);
		if (err == 0) {
			printf ("Set/Get 1: %s goal: %d\n", name,	state);
		} else {
			printf ("Set/Get 1 failed\n");
		}
			
		err = svcagt_db_set (1, false);
		if (err == 0)
			err = svcagt_db_get (1, &name, &state, true);
		if (err == 0) {
			printf ("Set/Get 1: %s goal: %d\n", name,	state);
		} else {
			printf ("Set/Get 1 failed\n");
		}
		svcagt_show_service_db ();
		err = svc_agt_get_all (&service_list, true);
	}

	if (err >= 0) {
		svcagt_show_service_list (service_list);
		svc_agt_remove_service_list (service_list);
	} else {
		printf ("Error %d on svcagt_db_get_all\n", err);
	}
	svc_agt_shutdown ();


	printf ("Test done!\n");
}
