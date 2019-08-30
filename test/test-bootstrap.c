
#include <mysql.h>

#include "../include/test.h"
#include "../include/ndo.h"

/* This file contains functions which allocate/populate sample structs and
 * return pointers to them, for use in testing the handler functions.
 */

void populate_commands() {

	mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
		"instance_id = 1, objecttype_id = 12, name1 = 'check_xi_host_ping', name2 = NULL, is_active = 1");

	mysql_query(mysql_connection, "INSERT INTO nagios_commands SET "
		"instance_id = 1, config_type = 1, "
		"object_id = (SELECT object_id from nagios_objects WHERE name1 = 'check_xi_host_ping' AND objecttype_id = 12), "
		"command_line = '$USER1$/check_icmp -H $HOSTADDRESS$ -w $ARG1$,$ARG2$ -c $ARG3$,$ARG4$ -p 5'");
}

void populate_all_objects() {
	populate_commands();
}