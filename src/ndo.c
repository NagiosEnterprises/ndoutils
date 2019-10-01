
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
#include <errmsg.h>
#include <time.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION)

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
extern hostescalation      ** hostescalation_ary;
extern serviceescalation    * serviceescalation_list;
extern serviceescalation   ** serviceescalation_ary;
extern hostdependency       * hostdependency_list;
extern hostdependency      ** hostdependency_ary;
extern servicedependency    * servicedependency_list;
extern servicedependency   ** servicedependency_ary;

extern char                 * config_file;
extern sched_info             scheduling_info;
extern char                 * global_host_event_handler;
extern char                 * global_service_event_handler;

extern int                  __nagios_object_structure_version;

extern struct object_count    num_objects;



/**********************************************/
/* Database varibles */

/* any given query_values string (in startup funcs) is ~120 chars
   and the query_base + query_on_update longest string (write_services) is ~4k
   so we pad a bit and hope we never go over this */
#define MAX_SQL_BUFFER ((MAX_OBJECT_INSERT * 150) + 8000)
#define MAX_SQL_BINDINGS 600
#define MAX_BIND_BUFFER 4096

int ndo_database_connected = FALSE;

char * ndo_db_host = NULL;
int    ndo_db_port = 3306;
char * ndo_db_socket = NULL;
char * ndo_db_user = NULL;
char * ndo_db_pass = NULL;
char * ndo_db_name = NULL;

int ndo_db_reconnect_attempts = 0;
int ndo_db_max_reconnect_attempts = 5;


ndo_query_data * ndo_sql = NULL;

MYSQL_STMT * ndo_write_stmt[NUM_WRITE_QUERIES];
MYSQL_BIND ndo_write_bind[NUM_WRITE_QUERIES][MAX_OBJECT_INSERT];
int ndo_write_i[NUM_WRITE_QUERIES] = { 0 };
long ndo_write_tmp_len[NUM_WRITE_QUERIES][MAX_OBJECT_INSERT];

int num_result_bindings[NUM_QUERIES] = { 0 };

int num_bindings[NUM_QUERIES] = { 0 };

int ndo_return = 0;
char ndo_error_msg[1024] = { 0 };
/*char ndo_query[NUM_QUERIES][MAX_SQL_BUFFER] = { 0 };*/
char ndo_query[MAX_SQL_BUFFER] = { 0 };
long ndo_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };
long ndo_result_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };

int ndo_bind_i = 0;
int ndo_result_i = 0;
int ndo_last_handle_function = 0;



char active_objects_query[MAX_SQL_BUFFER] = { 0 };
int active_objects_count = 0;
int active_objects_max_params = 0;
int active_objects_i = 0;
MYSQL_BIND * active_objects_bind = NULL;
int * active_objects_object_ids = NULL;
int active_objects_run = 0;

int ndo_max_object_insert_count = 200;

#define DEFAULT_STARTUP_HASH_SCRIPT_PATH "/usr/local/nagios/bin/ndo-startup-hash.sh"
int ndo_startup_check_enabled = FALSE;
char * ndo_startup_hash_script_path = NULL;
int ndo_startup_skip_writing_objects = FALSE;


MYSQL_BIND ndo_log_data_bind[5];
MYSQL_STMT * ndo_stmt_log_data = NULL;
int ndo_log_data_return = 0;

void * ndo_handle = NULL;
int ndo_process_options = 0;
int ndo_config_dump_options = 0;
char * ndo_config_file = NULL;

long ndo_last_notification_id = 0L;
long ndo_last_contact_notification_id = 0L;


#include "ndo-startup.c"
#include "ndo-handlers.c"


void ndo_log(char * buffer)
{
    if (write_to_log(buffer, NSLOG_INFO_MESSAGE, NULL) != NDO_OK) {
        return;
    }
}


void ndo_debug(int write_to_log, const char * fmt, ...)
{
    char buffer[1024] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, 1023, fmt, ap);
    va_end(ap);

    printf("%s\n", buffer);

#ifndef TESTING
    if (write_to_log == TRUE) {
        ndo_log(buffer);
    }
#endif
}


int nebmodule_init(int flags, char * args, void * handle)
{
    trace("flags=%d, args=%s, handle=%p", flags, args, handle);

    int result = NDO_ERROR;

    /* save handle passed from core */
    ndo_handle = handle;

    ndo_log("NDO (c) Copyright 2009-2019 Nagios - Nagios Core Development Team");

    result = ndo_process_arguments(args);
    if (result != NDO_OK) {
        return NDO_ERROR;
    }

    /* this needs to happen before we process the config file so that
       mysql options are valid for the upcoming session */
    ndo_initialize_mysql_connection();

    result = ndo_process_config_file();
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

    if (ndo_startup_check_enabled == TRUE) {
        ndo_calculate_startup_hash();
    }

    trace_func_end();
    return NDO_OK;
}


int nebmodule_deinit(int flags, int reason)
{
    trace("flags=%d, reason=%d", flags, reason);

    ndo_deregister_callbacks();
    ndo_disconnect_database();

    if (ndo_config_file != NULL) {
        free(ndo_config_file);
    }

    free(ndo_db_user);
    free(ndo_db_pass);
    free(ndo_db_name);
    free(ndo_db_host);

    free(ndo_startup_hash_script_path);

    ndo_log("NDO - Shutdown complete");

    trace_func_end();
    return NDO_OK;
}


/* free whatever this returns (unless it's null duh) */
char * ndo_strip(char * s)
{
    trace("s=%s", s);

    int i = 0;
    int len = 0;
    char * str = NULL;
    char * orig = NULL;
    char * tmp = NULL;

    if (s == NULL || strlen(s) == 0) {
        return NULL;
    }

    str = strdup(s);
    orig = str;

    if (str == NULL) {
        return NULL;
    }

    len = strlen(str);

    if (len == 0) {
        return str;
    }

    for (i = 0; i < len; i++) {
        if (   str[i] == ' '
            || str[i] == '\t'
            || str[i] == '\n'
            || str[i] == '\r') {

            continue;
        }
        break;
    }

    str += i;

    if (i >= (len - 1)) {
        return str;
    }

    len = strlen(str);

    for (i = (len - 1); i >= 0; i--) {
        if (   str[i] == ' '
            || str[i] == '\t'
            || str[i] == '\n'
            || str[i] == '\r') {

            continue;
        }
        break;
    }

    str[i + 1] = '\0';

    tmp = strdup(str);
    free(orig);
    str = tmp;

    trace_func_end();
    return str;
}


int ndo_process_arguments(char * args)
{
    trace("args=%s", args);

    /* the only argument we accept is a config file location */
    ndo_config_file = ndo_strip(args);

    if (ndo_config_file == NULL || strlen(ndo_config_file) <= 0) {
        ndo_log("No config file specified! (broker_module=/path/to/ndo.o /PATH/TO/CONFIG/FILE)");
        return NDO_ERROR;
    }

    trace_func_end();
    return NDO_OK;
}


char * ndo_read_config_file()
{
    trace_func_begin();

    FILE * fp = NULL;
    char * contents = NULL;
    int file_size = 0;
    int read_size = 0;

    if (ndo_config_file == NULL) {
        ndo_log("No configuration specified");
        return NULL;
    }

    fp = fopen(ndo_config_file, "r");

    if (fp == NULL) {
        char err[1024] = { 0 };
        strcpy(err, "Unable to open config file specified - ");
        strcat(err, ndo_config_file);
        ndo_log(err);
        return NULL;
    }

    /* see how large the file is */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    contents = calloc(file_size + 1, sizeof(char));

    if (contents == NULL) {
        ndo_log("Out of memory apparently. Something bad is about to happen");
        fclose(fp);
        return NULL;
    }

    read_size = fread(contents, sizeof(char), file_size, fp);

    if (read_size != file_size) {
        ndo_log("Unable to fread() config file");
        free(contents);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    trace_func_end();
    return contents;
}


int ndo_process_config_file()
{
    trace_func_begin();

    char * config_file_contents = ndo_read_config_file();
    char * current_line = config_file_contents;

    if (config_file_contents == NULL) {
        return NDO_ERROR;
    }

    while (current_line != NULL) {

        char * next_line = strchr(current_line, '\n');

        if (next_line != NULL) {
            (* next_line) = '\0';
        }

        ndo_process_config_line(current_line);

        if (next_line != NULL) {
            (* next_line) = '\n';
            current_line = next_line + 1;
        }
        else {
            current_line = NULL;
            break;
        }
    }

    free(config_file_contents);

    trace_func_end();
    return ndo_config_sanity_check();
}

int ndo_config_sanity_check()
{
    trace_func_begin();
    trace_func_end();
    return NDO_OK;
}


void ndo_process_config_line(char * line)
{
    trace("line=%s", line);

    char * key = NULL;
    char * val = NULL;

    if (line == NULL) {
        return;
    }

    key = strtok(line, "=");
    if (key == NULL) {
        return;
    }

    val = strtok(NULL, "\0");
    if (val == NULL) {
        return;
    }

    key = ndo_strip(key);
    if (key == NULL || strlen(key) == 0) {
        return;
    }

    val = ndo_strip(val);

    if (val == NULL || strlen(val) == 0) {

        free(key);

        if (val != NULL) {
            free(val);
        }

        return;
    }

    /* skip comments */
    if (key[0] == '#') {
        free(key);
        free(val);
        return;
    }

    /* database connectivity */
    else if (!strcmp("db_host", key)) {
        ndo_db_host = strdup(val);
    }
    else if (!strcmp("db_name", key)) {
        ndo_db_name = strdup(val);
    }
    else if (!strcmp("db_user", key)) {
        ndo_db_user = strdup(val);
    }
    else if (!strcmp("db_pass", key)) {
        ndo_db_pass = strdup(val);
    }
    else if (!strcmp("db_port", key)) {
        ndo_db_port = atoi(val);
    }
    else if (!strcmp("db_socket", key)) {
        ndo_db_socket = strdup(val);
    }
    else if (!strcmp("db_max_reconnect_attempts", key)) {
        ndo_db_max_reconnect_attempts = atoi(val);
    }

    /* configuration dumping */
    else if (!strcmp("config_dump_options", key)) {
        ndo_config_dump_options = atoi(val);
    }

    /* determine the maximum amount of objects to send to mysql
       in a single bulk insert statement */
    else if (!strcmp("max_object_insert_count", key)) {
        ndo_max_object_insert_count = atoi(val);

        if (ndo_max_object_insert_count > (MAX_OBJECT_INSERT - 1)) {
            ndo_max_object_insert_count = MAX_OBJECT_INSERT - 1;
        }
    }

    /* should we check the contents of the nagios config directory
       before doing massive table operations in some cases? */
    else if (!strcmp("enable_startup_hash", key)) {
        ndo_startup_check_enabled = atoi(val);
    }
    else if (!strcmp("startup_hash_script_path", key)) {
        ndo_startup_hash_script_path = strdup(val);
    }

    /* neb handlers */
    else if (!strcmp("process_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_PROCESS;
        }
    }
    else if (!strcmp("timed_event_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_TIMED_EVENT;
        }
    }
    else if (!strcmp("log_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_LOG;
        }
    }
    else if (!strcmp("system_command_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_SYSTEM_COMMAND;
        }
    }
    else if (!strcmp("event_handler_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_EVENT_HANDLER;
        }
    }
    else if (!strcmp("host_check_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_HOST_CHECK;
        }
    }
    else if (!strcmp("service_check_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_SERVICE_CHECK;
        }
    }
    else if (!strcmp("comment_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_COMMENT;
        }
    }
    else if (!strcmp("downtime_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_DOWNTIME;
        }
    }
    else if (!strcmp("flapping_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_FLAPPING;
        }
    }
    else if (!strcmp("program_status_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_PROGRAM_STATUS;
        }
    }
    else if (!strcmp("host_status_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_HOST_STATUS;
        }
    }
    else if (!strcmp("service_status_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_SERVICE_STATUS;
        }
    }
    else if (!strcmp("contact_status_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_CONTACT_STATUS;
        }
    }
    else if (!strcmp("notification_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_NOTIFICATION;
        }
    }
    else if (!strcmp("external_command_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_EXTERNAL_COMMAND;
        }
    }
    else if (!strcmp("acknowledgement_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_ACKNOWLEDGEMENT;
        }
    }
    else if (!strcmp("state_change_data", key)) {
        if (atoi(val) > 0) {
            ndo_process_options |= NDO_PROCESS_STATE_CHANGE;
        }
    }

    /* mysql options */
    else if (!strcmp("mysql.charset", key)) {
        mysql_options(mysql_connection, MYSQL_SET_CHARSET_NAME, val);
    }

    free(key);
    free(val);

    trace_func_end();
}


int ndo_initialize_mysql_connection()
{
    trace_func_begin();
    mysql_connection = mysql_init(NULL);
    trace_func_end();
}


int ndo_initialize_database()
{
    trace_func_begin();

    /* if we've already connected, we execute a ping because we set our
       auto reconnect options appropriately */
    if (ndo_database_connected == TRUE) {

        if (ndo_db_reconnect_attempts >= ndo_db_max_reconnect_attempts) {

            /* todo: handle catastrophic failure */
        }

        /* todo - according to the manpage, mysql_thread_id can't be used
           on machines where the possibility exists for it to grow over
           32 bits. instead use a `SELECT CONNECTION_ID()` select statement */
        unsigned long thread_id = mysql_thread_id(mysql_connection);

        int result = mysql_ping(mysql_connection);
        ndo_db_reconnect_attempts++;

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

        /* if our ping went fine, we reconnected successfully */
        ndo_db_reconnect_attempts = 0;

        /* if we did a reconnect, we need to reinitialize the prepared
           statements - to check if reconnection occured, we check the current
           thread_id with whatever the newest is. if they differ, reconnection
           has occured */
        if (thread_id != mysql_thread_id(mysql_connection)) {

            /* TODO: the mysql c api documentation is *wonderful* and it talks
               about after a reconnect that prepared statements go away

               so i'm not sure if that means they need to be initialized again
               or if they just need the prepared statement stuff */
            return initialize_stmt_data();
        }

        return NDO_OK;
    }

    /* this is for the actual initial connection */
    else {

        int reconnect = 1;
        MYSQL * connected = NULL;

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

    ndo_log("Database initialized");
    ndo_database_connected = TRUE;

    trace_func_end();
    return initialize_stmt_data();
}


int ndo_initialize_prepared_statements()
{
    trace_func_begin();


    trace_func_end();
    return NDO_OK;
}


void ndo_disconnect_database()
{
    trace_func_begin();

    deinitialize_stmt_data();

    if (ndo_database_connected == TRUE) {
        mysql_close(mysql_connection);
    }
    mysql_library_end();
    trace_func_end();
}


int ndo_register_callbacks()
{
    trace_func_begin();
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
    if (ndo_process_options & NDO_PROCESS_HOST_CHECK) {
        result += neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle, 0, ndo_handle_host_check);
    }
    if (ndo_process_options & NDO_PROCESS_SERVICE_CHECK) {
        result += neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle, 0, ndo_handle_service_check);
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
    if (ndo_process_options & NDO_PROCESS_CONTACT_STATUS) {
        result += neb_register_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle, 0, ndo_handle_contact_status);
    }
    if (ndo_process_options & NDO_PROCESS_NOTIFICATION) {
        result += neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_notification);
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_contact_notification);
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle, 0, ndo_handle_contact_notification_method);
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

    if (ndo_config_dump_options & NDO_CONFIG_DUMP_RETAINED) {
        result += neb_register_callback(NEBCALLBACK_RETENTION_DATA, ndo_handle, 0, ndo_handle_retention);
    }

    if (result != 0) {
        ndo_log("Something went wrong registering callbacks!");
        return NDO_ERROR;
    }

    ndo_log("Callbacks registered");
    trace_func_end();
    return NDO_OK;
}


int ndo_deregister_callbacks()
{
    trace_func_begin();
    neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, ndo_handle_process);
    neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_handle_timed_event);
    neb_deregister_callback(NEBCALLBACK_LOG_DATA, ndo_handle_log);
    neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndo_handle_system_command);
    neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_handle_event_handler);
    neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle_host_check);
    neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle_service_check);
    neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndo_handle_comment);
    neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_handle_downtime);
    neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndo_handle_flapping);
    neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndo_handle_program_status);
    neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_handle_host_status);
    neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_handle_service_status);
    neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle_contact_status);
    neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle_contact_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle_contact_notification_method);
    neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndo_handle_external_command);
    neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_handle_acknowledgement);
    neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_handle_state_change);

    neb_deregister_callback(NEBCALLBACK_RETENTION_DATA, ndo_handle_retention);

    ndo_log("Callbacks deregistered");
    trace_func_end();
    return NDO_OK;
}


void ndo_calculate_startup_hash()
{
    trace_func_begin();

    int result = 0;
    int early_timeout = FALSE;
    double exectime = 0.0;
    char * output = NULL;

    if (ndo_startup_hash_script_path == NULL) {
        ndo_startup_hash_script_path = strdup(DEFAULT_STARTUP_HASH_SCRIPT_PATH);
    }

    result = my_system_r(NULL, ndo_startup_hash_script_path, 0, &early_timeout, &exectime, &output, 0);

    /* 0 ret code means that the new hash of the directory matches
       the old hash of the directory */
    if (result == 0) {
        ndo_log("Startup hashes match - SKIPPING OBJECT TRUNCATION/RE-INSERTION");
        ndo_startup_skip_writing_objects = TRUE;
    }

    else if (result == 2) {
        char msg[1024] = { 0 };
        snprintf(msg, 1023, "Bad permissions on hashfile in (%s)", ndo_startup_hash_script_path);
        ndo_log(msg);
    }

    trace_func_end();
}


int ndo_get_object_id_name1(int insert, int object_type, char * name1)
{
    trace("insert=%d, object_type=%d, name1=%s", insert, object_type, name1);

    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_get_object_id_name1() - name1 is null");
        return NDO_ERROR;
    }

    int object_id = 0;

    MYSQL_RESET_BIND(GET_OBJECT_ID_NAME1);
    MYSQL_RESET_RESULT(GET_OBJECT_ID_NAME1);

    MYSQL_BIND_INT(GET_OBJECT_ID_NAME1, object_type);
    MYSQL_BIND_STR(GET_OBJECT_ID_NAME1, name1);

    MYSQL_RESULT_INT(GET_OBJECT_ID_NAME1, object_id);

    MYSQL_BIND(GET_OBJECT_ID_NAME1);
    MYSQL_BIND_RESULT(GET_OBJECT_ID_NAME1);
    MYSQL_EXECUTE(GET_OBJECT_ID_NAME1);
    MYSQL_STORE_RESULT(GET_OBJECT_ID_NAME1);

    while (!MYSQL_FETCH(GET_OBJECT_ID_NAME1)) {
        return object_id;
    }

    if (insert == TRUE) {
        return ndo_insert_object_id_name1(object_type, name1);
    }

    trace_func_end();
    return NDO_ERROR;
}


int ndo_get_object_id_name2(int insert, int object_type, char * name1, char * name2)
{
    trace("insert=%d, object_type=%d, name1=%s, name2=%s", insert, object_type, name1, name2);

    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    if (name2 == NULL || strlen(name2) == 0) {
        return ndo_get_object_id_name1(insert, object_type, name1);
    }

    int object_id = 0;

    MYSQL_RESET_BIND(GET_OBJECT_ID_NAME2);
    MYSQL_RESET_RESULT(GET_OBJECT_ID_NAME2);

    MYSQL_BIND_INT(GET_OBJECT_ID_NAME2, object_type);
    MYSQL_BIND_STR(GET_OBJECT_ID_NAME2, name1);

    MYSQL_RESULT_INT(GET_OBJECT_ID_NAME2, object_id);

    MYSQL_BIND(GET_OBJECT_ID_NAME2);
    MYSQL_BIND_RESULT(GET_OBJECT_ID_NAME2);
    MYSQL_EXECUTE(GET_OBJECT_ID_NAME2);
    MYSQL_STORE_RESULT(GET_OBJECT_ID_NAME2);

    while (!MYSQL_FETCH(GET_OBJECT_ID_NAME2)) {
        return object_id;
    }

    /* if we made it this far it means nothing exists in the database yet */

    if (insert == TRUE) {
        return ndo_insert_object_id_name2(object_type, name1, name2);
    }

    trace_func_end();
    return NDO_ERROR;
}


/* todo: these insert_object_id functions should be broken into a prepare
   and then an insert. the reason for this is that usually, this function
   will be called by get_object_id() functions ..and the object_bind is already
   appropriately set */
int ndo_insert_object_id_name1(int object_type, char * name1)
{
    trace("object_type=%d, name1=%s", object_type, name1);

    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    int object_id = 0;

    MYSQL_RESET_BIND(INSERT_OBJECT_ID_NAME1);
    MYSQL_RESET_RESULT(INSERT_OBJECT_ID_NAME1);

    MYSQL_BIND_INT(INSERT_OBJECT_ID_NAME1, object_type);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME1, name1);

    MYSQL_BIND(INSERT_OBJECT_ID_NAME1);
    MYSQL_EXECUTE(INSERT_OBJECT_ID_NAME1);

    trace_func_end();
    return mysql_insert_id(mysql_connection);
}


int ndo_insert_object_id_name2(int object_type, char * name1, char * name2)
{
    trace("object_type=%d, name1=%s, name2=%s", object_type, name1, name2);

    if (name1 == NULL || strlen(name1) == 0) {
        return NDO_ERROR;
    }

    if (name2 == NULL || strlen(name2) == 0) {
        return ndo_insert_object_id_name1(object_type, name1);
    }

    int object_id = 0;

    MYSQL_RESET_BIND(INSERT_OBJECT_ID_NAME2);
    MYSQL_RESET_RESULT(INSERT_OBJECT_ID_NAME2);

    MYSQL_BIND_INT(INSERT_OBJECT_ID_NAME2, object_type);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME2, name1);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME2, name2);

    MYSQL_BIND(INSERT_OBJECT_ID_NAME2);
    MYSQL_EXECUTE(INSERT_OBJECT_ID_NAME2);

    trace_func_end();
    return mysql_insert_id(mysql_connection);
}


int ndo_write_config(int type)
{

}

void initialize_bindings_array()
{
    trace_func_begin();

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

    num_result_bindings[GET_OBJECT_ID_NAME1] = 1;
    num_result_bindings[GET_OBJECT_ID_NAME2] = 1;

    trace_func_end();
}


int initialize_stmt_data()
{
    trace_func_begin();

    int i = 0;
    int errors = 0;

    initialize_bindings_array();

    if (ndo_sql == NULL) {
        ndo_sql = calloc(NUM_QUERIES, sizeof(* ndo_sql));
    }

    if (ndo_sql == NULL) {
        char msg[] = "Unable to allocate memory for ndo_sql";
        ndo_log(msg);
        return NDO_ERROR;
    }

    for (i = 0; i < NUM_QUERIES; i++) {

        if (ndo_sql[i].stmt != NULL) {
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
    num_bindings[HANDLE_COMMENT_DELETE] = 2;
    num_bindings[HANDLE_COMMENT_HISTORY_DELETE] = 4;

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
        ndo_log("errors1");
        return NDO_ERROR;
    }

    /* now prepare each statement - we start at one since GENERIC doesn't
       have a query at this point */
    for (i = 1; i < NUM_QUERIES; i++) {
        if (mysql_stmt_prepare(ndo_sql[i].stmt, ndo_sql[i].query, strlen(ndo_sql[i].query))) {
            char msg[256] = { 0 };
            snprintf(msg, 255, "Unable to prepare statement for query (%d)", i);
            ndo_log(msg);
            errors++;
        }
    }

    if (errors > 0) {
        ndo_log("errors2");
        return NDO_ERROR;
    }

    trace_func_end();
    return NDO_OK;
}





int deinitialize_stmt_data()
{
    trace_func_begin();

    int i = 0;
    int errors = 0;

    if (ndo_sql == NULL) {
        return NDO_OK;
    }

    for (i = 0; i < NUM_QUERIES; i++) {
        if (ndo_sql[i].stmt != NULL) {
            mysql_stmt_close(ndo_sql[i].stmt);
        }

        if (ndo_sql[i].query != NULL) {
            free(ndo_sql[i].query);
        }

        if (ndo_sql[i].bind != NULL) {
            free(ndo_sql[i].bind);
        }

        if (ndo_sql[i].strlen != NULL) {
            free(ndo_sql[i].strlen);
        }

        if (ndo_sql[i].result != NULL) {
            free(ndo_sql[i].result);
        }

        if (ndo_sql[i].result_strlen != NULL) {
            free(ndo_sql[i].result_strlen);
        }
    }

    free(ndo_sql);

    trace_func_end();
}
