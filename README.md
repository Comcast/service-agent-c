# service_agent
systemd service agent

## Build Requirements

- https://github.com/troydhanson/uthash   (header file only)

## Files used by service agent

Initialization of the service agent requires a directory name to be
provided.  In this directory, the following files will be written:

- /logs 	
-- directory where log files will be written


- svcagt_exclude_services.txt	
-- list of services to be excluded (one per line)


- svcagt_goal_states.txt	
-- list of services that have been started or stopped by service agent
-- and the state.

