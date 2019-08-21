/*

apt-get install -y check lcov gcovr

just compile:
gcc -g test.c -lcheck -lsubunit -lm -lrt -lpthread -o test

compile for coverage:
gcc $(mysql_config --cflags) -g -ggdb3 -fprofile-arcs -ftest-coverage -o test test.c ../src/ndo.obj $(mysql_config --libs) -lcheck -lsubunit -lm -lrt -lpthread -ldl

make coverage:
lcov -c -d . -o coverage.info-file
genhtml coverage.info-file -o coverage/

coverage report:
gcovr --exclude="test.c" -r . 

*/

#include <mysql.h>
#include <dlfcn.h>


#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include "../include/ndo.h"
#include "../include/nagios/downtime.h"

#define NUM_SUITES 1


START_TEST (booleans_are_sane)
{
    ck_assert_int_eq(TRUE, 1);
    ck_assert_int_eq(FALSE, 0);
}
END_TEST


Suite * t_suite(void)
{
    Suite * suite   = NULL;
    TCase * tc_core = NULL;

    suite = suite_create("t_suite");

    tc_core = tcase_create("core functionality");

    /* add our tests */
    tcase_add_test(tc_core, booleans_are_sane);

    /* link them with the suite */
    suite_add_tcase(suite, tc_core);

    return suite;
}

void * neb_handle = NULL;

void load_neb_module()
{
    neb_handle = malloc(1);
    nebmodule_init(0, "", neb_handle);
}

void unload_neb_module()
{
    nebmodule_deinit(0, 0);
    free(neb_handle);
}


int main(void)
{
    int number_failed = 0;
    int i = 0;

    load_neb_module();

    Suite * s_core = t_suite();

    SRunner * runner[NUM_SUITES] = { NULL };

    runner[0] = srunner_create(s_core);

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
