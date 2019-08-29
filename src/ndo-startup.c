
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

    mysql_query(mysql_connection, "LOCK TABLES nagios_commands WRITE");

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

    mysql_query(mysql_connection, "UNLOCK TABLES");

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

    size_t count = 0;
    int * object_ids = NULL;
    int * contact_ids = NULL;

    int notify_options[11] = { 0 };
    int host_timeperiod_object_id = 0;
    int service_timeperiod_object_id = 0;

    COUNT_OBJECTS(tmp, contact_list, count);

    object_ids = calloc(count, sizeof(int));
    contact_ids = calloc(count, sizeof(int));

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
    int object_id = 0;
    int i = 0;

    size_t count = 0;
    int * object_ids = NULL;
    int * host_ids = NULL;

    int host_options[11] = { 0 };
    int check_command_id = 0;
    char * check_command = NULL;
    char * check_command_args = NULL;
    int event_handler_id = 0;
    char * event_handler = NULL;
    char * event_handler_args = NULL;
    int check_timeperiod_id = 0;
    int notification_timeperiod_id = 0;

    COUNT_OBJECTS(tmp, host_list, count);

    object_ids = calloc(count, sizeof(int));
    host_ids = calloc(count, sizeof(int));

    mysql_query(mysql_connection, "LOCK TABLES nagios_hosts WRITE");

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_hosts SET instance_id = 1, config_type = ?, host_object_id = ?, alias = ?, display_name = ?, address = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_down = ?, notify_on_unreachable = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_up = ?, stalk_on_down = ?, stalk_on_unreachable = ?, flap_detection_enabled = ?, flap_detection_on_up = ?, flap_detection_on_down = ?, flap_detection_on_unreachable = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_host = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, vrml_image = ?, statusmap_image = ?, have_2d_coords = ?, x_2d = ?, y_2d = ?, have_3d_coords = ?, x_3d = ?, y_3d = ?, z_3d = ?, importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, host_object_id = ?, alias = ?, display_name = ?, address = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_down = ?, notify_on_unreachable = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_up = ?, stalk_on_down = ?, stalk_on_unreachable = ?, flap_detection_enabled = ?, flap_detection_on_up = ?, flap_detection_on_down = ?, flap_detection_on_unreachable = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_host = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, vrml_image = ?, statusmap_image = ?, have_2d_coords = ?, x_2d = ?, y_2d = ?, have_3d_coords = ?, x_3d = ?, y_3d = ?, z_3d = ?, importance = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->name);
        object_ids[i] = object_id;

        MYSQL_RESET_BIND();

        check_command = strtok(tmp->check_command, "!");
        check_command_args = strtok(NULL, "\0");
        check_command_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command);

        event_handler = strtok(tmp->event_handler, "!");
        event_handler_args = strtok(NULL, "\0");
        event_handler_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command);

        check_timeperiod_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        host_options[0] = flag_isset(tmp->notification_options, OPT_DOWN);
        host_options[1] = flag_isset(tmp->notification_options, OPT_UNREACHABLE);
        host_options[2] = flag_isset(tmp->notification_options, OPT_RECOVERY);
        host_options[3] = flag_isset(tmp->notification_options, OPT_FLAPPING);
        host_options[4] = flag_isset(tmp->notification_options, OPT_DOWNTIME);
        host_options[5] = flag_isset(tmp->stalking_options, OPT_UP);
        host_options[6] = flag_isset(tmp->stalking_options, OPT_DOWN);
        host_options[7] = flag_isset(tmp->stalking_options, OPT_UNREACHABLE);
        host_options[8] = flag_isset(tmp->flap_detection_options, OPT_UP);
        host_options[9] = flag_isset(tmp->flap_detection_options, OPT_DOWN);
        host_options[10] = flag_isset(tmp->flap_detection_options, OPT_UNREACHABLE);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->display_name);
        MYSQL_BIND_STR(tmp->address);
        MYSQL_BIND_INT(check_command_id);
        MYSQL_BIND_STR(check_command_args);
        MYSQL_BIND_INT(event_handler_id);
        MYSQL_BIND_STR(event_handler_args);
        MYSQL_BIND_INT(check_timeperiod_id);
        MYSQL_BIND_INT(notification_timeperiod_id);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(host_options[0]);
        MYSQL_BIND_INT(host_options[1]);
        MYSQL_BIND_INT(host_options[2]);
        MYSQL_BIND_INT(host_options[3]);
        MYSQL_BIND_INT(host_options[4]);
        MYSQL_BIND_INT(host_options[5]);
        MYSQL_BIND_INT(host_options[6]);
        MYSQL_BIND_INT(host_options[7]);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(host_options[8]);
        MYSQL_BIND_INT(host_options[9]);
        MYSQL_BIND_INT(host_options[10]);
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

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->alias);
        MYSQL_BIND_STR(tmp->display_name);
        MYSQL_BIND_STR(tmp->address);
        MYSQL_BIND_INT(check_command_id);
        MYSQL_BIND_STR(check_command_args);
        MYSQL_BIND_INT(event_handler_id);
        MYSQL_BIND_STR(event_handler_args);
        MYSQL_BIND_INT(check_timeperiod_id);
        MYSQL_BIND_INT(notification_timeperiod_id);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(host_options[0]);
        MYSQL_BIND_INT(host_options[1]);
        MYSQL_BIND_INT(host_options[2]);
        MYSQL_BIND_INT(host_options[3]);
        MYSQL_BIND_INT(host_options[4]);
        MYSQL_BIND_INT(host_options[5]);
        MYSQL_BIND_INT(host_options[6]);
        MYSQL_BIND_INT(host_options[7]);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(host_options[8]);
        MYSQL_BIND_INT(host_options[9]);
        MYSQL_BIND_INT(host_options[10]);
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

        MYSQL_BIND();
        MYSQL_EXECUTE();

        host_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    mysql_query(mysql_connection, "UNLOCK TABLES");

    ndo_write_host_parenthosts(host_ids);
    ndo_write_host_contactgroups(host_ids);
    ndo_write_host_contacts(host_ids);
    SAVE_CUSTOMVARIABLES(tmp, object_ids, NDO_OBJECTTYPE_HOST, tmp->custom_variables, i);

    free(host_ids);
    free(object_ids);
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
    int object_id = 0;
    int host_object_id = 0;
    int i = 0;

    size_t count = 0;
    int * object_ids = NULL;
    int * service_ids = NULL;

    int service_options[14] = { 0 };
    int check_command_id = 0;
    char * check_command = NULL;
    char * check_command_args = NULL;
    int event_handler_id = 0;
    char * event_handler = NULL;
    char * event_handler_args = NULL;
    int check_timeperiod_id = 0;
    int notification_timeperiod_id = 0;

    COUNT_OBJECTS(tmp, service_list, count);

    object_ids = calloc(count, sizeof(int));
    service_ids = calloc(count, sizeof(int));

    mysql_query(mysql_connection, "LOCK TABLES nagios_services WRITE");

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_services SET instance_id = 1, config_type = ?, host_object_id = ?, service_object_id = ?, display_name = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_warning = ?, notify_on_unknown = ?, notify_on_critical = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_ok = ?, stalk_on_warning = ?, stalk_on_unknown = ?, stalk_on_critical = ?, is_volatile = ?, flap_detection_enabled = ?, flap_detection_on_ok = ?, flap_detection_on_warning = ?, flap_detection_on_unknown = ?, flap_detection_on_critical = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_service = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, host_object_id = ?, service_object_id = ?, display_name = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_warning = ?, notify_on_unknown = ?, notify_on_critical = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_ok = ?, stalk_on_warning = ?, stalk_on_unknown = ?, stalk_on_critical = ?, is_volatile = ?, flap_detection_enabled = ?, flap_detection_on_ok = ?, flap_detection_on_warning = ?, flap_detection_on_unknown = ?, flap_detection_on_critical = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_service = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, importance = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, tmp->host_name, tmp->description);
        object_ids[i] = object_id;

        host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->host_name);

        MYSQL_RESET_BIND();

        check_command = strtok(tmp->check_command, "!");
        check_command_args = strtok(NULL, "\0");
        check_command_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command);

        event_handler = strtok(tmp->event_handler, "!");
        event_handler_args = strtok(NULL, "\0");
        event_handler_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, check_command);

        check_timeperiod_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->check_period);
        notification_timeperiod_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->notification_period);

        service_options[0] = flag_isset(tmp->notification_options, OPT_WARNING);
        service_options[1] = flag_isset(tmp->notification_options, OPT_UNKNOWN);
        service_options[2] = flag_isset(tmp->notification_options, OPT_CRITICAL);
        service_options[3] = flag_isset(tmp->notification_options, OPT_RECOVERY);
        service_options[4] = flag_isset(tmp->notification_options, OPT_FLAPPING);
        service_options[5] = flag_isset(tmp->notification_options, OPT_DOWNTIME);
        service_options[6] = flag_isset(tmp->stalking_options, OPT_OK);
        service_options[7] = flag_isset(tmp->stalking_options, OPT_WARNING);
        service_options[8] = flag_isset(tmp->stalking_options, OPT_UNKNOWN);
        service_options[9] = flag_isset(tmp->stalking_options, OPT_CRITICAL);
        service_options[10] = flag_isset(tmp->flap_detection_options, OPT_OK);
        service_options[11] = flag_isset(tmp->flap_detection_options, OPT_WARNING);
        service_options[12] = flag_isset(tmp->flap_detection_options, OPT_UNKNOWN);
        service_options[13] = flag_isset(tmp->flap_detection_options, OPT_CRITICAL);

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(host_object_id);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->description);
        MYSQL_BIND_INT(check_command_id);
        MYSQL_BIND_STR(check_command_args);
        MYSQL_BIND_INT(event_handler_id);
        MYSQL_BIND_STR(event_handler_args);
        MYSQL_BIND_INT(check_timeperiod_id);
        MYSQL_BIND_INT(notification_timeperiod_id);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(service_options[0]);
        MYSQL_BIND_INT(service_options[1]);
        MYSQL_BIND_INT(service_options[2]);
        MYSQL_BIND_INT(service_options[3]);
        MYSQL_BIND_INT(service_options[4]);
        MYSQL_BIND_INT(service_options[5]);
        MYSQL_BIND_INT(service_options[6]);
        MYSQL_BIND_INT(service_options[7]);
        MYSQL_BIND_INT(service_options[8]);
        MYSQL_BIND_INT(service_options[9]);
        MYSQL_BIND_INT(tmp->is_volatile);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(service_options[10]);
        MYSQL_BIND_INT(service_options[11]);
        MYSQL_BIND_INT(service_options[12]);
        MYSQL_BIND_INT(service_options[13]);
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

        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_INT(host_object_id);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_STR(tmp->description);
        MYSQL_BIND_INT(check_command_id);
        MYSQL_BIND_STR(check_command_args);
        MYSQL_BIND_INT(event_handler_id);
        MYSQL_BIND_STR(event_handler_args);
        MYSQL_BIND_INT(check_timeperiod_id);
        MYSQL_BIND_INT(notification_timeperiod_id);
        MYSQL_BIND_FLOAT(tmp->check_interval);
        MYSQL_BIND_FLOAT(tmp->retry_interval);
        MYSQL_BIND_INT(tmp->max_attempts);
        MYSQL_BIND_FLOAT(tmp->first_notification_delay);
        MYSQL_BIND_FLOAT(tmp->notification_interval);
        MYSQL_BIND_INT(service_options[0]);
        MYSQL_BIND_INT(service_options[1]);
        MYSQL_BIND_INT(service_options[2]);
        MYSQL_BIND_INT(service_options[3]);
        MYSQL_BIND_INT(service_options[4]);
        MYSQL_BIND_INT(service_options[5]);
        MYSQL_BIND_INT(service_options[6]);
        MYSQL_BIND_INT(service_options[7]);
        MYSQL_BIND_INT(service_options[8]);
        MYSQL_BIND_INT(service_options[9]);
        MYSQL_BIND_INT(tmp->is_volatile);
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(service_options[10]);
        MYSQL_BIND_INT(service_options[11]);
        MYSQL_BIND_INT(service_options[12]);
        MYSQL_BIND_INT(service_options[13]);
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

        MYSQL_BIND();
        MYSQL_EXECUTE();

        service_ids[i] = mysql_insert_id(mysql_connection);
        i++;
        tmp = tmp->next;
    }

    mysql_query(mysql_connection, "UNLOCK TABLES");

    ndo_write_service_parentservices(service_ids);
    ndo_write_service_contactgroups(service_ids);
    ndo_write_service_contacts(service_ids);

    SAVE_CUSTOMVARIABLES(tmp, object_ids, NDO_OBJECTTYPE_SERVICE, tmp->custom_variables, i);

    free(service_ids);
    free(object_ids);
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
