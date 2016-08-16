/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _SVCAGT_SYSTEMCTL_H
#define  _SVCAGT_SYSTEMCTL_H

#include <stdbool.h>

/**
 * Invoke systemctl to query the status of a service
 *
 * @param svc_name
 * @return 0 service stopped, 1 service running, -1 on error
 */
int svcagt_get_service_state (const char *svc_name);

/**
 * Invoke systemctl to start or stop a service
 *
 * @param svc_name
 * @param state true=running, false=stopped
 * @return 0 on success, else an error code
 */
int svcagt_set_service_state (const char *svc_name, bool state);

#endif
