
int ndo_set_all_objects_inactive()
{
    trace_func_begin();

    char * deactivate_sql = "UPDATE nagios_objects SET is_active = 0";

    ndo_return = mysql_query(mysql_connection, deactivate_sql);
    if (ndo_return != 0) {

        char err[1024] = { 0 };
        snprintf(err, 1023, "query(%s) failed with rc (%d), mysql (%d: %s)", deactivate_sql, ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(err);
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_table_genocide()
{
    trace_func_begin();

    int i = 0;

    char * truncate_sql[] = {
        "TRUNCATE TABLE nagios_programstatus",
        "TRUNCATE TABLE nagios_hoststatus",
        "TRUNCATE TABLE nagios_servicestatus",
        "TRUNCATE TABLE nagios_contactstatus",
        "TRUNCATE TABLE nagios_timedeventqueue",
        "TRUNCATE TABLE nagios_comments",
        "TRUNCATE TABLE nagios_scheduleddowntime",
        "TRUNCATE TABLE nagios_runtimevariables",
        "TRUNCATE TABLE nagios_customvariablestatus",
        "TRUNCATE TABLE nagios_configfiles",
        "TRUNCATE TABLE nagios_configfilevariables",
        "TRUNCATE TABLE nagios_customvariables",
        "TRUNCATE TABLE nagios_commands",
        "TRUNCATE TABLE nagios_timeperiods",
        "TRUNCATE TABLE nagios_timeperiod_timeranges",
        "TRUNCATE TABLE nagios_contactgroups",
        "TRUNCATE TABLE nagios_contactgroup_members",
        "TRUNCATE TABLE nagios_hostgroups",
        "TRUNCATE TABLE nagios_servicegroups",
        "TRUNCATE TABLE nagios_servicegroup_members",
        "TRUNCATE TABLE nagios_hostescalations",
        "TRUNCATE TABLE nagios_hostescalation_contacts",
        "TRUNCATE TABLE nagios_serviceescalations",
        "TRUNCATE TABLE nagios_serviceescalation_contacts",
        "TRUNCATE TABLE nagios_hostdependencies",
        "TRUNCATE TABLE nagios_servicedependencies",
        "TRUNCATE TABLE nagios_contacts",
        "TRUNCATE TABLE nagios_contact_addresses",
        "TRUNCATE TABLE nagios_contact_notificationcommands",
        "TRUNCATE TABLE nagios_hosts",
        "TRUNCATE TABLE nagios_host_parenthosts",
        "TRUNCATE TABLE nagios_host_contacts",
        "TRUNCATE TABLE nagios_services",
        "TRUNCATE TABLE nagios_service_parentservices",
        "TRUNCATE TABLE nagios_service_contacts",
        "TRUNCATE TABLE nagios_service_contactgroups",
        "TRUNCATE TABLE nagios_host_contactgroups",
        "TRUNCATE TABLE nagios_hostescalation_contactgroups",
        "TRUNCATE TABLE nagios_serviceescalation_contactgroups",
    };

    for (i = 0; i < ARRAY_SIZE(truncate_sql); i++) {

        ndo_return = mysql_query(mysql_connection, truncate_sql[i]);
        if (ndo_return != 0) {

            char err[1024] = { 0 };
            snprintf(err, 1023, "query(%s) failed with rc (%d), mysql (%d: %s)", truncate_sql[i], ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
            ndo_log(err);
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_config_files()
{
    trace_func_begin();
    trace_func_end();
    return NDO_OK;
}


int ndo_write_object_config(int config_type)
{
    trace("config_type=%d", config_type);

    ndo_write_commands(config_type);
    ndo_write_timeperiods(config_type);
    ndo_write_contacts(config_type);
    ndo_write_contactgroups(config_type);
    ndo_write_hosts(config_type);
    ndo_write_hostgroups(config_type);
    ndo_write_services(config_type);
    ndo_write_servicegroups(config_type);
    ndo_write_hostescalations(config_type);
    ndo_write_serviceescalations(config_type);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_runtime_variables()
{
    trace_func_begin();
    trace_func_end();
    return NDO_OK;
}


int ndo_begin_active_objects(int run_count)
{
    trace("run_count=%d", run_count);

    char * active_objects_query_base = "UPDATE nagios_objects SET is_active = 1 WHERE object_id IN (";
    size_t active_objects_query_base_len = STRLIT_LEN(active_objects_query_base);
    
    active_objects_count = 0;
    active_objects_i = 0;

    if (run_count == 0) {
        ndo_stmt_new[WRITE_ACTIVE_OBJECTS] = mysql_stmt_init(mysql_connection);
        MYSQL_RESET_BIND_NEW(WRITE_CONTACT_ADDRESSES);

        active_objects_max_params = (MAX_SQL_BUFFER - 1 - active_objects_query_base_len) / 2;

        if (active_objects_max_params < 0) {
            ndo_log("Something is very very wrong with MAX_SQL_BUFFER and/or active_objects_query_base_len");
            return NDO_ERROR;
        }

        /* 65536 is the absolute max params we can bind (mysql official docs) */
        if (active_objects_max_params > 65536) {
            active_objects_max_params = 65536;
        }
    }

    /* from here, everytime that ndo_set_object_active is called, we cat
        "?," to the string, and once it's done, we overwrite the final "," with ")" */

    if (run_count > 0) {
        memset(active_objects_query, 0, sizeof(active_objects_query));
    }
    strcpy(active_objects_query, active_objects_query_base);

    /* only set up the bind array once */
    if (run_count == 0) {
        active_objects_bind = calloc(active_objects_max_params, sizeof(MYSQL_BIND));
        active_objects_object_ids = calloc(active_objects_max_params, sizeof(int));
        if (active_objects_bind == NULL || active_objects_object_ids == NULL) {
            char msg[1024];
            snprintf(msg, 1023, "Unable to allocate (%d) MYSQL_BINDs/ints", active_objects_max_params);
            ndo_log(msg);
            return NDO_ERROR;
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_end_active_objects()
{
    trace_func_begin();

    free(active_objects_bind);
    mysql_stmt_close(ndo_stmt_new[WRITE_ACTIVE_OBJECTS]);

    trace_func_end();
    return NDO_OK;
}


int ndo_set_object_active(int object_id, int config_type, void * next)
{
    trace("object_id=%d, config_type=%d, next=%p", object_id, config_type, next);

    if (config_type != NDO_CONFIG_DUMP_ORIGINAL) {
        return NDO_OK;
    }

    active_objects_object_ids[active_objects_i] = object_id;

    active_objects_bind[active_objects_i].buffer_type = MYSQL_TYPE_LONG;
    active_objects_bind[active_objects_i].buffer = &(active_objects_object_ids[active_objects_i]);

    strcat(active_objects_query, "?,");

    active_objects_i++;

    if (active_objects_i >= active_objects_max_params || next == NULL) {

        /* update last comma to a ) */
        char * last_comma = strrchr(active_objects_query, ',');
        * last_comma = ')';

        ndo_return = mysql_stmt_prepare(ndo_stmt_new[WRITE_ACTIVE_OBJECTS], active_objects_query, strlen(active_objects_query));
        if (ndo_return != 0) {
            snprintf(ndo_error_msg, 1023, "Unable to prepare: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[WRITE_ACTIVE_OBJECTS]));
            ndo_log(ndo_error_msg);
        }

        ndo_return = mysql_stmt_bind_param(ndo_stmt_new[WRITE_ACTIVE_OBJECTS], active_objects_bind);
        if (ndo_return != 0) {
            snprintf(ndo_error_msg, 1023, "Unable to bind parameters: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[WRITE_ACTIVE_OBJECTS]));
            ndo_log(ndo_error_msg);
        }

        ndo_return = mysql_stmt_execute(ndo_stmt_new[WRITE_ACTIVE_OBJECTS]);
        if (ndo_return != 0) {
            snprintf(ndo_error_msg, 1023, "Unable to execute: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[WRITE_ACTIVE_OBJECTS]));
            ndo_log(ndo_error_msg);
        }

        active_objects_run++;
        ndo_begin_active_objects(active_objects_run);
    }

    trace_func_end();
    return NDO_OK;
}


int send_subquery(int stmt, int * counter, char * query, char * query_on_update, size_t * query_len, size_t query_base_len, size_t query_on_update_len)
{
    trace("stmt=%d, *counter=%d, query=%s, query_on_update=%s, *query_len=%zu, query_base_lan=%zu, query_on_update_len=%zu", stmt, * counter, query, query_on_update, * query_len, query_base_len, query_on_update_len);

    strcpy(query + (* query_len) - 1, query_on_update);

    ndo_return = mysql_stmt_prepare(ndo_stmt_new[stmt], query, strlen(query));
    if (ndo_return != 0) {
        snprintf(ndo_error_msg, 1023, "Unable to prepare: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[stmt]));
        ndo_log(ndo_error_msg);
    }

    ndo_return = mysql_stmt_bind_param(ndo_stmt_new[stmt], ndo_bind_new[stmt]);
    if (ndo_return != 0) {
        snprintf(ndo_error_msg, 1023, "Unable to bind: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[stmt]));
        ndo_log(ndo_error_msg);
    }

    ndo_return = mysql_stmt_execute(ndo_stmt_new[stmt]);
    if (ndo_return != 0) {
        snprintf(ndo_error_msg, 1023, "Unable to execute: ndo_return = %d (%s)", ndo_return, mysql_stmt_error(ndo_stmt_new[stmt]));
        ndo_log(ndo_error_msg);
    }

    memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);

    * query_len = query_base_len;
    * counter = 0;
    ndo_bind_new_i[stmt] = 0;

    trace_func_end();
    return NDO_OK;
}


int ndo_write_commands(int config_type)
{
    trace("config_type=%d", config_type);

    command * tmp = command_list;
    int object_id = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_commands (instance_id, object_id, config_type, command_line) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), command_line = VALUES(command_line)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, tmp->name);
        ndo_set_object_active(object_id, config_type, tmp->next);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->command_line);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        tmp = tmp->next;
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_timeperiods(int config_type)
{
    trace("config_type=%d", config_type);

    timeperiod * tmp = timeperiod_list;
    timerange * range = NULL;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * timeperiod_ids = NULL;

    timeperiod_ids = calloc(num_objects.timeperiods, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiods (instance_id, timeperiod_object_id, config_type, alias) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), timeperiod_object_id = VALUES(timeperiod_object_id), config_type = VALUES(config_type), alias = VALUES(alias)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->name);
        ndo_set_object_active(object_id, config_type, tmp->next);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        timeperiod_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    ndo_write_timeperiod_timeranges(timeperiod_ids);

    free(timeperiod_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_timeperiod_timeranges(int * timeperiod_ids)
{
    trace("timeperiod_ids=%p", timeperiod_ids);

    timeperiod * tmp = timeperiod_list;
    timerange * range = NULL;
    int i = 0;

    int day = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiod_timeranges (instance_id, timeperiod_id, day, start_sec, end_sec) VALUES (1,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), timeperiod_id = VALUES(timeperiod_id), day = VALUES(day), start_sec = VALUES(start_sec), end_sec = VALUES(end_sec)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        for (day = 0; day < 7; day++) {
            for (range = tmp->days[day]; range != NULL; range = range->next) {

                MYSQL_BIND_INT(timeperiod_ids[i]);
                MYSQL_BIND_INT(day);
                MYSQL_BIND_INT(range->range_start);
                MYSQL_BIND_INT(range->range_end);

                MYSQL_BIND();
                MYSQL_EXECUTE();
            }
        }

        i++;
        tmp = tmp->next;
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_contacts(int config_type)
{
    trace("config_type=%d", config_type);

    contact * tmp = contact_list;
    int i = 0;

    int max_object_insert_count = 0;
    int loops = 0;
    int loop = 0;
    int write_query = FALSE;
    int dont_reset_query = FALSE;

    size_t cur_pos = 0;

    int object_ids[MAX_OBJECT_INSERT] = { 0 };
    int host_timeperiod_object_id[MAX_OBJECT_INSERT] = { 0 };
    int service_timeperiod_object_id[MAX_OBJECT_INSERT] = { 0 };

    int notify_options[11] = { 0 };

    char query[MAX_SQL_BUFFER] = { 0 };

    char * query_base = "INSERT INTO nagios_contacts (instance_id, config_type, contact_object_id, alias, email_address, pager_address, host_timeperiod_object_id, service_timeperiod_object_id, host_notifications_enabled, service_notifications_enabled, can_submit_commands, notify_service_recovery, notify_service_warning, notify_service_unknown, notify_service_critical, notify_service_flapping, notify_service_downtime, notify_host_recovery, notify_host_down, notify_host_unreachable, notify_host_flapping, notify_host_downtime, minimum_importance) VALUES ";
    size_t query_base_len = STRLIT_LEN(query_base);
    size_t query_len = query_base_len;

    char * query_values = "(1,?,?,?,?,?,?,?,X,X,X,X,X,X,X,X,X,X,X,X,X,X,?),";
    size_t query_values_len = STRLIT_LEN(query_values);

    char * query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), contact_object_id = VALUES(contact_object_id), alias = VALUES(alias), email_address = VALUES(email_address), pager_address = VALUES(pager_address), host_timeperiod_object_id = VALUES(host_timeperiod_object_id), service_timeperiod_object_id = VALUES(service_timeperiod_object_id), host_notifications_enabled = VALUES(host_notifications_enabled), service_notifications_enabled = VALUES(service_notifications_enabled), can_submit_commands = VALUES(can_submit_commands), notify_service_recovery = VALUES(notify_service_recovery), notify_service_warning = VALUES(notify_service_warning), notify_service_unknown = VALUES(notify_service_unknown), notify_service_critical = VALUES(notify_service_critical), notify_service_flapping = VALUES(notify_service_flapping), notify_service_downtime = VALUES(notify_service_downtime), notify_host_recovery = VALUES(notify_host_recovery), notify_host_down = VALUES(notify_host_down), notify_host_unreachable = VALUES(notify_host_unreachable), notify_host_flapping = VALUES(notify_host_flapping), notify_host_downtime = VALUES(notify_host_downtime), minimum_importance = VALUES(minimum_importance)";
    size_t query_on_update_len = STRLIT_LEN(query_on_update);

    ndo_return = mysql_query(mysql_connection, "LOCK TABLES nagios_logentries WRITE, nagios_objects WRITE, nagios_contacts WRITE");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
        return NDO_ERROR;
    }

    strcpy(query, query_base);

    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * query_values_len + query_base_len + query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    loops = num_objects.contacts / max_object_insert_count;

    if (num_objects.contacts % max_object_insert_count != 0) {
        loops++;
    }

    /* if num contacts is evenly divisible, we never need to write 
       the query after the first time */
    else {
        dont_reset_query = TRUE;
    }

    write_query = TRUE;
    loop = 1;

    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        if (write_query == TRUE) {
            memcpy(query + query_len, query_values, query_values_len);
            query_len += query_values_len;
        }
        /* put our "cursor" at the beginning of whichever query_values we are at
           specifically at the '(' character of current values section */
        cur_pos = query_base_len + (i * query_values_len);

        object_ids[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, tmp->name);
        ndo_set_object_active(object_ids[i], config_type, tmp->next);

        host_timeperiod_object_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->host_notification_period);
        service_timeperiod_object_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->service_notification_period);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_ids[i]);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->email);
        MYSQL_BIND_STR(tmp->pager);
        MYSQL_BIND_INT(host_timeperiod_object_id[i]);
        MYSQL_BIND_INT(service_timeperiod_object_id[i]);

        UPDATE_QUERY_X_POS(query, cur_pos, 17, tmp->host_notifications_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 19, tmp->service_notifications_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 21, tmp->can_submit_commands);

        UPDATE_QUERY_X_POS(query, cur_pos, 17, flag_isset(tmp->service_notification_options, OPT_RECOVERY));
        UPDATE_QUERY_X_POS(query, cur_pos, 19, flag_isset(tmp->service_notification_options, OPT_WARNING));
        UPDATE_QUERY_X_POS(query, cur_pos, 21, flag_isset(tmp->service_notification_options, OPT_UNKNOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 23, flag_isset(tmp->service_notification_options, OPT_CRITICAL));
        UPDATE_QUERY_X_POS(query, cur_pos, 25, flag_isset(tmp->service_notification_options, OPT_FLAPPING));
        UPDATE_QUERY_X_POS(query, cur_pos, 27, flag_isset(tmp->service_notification_options, OPT_DOWNTIME));
        UPDATE_QUERY_X_POS(query, cur_pos, 29, flag_isset(tmp->host_notification_options, OPT_RECOVERY));
        UPDATE_QUERY_X_POS(query, cur_pos, 31, flag_isset(tmp->host_notification_options, OPT_DOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 33, flag_isset(tmp->host_notification_options, OPT_UNREACHABLE));
        UPDATE_QUERY_X_POS(query, cur_pos, 35, flag_isset(tmp->host_notification_options, OPT_FLAPPING));
        UPDATE_QUERY_X_POS(query, cur_pos, 37, flag_isset(tmp->host_notification_options, OPT_DOWNTIME));

        MYSQL_BIND_INT(tmp->minimum_value);

        i++;

        /* we need to finish the query and execute */
        if (i >= max_object_insert_count || tmp->next == NULL) {

            if (write_query == TRUE) {
                memcpy(query + query_len - 1, query_on_update, query_on_update_len);
                query_len += query_on_update_len;
            }

            if (loop == 1 || loop == loops) {
                ndo_return = mysql_stmt_prepare(ndo_stmt, query, query_len);
                ndo_return = mysql_stmt_bind_param(ndo_stmt, ndo_bind);
            }
            ndo_return = mysql_stmt_execute(ndo_stmt);

            ndo_bind_i = 0;
            i = 0;
            write_query = FALSE;

            /* if we're on the second to last loop we reset to build the final query */
            if (loop == loops - 1 && dont_reset_query == FALSE) {
                memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);
                query_len = query_base_len;
                write_query = TRUE;
            }

            loop++;
        }

        tmp = tmp->next;
    }

    ndo_return = mysql_query(mysql_connection, "UNLOCK TABLES");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
    }

    ndo_write_contact_objects(config_type);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_contact_objects(int config_type)
{
    trace("config_type=%d", config_type);

    contact * tmp = contact_list;

    int address_number = 0;
    commandsmember * cmd = NULL;
    customvariablesmember * var = NULL;

    int max_object_insert_count = 0;

    int host_notification_command_type = NDO_DATA_HOSTNOTIFICATIONCOMMAND;
    int service_notification_command_type = NDO_DATA_SERVICENOTIFICATIONCOMMAND;

    int addresses_count = 0;
    char addresses_query[MAX_SQL_BUFFER] = { 0 };
    char addresses_query_base[] = "INSERT INTO nagios_contact_addresses (instance_id, contact_id, address_number, address) VALUES ";
    size_t addresses_query_base_len = STRLIT_LEN(addresses_query_base);
    size_t addresses_query_len = addresses_query_base_len;
    char addresses_query_values[] = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?),?,?),";
    size_t addresses_query_values_len = STRLIT_LEN(addresses_query_values);
    char addresses_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contact_id = VALUES(contact_id), address_number = VALUES(address_number), address = VALUES(address)";
    size_t addresses_query_on_update_len = STRLIT_LEN(addresses_query_on_update);

    int notificationcommands_count = 0;
    char notificationcommands_query[MAX_SQL_BUFFER] = { 0 };
    char notificationcommands_query_base[] = "INSERT INTO nagios_contact_notificationcommands (instance_id, contact_id, notification_type, command_object_id) VALUES ";
    size_t notificationcommands_query_base_len = STRLIT_LEN(notificationcommands_query_base);
    size_t notificationcommands_query_len = notificationcommands_query_base_len;
    char notificationcommands_query_values[] = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?),?,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 12 AND name1 = ?)),";
    size_t notificationcommands_query_values_len = STRLIT_LEN(notificationcommands_query_values);
    char notificationcommands_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contact_id = VALUES(contact_id), notification_type = VALUES(notification_type), command_object_id = VALUES(command_object_id)";
    size_t notificationcommands_query_on_update_len = STRLIT_LEN(notificationcommands_query_on_update);

    int var_count = 0;
    char var_query[MAX_SQL_BUFFER] = { 0 };
    char var_query_base[] = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = STRLIT_LEN(var_query_base);
    size_t var_query_len = var_query_base_len;
    char var_query_values[] = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?),?,?,?,?),";
    size_t var_query_values_len = STRLIT_LEN(var_query_values);
    char var_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";
    size_t var_query_on_update_len = STRLIT_LEN(var_query_on_update);

    ndo_stmt_new[WRITE_CONTACT_ADDRESSES] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_CONTACT_NOTIFICATIONCOMMANDS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_CUSTOMVARS] = mysql_stmt_init(mysql_connection);

    MYSQL_RESET_BIND_NEW(WRITE_CONTACT_ADDRESSES);
    MYSQL_RESET_BIND_NEW(WRITE_CONTACT_NOTIFICATIONCOMMANDS);
    MYSQL_RESET_BIND_NEW(WRITE_CUSTOMVARS);

    strcpy(addresses_query, addresses_query_base);
    strcpy(notificationcommands_query, notificationcommands_query_base);
    strcpy(var_query, var_query_base);

    /* notificationcommands query is the longest. so if this math works for that, it works for the other queries as well */
    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * notificationcommands_query_values_len + notificationcommands_query_base_len + notificationcommands_query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    while (tmp != NULL) {

        for (address_number = 1; address_number < (MAX_CONTACT_ADDRESSES + 1); address_number++) {

            strcpy(addresses_query + addresses_query_len, addresses_query_values);
            addresses_query_len += addresses_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CONTACT_ADDRESSES, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CONTACT_ADDRESSES, address_number);
            MYSQL_BIND_NEW_STR(WRITE_CONTACT_ADDRESSES, tmp->address[address_number - 1]);

            addresses_count++;

            if (addresses_count >= max_object_insert_count) {
                send_subquery(WRITE_CONTACT_ADDRESSES, &addresses_count, addresses_query, addresses_query_on_update, &addresses_query_len, addresses_query_base_len, addresses_query_on_update_len);
            }
        }

        cmd = tmp->host_notification_commands;
        while (cmd != NULL) {

            strcpy(notificationcommands_query + notificationcommands_query_len, notificationcommands_query_values);
            notificationcommands_query_len += notificationcommands_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CONTACT_NOTIFICATIONCOMMANDS, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CONTACT_NOTIFICATIONCOMMANDS, host_notification_command_type);
            MYSQL_BIND_NEW_STR(WRITE_CONTACT_NOTIFICATIONCOMMANDS, cmd->command);

            cmd = cmd->next;
            notificationcommands_count++;

            if (notificationcommands_count >= max_object_insert_count) {
                send_subquery(WRITE_CONTACT_NOTIFICATIONCOMMANDS, &notificationcommands_count, notificationcommands_query, notificationcommands_query_on_update, &notificationcommands_query_len, notificationcommands_query_base_len, notificationcommands_query_on_update_len);
            }
        }

        cmd = tmp->service_notification_commands;
        while (cmd != NULL) {

            strcpy(notificationcommands_query + notificationcommands_query_len, notificationcommands_query_values);
            notificationcommands_query_len += notificationcommands_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CONTACT_NOTIFICATIONCOMMANDS, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CONTACT_NOTIFICATIONCOMMANDS, service_notification_command_type);
            MYSQL_BIND_NEW_STR(WRITE_CONTACT_NOTIFICATIONCOMMANDS, cmd->command);

            cmd = cmd->next;
            notificationcommands_count++;

            if (notificationcommands_count >= max_object_insert_count) {
                send_subquery(WRITE_CONTACT_NOTIFICATIONCOMMANDS, &notificationcommands_count, notificationcommands_query, notificationcommands_query_on_update, &notificationcommands_query_len, notificationcommands_query_base_len, notificationcommands_query_on_update_len);
            }
        }

        var = tmp->custom_variables;
        while (var != NULL) {

            strcpy(var_query + var_query_len, var_query_values);
            var_query_len += var_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, config_type);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, var->has_been_modified);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_value);

            var = var->next;
            var_count++;

            if (var_count >= max_object_insert_count) {
                send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
            }
        }

        if (addresses_count > 0 && (addresses_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_CONTACT_ADDRESSES, &addresses_count, addresses_query, addresses_query_on_update, &addresses_query_len, addresses_query_base_len, addresses_query_on_update_len);
        }

        if (notificationcommands_count > 0 && (notificationcommands_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_CONTACT_NOTIFICATIONCOMMANDS, &notificationcommands_count, notificationcommands_query, notificationcommands_query_on_update, &notificationcommands_query_len, notificationcommands_query_base_len, notificationcommands_query_on_update_len);
        }

        if (var_count > 0 && (var_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
        }

        tmp = tmp->next;
    }

    mysql_stmt_close(ndo_stmt_new[WRITE_CONTACT_ADDRESSES]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CONTACT_NOTIFICATIONCOMMANDS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_contactgroups(int config_type)
{
    trace("config_type=%d", config_type);

    contactgroup * tmp = contactgroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * contactgroup_ids = NULL;

    contactgroup_ids = calloc(num_objects.contactgroups, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroups (instance_id, contactgroup_object_id, config_type, alias) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contactgroup_object_id = VALUES(contactgroup_object_id), config_type = VALUES(config_type), alias = VALUES(alias)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, tmp->group_name);
        ndo_set_object_active(object_id, config_type, tmp->next);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        contactgroup_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    ndo_write_contactgroup_members(contactgroup_ids);

    free(contactgroup_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_contactgroup_members(int * contactgroup_ids)
{
    trace("contactgroup_ids=%p", contactgroup_ids);

    contactgroup * tmp = contactgroup_list;
    contactsmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroup_members (instance_id, contactgroup_id, contact_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contactgroup_id = VALUES(contactgroup_id), contact_object_id = VALUES(contact_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, member->contact_name);

            MYSQL_BIND_INT(contactgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hosts(int config_type)
{
    trace("config_type=%d", config_type);

    host * tmp = host_list;
    int i = 0;

    int max_object_insert_count = 0;
    int loops = 0;
    int loop = 0;
    int write_query = FALSE;
    int dont_reset_query = FALSE;

    size_t cur_pos = 0;

    int object_ids[MAX_OBJECT_INSERT] = { 0 };

    int host_options[MAX_OBJECT_INSERT][11] = { 0 };
    int check_command_id[MAX_OBJECT_INSERT] = { 0 };
    char * check_command[MAX_OBJECT_INSERT] = { NULL };
    char * check_command_args[MAX_OBJECT_INSERT] = { NULL };
    int event_handler_id[MAX_OBJECT_INSERT] = { 0 };
    char * event_handler[MAX_OBJECT_INSERT] = { NULL };
    char * event_handler_args[MAX_OBJECT_INSERT] = { NULL };
    int check_timeperiod_id[MAX_OBJECT_INSERT] = { 0 };
    int notification_timeperiod_id[MAX_OBJECT_INSERT] = { 0 };

    char query[MAX_SQL_BUFFER] = { 0 };

    char query_base[] = "INSERT INTO nagios_hosts (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d, importance) VALUES ";
    size_t query_base_len = STRLIT_LEN(query_base);
    size_t query_len = query_base_len;

    char query_values[] = "(1,?,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,X,X,X,X,X,X,X,X,X,X,X,X,?,?,X,X,?,X,X,X,X,X,X,X,0,?,?,?,?,?,?,?,X,?,?,X,?,?,?,?),";
    size_t query_values_len = STRLIT_LEN(query_values);

    char query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), alias = VALUES(alias), display_name = VALUES(display_name), address = VALUES(address), check_command_object_id = VALUES(check_command_object_id), check_command_args = VALUES(check_command_args), eventhandler_command_object_id = VALUES(eventhandler_command_object_id), eventhandler_command_args = VALUES(eventhandler_command_args), check_timeperiod_object_id = VALUES(check_timeperiod_object_id), notification_timeperiod_object_id = VALUES(notification_timeperiod_object_id), failure_prediction_options = VALUES(failure_prediction_options), check_interval = VALUES(check_interval), retry_interval = VALUES(retry_interval), max_check_attempts = VALUES(max_check_attempts), first_notification_delay = VALUES(first_notification_delay), notification_interval = VALUES(notification_interval), notify_on_down = VALUES(notify_on_down), notify_on_unreachable = VALUES(notify_on_unreachable), notify_on_recovery = VALUES(notify_on_recovery), notify_on_flapping = VALUES(notify_on_flapping), notify_on_downtime = VALUES(notify_on_downtime), stalk_on_up = VALUES(stalk_on_up), stalk_on_down = VALUES(stalk_on_down), stalk_on_unreachable = VALUES(stalk_on_unreachable), flap_detection_enabled = VALUES(flap_detection_enabled), flap_detection_on_up = VALUES(flap_detection_on_up), flap_detection_on_down = VALUES(flap_detection_on_down), flap_detection_on_unreachable = VALUES(flap_detection_on_unreachable), low_flap_threshold = VALUES(low_flap_threshold), high_flap_threshold = VALUES(high_flap_threshold), process_performance_data = VALUES(process_performance_data), freshness_checks_enabled = VALUES(freshness_checks_enabled), freshness_threshold = VALUES(freshness_threshold), passive_checks_enabled = VALUES(passive_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), active_checks_enabled = VALUES(active_checks_enabled), retain_status_information = VALUES(retain_status_information), retain_nonstatus_information = VALUES(retain_nonstatus_information), notifications_enabled = VALUES(notifications_enabled), obsess_over_host = VALUES(obsess_over_host), failure_prediction_enabled = VALUES(failure_prediction_enabled), notes = VALUES(notes), notes_url = VALUES(notes_url), action_url = VALUES(action_url), icon_image = VALUES(icon_image), icon_image_alt = VALUES(icon_image_alt), vrml_image = VALUES(vrml_image), statusmap_image = VALUES(statusmap_image), have_2d_coords = VALUES(have_2d_coords), x_2d = VALUES(x_2d), y_2d = VALUES(y_2d), have_3d_coords = VALUES(have_3d_coords), x_3d = VALUES(x_3d), y_3d = VALUES(y_3d), z_3d = VALUES(z_3d), importance = VALUES(importance)";
    size_t query_on_update_len = STRLIT_LEN(query_on_update);

    ndo_return = mysql_query(mysql_connection, "LOCK TABLES nagios_logentries WRITE, nagios_objects WRITE, nagios_hosts WRITE");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
        return NDO_ERROR;
    }

    strcpy(query, query_base);

    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * query_values_len + query_base_len + query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    loops = num_objects.hosts / max_object_insert_count;

    if (num_objects.hosts % max_object_insert_count != 0) {
        loops++;
    }

    /* if num hosts is evenly divisible, we never need to write 
       the query after the first time */
    else {
        dont_reset_query = TRUE;
    }

    write_query = TRUE;
    loop = 1;

    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        if (write_query == TRUE) {
            memcpy(query + query_len, query_values, query_values_len);
            query_len += query_values_len;
        }
        /* put our "cursor" at the beginning of whichever query_values we are at
           specifically at the '(' character of current values section */
        cur_pos = query_base_len + (i * query_values_len);

        object_ids[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->name);
        ndo_set_object_active(object_ids[i], config_type, tmp->next);

        check_command[i] = strtok(tmp->check_command, "!");
        if (check_command[i] == NULL) {
            check_command[i] = "";
            check_command_args[i] = "";
            check_command_id[i] = 0;
        } else {
            check_command_args[i] = strtok(NULL, "\0");
            check_command_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);
        }

        event_handler[i] = strtok(tmp->event_handler, "!");
        if (event_handler[i] == NULL) {
            event_handler[i] = "";
            event_handler_args[i] = "";
            event_handler_id[i] = 0;
        } else {
            event_handler_args[i] = strtok(NULL, "\0");
            event_handler_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, event_handler[i]);
        }

        check_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_ids[i]);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->display_name);
        MYSQL_BIND_STR(tmp->address);
        MYSQL_BIND_INT(check_command_id[i]);
        MYSQL_BIND_STR(check_command_args[i]);
        MYSQL_BIND_INT(event_handler_id[i]);
        MYSQL_BIND_STR(event_handler_args[i]);
        MYSQL_BIND_INT(check_timeperiod_id[i]);
        MYSQL_BIND_INT(notification_timeperiod_id[i]);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);

        UPDATE_QUERY_X_POS(query, cur_pos, 38, flag_isset(tmp->notification_options, OPT_DOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 40, flag_isset(tmp->notification_options, OPT_UNREACHABLE));
        UPDATE_QUERY_X_POS(query, cur_pos, 42, flag_isset(tmp->notification_options, OPT_RECOVERY));
        UPDATE_QUERY_X_POS(query, cur_pos, 44, flag_isset(tmp->notification_options, OPT_FLAPPING));
        UPDATE_QUERY_X_POS(query, cur_pos, 46, flag_isset(tmp->notification_options, OPT_DOWNTIME));
        UPDATE_QUERY_X_POS(query, cur_pos, 48, flag_isset(tmp->stalking_options, OPT_UP));
        UPDATE_QUERY_X_POS(query, cur_pos, 50, flag_isset(tmp->stalking_options, OPT_DOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 52, flag_isset(tmp->stalking_options, OPT_UNREACHABLE));
        UPDATE_QUERY_X_POS(query, cur_pos, 54, tmp->flap_detection_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 56, flag_isset(tmp->flap_detection_options, OPT_UP));
        UPDATE_QUERY_X_POS(query, cur_pos, 58, flag_isset(tmp->flap_detection_options, OPT_DOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 60, flag_isset(tmp->flap_detection_options, OPT_UNREACHABLE));

        MYSQL_BIND_DOUBLE(tmp->low_flap_threshold);
        MYSQL_BIND_DOUBLE(tmp->high_flap_threshold);

        UPDATE_QUERY_X_POS(query, cur_pos, 66, tmp->process_performance_data);
        UPDATE_QUERY_X_POS(query, cur_pos, 68, tmp->check_freshness);

        MYSQL_BIND_INT(tmp->freshness_threshold);

        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->accept_passive_checks);
        UPDATE_QUERY_X_POS(query, cur_pos, 74, tmp->event_handler_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 76, tmp->checks_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 78, tmp->retain_status_information);
        UPDATE_QUERY_X_POS(query, cur_pos, 80, tmp->retain_nonstatus_information);
        UPDATE_QUERY_X_POS(query, cur_pos, 82, tmp->notifications_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 84, tmp->obsess);

        MYSQL_BIND_STR(tmp->notes);
        MYSQL_BIND_STR(tmp->notes_url);
        MYSQL_BIND_STR(tmp->action_url);
        MYSQL_BIND_STR(tmp->icon_image);
        MYSQL_BIND_STR(tmp->icon_image_alt);
        MYSQL_BIND_STR(tmp->vrml_image);
        MYSQL_BIND_STR(tmp->statusmap_image);

        UPDATE_QUERY_X_POS(query, cur_pos, 102, tmp->have_2d_coords);

        MYSQL_BIND_INT(tmp->x_2d);
        MYSQL_BIND_INT(tmp->y_2d);

        UPDATE_QUERY_X_POS(query, cur_pos, 108, tmp->have_3d_coords);

        MYSQL_BIND_FLOAT(tmp->x_3d);
        MYSQL_BIND_FLOAT(tmp->y_3d);
        MYSQL_BIND_FLOAT(tmp->z_3d);
        MYSQL_BIND_INT(tmp->hourly_value);

        i++;

        /* we need to finish the query and execute */
        if (i >= max_object_insert_count || tmp->next == NULL) {

            if (write_query == TRUE) {
                memcpy(query + query_len - 1, query_on_update, query_on_update_len);
                query_len += query_on_update_len;
            }

            if (loop == 1 || loop == loops) {
                ndo_return = mysql_stmt_prepare(ndo_stmt, query, query_len);
                ndo_return = mysql_stmt_bind_param(ndo_stmt, ndo_bind);
            }
            ndo_return = mysql_stmt_execute(ndo_stmt);

            ndo_bind_i = 0;
            i = 0;
            write_query = FALSE;

            /* if we're on the second to last loop we reset to build the final query */
            if (loop == loops - 1 && dont_reset_query == FALSE) {
                memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);
                query_len = query_base_len;
                write_query = TRUE;
            }

            loop++;
        }

        tmp = tmp->next;
    }

    ndo_return = mysql_query(mysql_connection, "UNLOCK TABLES");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
    }

    ndo_write_hosts_objects(config_type);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hosts_objects(int config_type)
{
    trace("config_type=%d", config_type);

    host * tmp = host_list;

    customvariablesmember * var = NULL;
    hostsmember * parent = NULL;
    contactgroupsmember * group = NULL;
    contactsmember * cnt = NULL;

    int max_object_insert_count = 0;

    int parenthosts_count = 0;
    char parenthosts_query[MAX_SQL_BUFFER] = { 0 };
    char parenthosts_query_base[] = "INSERT INTO nagios_host_parenthosts (instance_id, host_id, parent_host_object_id) VALUES ";
    size_t parenthosts_query_base_len = STRLIT_LEN(parenthosts_query_base);
    size_t parenthosts_query_len = parenthosts_query_base_len;
    char parenthosts_query_values[] = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),";
    size_t parenthosts_query_values_len = STRLIT_LEN(parenthosts_query_values);
    char parenthosts_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), parent_host_object_id = VALUES(parent_host_object_id)";
    size_t parenthosts_query_on_update_len = STRLIT_LEN(parenthosts_query_on_update);

    int contactgroups_count = 0;
    char contactgroups_query[MAX_SQL_BUFFER] = { 0 };
    char contactgroups_query_base[] = "INSERT INTO nagios_host_contactgroups (instance_id, host_id, contactgroup_object_id) VALUES ";
    size_t contactgroups_query_base_len = STRLIT_LEN(contactgroups_query_base);
    size_t contactgroups_query_len = contactgroups_query_base_len;
    char contactgroups_query_values[] = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 11 AND name1 = ?)),";
    size_t contactgroups_query_values_len = STRLIT_LEN(contactgroups_query_values);
    char contactgroups_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), contactgroup_object_id = VALUES(contactgroup_object_id)";
    size_t contactgroups_query_on_update_len = STRLIT_LEN(contactgroups_query_on_update);

    int contacts_count = 0;
    char contacts_query[MAX_SQL_BUFFER] = { 0 };
    char contacts_query_base[] = "INSERT INTO nagios_host_contacts (instance_id, host_id, contact_object_id) VALUES ";
    size_t contacts_query_base_len = STRLIT_LEN(contacts_query_base);
    size_t contacts_query_len = contacts_query_base_len;
    char contacts_query_values[] = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?)),";
    size_t contacts_query_values_len = STRLIT_LEN(contacts_query_values);
    char contacts_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), contact_object_id = VALUES(contact_object_id)";
    size_t contacts_query_on_update_len = STRLIT_LEN(contacts_query_on_update);

    int var_count = 0;
    char var_query[MAX_SQL_BUFFER] = { 0 };
    char var_query_base[] = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = STRLIT_LEN(var_query_base);
    size_t var_query_len = var_query_base_len;
    char var_query_values[] = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?),?,?,?,?),";
    size_t var_query_values_len = STRLIT_LEN(var_query_values);
    char var_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";
    size_t var_query_on_update_len = STRLIT_LEN(var_query_on_update);

    ndo_stmt_new[WRITE_HOST_PARENTHOSTS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_HOST_CONTACTGROUPS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_HOST_CONTACTS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_CUSTOMVARS] = mysql_stmt_init(mysql_connection);

    MYSQL_RESET_BIND_NEW(WRITE_HOST_PARENTHOSTS);
    MYSQL_RESET_BIND_NEW(WRITE_HOST_CONTACTGROUPS);
    MYSQL_RESET_BIND_NEW(WRITE_HOST_CONTACTS);
    MYSQL_RESET_BIND_NEW(WRITE_CUSTOMVARS);

    strcpy(parenthosts_query, parenthosts_query_base);
    strcpy(contactgroups_query, contactgroups_query_base);
    strcpy(contacts_query, contacts_query_base);
    strcpy(var_query, var_query_base);

    /* contacts query is the longest. so if this math works for that, it works for the other queries as well */
    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * contacts_query_values_len + contacts_query_base_len + contacts_query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    while (tmp != NULL) {

        parent = tmp->parent_hosts;
        while (parent != NULL) {

            strcpy(parenthosts_query + parenthosts_query_len, parenthosts_query_values);
            parenthosts_query_len += parenthosts_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_HOST_PARENTHOSTS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_PARENTHOSTS, parent->host_name);

            parent = parent->next;
            parenthosts_count++;

            if (parenthosts_count >= max_object_insert_count) {
                send_subquery(WRITE_HOST_PARENTHOSTS, &parenthosts_count, parenthosts_query, parenthosts_query_on_update, &parenthosts_query_len, parenthosts_query_base_len, parenthosts_query_on_update_len);
            }
        }

        group = tmp->contact_groups;
        while (group != NULL) {

            strcpy(contactgroups_query + contactgroups_query_len, contactgroups_query_values);
            contactgroups_query_len += contactgroups_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTGROUPS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTGROUPS, group->group_name);

            group = group->next;
            contactgroups_count++;

            if (contactgroups_count >= max_object_insert_count) {
                send_subquery(WRITE_HOST_CONTACTGROUPS, &contactgroups_count, contactgroups_query, contactgroups_query_on_update, &contactgroups_query_len, contactgroups_query_base_len, contactgroups_query_on_update_len);
            }
        }

        cnt = tmp->contacts;
        while (cnt != NULL) {

            strcpy(contacts_query + contacts_query_len, contacts_query_values);
            contacts_query_len += contacts_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTS, cnt->contact_name);

            cnt = cnt->next;
            contacts_count++;

            if (contacts_count >= max_object_insert_count) {
                send_subquery(WRITE_HOST_CONTACTS, &contacts_count, contacts_query, contacts_query_on_update, &contacts_query_len, contacts_query_base_len, contacts_query_on_update_len);
            }
        }

        var = tmp->custom_variables;
        while (var != NULL) {

            strcpy(var_query + var_query_len, var_query_values);
            var_query_len += var_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, config_type);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, var->has_been_modified);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_value);

            var = var->next;
            var_count++;

            if (var_count >= max_object_insert_count) {
                send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
            }
        }

        if (parenthosts_count > 0 && (parenthosts_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_HOST_PARENTHOSTS, &parenthosts_count, parenthosts_query, parenthosts_query_on_update, &parenthosts_query_len, parenthosts_query_base_len, parenthosts_query_on_update_len);
        }

        if (contactgroups_count > 0 && (contactgroups_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_HOST_CONTACTGROUPS, &contactgroups_count, contactgroups_query, contactgroups_query_on_update, &contactgroups_query_len, contactgroups_query_base_len, contactgroups_query_on_update_len);
        }

        if (contacts_count > 0 && (contacts_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_HOST_CONTACTS, &contacts_count, contacts_query, contacts_query_on_update, &contacts_query_len, contacts_query_base_len, contacts_query_on_update_len);
        }

        if (var_count > 0 && (var_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
        }

        tmp = tmp->next;
    }

    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_PARENTHOSTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTGROUPS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostgroups(int config_type)
{
    trace("config_type=%d", config_type);

    hostgroup * tmp = hostgroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * hostgroup_ids = NULL;

    hostgroup_ids = calloc(num_objects.hostgroups, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostgroups (instance_id, hostgroup_object_id, config_type, alias) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), hostgroup_object_id = VALUES(hostgroup_object_id), config_type = VALUES(config_type), alias = VALUES(alias)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOSTGROUP, tmp->group_name);
        ndo_set_object_active(object_id, config_type, tmp->next);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        hostgroup_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    ndo_write_hostgroup_members(hostgroup_ids);

    free(hostgroup_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostgroup_members(int * hostgroup_ids)
{
    trace("hostgroup_ids=%p", hostgroup_ids);

    hostgroup * tmp = hostgroup_list;
    hostsmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostgroup_members (instance_id, hostgroup_id, host_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), hostgroup_id = VALUES(hostgroup_id), host_object_id = VALUES(host_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, member->host_name);

            MYSQL_BIND_INT(hostgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_services(int config_type)
{
    trace("config_type=%d", config_type);

    service * tmp = service_list;
    int i = 0;

    int max_object_insert_count = 0;
    int loops = 0;
    int loop = 0;
    int write_query = FALSE;
    int dont_reset_query = FALSE;

    size_t cur_pos = 0;

    int object_ids[MAX_OBJECT_INSERT] = { 0 };
    int host_object_id[MAX_OBJECT_INSERT] = { 0 };

    int service_options[MAX_OBJECT_INSERT][14] = { 0 };
    int check_command_id[MAX_OBJECT_INSERT] = { 0 };
    char * check_command[MAX_OBJECT_INSERT] = { NULL };
    char * check_command_args[MAX_OBJECT_INSERT] = { NULL };
    int event_handler_id[MAX_OBJECT_INSERT] = { 0 };
    char * event_handler[MAX_OBJECT_INSERT] = { NULL };
    char * event_handler_args[MAX_OBJECT_INSERT] = { NULL };
    int check_timeperiod_id[MAX_OBJECT_INSERT] = { 0 };
    int notification_timeperiod_id[MAX_OBJECT_INSERT] = { 0 };

    char query[MAX_SQL_BUFFER] = { 0 };

    char query_base[] = "INSERT INTO nagios_services (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, importance) VALUES ";
    size_t query_base_len = STRLIT_LEN(query_base);
    size_t query_len = query_base_len;

    char query_values[] = "(1,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,?,?,X,X,?,X,X,X,X,X,X,X,0,?,?,?,?,?,?),";
    size_t query_values_len = STRLIT_LEN(query_values);

    char query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), service_object_id = VALUES(service_object_id), display_name = VALUES(display_name), check_command_object_id = VALUES(check_command_object_id), check_command_args = VALUES(check_command_args), eventhandler_command_object_id = VALUES(eventhandler_command_object_id), eventhandler_command_args = VALUES(eventhandler_command_args), check_timeperiod_object_id = VALUES(check_timeperiod_object_id), notification_timeperiod_object_id = VALUES(notification_timeperiod_object_id), failure_prediction_options = VALUES(failure_prediction_options), check_interval = VALUES(check_interval), retry_interval = VALUES(retry_interval), max_check_attempts = VALUES(max_check_attempts), first_notification_delay = VALUES(first_notification_delay), notification_interval = VALUES(notification_interval), notify_on_warning = VALUES(notify_on_warning), notify_on_unknown = VALUES(notify_on_unknown), notify_on_critical = VALUES(notify_on_critical), notify_on_recovery = VALUES(notify_on_recovery), notify_on_flapping = VALUES(notify_on_flapping), notify_on_downtime = VALUES(notify_on_downtime), stalk_on_ok = VALUES(stalk_on_ok), stalk_on_warning = VALUES(stalk_on_warning), stalk_on_unknown = VALUES(stalk_on_unknown), stalk_on_critical = VALUES(stalk_on_critical), is_volatile = VALUES(is_volatile), flap_detection_enabled = VALUES(flap_detection_enabled), flap_detection_on_ok = VALUES(flap_detection_on_ok), flap_detection_on_warning = VALUES(flap_detection_on_warning), flap_detection_on_unknown = VALUES(flap_detection_on_unknown), flap_detection_on_critical = VALUES(flap_detection_on_critical), low_flap_threshold = VALUES(low_flap_threshold), high_flap_threshold = VALUES(high_flap_threshold), process_performance_data = VALUES(process_performance_data), freshness_checks_enabled = VALUES(freshness_checks_enabled), freshness_threshold = VALUES(freshness_threshold), passive_checks_enabled = VALUES(passive_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), active_checks_enabled = VALUES(active_checks_enabled), retain_status_information = VALUES(retain_status_information), retain_nonstatus_information = VALUES(retain_nonstatus_information), notifications_enabled = VALUES(notifications_enabled), obsess_over_service = VALUES(obsess_over_service), failure_prediction_enabled = VALUES(failure_prediction_enabled), notes = VALUES(notes), notes_url = VALUES(notes_url), action_url = VALUES(action_url), icon_image = VALUES(icon_image), icon_image_alt = VALUES(icon_image_alt), importance = VALUES(importance)";
    size_t query_on_update_len = STRLIT_LEN(query_on_update);

    ndo_return = mysql_query(mysql_connection, "LOCK TABLES nagios_logentries WRITE, nagios_objects WRITE, nagios_services WRITE, nagios_hosts READ");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
        return NDO_ERROR;
    }

    strcpy(query, query_base);

    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * query_values_len + query_base_len + query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    loops = num_objects.hosts / max_object_insert_count;

    if (num_objects.hosts % max_object_insert_count != 0) {
        loops++;
    }

    /* if num hosts is evenly divisible, we never need to write 
       the query after the first time */
    else {
        dont_reset_query = TRUE;
    }

    write_query = TRUE;
    loop = 1;

    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        if (write_query == TRUE) {
            memcpy(query + query_len, query_values, query_values_len);
            query_len += query_values_len;
        }

        /* put our "cursor" at the beginning of whichever query_values we are at
           specifically at the '(' character of current values section */
        cur_pos = query_base_len + (i * query_values_len);

        object_ids[i] = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->description);
        host_object_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);
        ndo_set_object_active(object_ids[i], config_type, tmp->next);

        check_command[i] = strtok(tmp->check_command, "!");
        if (check_command[i] == NULL) {
            check_command[i] = "";
            check_command_args[i] = "";
            check_command_id[i] = 0;
        } else {
            check_command_args[i] = strtok(NULL, "\0");
            check_command_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);
        }

        event_handler[i] = strtok(tmp->event_handler, "!");
        if (event_handler[i] == NULL) {
            event_handler[i] = "";
            event_handler_args[i] = "";
            event_handler_id[i] = 0;
        } else {
            event_handler_args[i] = strtok(NULL, "\0");
            event_handler_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, event_handler[i]);
        }

        check_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(host_object_id[i]);
        MYSQL_BIND_INT(object_ids[i]);
        MYSQL_BIND_STR(tmp->description);
        MYSQL_BIND_INT(check_command_id[i]);
        MYSQL_BIND_STR(check_command_args[i]);
        MYSQL_BIND_INT(event_handler_id[i]);
        MYSQL_BIND_STR(event_handler_args[i]);
        MYSQL_BIND_INT(check_timeperiod_id[i]);
        MYSQL_BIND_INT(notification_timeperiod_id[i]);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);

        UPDATE_QUERY_X_POS(query, cur_pos, 36, flag_isset(tmp->notification_options, OPT_WARNING));
        UPDATE_QUERY_X_POS(query, cur_pos, 38, flag_isset(tmp->notification_options, OPT_UNKNOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 40, flag_isset(tmp->notification_options, OPT_CRITICAL));
        UPDATE_QUERY_X_POS(query, cur_pos, 42, flag_isset(tmp->notification_options, OPT_RECOVERY));
        UPDATE_QUERY_X_POS(query, cur_pos, 44, flag_isset(tmp->notification_options, OPT_FLAPPING));
        UPDATE_QUERY_X_POS(query, cur_pos, 46, flag_isset(tmp->notification_options, OPT_DOWNTIME));
        UPDATE_QUERY_X_POS(query, cur_pos, 48, flag_isset(tmp->stalking_options, OPT_OK));
        UPDATE_QUERY_X_POS(query, cur_pos, 50, flag_isset(tmp->stalking_options, OPT_WARNING));
        UPDATE_QUERY_X_POS(query, cur_pos, 52, flag_isset(tmp->stalking_options, OPT_UNKNOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 54, flag_isset(tmp->stalking_options, OPT_CRITICAL));
        UPDATE_QUERY_X_POS(query, cur_pos, 56, tmp->is_volatile);
        UPDATE_QUERY_X_POS(query, cur_pos, 60, tmp->flap_detection_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 62, flag_isset(tmp->flap_detection_options, OPT_OK));
        UPDATE_QUERY_X_POS(query, cur_pos, 64, flag_isset(tmp->flap_detection_options, OPT_WARNING));
        UPDATE_QUERY_X_POS(query, cur_pos, 66, flag_isset(tmp->flap_detection_options, OPT_UNKNOWN));
        UPDATE_QUERY_X_POS(query, cur_pos, 68, flag_isset(tmp->flap_detection_options, OPT_CRITICAL));

        MYSQL_BIND_DOUBLE(tmp->low_flap_threshold);
        MYSQL_BIND_DOUBLE(tmp->high_flap_threshold);

        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->is_volatile);
        UPDATE_QUERY_X_POS(query, cur_pos, 74, tmp->flap_detection_enabled);

        MYSQL_BIND_INT(tmp->freshness_threshold);

        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->accept_passive_checks);
        UPDATE_QUERY_X_POS(query, cur_pos, 74, tmp->event_handler_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->checks_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 74, tmp->retain_status_information);
        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->retain_nonstatus_information);
        UPDATE_QUERY_X_POS(query, cur_pos, 74, tmp->notifications_enabled);
        UPDATE_QUERY_X_POS(query, cur_pos, 72, tmp->obsess);

        MYSQL_BIND_STR(tmp->notes);
        MYSQL_BIND_STR(tmp->notes_url);
        MYSQL_BIND_STR(tmp->action_url);
        MYSQL_BIND_STR(tmp->icon_image);
        MYSQL_BIND_STR(tmp->icon_image_alt);
        MYSQL_BIND_INT(tmp->hourly_value);

        i++;

        /* we need to finish the query and execute */
        if (i >= max_object_insert_count || tmp->next == NULL) {

            if (write_query == TRUE) {
                memcpy(query + query_len - 1, query_on_update, query_on_update_len);
                query_len += query_on_update_len;
            }

            if (loop == 1 || loop == loops) {
                ndo_return = mysql_stmt_prepare(ndo_stmt, query, query_len);
                ndo_return = mysql_stmt_bind_param(ndo_stmt, ndo_bind);
            }
            ndo_return = mysql_stmt_execute(ndo_stmt);

            ndo_bind_i = 0;
            i = 0;
            write_query = FALSE;

            /* if we're on the second to last loop we reset to build the final query */
            if (loop == loops - 1 && dont_reset_query == FALSE) {
                memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);
                query_len = query_base_len;
                write_query = TRUE;
            }

            loop++;
        }

        tmp = tmp->next;
    }

    ndo_return = mysql_query(mysql_connection, "UNLOCK TABLES");
    if (ndo_return != 0) {
        char msg[1024];
        snprintf(msg, 1023, "ret = %d, (%d) %s", ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(msg);
    }

    ndo_write_services_objects(config_type);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_services_objects(int config_type)
{
    trace("config_type=%d", config_type);

    service * tmp = service_list;

    servicesmember * parent = NULL;
    contactgroupsmember * group = NULL;
    contactsmember * cnt = NULL;
    customvariablesmember * var = NULL;

    int max_object_insert_count = 0;

    int parentservices_count = 0;
    char parentservices_query[MAX_SQL_BUFFER] = { 0 };
    char parentservices_query_base[] = "INSERT INTO nagios_service_parentservices (instance_id, service_id, parent_service_object_id) VALUES ";
    size_t parentservices_query_base_len = STRLIT_LEN(parentservices_query_base);
    size_t parentservices_query_len = parentservices_query_base_len;
    char parentservices_query_values[] = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),";
    size_t parentservices_query_values_len = STRLIT_LEN(parentservices_query_values);
    char parentservices_query_on_update[] = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), parent_service_object_id = VALUES(parent_service_object_id)";
    size_t parentservices_query_on_update_len = STRLIT_LEN(parentservices_query_on_update);

    int contactgroups_count = 0;
    char contactgroups_query[MAX_SQL_BUFFER] = { 0 };
    char contactgroups_query_base[] = "INSERT INTO nagios_service_contactgroups (instance_id, service_id, contactgroup_object_id) VALUES ";
    size_t contactgroups_query_base_len = STRLIT_LEN(contactgroups_query_base);
    size_t contactgroups_query_len = contactgroups_query_base_len;
    char contactgroups_query_values[] = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 11 AND name1 = ?)),";
    size_t contactgroups_query_values_len = STRLIT_LEN(contactgroups_query_values);
    char contactgroups_query_on_update[] = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), contactgroup_object_id = VALUES(contactgroup_object_id)";
    size_t contactgroups_query_on_update_len = STRLIT_LEN(contactgroups_query_on_update);

    int contacts_count = 0;
    char contacts_query[MAX_SQL_BUFFER] = { 0 };
    char contacts_query_base[] = "INSERT INTO nagios_service_contacts (instance_id, service_id, contact_object_id) VALUES ";
    size_t contacts_query_base_len = STRLIT_LEN(contacts_query_base);
    size_t contacts_query_len = contacts_query_base_len;
    char contacts_query_values[] = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?)),";
    size_t contacts_query_values_len = STRLIT_LEN(contacts_query_values);
    char contacts_query_on_update[] = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), contact_object_id = VALUES(contact_object_id)";
    size_t contacts_query_on_update_len = STRLIT_LEN(contacts_query_on_update);

    int var_count = 0;
    char var_query[MAX_SQL_BUFFER] = { 0 };
    char var_query_base[] = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = STRLIT_LEN(var_query_base);
    size_t var_query_len = var_query_base_len;
    char var_query_values[] = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?),?,?,?,?),";
    size_t var_query_values_len = STRLIT_LEN(var_query_values);
    char var_query_on_update[] = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";
    size_t var_query_on_update_len = STRLIT_LEN(var_query_on_update);

    ndo_stmt_new[WRITE_SERVICE_PARENTSERVICES] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_SERVICE_CONTACTGROUPS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_SERVICE_CONTACTS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_CUSTOMVARS] = mysql_stmt_init(mysql_connection);

    MYSQL_RESET_BIND_NEW(WRITE_SERVICE_PARENTSERVICES);
    MYSQL_RESET_BIND_NEW(WRITE_SERVICE_CONTACTGROUPS);
    MYSQL_RESET_BIND_NEW(WRITE_SERVICE_CONTACTS);
    MYSQL_RESET_BIND_NEW(WRITE_CUSTOMVARS);

    strcpy(parentservices_query, parentservices_query_base);
    strcpy(contactgroups_query, contactgroups_query_base);
    strcpy(contacts_query, contacts_query_base);
    strcpy(var_query, var_query_base);

    /* parentservices query is the longest. so if this math works for that, it works for the other queries as well */
    max_object_insert_count = ndo_max_object_insert_count;
    while ((max_object_insert_count * parentservices_query_values_len + parentservices_query_base_len + parentservices_query_on_update_len) > (MAX_SQL_BUFFER - 1)) {
        max_object_insert_count--;
    }

    while (tmp != NULL) {

        parent = tmp->parents;
        while (parent != NULL) {

            strcpy(parentservices_query + parentservices_query_len, parentservices_query_values);
            parentservices_query_len += parentservices_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, parent->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, parent->service_description);

            parent = parent->next;
            parentservices_count++;

            if (parentservices_count >= max_object_insert_count) {
                send_subquery(WRITE_SERVICE_PARENTSERVICES, &parentservices_count, parentservices_query, parentservices_query_on_update, &parentservices_query_len, parentservices_query_base_len, parentservices_query_on_update_len);
            }
        }

        group = tmp->contact_groups;
        while (group != NULL) {

            strcpy(contactgroups_query + contactgroups_query_len, contactgroups_query_values);
            contactgroups_query_len += contactgroups_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, group->group_name);

            group = group->next;
            contactgroups_count++;

            if (contactgroups_count >= max_object_insert_count) {
                send_subquery(WRITE_SERVICE_CONTACTGROUPS, &contactgroups_count, contactgroups_query, contactgroups_query_on_update, &contactgroups_query_len, contactgroups_query_base_len, contactgroups_query_on_update_len);
            }
        }

        cnt = tmp->contacts;
        while (cnt != NULL) {

            strcpy(contacts_query + contacts_query_len, contacts_query_values);
            contacts_query_len += contacts_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, cnt->contact_name);

            cnt = cnt->next;
            contacts_count++;

            if (contacts_count >= max_object_insert_count) {
                send_subquery(WRITE_SERVICE_CONTACTS, &contacts_count, contacts_query, contacts_query_on_update, &contacts_query_len, contacts_query_base_len, contacts_query_on_update_len);
            }
        }

        var = tmp->custom_variables;
        while (var != NULL) {

            strcpy(var_query + var_query_len, var_query_values);
            var_query_len += var_query_values_len;

            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->description);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, config_type);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, var->has_been_modified);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_value);

            var = var->next;
            var_count++;

            if (var_count >= max_object_insert_count) {
                send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
            }
        }

        if (parentservices_count > 0 && (parentservices_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_SERVICE_PARENTSERVICES, &parentservices_count, parentservices_query, parentservices_query_on_update, &parentservices_query_len, parentservices_query_base_len, parentservices_query_on_update_len);
        }

        if (contactgroups_count > 0 && (contactgroups_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_SERVICE_CONTACTGROUPS, &contactgroups_count, contactgroups_query, contactgroups_query_on_update, &contactgroups_query_len, contactgroups_query_base_len, contactgroups_query_on_update_len);
        }

        if (contacts_count > 0 && (contacts_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_SERVICE_CONTACTS, &contacts_count, contacts_query, contacts_query_on_update, &contacts_query_len, contacts_query_base_len, contacts_query_on_update_len);
        }

        if (var_count > 0 && (var_count >= max_object_insert_count || tmp->next == NULL)) {
            send_subquery(WRITE_CUSTOMVARS, &var_count, var_query, var_query_on_update, &var_query_len, var_query_base_len, var_query_on_update_len);
        }

        tmp = tmp->next;
    }

    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_PARENTSERVICES]);
    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_CONTACTGROUPS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_CONTACTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_servicegroups(int config_type)
{
    trace("config_type=%d", config_type);

    servicegroup * tmp = servicegroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * servicegroup_ids = NULL;

    servicegroup_ids = calloc(num_objects.servicegroups, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicegroups (instance_id, servicegroup_object_id, config_type, alias) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), servicegroup_object_id = VALUES(servicegroup_object_id), config_type = VALUES(config_type), alias = VALUES(alias)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_SERVICEGROUP, tmp->group_name);
        ndo_set_object_active(object_id, config_type, tmp->next);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        servicegroup_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    ndo_write_servicegroup_members(servicegroup_ids);

    free(servicegroup_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_servicegroup_members(int * servicegroup_ids)
{
    servicegroup * tmp = servicegroup_list;
    servicesmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicegroup_members (instance_id, servicegroup_id, service_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), servicegroup_id = VALUES(servicegroup_id), service_object_id = VALUES(service_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, member->host_name, member->service_description);

            MYSQL_BIND_INT(servicegroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostescalations(int config_type)
{
    trace("config_type=%d", config_type);

    hostescalation * tmp = NULL;
    int host_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    size_t count = (size_t) num_objects.hostescalations;
    int * object_ids = calloc(count, sizeof(int));
    int * hostescalation_ids = calloc(count, sizeof(int));

    int hostescalation_options[3] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalations (instance_id, config_type, host_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_down, escalate_on_unreachable) VALUES (1,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), timeperiod_object_id = VALUES(timeperiod_object_id), first_notification = VALUES(first_notification), last_notification = VALUES(last_notification), notification_interval = VALUES(notification_interval), escalate_on_recovery = VALUES(escalate_on_recovery), escalate_on_down = VALUES(escalate_on_down), escalate_on_unreachable = VALUES(escalate_on_unreachable)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->escalation_period);

        hostescalation_options[0] = flag_isset(tmp->escalation_options, OPT_RECOVERY);
        hostescalation_options[1] = flag_isset(tmp->escalation_options, OPT_DOWN);
        hostescalation_options[2] = flag_isset(tmp->escalation_options, OPT_UNREACHABLE);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(host_object_id);
        MYSQL_BIND_INT(timeperiod_object_id);
        MYSQL_BIND_INT(tmp->first_notification);
        MYSQL_BIND_INT(tmp->last_notification);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(hostescalation_options[0]);
        MYSQL_BIND_INT(hostescalation_options[1]);
        MYSQL_BIND_INT(hostescalation_options[2]);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        hostescalation_ids[i] = mysql_insert_id(mysql_connection);
    }

    ndo_write_hostescalation_contactgroups(hostescalation_ids);
    ndo_write_hostescalation_contacts(hostescalation_ids);

    free(object_ids);
    free(hostescalation_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostescalation_contactgroups(int * hostescalation_ids)
{
    hostescalation * tmp = NULL;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalation_contactgroups (instance_id, hostescalation_id, contactgroup_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), hostescalation_id = VALUES(hostescalation_id), contactgroup_object_id = VALUES(contactgroup_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostescalation_contacts(int * hostescalation_ids)
{
    hostescalation * tmp = NULL;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalation_contacts (instance_id, hostescalation_id, contact_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), hostescalation_id = VALUES(hostescalation_id), contact_object_id = VALUES(contact_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_serviceescalations(int config_type)
{
    trace("config_type=%d", config_type);

    serviceescalation * tmp = NULL;
    int service_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    size_t count = (size_t) num_objects.serviceescalations;
    int * object_ids = calloc(count, sizeof(int));
    int * serviceescalation_ids = calloc(count, sizeof(int));

    int serviceescalation_options[4] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalations (instance_id, config_type, service_object_id, timeperiod_object_id, first_notification, last_notification, notification_interval, escalate_on_recovery, escalate_on_warning, escalate_on_unknown, escalate_on_critical) VALUES (1,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), service_object_id = VALUES(service_object_id), timeperiod_object_id = VALUES(timeperiod_object_id), first_notification = VALUES(first_notification), last_notification = VALUES(last_notification), notification_interval = VALUES(notification_interval), escalate_on_recovery = VALUES(escalate_on_recovery), escalate_on_warning = VALUES(escalate_on_warning), escalate_on_unknown = VALUES(escalate_on_unknown), escalate_on_critical = VALUES(escalate_on_critical)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->description);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->escalation_period);

        serviceescalation_options[0] = flag_isset(tmp->escalation_options, OPT_RECOVERY);
        serviceescalation_options[1] = flag_isset(tmp->escalation_options, OPT_WARNING);
        serviceescalation_options[2] = flag_isset(tmp->escalation_options, OPT_UNKNOWN);
        serviceescalation_options[3] = flag_isset(tmp->escalation_options, OPT_CRITICAL);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(service_object_id);
        MYSQL_BIND_INT(timeperiod_object_id);
        MYSQL_BIND_INT(tmp->first_notification);
        MYSQL_BIND_INT(tmp->last_notification);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(serviceescalation_options[0]);
        MYSQL_BIND_INT(serviceescalation_options[1]);
        MYSQL_BIND_INT(serviceescalation_options[2]);
        MYSQL_BIND_INT(serviceescalation_options[3]);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        serviceescalation_ids[i] = mysql_insert_id(mysql_connection);   
    }

    ndo_write_serviceescalation_contactgroups(serviceescalation_ids);
    ndo_write_serviceescalation_contacts(serviceescalation_ids);

    free(object_ids);
    free(serviceescalation_ids);

    trace_func_end();
    return NDO_OK;
}


int ndo_write_serviceescalation_contactgroups(int * serviceescalation_ids)
{
    serviceescalation * tmp = NULL;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalation_contactgroups (instance_id, serviceescalation_id, contactgroup_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), serviceescalation_id = VALUES(serviceescalation_id), contactgroup_object_id = VALUES(contactgroup_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_serviceescalation_contacts(int * serviceescalation_ids)
{
    serviceescalation * tmp = NULL;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalation_contacts (instance_id, serviceescalation_id, contact_object_id) VALUES (1,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), serviceescalation_id = VALUES(serviceescalation_id), contact_object_id = VALUES(contact_object_id)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_hostdependencies(int config_type)
{
    trace("config_type=%d", config_type);

    hostdependency * tmp = NULL;
    int host_object_id = 0;
    int dependent_host_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    int hostdependency_options[3] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostdependencies (instance_id, config_type, host_object_id, dependent_host_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_up, fail_on_down, fail_on_unreachable) VALUES (1,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), dependent_host_object_id = VALUES(dependent_host_object_id), dependency_type = VALUES(dependency_type), inherits_parent = VALUES(inherits_parent), timeperiod_object_id = VALUES(timeperiod_object_id), fail_on_up = VALUES(fail_on_up), fail_on_down = VALUES(fail_on_down), fail_on_unreachable = VALUES(fail_on_unreachable)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.hostdependencies; i++) {

        tmp = hostdependency_ary[i];

        host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);
        dependent_host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->dependent_host_name);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->dependency_period);

        hostdependency_options[0] = flag_isset(tmp->failure_options, OPT_UP);
        hostdependency_options[1] = flag_isset(tmp->failure_options, OPT_DOWN);
        hostdependency_options[2] = flag_isset(tmp->failure_options, OPT_UNREACHABLE);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(host_object_id);
        MYSQL_BIND_INT(dependent_host_object_id);
        MYSQL_BIND_INT(tmp->dependency_type);
        MYSQL_BIND_INT(tmp->inherits_parent);
        MYSQL_BIND_INT(timeperiod_object_id);
        MYSQL_BIND_INT(hostdependency_options[0]);
        MYSQL_BIND_INT(hostdependency_options[1]);
        MYSQL_BIND_INT(hostdependency_options[2]);

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    trace_func_end();
    return NDO_OK;
}


int ndo_write_servicedependencies(int config_type)
{
    trace("config_type=%d", config_type);

    servicedependency * tmp = NULL;
    int service_object_id = 0;
    int dependent_service_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    int servicedependency_options[4] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicedependencies (instance_id, config_type, service_object_id, dependent_service_object_id, dependency_type, inherits_parent, timeperiod_object_id, fail_on_ok, fail_on_warning, fail_on_unknown, fail_on_critical) VALUES (1,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), service_object_id = VALUES(service_object_id), dependent_service_object_id = VALUES(dependent_service_object_id), dependency_type = VALUES(dependency_type), inherits_parent = VALUES(inherits_parent), timeperiod_object_id = VALUES(timeperiod_object_id), fail_on_ok = VALUES(fail_on_ok), fail_on_warning = VALUES(fail_on_warning), fail_on_unknown = VALUES(fail_on_unknown), fail_on_critical = VALUES(fail_on_critical)");

    MYSQL_PREPARE();
    MYSQL_RESET_BIND();

    for (i = 0; i < num_objects.servicedependencies; i++) {

        tmp = servicedependency_ary[i];

        service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->service_description);
        dependent_service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->dependent_host_name, tmp->dependent_service_description);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->dependency_period);

        servicedependency_options[0] = flag_isset(tmp->failure_options, OPT_OK);
        servicedependency_options[1] = flag_isset(tmp->failure_options, OPT_WARNING);
        servicedependency_options[2] = flag_isset(tmp->failure_options, OPT_UNKNOWN);
        servicedependency_options[3] = flag_isset(tmp->failure_options, OPT_CRITICAL);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(service_object_id);
        MYSQL_BIND_INT(dependent_service_object_id);
        MYSQL_BIND_INT(tmp->dependency_type);
        MYSQL_BIND_INT(tmp->inherits_parent);
        MYSQL_BIND_INT(timeperiod_object_id);
        MYSQL_BIND_INT(servicedependency_options[0]);
        MYSQL_BIND_INT(servicedependency_options[1]);
        MYSQL_BIND_INT(servicedependency_options[2]);
        MYSQL_BIND_INT(servicedependency_options[3]);
        
        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    trace_func_end();
    return NDO_OK;
}
