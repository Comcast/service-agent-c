/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _SVC_AGT_H
#define  _SVC_AGT_H

#include <stdbool.h>
#include "svcagt_log.h"


// goal states that we understand
// Running
// Stopped
// Default - not currently supported
//

typedef struct {
	const char *svc_name;
	const char *goal_state;
} service_info_t;

typedef struct service_list_item {
	unsigned index;
	service_info_t svc_info;
	struct service_list_item *prev, *next;
} service_list_item_t;

/**
 * Initialize the service agent
 *
 * @param svcagt_directory directory where the service agent saves info
 * @param svcagt_ex_directory directory where extra input files are found
 *        currently, this directory has the exclude services file
 * @param log_handler handler to receive all log notifications
 * @return 0 on success, valid errno otherwise.
 */
int svc_agt_init (const char *svcagt_directory, 
		const char *svcagt_ex_directory,
		svcagtLogHandler log_handler);

/**
 * Shut down the service agent
 *
 */
int svc_agt_shutdown (void);

/**
 * Get the state of all services
 *
 * @param service_list Pointer to a linked list of service_list_item's
 * @param db_query if true just get db state, no systemd
 * @return # items on success, -1 otherwise.
 */
int svc_agt_get_all (service_list_item_t **service_list, bool db_query);

/**
 * Remove a service list
 *
 * @param service_list linked list of service_list_item's to be removed.
 */
void svc_agt_remove_service_list (service_list_item_t *service_list);

/**
 * Get the state of a specified service
 *
 * @param index index of the service to be queried
 * @param service_info Pointer to structure containing the info.
 * @param db_query if true just get db state, no systemd
 * @return 0 on success, -1 otherwise.
 */
int svc_agt_get (unsigned index, service_info_t *service_info, bool db_query);

/**
 * Set the state of a specified service
 *
 * @param index index of the service to be updated
 * @param new_state new state of the service, "Running" or "Stopped"
 * @return 0 on success, -1 otherwise.
 */
int svc_agt_set (unsigned index, const char *new_state);

#endif
