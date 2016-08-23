#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define HOME_DIR "/home/bwilla0627c/"
#define FILE_NAME HOME_DIR "test_service_output.txt"


const char *default_file_name = FILE_NAME;
int fd = -1;

void dbg_err (int err, const char *fmt, ...)
{
		char errbuf[100];

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
		printf ("%s\n", strerror_r (err, errbuf, 100));
}

int get_current_time (struct timeval *tv, struct tm *split_time)
{
	time_t secs;
	int err = gettimeofday (tv, NULL);
	if (err != 0) {
		dbg_err (err, "Error getting time of day: ");
		return err;
	}
	secs = (time_t) tv->tv_sec;
	localtime_r (&secs, split_time);
	return 0;
}

void make_timestamp (struct tm *split_time, unsigned msecs, char *timestamp)
{
	sprintf (timestamp, "%04d%02d%02d-%02d%02d%02d-%03d\n", 
		split_time->tm_year+1900, split_time->tm_mon+1, split_time->tm_mday,
		split_time->tm_hour, split_time->tm_min, split_time->tm_sec,
		msecs);
}

int make_current_timestamp (char *timestamp)
{
	int err;
	struct timeval tv;
	struct tm split_time;
  
	err = get_current_time (&tv, &split_time);
	if (err != 0)
		return err;
	make_timestamp (&split_time, tv.tv_usec / 1000, timestamp);
	return 0;
}

int open_file_output (const char *file_specifier)
{
	const char *file_name = default_file_name;
	char file_name_buf[128];
	int err;

	if (file_specifier != NULL) {
		sprintf (file_name_buf, "%stest_service_output.%s.txt", HOME_DIR, file_specifier);
		file_name = file_name_buf;
	}
	fd = open (file_name, O_WRONLY | O_CREAT | O_SYNC | O_APPEND, 0666);
	if (fd < 0) {
		dbg_err (errno, "Error(1) opening output file %s: ", file_name);
		return errno;
	}
	return 0;
}

static void sig_handler (int signum, siginfo_t *siginfo, void *context)
{
	printf ("Ctl-C\n");
	if (-1 != fd)
		close (fd);
	exit(0);
}


int main(int argc, char *argv[])
{
	int err;
	char timestamp[21];
	struct sigaction act;

	memset (&act, '\0', sizeof(act));
	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO;

	if (sigaction (SIGINT, &act, NULL) < 0) {
		perror ("sigaction");
		return 4;
	}
	
	if (sigaction (SIGTERM, &act, NULL) < 0) {
		perror ("sigaction");
		return 4;
	}

	if (argc < 2)
		err = open_file_output (NULL);
	else
		err = open_file_output (argv[1]);

	if (err != 0)
		return 4;

	while (1) {
		if (make_current_timestamp (timestamp) != 0)
			return 4;
		err = write (fd, timestamp, 20);
		if (err < 0)
			perror ("write");
		if (fsync (fd) < 0)
			perror ("fsync");
		sleep (3);
	}

}
