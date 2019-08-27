
#include "../include/nagios/logging.h"
#include "../include/nagios/nebstructs.h"
#include "../include/nagios/nebmodules.h"
#include "../include/nagios/nebcallbacks.h"
#include "../include/nagios/broker.h"
#include "../include/nagios/common.h"
#include "../include/nagios/nagios.h"
#include "../include/nagios/downtime.h"
#include "../include/nagios/comments.h"
#include "../include/nagios/macros.h"

#include "../include/ndo.h"
#include "../include/mysql-helpers.h"

#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <errmsg.h>

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

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiods SET instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    timeperiod_ids = calloc(count, sizeof(int));

    tmp = timeperiod_list;

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
    int notify_options_i = 0;

    commandsmember * cmd = NULL;
    int notification_type = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contacts SET instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), service_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ? ON DUPLICATE KEY UPDATE instance_id = 1, config_type = ?, contact_object_id = ?, alias = ?, email_address = ?, pager_address = ?, host_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), service_timeperiod_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1), host_notifications_enabled = ?, service_notifications_enabled = ?, can_submit_commands = ?, notify_service_recovery = ?, notify_service_warning = ?, notify_service_unknown = ?, notify_service_critical = ?, notify_service_flapping = ?, notify_service_downtime = ?, notify_host_recovery = ?, notify_host_down = ?, notify_host_unreachable = ?, notify_host_flapping = ?, notify_host_downtime = ?, minimum_importance = ?");
    MYSQL_PREPARE();

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    contact_ids = calloc(count, sizeof(int));

    tmp = contact_list;

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, tmp->name);

        contact_ids[i] = object_id;
        i++;

        MYSQL_RESET_BIND();

        notify_options_i = 0;
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_UNKNOWN);
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_WARNING);
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_CRITICAL);
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_RECOVERY);
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_FLAPPING);
        notify_options[notify_options_i++] = flag_isset(tmp->service_notification_options, OPT_DOWNTIME);
        notify_options[notify_options_i++] = flag_isset(tmp->host_notification_options, OPT_RECOVERY);
        notify_options[notify_options_i++] = flag_isset(tmp->host_notification_options, OPT_DOWN);
        notify_options[notify_options_i++] = flag_isset(tmp->host_notification_options, OPT_UNREACHABLE);
        notify_options[notify_options_i++] = flag_isset(tmp->host_notification_options, OPT_FLAPPING);
        notify_options[notify_options_i++] = flag_isset(tmp->host_notification_options, OPT_DOWNTIME);

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

        object_id = contacts_id[i];
        i++;

        for (j = 0; j < MAX_CONTACT_ADDRESS; j++) {

            MYSQL_RESET_BIND();

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(j + 1);
            MYSQL_BIND_STR(tmp->address[j]);

            MYSQL_BIND_INT(object_id);
            MYSQL_BIND_INT(j + 1);
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

        object_id = contacts_id[i];
        i++;

        /* host commands */

        cmd = tmp->host_notification_command;
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

        cmd = tmp->host_notification_command;
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
    contactgroup * tmp = contactgroups_list;
    int object_id = 0;
    int contact_object_id = 0;

    size_t count = 0;
    int * contactgroup_ids = NULL;
    int i = 0;

    contactgroupmember * member = NULL;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactgroups SET instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactgroup_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    /* get a count */
    while (tmp != NULL) {
        count++;
        tmp = tmp->next;
    }

    contactgroup_ids = calloc(count, sizeof(int));

    tmp = contactgroups_list;

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACTGROUP, tmp->name);

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

    tmp = contactgroups_list;
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

            MYSQL_BIND_INT(ndomod, object_id);
            MYSQL_BIND_INT(ndomod, contact_object_id);

            MYSQL_BIND_INT(ndomod, object_id);
            MYSQL_BIND_INT(ndomod, contact_object_id);

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

