/*

apt-get install -y check lcov gcovr

just compile:
gcc -g test.c -lcheck -lsubunit -lm -lrt -lpthread -o test

compile for coverage:
gcc $(mysql_config --cflags) -g -ggdb3 -fprofile-arcs -ftest-coverage -o test test.c ../src/ndo.obj $(mysql_config --libs) -lcheck -lsubunit -lm -lrt -lpthread -ldl

make coverage: (change the .. to . once this makes it in the main make file)
lcov -c -d ../ -o coverage.info-file
genhtml coverage.info-file -o coverage/
gcovr --exclude="test.c" -r ..

*/

#include <mysql.h>
#include <dlfcn.h>


#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include "../include/ndo.h"
#include "../include/nagios/downtime.h"
#include "../include/nagios/comments.h"
#include "../include/nagios/nebstructs.h"
#include "../include/nagios/neberrors.h"
#include "../include/nagios/broker.h"
#include "../include/test.h"

int my_system_r_output = 0;
#include "nagios-stubs.c"

/**** NAGIOS VARIABLES ****/
command              * command_list;
timeperiod           * timeperiod_list;
contact              * contact_list;
contactgroup         * contactgroup_list;
host                 * host_list;
hostgroup            * hostgroup_list;
service              * service_list;
servicegroup         * servicegroup_list;
hostescalation       * hostescalation_list;
hostescalation      ** hostescalation_ary;
serviceescalation    * serviceescalation_list;
serviceescalation   ** serviceescalation_ary;
hostdependency       * hostdependency_list;
hostdependency      ** hostdependency_ary;
servicedependency    * servicedependency_list;
servicedependency   ** servicedependency_ary;
char                 * config_file;
sched_info             scheduling_info;
char                 * global_host_event_handler;
char                 * global_service_event_handler;
int                  __nagios_object_structure_version;
struct object_count    num_objects;


#define NUM_SUITES 2


void * neb_handle = NULL;


int load_neb_module(char * config_file)
{
    neb_handle = malloc(1);
    if (neb_handle == NULL) {
        return NDO_ERROR;
    }

    return nebmodule_init(0, config_file, neb_handle);
}


void unload_neb_module()
{
    nebmodule_deinit(0, 0);
    free(neb_handle);
}


START_TEST (booleans_are_sane)
{
    ck_assert_int_eq(TRUE, 1);
    ck_assert_int_eq(FALSE, 0);
}
END_TEST


START_TEST (ndo_types_wont_kill_broker)
{
    ck_assert_int_eq(NDO_OK, NEB_OK);
    ck_assert_int_eq(NDO_ERROR, NEB_ERROR);
    ck_assert_int_eq(TRUE, NEB_TRUE);
    ck_assert_int_eq(FALSE, NEB_FALSE);
}
END_TEST


Suite * t_suite(void)
{
    Suite * suite   = NULL;
    TCase * tc_core = NULL;

    suite = suite_create("t_suite");

    tc_core = tcase_create("core functionality");

    tcase_add_test(tc_core, booleans_are_sane);
    tcase_add_test(tc_core, ndo_types_wont_kill_broker);

    suite_add_tcase(suite, tc_core);

    return suite;
}


START_TEST (test_program_state)
{
    nebstruct_process_data d;
}
END_TEST


START_TEST (test_timed_event)
{
    nebstruct_timed_event_data d;
}
END_TEST


START_TEST (test_log_data)
{
    nebstruct_log_data d;

    d.type = 0;
    d.flags = 0;
    d.attr = 0;
    gettimeofday(&(d.timestamp), NULL);

    d.entry_time = 0;
    d.data_type = 0;
    d.data = strdup("log data");

    ndo_handle_log(0, (void *) &d);

    // do a query against log_data
    // assert the values are what we set them
    // set d.data to null and check again

    free(d.data);
}
END_TEST


/* Note: This is no longer used in nagios by default.
 * The data used for this is extremely suspect
 */
START_TEST (test_system_command)
{
    nebstruct_system_command_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = 400;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568303762, .tv_usec = 517109 };
    d.start_time = (struct timeval) { .tv_sec = 1568303746, .tv_usec = 165485 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.timeout = 5;
    d.command_line = strdup("/usr/bin/echo fake_command");
    d.early_timeout = 0;
    d.execution_time = 0;
    d.return_code = 0;
    d.output = NULL;

    ndo_handle_system_command(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_systemcommands WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1568303746) "
        "AND start_time_usec = 165485 AND end_time = FROM_UNIXTIME(0) "
        "AND end_time_usec = 0 AND command_line = '/usr/bin/echo fake_command' "
        "AND timeout = 5 AND early_timeout = 0 "
        "AND execution_time = 0 AND return_code = 0 "
        "AND output = '' AND long_output = '' ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_event_handler)
{
    nebstruct_event_handler_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_EVENTHANDLER_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568309570, .tv_usec = 315774 };
    d.eventhandler_type = GLOBAL_SERVICE_EVENTHANDLER;
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.state_type = 1;
    d.state = 0;
    d.timeout = 30;
    d.command_name = strdup("check_xi_host_ping");
    d.command_args = NULL;
    d.command_line = strdup("/usr/bin/echo fake global event handler command");
    d.start_time = (struct timeval) { .tv_sec = 1568308620, .tv_usec = 5233 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.early_timeout = 0;
    d.execution_time = 0;
    d.return_code = 0;
    d.output = NULL;
    d.object_ptr = &test_service;

    ndo_handle_event_handler(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_eventhandlers WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1568308620) "
        "AND start_time_usec = 5233 AND end_time = FROM_UNIXTIME(0) "
        "AND end_time_usec = 0 AND eventhandler_type = 3 "
        "AND object_id = (SELECT object_id from nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1) AND state = 0 "
        "AND state_type = 1 AND command_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 12 AND name1 = 'check_xi_host_ping' AND name2 IS NULL LIMIT 1) "
        "AND command_args = '' AND command_line = '/usr/bin/echo fake global event handler command' "
        "AND timeout = 30 AND early_timeout = 0 "
        "AND execution_time = 0 AND return_code = 0 "
        "AND output = '' AND long_output = '' ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    d.type = NEBTYPE_EVENTHANDLER_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568312563, .tv_usec = 952580 };
    d.eventhandler_type = GLOBAL_HOST_EVENTHANDLER;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.state_type = 0;
    d.state = 1;
    d.timeout = 30;
    d.command_name = strdup("check_xi_host_ping");
    d.command_args = NULL;
    d.command_line = strdup("/usr/bin/echo fake global event handler command");
    d.start_time = (struct timeval) { .tv_sec = 1568312540, .tv_usec = 976228 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.early_timeout = 0;
    d.execution_time = 0;
    d.return_code = 0;
    d.output = NULL;
    d.object_ptr = &test_host;

    ndo_handle_event_handler(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_eventhandlers WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1568312540) "
        "AND start_time_usec = 976228 AND end_time = FROM_UNIXTIME(0) "
        "AND end_time_usec = 0 AND eventhandler_type = 2 "
        "AND object_id = (SELECT object_id from nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) AND state = 1 "
        "AND state_type = 0 AND command_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 12 AND name1 = 'check_xi_host_ping' AND name2 IS NULL LIMIT 1) "
        "AND command_args = '' AND command_line = '/usr/bin/echo fake global event handler command' "
        "AND timeout = 30 AND early_timeout = 0 "
        "AND execution_time = 0 AND return_code = 0 "
        "AND output = '' AND long_output = '' ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_host_check)
{
    nebstruct_host_check_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* NDO should ignore any HOSTCHECK broker messages which are not NEBTYPE_HOSTCHECK_PROCESSED */

    d.type = NEBTYPE_HOSTCHECK_INITIATE;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567102596, .tv_usec = 116506 };
    d.host_name = strdup("_testhost_01");
    d.current_attempt = 1;
    d.check_type = 0;
    d.max_attempts = 5;
    d.state_type = 1;
    d.state = 0;
    d.timeout = 30;
    d.command_name = strdup("check_xi_host_ping");
    d.command_args = strdup("3000.0!80%!5000.0!100%");
    d.command_line = strdup("/usr/local/nagios/libexec/check_icmp -H 127.0.0.1 -w 3000.0,80%% -c 5000.0,100%% -p 5");
    d.start_time = (struct timeval) { .tv_sec = 1567102578, .tv_usec = 740516 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.early_timeout = 0;
    d.execution_time = 0;
    d.latency = 17.001623153686523;
    d.return_code = 0;
    d.output = NULL;
    d.long_output = NULL;
    d.perf_data = NULL;
    /* These two pointers were not originally null. However, the handler function doesn't use the pointers */
    d.check_result_ptr = NULL;
    d.object_ptr = NULL;

    ndo_handle_host_check(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_hostchecks "
        "WHERE instance_id = 1 AND start_time = FROM_UNIXTIME(1567102578) AND start_time_usec = 740516 "
        "AND end_time = FROM_UNIXTIME(0) AND end_time_usec = 0 AND host_object_id = 0 AND check_type = 0 "
        "AND current_check_attempt = 1 AND max_check_attempts = 5 AND state = 0 AND state_type = 1 "
        "AND timeout = 30 AND early_timeout = 0 AND execution_time = 0 AND latency = 17.001623153686523 "
        "AND return_code = 0 AND output = NULL AND long_output = NULL AND perfdata = NULL "
        "AND command_args = '3000.0!80%%!5000.0!100%%' "
        "AND command_line = '/usr/local/nagios/libexec/check_icmp -H 127.0.0.1 -w 3000.0,80%% -c 5000.0,100%% -p 5' ");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row == NULL);

    mysql_free_result(tmp_result);

    /* This check is processed, so it should show up in the database */

    d.type = NEBTYPE_HOSTCHECK_PROCESSED;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567104929, .tv_usec = 252810 };
    d.host_name = strdup("_testhost_912");
    d.current_attempt = 1;
    d.check_type = 0;
    d.max_attempts = 5;
    d.state_type = 1;
    d.state = 0;
    d.timeout = 30;
    d.command_name = strdup("check_xi_host_ping");
    d.command_args = strdup("3000.0!80%!5000.0!100%");
    d.command_line = NULL;
    d.start_time = (struct timeval) { .tv_sec = 1567104907, .tv_usec = 365285 };
    d.end_time = (struct timeval) { .tv_sec = 1567104907, .tv_usec = 396672 };
    d.early_timeout = 0;
    d.execution_time = 0.031386999999999998;
    d.latency = 13.030651092529297;
    d.return_code = 0;
    d.output = strdup("OK - 127.0.0.1 rta 0.037ms lost 0%%");
    d.long_output = NULL;
    d.perf_data = strdup("rta=0.037ms;3000.000;5000.000;0; rtmax=0.082ms;;;; rtmin=0.014ms;;;; pl=0%%;80;100;0;100");
    d.check_result_ptr = NULL;
    d.object_ptr = NULL;

    ndo_handle_host_check(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_hostchecks "
        "WHERE instance_id = 1 AND start_time = FROM_UNIXTIME(1567104907) AND start_time_usec = 365285 "
        "AND end_time = FROM_UNIXTIME(1567104907) AND end_time_usec = 396672 AND check_type = 0 "
        "AND current_check_attempt = 1 AND max_check_attempts = 5 AND state = 0 AND state_type = 1 "
        "AND timeout = 30 AND early_timeout = 0 AND execution_time = 0.031386999999999998 AND latency = 13.030651092529297 "
        "AND return_code = 0 AND output = 'OK - 127.0.0.1 rta 0.037ms lost 0%%' AND long_output = '' "
        "AND perfdata = 'rta=0.037ms;3000.000;5000.000;0; rtmax=0.082ms;;;; rtmin=0.014ms;;;; pl=0%%;80;100;0;100' "
        "AND command_args = '3000.0!80%!5000.0!100%' ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_service_check)
{
    nebstruct_service_check_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_SERVICECHECK_ASYNC_PRECHECK;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567180338, .tv_usec = 786503 };
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_ping");
    d.check_type = 0;
    d.current_attempt = 1;
    d.max_attempts = 5;
    d.state_type = 1;
    d.state = 0;
    d.timeout = 0;
    d.command_name = strdup("check_xi_service_ping");
    d.command_args = strdup("3000.0!80%!5000.0!100%");
    d.command_line = NULL;
    d.start_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.early_timeout = 0;
    d.execution_time = 0;
    d.latency = 103.66160583496094;
    d.return_code = 0;
    d.output = strdup("OK - 127.0.0.1 rta 0.012ms lost 0%");
    d.long_output = NULL;
    d.perf_data = strdup("rta=0.012ms;3000.000;5000.000;0; rtmax=0.035ms;;;; rtmin=0.006ms;;;; pl=0%;80;100;0;100");
    d.check_result_ptr = NULL;
    /* object pointer was not originally null */
    d.object_ptr = NULL;

    ndo_handle_service_check(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_servicechecks WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(0) AND start_time_usec = 0 "
        "AND end_time = FROM_UNIXTIME(0) AND end_time_usec = 0 "
        "AND check_type = 0 AND current_check_attempt = 1 AND max_check_attempts = 5 "
        "AND state = 0 AND state_type = 1 AND timeout = 30 AND early_timeout = 0 "
        "AND execution_time = 0 AND latency = 103.66160583496094 AND return_code = 0 "
        "AND output = 'OK - 127.0.0.1 rta 0.012ms lost 0%' "
        "AND long_output = '' "
        "AND perfdata = 'rta=0.012ms;3000.000;5000.000;0; rtmax=0.035ms;;;; rtmin=0.006ms;;;; pl=0%;80;100;0;100' "
        "AND command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = 'check_xi_service_ping' AND objecttype_id = 12 LIMIT 1) "
        "AND command_args = '3000.0!80%!5000.0!100%' AND command_line = ''");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row == NULL);

    mysql_free_result(tmp_result);


    d.type = NEBTYPE_SERVICECHECK_PROCESSED;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567180773, .tv_usec = 733435 };
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_dns");
    d.check_type = 0;
    d.current_attempt = 5;
    d.max_attempts = 5;
    d.state_type = 1;
    d.state = 2;
    d.timeout = 60;
    d.command_name = NULL;
    d.command_args = NULL;
    d.command_line = NULL;
    d.start_time = (struct timeval) { .tv_sec = 1567180765, .tv_usec = 383185 };
    d.end_time = (struct timeval) { .tv_sec = 1567180765, .tv_usec = 493635 };
    d.early_timeout = 0;
    d.execution_time = 0.11045000000000001;
    d.latency = 12.436541557312012;
    d.return_code = 2;
    d.output = strdup("DNS CRITICAL - query type of -querytype=A was not found for 127.0.0.1");
    d.long_output = NULL;
    d.perf_data = strdup("fakedata=0;;;;");
    /* these two pointers not originally null */
    d.check_result_ptr = NULL;
    d.object_ptr = NULL;

    ndo_handle_service_check(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_servicechecks WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1567180765) AND start_time_usec = 383185 "
        "AND end_time = FROM_UNIXTIME(1567180765) AND end_time_usec = 493635 "
        "AND check_type = 0 AND current_check_attempt = 5 AND max_check_attempts = 5 "
        "AND state = 2 AND state_type = 1 AND timeout = 60 AND early_timeout = 0 "
        "AND execution_time = 0.11045000000000001 AND latency = 12.436541557312012 AND return_code = 2 "
        "AND output = 'DNS CRITICAL - query type of -querytype=A was not found for 127.0.0.1'"
        "AND long_output = '' "
        "AND perfdata = 'fakedata=0;;;;' ");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }

    mysql_free_result(tmp_result);


}
END_TEST


START_TEST (test_comment_data)
{
    nebstruct_comment_data d_add_host;
    nebstruct_comment_data d_delete_host;
    nebstruct_comment_data d_add_service;
    nebstruct_comment_data d_delete_service;
    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* Add a host comment */
    d_add_host.type = NEBTYPE_COMMENT_ADD;
    d_add_host.flags = 0;
    d_add_host.attr = 0;
    d_add_host.timestamp = (struct timeval) { .tv_sec = 1567021700, .tv_usec = 29064 };
    d_add_host.comment_type = HOST_COMMENT;
    d_add_host.host_name = strdup("_testhost_1");
    d_add_host.service_description = NULL;
    d_add_host.entry_time = 1567021619,
    d_add_host.author_name = strdup("Nagios Admin");
    d_add_host.comment_data = strdup("this is a unique comment");
    d_add_host.persistent = 1;
    d_add_host.source = 1;
    d_add_host.entry_type = 1;
    d_add_host.expires = 0;
    d_add_host.expire_time = 0;
    d_add_host.comment_id = 1;
    d_add_host.object_ptr = NULL;

    ndo_handle_comment(0, &d_add_host);


    /* Verify that the comment shows in commenthistory */
    mysql_query(mysql_connection, "SELECT 1 FROM nagios_commenthistory "
     " WHERE comment_type = 1 AND entry_type = 1 AND comment_time = FROM_UNIXTIME(1567021619) "
       " AND internal_comment_id = 1 AND author_name = 'Nagios Admin' "
       " AND comment_data = 'this is a unique comment' AND is_persistent = 1 "
       " AND comment_source = 1 AND expires = 0 AND expiration_time = FROM_UNIXTIME(0) "
       " AND entry_time = FROM_UNIXTIME(1567021700) AND entry_time_usec = 29064 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    /* Verify that the comment shows in comments */
    mysql_query(mysql_connection, "SELECT 2 FROM nagios_comments "
     " WHERE comment_type = 1 AND entry_type = 1 AND comment_time = FROM_UNIXTIME(1567021619) "
       " AND internal_comment_id = 1 AND author_name = 'Nagios Admin' "
       " AND comment_data = 'this is a unique comment' AND is_persistent = 1 "
       " AND comment_source = 1 AND expires = 0 AND expiration_time = FROM_UNIXTIME(0) "
       " AND entry_time = FROM_UNIXTIME(1567021700) AND entry_time_usec = 29064 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);

    /* Now, delete the comment */

    d_delete_host.type = NEBTYPE_COMMENT_DELETE;
    d_delete_host.flags = 0;
    d_delete_host.attr = 0;
    d_delete_host.timestamp = (struct timeval) { .tv_sec = 1567089801, .tv_usec = 725979 };
    d_delete_host.comment_type = HOST_COMMENT;
    d_delete_host.host_name = strdup("_testhost_1");
    d_delete_host.service_description = NULL;
    d_delete_host.entry_time = 1567021619;
    d_delete_host.author_name = strdup("Nagid_delete_hostos Admin");
    d_delete_host.comment_data = strdup("this is a unique comment");
    d_delete_host.persistent = 1;
    d_delete_host.source = 1;
    d_delete_host.entry_type = 1;
    d_delete_host.expires = 0;
    d_delete_host.expire_time = 0;
    d_delete_host.comment_id = 1;
    d_delete_host.object_ptr = NULL;

    ndo_handle_comment(0, &d_delete_host);

    /* Comment should be present in commenthistory with deletion time */
    mysql_query(mysql_connection, "SELECT 3 FROM nagios_commenthistory "
        " WHERE internal_comment_id = 1 AND deletion_time IS NOT NULL");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "3"), 0);
    }
    mysql_free_result(tmp_result);

    /* Comment should be deleted from comments table */
    mysql_query(mysql_connection, "SELECT 4 FROM nagios_comments "
        " WHERE internal_comment_id = 1");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    /* There should always be "no more rows" */
    ck_assert(tmp_row == NULL);

    mysql_free_result(tmp_result);



    /* Add a service comment */
    d_add_service.type = NEBTYPE_COMMENT_ADD;
    d_add_service.flags = 0;
    d_add_service.attr = 0;
    d_add_service.timestamp = (struct timeval) { .tv_sec = 1567021701, .tv_usec = 29065 };
    d_add_service.comment_type = SERVICE_COMMENT;
    d_add_service.host_name = strdup("_testhost_1");
    d_add_service.service_description = strdup("test_service");
    d_add_service.entry_time = 1567021620,
    d_add_service.author_name = strdup("Nagios Admin");
    d_add_service.comment_data = strdup("this is a different comment");
    d_add_service.persistent = 1;
    d_add_service.source = 1;
    d_add_service.entry_type = 1;
    d_add_service.expires = 0;
    d_add_service.expire_time = 0;
    d_add_service.comment_id = 2;
    d_add_service.object_ptr = NULL;

    ndo_handle_comment(0, &d_add_service);


    /* Verify that the comment shows in commenthistory */
    mysql_query(mysql_connection, "SELECT 5 FROM nagios_commenthistory "
     " WHERE comment_type = 2 AND entry_type = 1 AND comment_time = FROM_UNIXTIME(1567021620) "
       " AND internal_comment_id = 2 AND author_name = 'Nagios Admin' "
       " AND comment_data = 'this is a different comment' AND is_persistent = 1 "
       " AND comment_source = 1 AND expires = 0 AND expiration_time = FROM_UNIXTIME(0) "
       " AND entry_time = FROM_UNIXTIME(1567021701) AND entry_time_usec = 29065 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "5"), 0);
    }
    mysql_free_result(tmp_result);

    /* Verify that the comment shows in comments */
    mysql_query(mysql_connection, "SELECT 6 FROM nagios_comments "
     " WHERE comment_type = 2 AND entry_type = 1 AND comment_time = FROM_UNIXTIME(1567021620) "
       " AND internal_comment_id = 2 AND author_name = 'Nagios Admin' "
       " AND comment_data = 'this is a different comment' AND is_persistent = 1 "
       " AND comment_source = 1 AND expires = 0 AND expiration_time = FROM_UNIXTIME(0) "
       " AND entry_time = FROM_UNIXTIME(1567021701) AND entry_time_usec = 29065 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "6"), 0);
    }
    mysql_free_result(tmp_result);

    /* Now, delete the comment */

    d_delete_service.type = NEBTYPE_COMMENT_DELETE;
    d_delete_service.flags = 0;
    d_delete_service.attr = 0;
    d_delete_service.timestamp = (struct timeval) { .tv_sec = 1567089801, .tv_usec = 725979 };
    d_delete_service.comment_type = SERVICE_COMMENT;
    d_delete_service.host_name = strdup("_testhost_1");
    d_delete_service.service_description = strdup("test_service");
    d_delete_service.entry_time = 1567021620;
    d_delete_service.author_name = strdup("Nagios Admin");
    d_delete_service.comment_data = strdup("this is a unique comment");
    d_delete_service.persistent = 1;
    d_delete_service.source = 1;
    d_delete_service.entry_type = 1;
    d_delete_service.expires = 0;
    d_delete_service.expire_time = 0;
    d_delete_service.comment_id = 2;
    d_delete_service.object_ptr = NULL;

    ndo_handle_comment(0, &d_delete_service);

    /* Comment should be present in commenthistory with deletion time */
    mysql_query(mysql_connection, "SELECT 7 FROM nagios_commenthistory "
        " WHERE internal_comment_id = 2 AND deletion_time IS NOT NULL");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "7"), 0);
    }
    mysql_free_result(tmp_result);

    /* Comment should be deleted from comments table */
    mysql_query(mysql_connection, "SELECT 8 FROM nagios_comments "
        " WHERE internal_comment_id = 2");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row == NULL);

    mysql_free_result(tmp_result);


}
END_TEST


START_TEST (test_downtime_data)
{
    nebstruct_downtime_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_DOWNTIME_ADD;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567191778, .tv_usec = 909038 };
    d.downtime_type = 2;
    d.host_name = strdup("_testhost_1");
    d.service_description = 0x0;
    d.entry_time = 1567191710;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup("this is a test comment for downtime");
    d.start_time = 1567192501;
    d.end_time = 1567193401;
    d.fixed = 1;
    d.duration = 900;
    d.triggered_by = 0;
    d.downtime_id = 1;
    d.object_ptr = 0x0;

    ndo_handle_downtime(d.type, &d);

    /* Verify that the downtime was added to scheduleddowntime */
    mysql_query(mysql_connection, "SELECT 1 FROM nagios_scheduleddowntime WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    /* Verify that the exact same entry was added to downtimehistory */

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_downtimehistory WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);


    /* Start the downtime */

    d.type = NEBTYPE_DOWNTIME_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567192611, .tv_usec = 90214 };
    d.downtime_type = 2;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.entry_time = 1567191710;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup("hey this host isnt rly going down but whatever");
    d.start_time = 1567192501;
    d.end_time = 1567193401;
    d.fixed = 1;
    d.duration = 900;
    d.triggered_by = 0;
    d.downtime_id = 1;
    d.object_ptr = NULL;


    ndo_handle_downtime(d.type, &d);

    /* Verify that the same downtime above is present in downtimehistory and scheduleddowntime,
     * but that actual_start_time, actual_start_time_usec, and was_started were updated */
    mysql_query(mysql_connection, "SELECT 3 FROM nagios_scheduleddowntime WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) "
        /* query is the same as "SELECT 1..." up until this line */
        "AND actual_start_time = FROM_UNIXTIME(1567192611) AND actual_start_time_usec = 90214 AND was_started = 1 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "3"), 0);
    }
    mysql_free_result(tmp_result);

    /* Verify that the updated values are also in downtimehistory */

    mysql_query(mysql_connection, "SELECT 4 FROM nagios_downtimehistory WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) "
        /* query is the same as "SELECT 2..." up until this line */
        "AND actual_start_time = FROM_UNIXTIME(1567192611) AND actual_start_time_usec = 90214 AND was_started = 1 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "4"), 0);
    }
    mysql_free_result(tmp_result);


    /* End the downtime */


    d.type = NEBTYPE_DOWNTIME_STOP;
    d.flags = 0;
    d.attr = NEBATTR_DOWNTIME_STOP_NORMAL;
    d.timestamp = (struct timeval) { .tv_sec = 1567521860, .tv_usec = 732623 };
    d.downtime_type = 2;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.entry_time = 1567191710;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup("hey this host isnt rly going down but whatever");
    d.start_time = 1567192501;
    d.end_time = 1567193401;
    d.fixed = 1;
    d.duration = 900;
    d.triggered_by = 0;
    d.downtime_id = 1;
    d.object_ptr = NULL;

    ndo_handle_downtime(d.type, &d);

    /* Verify that the downtime was deleted from scheduleddowntime */
    mysql_query(mysql_connection, "SELECT 5 FROM nagios_scheduleddowntime WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row == NULL);

    /* Verify that the the entry still exists in downtimehistory with updated actual_end_time, actual_end_time_usec, was_cancelled */

    mysql_query(mysql_connection, "SELECT 6 FROM nagios_downtimehistory WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191710) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'this is a test comment for downtime' AND internal_downtime_id = 1 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567192501) AND scheduled_end_time = FROM_UNIXTIME(1567193401) "
        "AND actual_start_time = FROM_UNIXTIME(1567192611) AND actual_start_time_usec = 90214 AND was_started = 1 "
        /* query is the same as "SELECT 4..." up until this line */
        "AND actual_end_time = FROM_UNIXTIME(1567521860) AND actual_end_time_usec = 732623 AND was_cancelled = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "6"), 0);
    }
    mysql_free_result(tmp_result);

    /* Also test the cancelled downtime, since this is a separate branch for this handler */

    d.type = NEBTYPE_DOWNTIME_ADD;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567521960, .tv_usec = 909038 };
    d.downtime_type = 2;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.entry_time = 1567191810;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup("cancellable downtime");
    d.start_time = 1567202501;
    d.end_time = 1567203401;
    d.fixed = 1;
    d.duration = 900;
    d.triggered_by = 0;
    d.downtime_id = 5;
    d.object_ptr = NULL;

    ndo_handle_downtime(d.type, &d);


    d.type = NEBTYPE_DOWNTIME_STOP;
    d.flags = 0;
    d.attr = NEBATTR_DOWNTIME_STOP_CANCELLED;
    d.timestamp = (struct timeval) { .tv_sec = 1567522060, .tv_usec = 732623 };
    d.downtime_type = 2;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.entry_time = 1567191810;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup("cancellable downtime");
    d.start_time = 1567202501;
    d.end_time = 1567203401;
    d.fixed = 1;
    d.duration = 900;
    d.triggered_by = 0;
    d.downtime_id = 5;
    d.object_ptr = NULL;

    ndo_handle_downtime(d.type, &d);

    /* Verify that the the entry still exists in downtimehistory with updated was_cancelled */

    mysql_query(mysql_connection, "SELECT 8 FROM nagios_downtimehistory WHERE "
        "instance_id = 1 AND downtime_type = 2 " /* AND object_id = ? "*/
        "AND entry_time = FROM_UNIXTIME(1567191810) AND author_name = 'Nagios Admin' "
        "AND comment_data = 'cancellable downtime' AND internal_downtime_id = 5 "
        "AND triggered_by_id = 0 AND is_fixed = 1 AND duration = 900 "
        "AND scheduled_start_time = FROM_UNIXTIME(1567202501) AND scheduled_end_time = FROM_UNIXTIME(1567203401) "
        "AND actual_end_time = FROM_UNIXTIME(1567522060) AND actual_end_time_usec = 732623 AND was_cancelled = 1 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "8"), 0);
    }
    mysql_free_result(tmp_result);


}
END_TEST


START_TEST (test_flapping_data)
{
    nebstruct_flapping_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_FLAPPING_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567539851, .tv_usec = 906317};
    d.flapping_type = 1;
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.percent_change = 23.026315789473681;
    d.high_threshold = 20;
    d.low_threshold = 5;
    d.comment_id = 0;
    d.object_ptr = NULL;

    ndo_handle_flapping(d.type, &d);

    /* Verify that an entry is created for FLAPPING_START with services */
    mysql_query(mysql_connection, "SELECT 1 FROM nagios_flappinghistory WHERE "
        "instance_id = 1 AND event_time = FROM_UNIXTIME(1567539851) AND event_time_usec = 906317 "
        "AND event_type = 1000 AND reason_type = 0 AND flapping_type = 1 "
        "AND object_id = (SELECT object_id from nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1) "
        "AND percent_state_change = 23.026315789473681 AND low_threshold = 5 "
        "AND high_threshold = 20 AND comment_time = FROM_UNIXTIME(0) AND "
        "internal_comment_id = 0 ");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    /* Verify that an entry is created for FLAPPING_STOP with hosts */

    d.type = NEBTYPE_FLAPPING_STOP;
    d.flags = 0;
    d.attr = 2;
    d.timestamp = (struct timeval) { .tv_sec = 1567543077, .tv_usec = 95771 };
    d.flapping_type = 0;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.percent_change = 23.35526315789474;
    d.high_threshold = 0;
    d.low_threshold = 0;
    d.comment_id = 0;
    d.object_ptr = NULL;

    ndo_handle_flapping(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_flappinghistory WHERE "
        "instance_id = 1 AND event_time = FROM_UNIXTIME(1567543077) AND event_time_usec = 95771 "
        "AND event_type = 1001 AND reason_type = 2 AND flapping_type = 0 "
        "AND object_id = (SELECT object_id from nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) "
        "AND percent_state_change = 23.35526315789474 AND low_threshold = 0 "
        "AND high_threshold = 0 AND comment_time = FROM_UNIXTIME(0) AND "
        "internal_comment_id = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_program_status)
{
    nebstruct_program_status_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_PROGRAMSTATUS_UPDATE;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567626021, .tv_usec = 823127 };
    d.program_start = 1567542969;
    d.pid = 107564;
    d.daemon_mode = 0;
    d.last_log_rotation = 1567573200;
    d.notifications_enabled = 1;
    d.active_service_checks_enabled = 1;
    d.passive_service_checks_enabled = 1;
    d.active_host_checks_enabled = 1;
    d.passive_host_checks_enabled = 1;
    d.event_handlers_enabled = 1;
    d.flap_detection_enabled = 1;
    d.process_performance_data = 0;
    d.obsess_over_hosts = 0;
    d.obsess_over_services = 0;
    d.modified_host_attributes = 0;
    d.modified_service_attributes = 0;
    d.global_host_event_handler = NULL;
    d.global_service_event_handler = NULL;

    ndo_handle_program_status(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_programstatus WHERE "
        "instance_id = 1 AND status_update_time = FROM_UNIXTIME(1567626021) "
        "AND program_start_time = FROM_UNIXTIME(1567542969) AND is_currently_running = 1 "
        "AND process_id = 107564 AND daemon_mode = 0 "
        "AND last_command_check = FROM_UNIXTIME(0) AND last_log_rotation = FROM_UNIXTIME(1567573200) "
        "AND notifications_enabled = 1 AND active_service_checks_enabled = 1 "
        "AND passive_service_checks_enabled = 1 AND active_host_checks_enabled = 1 "
        "AND passive_host_checks_enabled = 1 AND event_handlers_enabled = 1 "
        "AND flap_detection_enabled = 1 AND failure_prediction_enabled = 0 "
        "AND process_performance_data = 0 AND obsess_over_hosts = 0 "
        "AND obsess_over_services = 0 AND modified_host_attributes = 0 "
        "AND modified_service_attributes = 0 AND global_host_event_handler = '' AND global_service_event_handler = ''");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_host_status)
{
    nebstruct_host_status_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* also uses test_host global */

    d.type = NEBTYPE_HOSTSTATUS_UPDATE;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567634416, .tv_usec = 981815 };
    d.object_ptr = &test_host;

    ndo_handle_host_status(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_hoststatus WHERE "
        "instance_id = 1 AND host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) "
        "AND status_update_time = FROM_UNIXTIME(1567634416) AND output = 'OK - 127.0.0.1 rta 0.012ms lost 0%' "
        "AND long_output = '' AND perfdata = 'rta=0.012ms;3000.000;5000.000;0; rtmax=0.036ms;;;; rtmin=0.005ms;;;; pl=0%;80;100;0;100' "
        "AND current_state = 0 AND has_been_checked = 1 AND should_be_scheduled = 1 "
        "AND current_check_attempt = 1 AND max_check_attempts = 5 "
        "AND last_check = FROM_UNIXTIME(1567626758) AND next_check = FROM_UNIXTIME(1567627058) "
        "AND check_type = 0 AND last_state_change = FROM_UNIXTIME(1565516515) "
        "AND last_hard_state_change = FROM_UNIXTIME(1565516515) AND last_hard_state = 0 "
        "AND last_time_up = FROM_UNIXTIME(1567626758) AND last_time_down = FROM_UNIXTIME(0) "
        "AND last_time_unreachable = FROM_UNIXTIME(0) AND state_type = 1 "
        "AND last_notification = FROM_UNIXTIME(0) AND next_notification = FROM_UNIXTIME(0) "
        "AND no_more_notifications = 0 AND notifications_enabled = 1 "
        "AND problem_has_been_acknowledged = 0 AND acknowledgement_type = 0 "
        "AND current_notification_number = 0 AND passive_checks_enabled = 1 "
        "AND active_checks_enabled = 1 AND event_handler_enabled = 1 "
        "AND flap_detection_enabled = 1 AND is_flapping = 0 "
        "AND percent_state_change = 0 AND latency = 0.0019690000917762518 "
        "AND execution_time = 0.0041120000000000002 AND scheduled_downtime_depth = 0 "
        "AND failure_prediction_enabled = 0 AND process_performance_data = 1 "
        "AND obsess_over_host = 1 AND modified_host_attributes = 0 "
        "AND event_handler = '' AND check_command = 'check_xi_host_ping!3000.0!80%!5000.0!100%' "
        "AND normal_check_interval = 5 AND retry_check_interval = 1 "
        "AND check_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 9 AND name1 = 'xi_timeperiod_24x7' AND name2 IS NULL LIMIT 1)");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_service_status)
{
    nebstruct_service_status_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* also uses test_host global */

    d.type = NEBTYPE_SERVICESTATUS_UPDATE;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1567709361, .tv_usec = 154555 };
    d.object_ptr = &test_service;

    ndo_handle_service_status(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_servicestatus WHERE "
        "instance_id = 1 AND service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1)"
        "AND status_update_time = FROM_UNIXTIME(1567709361) AND output = 'This is not real output' "
        "AND long_output = '' AND perfdata = '' AND current_state = 0 "
        "AND has_been_checked = 1 AND should_be_scheduled = 1 "
        "AND current_check_attempt = 1 AND max_check_attempts = 5 "
        "AND last_check = FROM_UNIXTIME(1567709309) AND next_check = FROM_UNIXTIME(1567709609) "
        "AND check_type = 0 AND last_state_change = FROM_UNIXTIME(1567621885) "
        "AND last_hard_state_change = FROM_UNIXTIME(1567621885) AND last_hard_state = 0 "
        "AND last_time_ok = FROM_UNIXTIME(1567630417) AND last_time_warning = FROM_UNIXTIME(1567542993) "
        "AND last_time_unknown = FROM_UNIXTIME(0) AND last_time_critical = FROM_UNIXTIME(1567539221) "
        "AND state_type = 1 AND last_notification = FROM_UNIXTIME(0) "
        "AND next_notification = FROM_UNIXTIME(3600) AND no_more_notifications = 0 "
        "AND notifications_enabled = 1 AND problem_has_been_acknowledged = 0 "
        "AND acknowledgement_type = 0 AND current_notification_number = 0 "
        "AND passive_checks_enabled = 1 AND active_checks_enabled = 1 "
        "AND event_handler_enabled = 1 AND flap_detection_enabled = 0 "
        "AND is_flapping = 0 AND percent_state_change = 0 AND latency = 0.0020620001014322042 "
        "AND execution_time = 0.0032190000000000001 AND scheduled_downtime_depth = 0 "
        "AND failure_prediction_enabled = 0 AND process_performance_data = 1 "
        "AND obsess_over_service = 1 AND modified_service_attributes = 16 AND event_handler = '' "
        "AND check_command = 'check_xi_service_http' AND normal_check_interval = 5 AND retry_check_interval = 1 "
        "AND check_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 9 AND name1 = 'xi_timeperiod_24x7' AND name2 IS NULL LIMIT 1) ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);
}
END_TEST


START_TEST (test_contact_status)
{
    nebstruct_contact_status_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_CONTACTSTATUS_UPDATE;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568226545, .tv_usec = 362038 };
    d.object_ptr = &test_contact;

    ndo_handle_contact_status(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_contactstatus WHERE "
        "instance_id = 1 AND contact_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = 'nagiosadmin' AND name2 IS NULL LIMIT 1) "
        "AND status_update_time = FROM_UNIXTIME(1568226545) AND host_notifications_enabled = 1 "
        "AND service_notifications_enabled = 1 AND last_host_notification = FROM_UNIXTIME(1567525739) "
        "AND last_service_notification = FROM_UNIXTIME(1567543267) AND modified_attributes = 0 "
        "AND modified_host_attributes = 0 AND modified_service_attributes = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);
}
END_TEST


START_TEST (test_notification_data)
{
    nebstruct_notification_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* Create a service notification */
    d.type = NEBTYPE_NOTIFICATION_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568228348, .tv_usec = 750099 };
    d.notification_type = SERVICE_NOTIFICATION;
    d.start_time = (struct timeval) { .tv_sec = 1568228312, .tv_usec = 479539 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.reason_type = 0;
    d.state = 0;
    d.output = strdup("Return some status message");
    d.ack_author = NULL;
    d.ack_data = NULL;
    d.escalated = 0;
    d.contacts_notified = 0;
    d.object_ptr = &test_service;

    ndo_handle_notification(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_notifications WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1568228312) "
        "AND start_time_usec = 479539 AND end_time = FROM_UNIXTIME(0) "
        "AND end_time_usec = 0 AND notification_type = 1 "
        "AND notification_reason = 0 AND object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1) "
        "AND state = 0 AND output = 'Return some status message' "
        "AND long_output = 'Return some status message' AND escalated = 0 "
        "AND contacts_notified = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);


    /* Create a host notification */
    d.type = NEBTYPE_NOTIFICATION_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568228350, .tv_usec = 750101 };
    d.notification_type = HOST_NOTIFICATION;
    d.start_time = (struct timeval) { .tv_sec = 1568228314, .tv_usec = 479541 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.reason_type = 0;
    d.state = 0;
    d.output = strdup("Return some other status message");
    d.ack_author = NULL;
    d.ack_data = NULL;
    d.escalated = 0;
    d.contacts_notified = 0;
    d.object_ptr = &test_service;

    ndo_handle_notification(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_notifications WHERE "
        "instance_id = 1 AND start_time = FROM_UNIXTIME(1568228314) "
        "AND start_time_usec = 479541 AND end_time = FROM_UNIXTIME(0) "
        "AND end_time_usec = 0 AND notification_type = 0 "
        "AND notification_reason = 0 AND object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) "
        "AND state = 0 AND output = 'Return some other status message' "
        "AND long_output = 'Return some other status message' AND escalated = 0 "
        "AND contacts_notified = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);


}
END_TEST


START_TEST (test_contact_notification_data)
{
    nebstruct_contact_notification_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_CONTACTNOTIFICATION_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568235242, .tv_usec = 587705 };
    d.notification_type = 1;
    d.start_time = (struct timeval) { .tv_sec = 1568235236, .tv_usec = 683676 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.contact_name = strdup("nagiosadmin");
    d.reason_type = 0;
    d.state = 0;
    d.output = strdup("Return another status message");
    d.ack_author = 0x0;
    d.ack_data = 0x0;
    d.escalated = 0;
    d.object_ptr = &test_contact;
    d.contact_ptr = &test_contact;

    ndo_handle_contact_notification(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_contactnotifications WHERE "
        "instance_id = 1 AND notification_id = 0 "
        "AND start_time = FROM_UNIXTIME(1568235236) AND start_time_usec = 683676 "
        "AND end_time = FROM_UNIXTIME(0) AND end_time_usec = 0 "
        "AND contact_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = 'nagiosadmin' AND name2 IS NULL LIMIT 1)");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_contact_notification_method_data)
{
    nebstruct_contact_notification_method_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = 604;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568237100, .tv_usec = 616343 };
    d.notification_type = 1;
    d.start_time = (struct timeval) { .tv_sec = 1568237093, .tv_usec = 608098 };
    d.end_time = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.contact_name = strdup("nagiosadmin");
    d.command_name = strdup(""); // This is probably a Core bug and will result in command_object_id of -1 below.
    d.command_args = NULL;
    d.reason_type = 0;
    d.state = 0;
    d.output = strdup("some output");
    d.ack_author = NULL;
    d.ack_data = NULL;
    d.escalated = 0;
    d.object_ptr = &test_service;
    d.contact_ptr = &test_contact;

    ndo_handle_contact_notification_method(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_contactnotificationmethods WHERE "
        "instance_id = 1 AND contactnotification_id = 0 "
        "AND start_time = FROM_UNIXTIME(1568237093) AND start_time_usec = 608098 "
        "AND end_time = FROM_UNIXTIME(0) AND end_time_usec = 0 "
        "AND command_object_id = -1 AND command_args = '' ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);
}
END_TEST


START_TEST (test_external_command)
{
    nebstruct_external_command_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_EXTERNALCOMMAND_START;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568238575, .tv_usec = 842679 };
    d.command_type = 1;
    d.entry_time = 1568238569;
    d.command_string = strdup("ADD_HOST_COMMENT");
    d.command_args = strdup("_testhost_1;1;Nagios Admin; This is a comment");

    ndo_handle_external_command(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_externalcommands WHERE "
        "instance_id = 1 AND command_type = 1 "
        " AND entry_time = FROM_UNIXTIME(1568238569) AND command_name = 'ADD_HOST_COMMENT'"
        " AND command_args = '_testhost_1;1;Nagios Admin; This is a comment'");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_retention_data)
{
    nebstruct_retention_data d;

    /* Retention handling function doesn't exist yet */
}
END_TEST


START_TEST (test_acknowledgement_data)
{
    nebstruct_acknowledgement_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = NEBTYPE_ACKNOWLEDGEMENT_ADD;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568299147, .tv_usec = 318748 };
    d.acknowledgement_type = 1;
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.state = 1;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup(" this is an ack");
    d.is_sticky = 0;
    d.persistent_comment = 1;
    d.notify_contacts = 0;
    d.object_ptr = &test_service;

    ndo_handle_acknowledgement(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_acknowledgements WHERE "
        "instance_id = 1 AND entry_time = FROM_UNIXTIME(1568299147) "
        "AND entry_time_usec = 318748 AND acknowledgement_type = 1 "
        "AND object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1) AND state = 1 "
        "AND author_name = 'Nagios Admin' AND comment_data = ' this is an ack' "
        "AND is_sticky = 0 AND persistent_comment = 1 "
        "AND notify_contacts = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    d.type = NEBTYPE_ACKNOWLEDGEMENT_ADD;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568300150, .tv_usec = 797402 };
    d.acknowledgement_type = 0;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.state = 1;
    d.author_name = strdup("Nagios Admin");
    d.comment_data = strdup(" this is a different ack");
    d.is_sticky = 0;
    d.persistent_comment = 1;
    d.notify_contacts = 0;
    d.object_ptr = &test_host;

    ndo_handle_acknowledgement(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_acknowledgements WHERE "
        "instance_id = 1 AND entry_time = FROM_UNIXTIME(1568300150) "
        "AND entry_time_usec = 797402 AND acknowledgement_type = 0 "
        "AND object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) AND state = 1 "
        "AND author_name = 'Nagios Admin' AND comment_data = ' this is a different ack' "
        "AND is_sticky = 0 AND persistent_comment = 1 "
        "AND notify_contacts = 0 ");

    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);

}
END_TEST


START_TEST (test_statechange_data)
{
    nebstruct_statechange_data d;

    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    d.type = 1801;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568301129, .tv_usec = 715455 };
    d.statechange_type = 0;
    d.host_name = strdup("_testhost_1");
    d.service_description = NULL;
    d.state = 1;
    d.state_type = 0;
    d.current_attempt = 1;
    d.max_attempts = 5;
    d.output = strdup("not OK");
    d.object_ptr = &test_host;
    d.longoutput = NULL;

    ndo_handle_state_change(d.type, &d);

    mysql_query(mysql_connection, "SELECT 1 FROM nagios_statehistory WHERE "
        "instance_id = 1 AND state_time = FROM_UNIXTIME(1568301129) "
        "AND state_time_usec = 715455 AND object_id = (SELECT object_id from nagios_objects WHERE objecttype_id = 1 AND name1 = '_testhost_1' AND name2 IS NULL LIMIT 1) "
        "AND state_change = 1 AND state = 1 "
        "AND state_type = 0 AND current_check_attempt = 1 "
        "AND max_check_attempts = 5 AND last_state = 0 "
        "AND last_hard_state = 0 AND output = 'not OK' "
        "AND long_output = ''");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "1"), 0);
    }
    mysql_free_result(tmp_result);

    d.type = 1801;
    d.flags = 0;
    d.attr = 0;
    d.timestamp = (struct timeval) { .tv_sec = 1568301309, .tv_usec = 12692 };
    d.statechange_type = 1;
    d.host_name = strdup("_testhost_1");
    d.service_description = strdup("_testservice_http");
    d.state = 1;
    d.state_type = 0;
    d.current_attempt = 1;
    d.max_attempts = 5;
    d.output = strdup("not OK");
    d.object_ptr = &test_service;
    d.longoutput = NULL;

    ndo_handle_state_change(d.type, &d);

    mysql_query(mysql_connection, "SELECT 2 FROM nagios_statehistory WHERE "
        "instance_id = 1 AND state_time = FROM_UNIXTIME(1568301309) "
        "AND state_time_usec = 12692 AND object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = '_testhost_1' AND name2 = '_testservice_http' LIMIT 1) "
        "AND state_change = 1 AND state = 1 "
        "AND state_type = 0 AND current_check_attempt = 1 "
        "AND max_check_attempts = 5 AND last_state = 0 "
        "AND last_hard_state = 0 AND output = 'not OK' "
        "AND long_output = ''");


    tmp_result = mysql_store_result(mysql_connection);
    ck_assert(tmp_result != NULL);

    if (tmp_result != NULL) {
        tmp_row = mysql_fetch_row(tmp_result);
    }
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert_int_eq(strcmp(tmp_row[0], "2"), 0);
    }
    mysql_free_result(tmp_result);



}
END_TEST


Suite * handler_suite(void)
{
    Suite * suite      = NULL;
    TCase * tc_handler = NULL;

    suite = suite_create("handler_suite");

    tc_handler = tcase_create("broker handlers");

    tcase_add_test(tc_handler, test_program_state);
    tcase_add_test(tc_handler, test_timed_event);
    tcase_add_test(tc_handler, test_log_data);
    tcase_add_test(tc_handler, test_system_command);
    tcase_add_test(tc_handler, test_event_handler);
    tcase_add_test(tc_handler, test_host_check);
    tcase_add_test(tc_handler, test_service_check);
    tcase_add_test(tc_handler, test_comment_data);
    tcase_add_test(tc_handler, test_downtime_data);
    tcase_add_test(tc_handler, test_flapping_data);
    tcase_add_test(tc_handler, test_program_status);
    tcase_add_test(tc_handler, test_host_status);
    tcase_add_test(tc_handler, test_service_status);
    tcase_add_test(tc_handler, test_contact_status);
    tcase_add_test(tc_handler, test_notification_data);
    tcase_add_test(tc_handler, test_contact_notification_data);
    tcase_add_test(tc_handler, test_contact_notification_method_data);
    tcase_add_test(tc_handler, test_external_command);
    tcase_add_test(tc_handler, test_retention_data);
    tcase_add_test(tc_handler, test_acknowledgement_data);
    tcase_add_test(tc_handler, test_statechange_data);

    suite_add_tcase(suite, tc_handler);

    return suite;
}

int main(int argc, char const * argv[])
{
    int number_failed = 0;
    int i = 0;
    char * config_file = NULL;

    /* we must specify the location of the config file */
    if (argc <= 1) {
        printf("%s\n", "You must specify a config file location");
        exit(EXIT_FAILURE);
    }

    config_file = strdup(argv[1]);

    if (load_neb_module(config_file) != NDO_OK) {

        free(neb_handle);

        printf("%s\n", "Unable to load NEB Module");
        exit(EXIT_FAILURE);
    }

    free(config_file);

    populate_all_objects();

    Suite * s_core = t_suite();
    Suite * s_handler = handler_suite();

    SRunner * runner[NUM_SUITES] = { NULL };

    runner[0] = srunner_create(s_core);
    runner[1] = srunner_create(s_handler);

    for (i = 0; i < NUM_SUITES; i++) {

        /* disable forking for runners (for memory leak checks) */
        srunner_set_fork_status(runner[i], CK_NOFORK);

        /* available print_modes:
            CK_SILENT
            CK_MINIMAL
            CK_NORMAL
            CK_VERBOSE
            CK_ENV (get print mode from ENV CK_VERBOSITY (silent, minimal, etc.))
            CK_SUBUNIT
        */

        srunner_run_all(runner[i], CK_VERBOSE);
        number_failed += srunner_ntests_failed(runner[i]);

        srunner_free(runner[i]);
    }


    printf("%s\n", "************************");
    printf("Total failures: %d\n", number_failed);
    printf("%s\n", "************************");

    unload_neb_module();

    if (number_failed == 0) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
