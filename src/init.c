


int num_bindings[NUM_QUERIES] = { 0 };
num_bindings[GENERIC] = 50 * MAX_OBJECT_INSERT;
num_bindings[GET_OBJECT_ID_NAME1] = 2;
num_bindings[GET_OBJECT_ID_NAME2] = 3;
num_bindings[INSERT_OBJECT_ID_NAME1] = 2;
num_bindings[INSERT_OBJECT_ID_NAME2] = 3;
num_bindings[HANDLE_LOG_DATA] = 5;
num_bindings[HANDLE_PROCESS] = 6;
num_bindings[HANDLE_PROCESS_SHUTDOWN] = 1;
num_bindings[HANDLE_TIMEDEVENT_ADD] = 6;
num_bindings[HANDLE_TIMEDEVENT_REMOVE] = 4;
num_bindings[HANDLE_TIMEDEVENT_EXECUTE] = 1;
num_bindings[HANDLE_SYSTEM_COMMAND] = 11;
num_bindings[HANDLE_EVENT_HANDLER] = 17;
num_bindings[HANDLE_HOST_CHECK] = 21;
num_bindings[HANDLE_SERVICE_CHECK] = 21;
num_bindings[HANDLE_COMMENT_ADD] = 13;
num_bindings[HANDLE_COMMENT_HISTORY_ADD] = 13;
num_bindings[HANDLE_COMMENT_DELETE] = 2;
num_bindings[HANDLE_COMMENT_HISTORY_DELETE] = 4;
num_bindings[HANDLE_DOWNTIME_ADD] = 11;
num_bindings[HANDLE_DOWNTIME_HISTORY_ADD] = 11;
num_bindings[HANDLE_DOWNTIME_START] = 7;
num_bindings[HANDLE_DOWNTIME_HISTORY_START] = 7;
num_bindings[HANDLE_DOWNTIME_STOP] = 5;
num_bindings[HANDLE_DOWNTIME_HISTORY_STOP] = 8;
num_bindings[HANDLE_FLAPPING] = 11;
num_bindings[HANDLE_PROGRAM_STATUS] = 19;
num_bindings[HANDLE_HOST_STATUS] = 44;
num_bindings[HANDLE_SERVICE_STATUS] = 45;
num_bindings[HANDLE_CONTACT_STATUS] = 9;
num_bindings[HANDLE_NOTIFICATION] = 12;
num_bindings[HANDLE_CONTACT_NOTIFICATION] = 6;
num_bindings[HANDLE_CONTACT_NOTIFICATION_METHOD] = 7;
num_bindings[HANDLE_EXTERNAL_COMMAND] = 4;
num_bindings[HANDLE_ACKNOWLEDGEMENT] = 10;
num_bindings[HANDLE_STATE_CHANGE] = 11;

int num_result_bindings[NUM_QUERIES] = { 0 };
num_result_bindings[GET_OBJECT_ID_NAME1] = 1;
num_result_bindings[GET_OBJECT_ID_NAME2] = 1;


#define GENERIC 0
#define GET_OBJECT_ID_NAME1 1
#define GET_OBJECT_ID_NAME2 2
#define INSERT_OBJECT_ID_NAME1 3
#define INSERT_OBJECT_ID_NAME2 4
#define HANDLE_LOG_DATA 5
#define HANDLE_PROCESS 6
#define HANDLE_PROCESS_SHUTDOWN 7
#define HANDLE_TIMEDEVENT_ADD 8
#define HANDLE_TIMEDEVENT_REMOVE 9
#define HANDLE_TIMEDEVENT_EXECUTE 10
#define HANDLE_SYSTEM_COMMAND 11
#define HANDLE_EVENT_HANDLER 12
#define HANDLE_HOST_CHECK 13
#define HANDLE_SERVICE_CHECK 14
#define HANDLE_COMMENT_ADD 15
#define HANDLE_COMMENT_HISTORY_ADD 16
#define HANDLE_COMMENT_DELETE 17
#define HANDLE_COMMENT_HISTORY_DELETE 18
#define HANDLE_DOWNTIME_ADD 19
#define HANDLE_DOWNTIME_HISTORY_ADD 20
#define HANDLE_DOWNTIME_START 21
#define HANDLE_DOWNTIME_HISTORY_START 22
#define HANDLE_DOWNTIME_STOP 23
#define HANDLE_DOWNTIME_HISTORY_STOP 24
#define HANDLE_FLAPPING 25
#define HANDLE_PROGRAM_STATUS 26
#define HANDLE_HOST_STATUS 27
#define HANDLE_SERVICE_STATUS 28
#define HANDLE_CONTACT_STATUS 29
#define HANDLE_NOTIFICATION 30
#define HANDLE_CONTACT_NOTIFICATION 31
#define HANDLE_CONTACT_NOTIFICATION_METHOD 32
#define HANDLE_EXTERNAL_COMMAND 33
#define HANDLE_ACKNOWLEDGEMENT 34
#define HANDLE_STATE_CHANGE 35

#define WRITE_ACTIVE_OBJECTS 0
#define WRITE_CUSTOMVARS 1
#define WRITE_CONTACT_ADDRESSES 2
#define WRITE_CONTACT_NOTIFICATIONCOMMANDS 3
#define WRITE_HOST_PARENTHOSTS 4
#define WRITE_HOST_CONTACTGROUPS 5
#define WRITE_HOST_CONTACTS 6
#define WRITE_SERVICE_PARENTSERVICES 7
#define WRITE_SERVICE_CONTACTGROUPS 8
#define WRITE_SERVICE_CONTACTS 9

#define NUM_WRITE_QUERIES 10

MYSQL_STMT ndo_write_stmt[NUM_WRITE_QUERIES];
MYSQL_BIND ndo_write_bind[NUM_WRITE_QUERIES][MAX_OBJECT_INSERT];
int ndo_write_i[NUM_WRITE_QUERIES] = { 0 };
int ndo_write_tmp_len[NUM_WRITE_QUERIES][MAX_OBJECT_INSERT];


typedef struct ndo_query_data {
    //char query[MAX_SQL_BUFFER];
    char * query;
    MYSQL_STMT * stmt;
    MYSQL_BIND * bind;
    MYSQL_BIND * result;
    long * strlen;
    long * result_strlen;
    int bind_i;
    int result_i;
} ndo_query_data;

int deinitialize


int initialize_stmt_data()
{
    int i = 0;
    int errors = 0;
    ndo_sql = calloc(NUM_QUERIES, sizeof(* ndo_sql));

    for (i = 0; i < NUM_QUERIES; i++) {

        if (ndo_sql[i] != NULL) {
            mysql_stmt_close(ndo_sql[i].stmt);
        }

        ndo_sql[i].stmt = mysql_stmt_init(mysql_connection);

        if (num_bindings[i] > 0) {
            ndo_sql[i].bind = calloc(num_bindings[i], sizeof(MYSQL_BIND));
            ndo_sql[i].strlen = calloc(num_bindings[i], sizeof(long));
        }

        if (num_result_bindings[i] > 0) {
            ndo_sql[i].result = calloc(num_result_bindings[i], sizeof(MYSQL_BIND));
            ndo_sql[i].result_strlen = calloc(num_result_bindings[i], sizeof(long));
        }

        ndo_sql[i].bind_i = 0;
        ndo_sql[i].result_i = 0;
    }

    ndo_sql[GENERIC].query = calloc(MAX_SQL_BUFFER, sizeof(char));

    ndo_sql[GET_OBJECT_ID_NAME1].query = strdup("SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 IS NULL");
    ndo_sql[GET_OBJECT_ID_NAME2].query = strdup("SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 = ?");
    ndo_sql[INSERT_OBJECT_ID_NAME1].query = strdup("INSERT INTO nagios_objects (objecttype_id, name1) VALUES (?,?) ON DUPLICATE KEY UPDATE is_active = 1");
    ndo_sql[INSERT_OBJECT_ID_NAME2].query = strdup("INSERT INTO nagios_objects (objecttype_id, name1, name2) VALUES (?,?,?) ON DUPLICATE KEY UPDATE is_active = 1");
    ndo_sql[HANDLE_LOG_DATA].query = strdup("INSERT INTO nagios_logentries (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (1,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,1,1)");
    ndo_sql[HANDLE_PROCESS].query = strdup("INSERT INTO nagios_processevents (instance_id, event_type, event_time, event_time_usec, process_id, program_name, program_version, program_date) VALUES (1,?,FROM_UNIXTIME(?),?,?,'Nagios',?,?)");
    ndo_sql[HANDLE_PROCESS_SHUTDOWN].query = strdup("UPDATE nagios_programstatus SET program_end_time = FROM_UNIXTIME(?), is_currently_running = 0");
    ndo_sql[HANDLE_TIMEDEVENT_ADD].query = strdup("INSERT INTO nagios_timedeventqueue (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), event_type = VALUES(event_type), queued_time = VALUES(queued_time), queued_time_usec = VALUES(queued_time_usec), scheduled_time = VALUES(scheduled_time), recurring_event = VALUES(recurring_event), object_id = VALUES(object_id)");
    ndo_sql[HANDLE_TIMEDEVENT_REMOVE].query = strdup("DELETE FROM nagios_timedeventqueue WHERE instance_id = 1 AND event_type = ? AND scheduled_time = FROM_UNIXTIME(?) AND recurring_event = ? AND object_id = ?");
    ndo_sql[HANDLE_TIMEDEVENT_EXECUTE].query = strdup("DELETE FROM nagios_timedeventqueue WHERE instance_id = 1 AND scheduled_time < FROM_UNIXTIME(?)");
    ndo_sql[HANDLE_SYSTEM_COMMAND].query = strdup("INSERT INTO nagios_systemcommands (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), command_line = VALUES(command_line), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output)");
    ndo_sql[HANDLE_EVENT_HANDLER].query = strdup("INSERT INTO nagios_eventhandlers (instance_id, start_time, start_time_usec, end_time, end_time_usec, eventhandler_type, object_id, state, state_type, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), eventhandler_type = VALUES(eventhandler_type), object_id = VALUES(object_id), state = VALUES(state), state_type = VALUES(state_type), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output)");
    ndo_sql[HANDLE_HOST_CHECK].query = strdup("INSERT INTO nagios_hostchecks (instance_id, start_time, start_time_usec, end_time, end_time_usec, host_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), host_object_id = VALUES(host_object_id), check_type = VALUES(check_type), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), state = VALUES(state), state_type = VALUES(state_type), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), latency = VALUES(latency), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line)");
    ndo_sql[HANDLE_SERVICE_CHECK].query = strdup("INSERT INTO nagios_servicechecks (instance_id, start_time, start_time_usec, end_time, end_time_usec, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), service_object_id = VALUES(service_object_id), check_type = VALUES(check_type), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), state = VALUES(state), state_type = VALUES(state_type), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), latency = VALUES(latency), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line)");
    ndo_sql[HANDLE_COMMENT_ADD].query = strdup("INSERT INTO nagios_comments (instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time, entry_time, entry_time_usec) VALUES (1,?,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), comment_type = VALUES(comment_type), entry_type = VALUES(entry_type), object_id = VALUES(object_id), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_persistent = VALUES(is_persistent), comment_source = VALUES(comment_source), expires = VALUES(expires), expiration_time = VALUES(expiration_time), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec)");
    ndo_sql[HANDLE_COMMENT_HISTORY_ADD].query = strdup("INSERT INTO nagios_commenthistory (instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time, entry_time, entry_time_usec) VALUES (1,?,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), comment_type = VALUES(comment_type), entry_type = VALUES(entry_type), object_id = VALUES(object_id), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_persistent = VALUES(is_persistent), comment_source = VALUES(comment_source), expires = VALUES(expires), expiration_time = VALUES(expiration_time), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec)");
    ndo_sql[HANDLE_COMMENT_DELETE].query = strdup("DELETE FROM nagios_comments WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");
    ndo_sql[HANDLE_COMMENT_HISTORY_DELETE].query = strdup("UPDATE nagios_commenthistory SET deletion_time = FROM_UNIXTIME(?), deletion_time_usec = ? WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");
    ndo_sql[HANDLE_DOWNTIME_ADD].query = strdup("INSERT INTO nagios_scheduleddowntime (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (1,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?)) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), downtime_type = VALUES(downtime_type), object_id = VALUES(object_id), entry_time = VALUES(entry_time), author_name = VALUES(author_name), comment_data = VALUES(comment_data), internal_downtime_id = VALUES(internal_downtime_id), triggered_by_id = VALUES(triggered_by_id), is_fixed = VALUES(is_fixed), duration = VALUES(duration), scheduled_start_time = VALUES(scheduled_start_time), scheduled_end_time = VALUES(scheduled_end_time)");
    ndo_sql[HANDLE_DOWNTIME_HISTORY_ADD].query = strdup("INSERT INTO nagios_downtimehistory (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (1,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?)) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), downtime_type = VALUES(downtime_type), object_id = VALUES(object_id), entry_time = VALUES(entry_time), author_name = VALUES(author_name), comment_data = VALUES(comment_data), internal_downtime_id = VALUES(internal_downtime_id), triggered_by_id = VALUES(triggered_by_id), is_fixed = VALUES(is_fixed), duration = VALUES(duration), scheduled_start_time = VALUES(scheduled_start_time), scheduled_end_time = VALUES(scheduled_end_time)");
    ndo_sql[HANDLE_DOWNTIME_START].query = strdup("UPDATE nagios_scheduleddowntime SET actual_start_time = FROM_UNIXTIME(?), actual_start_time_usec = ?, was_started = 1 WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    ndo_sql[HANDLE_DOWNTIME_HISTORY_START].query = strdup("UPDATE nagios_downtimehistory SET actual_start_time = FROM_UNIXTIME(?), actual_start_time_usec = ?, was_started = 1 WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    ndo_sql[HANDLE_DOWNTIME_STOP].query = strdup("DELETE FROM nagios_scheduleddowntime WHERE downtime_type = ? AND object_id = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    ndo_sql[HANDLE_DOWNTIME_HISTORY_STOP].query = strdup("UPDATE nagios_downtimehistory SET actual_end_time = FROM_UNIXTIME(?), actual_end_time_usec = ?, was_cancelled = ? WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    ndo_sql[HANDLE_FLAPPING].query = strdup("INSERT INTO nagios_flappinghistory (instance_id, event_time, event_time_usec, event_type, reason_type, flapping_type, object_id, percent_state_change, low_threshold, high_threshold, comment_time, internal_comment_id) VALUES (1,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), event_time = VALUES(event_time), event_time_usec = VALUES(event_time_usec), event_type = VALUES(event_type), reason_type = VALUES(reason_type), flapping_type = VALUES(flapping_type), object_id = VALUES(object_id), percent_state_change = VALUES(percent_state_change), low_threshold = VALUES(low_threshold), high_threshold = VALUES(high_threshold), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id)");
    ndo_sql[HANDLE_PROGRAM_STATUS].query = strdup("INSERT INTO nagios_programstatus (instance_id, status_update_time, program_start_time, is_currently_running, process_id, daemon_mode, last_command_check, last_log_rotation, notifications_enabled, active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, failure_prediction_enabled, process_performance_data, obsess_over_hosts, obsess_over_services, modified_host_attributes, modified_service_attributes, global_host_event_handler, global_service_event_handler) VALUES (1,FROM_UNIXTIME(?),FROM_UNIXTIME(?),1,?,?,FROM_UNIXTIME(0),FROM_UNIXTIME(?),?,?,?,?,?,?,?,0,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), status_update_time = VALUES(status_update_time), program_start_time = VALUES(program_start_time), is_currently_running = VALUES(is_currently_running), process_id = VALUES(process_id), daemon_mode = VALUES(daemon_mode), last_command_check = VALUES(last_command_check), last_log_rotation = VALUES(last_log_rotation), notifications_enabled = VALUES(notifications_enabled), active_service_checks_enabled = VALUES(active_service_checks_enabled), passive_service_checks_enabled = VALUES(passive_service_checks_enabled), active_host_checks_enabled = VALUES(active_host_checks_enabled), passive_host_checks_enabled = VALUES(passive_host_checks_enabled), event_handlers_enabled = VALUES(event_handlers_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_hosts = VALUES(obsess_over_hosts), obsess_over_services = VALUES(obsess_over_services), modified_host_attributes = VALUES(modified_host_attributes), modified_service_attributes = VALUES(modified_service_attributes), global_host_event_handler = VALUES(global_host_event_handler), global_service_event_handler = VALUES(global_service_event_handler)");
    ndo_sql[HANDLE_HOST_STATUS].query = strdup("INSERT INTO nagios_hoststatus (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_object_id = VALUES(host_object_id), status_update_time = VALUES(status_update_time), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), current_state = VALUES(current_state), has_been_checked = VALUES(has_been_checked), should_be_scheduled = VALUES(should_be_scheduled), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), last_check = VALUES(last_check), next_check = VALUES(next_check), check_type = VALUES(check_type), last_state_change = VALUES(last_state_change), last_hard_state_change = VALUES(last_hard_state_change), last_hard_state = VALUES(last_hard_state), last_time_up = VALUES(last_time_up), last_time_down = VALUES(last_time_down), last_time_unreachable = VALUES(last_time_unreachable), state_type = VALUES(state_type), last_notification = VALUES(last_notification), next_notification = VALUES(next_notification), no_more_notifications = VALUES(no_more_notifications), notifications_enabled = VALUES(notifications_enabled), problem_has_been_acknowledged = VALUES(problem_has_been_acknowledged), acknowledgement_type = VALUES(acknowledgement_type), current_notification_number = VALUES(current_notification_number), passive_checks_enabled = VALUES(passive_checks_enabled), active_checks_enabled = VALUES(active_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), is_flapping = VALUES(is_flapping), percent_state_change = VALUES(percent_state_change), latency = VALUES(latency), execution_time = VALUES(execution_time), scheduled_downtime_depth = VALUES(scheduled_downtime_depth), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_host = VALUES(obsess_over_host), modified_host_attributes = VALUES(modified_host_attributes), event_handler = VALUES(event_handler), check_command = VALUES(check_command), normal_check_interval = VALUES(normal_check_interval), retry_check_interval = VALUES(retry_check_interval), check_timeperiod_object_id = VALUES(check_timeperiod_object_id)");
    ndo_sql[HANDLE_SERVICE_STATUS].query = strdup("INSERT INTO nagios_servicestatus (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_object_id = VALUES(service_object_id), status_update_time = VALUES(status_update_time), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), current_state = VALUES(current_state), has_been_checked = VALUES(has_been_checked), should_be_scheduled = VALUES(should_be_scheduled), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), last_check = VALUES(last_check), next_check = VALUES(next_check), check_type = VALUES(check_type), last_state_change = VALUES(last_state_change), last_hard_state_change = VALUES(last_hard_state_change), last_hard_state = VALUES(last_hard_state), last_time_ok = VALUES(last_time_ok), last_time_warning = VALUES(last_time_warning), last_time_unknown = VALUES(last_time_unknown), last_time_critical = VALUES(last_time_critical), state_type = VALUES(state_type), last_notification = VALUES(last_notification), next_notification = VALUES(next_notification), no_more_notifications = VALUES(no_more_notifications), notifications_enabled = VALUES(notifications_enabled), problem_has_been_acknowledged = VALUES(problem_has_been_acknowledged), acknowledgement_type = VALUES(acknowledgement_type), current_notification_number = VALUES(current_notification_number), passive_checks_enabled = VALUES(passive_checks_enabled), active_checks_enabled = VALUES(active_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), is_flapping = VALUES(is_flapping), percent_state_change = VALUES(percent_state_change), latency = VALUES(latency), execution_time = VALUES(execution_time), scheduled_downtime_depth = VALUES(scheduled_downtime_depth), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_service = VALUES(obsess_over_service), modified_service_attributes = VALUES(modified_service_attributes), event_handler = VALUES(event_handler), check_command = VALUES(check_command), normal_check_interval = VALUES(normal_check_interval), retry_check_interval = VALUES(retry_check_interval), check_timeperiod_object_id = VALUES(check_timeperiod_object_id)");
    ndo_sql[HANDLE_CONTACT_STATUS].query = strdup("INSERT INTO nagios_contactstatus (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (1,?,FROM_UNIXTIME(?),?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contact_object_id = VALUES(contact_object_id), status_update_time = VALUES(status_update_time), host_notifications_enabled = VALUES(host_notifications_enabled), service_notifications_enabled = VALUES(service_notifications_enabled), last_host_notification = VALUES(last_host_notification), last_service_notification = VALUES(last_service_notification), modified_attributes = VALUES(modified_attributes), modified_host_attributes = VALUES(modified_host_attributes), modified_service_attributes = VALUES(modified_service_attributes)");
    ndo_sql[HANDLE_NOTIFICATION].query = strdup("INSERT INTO nagios_notifications (instance_id, start_time, start_time_usec, end_time, end_time_usec, notification_type, notification_reason, object_id, state, output, long_output, escalated, contacts_notified) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), notification_type = VALUES(notification_type), notification_reason = VALUES(notification_reason), object_id = VALUES(object_id), state = VALUES(state), output = VALUES(output), long_output = VALUES(long_output), escalated = VALUES(escalated), contacts_notified = VALUES(contacts_notified)");
    ndo_sql[HANDLE_CONTACT_NOTIFICATION].query = strdup("INSERT INTO nagios_contactnotifications (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), notification_id = VALUES(notification_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), contact_object_id = VALUES(contact_object_id)");
    ndo_sql[HANDLE_CONTACT_NOTIFICATION_METHOD].query = strdup("INSERT INTO nagios_contactnotificationmethods (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contactnotification_id = VALUES(contactnotification_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args)");
    ndo_sql[HANDLE_EXTERNAL_COMMAND].query = strdup("INSERT INTO nagios_externalcommands (instance_id, command_type, entry_time, command_name, command_args) VALUES (1,?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), command_type = VALUES(command_type), entry_time = VALUES(entry_time), command_name = VALUES(command_name), command_args = VALUES(command_args)");
    ndo_sql[HANDLE_ACKNOWLEDGEMENT].query = strdup("INSERT INTO nagios_acknowledgements (instance_id, entry_time, entry_time_usec, acknowledgement_type, object_id, state, author_name, comment_data, is_sticky, persistent_comment, notify_contacts) VALUES (1,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec), acknowledgement_type = VALUES(acknowledgement_type), object_id = VALUES(object_id), state = VALUES(state), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_sticky = VALUES(is_sticky), persistent_comment = VALUES(persistent_comment), notify_contacts = VALUES(notify_contacts)");
    ndo_sql[HANDLE_STATE_CHANGE].query = strdup("INSERT INTO nagios_statehistory SET instance_id = 1, state_time = FROM_UNIXTIME(?), state_time_usec = ?, object_id = ?, state_change = 1, state = ?, state_type = ?, current_check_attempt = ?, max_check_attempts = ?, last_state = ?, last_hard_state = ?, output = ?, long_output = ?");

    /* now check to make sure all those strdups worked */
    for (i = 0; i < NUM_QUERIES; i++) {
        if (ndo_sql[i].query == NULL) {
            char msg[256] = { 0 };
            snprintf(msg, 255, "Unable to allocate memory for query (%d)", i);
            ndo_log(msg);
            errors++;
        }
    }

    if (errors > 0) {
        return NDO_ERROR;
    }

    /* now prepare each statement - we start at one since GENERIC doesn't
       have a query at this point */
    for (i = 1; i < NUM_QUERIES; i++) {
        if (mysql_prepare_statement(ndo_sql[i].stmt, ndo_sql[i].query, strlen(ndo_sql[i].query))) {
            char msg[256] = { 0 };
            snprintf(msg, 255, "Unable to prepare statement for query (%d)", i);
            ndo_log(msg);
            errors++;
        }
    }

    if (errors > 0) {
        return NDO_ERROR;
    }
}

