#define MAX_OBJECT_INSERT 10
#define MAX_SQL_BUFFER ((MAX_OBJECT_INSERT * 150) + 8000)

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

        i++;

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

    tmp = host_list;
    i = 0;

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

    int var_count = 0;
    customvariablesmember * var = NULL;
    int parenthosts_count = 0;
    hostsmember * parent = NULL;
    int contactgroups_count = 0;
    contactgroupsmember * group = NULL;
    int contacts_count = 0;
    contactsmember * cnt = NULL;

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

        i++;

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
/*
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_PARENTHOSTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTGROUPS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_HOST_CONTACTS]);
    mysql_stmt_close(ndo_stmt_new[WRITE_CUSTOMVARS]);
*/
}

void add_host(char * name)
{
    host * hst = calloc(1, sizeof(* hst));

    hst->name = name;
    hst->display_name = name;
    hst->alias = name;
    hst->address = "127.0.0.1";
    hst->check_period = "24x7";
    hst->check_period_ptr = check_period_ptr;
    hst->notification_period = "24x7";
    hst->notification_period_ptr = check_period_ptr;
    hst->check_command = "check_command!args1!args2";
    hst->check_command_ptr = cmd_ptr;
    hst->event_handler = "event_handler";
    hst->event_handler_ptr = cmd_ptr;
    hst->notes = "notes";
    hst->notes_url = "notes_url";
    hst->action_url = "action_url";
    hst->icon_image = "icon_image";
    hst->icon_image_alt = "icon_image_alt";
    hst->vrml_image = "vrml_image";
    hst->statusmap_image = "statusmap_image";
    hst->hourly_value = 0;
    hst->max_attempts = 5;
    hst->check_interval = 300;
    hst->retry_interval = 60;
    hst->notification_interval = 300;
    hst->first_notification_delay = 0;
    hst->notification_options = notification_options;
    hst->flap_detection_enabled = 0;
    hst->low_flap_threshold = 0;
    hst->high_flap_threshold = 0;
    hst->flap_detection_options = 0;
    hst->stalking_options = 0;
    hst->process_performance_data = 1;
    hst->check_freshness = 0;
    hst->freshness_threshold = 0;
    hst->checks_enabled = 1;
    hst->accept_passive_checks = 1;
    hst->event_handler_enabled = 1;
    hst->x_2d = 0;
    hst->y_2d = 0;
    hst->have_2d_coords = 1;
    hst->x_3d = 0;
    hst->y_3d = 0;
    hst->z_3d = 0;
    hst->have_3d_coords = 0;
    hst->should_be_drawn = 1;
    hst->obsess = 0;
    hst->retain_status_information = 1;
    hst->retain_nonstatus_information = 0;
    hst->current_state = 0;
    hst->last_state = 1;
    hst->last_hard_state = 0;
    hst->check_type = 0;
    hst->should_be_scheduled = 1;
    hst->current_attempt = 1;
    hst->state_type = 1;
    hst->acknowledgement_type = 0;
    hst->notifications_enabled = 1;
    hst->check_options = 1;
}

void add_check_command(char * name)
{
    check_command * cmd = calloc(1, sizeof(* cmd));

    cmd->id = 0;
    cmd->name = name;
    cmd->command_line = "command";
    cmd->next = NULL;
}

void add_timeperiod(char * name)
{
    timeperiod * tim = calloc(1, sizeof(* tim));

    tim->id = 0;
    tim->name = name;
    tim->alias = name;
    tim->timerange = NULL;
    tim->daterange = NULL;
    tim->exclusions = NULL;
    tim->next = NULL;
}