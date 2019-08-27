
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
    ndo_write_commands();
    ndo_write_timeperiods();
    ndo_write_contacts();
    ndo_write_contactgroups();
    ndo_write_hosts();
    ndo_write_hostgroups();
    ndo_write_services();
    ndo_write_servicegroups();
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

    int day = 0;

    size_t count = 0;
    int * timeperiod_ids = NULL;
    int i = 0;

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    timeperiod_ids = calloc(count, sizeof(int));

    tmp = timeperiod_list;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiods SET instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->name);

        timeperiod_ids[i] = object_id;
        i++;

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        tmp = tmp->next;
    }

    /* now lets do the ranges */

    tmp = timeperiod_list;
    i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiod_timeranges SET instance_id = 1, timeperiod_id = ?, day = ?, start_sec = ?, end_sec = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_id = ?, day = ?, start_sec = ?, end_sec = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = timeperiod_ids[i];

        for (day = 0; day < 7; day++) {

            range = tmp->days[day];

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(day);
            MYSQL_BIND_INT(range->range_start);
            MYSQL_BIND_INT(range->range_end);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(day);
            MYSQL_BIND_INT(range->range_start);
            MYSQL_BIND_INT(range->range_end);

            MYSQL_BIND();
            MYSQL_EXECUTE();
        }

        i++;
        tmp = tmp->next;
    }

    free(timeperiod_ids);
}


int ndo_write_contacts(int config_type)
{
    contact * tmp = contact_list;
    int object_id = 0;

    size_t count = 0;
    int * contact_ids = NULL;
    int i = 0;
    int j = 0;

    int notify_options[11] = { 0 };

    commandsmember * cmd = NULL;
    int notification_type = 0;
    int address_number = 0;

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    contact_ids = calloc(count, sizeof(int));

    tmp = contact_list;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contacts SET instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), service_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), service_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, tmp->name);

        contact_ids[i] = object_id;
        i++;

        MYSQL_RESET_BIND();

        notify_options[0] = flag_isset(tmp->service_notification_options, OPT_UNKNOWN);
        notify_options[1] = flag_isset(tmp->service_notification_options, OPT_WARNING);
        notify_options[2] = flag_isset(tmp->service_notification_options, OPT_CRITICAL);
        notify_options[3] = flag_isset(tmp->service_notification_options, OPT_RECOVERY);
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
        MYSQL_BIND_STR(tmp->email);
        MYSQL_BIND_STR(tmp->host_notification_period);
        MYSQL_BIND_STR(tmp->service_notification_period);
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
        MYSQL_BIND_STR(tmp->email);
        MYSQL_BIND_STR(tmp->host_notification_period);
        MYSQL_BIND_STR(tmp->service_notification_period);
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

        tmp = tmp->next;
    }

    /* now addresses */

    tmp = contact_list;
    i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contact_addresses SET instance_id = 1, contact_id = ?, address_number = ?, address = ? ON DUPLICATE KEY UPDATE instance_id = 1, contact_id = ?, address_number = ?, address = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = contact_ids[i];
        i++;

        for (j = 0; j < MAX_CONTACT_ADDRESSES; j++) {

            address_number = j + 1;

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(address_number);
            MYSQL_BIND_STR(tmp->address[j]);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(address_number);
            MYSQL_BIND_STR(tmp->address[j]);

            MYSQL_BIND();
            MYSQL_EXECUTE();
        }

        tmp = tmp->next;        
    }

    /* now commands */

    tmp = contact_list;
    i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contact_notificationcommands SET instance_id = 1, contact_id = ?, notification_type = ?, command_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, contact_id = ?, notification_type = ?, command_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = contact_ids[i];
        i++;

        /* host commands */

        cmd = tmp->host_notification_commands;
        notification_type = NDO_DATA_HOSTNOTIFICATIONCOMMAND;

        while (cmd != NULL) {

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(notification_type);
            MYSQL_BIND_STR(cmd->command);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(notification_type);
            MYSQL_BIND_STR(cmd->command);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cmd = cmd->next;
        }

        /* service commands */

        cmd = tmp->host_notification_commands;
        notification_type = NDO_DATA_SERVICENOTIFICATIONCOMMAND;

        while (cmd != NULL) {

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(notification_type);
            MYSQL_BIND_STR(cmd->command);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(notification_type);
            MYSQL_BIND_STR(cmd->command);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            cmd = cmd->next;
        }

        tmp = tmp->next;        
    }

    /* and now custom variables */

    tmp = contact_list;
    i = 0;

    while (tmp != NULL) {

        object_id = contact_ids[i];
        i++;

        ndo_save_customvariables(object_id, NDO_OBJECTTYPE_CONTACT, tmp->custom_variables);

        tmp = tmp->next;
    }

    free(contact_ids);
}


int ndo_write_contactgroups(int config_type)
{
    contactgroup * tmp = contactgroup_list;
    int object_id = 0;
    int contact_object_id = 0;

    size_t count = 0;
    int * contactgroup_ids = NULL;
    int i = 0;

    contactsmember * member = NULL;

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    contactgroup_ids = calloc(count, sizeof(int));

    tmp = contactgroup_list;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroups SET instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, tmp->group_name);

        contactgroup_ids[i] = object_id;
        i++;

        MYSQL_RESET_BIND();

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(config_type);
        MYSQL_BIND_STR(tmp->alias);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        tmp = tmp->next;
    }

    /* members */

    tmp = contactgroup_list;
    i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroup_members SET instance_id = 1, contactgroup_id = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactgroup_id = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = contactgroup_ids[i];
        i++;

        member = tmp->members;

        while (member != NULL) {

            contact_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, member->contact_name);

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(contact_object_id);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(contact_object_id);

            MYSQL_BIND();
            MYSQL_EXECUTE();

            member = member->next;
        }

        tmp = tmp->next;
    }

    free(contactgroup_ids);
}


int ndo_write_hosts(int config_type)
{
    host * tmp = host_list;
    int object_id = 0;

    size_t count = 0;
    int * host_ids = NULL;
    int i = 0;

    int host_options[11] = { 0 };

    int check_command_id = 0;
    char * check_command = NULL;
    char * check_command_args = NULL;
    int event_handler_id = 0;
    char * event_handler = NULL;
    char * event_handler_args = NULL;
    int check_timeperiod_id = 0;
    int notification_timeperiod_id = 0;

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    MYSQL_RESET_SQL();

    /*
    instance_id='%lu', config_type='%d', host_object_id='%lu', alias='%s', display_name='%s', address='%s', check_command_object_id='%lu', check_command_args='%s', eventhandler_command_object_id='%lu', eventhandler_command_args='%s', check_timeperiod_object_id='%lu', notification_timeperiod_object_id='%lu', failure_prediction_options='%s', check_interval='%lf', retry_interval='%lf', max_check_attempts='%d', first_notification_delay='%lf', notification_interval='%lf', notify_on_down='%d', notify_on_unreachable='%d', notify_on_recovery='%d', notify_on_flapping='%d', notify_on_downtime='%d', stalk_on_up='%d', stalk_on_down='%d', stalk_on_unreachable='%d', flap_detection_enabled='%d', flap_detection_on_up='%d', flap_detection_on_down='%d', flap_detection_on_unreachable='%d', low_flap_threshold='%lf', high_flap_threshold='%lf', process_performance_data='%d', freshness_checks_enabled='%d', freshness_threshold='%d', passive_checks_enabled='%d', event_handler_enabled='%d', active_checks_enabled='%d', retain_status_information='%d', retain_nonstatus_information='%d', notifications_enabled='%d', obsess_over_host='%d', failure_prediction_enabled='%d', notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords='%d', x_2d='%d', y_2d='%d', have_3d_coords='%d', x_3d='%lf', y_3d='%lf', z_3d='%lf', importance='%d'
    */


    MYSQL_SET_SQL("INSERT INTO nagios_hosts SET instance_id = 1, config_type = ?, host_object_id = ?, alias = ?, display_name = ?, address = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_down = ?, notify_on_unreachable = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_up = ?, stalk_on_down = ?, stalk_on_unreachable = ?, flap_detection_enabled = ?, flap_detection_on_up = ?, flap_detection_on_down = ?, flap_detection_on_unreachable = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_host = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, vrml_image = ?, statusmap_image = ?, have_2d_coords = ?, x_2d = ?, y_2d = ?, have_3d_coords = ?, x_3d = ?, y_3d = ?, z_3d = ?, importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, host_object_id = ?, alias = ?, display_name = ?, address = ?, check_command_object_id = ?, check_command_args = ?, eventhandler_command_object_id = ?, eventhandler_command_args = ?, check_timeperiod_object_id = ?, notification_timeperiod_object_id = ?, failure_prediction_options = '', check_interval = ?, retry_interval = ?, max_check_attempts = ?, first_notification_delay = ?, notification_interval = ?, notify_on_down = ?, notify_on_unreachable = ?, notify_on_recovery = ?, notify_on_flapping = ?, notify_on_downtime = ?, stalk_on_up = ?, stalk_on_down = ?, stalk_on_unreachable = ?, flap_detection_enabled = ?, flap_detection_on_up = ?, flap_detection_on_down = ?, flap_detection_on_unreachable = ?, low_flap_threshold = ?, high_flap_threshold = ?, process_performance_data = ?, freshness_checks_enabled = ?, freshness_threshold = ?, passive_checks_enabled = ?, event_handler_enabled = ?, active_checks_enabled = ?, retain_status_information = ?, retain_nonstatus_information = ?, notifications_enabled = ?, obsess_over_host = ?, failure_prediction_enabled = 0, notes = ?, notes_url = ?, action_url = ?, icon_image = ?, icon_image_alt = ?, vrml_image = ?, statusmap_image = ?, have_2d_coords = ?, x_2d = ?, y_2d = ?, have_3d_coords = ?, x_3d = ?, y_3d = ?, z_3d = ?, importance = ?");
    MYSQL_PREPARE();

    host_ids = calloc(count, sizeof(int));

    tmp = host_list;

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, tmp->name);

        host_ids[i] = object_id;
        i++;

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
        host_options[5] = flag_isset(tmp->flap_detection_options, OPT_UP);
        host_options[6] = flag_isset(tmp->flap_detection_options, OPT_DOWN);
        host_options[7] = flag_isset(tmp->flap_detection_options, OPT_UNREACHABLE);
        host_options[8] = flag_isset(tmp->stalking_options, OPT_UP);
        host_options[9] = flag_isset(tmp->stalking_options, OPT_DOWN);
        host_options[10] = flag_isset(tmp->stalking_options, OPT_UNREACHABLE);

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
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(host_options[5]);
        MYSQL_BIND_INT(host_options[6]);
        MYSQL_BIND_INT(host_options[7]);
        MYSQL_BIND_INT(tmp->low_flap_threshold);
        MYSQL_BIND_INT(tmp->high_flap_threshold);
        MYSQL_BIND_INT(host_options[8]);
        MYSQL_BIND_INT(host_options[9]);
        MYSQL_BIND_INT(host_options[10]);
        MYSQL_BIND_INT(tmp->stalking_options);
        MYSQL_BIND_INT(tmp->check_freshness);
        MYSQL_BIND_INT(tmp->freshness_threshold);
        MYSQL_BIND_INT(tmp->process_performance_data);
        MYSQL_BIND_INT(tmp->checks_enabled);
        MYSQL_BIND_INT(tmp->accept_passive_checks);
        MYSQL_BIND_INT(tmp->event_handler_enabled);
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
        MYSQL_BIND_INT(tmp->flap_detection_enabled);
        MYSQL_BIND_INT(host_options[5]);
        MYSQL_BIND_INT(host_options[6]);
        MYSQL_BIND_INT(host_options[7]);
        MYSQL_BIND_INT(tmp->low_flap_threshold);
        MYSQL_BIND_INT(tmp->high_flap_threshold);
        MYSQL_BIND_INT(host_options[8]);
        MYSQL_BIND_INT(host_options[9]);
        MYSQL_BIND_INT(host_options[10]);
        MYSQL_BIND_INT(tmp->stalking_options);
        MYSQL_BIND_INT(tmp->check_freshness);
        MYSQL_BIND_INT(tmp->freshness_threshold);
        MYSQL_BIND_INT(tmp->process_performance_data);
        MYSQL_BIND_INT(tmp->checks_enabled);
        MYSQL_BIND_INT(tmp->accept_passive_checks);
        MYSQL_BIND_INT(tmp->event_handler_enabled);
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

        tmp = tmp->next;
    }
}


int ndo_write_hostgroups(int config_type)
{

}


int ndo_write_services(int config_type)
{

}


int ndo_write_servicegroups(int config_type)
{

}

int ndo_save_customvariables(int object_id, int config_type, customvariablesmember * vars)
{
    customvariablesmember * tmp = vars;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_customvariables SET instance_id = 1, object_id = ?, config_type = ?, has_been_modified = ?, varname = ?, varvalue = ? ON DUPLICATE KEY UPDATE instance_id = 1, object_id = ?, config_type = ?, has_been_modified = ?, varname = ?, varvalue = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

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
    }
}
