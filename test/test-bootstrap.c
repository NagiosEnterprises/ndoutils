
#include <mysql.h>
#include <string.h>

#include "../include/test.h"
#include "../include/ndo.h"

/* This file contains functions which allocate/populate sample structs and
 * return pointers to them, for use in testing the handler functions.
 * It also populates nagios_objects and related tables with similar sample objects.
 */

void populate_commands()
{

    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 12, name1 = 'check_xi_host_ping', name2 = '', is_active = 1");
    mysql_query(mysql_connection, "INSERT INTO nagios_commands SET "
                                  "instance_id = 1, config_type = 1, "
                                  "object_id = (SELECT object_id from nagios_objects WHERE name1 = 'check_xi_host_ping' AND objecttype_id = 12), "
                                  "command_line = '$USER1$/check_icmp -H $HOSTADDRESS$ -w $ARG1$,$ARG2$ -c $ARG3$,$ARG4$ -p 5'");

    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 12, name1 = 'check_xi_service_ping', name2 = '', is_active = 1");
    mysql_query(mysql_connection, "INSERT INTO nagios_commands SET "
                                  "instance_id = 1, config_type = 1, "
                                  "object_id = (SELECT object_id from nagios_objects WHERE name1 = 'check_xi_service_ping' AND objecttype_id = 12) "
                                  "command_line = '$USER1$/check_icmp -H $HOSTADDRESS$ -w $ARG1$,$ARG2$ -c $ARG3$,$ARG4$ -p 5'");
}

struct host populate_hosts(timeperiod * tp)
{
    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 1, name1 = '_testhost_1', name2 = ', is_active = 1");

    struct host the_host = {
        .id = 0,
        .name = strdup("_testhost_1"),
        .display_name = strdup("_testhost_1"),
        .alias = strdup("_testhost_1"),
        .address = strdup("127.0.0.1"),
        .parent_hosts = NULL,
        .child_hosts = NULL,
        .services = NULL, // Originally set to 0x6f95f
        .check_command = strdup("check_xi_host_ping!3000.0!80%!5000.0!100%"),
        .initial_state = 0,
        .check_interval = 5,
        .retry_interval = 1,
        .max_attempts = 5,
        .event_handler = NULL,
        .contact_groups = NULL,
        .contacts = NULL, // Originally set to 0x6de3c
        .notification_interval = 60,
        .first_notification_delay = 0,
        .notification_options = 4294967295,
        .hourly_value = 0,
        .notification_period = strdup("xi_timeperiod_24x7"),
        .check_period = strdup("xi_timeperiod_24x7"),
        .flap_detection_enabled = 1,
        .low_flap_threshold = 0,
        .high_flap_threshold = 0,
        .flap_detection_options = -1,
        .stalking_options = 0,
        .check_freshness = 0,
        .freshness_threshold = 0,
        .process_performance_data = 1,
        .checks_enabled = 1,
        .check_source = "Core Worker 107565",
        .accept_passive_checks = 1,
        .event_handler_enabled = 1,
        .retain_status_information = 1,
        .retain_nonstatus_information = 1,
        .obsess = 1,
        .notes = NULL,
        .notes_url = NULL,
        .action_url = NULL,
        .icon_image = NULL,
        .icon_image_alt = NULL,
        .statusmap_image = NULL,
        .vrml_image = NULL,
        .have_2d_coords = 0,
        .x_2d = -1,
        .y_2d = -1,
        .have_3d_coords = 0,
        .x_3d = 0,
        .y_3d = 0,
        .z_3d = 0,
        .should_be_drawn = 1,
        .custom_variables = NULL,
        .problem_has_been_acknowledged = 0,
        .acknowledgement_type = 0,
        .check_type = 0,
        .current_state = 0,
        .last_state = 0,
        .last_hard_state = 0,
        .plugin_output = strdup("OK - 127.0.0.1 rta 0.012ms lost 0%"),
        .long_plugin_output = NULL,
        .perf_data = strdup("rta=0.012ms;3000.000;5000.000;0; rtmax=0.036ms;;;; rtmin=0.005ms;;;; pl=0%;80;100;0;100"),
        .state_type = 1,
        .current_attempt = 1,
        .current_event_id = 0,
        .last_event_id = 0,
        .current_problem_id = 0,
        .last_problem_id = 0,
        .latency = 0.0019690000917762518,
        .execution_time = 0.0041120000000000002,
        .is_executing = 0,
        .check_options = 0,
        .notifications_enabled = 1,
        .last_notification = 0,
        .next_notification = 0,
        .next_check = 1567627058,
        .should_be_scheduled = 1,
        .last_check = 1567626758,
        .last_state_change = 1565516515,
        .last_hard_state_change = 1565516515,
        .last_time_up = 1567626758,
        .last_time_down = 0,
        .last_time_unreachable = 0,
        .has_been_checked = 1,
        .is_being_freshened = 0,
        .notified_on = 0,
        .current_notification_number = 0,
        .no_more_notifications = 0,
        .current_notification_id = 1106495,
        .check_flapping_recovery_notification = 0,
        .scheduled_downtime_depth = 0,
        .pending_flex_downtime = 0,
        .state_history = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .state_history_index = 20,
        .last_state_history_update = 1567626758,
        .is_flapping = 0,
        .flapping_comment_id = 0,
        .percent_state_change = 0,
        .total_services = 2,
        .total_service_check_interval = 0,
        .modified_attributes = 0,
        .event_handler_ptr = NULL,
        .check_command_ptr = NULL, // Originally set to 0x6f711
        .check_period_ptr = tp, // Originally set to 0x6c352
        .notification_period_ptr = tp, // Originally set to 0x6c352
        .hostgroups_ptr = NULL,
        .exec_deps = NULL,
        .notify_deps = NULL,
        .escalation_list = NULL,
        .next = NULL,
        .next_check_event = NULL, // Originally set to 0x6d8f9
    };

    return the_host;
}

void free_host(struct host the_host)
{

    free(the_host.name);
    free(the_host.display_name);
    free(the_host.alias);
    free(the_host.address);
    free(the_host.check_command);
    free(the_host.notification_period);
    free(the_host.check_period);
    free(the_host.plugin_output);
    free(the_host.perf_data);
}

struct service populate_service(timeperiod * tp, host * hst)
{
    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 2, name1 = '_testhost_1', name2 = '_testservice_ping', is_active = 1");

    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 2, name1 = '_testhost_1', name2 = '_testservice_http', is_active = 1");

    struct service the_service = {
        .id = 0,
        .host_name = strdup("_testhost_1"),
        .description = strdup("_testservice_http"),
        .display_name = strdup("_testservice_http"),
        .parents = NULL,
        .children = NULL,
        .check_command = strdup("check_xi_service_http"),
        .event_handler = NULL,
        .initial_state = 0,
        .check_interval = 5,
        .retry_interval = 1,
        .max_attempts = 5,
        .parallelize = 1,
        .contact_groups = NULL,
        .contacts = NULL, // Originally set to 0x6f91f0
        .notification_interval = 60,
        .first_notification_delay = 0,
        .notification_options = 4294967295,
        .stalking_options = 0,
        .hourly_value = 0,
        .is_volatile = 0,
        .notification_period = strdup("xi_timeperiod_24x7"),
        .check_period = strdup("xi_timeperiod_24x7"),
        .flap_detection_enabled = 0,
        .low_flap_threshold = 0,
        .high_flap_threshold = 0,
        .flap_detection_options = 4294967295,
        .process_performance_data = 1,
        .check_freshness = 0,
        .freshness_threshold = 0,
        .accept_passive_checks = 1,
        .event_handler_enabled = 1,
        .checks_enabled = 1,
        .check_source = "Core Worker 107566",
        .retain_status_information = 1,
        .retain_nonstatus_information = 1,
        .notifications_enabled = 1,
        .obsess = 1,
        .notes = NULL,
        .notes_url = NULL,
        .action_url = NULL,
        .icon_image = NULL,
        .icon_image_alt = NULL,
        .custom_variables = NULL,
        .problem_has_been_acknowledged = 0,
        .acknowledgement_type = 0,
        .host_problem_at_last_check = 0,
        .check_type = 0,
        .current_state = 0,
        .last_state = 0,
        .last_hard_state = 0,
        .plugin_output = strdup("This is not real output"),
        .long_plugin_output = NULL,
        .perf_data = NULL,
        .state_type = 1,
        .next_check = 1567709609,
        .should_be_scheduled = 1,
        .last_check = 1567709309,
        .current_attempt = 1,
        .current_event_id = 100176,
        .last_event_id = 100175,
        .current_problem_id = 0,
        .last_problem_id = 22135,
        .last_notification = 0,
        .next_notification = 3600,
        .no_more_notifications = 0,
        .check_flapping_recovery_notification = 0,
        .last_state_change = 1567621885,
        .last_hard_state_change = 1567621885,
        .last_time_ok = 1567630417,
        .last_time_warning = 1567542993,
        .last_time_unknown = 0,
        .last_time_critical = 1567539221,
        .has_been_checked = 1,
        .is_being_freshened = 0,
        .notified_on = 0,
        .current_notification_number = 0,
        .current_notification_id = 1106501,
        .latency = 0.0020620001014322042,
        .execution_time = 0.0032190000000000001,
        .is_executing = 0,
        .check_options = 0,
        .scheduled_downtime_depth = 0,
        .pending_flex_downtime = 0,
        .state_history = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .state_history_index = 20,
        .is_flapping = 0,
        .flapping_comment_id = 0,
        .percent_state_change = 0,
        .modified_attributes = 16,
        .host_ptr = hst, // Originally set to 0x6f8b00
        .event_handler_ptr = NULL,
        .event_handler_args = NULL,
        .check_command_ptr = NULL, // Originally set to 0x6f7950
        .check_command_args = NULL,
        .check_period_ptr = tp, // Originally set to 0x6c3520
        .notification_period_ptr = tp, // Originally set to 0x6c3520
        .servicegroups_ptr = NULL,
        .exec_deps = NULL,
        .notify_deps = NULL,
        .escalation_list = NULL,
        .next = NULL, // Originally set to 0x6f9210
        .next_check_event = NULL, // Originally set to 0x6d9fb0
    };

    return the_service;
}

void free_service(struct service the_service)
{

    free(the_service.host_name);
    free(the_service.description);
    free(the_service.display_name);
    free(the_service.check_command);
    free(the_service.notification_period);
    free(the_service.check_period);
    free(the_service.plugin_output);
}

struct contact populate_contact(timeperiod * tp)
{

    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 10, name1 = 'nagiosadmin', name2 = '', is_active = 1");

    struct contact the_contact = {
        .id = 0,
        .name = strdup("nagiosadmin"),
        .alias = strdup("Nagios Admin"),
        .email = strdup("nagios@localhost"),
        .pager = NULL,
        .address = {NULL, NULL, NULL, NULL, NULL, NULL},
        .host_notification_commands = NULL, // Originally set to 0x6e07d0
        .service_notification_commands = NULL, // Originally set to 0x6e0bf0
        .host_notification_options = 6151,
        .service_notification_options = 6159,
        .minimum_value = 0,
        .host_notification_period = strdup("24x7"),
        .service_notification_period = strdup("24x7"),
        .host_notifications_enabled = 1,
        .service_notifications_enabled = 1,
        .can_submit_commands = 1,
        .retain_status_information = 1,
        .retain_nonstatus_information = 1,
        .custom_variables = NULL,
        .last_host_notification = 1567525739,
        .last_service_notification = 1567543267,
        .modified_attributes = 0,
        .modified_host_attributes = 0,
        .modified_service_attributes = 0,
        .host_notification_period_ptr = tp,
        .service_notification_period_ptr = tp,
        .contactgroups_ptr = NULL, // Originally set to 0x6f9690
        .next = NULL,
    };

    return the_contact;
}

void free_contact(struct contact the_contact)
{

    free(the_contact.name);
    free(the_contact.alias);
    free(the_contact.email);
    free(the_contact.host_notification_period);
    free(the_contact.service_notification_period);
}

struct timeperiod populate_timeperiods()
{

    mysql_query(mysql_connection, "INSERT INTO nagios_objects SET "
                                  "instance_id = 1, objecttype_id = 9, name1 = 'xi_timeperiod_24x7', is_active = 1");
    struct timerange * tr = calloc(1, sizeof(struct timerange));

    tr->range_start = 0;
    tr->range_end = 86400;
    tr->next = NULL;

    struct timeperiod the_timeperiod = {
        .id = 6,
        .name = strdup("xi_timeperiod_24x7"),
        .alias = strdup("24 Hours A Day, 7 Days A Week"),
        .days = {tr, tr, tr, tr, tr, tr, tr},
        .exceptions = {NULL, NULL, NULL, NULL, NULL},
        .exclusions = NULL,
        .next = NULL,
    };

    return the_timeperiod;
}

void free_timeperiods(struct timeperiod the_timeperiod)
{

    free(the_timeperiod.name);
    free(the_timeperiod.alias);
    free(the_timeperiod.days[0]);
}

void populate_all_objects()
{
    populate_commands();

    test_tp = populate_timeperiods();

    test_host = populate_hosts(&test_tp);
    test_service = populate_service(&test_tp, &test_host);
    test_contact = populate_contact(&test_tp);
}

void free_all_objects()
{
    free_timeperiods(test_tp);
    free_contact(test_contact);
    free_service(test_service);
    free_host(test_host);
}