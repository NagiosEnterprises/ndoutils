
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


int ndo_write_config_file()
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

    /* if you have more than 2k timeperiods, you're doing something wrong */
    int timeperiod_ids[2048] = { 0 };
    int i = 0;

    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_timeperiods SET instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ? ON DUPLICATE KEY UPDATE instance_id = 1, timeperiod_object_id = ?, config_type = ?, alias = ?");
    MYSQL_PREPARE();

    while (tmp != NULL) {

        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tmp->name);

        /* for later lookup */
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
}


int ndo_write_contacts(int config_type)
{

}


int ndo_write_contactgroups(int config_type)
{

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

