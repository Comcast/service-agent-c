/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _SVCAGT_DB_H
#define  _SVCAGT_DB_H

#include "svc_agt.h"
#include <errno.h>
#include <stdbool.h>

/**
 * Initialize the service agent database
 */
int svcagt_db_init(void);

/**
 * Shut down the service agent database
 */
void svcagt_db_shutdown (void);

/**
 * Add a service to the database
 * 
 * @param name name of service to be added
*/  
int svcagt_db_add (const char *name);

/**
 * Remove a service from the database
 * 
 * @param name name of service to be removed
*/  
int svcagt_db_remove (const char *name);

/**
 * Get the state of a specified service
 *
 * @param index index of the service to be queried
 * @param name pointer to variable receiving the service name
 * @param state pointer to variable receiving the state
 * @param db_query if true just get db state, no systemd
 * @return 0 on success, -1 otherwise.
 */
int svcagt_db_get (unsigned index, const char **name, bool *state, bool db_query);

/**
 * Set the state of a named service
 *  
 * @param name name of service
 * @param state state of the service
 * @param state_file_pos position in the goals state file where the state is recorded.
 * @note called during initialization.
 * @note starts or stops the service and records the file position.
 * @return 0 on success, otherwise an error code.
 *
*/  
int svcagt_set_by_name (const char *name, bool state, long state_file_pos);

/**
 * Set the state of a specified service
 *
 * @param index index of the service to be updated
 * @param new_state new state of the service
 * @return 0 on success, -1 otherwise.
 * @note state is Running, Stopped, or Default
 */
int svcagt_db_set (unsigned index, bool state);


/**
 * Get the state of all services
 *
 * @param service_list Pointer to a linked list of service_list_item's
 * @param db_query if true just get db state, no systemd
 * @return # items on success, -1 otherwise.
 */
int svcagt_db_get_all (service_list_item_t **service_list, bool db_query);

/**
 * Remove a service list
 *
 * @param service_list linked list of service_list_item's to be removed.
 */
void svcagt_db_remove_service_list (service_list_item_t *service_list);

#endif
