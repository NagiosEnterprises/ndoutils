
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

#define NDO_REPORT_ERROR(err) \
do { \
    snprintf(ndo_error_msg, 1023, "%s(%s:%d): %s", __func__, __FILE__, __LINE__, err); \
    ndo_log(ndo_error_msg); \
} while (0)


#define NDO_HANDLE_ERROR_STMT(err, stmt) \
do { \
    if (ndo_return != 0) { \
        snprintf(ndo_error_msg, 1023, "ndo_return = %d (%s)", ndo_return, mysql_stmt_error(stmt)); \
        ndo_log(ndo_error_msg); \
        NDO_REPORT_ERROR(err); \
        return NDO_ERROR; \
    } \
} while (0)


#define NDO_HANDLE_ERROR(err) NDO_HANDLE_ERROR_STMT(err, ndo_stmt)


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

/* any given query_values string (in startup funcs) is ~120 chars
   and the query_base + query_on_update longest string (write_services) is ~4k
   so we pad a bit and hope we never go over this */
#define MAX_SQL_BUFFER ((MAX_OBJECT_INSERT * 150) + 8000)
#define MAX_SQL_BINDINGS 400
#define MAX_BIND_BUFFER 4096

int ndo_database_connected = FALSE;

char * ndo_db_host = NULL;
int    ndo_db_port = 3306;
char * ndo_db_socket = NULL;
char * ndo_db_user = NULL;
char * ndo_db_pass = NULL;
char * ndo_db_name = NULL;


MYSQL_STMT * ndo_stmt = NULL;
MYSQL_BIND ndo_bind[MAX_SQL_BINDINGS];
MYSQL_BIND ndo_result[MAX_SQL_BINDINGS];

MYSQL_STMT * ndo_stmt_new[NUM_WRITE_QUERIES];
MYSQL_BIND    ndo_bind_new[NUM_WRITE_QUERIES][MAX_SQL_BINDINGS];
int ndo_bind_new_i[NUM_WRITE_QUERIES];
long ndo_tmp_str_len_new[NUM_WRITE_QUERIES][MAX_SQL_BINDINGS];

int ndo_return = 0;
char ndo_error_msg[1024] = { 0 };
/*char ndo_query[NUM_QUERIES][MAX_SQL_BUFFER] = { 0 };*/
char ndo_query[MAX_SQL_BUFFER] = { 0 };
long ndo_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };
long ndo_result_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };

int ndo_bind_i = 0;
int ndo_result_i = 0;
int ndo_last_handle_function = 0;

int ndo_current_query = 0;

/* 3 = objecttype_id, name1, name2 */
MYSQL_BIND ndo_object_bind[3];
MYSQL_BIND ndo_object_result[1];
MYSQL_STMT * ndo_stmt_object_get_name1 = NULL;
MYSQL_STMT * ndo_stmt_object_get_name2 = NULL;
MYSQL_STMT * ndo_stmt_object_insert_name1 = NULL;
MYSQL_STMT * ndo_stmt_object_insert_name2 = NULL;


char active_objects_query[MAX_SQL_BUFFER] = { 0 };
int active_objects_count = 0;
int active_objects_max_params = 0;
int active_objects_i = 0;
MYSQL_BIND * active_objects_bind = NULL;
int * active_objects_object_ids = NULL;
int active_objects_run = 0;


MYSQL_BIND * ndo_startup_bindaaa[NUM_WRITE_QUERIES];
MYSQL_BIND   ndo_bindaaa[NUM_HANDLER_QUERIES][MAX_SQL_BINDINGS];
MYSQL_BIND   ndo_resultaaa[MAX_SQL_BINDINGS];
MYSQL_STMT * ndo_stmtaaa[NUM_QUERIES] = { NULL };


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

    return NDO_OK;
}


int nebmodule_deinit(int flags, int reason)
{
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

    return NDO_OK;
}


/* free whatever this returns (unless it's null duh) */
char * ndo_strip(char * s)
{
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

    return str;
}


int ndo_process_arguments(char * args)
{
    /* the only argument we accept is a config file location */
    ndo_config_file = ndo_strip(args);

    if (ndo_config_file == NULL || strlen(ndo_config_file) <= 0) {
        ndo_log("No config file specified! (broker_module=/path/to/ndo.o /PATH/TO/CONFIG/FILE)");
        return NDO_ERROR;
    }

    return NDO_OK;
}


char * ndo_read_config_file()
{
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
    return contents;
}


int ndo_process_config_file()
{
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
    return ndo_config_sanity_check();
}

int ndo_config_sanity_check()
{
    return NDO_OK;
}


void ndo_process_config_line(char * line)
{
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

    free(key);
    free(val);
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

    ndo_log("Database initialized");
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

    ndo_stmt_log_data = mysql_stmt_init(mysql_connection);

    if (   ndo_stmt == NULL
        || ndo_stmt_object_get_name1 == NULL
        || ndo_stmt_object_get_name2 == NULL
        || ndo_stmt_object_insert_name1 == NULL
        || ndo_stmt_object_insert_name2 == NULL
        || ndo_stmt_log_data == NULL) {

        ndo_log("Unable to initialize prepared statements");
    }

    /* also let's initialize the queries that we re-use constantly */

    strncpy(ndo_query, "SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 IS NULL", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_get_name1, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 = ?", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_get_name2, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "INSERT INTO nagios_objects SET objecttype_id = ?, name1 = ? ON DUPLICATE KEY UPDATE is_active = 1", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_insert_name1, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "INSERT INTO nagios_objects SET objecttype_id = ?, name1 = ?, name2 = ? ON DUPLICATE KEY UPDATE is_active = 1", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_object_insert_name2, ndo_query, strlen(ndo_query));
    // todo

    strncpy(ndo_query, "INSERT INTO nagios_logentries SET instance_id = 1, logentry_time = FROM_UNIXTIME(?), entry_time = FROM_UNIXTIME(?), entry_time_usec = ?, logentry_type = ?, logentry_data = ?, realtime_data = 1, inferred_data_extracted = 1", MAX_SQL_BUFFER);
    ndo_return = mysql_stmt_prepare(ndo_stmt_log_data, ndo_query, strlen(ndo_query));
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
    return NDO_OK;
}


int ndo_deregister_callbacks()
{
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
    return NDO_OK;
}


void ndo_calculate_startup_hash()
{
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
    NDO_HANDLE_ERROR_STMT("Unable to bind parameters", ndo_stmt_object_get_name1);

    ndo_return = mysql_stmt_bind_result(ndo_stmt_object_get_name1, ndo_object_result);
    NDO_HANDLE_ERROR_STMT("Unable to bind result parameters", ndo_stmt_object_get_name1);

    ndo_return = mysql_stmt_execute(ndo_stmt_object_get_name1);
    NDO_HANDLE_ERROR_STMT("Unable to execute statement", ndo_stmt_object_get_name1);

    ndo_return = mysql_stmt_store_result(ndo_stmt_object_get_name1);
    NDO_HANDLE_ERROR_STMT("Unable to store results", ndo_stmt_object_get_name1);

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
    NDO_HANDLE_ERROR_STMT("Unable to bind parameters", ndo_stmt_object_get_name2);

    ndo_return = mysql_stmt_bind_result(ndo_stmt_object_get_name2, ndo_object_result);
    NDO_HANDLE_ERROR_STMT("Unable to bind result parameters", ndo_stmt_object_get_name2);

    ndo_return = mysql_stmt_execute(ndo_stmt_object_get_name2);
    NDO_HANDLE_ERROR_STMT("Unable to execute statement", ndo_stmt_object_get_name2);

    ndo_return = mysql_stmt_store_result(ndo_stmt_object_get_name2);
    NDO_HANDLE_ERROR_STMT("Unable to store results", ndo_stmt_object_get_name2);

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
    NDO_HANDLE_ERROR_STMT("Unable to bind parameters", ndo_stmt_object_insert_name1);

    ndo_return = mysql_stmt_execute(ndo_stmt_object_insert_name1);
    NDO_HANDLE_ERROR_STMT("Unable to execute statement", ndo_stmt_object_insert_name1);

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
    NDO_HANDLE_ERROR_STMT("Unable to bind parameters", ndo_stmt_object_insert_name2);

    ndo_return = mysql_stmt_execute(ndo_stmt_object_insert_name2);
    NDO_HANDLE_ERROR_STMT("Unable to execute statement", ndo_stmt_object_insert_name2);

    return mysql_insert_id(mysql_connection);
}

int ndo_write_config(int type)
{

}
