
int ndo_set_all_objects_inactive()
{
    char * deactivate_sql = "UPDATE nagios_objects SET is_active = 0";

    ndo_return = mysql_query(mysql_connection, deactivate_sql);
    if (ndo_return != 0) {

        char err[1024] = { 0 };
        snprintf(err, 1023, "query(%s) failed with rc (%d), mysql (%d: %s)", deactivate_sql, ndo_return, mysql_errno(mysql_connection), mysql_error(mysql_connection));
        ndo_log(err);
    }

    return NDO_OK;
}


int ndo_table_genocide()
{
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

    return NDO_OK;
}


int ndo_write_active_objects()
{
    struct timeval tv;
    char msg[1024];

    gettimeofday(&tv, NULL);

    sprintf(msg, "write_active_objects begin - %ld.%06ld", tv.tv_sec, tv.tv_usec);
    ndo_log(msg);

    ndo_write_commands(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_timeperiods(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_contacts(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_contactgroups(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_hosts(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_hostgroups(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_services(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_servicegroups(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_hostescalations(NDO_CONFIG_DUMP_ORIGINAL);
    ndo_write_serviceescalations(NDO_CONFIG_DUMP_ORIGINAL);

    gettimeofday(&tv, NULL);

    sprintf(msg, "write_active_objects end   - %ld.%06ld", tv.tv_sec, tv.tv_usec);
    ndo_log(msg);

    return NDO_OK;
}


int ndo_write_config_files()
{
    return NDO_OK;
}


int ndo_write_object_config(int type)
{
    return NDO_OK;
}


int ndo_write_runtime_variables()
{
    return NDO_OK;
}


int ndo_write_commands(int config_type)
{
    command * tmp = command_list;
    int object_id = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_commands SET instance_id = 1, object_id = ?, config_type = ?, command_line = ? ON DUPLICATE KEY UPDATE instance_id = 1, object_id = ?, config_type = ?, command_line = ?");

    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, tmp->name);

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->command_line);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->command_line);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        tmp = tmp->next;
    }

    return NDO_OK;
}


int ndo_write_timeperiods(int config_type)
{
    timeperiod * tmp = timeperiod_list;
    timerange * range = NULL;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * timeperiod_ids = NULL;

    COUNT_OBJECTS(tmp, timeperiod_list, count);

    timeperiod_ids = calloc(count, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiods SET instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->name);

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

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
}


int ndo_write_timeperiod_timeranges(int * timeperiod_ids)
{
    timeperiod * tmp = timeperiod_list;
    timerange * range = NULL;
    int i = 0;

    int day = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiod_timeranges SET instance_id = 1, timeperiod_id = ?, day = ?, start_sec = ?, end_sec = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_id = ?, day = ?, start_sec = ?, end_sec = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        for (day = 0; day < 7; day++) {
            for (range = tmp->days[day]; range != NULL; range = range->next) {

                MYSQL_RESET_BIND();

                MYSQL_BIND_INT(timeperiod_ids[i]);
                MYSQL_BIND_INT(day);
                MYSQL_BIND_INT(range->range_start);
                MYSQL_BIND_INT(range->range_end);

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
}


int ndo_write_contacts(int config_type)
{
    contact * tmp = contact_list;
    int object_id = 0;
    int i = 0;

    int * object_ids = NULL;
    int * contact_ids = NULL;

    int notify_options[11] = { 0 };
    int host_timeperiod_object_id = 0;
    int service_timeperiod_object_id = 0;

    object_ids = calloc((size_t) num_objects.contacts, sizeof(int));
    contact_ids = calloc((size_t) num_objects.contacts, sizeof(int));

    MYSQL_SET_QUERY(WRITE_CONTACTS);

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contacts SET instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = ?, service_timeperiod_object_id = ?, host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = ?, service_timeperiod_object_id = ?, host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, tmp->name);
        object_ids[i] = object_id;

        host_timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->host_notification_period);
        service_timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->service_notification_period);

        MYSQL_RESET_BIND();

        notify_options[0] = flag_isset(tmp->service_notification_options, OPT_RECOVERY);
        notify_options[1] = flag_isset(tmp->service_notification_options, OPT_WARNING);
        notify_options[2] = flag_isset(tmp->service_notification_options, OPT_UNKNOWN);
        notify_options[3] = flag_isset(tmp->service_notification_options, OPT_CRITICAL);
        notify_options[4] = flag_isset(tmp->service_notification_options, OPT_FLAPPING);
        notify_options[5] = flag_isset(tmp->service_notification_options, OPT_DOWNTIME);
        notify_options[6] = flag_isset(tmp->host_notification_options, OPT_RECOVERY);
        notify_options[7] = flag_isset(tmp->host_notification_options, OPT_DOWN);
        notify_options[8] = flag_isset(tmp->host_notification_options, OPT_UNREACHABLE);
        notify_options[9] = flag_isset(tmp->host_notification_options, OPT_FLAPPING);
        notify_options[10] = flag_isset(tmp->host_notification_options, OPT_DOWNTIME);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->email);
        MYSQL_BIND_STR(tmp->pager);
        MYSQL_BIND_INT(host_timeperiod_object_id);
        MYSQL_BIND_INT(service_timeperiod_object_id);
        MYSQL_BIND_INT(tmp->host_notifications_enabled);
        MYSQL_BIND_INT(tmp->service_notifications_enabled);
        MYSQL_BIND_INT(tmp->can_submit_commands);
        MYSQL_BIND_INT(notify_options[0]);
        MYSQL_BIND_INT(notify_options[1]);
        MYSQL_BIND_INT(notify_options[2]);
        MYSQL_BIND_INT(notify_options[3]);
        MYSQL_BIND_INT(notify_options[4]);
        MYSQL_BIND_INT(notify_options[5]);
        MYSQL_BIND_INT(notify_options[6]);
        MYSQL_BIND_INT(notify_options[7]);
        MYSQL_BIND_INT(notify_options[8]);
        MYSQL_BIND_INT(notify_options[9]);
        MYSQL_BIND_INT(notify_options[10]);
        MYSQL_BIND_INT(tmp->minimum_value);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->email);
        MYSQL_BIND_STR(tmp->pager);
        MYSQL_BIND_INT(host_timeperiod_object_id);
        MYSQL_BIND_INT(service_timeperiod_object_id);
        MYSQL_BIND_INT(tmp->host_notifications_enabled);
        MYSQL_BIND_INT(tmp->service_notifications_enabled);
        MYSQL_BIND_INT(tmp->can_submit_commands);
        MYSQL_BIND_INT(notify_options[0]);
        MYSQL_BIND_INT(notify_options[1]);
        MYSQL_BIND_INT(notify_options[2]);
        MYSQL_BIND_INT(notify_options[3]);
        MYSQL_BIND_INT(notify_options[4]);
        MYSQL_BIND_INT(notify_options[5]);
        MYSQL_BIND_INT(notify_options[6]);
        MYSQL_BIND_INT(notify_options[7]);
        MYSQL_BIND_INT(notify_options[8]);
        MYSQL_BIND_INT(notify_options[9]);
        MYSQL_BIND_INT(notify_options[10]);
        MYSQL_BIND_INT(tmp->minimum_value);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        contact_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    ndo_write_contact_addresses(contact_ids);
    ndo_write_contact_notificationcommands(contact_ids);
    SAVE_CUSTOMVARIABLES(tmp, object_ids, NDO_OBJECTTYPE_CONTACT, tmp->custom_variables, i);

    free(contact_ids);
    free(object_ids);
}


int ndo_write_contact_addresses(int * contact_ids)
{
    contact * tmp = contact_list;
    int i = 0;

    int address_number = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contact_addresses SET instance_id = 1, contact_id = ?, address_number = ?, address = ? ON DUPLICATE KEY UPDATE instance_id = 1, contact_id = ?, address_number = ?, address = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        for (address_number = 1; address_number < (MAX_CONTACT_ADDRESSES + 1); address_number++) {

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(contact_ids[i]);
            MYSQL_BIND_INT(address_number);
            MYSQL_BIND_STR(tmp->address[address_number - 1]);

            MYSQL_BIND_INT(contact_ids[i]);
            MYSQL_BIND_INT(address_number);
            MYSQL_BIND_STR(tmp->address[address_number - 1]);

            MYSQL_BIND();
            MYSQL_EXECUTE();
        }

        i++;
        tmp = tmp->next;        
    }
}


int ndo_write_contact_notificationcommands(int * contact_ids)
{
    contact * tmp = contact_list;
    commandsmember * cmd = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contact_notificationcommands SET instance_id = 1, contact_id = ?, notification_type = ?, command_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, contact_id = ?, notification_type = ?, command_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        ndo_write_contact_notificationcommands_cmd(tmp->host_notification_commands, NDO_DATA_HOSTNOTIFICATIONCOMMAND, contact_ids, i);
        ndo_write_contact_notificationcommands_cmd(tmp->service_notification_commands, NDO_DATA_SERVICENOTIFICATIONCOMMAND, contact_ids, i);

        i++;
        tmp = tmp->next;        
    }
}


int ndo_write_contact_notificationcommands_cmd(commandsmember * cmd, int notification_type, int * contact_ids, int i)
{
    int object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, cmd->command);

    while (cmd != NULL) {

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(contact_ids[i]);
        MYSQL_BIND_INT(notification_type);
        MYSQL_BIND_INT(object_id);

        MYSQL_BIND_INT(contact_ids[i]);
        MYSQL_BIND_INT(notification_type);
        MYSQL_BIND_INT(object_id);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        cmd = cmd->next;
    }
}


int ndo_write_contactgroups(int config_type)
{
    contactgroup * tmp = contactgroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * contactgroup_ids = NULL;

    COUNT_OBJECTS(tmp, contactgroup_list, count);

    contactgroup_ids = calloc(count, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroups SET instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, tmp->group_name);

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

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
}


int ndo_write_contactgroup_members(int * contactgroup_ids)
{
    contactgroup * tmp = contactgroup_list;
    contactsmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroup_members SET instance_id = 1, contactgroup_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactgroup_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, member->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(contactgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(contactgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_hosts(int config_type)
{
    host * tmp = host_list;
    int i = 0;

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

    char query[MAX_SQL_BUFFER];

    char * query_base = "INSERT INTO nagios_hosts (instance_id, config_type, host_object_id, alias, display_name, address, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_down, notify_on_unreachable, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_up, stalk_on_down, stalk_on_unreachable, flap_detection_enabled, flap_detection_on_up, flap_detection_on_down, flap_detection_on_unreachable, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_host, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, vrml_image, statusmap_image, have_2d_coords, x_2d, y_2d, have_3d_coords, x_3d, y_3d, z_3d, importance) VALUES ";
    size_t query_base_len = strlen(query_base);

    char * query_values = "(1,?,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?),";

    char * query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), alias = VALUES(alias), display_name = VALUES(display_name), address = VALUES(address), check_command_object_id = VALUES(check_command_object_id), check_command_args = VALUES(check_command_args), eventhandler_command_object_id = VALUES(eventhandler_command_object_id), eventhandler_command_args = VALUES(eventhandler_command_args), check_timeperiod_object_id = VALUES(check_timeperiod_object_id), notification_timeperiod_object_id = VALUES(notification_timeperiod_object_id), failure_prediction_options = VALUES(failure_prediction_options), check_interval = VALUES(check_interval), retry_interval = VALUES(retry_interval), max_check_attempts = VALUES(max_check_attempts), first_notification_delay = VALUES(first_notification_delay), notification_interval = VALUES(notification_interval), notify_on_down = VALUES(notify_on_down), notify_on_unreachable = VALUES(notify_on_unreachable), notify_on_recovery = VALUES(notify_on_recovery), notify_on_flapping = VALUES(notify_on_flapping), notify_on_downtime = VALUES(notify_on_downtime), stalk_on_up = VALUES(stalk_on_up), stalk_on_down = VALUES(stalk_on_down), stalk_on_unreachable = VALUES(stalk_on_unreachable), flap_detection_enabled = VALUES(flap_detection_enabled), flap_detection_on_up = VALUES(flap_detection_on_up), flap_detection_on_down = VALUES(flap_detection_on_down), flap_detection_on_unreachable = VALUES(flap_detection_on_unreachable), low_flap_threshold = VALUES(low_flap_threshold), high_flap_threshold = VALUES(high_flap_threshold), process_performance_data = VALUES(process_performance_data), freshness_checks_enabled = VALUES(freshness_checks_enabled), freshness_threshold = VALUES(freshness_threshold), passive_checks_enabled = VALUES(passive_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), active_checks_enabled = VALUES(active_checks_enabled), retain_status_information = VALUES(retain_status_information), retain_nonstatus_information = VALUES(retain_nonstatus_information), notifications_enabled = VALUES(notifications_enabled), obsess_over_host = VALUES(obsess_over_host), failure_prediction_enabled = VALUES(failure_prediction_enabled), notes = VALUES(notes), notes_url = VALUES(notes_url), action_url = VALUES(action_url), icon_image = VALUES(icon_image), icon_image_alt = VALUES(icon_image_alt), vrml_image = VALUES(vrml_image), statusmap_image = VALUES(statusmap_image), have_2d_coords = VALUES(have_2d_coords), x_2d = VALUES(x_2d), y_2d = VALUES(y_2d), have_3d_coords = VALUES(have_3d_coords), x_3d = VALUES(x_3d), y_3d = VALUES(y_3d), z_3d = VALUES(z_3d), importance = VALUES(importance)";

    strcpy(query, query_base);

    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        strcat(query, query_values);

        object_ids[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->name);

        check_command[i] = strtok(tmp->check_command, "!");
        check_command_args[i] = strtok(NULL, "\0");
        check_command_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);

        event_handler[i] = strtok(tmp->event_handler, "!");
        event_handler_args[i] = strtok(NULL, "\0");
        event_handler_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);

        check_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        host_options[i][0] = flag_isset(tmp->notification_options, OPT_DOWN);
        host_options[i][1] = flag_isset(tmp->notification_options, OPT_UNREACHABLE);
        host_options[i][2] = flag_isset(tmp->notification_options, OPT_RECOVERY);
        host_options[i][3] = flag_isset(tmp->notification_options, OPT_FLAPPING);
        host_options[i][4] = flag_isset(tmp->notification_options, OPT_DOWNTIME);
        host_options[i][5] = flag_isset(tmp->stalking_options, OPT_UP);
        host_options[i][6] = flag_isset(tmp->stalking_options, OPT_DOWN);
        host_options[i][7] = flag_isset(tmp->stalking_options, OPT_UNREACHABLE);
        host_options[i][8] = flag_isset(tmp->flap_detection_options, OPT_UP);
        host_options[i][9] = flag_isset(tmp->flap_detection_options, OPT_DOWN);
        host_options[i][10] = flag_isset(tmp->flap_detection_options, OPT_UNREACHABLE);

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
        MYSQL_BIND_INT(host_options[i][0]);
        MYSQL_BIND_INT(host_options[i][1]);
        MYSQL_BIND_INT(host_options[i][2]);
        MYSQL_BIND_INT(host_options[i][3]);
        MYSQL_BIND_INT(host_options[i][4]);
        MYSQL_BIND_INT(host_options[i][5]);
        MYSQL_BIND_INT(host_options[i][6]);
        MYSQL_BIND_INT(host_options[i][7]);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(host_options[i][8]);
        MYSQL_BIND_INT(host_options[i][9]);
        MYSQL_BIND_INT(host_options[i][10]);
        MYSQL_BIND_DOUBLE(tmp->low_flap_threshold);
        MYSQL_BIND_DOUBLE(tmp->high_flap_threshold);
        MYSQL_BIND_INT(tmp->process_performance_data);
        MYSQL_BIND_INT(tmp->check_freshness);
        MYSQL_BIND_INT(tmp->freshness_threshold);
        MYSQL_BIND_INT(tmp->accept_passive_checks);
        MYSQL_BIND_INT(tmp->event_handler_enabled);
        MYSQL_BIND_INT(tmp->checks_enabled);
        MYSQL_BIND_INT(tmp->retain_status_information);
        MYSQL_BIND_INT(tmp->retain_nonstatus_information);
        MYSQL_BIND_INT(tmp->notifications_enabled);
        MYSQL_BIND_INT(tmp->obsess);
        MYSQL_BIND_STR(tmp->notes);
        MYSQL_BIND_STR(tmp->notes_url);
        MYSQL_BIND_STR(tmp->action_url);
        MYSQL_BIND_STR(tmp->icon_image);
        MYSQL_BIND_STR(tmp->icon_image_alt);
        MYSQL_BIND_STR(tmp->vrml_image);
        MYSQL_BIND_STR(tmp->statusmap_image);
        MYSQL_BIND_INT(tmp->have_2d_coords);
        MYSQL_BIND_INT(tmp->x_2d);
        MYSQL_BIND_INT(tmp->y_2d);
        MYSQL_BIND_INT(tmp->have_3d_coords);
        MYSQL_BIND_FLOAT(tmp->x_3d);
        MYSQL_BIND_FLOAT(tmp->y_3d);
        MYSQL_BIND_FLOAT(tmp->z_3d);
        MYSQL_BIND_INT(tmp->hourly_value);

        i++;

        /* we need to finish the query and execute */
        if (i >= ndo_max_object_insert_count || tmp->next == NULL) {

            strcpy(strrchr(query, ','), query_on_update);

            MYSQL_RESET_SQL();
            MYSQL_SET_SQL(query);
            MYSQL_PREPARE();

            MYSQL_BIND();
            MYSQL_EXECUTE();

            memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);

            ndo_bind_i = 0;
            i = 0;
        }

        tmp = tmp->next;
    }
}

int ndo_write_hosts_objects(int config_type)
{
    host * tmp = host_list;

    int var_count = 0;
    customvariablesmember * var = NULL;
    int parenthosts_count = 0;
    hostsmember * parent = NULL;
    int contactgroups_count = 0;
    contactgroupsmember * group = NULL;
    int contacts_count = 0;
    contactsmember * cnt = NULL;

    char parenthosts_query[MAX_SQL_BUFFER];
    char * parenthosts_query_base = "INSERT INTO nagios_host_parenthosts (instance_id, host_id, parent_host_object_id) VALUES ";
    size_t parenthosts_query_base_len = strlen(parenthosts_query_base);
    char * parenthosts_query_values = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),";
    char * parenthosts_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), parent_host_object_id = VALUES(parent_host_object_id)";

    char contactgroups_query[MAX_SQL_BUFFER];
    char * contactgroups_query_base = "INSERT INTO nagios_host_contactgroups (instance_id, host_id, contactgroup_object_id) VALUES ";
    size_t contactgroups_query_base_len = strlen(contactgroups_query_base);
    char * contactgroups_query_values = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 11 AND name1 = ?)),";
    char * contactgroups_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), contactgroup_object_id = VALUES(contactgroup_object_id)";

    char contacts_query[MAX_SQL_BUFFER];
    char * contacts_query_base = "INSERT INTO nagios_host_contacts (instance_id, host_id, contact_object_id) VALUES ";
    size_t contacts_query_base_len = strlen(contacts_query_base);
    char * contacts_query_values = "(1,(SELECT host_id FROM nagios_hosts WHERE host_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?)),";
    char * contacts_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_id = VALUES(host_id), contact_object_id = VALUES(contact_object_id)";

    char var_query[MAX_SQL_BUFFER];
    char * var_query_base = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = strlen(var_query_base);
    char * var_query_values = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?),?,?,?,?),";
    char * var_query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";

    ndo_stmt_new[WRITE_HOST_PARENTHOSTS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_HOST_CONTACTGROUPS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_HOST_CONTACTS] = mysql_stmt_init(mysql_connection);
    ndo_stmt_new[WRITE_CUSTOMVARS] = mysql_stmt_init(mysql_connection);

    MYSQL_RESET_BIND_NEW(WRITE_HOST_PARENTHOSTS);
    MYSQL_RESET_BIND_NEW(WRITE_HOST_CONTACTGROUPS);
    MYSQL_RESET_BIND_NEW(WRITE_HOST_CONTACTS);
    MYSQL_RESET_BIND_NEW(WRITE_CUSTOMVARS);

    strcpy(parenthosts_query, parenthosts_query);
    strcpy(contactgroups_query, contactgroups_query_base);
    strcpy(contacts_query, contacts_query_base);
    strcpy(var_query, var_query_base);

    while (tmp != NULL) {

        parent = tmp->parent_hosts;
        while (parent != NULL) {

            strcat(parenthosts_query, parenthosts_query_values);

            MYSQL_BIND_NEW_STR(WRITE_HOST_PARENTHOSTS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_PARENTHOSTS, parent->host_name);

            parent = parent->next;
            parenthosts_count++;

            if (parenthosts_count >= ndo_max_object_insert_count) {
                send_subquery(parenthosts_query, parenthosts_query_base_len, parenthosts_query_on_update, WRITE_HOST_PARENTHOSTS, &parenthosts_count);
            }
        }

        group = tmp->contact_groups;
        while (group != NULL) {

            strcat(contactgroups_query, contactgroups_query_values);

            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTGROUPS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTGROUPS, group->group_name);

            group = group->next;
            contactgroups_count++;

            if (contactgroups_count >= ndo_max_object_insert_count) {
                send_subquery(contactgroups_query, contactgroups_query_base_len, contactgroups_query_on_update, WRITE_HOST_CONTACTGROUPS, &contactgroups_count);
            }
        }

        cnt = tmp->contacts;
        while (cnt != NULL) {

            strcat(contacts_query, contacts_query_values);

            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTS, tmp->name);
            MYSQL_BIND_NEW_STR(WRITE_HOST_CONTACTS, cnt->contact_name);

            cnt = cnt->next;
            contacts_count++;

            if (contacts_count >= ndo_max_object_insert_count) {
                send_subquery(contacts_query, contacts_query_base_len, contacts_query_on_update, WRITE_HOST_CONTACTS, &contacts_count);
            }
        }

        var = tmp->custom_variables;
        while (var != NULL) {

            strcat(var_query, var_query_values);

            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->name);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, config_type);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, var->has_been_modified);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_value);

            var = var->next;
            var_count++;

            if (var_count >= ndo_max_object_insert_count) {
                send_subquery(var_query, var_query_base_len, var_query_on_update, WRITE_CUSTOMVARS, &var_count);
            }
        }

        if (parenthosts_count > 0 && (parenthosts_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(parenthosts_query, parenthosts_query_base_len, parenthosts_query_on_update, WRITE_HOST_PARENTHOSTS, &parenthosts_count);
        }

        if (contactgroups_count > 0 && (contactgroups_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(contactgroups_query, contactgroups_query_base_len, contactgroups_query_on_update, WRITE_HOST_CONTACTGROUPS, &contactgroups_count);
        }

        if (contacts_count > 0 && (contacts_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(contacts_query, contacts_query_base_len, contacts_query_on_update, WRITE_HOST_CONTACTS, &contacts_count);
        }

        if (var_count > 0 && (var_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(var_query, var_query_base_len, var_query_on_update, WRITE_CUSTOMVARS, &var_count);
        }

        tmp = tmp->next;
    }

    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_PARENTHOSTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTGROUPS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);
}


int send_subquery(char * query, size_t query_base_len, char * query_on_update, int stmt, int * counter)
{
    strcpy(strrchr(query, ','), query_on_update);

    MYSQL_PREPARE_NEW(stmt, query);
    MYSQL_BIND_NEW(stmt);
    MYSQL_EXECUTE_NEW(stmt);

    memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);

    *counter = 0;
    ndo_bind_new_i[stmt] = 0;
}




int ndo_write_host_parenthosts(int * host_ids)
{
    host * tmp = host_list;
    hostsmember * parent = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_host_parenthosts SET instance_id = 1, host_id = ?, parent_host_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, host_id = ?, parent_host_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        parent = tmp->parent_hosts;

        while (parent != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, parent->host_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            parent = parent->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_host_contactgroups(int * host_ids)
{
    host * tmp = host_list;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_host_contactgroups SET instance_id = 1, host_id = ?, contactgroup_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, host_id = ?, contactgroup_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_host_contacts(int * host_ids)
{
    host * tmp = host_list;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_host_contacts SET instance_id = 1, host_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, host_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(host_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_hostgroups(int config_type)
{
    hostgroup * tmp = hostgroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * hostgroup_ids = NULL;

    COUNT_OBJECTS(tmp, hostgroup_list, count);

    hostgroup_ids = calloc(count, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostgroups SET instance_id = 1, hostgroup_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, hostgroup_object_id = ?,config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOSTGROUP, tmp->group_name);

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

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
}


int ndo_write_hostgroup_members(int * hostgroup_ids)
{
    hostgroup * tmp = hostgroup_list;
    hostsmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostgroup_members SET instance_id = 1, hostgroup_id = ?, host_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, hostgroup_id = ?, host_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, member->host_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(hostgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(hostgroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_services(int config_type)
{
    service * tmp = service_list;
    int i = 0;

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

    char query[MAX_SQL_BUFFER];

    char * query_base = "INSERT INTO nagios_services (instance_id, config_type, host_object_id, service_object_id, display_name, check_command_object_id, check_command_args, eventhandler_command_object_id, eventhandler_command_args, check_timeperiod_object_id, notification_timeperiod_object_id, failure_prediction_options, check_interval, retry_interval, max_check_attempts, first_notification_delay, notification_interval, notify_on_warning, notify_on_unknown, notify_on_critical, notify_on_recovery, notify_on_flapping, notify_on_downtime, stalk_on_ok, stalk_on_warning, stalk_on_unknown, stalk_on_critical, is_volatile, flap_detection_enabled, flap_detection_on_ok, flap_detection_on_warning, flap_detection_on_unknown, flap_detection_on_critical, low_flap_threshold, high_flap_threshold, process_performance_data, freshness_checks_enabled, freshness_threshold, passive_checks_enabled, event_handler_enabled, active_checks_enabled, retain_status_information, retain_nonstatus_information, notifications_enabled, obsess_over_service, failure_prediction_enabled, notes, notes_url, action_url, icon_image, icon_image_alt, importance) VALUES ";
    size_t query_base_len = strlen(query_base);

    char * query_values = "(1,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?),";

    char * query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), config_type = VALUES(config_type), host_object_id = VALUES(host_object_id), service_object_id = VALUES(service_object_id), display_name = VALUES(display_name), check_command_object_id = VALUES(check_command_object_id), check_command_args = VALUES(check_command_args), eventhandler_command_object_id = VALUES(eventhandler_command_object_id), eventhandler_command_args = VALUES(eventhandler_command_args), check_timeperiod_object_id = VALUES(check_timeperiod_object_id), notification_timeperiod_object_id = VALUES(notification_timeperiod_object_id), failure_prediction_options = VALUES(failure_prediction_options), check_interval = VALUES(check_interval), retry_interval = VALUES(retry_interval), max_check_attempts = VALUES(max_check_attempts), first_notification_delay = VALUES(first_notification_delay), notification_interval = VALUES(notification_interval), notify_on_warning = VALUES(notify_on_warning), notify_on_unknown = VALUES(notify_on_unknown), notify_on_critical = VALUES(notify_on_critical), notify_on_recovery = VALUES(notify_on_recovery), notify_on_flapping = VALUES(notify_on_flapping), notify_on_downtime = VALUES(notify_on_downtime), stalk_on_ok = VALUES(stalk_on_ok), stalk_on_warning = VALUES(stalk_on_warning), stalk_on_unknown = VALUES(stalk_on_unknown), stalk_on_critical = VALUES(stalk_on_critical), is_volatile = VALUES(is_volatile), flap_detection_enabled = VALUES(flap_detection_enabled), flap_detection_on_ok = VALUES(flap_detection_on_ok), flap_detection_on_warning = VALUES(flap_detection_on_warning), flap_detection_on_unknown = VALUES(flap_detection_on_unknown), flap_detection_on_critical = VALUES(flap_detection_on_critical), low_flap_threshold = VALUES(low_flap_threshold), high_flap_threshold = VALUES(high_flap_threshold), process_performance_data = VALUES(process_performance_data), freshness_checks_enabled = VALUES(freshness_checks_enabled), freshness_threshold = VALUES(freshness_threshold), passive_checks_enabled = VALUES(passive_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), active_checks_enabled = VALUES(active_checks_enabled), retain_status_information = VALUES(retain_status_information), retain_nonstatus_information = VALUES(retain_nonstatus_information), notifications_enabled = VALUES(notifications_enabled), obsess_over_service = VALUES(obsess_over_service), failure_prediction_enabled = VALUES(failure_prediction_enabled), notes = VALUES(notes), notes_url = VALUES(notes_url), action_url = VALUES(action_url), icon_image = VALUES(icon_image), icon_image_alt = VALUES(icon_image_alt), importance = VALUES(importance)";

    strcpy(query, query_base);

    MYSQL_RESET_BIND();

    while (tmp != NULL) {

        strcat(query, query_values);

        object_ids[i] = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->description);
        host_object_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);

        check_command[i] = strtok(tmp->check_command, "!");
        check_command_args[i] = strtok(NULL, "\0");
        check_command_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);

        event_handler[i] = strtok(tmp->event_handler, "!");
        event_handler_args[i] = strtok(NULL, "\0");
        event_handler_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command[i]);

        check_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id[i] = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        service_options[i][0] = flag_isset(tmp->notification_options, OPT_WARNING);
        service_options[i][1] = flag_isset(tmp->notification_options, OPT_UNKNOWN);
        service_options[i][2] = flag_isset(tmp->notification_options, OPT_CRITICAL);
        service_options[i][3] = flag_isset(tmp->notification_options, OPT_RECOVERY);
        service_options[i][4] = flag_isset(tmp->notification_options, OPT_FLAPPING);
        service_options[i][5] = flag_isset(tmp->notification_options, OPT_DOWNTIME);
        service_options[i][6] = flag_isset(tmp->stalking_options, OPT_OK);
        service_options[i][7] = flag_isset(tmp->stalking_options, OPT_WARNING);
        service_options[i][8] = flag_isset(tmp->stalking_options, OPT_UNKNOWN);
        service_options[i][9] = flag_isset(tmp->stalking_options, OPT_CRITICAL);
        service_options[i][10] = flag_isset(tmp->flap_detection_options, OPT_OK);
        service_options[i][11] = flag_isset(tmp->flap_detection_options, OPT_WARNING);
        service_options[i][12] = flag_isset(tmp->flap_detection_options, OPT_UNKNOWN);
        service_options[i][13] = flag_isset(tmp->flap_detection_options, OPT_CRITICAL);

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
        MYSQL_BIND_INT(service_options[i][0]);
        MYSQL_BIND_INT(service_options[i][1]);
        MYSQL_BIND_INT(service_options[i][2]);
        MYSQL_BIND_INT(service_options[i][3]);
        MYSQL_BIND_INT(service_options[i][4]);
        MYSQL_BIND_INT(service_options[i][5]);
        MYSQL_BIND_INT(service_options[i][6]);
        MYSQL_BIND_INT(service_options[i][7]);
        MYSQL_BIND_INT(service_options[i][8]);
        MYSQL_BIND_INT(service_options[i][9]);
        MYSQL_BIND_INT(tmp->is_volatile);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(service_options[i][10]);
        MYSQL_BIND_INT(service_options[i][11]);
        MYSQL_BIND_INT(service_options[i][12]);
        MYSQL_BIND_INT(service_options[i][13]);
        MYSQL_BIND_DOUBLE(tmp->low_flap_threshold);
        MYSQL_BIND_DOUBLE(tmp->high_flap_threshold);
        MYSQL_BIND_INT(tmp->process_performance_data);
        MYSQL_BIND_INT(tmp->check_freshness);
        MYSQL_BIND_INT(tmp->freshness_threshold);
        MYSQL_BIND_INT(tmp->accept_passive_checks);
        MYSQL_BIND_INT(tmp->event_handler_enabled);
        MYSQL_BIND_INT(tmp->checks_enabled);
        MYSQL_BIND_INT(tmp->retain_status_information);
        MYSQL_BIND_INT(tmp->retain_nonstatus_information);
        MYSQL_BIND_INT(tmp->notifications_enabled);
        MYSQL_BIND_INT(tmp->obsess);
        MYSQL_BIND_STR(tmp->notes);
        MYSQL_BIND_STR(tmp->notes_url);
        MYSQL_BIND_STR(tmp->action_url);
        MYSQL_BIND_STR(tmp->icon_image);
        MYSQL_BIND_STR(tmp->icon_image_alt);
        MYSQL_BIND_INT(tmp->hourly_value);

        i++;

        /* we need to finish the query and execute */
        if (i >= ndo_max_object_insert_count || tmp->next == NULL) {

            strcpy(strrchr(query, ','), query_on_update);

            MYSQL_RESET_SQL();
            MYSQL_SET_SQL(query);
            MYSQL_PREPARE();

            MYSQL_BIND();
            MYSQL_EXECUTE();

            memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);

            ndo_bind_i = 0;
            i = 0;
        }

        tmp = tmp->next;
    }

    ndo_write_services_objects(config_type);
}


int ndo_write_services_objects(int config_type)
{
    service * tmp = service_list;

    int parentservices_count = 0;
    servicesmember * parent = NULL;
    int contactgroups_count = 0;
    contactgroupsmember * group = NULL;
    int contacts_count = 0;
    contactsmember * cnt = NULL;
    int var_count = 0;
    customvariablesmember * var = NULL;

    char parentservices_query[MAX_SQL_BUFFER];
    char * parentservices_query_base = "INSERT INTO nagios_service_parentservices (instance_id, service_id, parent_service_object_id) VALUES ";
    size_t parentservices_query_base_len = strlen(parentservices_query_base);
    char * parentservices_query_values = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),";
    char * parentservices_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), parent_service_object_id = VALUES(parent_service_object_id)";

    char contactgroups_query[MAX_SQL_BUFFER];
    char * contactgroups_query_base = "INSERT INTO nagios_service_contactgroups (instance_id, service_id, contactgroup_object_id) VALUES ";
    size_t contactgroups_query_base_len = strlen(contactgroups_query_base);
    char * contactgroups_query_values = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 11 AND name1 = ?)),";
    char * contactgroups_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), contactgroup_object_id = VALUES(contactgroup_object_id)";

    char contacts_query[MAX_SQL_BUFFER];
    char * contacts_query_base = "INSERT INTO nagios_service_contacts (instance_id, service_id, contact_object_id) VALUES ";
    size_t contacts_query_base_len = strlen(contacts_query_base);
    char * contacts_query_values = "(1,(SELECT service_id FROM nagios_services WHERE service_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?)),(SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ?)),";
    char * contacts_query_on_update = "ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_id = VALUES(service_id), contact_object_id = VALUES(contact_object_id)";

    char var_query[MAX_SQL_BUFFER];
    char * var_query_base = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = strlen(var_query_base);
    char * var_query_values = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 2 AND name1 = ? AND name2 = ?),?,?,?,?),";
    char * var_query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";

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

    while (tmp != NULL) {

        parent = tmp->parents;
        while (parent != NULL) {

            strcat(parentservices_query, parentservices_query_values);

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, parent->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_PARENTSERVICES, parent->service_description);

            parent = parent->next;
            parentservices_count++;

            if (parentservices_count >= ndo_max_object_insert_count) {
                send_subquery(parentservices_query, parentservices_query_base_len, parentservices_query_on_update, WRITE_SERVICE_PARENTSERVICES, &parentservices_count);
            }
        }

        group = tmp->contact_groups;
        while (group != NULL) {

            strcat(contactgroups_query, contactgroups_query_values);

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTGROUPS, group->group_name);

            group = group->next;
            contactgroups_count++;

            if (contactgroups_count >= ndo_max_object_insert_count) {
                send_subquery(contactgroups_query, contactgroups_query_base_len, contactgroups_query_on_update, WRITE_SERVICE_CONTACTGROUPS, &contactgroups_count);
            }
        }

        cnt = tmp->contacts;
        while (cnt != NULL) {

            strcat(contacts_query, contacts_query_values);

            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, tmp->description);
            MYSQL_BIND_NEW_STR(WRITE_SERVICE_CONTACTS, cnt->contact_name);

            cnt = cnt->next;
            contacts_count++;

            if (contacts_count >= ndo_max_object_insert_count) {
                send_subquery(contacts_query, contacts_query_base_len, contacts_query_on_update, WRITE_SERVICE_CONTACTS, &contacts_count);
            }
        }

        var = tmp->custom_variables;
        while (var != NULL) {

            strcat(var_query, var_query_values);

            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->host_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, tmp->description);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, config_type);
            MYSQL_BIND_NEW_INT(WRITE_CUSTOMVARS, var->has_been_modified);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_name);
            MYSQL_BIND_NEW_STR(WRITE_CUSTOMVARS, var->variable_value);

            var = var->next;
            var_count++;

            if (var_count >= ndo_max_object_insert_count) {
                send_subquery(var_query, var_query_base_len, var_query_on_update, WRITE_CUSTOMVARS, &var_count);
            }
        }

        if (parentservices_count > 0 && (parentservices_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(parentservices_query, parentservices_query_base_len, parentservices_query_on_update, WRITE_SERVICE_PARENTSERVICES, &parentservices_count);
        }

        if (contactgroups_count > 0 && (contactgroups_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(contactgroups_query, contactgroups_query_base_len, contactgroups_query_on_update, WRITE_SERVICE_CONTACTGROUPS, &contactgroups_count);
        }

        if (contacts_count > 0 && (contacts_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(contacts_query, contacts_query_base_len, contacts_query_on_update, WRITE_SERVICE_CONTACTS, &contacts_count);
        }

        if (var_count > 0 && (var_count >= ndo_max_object_insert_count || tmp->next == NULL)) {
            send_subquery(var_query, var_query_base_len, var_query_on_update, WRITE_CUSTOMVARS, &var_count);
        }

        tmp = tmp->next;
    }

    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_PARENTSERVICES]);
    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_CONTACTGROUPS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_SERVICE_CONTACTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);
}


int ndo_write_service_parentservices(int * service_ids)
{
    service * tmp = service_list;
    servicesmember * parent = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_service_parentservices SET instance_id = 1, service_id = ?, parent_service_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, service_id = ?, parent_service_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        parent = tmp->parents;

        while (parent != NULL) {

            object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, parent->host_name, parent->service_description);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            parent = parent->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_service_contactgroups(int * service_ids)
{
    service * tmp = service_list;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_service_contactgroups SET instance_id = 1, service_id = ?, contactgroup_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, service_id = ?, contactgroup_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_service_contacts(int * service_ids)
{
    service * tmp = service_list;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_service_contacts SET instance_id = 1, service_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, service_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(service_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_write_servicegroups(int config_type)
{
    servicegroup * tmp = servicegroup_list;
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * servicegroup_ids = NULL;

    COUNT_OBJECTS(tmp, servicegroup_list, count);

    servicegroup_ids = calloc(count, sizeof(int));

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicegroups SET instance_id = 1, servicegroup_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, servicegroup_object_id = ?,config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_SERVICEGROUP, tmp->group_name);

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

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
}


int ndo_write_servicegroup_members(int * servicegroup_ids)
{
    servicegroup * tmp = servicegroup_list;
    servicesmember * member = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicegroup_members SET instance_id = 1, servicegroup_id = ?, service_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, servicegroup_id = ?, service_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        member = tmp->members;

        while (member != NULL) {

            object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, member->host_name, member->service_description);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(servicegroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(servicegroup_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        i++;
        tmp = tmp->next;
    }
}


int ndo_save_customvariables(int object_id, int config_type, customvariablesmember * vars)
{
    customvariablesmember * tmp = vars;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_customvariables SET instance_id = 1, object_id = ?, config_type = ?, has_been_modified = ?, varname = ?, varvalue = ? ON DUPLICATE KEY UPDATE instance_id = 1, object_id = ?, config_type = ?, has_been_modified = ?, varname = ?, varvalue = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        // todo - probably don't need to reset each time for 1 prepared stmt
        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(tmp->has_been_modified);
        MYSQL_BIND_STR(tmp->variable_name);
        MYSQL_BIND_STR(tmp->variable_value);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(tmp->has_been_modified);
        MYSQL_BIND_STR(tmp->variable_name);
        MYSQL_BIND_STR(tmp->variable_value);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        tmp = tmp->next;
    }
}


int ndo_write_hostescalations(int config_type)
{
    hostescalation * tmp = NULL;
    int host_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    size_t count = (size_t) num_objects.hostescalations;
    int * object_ids = calloc(count, sizeof(int));
    int * hostescalation_ids = calloc(count, sizeof(int));

    int hostescalation_options[3] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalations SET instance_id = 1, config_type = ?, host_object_id = ?, timeperiod_object_id = ?, first_notification = ?, last_notification = ?, notification_interval = ?, escalate_on_recovery = ?, escalate_on_down = ?, escalate_on_unreachable = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, host_object_id = ?, timeperiod_object_id = ?, first_notification = ?, last_notification = ?, notification_interval = ?, escalate_on_recovery = ?, escalate_on_down = ?, escalate_on_unreachable = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->escalation_period);

        MYSQL_RESET_BIND();

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
}


int ndo_write_hostescalation_contactgroups(int * hostescalation_ids)
{
    hostescalation * tmp = NULL;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalation_contactgroups SET instance_id = 1, hostescalation_id = ?, contactgroup_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, hostescalation_id = ?, contactgroup_object_id = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }
    }
}


int ndo_write_hostescalation_contacts(int * hostescalation_ids)
{
    hostescalation * tmp = NULL;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostescalation_contacts SET instance_id = 1, hostescalation_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, hostescalation_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.hostescalations; i++) {

        tmp = hostescalation_ary[i];

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(hostescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }
    }
}



int ndo_write_serviceescalations(int config_type)
{
    serviceescalation * tmp = NULL;
    int service_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    size_t count = (size_t) num_objects.serviceescalations;
    int * object_ids = calloc(count, sizeof(int));
    int * serviceescalation_ids = calloc(count, sizeof(int));

    int serviceescalation_options[4] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalations SET instance_id = 1, config_type = ?, service_object_id = ?, timeperiod_object_id = ?, first_notification = ?, last_notification = ?, notification_interval = ?, escalate_on_recovery = ?, escalate_on_warning = ?, escalate_on_unknown = ?, escalate_on_critical = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, service_object_id = ?, timeperiod_object_id = ?, first_notification = ?, last_notification = ?, notification_interval = ?, escalate_on_recovery = ?, escalate_on_warning = ?, escalate_on_unknown = ?, escalate_on_critical = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->description);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->escalation_period);

        MYSQL_RESET_BIND();

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
}


int ndo_write_serviceescalation_contactgroups(int * serviceescalation_ids)
{
    serviceescalation * tmp = NULL;
    contactgroupsmember * group = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalation_contactgroups SET instance_id = 1, serviceescalation_id = ?, contactgroup_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, serviceescalation_id = ?, contactgroup_object_id = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        group = tmp->contact_groups;

        while (group != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, group->group_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            group = group->next;
        }
    }
}


int ndo_write_serviceescalation_contacts(int * serviceescalation_ids)
{
    serviceescalation * tmp = NULL;
    contactsmember * cnt = NULL;
    int object_id = 0;
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_serviceescalation_contacts SET instance_id = 1, serviceescalation_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, serviceescalation_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.serviceescalations; i++) {

        tmp = serviceescalation_ary[i];

        cnt = tmp->contacts;

        while (cnt != NULL) {

            object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND_INT(serviceescalation_ids[i]);
            MYSQL_BIND_INT(object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cnt = cnt->next;
        }
    }
}


int ndo_write_hostdependencies(int config_type)
{
    hostdependency * tmp = NULL;
    int host_object_id = 0;
    int dependent_host_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    int hostdependency_options[3] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hostdependencies SET instance_id = 1, config_type = ?, host_object_id = ?, dependent_host_object_id = ?, dependency_type = ?, inherits_parent = ?, timeperiod_object_id = ?, fail_on_up = ?, fail_on_down = ?, fail_on_unreachable = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, host_object_id = ?, dependent_host_object_id = ?, dependency_type = ?, inherits_parent = ?, timeperiod_object_id = ?, fail_on_up = ?, fail_on_down = ?, fail_on_unreachable = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.hostdependencies; i++) {

        tmp = hostdependency_ary[i];

        host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);
        dependent_host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->dependent_host_name);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->dependency_period);

        MYSQL_RESET_BIND();

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
}


int ndo_write_servicedependencies(int config_type)
{
    servicedependency * tmp = NULL;
    int service_object_id = 0;
    int dependent_service_object_id = 0;
    int timeperiod_object_id = 0;
    int i = 0;

    int servicedependency_options[4] = { 0 };

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_servicedependencies SET instance_id = 1, config_type = ?, service_object_id = ?, dependent_service_object_id = ?, dependency_type = ?, inherits_parent = ?, timeperiod_object_id = ?, fail_on_up = ?, fail_on_down = ?, fail_on_unreachable = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, service_object_id = ?, dependent_service_object_id = ?, dependency_type = ?, inherits_parent = ?, timeperiod_object_id = ?, fail_on_up = ?, fail_on_down = ?, fail_on_unreachable = ?");
    MYSQL_PREPARE();

    for (i = 0; i < num_objects.servicedependencies; i++) {

        tmp = servicedependency_ary[i];

        service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->service_description);
        dependent_service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->dependent_host_name, tmp->dependent_service_description);
        timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->dependency_period);

        MYSQL_RESET_BIND();

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
}
