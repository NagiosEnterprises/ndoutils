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


START_TEST (test_system_command)
{
    nebstruct_system_command_data d;

}
END_TEST


START_TEST (test_event_handler)
{
    nebstruct_event_handler_data d;
}
END_TEST


START_TEST (test_host_check)
{
    nebstruct_host_check_data d;
}
END_TEST


START_TEST (test_service_check)
{
    nebstruct_service_check_data d;
}
END_TEST


START_TEST (test_comment_data)
{
    nebstruct_comment_data d_add;
    nebstruct_comment_data d_delete;
    MYSQL_ROW tmp_row;
    MYSQL_RES *tmp_result;

    /* Add a host comment */
    d_add.type = NEBTYPE_COMMENT_ADD;
    d_add.flags = 0;
    d_add.attr = 0;
    d_add.timestamp = (struct timeval) { .tv_sec = 1567021700, .tv_usec = 29064 };
    d_add.comment_type = 1;
    d_add.host_name = strdup("_testhost_1");
    d_add.service_description = NULL;
    d_add.entry_time = 1567021619,
    d_add.author_name = strdup("Nagios Admin");
    d_add.comment_data = strdup("this is a unique comment");
    d_add.persistent = 1;
    d_add.source = 1;
    d_add.entry_type = 1;
    d_add.expires = 0;
    d_add.expire_time = 0;
    d_add.comment_id = 1;
    d_add.object_ptr = NULL;

    ndo_handle_comment(0, &d_add);


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

    d_delete.type = NEBTYPE_COMMENT_DELETE;
    d_delete.flags = 0;
    d_delete.attr = 0;
    d_delete.timestamp = (struct timeval) { .tv_sec = 1567089801, .tv_usec = 725979 };
    d_delete.comment_type = 1;
    d_delete.host_name = strdup("_testhost_1");
    d_delete.service_description = NULL;
    d_delete.entry_time = 1567021619;
    d_delete.author_name = strdup("Nagios Admin");
    d_delete.comment_data = strdup("this is a unique comment");
    d_delete.persistent = 1;
    d_delete.source = 1;
    d_delete.entry_type = 1;
    d_delete.expires = 0;
    d_delete.expire_time = 0;
    d_delete.comment_id = 1;
    d_delete.object_ptr = NULL;

    ndo_handle_comment(0, &d_delete);

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
    ck_assert(tmp_row != NULL);

    if (tmp_row != NULL) {
        ck_assert(tmp_row == NULL);
    }
    mysql_free_result(tmp_result);


}
END_TEST


START_TEST (test_downtime_data)
{
    nebstruct_downtime_data d;
}
END_TEST


START_TEST (test_flapping_data)
{
    nebstruct_flapping_data d;
}
END_TEST


START_TEST (test_program_status)
{
    nebstruct_program_status_data d;
}
END_TEST


START_TEST (test_host_status)
{
    nebstruct_host_status_data d;
}
END_TEST


START_TEST (test_service_status)
{
    nebstruct_service_status_data d;
}
END_TEST


START_TEST (test_contact_status)
{
    nebstruct_service_status_data d;
}
END_TEST


START_TEST (test_notification_data)
{
    nebstruct_notification_data d;
}
END_TEST


START_TEST (test_contact_notification_data)
{
    nebstruct_contact_notification_data d;
}
END_TEST


START_TEST (test_contact_notification_method_data)
{
    nebstruct_contact_notification_method_data d;
}
END_TEST


START_TEST (test_external_command)
{
    nebstruct_external_command_data d;
}
END_TEST


START_TEST (test_retention_data)
{
    nebstruct_retention_data d;
}
END_TEST


START_TEST (test_acknowledgement_data)
{
    nebstruct_acknowledgement_data d;
}
END_TEST


START_TEST (test_statechange_data)
{
    nebstruct_statechange_data d;
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
