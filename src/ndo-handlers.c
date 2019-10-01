
// todo: all of the mysql bindings need to be gone through and compared against the schema to ensure doubles aren't being cast as ints (won't error, but the data won't be right either)

int ndo_handle_process(int type, void * d)
{
    trace_handler(process);

    nebstruct_process_data * data = d;
    char * program_version = get_program_version();
    char * program_mod_date = get_program_modification_date();
    int program_pid = (int) getpid();

    switch (data->type) {

    case NEBTYPE_PROCESS_PRELAUNCH:

        if (ndo_startup_skip_writing_objects != TRUE) {
            ndo_table_genocide();
            ndo_set_all_objects_inactive();
        }

        break;


    case NEBTYPE_PROCESS_START:

        if (ndo_startup_skip_writing_objects != TRUE) {

            ndo_begin_active_objects(active_objects_run);
            ndo_write_object_config(NDO_CONFIG_DUMP_ORIGINAL);
            ndo_end_active_objects();

            ndo_write_config_files();
            ndo_write_config(NDO_CONFIG_DUMP_ORIGINAL);
        }

        break;


    case NEBTYPE_PROCESS_EVENTLOOPSTART:

        if (ndo_startup_skip_writing_objects != TRUE) {
            ndo_write_runtime_variables();
        }

        break;


    case NEBTYPE_PROCESS_SHUTDOWN:
    case NEBTYPE_PROCESS_RESTART:

        MYSQL_RESET_BIND(HANDLE_PROCESS_SHUTDOWN);

        MYSQL_BIND_INT(HANDLE_PROCESS_SHUTDOWN, data->timestamp.tv_sec);

        MYSQL_BIND(HANDLE_PROCESS_SHUTDOWN);
        MYSQL_EXECUTE(HANDLE_PROCESS_SHUTDOWN);

        break;

    }

    MYSQL_RESET_BIND(HANDLE_PROCESS);

    MYSQL_BIND_INT(HANDLE_PROCESS, data->type);
    MYSQL_BIND_INT(HANDLE_PROCESS, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_PROCESS, data->timestamp.tv_usec);
    MYSQL_BIND_INT(HANDLE_PROCESS, program_pid);
    MYSQL_BIND_STR(HANDLE_PROCESS, program_version);
    MYSQL_BIND_STR(HANDLE_PROCESS, program_mod_date);

    MYSQL_BIND(HANDLE_PROCESS);
    MYSQL_EXECUTE(HANDLE_PROCESS);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_timed_event(int type, void * d)
{
    trace_handler(timed_event);

    nebstruct_timed_event_data * data = d;

    int object_id = 0;

    if (data->type == NEBTYPE_TIMEDEVENT_SLEEP) {
        return NDO_OK;
    }

    if (data->event_type == EVENT_SERVICE_CHECK) {

        service * svc = data->event_data;
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, svc->host_name, svc->description);
    }
    else if (data->event_type == EVENT_HOST_CHECK) {

        host * hst = data->event_data;
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, hst->name);
    }
    else if (data->event_type == EVENT_SCHEDULED_DOWNTIME) {

        scheduled_downtime * dwn = find_downtime(ANY_DOWNTIME, (unsigned long) data->event_data);

        char * host_name = dwn->host_name;
        char * service_description = dwn->service_description;

        if (service_description != NULL && strlen(service_description) > 0) {
            object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, host_name, service_description);
        }
        else {
            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, host_name);
        }
    }

    if (data->type == NEBTYPE_TIMEDEVENT_ADD) {

        MYSQL_RESET_BIND(HANDLE_TIMEDEVENT_ADD);

        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, data->event_type);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, data->timestamp.tv_usec);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, data->run_time);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, data->recurring);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_ADD, object_id);

        MYSQL_BIND(HANDLE_TIMEDEVENT_ADD);
        MYSQL_EXECUTE(HANDLE_TIMEDEVENT_ADD);
    }

    else if (data->type == NEBTYPE_TIMEDEVENT_REMOVE || data->type == NEBTYPE_TIMEDEVENT_EXECUTE) {

        MYSQL_RESET_BIND(HANDLE_TIMEDEVENT_REMOVE);

        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_REMOVE, data->event_type);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_REMOVE, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_REMOVE, data->recurring);
        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_REMOVE, object_id);

        MYSQL_BIND(HANDLE_TIMEDEVENT_REMOVE);
        MYSQL_EXECUTE(HANDLE_TIMEDEVENT_REMOVE);
    }

    if (    data->type == NEBTYPE_TIMEDEVENT_EXECUTE
        && (data->event_type == EVENT_SERVICE_CHECK || data->event_type == EVENT_HOST_CHECK)) {

        MYSQL_RESET_BIND(HANDLE_TIMEDEVENT_EXECUTE);

        MYSQL_BIND_INT(HANDLE_TIMEDEVENT_EXECUTE, data->run_time);

        MYSQL_BIND(HANDLE_TIMEDEVENT_EXECUTE);
        MYSQL_EXECUTE(HANDLE_TIMEDEVENT_EXECUTE);
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_log(int type, void * d)
{
    /* trace_handler_nolog(log); */

    nebstruct_log_data * data = d;

    /* this particular function is a bit weird because it starts passing
       logs to the neb module before the database initialization has occured,
       so if the db hasn't been initialized, we just return */
    if (mysql_connection == NULL || ndo_database_connected != TRUE) {
        return NDO_OK;
    }

    if (data->data == NULL || strlen(data->data) == 0) {
        return NDO_OK;
    }

    MYSQL_RESET_BIND(HANDLE_LOG_DATA);

    MYSQL_BIND_INT(HANDLE_LOG_DATA, data->entry_time);
    MYSQL_BIND_INT(HANDLE_LOG_DATA, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_LOG_DATA, data->timestamp.tv_usec);
    MYSQL_BIND_INT(HANDLE_LOG_DATA, data->data_type);
    MYSQL_BIND_STR(HANDLE_LOG_DATA, data->data);

    MYSQL_BIND(HANDLE_TIMEDEVENT_EXECUTE);
    MYSQL_EXECUTE(HANDLE_TIMEDEVENT_EXECUTE);

    /* trace_func_end_nolog(); */
    return NDO_OK;
}


int ndo_handle_system_command(int type, void * d)
{
    trace_handler(system_command);

    nebstruct_system_command_data * data = d;

    MYSQL_RESET_BIND(HANDLE_SYSTEM_COMMAND);

    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->end_time.tv_usec);
    MYSQL_BIND_STR(HANDLE_SYSTEM_COMMAND, data->command_line);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->timeout);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->early_timeout);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->execution_time);
    MYSQL_BIND_INT(HANDLE_SYSTEM_COMMAND, data->return_code);
    MYSQL_BIND_STR(HANDLE_SYSTEM_COMMAND, data->output);
    MYSQL_BIND_STR(HANDLE_SYSTEM_COMMAND, data->output);

    MYSQL_BIND(HANDLE_SYSTEM_COMMAND);
    MYSQL_EXECUTE(HANDLE_SYSTEM_COMMAND);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_event_handler(int type, void * d)
{
    trace_handler(event_handler);

    nebstruct_event_handler_data * data = d;
    int object_id = 0;

    int command_object_id = 0;

    if (data->eventhandler_type == SERVICE_EVENTHANDLER || data->eventhandler_type == GLOBAL_SERVICE_EVENTHANDLER) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    command_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, data->command_name);

    MYSQL_RESET_BIND(HANDLE_EVENT_HANDLER);

    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->eventhandler_type);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, object_id);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->state);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->state_type);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, command_object_id);
    MYSQL_BIND_STR(HANDLE_EVENT_HANDLER, data->command_args);
    MYSQL_BIND_STR(HANDLE_EVENT_HANDLER, data->command_line);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->timeout);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->early_timeout);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->execution_time);
    MYSQL_BIND_INT(HANDLE_EVENT_HANDLER, data->return_code);
    MYSQL_BIND_STR(HANDLE_EVENT_HANDLER, data->output);
    MYSQL_BIND_STR(HANDLE_EVENT_HANDLER, data->output);

    MYSQL_BIND(HANDLE_EVENT_HANDLER);
    MYSQL_EXECUTE(HANDLE_EVENT_HANDLER);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_host_check(int type, void * d)
{
    trace_handler(host_check);

    nebstruct_host_check_data * data = d;
    int object_id = 0;

    int command_object_id = 0;

    /* this is the only data we care about / need */
    if (type != NEBTYPE_HOSTCHECK_PROCESSED) {
        return NDO_OK;
    }

    object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);

    command_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, data->command_name);

    MYSQL_RESET_BIND(HANDLE_HOST_CHECK);

    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, object_id);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->check_type);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->current_attempt);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->max_attempts);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->state);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->state_type);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->timeout);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->early_timeout);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_CHECK, data->execution_time);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_CHECK, data->latency);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, data->return_code);
    MYSQL_BIND_STR(HANDLE_HOST_CHECK, data->output);
    MYSQL_BIND_STR(HANDLE_HOST_CHECK, data->long_output);
    MYSQL_BIND_STR(HANDLE_HOST_CHECK, data->perf_data);
    MYSQL_BIND_INT(HANDLE_HOST_CHECK, command_object_id);
    MYSQL_BIND_STR(HANDLE_HOST_CHECK, data->command_args);
    MYSQL_BIND_STR(HANDLE_HOST_CHECK, data->command_line);

    MYSQL_BIND(HANDLE_HOST_CHECK);
    MYSQL_EXECUTE(HANDLE_HOST_CHECK);

    trace_func_end();
    return NDO_OK;
}

int ndo_handle_service_check(int type, void * d)
{
    trace_handler(service_check);

    nebstruct_service_check_data * data = d;
    int object_id = 0;

    int command_object_id = 0;

    /* this is the only data we care about / need */
    if (type != NEBTYPE_SERVICECHECK_PROCESSED) {
        return NDO_OK;
    }

    object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);

    if (data->command_name != NULL) {
        /* Nagios Core appears to always pass NULL for its command arguments when brokering NEBTYPE_SERVICECHECK_PROCESSED.
         * It's not clear why this was done, so we're working around it here for now.
         */
        command_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, data->command_name);
    }

    MYSQL_RESET_BIND(HANDLE_SERVICE_CHECK);

    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, object_id);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->check_type);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->current_attempt);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->max_attempts);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->state);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->state_type);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->timeout);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->early_timeout);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_CHECK, data->execution_time);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_CHECK, data->latency);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, data->return_code);
    MYSQL_BIND_STR(HANDLE_SERVICE_CHECK, data->output);
    MYSQL_BIND_STR(HANDLE_SERVICE_CHECK, data->long_output);
    MYSQL_BIND_STR(HANDLE_SERVICE_CHECK, data->perf_data);
    MYSQL_BIND_INT(HANDLE_SERVICE_CHECK, command_object_id);
    MYSQL_BIND_STR(HANDLE_SERVICE_CHECK, data->command_args);
    MYSQL_BIND_STR(HANDLE_SERVICE_CHECK, data->command_line);

    MYSQL_BIND(HANDLE_SERVICE_CHECK);
    MYSQL_EXECUTE(HANDLE_SERVICE_CHECK);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_comment(int type, void * d)
{
    trace_handler(comment);

    nebstruct_comment_data * data = d;
    int object_id = 0;

    if (data->comment_type == SERVICE_COMMENT) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    if (data->type == NEBTYPE_COMMENT_ADD || data->type == NEBTYPE_COMMENT_LOAD) {

        MYSQL_RESET_BIND(HANDLE_COMMENT_ADD);

        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->comment_type);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->entry_type);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, object_id);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->entry_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->comment_id);
        MYSQL_BIND_STR(HANDLE_COMMENT_ADD, data->author_name);
        MYSQL_BIND_STR(HANDLE_COMMENT_ADD, data->comment_data);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->persistent);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->source);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->expires);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->expire_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_COMMENT_ADD, data->timestamp.tv_usec);

        MYSQL_BIND(HANDLE_COMMENT_ADD);
        MYSQL_EXECUTE(HANDLE_COMMENT_ADD);


        MYSQL_RESET_BIND(HANDLE_COMMENT_HISTORY_ADD);

        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->comment_type);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->entry_type);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, object_id);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->entry_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->comment_id);
        MYSQL_BIND_STR(HANDLE_COMMENT_HISTORY_ADD, data->author_name);
        MYSQL_BIND_STR(HANDLE_COMMENT_HISTORY_ADD, data->comment_data);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->persistent);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->source);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->expires);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->expire_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_ADD, data->timestamp.tv_usec);

        MYSQL_BIND(HANDLE_COMMENT_HISTORY_ADD);
        MYSQL_EXECUTE(HANDLE_COMMENT_HISTORY_ADD);
    }

    if (data->type == NEBTYPE_COMMENT_DELETE) {

        MYSQL_RESET_BIND(HANDLE_COMMENT_HISTORY_DELETE);

        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_DELETE, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_DELETE, data->timestamp.tv_usec);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_DELETE, data->entry_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_HISTORY_DELETE, data->comment_id);

        MYSQL_BIND(HANDLE_COMMENT_HISTORY_DELETE);
        MYSQL_EXECUTE(HANDLE_COMMENT_HISTORY_DELETE);


        MYSQL_RESET_BIND(HANDLE_COMMENT_DELETE);

        MYSQL_BIND_INT(HANDLE_COMMENT_DELETE, data->entry_time);
        MYSQL_BIND_INT(HANDLE_COMMENT_DELETE, data->comment_id);

        MYSQL_BIND(HANDLE_COMMENT_DELETE);
        MYSQL_EXECUTE(HANDLE_COMMENT_DELETE);
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_downtime(int type, void * d)
{
    trace_handler(downtime);

    nebstruct_downtime_data * data = d;
    int object_id = 0;

    if (data->downtime_type == SERVICE_DOWNTIME) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    if (data->type == NEBTYPE_DOWNTIME_ADD || data->type == NEBTYPE_DOWNTIME_LOAD) {

        MYSQL_RESET_BIND(HANDLE_DOWNTIME_ADD);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->entry_time);
        MYSQL_BIND_STR(HANDLE_DOWNTIME_ADD, data->author_name);
        MYSQL_BIND_STR(HANDLE_DOWNTIME_ADD, data->comment_data);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->downtime_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->triggered_by);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->fixed);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->duration);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_ADD, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_ADD);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_ADD);


        MYSQL_RESET_BIND(HANDLE_DOWNTIME_HISTORY_ADD);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->entry_time);
        MYSQL_BIND_STR(HANDLE_DOWNTIME_HISTORY_ADD, data->author_name);
        MYSQL_BIND_STR(HANDLE_DOWNTIME_HISTORY_ADD, data->comment_data);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->downtime_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->triggered_by);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->fixed);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->duration);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_ADD, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_HISTORY_ADD);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_HISTORY_ADD);
    }

    if (data->type == NEBTYPE_DOWNTIME_START) {

        MYSQL_RESET_BIND(HANDLE_DOWNTIME_START);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->timestamp.tv_usec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->entry_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_START, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_START);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_START);


        MYSQL_RESET_BIND(HANDLE_DOWNTIME_HISTORY_START);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->timestamp.tv_usec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->entry_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_START, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_HISTORY_START);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_HISTORY_START);
    }

    if (data->type == NEBTYPE_DOWNTIME_STOP) {

        int cancelled = 0;

        if (data->attr == NEBATTR_DOWNTIME_STOP_CANCELLED) {
            cancelled = 1;
        }

        MYSQL_RESET_BIND(HANDLE_DOWNTIME_HISTORY_STOP);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->timestamp.tv_sec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->timestamp.tv_usec);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, cancelled);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->entry_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_HISTORY_STOP, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_HISTORY_STOP);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_HISTORY_STOP);
    }

    if (data->type == NEBTYPE_DOWNTIME_STOP || data->type == NEBTYPE_DOWNTIME_DELETE) {

        MYSQL_RESET_BIND(HANDLE_DOWNTIME_STOP);

        MYSQL_BIND_INT(HANDLE_DOWNTIME_STOP, data->downtime_type);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_STOP, object_id);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_STOP, data->entry_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_STOP, data->start_time);
        MYSQL_BIND_INT(HANDLE_DOWNTIME_STOP, data->end_time);

        MYSQL_BIND(HANDLE_DOWNTIME_STOP);
        MYSQL_EXECUTE(HANDLE_DOWNTIME_STOP);
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_flapping(int type, void * d)
{
    trace_handler(flapping);

    nebstruct_flapping_data * data = d;
    int object_id = 0;
    nagios_comment * comment = NULL;
    time_t comment_time = 0;

    if (data->flapping_type == SERVICE_FLAPPING) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
        comment = find_service_comment(data->comment_id);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
        comment = find_host_comment(data->comment_id);
    }

    if (data->comment_id != 0L && comment != NULL) {
        comment_time = comment->entry_time;
    }

    if (comment) { free(comment); }

    MYSQL_RESET_BIND(HANDLE_FLAPPING);

    MYSQL_BIND_INT(HANDLE_FLAPPING, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_FLAPPING, data->timestamp.tv_usec);
    MYSQL_BIND_INT(HANDLE_FLAPPING, data->type);
    MYSQL_BIND_INT(HANDLE_FLAPPING, data->attr);
    MYSQL_BIND_INT(HANDLE_FLAPPING, data->flapping_type);
    MYSQL_BIND_INT(HANDLE_FLAPPING, object_id);
    MYSQL_BIND_DOUBLE(HANDLE_FLAPPING, data->percent_change);
    MYSQL_BIND_DOUBLE(HANDLE_FLAPPING, data->low_threshold);
    MYSQL_BIND_DOUBLE(HANDLE_FLAPPING, data->high_threshold);
    MYSQL_BIND_INT(HANDLE_FLAPPING, comment_time);
    MYSQL_BIND_INT(HANDLE_FLAPPING, data->comment_id);

    MYSQL_BIND(HANDLE_FLAPPING);
    MYSQL_EXECUTE(HANDLE_FLAPPING);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_program_status(int type, void * d)
{
    trace_handler(program_status);

    nebstruct_program_status_data * data = d;

    MYSQL_RESET_BIND(HANDLE_PROGRAM_STATUS);

    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->program_start);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->pid);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->daemon_mode);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->last_log_rotation);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->notifications_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->active_service_checks_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->passive_service_checks_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->active_host_checks_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->passive_host_checks_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->event_handlers_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->flap_detection_enabled);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->process_performance_data);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->obsess_over_hosts);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->obsess_over_services);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->modified_host_attributes);
    MYSQL_BIND_INT(HANDLE_PROGRAM_STATUS, data->modified_service_attributes);
    MYSQL_BIND_STR(HANDLE_PROGRAM_STATUS, data->global_host_event_handler);
    MYSQL_BIND_STR(HANDLE_PROGRAM_STATUS, data->global_service_event_handler);

    MYSQL_BIND(HANDLE_PROGRAM_STATUS);
    MYSQL_EXECUTE(HANDLE_PROGRAM_STATUS);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_host_status(int type, void * d)
{
    trace_handler(host_status);

    nebstruct_host_status_data * data = d;
    int host_object_id        = 0;
    int timeperiod_object_id  = 0;
    host * hst                = NULL;
    timeperiod * tm           = NULL;

    if (data->object_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    hst = data->object_ptr;
    tm = hst->check_period_ptr;

    host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, hst->name);
    timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tm->name);

    MYSQL_RESET_BIND(HANDLE_HOST_STATUS);
    
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, host_object_id);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, data->timestamp.tv_sec);
    MYSQL_BIND_STR(HANDLE_HOST_STATUS, hst->plugin_output);
    MYSQL_BIND_STR(HANDLE_HOST_STATUS, hst->long_plugin_output);
    MYSQL_BIND_STR(HANDLE_HOST_STATUS, hst->perf_data);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->current_state);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->has_been_checked);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->should_be_scheduled);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->current_attempt);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->max_attempts);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_check);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->next_check);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->check_type);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_state_change);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_hard_state_change);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_hard_state);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_time_up);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_time_down);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_time_unreachable);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->state_type);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->last_notification);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->next_notification);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->no_more_notifications);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->notifications_enabled);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->problem_has_been_acknowledged);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->acknowledgement_type);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->current_notification_number);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->accept_passive_checks);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->checks_enabled);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->event_handler_enabled);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->flap_detection_enabled);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->is_flapping);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_STATUS, hst->percent_state_change);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_STATUS, hst->latency);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_STATUS, hst->execution_time);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->scheduled_downtime_depth);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->process_performance_data);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->obsess);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, hst->modified_attributes);
    MYSQL_BIND_STR(HANDLE_HOST_STATUS, hst->event_handler);
    MYSQL_BIND_STR(HANDLE_HOST_STATUS, hst->check_command);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_STATUS, hst->check_interval);
    MYSQL_BIND_DOUBLE(HANDLE_HOST_STATUS, hst->retry_interval);
    MYSQL_BIND_INT(HANDLE_HOST_STATUS, timeperiod_object_id);
    
    MYSQL_BIND(HANDLE_HOST_STATUS);
    MYSQL_EXECUTE(HANDLE_HOST_STATUS);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_service_status(int type, void * d)
{
    trace_handler(service_status);

    nebstruct_service_status_data * data = d;

    int service_object_id     = 0;
    int timeperiod_object_id  = 0;
    service * svc             = NULL;
    timeperiod * tm           = NULL;

    if (data->object_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    svc = data->object_ptr;
    tm = svc->check_period_ptr;

    service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, svc->host_name, svc->description);
    timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tm->name);

    MYSQL_RESET_BIND(HANDLE_SERVICE_STATUS);

    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, service_object_id);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, data->timestamp.tv_sec);
    MYSQL_BIND_STR(HANDLE_SERVICE_STATUS, svc->plugin_output);
    MYSQL_BIND_STR(HANDLE_SERVICE_STATUS, svc->long_plugin_output);
    MYSQL_BIND_STR(HANDLE_SERVICE_STATUS, svc->perf_data);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->current_state);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->has_been_checked);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->should_be_scheduled);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->current_attempt);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->max_attempts);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_check);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->next_check);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->check_type);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_state_change);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_hard_state_change);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_hard_state);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_time_ok);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_time_warning);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_time_unknown);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_time_critical);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->state_type);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->last_notification);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->next_notification);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->no_more_notifications);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->notifications_enabled);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->problem_has_been_acknowledged);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->acknowledgement_type);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->current_notification_number);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->accept_passive_checks);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->checks_enabled);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->event_handler_enabled);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->flap_detection_enabled);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->is_flapping);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_STATUS, svc->percent_state_change);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_STATUS, svc->latency);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_STATUS, svc->execution_time);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->scheduled_downtime_depth);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->process_performance_data);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->obsess);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, svc->modified_attributes);
    MYSQL_BIND_STR(HANDLE_SERVICE_STATUS, svc->event_handler);
    MYSQL_BIND_STR(HANDLE_SERVICE_STATUS, svc->check_command);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_STATUS, svc->check_interval);
    MYSQL_BIND_DOUBLE(HANDLE_SERVICE_STATUS, svc->retry_interval);
    MYSQL_BIND_INT(HANDLE_SERVICE_STATUS, timeperiod_object_id);

    MYSQL_BIND(HANDLE_SERVICE_STATUS);
    MYSQL_EXECUTE(HANDLE_SERVICE_STATUS);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_contact_status(int type, void * d)
{
    trace_handler(contact_status);

    nebstruct_contact_status_data * data = d;

    int contact_object_id     = 0;
    contact * cnt             = NULL;

    if (data->object_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    cnt = data->object_ptr;

    contact_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->name);

    MYSQL_RESET_BIND(HANDLE_CONTACT_STATUS);

    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, contact_object_id);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->host_notifications_enabled);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->service_notifications_enabled);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->last_host_notification);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->last_service_notification);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->modified_attributes);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->modified_host_attributes);
    MYSQL_BIND_INT(HANDLE_CONTACT_STATUS, cnt->modified_service_attributes);

    MYSQL_BIND(HANDLE_CONTACT_STATUS);
    MYSQL_EXECUTE(HANDLE_CONTACT_STATUS);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_notification(int type, void * d)
{
    trace_handler(notification);

    nebstruct_notification_data * data = d;
    int object_id = 0;

    if (data->notification_type == SERVICE_NOTIFICATION) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    MYSQL_RESET_BIND(HANDLE_NOTIFICATION);

    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->notification_type);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->reason_type);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, object_id);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->state);
    MYSQL_BIND_STR(HANDLE_NOTIFICATION, data->output);
    MYSQL_BIND_STR(HANDLE_NOTIFICATION, data->output);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->escalated);
    MYSQL_BIND_INT(HANDLE_NOTIFICATION, data->contacts_notified);

    MYSQL_BIND(HANDLE_NOTIFICATION);
    MYSQL_EXECUTE(HANDLE_NOTIFICATION);

    /* because of the way we've changed ndo, we can no longer keep passing this
       around inside the database object.

       we can be sure that this will be the appropriate id, as the brokered
       event data for notification happens directly before each contact is
       then notified (and that data also brokered) */
    ndo_last_notification_id = mysql_insert_id(mysql_connection);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_contact_notification(int type, void * d)
{
    trace_handler(contact_notification);

    nebstruct_contact_notification_data * data = d;

    int contact_object_id     = 0;
    contact * cnt             = NULL;

    if (data->contact_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer is null");
        return NDO_OK;
    }

    cnt = data->contact_ptr;

    contact_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->name);

    MYSQL_RESET_BIND(HANDLE_CONTACT_NOTIFICATION);

    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, ndo_last_notification_id);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION, contact_object_id);

    MYSQL_BIND(HANDLE_CONTACT_NOTIFICATION);
    MYSQL_EXECUTE(HANDLE_CONTACT_NOTIFICATION);

    /* see notes regarding `ndo_last_notification_id` - those apply here too */
    ndo_last_contact_notification_id = mysql_insert_id(mysql_connection);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_contact_notification_method(int type, void * d)
{
    trace_handler(contact_notification_method);

    nebstruct_contact_notification_method_data * data = d;

    int command_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, data->command_name);

    MYSQL_RESET_BIND(HANDLE_CONTACT_NOTIFICATION_METHOD);

    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, ndo_last_contact_notification_id);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, data->start_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, data->start_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, data->end_time.tv_sec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, data->end_time.tv_usec);
    MYSQL_BIND_INT(HANDLE_CONTACT_NOTIFICATION_METHOD, command_object_id);
    MYSQL_BIND_STR(HANDLE_CONTACT_NOTIFICATION_METHOD, data->command_args);

    MYSQL_BIND(HANDLE_CONTACT_NOTIFICATION_METHOD);
    MYSQL_EXECUTE(HANDLE_CONTACT_NOTIFICATION_METHOD);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_external_command(int type, void * d)
{
    trace_handler(external_command);

    nebstruct_external_command_data * data = d;

    if (data->type != NEBTYPE_EXTERNALCOMMAND_START) {
        return NDO_OK;
    }

    MYSQL_RESET_BIND(HANDLE_EXTERNAL_COMMAND);

    MYSQL_BIND_INT(HANDLE_EXTERNAL_COMMAND, data->command_type);
    MYSQL_BIND_INT(HANDLE_EXTERNAL_COMMAND, data->entry_time);
    MYSQL_BIND_STR(HANDLE_EXTERNAL_COMMAND, data->command_string);
    MYSQL_BIND_STR(HANDLE_EXTERNAL_COMMAND, data->command_args);

    MYSQL_BIND(HANDLE_EXTERNAL_COMMAND);
    MYSQL_EXECUTE(HANDLE_EXTERNAL_COMMAND);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_acknowledgement(int type, void * d)
{
    trace_handler(acknowledgement);

    nebstruct_acknowledgement_data * data = d;

    int object_id = 0;

    if (data->acknowledgement_type == SERVICE_ACKNOWLEDGEMENT) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    MYSQL_RESET_BIND(HANDLE_ACKNOWLEDGEMENT);

    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->timestamp.tv_usec);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->acknowledgement_type);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, object_id);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->state);
    MYSQL_BIND_STR(HANDLE_ACKNOWLEDGEMENT, data->author_name);
    MYSQL_BIND_STR(HANDLE_ACKNOWLEDGEMENT, data->comment_data);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->is_sticky);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->persistent_comment);
    MYSQL_BIND_INT(HANDLE_ACKNOWLEDGEMENT, data->notify_contacts);

    MYSQL_BIND(HANDLE_ACKNOWLEDGEMENT);
    MYSQL_EXECUTE(HANDLE_ACKNOWLEDGEMENT);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_state_change(int type, void * d)
{
    trace_handler(statechange);

    nebstruct_statechange_data * data = d;

    int object_id = 0;
    int last_state = 0;
    int last_hard_state = 0;

    if (data->type != NEBTYPE_STATECHANGE_END) {
        return NDO_OK;
    }

    if (data->statechange_type == SERVICE_STATECHANGE) {

        service * svc = data->object_ptr;

        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, svc->host_name, svc->description);

        last_state = svc->last_state;
        last_hard_state = svc->last_hard_state;
    }
    else {

        host * hst = data->object_ptr;

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, hst->name);

        last_state = hst->last_state;
        last_hard_state = hst->last_hard_state;
    }

    MYSQL_RESET_BIND(HANDLE_STATE_CHANGE);

    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->timestamp.tv_sec);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->timestamp.tv_usec);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, object_id);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->state);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->state_type);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->current_attempt);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, data->max_attempts);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, last_state);
    MYSQL_BIND_INT(HANDLE_STATE_CHANGE, last_hard_state);
    MYSQL_BIND_STR(HANDLE_STATE_CHANGE, data->output);
    MYSQL_BIND_STR(HANDLE_STATE_CHANGE, data->longoutput);

    MYSQL_BIND(HANDLE_STATE_CHANGE);
    MYSQL_EXECUTE(HANDLE_STATE_CHANGE);

    trace_func_end();
    return NDO_OK;
}


int ndo_handle_retention(int type, void * d)
{
    trace_handler(retention);

    nebstruct_retention_data * data = d;

    if (data->type == NEBTYPE_RETENTIONDATA_ENDLOAD) {
        ndo_write_config(NDO_CONFIG_DUMP_RETAINED);
    }

    trace_func_end();
    return NDO_OK;
}
