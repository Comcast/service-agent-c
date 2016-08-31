// mock_systemctl.c
//
// compile:
//   gcc -o mock_systemctl -lm mock_systemctl.c
//

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define NAME_BUFLEN 128
#define CMD_PIPE_NAME "mock_systemctl_cmds.txt"
#define RESPONSE_PIPE_NAME "mock_systemctl_responses.txt"

static char cmd_pipe_name[NAME_BUFLEN];
static char response_pipe_name[NAME_BUFLEN];

static char cmd_buf[NAME_BUFLEN];
static char response_buf[10];

static int cmd_pipe_fd = -1;
static int response_pipe_fd = -1;

static int open_pipe (const char *pipe_name, int mode)
{
	int pipe_fd = open (pipe_name, mode | O_SYNC, 0666);
	if (pipe_fd < 0) {
		printf ("Error(1) opening pipe %s\n", pipe_name);
	}
	return pipe_fd;
}

static int send_command (char cmd, const char *svc_name)
{
	int nbytes;

	cmd_pipe_fd = open_pipe (cmd_pipe_name, O_WRONLY);
	if (cmd_pipe_fd < 0)
		return 4;

	response_pipe_fd = open_pipe (response_pipe_name, O_RDONLY);
	if (response_pipe_fd < 0) {
		close (cmd_pipe_fd);
		return 4;
	}

	memset (cmd_buf, '\0', NAME_BUFLEN);
	cmd_buf[0] = cmd;
	sprintf (cmd_buf+1, "%s\n", svc_name);
	nbytes = write (cmd_pipe_fd, cmd_buf, NAME_BUFLEN);
	if (nbytes < 0) {
		printf ("Error writing to pipe %s\n", cmd_pipe_name);
		close (cmd_pipe_fd);
		close (response_pipe_fd);
		return 4;
	}
	nbytes = read (response_pipe_fd, response_buf, 2);
	close (cmd_pipe_fd);
	close (response_pipe_fd);
	if (nbytes < 0) {
		printf ("Error reading from pipe %s\n", response_pipe_name);
		return 4;
	}
	return 0;
}

int test_svc_active (const char *svc_name)
{
	int rtn = send_command ('?', svc_name);
	if (rtn != 0)
		return rtn;
	if (response_buf[0] == '1') // active
		return 0;
	return 1; // inactive
}

int main(int argc, char *argv[])
{
	const char *home_dir;
	const char *command;

	if (argc < 3) {
		printf ("Expecting 2 arguments:\n");
		printf (" 1) command\n");
		printf (" 2) service name\n");
		return EINVAL;
	}
	
	home_dir = getenv ("HOME");
	if (NULL == home_dir) {
		printf ("No $HOME defined\n");
		return ENOENT;
	}

	sprintf (cmd_pipe_name, "%s/%s", home_dir, CMD_PIPE_NAME);
	sprintf (response_pipe_name, "%s/%s", home_dir, RESPONSE_PIPE_NAME);

	command = argv[1];
	if (strcmp (command, "is-active") == 0)
		return test_svc_active (argv[2]);
	else if (strcmp (command, "start") == 0)
		return send_command ('1', argv[2]);
	else if (strcmp (command, "stop") == 0)
		return send_command ('0', argv[2]);
	else {
		printf ("Invalid command\n");
		return EINVAL;
	}
}
