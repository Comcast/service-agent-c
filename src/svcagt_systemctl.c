/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "svcagt_systemctl.h"
#include "svcagt_log.h"


// Can be changed for debugging
const char *svcagt_systemctl_cmd = "systemctl";

// Invoke systemctl to get the running/stopped state of a service
int svcagt_get_service_state (const char *svc_name)
{
	int exit_code;
	bool running;
	char cmdbuf[128];

	sprintf (cmdbuf, "%s is-active %s.service", svcagt_systemctl_cmd, svc_name);
	exit_code = system (cmdbuf);
	if (exit_code == -1) {
		svcagt_log (LEVEL_ERROR, errno, "Error invoking systemctl command\n");
		return -1;
	}
	running = (exit_code == 0);
	return running;
}

// Invoke systemctl to start or stop a service
int svcagt_set_service_state (const char *svc_name, bool state)
{
	int exit_code = 0;
	char cmdbuf[128];
	const char *start_stop_msg;
	const char *cmd_option;

	if (state) {
		start_stop_msg = "Starting";
		cmd_option = "start";
	} else {
		start_stop_msg = "Stopping";
		cmd_option = "stop";
	}

	svcagt_log (LEVEL_INFO, 0, "%s %s\n", start_stop_msg, svc_name);
	sprintf (cmdbuf, "%s %s %s.service", 
		svcagt_systemctl_cmd, cmd_option, svc_name); 
	exit_code = system (cmdbuf);
	if (exit_code != 0)
		svcagt_log (LEVEL_ERROR, errno, "Command %s failed with exit %d\n",
			cmdbuf, exit_code);
	return exit_code;
}

