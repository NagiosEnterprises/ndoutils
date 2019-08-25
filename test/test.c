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
#include "../include/nagios/nebstructs.h"
#include "../include/nagios/neberrors.h"

#include "nagios-stubs.c"


#define NUM_SUITES 2


void * neb_handle = NULL;


int load_neb_module()
{
    neb_handle = malloc(1);
    if (neb_handle == NULL) {
        return NDO_ERROR;
    }

    return nebmodule_init(0, "/home/travis/build/NagiosEnterprises/ndoutils/config/ndo.cfg-sample", neb_handle);
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
    ck_assert_int_eq(NDO_OK, NEB_TRUE);
    ck_assert_int_eq(NDO_ERROR, NEB_ERROR);
    ck_assert_int_eq(TRUE, NEB_TRUE);
    ck_assert_int_eq(FALSE, NEB_FALSE);
}


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


START_TEST (program_state)
{
    nebstruct_process_data d;
}
END_TEST


START_TEST (timed_event)
{
    nebstruct_timed_event_data d;
}
END_TEST


START_TEST (log_data)
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


START_TEST (system_command)
{
    nebstruct_system_command_data d;

}
END_TEST


START_TEST (event_handler)
{
    nebstruct_event_handler_data d;
}
END_TEST


START_TEST (host_check)
{
    nebstruct_host_check_data d;
}
END_TEST


START_TEST (service_check)
{
    nebstruct_service_check_data d;
}
END_TEST


START_TEST (comment_data)
{
    nebstruct_comment_data d;
}
END_TEST


START_TEST (downtime_data)
{
    nebstruct_downtime_data d;
}
END_TEST


START_TEST (flapping_data)
{
    nebstruct_flapping_data d;
}
END_TEST


START_TEST (program_status)
{
    nebstruct_program_status_data d;
}
END_TEST


START_TEST (host_status)
{
    nebstruct_host_status_data d;
}
END_TEST


START_TEST (service_status)
{
    nebstruct_service_status_data d;
}
END_TEST


START_TEST (contact_status)
{
    nebstruct_service_status_data d;
}
END_TEST


START_TEST (notification_data)
{
    nebstruct_notification_data d;
}
END_TEST


START_TEST (contact_notification_data)
{
    nebstruct_contact_notification_data d;
}
END_TEST


START_TEST (contact_notification_method_data)
{
    nebstruct_contact_notification_method_data d;
}
END_TEST


START_TEST (external_command)
{
    nebstruct_external_command_data d;
}
END_TEST


START_TEST (retention_data)
{
    nebstruct_retention_data d;
}
END_TEST


START_TEST (acknowledgement_data)
{
    nebstruct_acknowledgement_data d;
}
END_TEST


START_TEST (statechange_data)
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

    tcase_add_test(tc_handler, program_state);
    tcase_add_test(tc_handler, timed_event);
    tcase_add_test(tc_handler, log_data);
    tcase_add_test(tc_handler, system_command);
    tcase_add_test(tc_handler, event_handler);
    tcase_add_test(tc_handler, host_check);
    tcase_add_test(tc_handler, service_check);
    tcase_add_test(tc_handler, comment_data);
    tcase_add_test(tc_handler, downtime_data);
    tcase_add_test(tc_handler, flapping_data);
    tcase_add_test(tc_handler, program_status);
    tcase_add_test(tc_handler, host_status);
    tcase_add_test(tc_handler, service_status);
    tcase_add_test(tc_handler, contact_status);
    tcase_add_test(tc_handler, notification_data);
    tcase_add_test(tc_handler, contact_notification_data);
    tcase_add_test(tc_handler, contact_notification_method_data);
    tcase_add_test(tc_handler, external_command);
    tcase_add_test(tc_handler, retention_data);
    tcase_add_test(tc_handler, acknowledgement_data);
    tcase_add_test(tc_handler, statechange_data);

    suite_add_tcase(suite, tc_handler);

    return suite;
}


int main(void)
{
    int number_failed = 0;
    int i = 0;

    if (load_neb_module() != NDO_OK) {

        free(neb_handle);

        printf("%s\n", "Unable to load NEB Module");
        exit(EXIT_FAILURE);
    }

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
