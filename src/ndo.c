
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
#include <stdarg.h>
#include <pthread.h>

NEB_API_VERSION(CURRENT_NEB_API_VERSION)

extern command * command_list;
extern timeperiod * timeperiod_list;
extern contact * contact_list;
extern contactgroup * contactgroup_list;
extern host * host_list;
extern hostgroup * hostgroup_list;
extern service * service_list;
extern servicegroup * servicegroup_list;
extern hostescalation * hostescalation_list;
extern hostescalation ** hostescalation_ary;
extern serviceescalation * serviceescalation_list;
extern serviceescalation ** serviceescalation_ary;
extern hostdependency * hostdependency_list;
extern hostdependency ** hostdependency_ary;
extern servicedependency * servicedependency_list;
extern servicedependency ** servicedependency_ary;
extern char * config_file;
extern sched_info scheduling_info;
extern char * global_host_event_handler;
extern char * global_service_event_handler;
extern int __nagios_object_structure_version;
extern struct object_count num_objects;

int ndo_database_connected = FALSE;

char * ndo_db_host = NULL;
int ndo_db_port = 3306;
char * ndo_db_socket = NULL;
char * ndo_db_user = NULL;
char * ndo_db_pass = NULL;
char * ndo_db_name = NULL;

int ndo_db_max_reconnect_attempts = 5;

ndo_query_context * main_thread_context = NULL;
ndo_query_context * startup_thread_context = NULL;

int num_result_bindings[NUM_QUERIES] = { 0 };

int num_bindings[NUM_QUERIES] = { 0 };

int ndo_return = 0;
char ndo_error_msg[BUFSZ_LARGE] = { 0 };

int ndo_bind_i = 0;
int ndo_result_i = 0;

int ndo_max_object_insert_count = 200;

int ndo_writing_object_configuration = FALSE;

int ndo_startup_check_enabled = FALSE;
char * ndo_startup_hash_script_path = NULL;
int ndo_startup_skip_writing_objects = FALSE;


void * ndo_handle = NULL;
int ndo_process_options = 0;
int ndo_config_dump_options = 0;
char * ndo_config_file = NULL;

long ndo_last_notification_id = 0L;
long ndo_last_contact_notification_id = 0L;
long nagios_config_file_id = 0L;

int ndo_debug_stack_frames = 0;

pthread_t startup_thread;

queue_node * nebstruct_queue_timed_event = NULL;
queue_node * nebstruct_queue_event_handler = NULL;
queue_node * nebstruct_queue_host_check = NULL;
queue_node * nebstruct_queue_service_check = NULL;
queue_node * nebstruct_queue_comment = NULL;
queue_node * nebstruct_queue_downtime = NULL;
queue_node * nebstruct_queue_flapping = NULL;
queue_node * nebstruct_queue_host_status = NULL;
queue_node * nebstruct_queue_service_status = NULL;
queue_node * nebstruct_queue_contact_status = NULL;
queue_node * nebstruct_queue_notification = NULL;
queue_node * nebstruct_queue_acknowledgement = NULL;
queue_node * nebstruct_queue_statechange = NULL;

pthread_mutex_t queue_timed_event_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_event_handler_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_host_check_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_service_check_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_comment_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_downtime_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_flapping_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_host_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_service_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_contact_status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_notification_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_acknowledgement_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_statechange_mutex = PTHREAD_MUTEX_INITIALIZER;


#include "timing.c"
#include "ndo-startup.c"
#include "ndo-handlers.c"
#include "ndo-startup-queue.c"
#include "ndo-handlers-queue.c"



void ndo_log(char * buffer)
{
    if (write_to_log(buffer, NSLOG_INFO_MESSAGE, NULL) != NDO_OK) {
        return;
    }
}


void ndo_debug(int write_to_log, const char * fmt, ...)
{
    int frame_indentation = 2;
    char frame_fmt[BUFSZ_SMOL] = { 0 };
    char buffer[BUFSZ_XL] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, BUFSZ_XL - 1, fmt, ap);
    va_end(ap);

    if (strlen(buffer) >= BUFSZ_XL - 1) {
        char * warning = "[LINE TRUNCATED]";
        memcpy(buffer, warning, strlen(warning));
    }

    buffer[BUFSZ_XL - 1] = '\0';

    /* create the padding */
    if (ndo_debug_stack_frames > 0) {
        snprintf(frame_fmt, BUFSZ_SMOL - 1, "%%%ds", (frame_indentation * ndo_debug_stack_frames));
        printf(frame_fmt, " ");
    }

    printf("%s\n", buffer);
}


int nebmodule_init(int flags, char * args, void * handle)
{
    trace_func_args("flags=%d, args=%s, handle=%p", flags, args, handle);

    int result = NDO_ERROR;

    /* save handle passed from core */
    ndo_handle = handle;

    ndo_log("NDO (c) Copyright 2009-2019 Nagios - Nagios Core Development Team");

    result = ndo_process_arguments(args);
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_process_arguments() != NDO_OK");
    }

    /* this needs to happen before we process the config file so that
       mysql options are valid for the upcoming session */
    MYSQL *mysql_connection = mysql_init(NULL);
    main_thread_context = calloc(1, sizeof(ndo_query_context));
    main_thread_context->conn = mysql_connection;

    result = ndo_process_file(main_thread_context, ndo_config_file, ndo_process_ndo_config_line);
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_process_file() != NDO_OK");
    }

    result = ndo_config_sanity_check();
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_config_sanity_check() != NDO_OK");
    }    

    result = ndo_initialize_database(main_thread_context);
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_initialize_database() != NDO_OK");
    }

    if (ndo_startup_check_enabled == TRUE) {
        ndo_calculate_startup_hash();
    }

    result = ndo_register_static_callbacks();
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_register_static_callbacks() != NDO_OK");
    }

    result = ndo_register_queue_callbacks();
    if (result != NDO_OK) {
        trace_return_error_cond("ndo_register_queue_callbacks() != NDO_OK");
    }

    trace_return_ok();
}


int nebmodule_deinit(int flags, int reason)
{
    trace_func_args("flags=%d, reason=%d", flags, reason);

    ndo_deregister_callbacks();
    ndo_disconnect_database(main_thread_context);

    if (ndo_config_file != NULL) {
        free(ndo_config_file);
    }

    free(ndo_db_user);
    free(ndo_db_pass);
    free(ndo_db_name);
    free(ndo_db_host);

    free(ndo_startup_hash_script_path);

    ndo_log("NDO - Shutdown complete");

    trace_return_ok();
}


/* free whatever this returns (unless it's null duh) */
char * ndo_strip(char * s)
{
    trace_func_args("s=%s", s);

    int i = 0;
    int len = 0;
    char * str = NULL;
    char * orig = NULL;
    char * tmp = NULL;

    if (s == NULL || strlen(s) == 0) {
        trace_return_null_cond("s == NULL || strlen(s) == 0");
    }

    str = strdup(s);
    orig = str;

    if (str == NULL) {
        trace_return_null_cond("str == NULL");
    }

    len = strlen(str);

    for (i = 0; i < len; i++) {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {

            continue;
        }
        break;
    }

    str += i;

    if (i >= (len - 1)) {
        trace_return("%s", str);
    }

    len = strlen(str);

    for (i = (len - 1); i >= 0; i--) {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {

            continue;
        }
        break;
    }

    str[i + 1] = '\0';

    tmp = strdup(str);
    free(orig);
    str = tmp;

    trace_return("%s", str);
}


int ndo_process_arguments(char * args)
{
    trace_func_args("args=%s", args);

    /* the only argument we accept is a config file location */
    ndo_config_file = ndo_strip(args);

    if (ndo_config_file == NULL || strlen(ndo_config_file) <= 0) {
        ndo_log("No config file specified! (broker_module=/path/to/ndo.o /PATH/TO/CONFIG/FILE)");
        trace_return_error_cond("ndo_config_file == NULL || strlen(ndo_config_file) <= 0");
    }

    trace_return_ok();
}


int ndo_process_file(ndo_query_context *q_ctx, char * file, int (* process_line_cb)(ndo_query_context *q_ctx, char * line))
{
    trace_func_args("file=%s", file);

    FILE * fp = NULL;
    char * contents = NULL;
    int file_size = 0;
    int read_size = 0;
    int process_result = 0;

    if (file == NULL) {
        ndo_log("NULL file passed, skipping ndo_process_file()");
        trace_return_error_cond("file == NULL");
    }

    fp = fopen(file, "r");

    if (fp == NULL) {
        char err[BUFSZ_LARGE] = { 0 };
        snprintf(err, BUFSZ_LARGE - 1, "Unable to open config file specified - %s", file);
        ndo_log(err);
        trace_return_error_cond("fp == NULL");
    }

    /* see how large the file is */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    contents = calloc(file_size + 1, sizeof(char));

    if (contents == NULL) {
        ndo_log("Out of memory apparently. Something bad is about to happen");
        fclose(fp);
        trace_return_error_cond("contents == NULL");
    }

    read_size = fread(contents, sizeof(char), file_size, fp);

    if (read_size != file_size) {
        ndo_log("Unable to fread() file");
        free(contents);
        fclose(fp);
        trace_return_error_cond("read_size != file_size");
    }

    fclose(fp);

    process_result = ndo_process_file_lines(q_ctx, contents, process_line_cb);

    free(contents);

    trace_return("%d", process_result);
}


int ndo_process_file_lines(ndo_query_context *q_ctx, char * contents, int (* process_line_cb)(ndo_query_context *q_ctx, char * line))
{
    trace_func_args("contents=%s", contents);

    int process_result = NDO_ERROR;
    char * current_line = contents;

    if (contents == NULL) {
        trace_return_error_cond("contents == NULL");
    }

    while (current_line != NULL) {

        char * next_line = strchr(current_line, '\n');

        if (next_line != NULL) {
            (*next_line) = '\0';
        }

        process_result = process_line_cb(q_ctx, current_line);

        if (process_result == NDO_ERROR) {
            trace("line with error: [%s]", current_line);
            trace_return_error_cond("process_result == NDO_ERROR");
        }

        if (next_line != NULL) {
            (*next_line) = '\n';
            current_line = next_line + 1;
        }
        else {
            current_line = NULL;
            break;
        }
    }

    trace_return_ok();
}

/* q_ctx is only used in other callbacks, can be NULL for this one */
int ndo_process_ndo_config_line(ndo_query_context *q_ctx, char * line)
{
    trace_func_args("line=%s", line);

    char * key = NULL;
    char * val = NULL;

    if (line == NULL) {
        trace_return_ok_cond("line == NULL");
    }

    key = strtok(line, "=");
    if (key == NULL) {
        trace_return_ok_cond("key == NULL");
    }

    val = strtok(NULL, "\0");
    if (val == NULL) {
        trace_return_ok_cond("val == NULL");
    }

    key = ndo_strip(key);
    if (key == NULL || strlen(key) == 0) {
        trace_return_ok_cond("key == NULL || strlen(key) == 0");
    }

    val = ndo_strip(val);

    if (val == NULL || strlen(val) == 0) {

        free(key);

        if (val != NULL) {
            free(val);
        }

        trace_return_ok_cond("val == NULL || strlen(val) == 0");
    }

    /* skip comments */
    if (key[0] == '#') {
        free(key);
        free(val);
        trace_return_ok_cond("key[0] == '#'");
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
        mysql_options(q_ctx->conn, MYSQL_SET_CHARSET_NAME, val);
    }

    free(key);
    free(val);

    trace_return_ok();
}


int ndo_config_sanity_check()
{
    trace_func_void();
    trace_return_ok();
}

int ndo_initialize_database(ndo_query_context * q_ctx)
{
    trace_func_void();
    int init = NDO_ERROR;

    /* if we've already connected, we execute a ping because we set our
       auto reconnect options appropriately */
    if (q_ctx->connected == TRUE) {

        if (q_ctx->reconnect_counter >= ndo_db_max_reconnect_attempts) {

            /* todo: handle catastrophic failure */
        }

        /* todo - according to the manpage, mysql_thread_id can't be used
           on machines where the possibility exists for it to grow over
           32 bits. instead use a `SELECT CONNECTION_ID()` select statement */
        unsigned long thread_id = mysql_thread_id(q_ctx->conn);

        int result = mysql_ping(q_ctx->conn);
        q_ctx->reconnect_counter += 1;

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

            trace_return_error_cond("mysql_ping() != OK");
        }

        /* if our ping went fine, we reconnected successfully */
        q_ctx->reconnect_counter = 0;

        /* if we did a reconnect, we need to reinitialize the prepared
           statements - to check if reconnection occured, we check the current
           thread_id with whatever the newest is. if they differ, reconnection
           has occured */
        if (thread_id != mysql_thread_id(q_ctx->conn)) {

            /* TODO: the mysql c api documentation is *wonderful* and it talks
               about after a reconnect that prepared statements go away

               so i'm not sure if that means they need to be initialized again
               or if they just need the prepared statement stuff */
            initialize_stmt_data(q_ctx);
            //init = initialize_stmt_data();
            trace_return("%d", NDO_OK);
        }

        trace_return_ok();
    }

    /* this is for the actual initial connection */
    else {

        int reconnect = 1;
        MYSQL * connected = NULL;

        if (q_ctx->conn == NULL) {
            ndo_log("Unable to initialize mysql connection");
            trace_return_error_cond("q_ctx->conn == NULL");
        }

        /* without this flag set, then our mysql_ping() reconnection doesn't
           work so well [at the beginning of this function] */
        mysql_options(q_ctx->conn, MYSQL_OPT_RECONNECT, &reconnect);

        if (ndo_db_host == NULL) {
            ndo_db_host = strdup("localhost");
        }

        connected = mysql_real_connect(
            q_ctx->conn,
            ndo_db_host,
            ndo_db_user,
            ndo_db_pass,
            ndo_db_name,
            ndo_db_port,
            ndo_db_socket,
            CLIENT_REMEMBER_OPTIONS);

        if (connected == NULL) {
            ndo_log("Unable to connect to mysql. Check your configuration");
            trace_return_error_cond("connected == NULL");
        }
    }

    ndo_log("Database initialized");
    q_ctx->connected = TRUE;

#if defined(DEBUG) && DEBUG != FALSE
    mysql_debug("d:t:O,/tmp/client.trace");
    mysql_dump_debug_info(q_ctx->conn);
#endif

    initialize_stmt_data(q_ctx);
    //init = initialize_stmt_data();
    trace_return("%d", NDO_OK);
}


int ndo_initialize_prepared_statements()
{
    trace_func_void();
    trace_return_ok();
}


void ndo_disconnect_database(ndo_query_context *q_ctx)
{
    trace_func_void();

    deinitialize_stmt_data();

    if (q_ctx->connected == TRUE) {
        mysql_close(q_ctx->conn);
    }
    mysql_library_end();
    trace_return_void();
}


int ndo_register_static_callbacks()
{
    trace_func_void();
    int result = 0;

    /* this callback is always registered, as thats where the configuration writing
       comes from. ndo_process_options is actually checked in the case of a
       shutdown or restart */
    result += neb_register_callback(NEBCALLBACK_PROCESS_DATA, ndo_handle, 0, ndo_neb_handle_process);

    if (ndo_process_options & NDO_PROCESS_LOG) {
        result += neb_register_callback(NEBCALLBACK_LOG_DATA, ndo_handle, 0, ndo_neb_handle_log);
    }
    if (ndo_process_options & NDO_PROCESS_SYSTEM_COMMAND) {
        result += neb_register_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndo_handle, 0, ndo_neb_handle_system_command);
    }
    if (ndo_process_options & NDO_PROCESS_PROGRAM_STATUS) {
        result += neb_register_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndo_handle, 0, ndo_neb_handle_program_status);
    }
    if (ndo_process_options & NDO_PROCESS_EXTERNAL_COMMAND) {
        result += neb_register_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndo_handle, 0, ndo_neb_handle_external_command);
    }
    if (ndo_config_dump_options & NDO_CONFIG_DUMP_RETAINED) {
        result += neb_register_callback(NEBCALLBACK_RETENTION_DATA, ndo_handle, 0, ndo_neb_handle_retention);
    }

    if (result != 0) {
        ndo_log("Something went wrong registering callbacks!");
        trace_return_error_cond("result != 0");
    }

    ndo_log("Callbacks registered");
    trace_return_ok();
}


int ndo_register_queue_callbacks()
{
    trace_func_void();
    int result = 0;

    if (ndo_process_options & NDO_PROCESS_TIMED_EVENT) {
        result += neb_register_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_handle, 0, ndo_handle_queue_timed_event);
    }
    if (ndo_process_options & NDO_PROCESS_EVENT_HANDLER) {
        result += neb_register_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_handle, 0, ndo_handle_queue_event_handler);
    }
    if (ndo_process_options & NDO_PROCESS_HOST_CHECK) {
        result += neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle, 0, ndo_handle_queue_host_check);
    }
    if (ndo_process_options & NDO_PROCESS_SERVICE_CHECK) {
        result += neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle, 0, ndo_handle_queue_service_check);
    }
    if (ndo_process_options & NDO_PROCESS_COMMENT) {
        result += neb_register_callback(NEBCALLBACK_COMMENT_DATA, ndo_handle, 0, ndo_handle_queue_comment);
    }
    if (ndo_process_options & NDO_PROCESS_DOWNTIME) {
        result += neb_register_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_handle, 0, ndo_handle_queue_downtime);
    }
    if (ndo_process_options & NDO_PROCESS_FLAPPING) {
        result += neb_register_callback(NEBCALLBACK_FLAPPING_DATA, ndo_handle, 0, ndo_handle_queue_flapping);
    }
    if (ndo_process_options & NDO_PROCESS_HOST_STATUS) {
        result += neb_register_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_handle, 0, ndo_handle_queue_host_status);
    }
    if (ndo_process_options & NDO_PROCESS_SERVICE_STATUS) {
        result += neb_register_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_handle, 0, ndo_handle_queue_service_status);
    }
    if (ndo_process_options & NDO_PROCESS_CONTACT_STATUS) {
        result += neb_register_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle, 0, ndo_handle_queue_contact_status);
    }
    if (ndo_process_options & NDO_PROCESS_NOTIFICATION) {
        result += neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_queue_notification);
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle, 0, ndo_handle_queue_contact_notification);
        result += neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle, 0, ndo_handle_queue_contact_notification_method);
    }
    if (ndo_process_options & NDO_PROCESS_ACKNOWLEDGEMENT) {
        result += neb_register_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_handle, 0, ndo_handle_queue_acknowledgement);
    }
    if (ndo_process_options & NDO_PROCESS_STATE_CHANGE) {
        result += neb_register_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_handle, 0, ndo_handle_queue_statechange);
    }

    if (result != 0) {
        ndo_log("Something went wrong registering callbacks!");
        trace_return_error_cond("result != 0");
    }

    ndo_log("Callbacks registered");
    trace_return_ok();
}


int ndo_deregister_callbacks()
{
    trace_func_void();

    /* just to make sure these get deregistered if they were missed. i.e.: if the queue
       never gets empty? */
    neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_handle_queue_timed_event);
    neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_handle_queue_event_handler);
    neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_handle_queue_host_check);
    neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_handle_queue_service_check);
    neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndo_handle_queue_comment);
    neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_handle_queue_downtime);
    neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndo_handle_queue_flapping);
    neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_handle_queue_host_status);
    neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_handle_queue_service_status);
    neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_handle_queue_contact_status);
    neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle_queue_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle_queue_contact_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle_queue_contact_notification_method);
    neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_handle_queue_acknowledgement);
    neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_handle_queue_statechange);

    /* static callbacks */
    neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, ndo_neb_handle_process);
    neb_deregister_callback(NEBCALLBACK_LOG_DATA, ndo_neb_handle_log);
    neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndo_neb_handle_system_command);
    neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndo_neb_handle_program_status);
    neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndo_neb_handle_external_command);
    neb_deregister_callback(NEBCALLBACK_RETENTION_DATA, ndo_neb_handle_retention);

    /* normal handlers */
    neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndo_neb_handle_timed_event);
    neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndo_neb_handle_event_handler);
    neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndo_neb_handle_host_check);
    neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndo_neb_handle_service_check);
    neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndo_neb_handle_comment);
    neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndo_neb_handle_downtime);
    neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndo_neb_handle_flapping);
    neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndo_neb_handle_host_status);
    neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndo_neb_handle_service_status);
    neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndo_neb_handle_contact_status);
    neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_neb_handle_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_neb_handle_contact_notification);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_neb_handle_contact_notification_method);
    neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndo_neb_handle_acknowledgement);
    neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndo_neb_handle_statechange);

    ndo_log("Callbacks deregistered");
    trace_return_ok();
}


void ndo_calculate_startup_hash()
{
    trace_func_void();

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
        char msg[BUFSZ_LARGE] = { 0 };
        snprintf(msg, BUFSZ_LARGE - 1, "Bad permissions on hashfile in (%s)", ndo_startup_hash_script_path);
        ndo_log(msg);
    }

    trace_return_void();
}


long ndo_get_object_id_name1(ndo_query_context *q_ctx, int insert, int object_type, char * name1)
{
    trace_func_args("insert=%d, object_type=%d, name1=%s", insert, object_type, name1);
    long object_id = NDO_ERROR;
    long tmp = object_id;

    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_get_object_id_name1() - name1 is null");
        trace_return_error_cond("name1==NULL, returning error");
    }

    MYSQL_RESET_BIND(GET_OBJECT_ID_NAME1);
    MYSQL_RESET_RESULT(GET_OBJECT_ID_NAME1);

    MYSQL_BIND_INT(GET_OBJECT_ID_NAME1, object_type);
    MYSQL_BIND_STR(GET_OBJECT_ID_NAME1, name1);

    MYSQL_RESULT_LONG(GET_OBJECT_ID_NAME1, object_id);

    MYSQL_BIND(GET_OBJECT_ID_NAME1);
    MYSQL_BIND_RESULT(GET_OBJECT_ID_NAME1);
    MYSQL_EXECUTE(GET_OBJECT_ID_NAME1);
    MYSQL_STORE_RESULT(GET_OBJECT_ID_NAME1);

    if (MYSQL_FETCH(GET_OBJECT_ID_NAME1)) {
        object_id = NDO_ERROR;
    }

    trace("got object_id=%d", object_id);

    if (insert == TRUE && object_id == NDO_ERROR) {
        object_id = ndo_insert_object_id_name1(q_ctx, object_type, name1);
    }

    if (ndo_writing_object_configuration == TRUE && object_id != NDO_ERROR) {
        MYSQL_RESET_BIND(HANDLE_OBJECT_WRITING);
        MYSQL_BIND_INT(HANDLE_OBJECT_WRITING, object_id);
        MYSQL_BIND(HANDLE_OBJECT_WRITING);
        MYSQL_EXECUTE(HANDLE_OBJECT_WRITING);
    }

    trace_return("%d", object_id);
}


long ndo_get_object_id_name2(ndo_query_context *q_ctx, int insert, int object_type, char * name1, char * name2)
{
    trace_func_args("insert=%d, object_type=%d, name1=%s, name2=%s", insert, object_type, name1, name2);
    long object_id = NDO_ERROR;

    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_get_object_id_name2() - name1 is null");
        trace_return_error_cond("name1==NULL, returning error");
    }

    if (name2 == NULL || strlen(name2) == 0) {
        trace_return_error_cond("name2 == NULL || strlen(name2) == 0");
    }

    MYSQL_RESET_BIND(GET_OBJECT_ID_NAME2);
    MYSQL_RESET_RESULT(GET_OBJECT_ID_NAME2);

    MYSQL_BIND_INT(GET_OBJECT_ID_NAME2, object_type);
    MYSQL_BIND_STR(GET_OBJECT_ID_NAME2, name1);
    MYSQL_BIND_STR(GET_OBJECT_ID_NAME2, name2);

    MYSQL_RESULT_INT(GET_OBJECT_ID_NAME2, object_id);

    MYSQL_BIND(GET_OBJECT_ID_NAME2);
    MYSQL_BIND_RESULT(GET_OBJECT_ID_NAME2);
    MYSQL_EXECUTE(GET_OBJECT_ID_NAME2);
    MYSQL_STORE_RESULT(GET_OBJECT_ID_NAME2);

    if (MYSQL_FETCH(GET_OBJECT_ID_NAME2)) {
        object_id = NDO_ERROR;
    }

    trace("got object_id=%d", object_id);

    if (insert == TRUE && object_id == NDO_ERROR) {
        trace_info("insert==TRUE, calling ndo_insert_object_id_name2");
        object_id = ndo_insert_object_id_name2(q_ctx, object_type, name1, name2);
    }

    if (ndo_writing_object_configuration == TRUE && object_id != NDO_ERROR) {
        trace_info("ndo_writing_object_configuration==TRUE, setting is_active=1");
        MYSQL_RESET_BIND(HANDLE_OBJECT_WRITING);
        MYSQL_BIND_INT(HANDLE_OBJECT_WRITING, object_id);
        MYSQL_BIND(HANDLE_OBJECT_WRITING);
        MYSQL_EXECUTE(HANDLE_OBJECT_WRITING);
    }

    trace_return("%d", object_id);
}


/* todo: these insert_object_id functions should be broken into a prepare
   and then an insert. the reason for this is that usually, this function
   will be called by get_object_id() functions ..and the object_bind is already
   appropriately set */
long ndo_insert_object_id_name1(ndo_query_context *q_ctx, int object_type, char * name1)
{
    trace_func_args("object_type=%d, name1=%s", object_type, name1);
    long object_id = NDO_ERROR;

    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_insert_object_id_name1() - name1 is null");
        trace_return_error_cond("name1 == NULL || strlen(name1) == 0");
    }

    MYSQL_RESET_BIND(INSERT_OBJECT_ID_NAME1);
    MYSQL_RESET_RESULT(INSERT_OBJECT_ID_NAME1);

    MYSQL_BIND_INT(INSERT_OBJECT_ID_NAME1, object_type);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME1, name1);

    MYSQL_BIND(INSERT_OBJECT_ID_NAME1);
    MYSQL_EXECUTE(INSERT_OBJECT_ID_NAME1);

    object_id = mysql_insert_id(q_ctx->conn);
    trace_return("%ld", object_id);
}


long ndo_insert_object_id_name2(ndo_query_context *q_ctx, int object_type, char * name1, char * name2)
{
    trace_func_args("object_type=%d, name1=%s, name2=%s", object_type, name1, name2);
    long object_id = NDO_ERROR;

    if (name1 == NULL || strlen(name1) == 0) {
        ndo_log("ndo_insert_object_id_name2() - name1 is null");
        trace_return_error_cond("name1 == NULL || strlen(name1) == 0");
    }

    if (name2 == NULL || strlen(name2) == 0) {
        trace_info("name2==NULL, calling ndo_insert_object_id_name1");
        object_id = ndo_insert_object_id_name1(q_ctx, object_type, name1);
        trace_return("%lu", object_id);
    }

    MYSQL_RESET_BIND(INSERT_OBJECT_ID_NAME2);
    MYSQL_RESET_RESULT(INSERT_OBJECT_ID_NAME2);

    MYSQL_BIND_INT(INSERT_OBJECT_ID_NAME2, object_type);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME2, name1);
    MYSQL_BIND_STR(INSERT_OBJECT_ID_NAME2, name2);

    MYSQL_BIND(INSERT_OBJECT_ID_NAME2);
    MYSQL_EXECUTE(INSERT_OBJECT_ID_NAME2);

    object_id = mysql_insert_id(q_ctx->conn);
    trace_return("%ld", object_id);
}


int ndo_write_config(int type)
{
    trace_func_args("type=%d", type);
    trace_return_ok();
}

void initialize_bindings_array()
{
    trace_func_void();

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
    num_bindings[HANDLE_OBJECT_WRITING] = 1;

    num_bindings[WRITE_HANDLE_OBJECT_WRITING] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_ACTIVE_OBJECTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_CUSTOMVARS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_CONTACT_ADDRESSES] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_CONTACT_NOTIFICATIONCOMMANDS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_HOST_PARENTHOSTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_HOST_CONTACTGROUPS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_HOST_CONTACTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_SERVICE_PARENTSERVICES] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_SERVICE_CONTACTGROUPS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_SERVICE_CONTACTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_CONTACTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_HOSTS] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_SERVICES] = MAX_SQL_BINDINGS;
    num_bindings[WRITE_CONFIG] = MAX_SQL_BINDINGS;

    num_result_bindings[GET_OBJECT_ID_NAME1] = 1;
    num_result_bindings[GET_OBJECT_ID_NAME2] = 1;

    trace_return_void();
}


int initialize_stmt_data(ndo_query_context * q_ctx)
{
    trace_func_void();

    int i = 0;
    int errors = 0;
    int memory_errors_flag = FALSE;

    initialize_bindings_array();

    if (q_ctx == NULL) {
        q_ctx = calloc(1, sizeof(ndo_query_context));
    }

    if (q_ctx == NULL) {
        ndo_log("Unable to allocate memory for q_ctx");
        trace_return_error_cond("q_ctx == NULL");
    }

    for (i = 0; i < NUM_QUERIES; i++) {

        if (q_ctx->stmt[i] != NULL) {
            mysql_stmt_close(q_ctx->stmt[i]);
        }

        q_ctx->stmt[i] = mysql_stmt_init(q_ctx->conn);

        if (num_bindings[i] > 0) {
            q_ctx->bind[i] = calloc(num_bindings[i], sizeof(MYSQL_BIND));
            q_ctx->strlen[i] = calloc(num_bindings[i], sizeof(long));
        }

        if (num_result_bindings[i] > 0) {
            q_ctx->result[i] = calloc(num_result_bindings[i], sizeof(MYSQL_BIND));
            q_ctx->result_strlen[i] = calloc(num_bindings[i], sizeof(long));
        }

        q_ctx->bind_i[i] = 0;
        q_ctx->result_i[i] = 0;
    }

    q_ctx->query[GENERIC] = calloc(MAX_SQL_BUFFER, sizeof(char));

    q_ctx->query[GET_OBJECT_ID_NAME1] = strdup("SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ?");
    q_ctx->query[GET_OBJECT_ID_NAME2] = strdup("SELECT object_id FROM nagios_objects WHERE objecttype_id = ? AND name1 = ? AND name2 = ?");
    q_ctx->query[INSERT_OBJECT_ID_NAME1] = strdup("INSERT INTO nagios_objects (instance_id, objecttype_id, name1) VALUES (1,?,?) ON DUPLICATE KEY UPDATE is_active = 1");
    q_ctx->query[INSERT_OBJECT_ID_NAME2] = strdup("INSERT INTO nagios_objects (instance_id, objecttype_id, name1, name2) VALUES (1,?,?,?) ON DUPLICATE KEY UPDATE is_active = 1");
    q_ctx->query[HANDLE_LOG_DATA] = strdup("INSERT INTO nagios_logentries (instance_id, logentry_time, entry_time, entry_time_usec, logentry_type, logentry_data, realtime_data, inferred_data_extracted) VALUES (1,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,1,1)");
    q_ctx->query[HANDLE_PROCESS] = strdup("INSERT INTO nagios_processevents (instance_id, event_type, event_time, event_time_usec, process_id, program_name, program_version, program_date) VALUES (1,?,FROM_UNIXTIME(?),?,?,'Nagios',?,?)");
    q_ctx->query[HANDLE_PROCESS_SHUTDOWN] = strdup("UPDATE nagios_programstatus SET program_end_time = FROM_UNIXTIME(?), is_currently_running = 0");
    q_ctx->query[HANDLE_TIMEDEVENT_ADD] = strdup("INSERT INTO nagios_timedeventqueue (instance_id, event_type, queued_time, queued_time_usec, scheduled_time, recurring_event, object_id) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), event_type = VALUES(event_type), queued_time = VALUES(queued_time), queued_time_usec = VALUES(queued_time_usec), scheduled_time = VALUES(scheduled_time), recurring_event = VALUES(recurring_event), object_id = VALUES(object_id)");
    q_ctx->query[HANDLE_TIMEDEVENT_REMOVE] = strdup("DELETE FROM nagios_timedeventqueue WHERE instance_id = 1 AND event_type = ? AND scheduled_time = FROM_UNIXTIME(?) AND recurring_event = ? AND object_id = ?");
    q_ctx->query[HANDLE_TIMEDEVENT_EXECUTE] = strdup("DELETE FROM nagios_timedeventqueue WHERE instance_id = 1 AND scheduled_time < FROM_UNIXTIME(?)");
    q_ctx->query[HANDLE_SYSTEM_COMMAND] = strdup("INSERT INTO nagios_systemcommands (instance_id, start_time, start_time_usec, end_time, end_time_usec, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), command_line = VALUES(command_line), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output)");
    q_ctx->query[HANDLE_EVENT_HANDLER] = strdup("INSERT INTO nagios_eventhandlers (instance_id, start_time, start_time_usec, end_time, end_time_usec, eventhandler_type, object_id, state, state_type, command_object_id, command_args, command_line, timeout, early_timeout, execution_time, return_code, output, long_output) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), eventhandler_type = VALUES(eventhandler_type), object_id = VALUES(object_id), state = VALUES(state), state_type = VALUES(state_type), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output)");
    q_ctx->query[HANDLE_HOST_CHECK] = strdup("INSERT INTO nagios_hostchecks (instance_id, start_time, start_time_usec, end_time, end_time_usec, host_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), host_object_id = VALUES(host_object_id), check_type = VALUES(check_type), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), state = VALUES(state), state_type = VALUES(state_type), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), latency = VALUES(latency), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line)");
    q_ctx->query[HANDLE_SERVICE_CHECK] = strdup("INSERT INTO nagios_servicechecks (instance_id, start_time, start_time_usec, end_time, end_time_usec, service_object_id, check_type, current_check_attempt, max_check_attempts, state, state_type, timeout, early_timeout, execution_time, latency, return_code, output, long_output, perfdata, command_object_id, command_args, command_line) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), service_object_id = VALUES(service_object_id), check_type = VALUES(check_type), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), state = VALUES(state), state_type = VALUES(state_type), timeout = VALUES(timeout), early_timeout = VALUES(early_timeout), execution_time = VALUES(execution_time), latency = VALUES(latency), return_code = VALUES(return_code), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args), command_line = VALUES(command_line)");
    q_ctx->query[HANDLE_COMMENT_ADD] = strdup("INSERT INTO nagios_comments (instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time, entry_time, entry_time_usec) VALUES (1,?,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), comment_type = VALUES(comment_type), entry_type = VALUES(entry_type), object_id = VALUES(object_id), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_persistent = VALUES(is_persistent), comment_source = VALUES(comment_source), expires = VALUES(expires), expiration_time = VALUES(expiration_time), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec)");
    q_ctx->query[HANDLE_COMMENT_HISTORY_ADD] = strdup("INSERT INTO nagios_commenthistory (instance_id, comment_type, entry_type, object_id, comment_time, internal_comment_id, author_name, comment_data, is_persistent, comment_source, expires, expiration_time, entry_time, entry_time_usec) VALUES (1,?,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), comment_type = VALUES(comment_type), entry_type = VALUES(entry_type), object_id = VALUES(object_id), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_persistent = VALUES(is_persistent), comment_source = VALUES(comment_source), expires = VALUES(expires), expiration_time = VALUES(expiration_time), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec)");
    q_ctx->query[HANDLE_COMMENT_DELETE] = strdup("DELETE FROM nagios_comments WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");
    q_ctx->query[HANDLE_COMMENT_HISTORY_DELETE] = strdup("UPDATE nagios_commenthistory SET deletion_time = FROM_UNIXTIME(?), deletion_time_usec = ? WHERE comment_time = FROM_UNIXTIME(?) AND internal_comment_id = ?");
    q_ctx->query[HANDLE_DOWNTIME_ADD] = strdup("INSERT INTO nagios_scheduleddowntime (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (1,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?)) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), downtime_type = VALUES(downtime_type), object_id = VALUES(object_id), entry_time = VALUES(entry_time), author_name = VALUES(author_name), comment_data = VALUES(comment_data), internal_downtime_id = VALUES(internal_downtime_id), triggered_by_id = VALUES(triggered_by_id), is_fixed = VALUES(is_fixed), duration = VALUES(duration), scheduled_start_time = VALUES(scheduled_start_time), scheduled_end_time = VALUES(scheduled_end_time)");
    q_ctx->query[HANDLE_DOWNTIME_HISTORY_ADD] = strdup("INSERT INTO nagios_downtimehistory (instance_id, downtime_type, object_id, entry_time, author_name, comment_data, internal_downtime_id, triggered_by_id, is_fixed, duration, scheduled_start_time, scheduled_end_time) VALUES (1,?,?,FROM_UNIXTIME(?),?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?)) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), downtime_type = VALUES(downtime_type), object_id = VALUES(object_id), entry_time = VALUES(entry_time), author_name = VALUES(author_name), comment_data = VALUES(comment_data), internal_downtime_id = VALUES(internal_downtime_id), triggered_by_id = VALUES(triggered_by_id), is_fixed = VALUES(is_fixed), duration = VALUES(duration), scheduled_start_time = VALUES(scheduled_start_time), scheduled_end_time = VALUES(scheduled_end_time)");
    q_ctx->query[HANDLE_DOWNTIME_START] = strdup("UPDATE nagios_scheduleddowntime SET actual_start_time = FROM_UNIXTIME(?), actual_start_time_usec = ?, was_started = 1 WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    q_ctx->query[HANDLE_DOWNTIME_HISTORY_START] = strdup("UPDATE nagios_downtimehistory SET actual_start_time = FROM_UNIXTIME(?), actual_start_time_usec = ?, was_started = 1 WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    q_ctx->query[HANDLE_DOWNTIME_STOP] = strdup("DELETE FROM nagios_scheduleddowntime WHERE downtime_type = ? AND object_id = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    q_ctx->query[HANDLE_DOWNTIME_HISTORY_STOP] = strdup("UPDATE nagios_downtimehistory SET actual_end_time = FROM_UNIXTIME(?), actual_end_time_usec = ?, was_cancelled = ? WHERE object_id = ? AND downtime_type = ? AND entry_time = FROM_UNIXTIME(?) AND scheduled_start_time = FROM_UNIXTIME(?) AND scheduled_end_time = FROM_UNIXTIME(?)");
    q_ctx->query[HANDLE_FLAPPING] = strdup("INSERT INTO nagios_flappinghistory (instance_id, event_time, event_time_usec, event_type, reason_type, flapping_type, object_id, percent_state_change, low_threshold, high_threshold, comment_time, internal_comment_id) VALUES (1,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), event_time = VALUES(event_time), event_time_usec = VALUES(event_time_usec), event_type = VALUES(event_type), reason_type = VALUES(reason_type), flapping_type = VALUES(flapping_type), object_id = VALUES(object_id), percent_state_change = VALUES(percent_state_change), low_threshold = VALUES(low_threshold), high_threshold = VALUES(high_threshold), comment_time = VALUES(comment_time), internal_comment_id = VALUES(internal_comment_id)");
    q_ctx->query[HANDLE_PROGRAM_STATUS] = strdup("INSERT INTO nagios_programstatus (instance_id, status_update_time, program_start_time, is_currently_running, process_id, daemon_mode, last_command_check, last_log_rotation, notifications_enabled, active_service_checks_enabled, passive_service_checks_enabled, active_host_checks_enabled, passive_host_checks_enabled, event_handlers_enabled, flap_detection_enabled, failure_prediction_enabled, process_performance_data, obsess_over_hosts, obsess_over_services, modified_host_attributes, modified_service_attributes, global_host_event_handler, global_service_event_handler) VALUES (1,FROM_UNIXTIME(?),FROM_UNIXTIME(?),1,?,?,FROM_UNIXTIME(0),FROM_UNIXTIME(?),?,?,?,?,?,?,?,0,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), status_update_time = VALUES(status_update_time), program_start_time = VALUES(program_start_time), is_currently_running = VALUES(is_currently_running), process_id = VALUES(process_id), daemon_mode = VALUES(daemon_mode), last_command_check = VALUES(last_command_check), last_log_rotation = VALUES(last_log_rotation), notifications_enabled = VALUES(notifications_enabled), active_service_checks_enabled = VALUES(active_service_checks_enabled), passive_service_checks_enabled = VALUES(passive_service_checks_enabled), active_host_checks_enabled = VALUES(active_host_checks_enabled), passive_host_checks_enabled = VALUES(passive_host_checks_enabled), event_handlers_enabled = VALUES(event_handlers_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_hosts = VALUES(obsess_over_hosts), obsess_over_services = VALUES(obsess_over_services), modified_host_attributes = VALUES(modified_host_attributes), modified_service_attributes = VALUES(modified_service_attributes), global_host_event_handler = VALUES(global_host_event_handler), global_service_event_handler = VALUES(global_service_event_handler)");
    q_ctx->query[HANDLE_HOST_STATUS] = strdup("INSERT INTO nagios_hoststatus (instance_id, host_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_up, last_time_down, last_time_unreachable, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_host, modified_host_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), host_object_id = VALUES(host_object_id), status_update_time = VALUES(status_update_time), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), current_state = VALUES(current_state), has_been_checked = VALUES(has_been_checked), should_be_scheduled = VALUES(should_be_scheduled), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), last_check = VALUES(last_check), next_check = VALUES(next_check), check_type = VALUES(check_type), last_state_change = VALUES(last_state_change), last_hard_state_change = VALUES(last_hard_state_change), last_hard_state = VALUES(last_hard_state), last_time_up = VALUES(last_time_up), last_time_down = VALUES(last_time_down), last_time_unreachable = VALUES(last_time_unreachable), state_type = VALUES(state_type), last_notification = VALUES(last_notification), next_notification = VALUES(next_notification), no_more_notifications = VALUES(no_more_notifications), notifications_enabled = VALUES(notifications_enabled), problem_has_been_acknowledged = VALUES(problem_has_been_acknowledged), acknowledgement_type = VALUES(acknowledgement_type), current_notification_number = VALUES(current_notification_number), passive_checks_enabled = VALUES(passive_checks_enabled), active_checks_enabled = VALUES(active_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), is_flapping = VALUES(is_flapping), percent_state_change = VALUES(percent_state_change), latency = VALUES(latency), execution_time = VALUES(execution_time), scheduled_downtime_depth = VALUES(scheduled_downtime_depth), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_host = VALUES(obsess_over_host), modified_host_attributes = VALUES(modified_host_attributes), event_handler = VALUES(event_handler), check_command = VALUES(check_command), normal_check_interval = VALUES(normal_check_interval), retry_check_interval = VALUES(retry_check_interval), check_timeperiod_object_id = VALUES(check_timeperiod_object_id)");
    q_ctx->query[HANDLE_SERVICE_STATUS] = strdup("INSERT INTO nagios_servicestatus (instance_id, service_object_id, status_update_time, output, long_output, perfdata, current_state, has_been_checked, should_be_scheduled, current_check_attempt, max_check_attempts, last_check, next_check, check_type, last_state_change, last_hard_state_change, last_hard_state, last_time_ok, last_time_warning, last_time_unknown, last_time_critical, state_type, last_notification, next_notification, no_more_notifications, notifications_enabled, problem_has_been_acknowledged, acknowledgement_type, current_notification_number, passive_checks_enabled, active_checks_enabled, event_handler_enabled, flap_detection_enabled, is_flapping, percent_state_change, latency, execution_time, scheduled_downtime_depth, failure_prediction_enabled, process_performance_data, obsess_over_service, modified_service_attributes, event_handler, check_command, normal_check_interval, retry_check_interval, check_timeperiod_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), service_object_id = VALUES(service_object_id), status_update_time = VALUES(status_update_time), output = VALUES(output), long_output = VALUES(long_output), perfdata = VALUES(perfdata), current_state = VALUES(current_state), has_been_checked = VALUES(has_been_checked), should_be_scheduled = VALUES(should_be_scheduled), current_check_attempt = VALUES(current_check_attempt), max_check_attempts = VALUES(max_check_attempts), last_check = VALUES(last_check), next_check = VALUES(next_check), check_type = VALUES(check_type), last_state_change = VALUES(last_state_change), last_hard_state_change = VALUES(last_hard_state_change), last_hard_state = VALUES(last_hard_state), last_time_ok = VALUES(last_time_ok), last_time_warning = VALUES(last_time_warning), last_time_unknown = VALUES(last_time_unknown), last_time_critical = VALUES(last_time_critical), state_type = VALUES(state_type), last_notification = VALUES(last_notification), next_notification = VALUES(next_notification), no_more_notifications = VALUES(no_more_notifications), notifications_enabled = VALUES(notifications_enabled), problem_has_been_acknowledged = VALUES(problem_has_been_acknowledged), acknowledgement_type = VALUES(acknowledgement_type), current_notification_number = VALUES(current_notification_number), passive_checks_enabled = VALUES(passive_checks_enabled), active_checks_enabled = VALUES(active_checks_enabled), event_handler_enabled = VALUES(event_handler_enabled), flap_detection_enabled = VALUES(flap_detection_enabled), is_flapping = VALUES(is_flapping), percent_state_change = VALUES(percent_state_change), latency = VALUES(latency), execution_time = VALUES(execution_time), scheduled_downtime_depth = VALUES(scheduled_downtime_depth), failure_prediction_enabled = VALUES(failure_prediction_enabled), process_performance_data = VALUES(process_performance_data), obsess_over_service = VALUES(obsess_over_service), modified_service_attributes = VALUES(modified_service_attributes), event_handler = VALUES(event_handler), check_command = VALUES(check_command), normal_check_interval = VALUES(normal_check_interval), retry_check_interval = VALUES(retry_check_interval), check_timeperiod_object_id = VALUES(check_timeperiod_object_id)");
    q_ctx->query[HANDLE_CONTACT_STATUS] = strdup("INSERT INTO nagios_contactstatus (instance_id, contact_object_id, status_update_time, host_notifications_enabled, service_notifications_enabled, last_host_notification, last_service_notification, modified_attributes, modified_host_attributes, modified_service_attributes) VALUES (1,?,FROM_UNIXTIME(?),?,?,FROM_UNIXTIME(?),FROM_UNIXTIME(?),?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contact_object_id = VALUES(contact_object_id), status_update_time = VALUES(status_update_time), host_notifications_enabled = VALUES(host_notifications_enabled), service_notifications_enabled = VALUES(service_notifications_enabled), last_host_notification = VALUES(last_host_notification), last_service_notification = VALUES(last_service_notification), modified_attributes = VALUES(modified_attributes), modified_host_attributes = VALUES(modified_host_attributes), modified_service_attributes = VALUES(modified_service_attributes)");
    q_ctx->query[HANDLE_NOTIFICATION] = strdup("INSERT INTO nagios_notifications (instance_id, start_time, start_time_usec, end_time, end_time_usec, notification_type, notification_reason, object_id, state, output, long_output, escalated, contacts_notified) VALUES (1,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), notification_type = VALUES(notification_type), notification_reason = VALUES(notification_reason), object_id = VALUES(object_id), state = VALUES(state), output = VALUES(output), long_output = VALUES(long_output), escalated = VALUES(escalated), contacts_notified = VALUES(contacts_notified)");
    q_ctx->query[HANDLE_CONTACT_NOTIFICATION] = strdup("INSERT INTO nagios_contactnotifications (instance_id, notification_id, start_time, start_time_usec, end_time, end_time_usec, contact_object_id) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), notification_id = VALUES(notification_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), contact_object_id = VALUES(contact_object_id)");
    q_ctx->query[HANDLE_CONTACT_NOTIFICATION_METHOD] = strdup("INSERT INTO nagios_contactnotificationmethods (instance_id, contactnotification_id, start_time, start_time_usec, end_time, end_time_usec, command_object_id, command_args) VALUES (1,?,FROM_UNIXTIME(?),?,FROM_UNIXTIME(?),?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), contactnotification_id = VALUES(contactnotification_id), start_time = VALUES(start_time), start_time_usec = VALUES(start_time_usec), end_time = VALUES(end_time), end_time_usec = VALUES(end_time_usec), command_object_id = VALUES(command_object_id), command_args = VALUES(command_args)");
    q_ctx->query[HANDLE_EXTERNAL_COMMAND] = strdup("INSERT INTO nagios_externalcommands (instance_id, command_type, entry_time, command_name, command_args) VALUES (1,?,FROM_UNIXTIME(?),?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), command_type = VALUES(command_type), entry_time = VALUES(entry_time), command_name = VALUES(command_name), command_args = VALUES(command_args)");
    q_ctx->query[HANDLE_ACKNOWLEDGEMENT] = strdup("INSERT INTO nagios_acknowledgements (instance_id, entry_time, entry_time_usec, acknowledgement_type, object_id, state, author_name, comment_data, is_sticky, persistent_comment, notify_contacts) VALUES (1,FROM_UNIXTIME(?),?,?,?,?,?,?,?,?,?) ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), entry_time = VALUES(entry_time), entry_time_usec = VALUES(entry_time_usec), acknowledgement_type = VALUES(acknowledgement_type), object_id = VALUES(object_id), state = VALUES(state), author_name = VALUES(author_name), comment_data = VALUES(comment_data), is_sticky = VALUES(is_sticky), persistent_comment = VALUES(persistent_comment), notify_contacts = VALUES(notify_contacts)");
    q_ctx->query[HANDLE_STATE_CHANGE] = strdup("INSERT INTO nagios_statehistory SET instance_id = 1, state_time = FROM_UNIXTIME(?), state_time_usec = ?, object_id = ?, state_change = 1, state = ?, state_type = ?, current_check_attempt = ?, max_check_attempts = ?, last_state = ?, last_hard_state = ?, output = ?, long_output = ?");
    q_ctx->query[HANDLE_OBJECT_WRITING] = strdup("UPDATE nagios_objects SET is_active = 1 WHERE object_id = ?");

    /* now check to make sure all those strdups worked */
    for (i = 0; i < NUM_INITIALIZED_QUERIES; i++) {
        if (q_ctx->query[i] == NULL) {
            char msg[BUFSZ_MED] = { 0 };
            snprintf(msg, BUFSZ_MED - 1, "Unable to allocate memory for query (%d)", i);
            ndo_log(msg);
            errors++;
        }
    }

    if (errors > 0) {
        /* We still want to prepare valid statements so that not all data is lost */
        memory_errors_flag = TRUE;
    }

    /* now prepare each statement that has an assigned query (i.e. just the handlers) */
    for (i = 0; i < NUM_QUERIES; i++) {
        if (q_ctx->query[i] != NULL && mysql_stmt_prepare(q_ctx->stmt[i], q_ctx->query[i], strlen(q_ctx->query[i]))) {
            char msg[BUFSZ_MED] = { 0 };
            snprintf(msg, BUFSZ_MED - 1, "Unable to prepare statement for query (%d)", i);
            ndo_log(msg);
            errors++;
        }
    }

    if (errors > 0) {
        ndo_log(memory_errors_flag ? "Error allocating memory" : "Error preparing statements");
        trace_return_error_cond("errors > 0");
    }

    trace_return_ok();
}


int deinitialize_stmt_data(ndo_query_context * q_ctx)
{
    trace_func_void();

    int i = 0;

    if (q_ctx == NULL) {
        trace_return_ok_cond("q_ctx == NULL");
    }

    for (i = 0; i < NUM_QUERIES; i++) {
        if (q_ctx->stmt[i] != NULL) {
            mysql_stmt_close(q_ctx->stmt[i]);
        }

        if (q_ctx->query[i] != NULL) {
            free(q_ctx->query[i]);
        }

        if (q_ctx->bind[i] != NULL) {
            free(q_ctx->bind[i]);
        }

        if (q_ctx->strlen[i] != NULL) {
            free(q_ctx->strlen[i]);
        }

        if (q_ctx->result[i] != NULL) {
            free(q_ctx->result[i]);
        }

        if (q_ctx->result_strlen[i] != NULL) {
            free(q_ctx->result_strlen[i]);
        }
    }

    free(q_ctx);

    trace_return_ok();
}
