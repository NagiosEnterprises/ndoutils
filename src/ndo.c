
/*

module:
gcc -fPIC -g -O2 -I/usr/include/mysql -I../include/nagios -o ndo.o ndo.c -shared $(mysql_config --libs)

executable:
gcc -g -O2 -I/usr/include/mysql -I../include/nagios -o ndo ndo.c $(mysql_config --libs) -DTESTING

*/

#define NSCORE 1

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

NEB_API_VERSION(CURRENT_NEB_API_VERSION)

#define NDO_OK 0
#define NDO_ERROR -1

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define NDO_REPORT(s) 

/**** NAGIOS VARIABLES ****/
extern command              * command_list;
extern timeperiod           * timeperiod_list;
extern contact              * contact_list;
extern contactgroup         * contactgroup_list;
extern host                 * host_list;
extern hostgroup            * hostgroup_list;
extern service              * service_list;
extern servicegroup         * servicegroup_list;
extern hostescalation       * hostescalation_list;
extern serviceescalation    * serviceescalation_list;
extern hostdependency       * hostdependency_list;
extern servicedependency    * servicedependency_list;

extern char                 * config_file;
extern sched_info             scheduling_info;
extern char                 * global_host_event_handler;
extern char                 * global_service_event_handler;

extern int                  __nagios_object_structure_version;

#define NDO_REPORT_ERROR(err) \
do { \
    snprintf(ndo_error_msg, 127, "%s: %s", __func__, err); \
    ndo_log(ndo_error_msg); \
} while (0)


#define NDO_HANDLE_ERROR(err) \
do { \
    if (ndo_return != 0) { \
        NDO_REPORT_ERROR(err); \
        return NDO_ERROR; \
    } \
} while (0)


/* built this for handling checking the broker data being sent, but based
   on reviewing `nagioscore/base/broker.c` - i don't think these checks
   are actually necessary. leaving it here in case someone finds it useful */
#define NDO_HANDLE_INVALID_BROKER_DATA(_ptr, _type_var, _type) \
\
do { \
\
    if (_ptr == NULL) { \
        NDO_REPORT_ERROR("Broker data pointer is null"); \
        return NDO_OK; \
    } \
\
    if (_type_var != _type) { \
        NDO_REPORT_ERROR("Broker type doesn't meet expectations"); \
        return NDO_OK; \
    } \
\
} while (0)


/**********************************************/
/* Database varibles */

#define MAX_SQL_BUFFER 4096
#define MAX_SQL_BINDINGS 64
#define MAX_SQL_RESULT_BINDINGS 16
#define MAX_BIND_BUFFER 256
#define MAX_INSERT_VALUES 250

int ndo_database_connected = FALSE;

char * ndo_db_host = NULL;
int    ndo_db_port = 3306;
char * ndo_db_socket = NULL;
char * ndo_db_user = NULL;
char * ndo_db_pass = NULL;
char * ndo_db_name = NULL;

MYSQL * mysql_connection;

MYSQL_STMT * ndo_stmt = NULL;
MYSQL_BIND ndo_bind[MAX_SQL_BINDINGS];
MYSQL_BIND ndo_result[MAX_SQL_RESULT_BINDINGS];

int ndo_return = 0;
char ndo_error_msg[128] = { 0 };
char ndo_query[MAX_SQL_BUFFER] = { 0 };
long ndo_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };
long ndo_result_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };

int ndo_bind_i = 0;
int ndo_result_i = 0;
int ndo_last_handle_function = 0;

/* 3 = objecttype_id, name1, name2 */
MYSQL_BIND ndo_object_bind[3];
MYSQL_BIND ndo_object_result[1];
MYSQL_STMT * ndo_stmt_object_get_name1 = NULL;
MYSQL_STMT * ndo_stmt_object_get_name2 = NULL;
MYSQL_STMT * ndo_stmt_object_insert_name1 = NULL;
MYSQL_STMT * ndo_stmt_object_insert_name2 = NULL;

void * ndo_handle = NULL;
int ndo_process_options = 0;

long ndo_last_notification_id = 0L;
long ndo_last_contact_notification_id = 0L;

void ndo_log(char * buffer)
{
#ifndef TESTING
    /* this is a quick way to suppress warnings about checking return status */
    if (write_to_log(buffer, NSLOG_INFO_MESSAGE, NULL) != NDO_OK) {

        /* there isn't much we can do other than shut everything down.. */
        return;
    }
#else
    printf("%s\n", buffer);
#endif
}

#ifdef TESTING
int neb_register_callback(int callback_type, void *mod_handle, int priority, int (*callback_func)(int, void *))
{
    return NDO_OK;
}

int neb_deregister_callback(int callback_type, int (*callback_func)(int, void *))
{
    return NDO_OK;
}

char * get_program_version()
{
    return "1.0.0";
}

char * get_program_modification_date()
{
    return "2019-08-20";
}
#endif



int nebmodule_init(int flags, char * args, void * handle)
{
    int result = NDO_ERROR;

    /* save handle passed from core */
    ndo_handle = handle;

    ndo_log("NDO (c) Copyright 2009-2019 Nagios - Nagios Core Development Team");

    result = ndo_process_arguments(args);
    if (result != NDO_OK) {
        return NDO_ERROR;
    }

    result = ndo_initialize_database();
    if (result != NDO_OK) {
        return NDO_ERROR;
    }

    result = ndo_register_callbacks();
    if (result != NDO_OK) {
        return NDO_ERROR;
    }

    return NDO_OK;
}


int nebmodule_deinit(int flags, int reason)
{
    ndo_deregister_callbacks();
    ndo_disconnect_database();

    free(ndo_db_user);
    free(ndo_db_pass);
    free(ndo_db_name);
    free(ndo_db_host);

    ndo_log("NDO - Shutdown complete");

    return NDO_OK;
}


int ndo_process_arguments(char * args)
{
    ndo_db_user = strdup("ndo");
    ndo_db_pass = strdup("ndo");
    ndo_db_name = strdup("ndo");

    return NDO_OK;
}


int ndo_initialize_database()
{
    /* if we've already connected, we execute a ping because we set our
       auto reconnect options appropriately */
    if (ndo_database_connected == TRUE) {

        /* todo - according to the manpage, mysql_thread_id can't be used
           on machines where the possibility exists for it to grow over
           32 bits. instead use a `SELECT CONNECTION_ID()` select statement */
        unsigned long thread_id = mysql_thread_id(mysql_connection);

        int result = mysql_ping(mysql_connection);

        /* something went wrong */
        if (result != 0) {

            char * error_msg = NULL;

            switch (result) {

            case CR_COMMANDS_OUT_OF_SYNC:
                error_msg = strdup("MYSQL ERROR: Commands out of sync");
                break;

            case CR_SERVER_GONE_ERROR:
                error_msg = strdup("MYSQL ERROR: Server has gone away");
                break;

            case CR_UNKNOWN_ERROR:
            default:
                error_msg = strdup("MYSQL ERROR: Unknown error");
                break;
            }

            ndo_log(error_msg);
            free(error_msg);

            return NDO_ERROR;
        }

        /* if we did a reconnect, we need to reinitialize the prepared
           statements - to check if reconnection occured, we check the current
           thread_id with whatever the newest is. if they differ, reconnection
           has occured */
        if (thread_id != mysql_thread_id(mysql_connection)) {

            /* TODO: the mysql c api documentation is *wonderful* and it talks
               about after a reconnect that prepared statements go away

               so i'm not sure if that means they need to be initialized again
               or if they just need the prepared statement stuff */
            return ndo_initialize_prepared_statements();
        }

        return NDO_OK;
    }

    /* this is for the actual initial connection */
    else {

        int reconnect = 1;
        MYSQL * connected = NULL;
        mysql_connection = mysql_init(NULL);

        if (mysql_connection == NULL) {
            ndo_log("Unable to initialize mysql connection");
            return NDO_ERROR;
        }

        /* without this flag set, then our mysql_ping() reconnection doesn't
           work so well [at the beginning of this function] */
        mysql_options(mysql_connection, MYSQL_OPT_RECONNECT, &reconnect);

        if (ndo_db_host == NULL) {
            ndo_db_host = strdup("localhost");
        }

        connected = mysql_real_connect(
                mysql_connection,
                ndo_db_host,
                ndo_db_user,
                ndo_db_pass,
                ndo_db_name,
                ndo_db_port,
                ndo_db_socket,
                CLIENT_REMEMBER_OPTIONS);

        if (connected == NULL) {
            ndo_log("Unable to connect to mysql. Check your configuration");
            return NDO_ERROR;
        }
    }

    ndo_database_connected = TRUE;
    return ndo_initialize_prepared_statements();
}


int ndo_initialize_prepared_statements()
{
    ndo_stmt = mysql_stmt_init(mysql_connection);

    ndo_stmt_object_get_name1 = mysql_stmt_init(mysql_connection);
    ndo_stmt_object_get_name2 = mysql_stmt_init(mysql_connection);
    ndo_stmt_object_insert_name1 = mysql_stmt_init(mysql_connection);
    ndo_stmt_object_insert_name2 = mysql_stmt_init(mysql_connection);

    if (   ndo_stmt == NULL
        || ndo_stmt_object_get_name1 == NULL
        || ndo_stmt_object_get_name2 == NULL
        || ndo_stmt_object_insert_name1 == NULL
        || ndo_stmt_object_insert_name2 == NULL) {

        ndo_log("Unable to initialize prepared statements");
    }

    /* also let's initialize the queries that we re-use constantly */

    strncpy(ndo_query, "SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 IS NULL", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_get_name1, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 = ?", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_get_name2, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "INSERT INTO nagios_objects (objecttype_id, name1) VALUES (?, ?)", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_insert_name1, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "INSERT INTO nagios_objects (objecttype_id, name1, name2) VALUES (?, ?, ?)", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_insert_name2, ndo_query, strlen(ndo_query));
    // todo


    return NDO_OK;
}


void ndo_disconnect_database()
{
    mysql_stmt_close(ndo_stmt);

    mysql_stmt_close(ndo_stmt_object_get_name1);
    mysql_stmt_close(ndo_stmt_object_get_name2);
    mysql_stmt_close(ndo_stmt_object_insert_name1);
    mysql_stmt_close(ndo_stmt_object_insert_name2);

    if (ndo_database_connected == TRUE) {
        mysql_close(mysql_connection);
    }
    mysql_library_end();
}


int ndo_register_callbacks()
{
    int result = 0;

    if (ndo_process_options & NDO_PROCESS_PROCESS) {
        result += neb_register_callback(NEBCALLBACK_PROCESS_DATA, ndo_handle, 0, ndo_handle_process);
    }
    if (ndo_process_options & NDO_PROCESS_TIMED_EVENT) {
        result += neb_register_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_handle, 0, ndo_handle_timed_event);
    }
    if (ndo_process_options & NDO_PROCESS_LOG) {
        result += neb_register_callback(NEBCALLBACK_LOG_DATA, ndo_handle, 0, ndo_handle_log);
    }
    if (ndo_process_options & NDO_PROCESS_SYSTEM_COMMAND) {
        result += neb_register_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndo_handle, 0, ndo_handle_system_command);
    }
    if (ndo_process_options & NDO_PROCESS_EVENT_HANDLER) {
        result += neb_register_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_handle, 0, ndo_handle_event_handler);
    }
    if (ndo_process_options & NDO_PROCESS_NOTIFICATION) {
        result += neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_notification);
    }
    if (ndo_process_options & NDO_PROCESS_SERVICE_CHECK) {
        result += neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle, 0, ndo_handle_service_check);
    }
    if (ndo_process_options & NDO_PROCESS_HOST_CHECK) {
        result += neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle, 0, ndo_handle_host_check);
    }
    if (ndo_process_options & NDO_PROCESS_COMMENT) {
        result += neb_register_callback(NEBCALLBACK_COMMENT_DATA, ndo_handle, 0, ndo_handle_comment);
    }
    if (ndo_process_options & NDO_PROCESS_DOWNTIME) {
        result += neb_register_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_handle, 0, ndo_handle_downtime);
    }
    if (ndo_process_options & NDO_PROCESS_FLAPPING) {
        result += neb_register_callback(NEBCALLBACK_FLAPPING_DATA, ndo_handle, 0, ndo_handle_flapping);
    }
    if (ndo_process_options & NDO_PROCESS_PROGRAM_STATUS) {
        result += neb_register_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndo_handle, 0, ndo_handle_program_status);
    }
    if (ndo_process_options & NDO_PROCESS_HOST_STATUS) {
        result += neb_register_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_handle, 0, ndo_handle_host_status);
    }
    if (ndo_process_options & NDO_PROCESS_SERVICE_STATUS) {
        result += neb_register_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_handle, 0, ndo_handle_service_status);
    }
    if (ndo_process_options & NDO_PROCESS_EXTERNAL_COMMAND) {
        result += neb_register_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndo_handle, 0, ndo_handle_external_command);
    }
    if (ndo_process_options & NDO_PROCESS_ACKNOWLEDGEMENT) {
        result += neb_register_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_handle, 0, ndo_handle_acknowledgement);
    }
    if (ndo_process_options & NDO_PROCESS_STATE_CHANGE) {
        result += neb_register_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_handle, 0, ndo_handle_state_change);
    }
    if (ndo_process_options & NDO_PROCESS_CONTACT_STATUS) {
        result += neb_register_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle, 0, ndo_handle_contact_status);
    }
    if (ndo_process_options & NDO_PROCESS_CONTACT_NOTIFICATION) {
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_contact_notification);
    }
    if (ndo_process_options & NDO_PROCESS_CONTACT_NOTIFICATION_METHOD) {
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle, 0, ndo_handle_contact_notification_method);
    }

    if (result != 0) {
        ndo_log("Something went wrong registering callbacks!");
        return NDO_ERROR;
    }

    ndo_log("Callbacks registered");
    return NDO_OK;
}


int ndo_deregister_callbacks()
{
    int result = 0;

    result += neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, ndo_handle_process);
    result += neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_handle_timed_event);
    result += neb_deregister_callback(NEBCALLBACK_LOG_DATA, ndo_handle_log);
    result += neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndo_handle_system_command);
    result += neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_handle_event_handler);
    result += neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle_notification);
    result += neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle_service_check);
    result += neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle_host_check);
    result += neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndo_handle_comment);
    result += neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_handle_downtime);
    result += neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndo_handle_flapping);
    result += neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndo_handle_program_status);
    result += neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_handle_host_status);
    result += neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_handle_service_status);
    result += neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndo_handle_external_command);
    result += neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_handle_acknowledgement);
    result += neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_handle_state_change);
    result += neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle_contact_status);
    result += neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle_contact_notification);
    result += neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle_contact_notification_method);

    if (result != 0) {
        ndo_log("Something went wrong deregistering callbacks!");
        return NDO_ERROR;
    }

    ndo_log("Callbacks deregistered");
    return NDO_OK;
}


#ifdef TESTING
int main()
{
    /* just grab some memory for the handle */
    void * handle = malloc(1);
    nebmodule_init(0, NULL, handle);

    nebmodule_deinit(0, 0);
    free(handle);

    return 0;
}
#endif


int ndo_set_all_objects_inactive()
{
    MYSQL_SET_SQL("UPDATE nagios_objects SET is_active = 0");
    MYSQL_PREPARE();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_table_genocide()
{
    char * table[] = {
        "programstatus",
        "hoststatus",
        "servicestatus",
        "contactstatus",
        "timedeventqueue",
        "comments",
        "scheduleddowntime",
        "runtimevariables",
        "customvariablestatus",
        "configfiles",
        "configfilevariables",
        "customvariables",
        "commands",
        "timeperiods",
        "timeperiodtimeranges",
        "contactgroups",
        "contactgroup_members",
        "hostgroups",
        "servicegroups",
        "servicegroupmembers",
        "hostescalations",
        "hostescalationcontacts",
        "serviceescalations",
        "serviceescalationcontacts",
        "hostdependencies",
        "servicedependencies",
        "contacts",
        "contactaddresses",
        "contactnotificationcommands",
        "hosts",
        "hostparenthosts",
        "hostcontacts",
        "services",
        "serviceparentservices",
        "servicecontacts",
        "servicecontactgroups",
        "hostcontactgroups",
        "hostescalationcontactgroups",
        "serviceescalationcontactgroups",
    };

    MYSQL_RESET_SQL();
    MYSQL_SET_SQL("TRUNCATE TABLE nagios_");

    int i = 0;

    for (i = 0; i < ARRAY_SIZE(table); i++) {

        /* 22 is the character after the `nagios_` in the query */
        strncpy(ndo_query + 22, table[i], (80 - 22 - 1));
        ndo_query[22 + strlen(table[i])] = '\0';

        MYSQL_PREPARE();
        MYSQL_EXECUTE();
    }

    return NDO_OK;
}


int ndo_handle_process(int type, void * d)
{
    nebstruct_process_data * data = d;
    char * program_version = get_program_version();
    char * program_mod_date = get_program_modification_date();
    int program_pid = (int) getpid();

    switch (data->type) {

    case NEBTYPE_PROCESS_PRELAUNCH:

        ndo_table_genocide();
        ndo_set_all_objects_inactive();

        break;


    case NEBTYPE_PROCESS_START:

        ndo_write_active_objects();
        ndo_write_config_file();
        ndo_write_object_config(NDO_CONFIG_DUMP_ORIGINAL);

        break;


    case NEBTYPE_PROCESS_EVENTLOOPSTART:

        ndo_write_runtime_variables();

        break;


    case NEBTYPE_PROCESS_SHUTDOWN:
    case NEBTYPE_PROCESS_RESTART:

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("UPDATE nagios_programstatus SET program_end_time = FROM_UNIXTIME(?), is_currently_running = 0");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->timestamp.tv_sec);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        break;

    }

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_processevents SET instance_id = 1, event_type = ?, event_time = FROM_UNIXTIME(?), event_time_usec = ?, process_id = ?, program_name = 'Nagios', program_version = ?, program_date = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->type);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->timestamp.tv_usec);
    MYSQL_BIND_INT(program_pid);
    MYSQL_BIND_STR(program_version);
    MYSQL_BIND_STR(program_mod_date);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_timed_event(int type, void * d)
{

}


int ndo_handle_log(int type, void * d)
{
    nebstruct_log_data * data = d;

    /* this particular function is a bit weird because it starts passing
       logs to the neb module before the database initialization has occured,
       so if the db hasn't been initialized, we just return */
    if (mysql_connection == NULL || ndo_database_connected != TRUE) {
        return NDO_OK;
    }

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_logentries SET instance_id = 1, logentry_time = FROM_UNIXTIME(?), entry_time = FROM_UNIXTIME(?), entry_time_usec = ?, logentry_type = ?, logentry_data = ?, realtime_data = 1, inferred_data_extracted = 1");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->entry_time);
    MYSQL_BIND_INT(data->entry_time);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->timestamp.tv_usec);
    MYSQL_BIND_INT(data->data_type);
    MYSQL_BIND_STR(data->data);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_handle_system_command(int type, void * d)
{
    nebstruct_system_command_data * data = d;

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_systemcommands SET instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, command_line = ?, timeout = ?, early_timeout = ?, execution_time = ?, return_code = ?, output = ?, long_output = ? ON DUPLICATE KEY UPDATE instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, command_line = ?, timeout = ?, early_timeout = ?, execution_time = ?, return_code = ?, output = ?, long_output = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_STR(data->command_line);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_INT(data->execution_time);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_STR(data->command_line);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_INT(data->execution_time);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_handle_event_handler(int type, void * d)
{
    nebstruct_event_handler_data * data = d;
    int object_id = 0;

    if (data->eventhandler_type == SERVICE_EVENTHANDLER || data->eventhandler_type == GLOBAL_SERVICE_EVENTHANDLER) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_eventhandlers SET instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, eventhandler_type = ?, object_id = ?, state = ?, state_type = ?, command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1), command_args = ?, command_line = ? timeout = ?, early_timeout = ?, execution_time = ?, return_code = ?, output = ?, long_output = ? ON DUPLICATE KEY UPDATE instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, eventhandler_type = ?, object_id = ?, state = ?, state_type = ?, command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1), command_args = ?, command_line = ? timeout = ?, early_timeout = ?, execution_time = ?, return_code = ?, output = ?, long_output = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(data->eventhandler_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_STR(data->command_name);
    MYSQL_BIND_STR(data->command_args);
    MYSQL_BIND_STR(data->command_line);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_INT(data->execution_time);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(data->eventhandler_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_STR(data->command_name);
    MYSQL_BIND_STR(data->command_args);
    MYSQL_BIND_STR(data->command_line);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_INT(data->execution_time);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_handle_notification(int type, void * d)
{
    nebstruct_notification_data * data = d;
    int object_id = 0;

    if (data->notification_type == SERVICE_NOTIFICATION) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_notifications SET instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, notification_type = ?, notification_reason = ?, object_id = ?, state = ?, output = ?, long_output = ?, escalated = ?, contacts_notified = ? ON DUPLICATE KEY UPDATE instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, notification_type = ?, notification_reason = ?, object_id = ?, state = ?, output = ?, long_output = ?, escalated = ?, contacts_notified = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(data->notification_type);
    MYSQL_BIND_INT(data->reason_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_INT(data->escalated);
    MYSQL_BIND_INT(data->contacts_notified);

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(data->notification_type);
    MYSQL_BIND_INT(data->reason_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_INT(data->escalated);
    MYSQL_BIND_INT(data->contacts_notified);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    /* because of the way we've changed ndo, we can no longer keep passing this
       around inside the database object.

       we can be sure that this will be the appropriate id, as the brokered
       event data for notification happens directly before each contact is
       then notified (and that data also brokered) */
    ndo_last_notification_id = mysql_insert_id(mysql_connection);

    return NDO_OK;
}


int ndo_handle_service_check(int type, void * d)
{
    nebstruct_service_check_data * data = d;
    int object_id = 0;

    /* this is the only data we care about / need */
    if (type != NEBTYPE_SERVICECHECK_PROCESSED) {
        return NDO_OK;
    }

    object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    // todo get rid of this sub query and use the object_id methods
    MYSQL_SET_SQL("INSERT INTO nagios_servicechecks SET instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, service_object_id = ?, check_type = ?, current_check_attempt = ?, max_check_attempts = ?, state = ?, state_type = ?, timeout = ?, early_timeout = ?, execution_time = ?, latency = ?, return_code = ?, output = ?, long_output = ?, perfdata = ?, command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1), command_args = ?, command_line = ? ON DUPLICATE KEY UPDATE instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, service_object_id = ?, check_type = ?, current_check_attempt = ?, max_check_attempts = ?, state = ?, state_type = ?, timeout = ?, early_timeout = ?, execution_time = ?, latency = ?, return_code = ?, output = ?, long_output = ?, perfdata = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->check_type);
    MYSQL_BIND_INT(data->current_attempt);
    MYSQL_BIND_INT(data->max_attempts);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_DOUBLE(data->execution_time);
    MYSQL_BIND_DOUBLE(data->latency);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->long_output);
    MYSQL_BIND_STR(data->perf_data);
    MYSQL_BIND_STR(data->command_name);
    MYSQL_BIND_STR(data->command_args);
    MYSQL_BIND_STR(data->command_line);

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->check_type);
    MYSQL_BIND_INT(data->current_attempt);
    MYSQL_BIND_INT(data->max_attempts);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_DOUBLE(data->execution_time);
    MYSQL_BIND_DOUBLE(data->latency);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->long_output);
    MYSQL_BIND_STR(data->perf_data);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_handle_host_check(int type, void * d)
{
    nebstruct_host_check_data * data = d;
    int object_id = 0;

    /* this is the only data we care about / need */
    if (type != NEBTYPE_HOSTCHECK_PROCESSED) {
        return NDO_OK;
    }

    object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    // todo get rid of this sub query and use the object_id methods
    MYSQL_SET_SQL("INSERT INTO nagios_hostchecks SET instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, host_object_id = ?, check_type = ?, current_check_attempt = ?, max_check_attempts = ?, state = ?, state_type = ?, timeout = ?, early_timeout = ?, execution_time = ?, latency = ?, return_code = ?, output = ?, long_output = ?, perfdata = ?, command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1), command_args = ?, command_line = ? ON DUPLICATE KEY UPDATE instance_id = 1, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, host_object_id = ?, check_type = ?, current_check_attempt = ?, max_check_attempts = ?, state = ?, state_type = ?, timeout = ?, early_timeout = ?, execution_time = ?, latency = ?, return_code = ?, output = ?, long_output = ?, perfdata = ?");

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->check_type);
    MYSQL_BIND_INT(data->current_attempt);
    MYSQL_BIND_INT(data->max_attempts);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_DOUBLE(data->execution_time);
    MYSQL_BIND_DOUBLE(data->latency);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->long_output);
    MYSQL_BIND_STR(data->perf_data);
    MYSQL_BIND_STR(data->command_name);
    MYSQL_BIND_STR(data->command_args);
    MYSQL_BIND_STR(data->command_line);

    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->check_type);
    MYSQL_BIND_INT(data->current_attempt);
    MYSQL_BIND_INT(data->max_attempts);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_INT(data->timeout);
    MYSQL_BIND_INT(data->early_timeout);
    MYSQL_BIND_DOUBLE(data->execution_time);
    MYSQL_BIND_DOUBLE(data->latency);
    MYSQL_BIND_INT(data->return_code);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->long_output);
    MYSQL_BIND_STR(data->perf_data);

    MYSQL_BIND();
    MYSQL_EXECUTE();

    return NDO_OK;
}


int ndo_handle_comment(int type, void * d)
{
    nebstruct_comment_data * data = d;
    int object_id = 0;

    if (data->comment_type == SERVICE_COMMENT) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    if (data->type == NEBTYPE_COMMENT_ADD || data->type == NEBTYPE_COMMENT_LOAD) {

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("INSERT INTO nagios_commenthistory SET instance_id = 1, comment_type = ?, entry_type = ?, object_id = ?, comment_time = FROM_UNIXTIME(?), internal_comment_id = ?, author_name = ?, comment_data = ?, is_persistent = ?, comment_source = ?, expires = ?, expiration_time = FROM_UNIXTIME(?), entry_time = FROM_UNIXTIME(?), entry_time_usec = ? ON DUPLICATE KEY UPDATE instance_id = 1, comment_type = ?, entry_type = ?, object_id = ?, comment_time = FROM_UNIXTIME(?), internal_comment_id = ?, author_name = ?, comment_data = ?, is_persistent = ?, comment_source = ?, expires = ?, expiration_time = FROM_UNIXTIME(?)");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->comment_type);
        MYSQL_BIND_INT(data->entry_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->comment_id);
        MYSQL_BIND_STR(data->author_name);
        MYSQL_BIND_STR(data->comment_data);
        MYSQL_BIND_INT(data->persistent);
        MYSQL_BIND_INT(data->source);
        MYSQL_BIND_INT(data->expires);
        MYSQL_BIND_INT(data->expire_time);
        MYSQL_BIND_INT(data->timestamp.tv_sec);
        MYSQL_BIND_INT(data->timestamp.tv_usec);

        MYSQL_BIND_INT(data->comment_type);
        MYSQL_BIND_INT(data->entry_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->comment_id);
        MYSQL_BIND_STR(data->author_name);
        MYSQL_BIND_STR(data->comment_data);
        MYSQL_BIND_INT(data->persistent);
        MYSQL_BIND_INT(data->source);
        MYSQL_BIND_INT(data->expires);
        MYSQL_BIND_INT(data->expire_time);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        /* we just have to rename "commenthistory" to "comments"

            strlen("INSERT INTO nagios_comments      ") = 33

                           "INSERT INTO nagios_commenthistory" */
        strncpy(ndo_query, "INSERT INTO nagios_comments      ", 33);

        MYSQL_PREPARE();

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    if (data->type == NEBTYPE_COMMENT_DELETE) {

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("UPDATE nagios_commenthistory SET deletion_time = FROM_UNIXTIME(?), deletion_time_usec = ? WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->timestamp.tv_sec);
        MYSQL_BIND_INT(data->timestamp.tv_usec);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->comment_id);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        /** now we actually delete it **/

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("DELETE FROM nagios_comments WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");

        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->comment_id);

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    return NDO_OK;
}


int ndo_handle_downtime(int type, void * d)
{
    nebstruct_downtime_data * data = d;
    int object_id = 0;

    if (data->downtime_type == SERVICE_DOWNTIME) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    if (data->type == NEBTYPE_DOWNTIME_ADD || data->type == NEBTYPE_DOWNTIME_LOAD) {

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("INSERT INTO nagios_scheduleddowntime SET instance_id = 1, downtime_type = ?, object_id = ?, entry_time = FROM_UNIXTIME(?), author_name = ?, comment_data = ?, internal_downtime_id = ?, triggered_by_id = ?, is_fixed = ?, duration = ?, scheduled_start_time = ?, scheduled_end_time = ? ON DUPLICATE KEY UPDATE instance_id = 1, downtime_type = ?, object_id = ?, entry_time = FROM_UNIXTIME(?), author_name = ?, comment_data = ?, internal_downtime_id = ?, triggered_by_id = ?, is_fixed = ?, duration = ?, scheduled_start_time = ?, scheduled_end_time = ?");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->downtime_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_STR(data->author_name);
        MYSQL_BIND_STR(data->comment_data);
        MYSQL_BIND_INT(data->downtime_id);
        MYSQL_BIND_INT(data->triggered_by);
        MYSQL_BIND_INT(data->fixed);
        MYSQL_BIND_INT(data->duration);
        MYSQL_BIND_INT(data->start_time);
        MYSQL_BIND_INT(data->end_time);

        MYSQL_BIND_INT(data->downtime_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_STR(data->author_name);
        MYSQL_BIND_STR(data->comment_data);
        MYSQL_BIND_INT(data->downtime_id);
        MYSQL_BIND_INT(data->triggered_by);
        MYSQL_BIND_INT(data->fixed);
        MYSQL_BIND_INT(data->duration);
        MYSQL_BIND_INT(data->start_time);
        MYSQL_BIND_INT(data->end_time);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        /* we just have to rename "scheduleddowntime" to "downtimehistory"

           strlen("INSERT INTO nagios_downtimehistory      ") = 36

                           "INSERT INTO nagios_scheduleddowntime" */
        strncpy(ndo_query, "INSERT INTO nagios_downtimehistory  ", 36);

        MYSQL_PREPARE();

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    if (data->type == NEBTYPE_DOWNTIME_START) {

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("UPDATE nagios_scheduleddowntime SET actual_start_time = ?, actual_start_time_usec = ?, was_started = 1 WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = ? AND scheduled_end_time = ?");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->timestamp.tv_sec);
        MYSQL_BIND_INT(data->timestamp.tv_usec);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->downtime_type);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->start_time);
        MYSQL_BIND_INT(data->end_time);

        MYSQL_BIND();
        MYSQL_EXECUTE();

        /* we just have to rename "scheduleddowntime" to "downtimehistory"

            strlen("UPDATE nagios_downtimehistory") = 29

                           "UPDATE nagios_scheduleddowntime" */
        strncpy(ndo_query, "UPDATE nagios_downtimehistory  ", 29);

        MYSQL_PREPARE();

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    if (data->type == NEBTYPE_DOWNTIME_STOP) {

        int cancelled = 0;

        if (data->attr == NEBATTR_DOWNTIME_STOP_CANCELLED) {
            cancelled = 1;
        }

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("UPDATE nagios_downtimehistory SET actual_end_time = ?, actual_end_time_usec = ?, was_cancelled = ? WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = ? AND scheduled_end_time = ?");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->timestamp.tv_sec);
        MYSQL_BIND_INT(data->timestamp.tv_usec);
        MYSQL_BIND_INT(cancelled);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->downtime_type);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->start_time);
        MYSQL_BIND_INT(data->end_time);

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }

    if (data->type == NEBTYPE_DOWNTIME_STOP || data->type == NEBTYPE_DOWNTIME_DELETE) {

        MYSQL_RESET_SQL();
        MYSQL_RESET_BIND();

        MYSQL_SET_SQL("DELETE FROM nagios_scheduleddowntime WHERE downtime_type = ? AND object_id = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
        MYSQL_PREPARE();

        MYSQL_BIND_INT(data->downtime_type);
        MYSQL_BIND_INT(object_id);
        MYSQL_BIND_INT(data->entry_time);
        MYSQL_BIND_INT(data->start_time);
        MYSQL_BIND_INT(data->end_time);

        MYSQL_BIND();
        MYSQL_EXECUTE();
    }
}


int ndo_handle_flapping(int type, void * d)
{

}


int ndo_handle_program_status(int type, void * d)
{
    nebstruct_program_status_data * data = d;

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_programstatus SET instance_id = 1, status_update_time = ?, program_start_time = FROM_UNIXTIME(?), is_currently_running = 1, process_id = ?, daemon_mode = ?, last_command_check = FROM_UNIXTIME(0), last_log_rotation = FROM_UNIXTIME(?), notifications_enabled = ?, active_service_checks_enabled = ?, passive_service_checks_enabled = ?, active_host_checks_enabled = ?, passive_host_checks_enabled = ?, event_handlers_enabled = ?, flap_detection_enabled = ?, failure_prediction_enabled = ?, process_performance_data = ?, obsess_over_hosts = ?, obsess_over_services = ?, modified_host_attributes = ?, modified_service_attributes = ?, global_host_event_handler = ?, global_service_event_handler = ? ON DUPLICATE KEY UPDATE instance_id = 1, status_update_time = ?, program_start_time = FROM_UNIXTIME(?), is_currently_running = 1, process_id = ?, daemon_mode = ?, last_command_check = FROM_UNIXTIME(0), last_log_rotation = FROM_UNIXTIME(?), notifications_enabled = ?, active_service_checks_enabled = ?, passive_service_checks_enabled = ?, active_host_checks_enabled = ?, passive_host_checks_enabled = ?, event_handlers_enabled = ?, flap_detection_enabled = ?, failure_prediction_enabled = ?, process_performance_data = ?, obsess_over_hosts = ?, obsess_over_services = ?, modified_host_attributes = ?, modified_service_attributes = ?, global_host_event_handler = ?, global_service_event_handler = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->program_start);
    MYSQL_BIND_INT(data->pid);
    MYSQL_BIND_INT(data->daemon_mode);
    MYSQL_BIND_INT(data->last_log_rotation);
    MYSQL_BIND_INT(data->notifications_enabled);
    MYSQL_BIND_INT(data->active_service_checks_enabled);
    MYSQL_BIND_INT(data->passive_service_checks_enabled);
    MYSQL_BIND_INT(data->active_host_checks_enabled);
    MYSQL_BIND_INT(data->passive_host_checks_enabled);
    MYSQL_BIND_INT(data->event_handlers_enabled);
    MYSQL_BIND_INT(data->flap_detection_enabled);
    MYSQL_BIND_INT(data->process_performance_data);
    MYSQL_BIND_INT(data->obsess_over_hosts);
    MYSQL_BIND_INT(data->obsess_over_services);
    MYSQL_BIND_INT(data->modified_host_attributes);
    MYSQL_BIND_INT(data->modified_service_attributes);
    MYSQL_BIND_STR(data->global_host_event_handler);
    MYSQL_BIND_STR(data->global_service_event_handler);

    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->program_start);
    MYSQL_BIND_INT(data->pid);
    MYSQL_BIND_INT(data->daemon_mode);
    MYSQL_BIND_INT(data->last_log_rotation);
    MYSQL_BIND_INT(data->notifications_enabled);
    MYSQL_BIND_INT(data->active_service_checks_enabled);
    MYSQL_BIND_INT(data->passive_service_checks_enabled);
    MYSQL_BIND_INT(data->active_host_checks_enabled);
    MYSQL_BIND_INT(data->passive_host_checks_enabled);
    MYSQL_BIND_INT(data->event_handlers_enabled);
    MYSQL_BIND_INT(data->flap_detection_enabled);
    MYSQL_BIND_INT(data->process_performance_data);
    MYSQL_BIND_INT(data->obsess_over_hosts);
    MYSQL_BIND_INT(data->obsess_over_services);
    MYSQL_BIND_INT(data->modified_host_attributes);
    MYSQL_BIND_INT(data->modified_service_attributes);
    MYSQL_BIND_STR(data->global_host_event_handler);
    MYSQL_BIND_STR(data->global_service_event_handler);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_host_status(int type, void * d)
{
    nebstruct_host_status_data * data = d;
    int host_object_id        = 0;
    int timeperiod_object_id  = 0;
    host * hst                = NULL;
    timeperiod * tm           = NULL; 
    char * event_handler_name = "";
    char * check_command_name = "";

    if (   data->object_ptr == NULL 
        || ((host *) data->object_ptr)->event_handler_ptr == NULL
        || ((host *) data->object_ptr)->check_command_ptr == NULL
        || ((host *) data->object_ptr)->event_handler_ptr->name == NULL
        || ((host *) data->object_ptr)->check_command_ptr->name == NULL) {

        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    hst = data->object_ptr;
    tm = hst->check_period_ptr;
    event_handler_name = hst->event_handler_ptr->name;
    check_command_name = hst->check_command_ptr->name;

    host_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, hst->name);
    timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tm->name);

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_hoststatus SET instance_id = 1, host_object_id = ?, status_update_time = FROM_UNIXTIME(?), output = ?, long_output = ?, perfdata = ?, current_state = ?, has_been_checked = ?, should_be_scheduled = ?, current_check_attempt = ?, max_check_attempts = ?, last_check = FROM_UNIXTIME(?), next_check = FROM_UNIXTIME(?), check_type = ?, last_state_change = FROM_UNIXTIME(?), last_hard_state_change = FROM_UNIXTIME(?), last_hard_state = ?, last_time_up = FROM_UNIXTIME(?), last_time_down = FROM_UNIXTIME(?), last_time_unreachable = FROM_UNIXTIME(?), state_type = ?, last_notification = FROM_UNIXTIME(?), next_notification = FROM_UNIXTIME(?), no_more_notifications = ?, notifications_enabled = ?, problem_has_been_acknowledged = ?, acknowledgement_type = ?, current_notification_number = ?, passive_checks_enabled = ?, active_checks_enabled = ?, event_handler_enabled = ?, flap_detection_enabled = ?, is_flapping = ?, percent_state_change = ?, latency = ?, execution_time = ?, scheduled_downtime_depth = ?, failure_prediction_enabled = 0, process_performance_data = ?, obsess_over_host = ?, modified_host_attributes = ?, event_handler = ?, check_command = ?, normal_check_interval = ?, retry_check_interval = ?, check_timeperiod_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, host_object_id = ?, status_update_time = FROM_UNIXTIME(?), output = ?, long_output = ?, perfdata = ?, current_state = ?, has_been_checked = ?, should_be_scheduled = ?, current_check_attempt = ?, max_check_attempts = ?, last_check = FROM_UNIXTIME(?), next_check = FROM_UNIXTIME(?), check_type = ?, last_state_change = FROM_UNIXTIME(?), last_hard_state_change = FROM_UNIXTIME(?), last_hard_state = ?, last_time_up = FROM_UNIXTIME(?), last_time_down = FROM_UNIXTIME(?), last_time_unreachable = FROM_UNIXTIME(?), state_type = ?, last_notification = FROM_UNIXTIME(?), next_notification = FROM_UNIXTIME(?), no_more_notifications = ?, notifications_enabled = ?, problem_has_been_acknowledged = ?, acknowledgement_type = ?, current_notification_number = ?, passive_checks_enabled = ?, active_checks_enabled = ?, event_handler_enabled = ?, flap_detection_enabled = ?, is_flapping = ?, percent_state_change = ?, latency = ?, execution_time = ?, scheduled_downtime_depth = ?, failure_prediction_enabled = 0, process_performance_data = ?, obsess_over_host = ?, modified_host_attributes = ?, event_handler = ?, check_command = ?, normal_check_interval = ?, retry_check_interval = ?, check_timeperiod_object_id = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(host_object_id);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_STR(hst->plugin_output);
    MYSQL_BIND_STR(hst->long_plugin_output);
    MYSQL_BIND_STR(hst->perf_data);
    MYSQL_BIND_INT(hst->current_state);
    MYSQL_BIND_INT(hst->has_been_checked);
    MYSQL_BIND_INT(hst->should_be_scheduled);
    MYSQL_BIND_INT(hst->current_attempt);
    MYSQL_BIND_INT(hst->max_attempts);
    MYSQL_BIND_INT(hst->last_check);
    MYSQL_BIND_INT(hst->next_check);
    MYSQL_BIND_INT(hst->check_type);
    MYSQL_BIND_INT(hst->last_state_change);
    MYSQL_BIND_INT(hst->last_hard_state_change);
    MYSQL_BIND_INT(hst->last_hard_state);
    MYSQL_BIND_INT(hst->last_time_up);
    MYSQL_BIND_INT(hst->last_time_down);
    MYSQL_BIND_INT(hst->last_time_unreachable);
    MYSQL_BIND_INT(hst->state_type);
    MYSQL_BIND_INT(hst->last_notification);
    MYSQL_BIND_INT(hst->next_notification);
    MYSQL_BIND_INT(hst->no_more_notifications);
    MYSQL_BIND_INT(hst->notifications_enabled);
    MYSQL_BIND_INT(hst->problem_has_been_acknowledged);
    MYSQL_BIND_INT(hst->acknowledgement_type);
    MYSQL_BIND_INT(hst->current_notification_number);
    MYSQL_BIND_INT(hst->accept_passive_checks);
    MYSQL_BIND_INT(hst->checks_enabled);
    MYSQL_BIND_INT(hst->event_handler_enabled);
    MYSQL_BIND_INT(hst->flap_detection_enabled);
    MYSQL_BIND_INT(hst->is_flapping);
    MYSQL_BIND_DOUBLE(hst->percent_state_change);
    MYSQL_BIND_DOUBLE(hst->latency);
    MYSQL_BIND_DOUBLE(hst->execution_time);
    MYSQL_BIND_INT(hst->scheduled_downtime_depth);
    MYSQL_BIND_INT(hst->process_performance_data);
    MYSQL_BIND_INT(hst->obsess);
    MYSQL_BIND_INT(hst->modified_attributes);
    MYSQL_BIND_STR(event_handler_name);
    MYSQL_BIND_STR(check_command_name);
    MYSQL_BIND_DOUBLE(hst->check_interval);
    MYSQL_BIND_DOUBLE(hst->retry_interval);
    MYSQL_BIND_INT(timeperiod_object_id);

    MYSQL_BIND_INT(host_object_id);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_STR(hst->plugin_output);
    MYSQL_BIND_STR(hst->long_plugin_output);
    MYSQL_BIND_STR(hst->perf_data);
    MYSQL_BIND_INT(hst->current_state);
    MYSQL_BIND_INT(hst->should_be_scheduled);
    MYSQL_BIND_INT(hst->has_been_checked);
    MYSQL_BIND_INT(hst->current_attempt);
    MYSQL_BIND_INT(hst->max_attempts);
    MYSQL_BIND_INT(hst->last_check);
    MYSQL_BIND_INT(hst->next_check);
    MYSQL_BIND_INT(hst->check_type);
    MYSQL_BIND_INT(hst->last_state_change);
    MYSQL_BIND_INT(hst->last_hard_state_change);
    MYSQL_BIND_INT(hst->last_hard_state);
    MYSQL_BIND_INT(hst->last_time_up);
    MYSQL_BIND_INT(hst->last_time_down);
    MYSQL_BIND_INT(hst->last_time_unreachable);
    MYSQL_BIND_INT(hst->state_type);
    MYSQL_BIND_INT(hst->last_notification);
    MYSQL_BIND_INT(hst->next_notification);
    MYSQL_BIND_INT(hst->no_more_notifications);
    MYSQL_BIND_INT(hst->notifications_enabled);
    MYSQL_BIND_INT(hst->problem_has_been_acknowledged);
    MYSQL_BIND_INT(hst->acknowledgement_type);
    MYSQL_BIND_INT(hst->current_notification_number);
    MYSQL_BIND_INT(hst->accept_passive_checks);
    MYSQL_BIND_INT(hst->checks_enabled);
    MYSQL_BIND_INT(hst->event_handler_enabled);
    MYSQL_BIND_INT(hst->flap_detection_enabled);
    MYSQL_BIND_INT(hst->is_flapping);
    MYSQL_BIND_DOUBLE(hst->percent_state_change);
    MYSQL_BIND_DOUBLE(hst->latency);
    MYSQL_BIND_DOUBLE(hst->execution_time);
    MYSQL_BIND_INT(hst->scheduled_downtime_depth);
    MYSQL_BIND_INT(hst->process_performance_data);
    MYSQL_BIND_INT(hst->obsess);
    MYSQL_BIND_INT(hst->modified_attributes);
    MYSQL_BIND_STR(event_handler_name);
    MYSQL_BIND_STR(check_command_name);
    MYSQL_BIND_DOUBLE(hst->check_interval);
    MYSQL_BIND_DOUBLE(hst->retry_interval);
    MYSQL_BIND_INT(timeperiod_object_id);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_service_status(int type, void * d)
{
    nebstruct_service_status_data * data = d;

    int service_object_id     = 0;
    int timeperiod_object_id  = 0;
    service * svc             = NULL;
    timeperiod * tm           = NULL;
    char * event_handler_name = "";
    char * check_command_name = "";

    if (   data->object_ptr == NULL 
        || ((service *) data->object_ptr)->event_handler_ptr == NULL
        || ((service *) data->object_ptr)->check_command_ptr == NULL
        || ((service *) data->object_ptr)->event_handler_ptr->name == NULL
        || ((service *) data->object_ptr)->check_command_ptr->name == NULL) {

        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    svc = data->object_ptr;
    tm = svc->check_period_ptr;
    event_handler_name = svc->event_handler_ptr->name;
    check_command_name = svc->check_command_ptr->name;

    service_object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, svc->host_name, svc->description);
    timeperiod_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_TIMEPERIOD, tm->name);

    MYSQL_RESET_SQL();
    MYSQL_RESET_BIND();

    MYSQL_SET_SQL("INSERT INTO nagios_servicestatus SET instance_id = 1, service_object_id = ?, status_update_time = FROM_UNIXTIME(?), output = ?, long_output = ?, perfdata = ?, current_state = ?, has_been_checked = ?, should_be_scheduled = ?, current_check_attempt = ?, max_check_attempts = ?, last_check = FROM_UNIXTIME(?), next_check = FROM_UNIXTIME(?), check_type = ?, last_state_change = FROM_UNIXTIME(?), last_hard_state_change = FROM_UNIXTIME(?), last_hard_state = ?, last_time_ok = FROM_UNIXTIME(?), last_time_warning = FROM_UNIXTIME(?), last_time_unknown = FROM_UNIXTIME(?), last_time_critical = FROM_UNIXTIME(?), state_type = ?, last_notification = FROM_UNIXTIME(?), next_notification = FROM_UNIXTIME(?), no_more_notifications = ?, notifications_enabled = ?, problem_has_been_acknowledged = ?, acknowledgement_type = ?, current_notification_number = ?, passive_checks_enabled = ?, active_checks_enabled = ?, event_handler_enabled = ?, flap_detection_enabled = ?, is_flapping = ?, percent_state_change = ?, latency = ?, execution_time = ?, scheduled_downtime_depth = ?, failure_prediction_enabled = 0, process_performance_data = ?, obsess_over_service = ?, modified_service_attributes = ?, event_handler = ?, check_command = ?, normal_check_interval = ?, retry_check_interval = ?, check_timeperiod_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, service_object_id = ?, status_update_time = FROM_UNIXTIME(?), output = ?, long_output = ?, perfdata = ?, current_state = ?, has_been_checked = ?, should_be_scheduled = ?, current_check_attempt = ?, max_check_attempts = ?, last_check = FROM_UNIXTIME(?), next_check = FROM_UNIXTIME(?), check_type = ?, last_state_change = FROM_UNIXTIME(?), last_hard_state_change = FROM_UNIXTIME(?), last_hard_state = ?, last_time_ok = FROM_UNIXTIME(?), last_time_warning = FROM_UNIXTIME(?), last_time_unknown = FROM_UNIXTIME(?), last_time_critical = FROM_UNIXTIME(?), state_type = ?, last_notification = FROM_UNIXTIME(?), next_notification = FROM_UNIXTIME(?), no_more_notifications = ?, notifications_enabled = ?, problem_has_been_acknowledged = ?, acknowledgement_type = ?, current_notification_number = ?, passive_checks_enabled = ?, active_checks_enabled = ?, event_handler_enabled = ?, flap_detection_enabled = ?, is_flapping = ?, percent_state_change = ?, latency = ?, execution_time = ?, scheduled_downtime_depth = ?, failure_prediction_enabled = 0, process_performance_data = ?, obsess_over_service = ?, modified_service_attributes = ?, event_handler = ?, check_command = ?, normal_check_interval = ?, retry_check_interval = ?, check_timeperiod_object_id = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(service_object_id);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_STR(svc->plugin_output);
    MYSQL_BIND_STR(svc->long_plugin_output);
    MYSQL_BIND_STR(svc->perf_data);
    MYSQL_BIND_INT(svc->current_state);
    MYSQL_BIND_INT(svc->has_been_checked);
    MYSQL_BIND_INT(svc->should_be_scheduled);
    MYSQL_BIND_INT(svc->current_attempt);
    MYSQL_BIND_INT(svc->max_attempts);
    MYSQL_BIND_INT(svc->last_check);
    MYSQL_BIND_INT(svc->next_check);
    MYSQL_BIND_INT(svc->check_type);
    MYSQL_BIND_INT(svc->last_state_change);
    MYSQL_BIND_INT(svc->last_hard_state_change);
    MYSQL_BIND_INT(svc->last_hard_state);
    MYSQL_BIND_INT(svc->last_time_ok);
    MYSQL_BIND_INT(svc->last_time_warning);
    MYSQL_BIND_INT(svc->last_time_unknown);
    MYSQL_BIND_INT(svc->last_time_critical);
    MYSQL_BIND_INT(svc->state_type);
    MYSQL_BIND_INT(svc->last_notification);
    MYSQL_BIND_INT(svc->next_notification);
    MYSQL_BIND_INT(svc->no_more_notifications);
    MYSQL_BIND_INT(svc->notifications_enabled);
    MYSQL_BIND_INT(svc->problem_has_been_acknowledged);
    MYSQL_BIND_INT(svc->acknowledgement_type);
    MYSQL_BIND_INT(svc->current_notification_number);
    MYSQL_BIND_INT(svc->accept_passive_checks);
    MYSQL_BIND_INT(svc->checks_enabled);
    MYSQL_BIND_INT(svc->event_handler_enabled);
    MYSQL_BIND_INT(svc->flap_detection_enabled);
    MYSQL_BIND_INT(svc->is_flapping);
    MYSQL_BIND_DOUBLE(svc->percent_state_change);
    MYSQL_BIND_DOUBLE(svc->latency);
    MYSQL_BIND_DOUBLE(svc->execution_time);
    MYSQL_BIND_INT(svc->scheduled_downtime_depth);
    MYSQL_BIND_INT(svc->process_performance_data);
    MYSQL_BIND_INT(svc->obsess);
    MYSQL_BIND_INT(svc->modified_attributes);
    MYSQL_BIND_STR(event_handler_name); 
    MYSQL_BIND_STR(check_command_name); 
    MYSQL_BIND_DOUBLE(svc->check_interval);
    MYSQL_BIND_DOUBLE(svc->retry_interval);
    MYSQL_BIND_INT(timeperiod_object_id);

    MYSQL_BIND_INT(service_object_id); 
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_STR(svc->plugin_output);
    MYSQL_BIND_STR(svc->long_plugin_output);
    MYSQL_BIND_STR(svc->perf_data);
    MYSQL_BIND_INT(svc->current_state);
    MYSQL_BIND_INT(svc->has_been_checked);
    MYSQL_BIND_INT(svc->should_be_scheduled);
    MYSQL_BIND_INT(svc->current_attempt);
    MYSQL_BIND_INT(svc->max_attempts);
    MYSQL_BIND_INT(svc->last_check);
    MYSQL_BIND_INT(svc->next_check);
    MYSQL_BIND_INT(svc->check_type);
    MYSQL_BIND_INT(svc->last_state_change);
    MYSQL_BIND_INT(svc->last_hard_state_change);
    MYSQL_BIND_INT(svc->last_hard_state);
    MYSQL_BIND_INT(svc->last_time_ok);
    MYSQL_BIND_INT(svc->last_time_warning);
    MYSQL_BIND_INT(svc->last_time_unknown);
    MYSQL_BIND_INT(svc->last_time_critical);
    MYSQL_BIND_INT(svc->state_type);
    MYSQL_BIND_INT(svc->last_notification);
    MYSQL_BIND_INT(svc->next_notification);
    MYSQL_BIND_INT(svc->no_more_notifications);
    MYSQL_BIND_INT(svc->notifications_enabled);
    MYSQL_BIND_INT(svc->problem_has_been_acknowledged);
    MYSQL_BIND_INT(svc->acknowledgement_type);
    MYSQL_BIND_INT(svc->current_notification_number);
    MYSQL_BIND_INT(svc->accept_passive_checks);
    MYSQL_BIND_INT(svc->checks_enabled);
    MYSQL_BIND_INT(svc->event_handler_enabled);
    MYSQL_BIND_INT(svc->flap_detection_enabled);
    MYSQL_BIND_INT(svc->is_flapping);
    MYSQL_BIND_DOUBLE(svc->percent_state_change);
    MYSQL_BIND_DOUBLE(svc->latency);
    MYSQL_BIND_DOUBLE(svc->execution_time);
    MYSQL_BIND_INT(svc->scheduled_downtime_depth);
    MYSQL_BIND_INT(svc->process_performance_data);
    MYSQL_BIND_INT(svc->obsess);
    MYSQL_BIND_INT(svc->modified_attributes);
    MYSQL_BIND_STR(event_handler_name); 
    MYSQL_BIND_STR(check_command_name); 
    MYSQL_BIND_DOUBLE(svc->check_interval);
    MYSQL_BIND_DOUBLE(svc->retry_interval);
    MYSQL_BIND_INT(timeperiod_object_id); 

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_external_command(int type, void * d)
{
    nebstruct_external_command_data * data = d;

    if (data->type != NEBTYPE_EXTERNALCOMMAND_START) {
        return NDO_OK;
    }

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_externalcommands SET instance_id = 1, command_type = ?, entry_time = FROM_UNIXTIME(?), command_name = ?, command_args = ? ON DUPLICATE KEY UPDATE instance_id = 1, command_type = ?, entry_time = FROM_UNIXTIME(?), command_name = ?, command_args = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->command_type);
    MYSQL_BIND_INT(data->entry_time);
    MYSQL_BIND_STR(data->command_string);
    MYSQL_BIND_STR(data->command_args);

    MYSQL_BIND_INT(data->command_type);
    MYSQL_BIND_INT(data->entry_time);
    MYSQL_BIND_STR(data->command_string);
    MYSQL_BIND_STR(data->command_args);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_acknowledgement(int type, void * d)
{
    nebstruct_acknowledgement_data * data = d;

    int object_id = 0;

    if (data->acknowledgement_type == SERVICE_ACKNOWLEDGEMENT) {
        object_id = ndo_get_object_id_name2(TRUE, NDO_OBJECTTYPE_SERVICE, data->host_name, data->service_description);
    }
    else {
        object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_HOST, data->host_name);
    }

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_acknowledgement SET instance_id = 1, entry_time = FROM_UNIXTIME(?), entry_time_usec = ?, acknowledgement_type = ?, object_id = ?, state = ?, author_name = ?, comment_data = ?, is_sticky = ?, persistent_comment = ?, notify_contacts = ? ON DUPLICATE KEY UPDATE instance_id = 1, entry_time = FROM_UNIXTIME(?), entry_time_usec = ?, acknowledgement_type = ?, object_id = ?, state = ?, author_name = ?, comment_data = ?, is_sticky = ?, persistent_comment = ?, notify_contacts = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->timestamp.tv_usec);
    MYSQL_BIND_INT(data->acknowledgement_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_STR(data->author_name);
    MYSQL_BIND_STR(data->comment_data);
    MYSQL_BIND_INT(data->is_sticky);
    MYSQL_BIND_INT(data->persistent_comment);
    MYSQL_BIND_INT(data->notify_contacts);

    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->timestamp.tv_usec);
    MYSQL_BIND_INT(data->acknowledgement_type);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_STR(data->author_name);
    MYSQL_BIND_STR(data->comment_data);
    MYSQL_BIND_INT(data->is_sticky);
    MYSQL_BIND_INT(data->persistent_comment);
    MYSQL_BIND_INT(data->notify_contacts);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_state_change(int type, void * d)
{
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

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_statehistory SET instance_id = 1, state_time = FROM_UNIXTIME(?), state_time_usec = ?, object_id = ?, state_change = 1, state = ?, state_type = ?, current_check_attempt = ?, max_check_attempts = ?, last_state = ?, last_hard_state = ?, output = ?, long_output = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(data->timestamp.tv_usec);
    MYSQL_BIND_INT(object_id);
    MYSQL_BIND_INT(data->state);
    MYSQL_BIND_INT(data->state_type);
    MYSQL_BIND_INT(data->current_attempt);
    MYSQL_BIND_INT(data->max_attempts);
    MYSQL_BIND_INT(last_state);
    MYSQL_BIND_INT(last_hard_state);
    MYSQL_BIND_STR(data->output);
    MYSQL_BIND_STR(data->longoutput);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_contact_status(int type, void * d)
{
    nebstruct_contact_status_data * data = d;

    int contact_object_id     = 0;
    contact * cnt             = NULL;

    if (data->object_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer(s) is/are null");
        return NDO_OK;
    }

    cnt = data->object_ptr;

    contact_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->name);

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactstatus SET instance_id = 1, contact_object_id = ?, status_update_time = FROM_UNIXTIME(?), host_notifications_enabled = ?, service_notifications_enabled = ?, last_host_notification = FROM_UNIXTIME(?), last_service_notification = FROM_UNIXTIME(?), modified_attributes = ?, modified_host_attributes = ?, modified_service_attributes = ? ON DUPLICATE KEY UPDATE instance_id = 1, contact_object_id = ?, status_update_time = FROM_UNIXTIME(?), host_notifications_enabled = ?, service_notifications_enabled = ?, last_host_notification = FROM_UNIXTIME(?), last_service_notification = FROM_UNIXTIME(?), modified_attributes = ?, modified_host_attributes = ?, modified_service_attributes = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(contact_object_id);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(cnt->host_notifications_enabled);
    MYSQL_BIND_INT(cnt->service_notifications_enabled);
    MYSQL_BIND_INT(cnt->last_host_notification);
    MYSQL_BIND_INT(cnt->last_service_notification);
    MYSQL_BIND_INT(cnt->modified_attributes);
    MYSQL_BIND_INT(cnt->modified_host_attributes);
    MYSQL_BIND_INT(cnt->modified_service_attributes);

    MYSQL_BIND_INT(contact_object_id);
    MYSQL_BIND_INT(data->timestamp.tv_sec);
    MYSQL_BIND_INT(cnt->host_notifications_enabled);
    MYSQL_BIND_INT(cnt->service_notifications_enabled);
    MYSQL_BIND_INT(cnt->last_host_notification);
    MYSQL_BIND_INT(cnt->last_service_notification);
    MYSQL_BIND_INT(cnt->modified_attributes);
    MYSQL_BIND_INT(cnt->modified_host_attributes);
    MYSQL_BIND_INT(cnt->modified_service_attributes);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}

int ndo_handle_contact_notification(int type, void * d)
{
    nebstruct_contact_notification_data * data = d;

    int contact_object_id     = 0;
    contact * cnt             = NULL;

    if (data->contact_ptr == NULL) {
        NDO_REPORT_ERROR("Broker data pointer is null");
        return NDO_OK;
    }

    cnt = data->contact_ptr;

    contact_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_CONTACT, cnt->name);

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactnotifications SET instance_id = 1, notification_id = ?, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, contact_object_id = ? ON DUPLICATE KEY UPDATE instance_id = 1, notification_id = ?, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, contact_object_id = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(ndo_last_notification_id);
    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(contact_object_id);

    MYSQL_BIND_INT(ndo_last_notification_id);
    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(contact_object_id);

    /* see notes regarding `ndo_last_notification_id` - those apply here too */
    ndo_last_contact_notification_id = mysql_insert_id(mysql_connection);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}


int ndo_handle_contact_notification_method(int type, void * d)
{
    nebstruct_contact_notification_method_data * data = d;

    int command_object_id = ndo_get_object_id_name1(TRUE, NDO_OBJECTTYPE_COMMAND, data->command_name);

    MYSQL_RESET_BIND();
    MYSQL_RESET_SQL();

    MYSQL_SET_SQL("INSERT INTO nagios_contactnotificationmethod SET instance_id = 1, contactnotification_id = ?, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, command_object_id = ?, command_args = ? ON DUPLICATE KEY UPDATE instance_id = 1, contactnotification_id = ?, start_time = FROM_UNIXTIME(?), start_time_usec = ?, end_time = FROM_UNIXTIME(?), end_time_usec = ?, command_object_id = ?, command_args = ?");
    MYSQL_PREPARE();

    MYSQL_BIND_INT(ndo_last_contact_notification_id);
    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(command_object_id);
    MYSQL_BIND_STR(data->command_args);

    MYSQL_BIND_INT(ndo_last_contact_notification_id);
    MYSQL_BIND_INT(data->start_time.tv_sec);
    MYSQL_BIND_INT(data->start_time.tv_usec);
    MYSQL_BIND_INT(data->end_time.tv_sec);
    MYSQL_BIND_INT(data->end_time.tv_usec);
    MYSQL_BIND_INT(command_object_id);
    MYSQL_BIND_STR(data->command_args);

    MYSQL_BIND();
    MYSQL_EXECUTE();
}

int ndo_get_object_id_name1(int insert, int object_type, char * name1)
{
    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_get_object_id_name1() - name1 is null");
        return NDO_ERROR;
    }

    int object_id = 0;

    ndo_tmp_str_len[0] = strlen(name1);

    ndo_object_bind[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_bind[0].buffer = &(object_type);

    ndo_object_bind[1].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[1].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[1].buffer = name1;
    ndo_object_bind[1].length = &(ndo_tmp_str_len[0]);

    ndo_object_result[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_result[0].buffer = &object_id;

    ndo_return = mysql_stmt_bind_param(ndo_stmt_object_get_name1, ndo_object_bind);
    NDO_HANDLE_ERROR("Unable to bind parameters");

    ndo_return = mysql_stmt_bind_result(ndo_stmt_object_get_name1, ndo_object_result);
    NDO_HANDLE_ERROR("Unable to bind result parameters");

    ndo_return = mysql_stmt_execute(ndo_stmt_object_get_name1);
    NDO_HANDLE_ERROR("Unable to execute statement");

    ndo_return = mysql_stmt_store_result(ndo_stmt_object_get_name1);
    NDO_HANDLE_ERROR("Unable to store results");

    while (!mysql_stmt_fetch(ndo_stmt_object_get_name1)) {
        return object_id;
    }

    /* if we made it this far it means nothing exists in the database yet */

    if (insert == TRUE) {
        return ndo_insert_object_id_name1(object_type, name1);
    }

    return NDO_ERROR;
}


int ndo_get_object_id_name2(int insert, int object_type, char * name1, char * name2)
{
    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    if (name2 == NULL || strlen(name2) == 0) {
        return ndo_get_object_id_name1(insert, object_type, name1);
    }

    int object_id = 0;

    ndo_tmp_str_len[0] = strlen(name1);
    ndo_tmp_str_len[1] = strlen(name2);

    ndo_object_bind[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_bind[0].buffer = &(object_type);

    ndo_object_bind[1].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[1].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[1].buffer = name1;
    ndo_object_bind[1].length = &(ndo_tmp_str_len[0]);

    ndo_object_bind[2].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[2].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[2].buffer = name2;
    ndo_object_bind[2].length = &(ndo_tmp_str_len[1]);

    ndo_object_result[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_result[0].buffer = &object_id;

    ndo_return = mysql_stmt_bind_param(ndo_stmt_object_get_name2, ndo_object_bind);
    NDO_HANDLE_ERROR("Unable to bind parameters");

    ndo_return = mysql_stmt_bind_result(ndo_stmt_object_get_name2, ndo_object_result);
    NDO_HANDLE_ERROR("Unable to bind result parameters");

    ndo_return = mysql_stmt_execute(ndo_stmt_object_get_name2);
    NDO_HANDLE_ERROR("Unable to execute statement");

    ndo_return = mysql_stmt_store_result(ndo_stmt_object_get_name2);
    NDO_HANDLE_ERROR("Unable to store results");

    while (!mysql_stmt_fetch(ndo_stmt_object_get_name2)) {
        return object_id;
    }

    /* if we made it this far it means nothing exists in the database yet */

    if (insert == TRUE) {
        return ndo_insert_object_id_name2(object_type, name1, name2);
    }

    return NDO_ERROR;
}


/* todo: these insert_object_id functions should be broken into a prepare
   and then an insert. the reason for this is that usually, this function
   will be called by get_object_id() functions ..and the object_bind is already
   appropriately set */
int ndo_insert_object_id_name1(int object_type, char * name1)
{
    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    int object_id = 0;

    ndo_tmp_str_len[0] = strlen(name1);

    ndo_object_bind[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_bind[0].buffer = &(object_type);

    ndo_object_bind[1].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[1].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[1].buffer = name1;
    ndo_object_bind[1].length = &(ndo_tmp_str_len[0]);

    ndo_return = mysql_stmt_bind_param(ndo_stmt_object_insert_name1, ndo_object_bind);
    NDO_HANDLE_ERROR("Unable to bind parameters");

    ndo_return = mysql_stmt_execute(ndo_stmt_object_insert_name1);
    NDO_HANDLE_ERROR("Unable to execute statement");

    return mysql_insert_id(mysql_connection);
}


int ndo_insert_object_id_name2(int object_type, char * name1, char * name2)
{
    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    if (name2 == NULL || strlen(name2) == 0) {
        return ndo_insert_object_id_name1(object_type, name1);
    }

    int object_id = 0;

    ndo_tmp_str_len[0] = strlen(name1);
    ndo_tmp_str_len[1] = strlen(name2);

    ndo_object_bind[0].buffer_type = MYSQL_TYPE_LONG;
    ndo_object_bind[0].buffer = &(object_type);

    ndo_object_bind[1].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[1].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[1].buffer = name1;
    ndo_object_bind[1].length = &(ndo_tmp_str_len[0]);

    ndo_object_bind[2].buffer_type = MYSQL_TYPE_STRING;
    ndo_object_bind[2].buffer_length = MAX_BIND_BUFFER;
    ndo_object_bind[2].buffer = name2;
    ndo_object_bind[2].length = &(ndo_tmp_str_len[1]);

    ndo_return = mysql_stmt_bind_param(ndo_stmt_object_insert_name2, ndo_object_bind);
    NDO_HANDLE_ERROR("Unable to bind parameters");

    ndo_return = mysql_stmt_execute(ndo_stmt_object_insert_name2);
    NDO_HANDLE_ERROR("Unable to execute statement");

    return mysql_insert_id(mysql_connection);
}

int ndo_write_active_objects()
{
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