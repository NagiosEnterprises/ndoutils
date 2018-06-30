/**
 * @file ndomod.c Nagios Data Output Event Broker Module
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * Last Modified: 2017-04 - 13
 *
 * This file is part of NDOUtils.
 *
 * First Written: 05 - 19-2005
 * Last Modified: 2017-04 - 13
 *
 *****************************************************************************
 * NDOUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NDOUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NDOUtils. If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @todo Add service parents
 * @todo hourly value (hosts / services)
 * @todo minimum value (contacts)
 */

/* include our project's header files */
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndomod.h"

#include <pthread.h>

/* include (minimum required) event broker header files */
#include "../include/nagios/nebstructs.h"
#include "../include/nagios/nebmodules.h"
#include "../include/nagios/nebcallbacks.h"
#include "../include/nagios/broker.h"

/* include other Nagios header files for access to functions, data structs, etc. */
#include "../include/nagios/common.h"
#include "../include/nagios/nagios.h"
#include "../include/nagios/downtime.h"
#include "../include/nagios/comments.h"
#include "../include/nagios/macros.h"

/* specify event broker API version (required) */
NEB_API_VERSION(CURRENT_NEB_API_VERSION)


#define NDOMOD_VERSION "2.1.3"
#define NDOMOD_NAME "NDOMOD"
#define NDOMOD_DATE "2017-04 - 13"

#define BD_INT              0
#define BD_TIMEVAL          1
#define BD_STRING           2
#define BD_UNSIGNED_LONG    3
#define BD_FLOAT            4

/* helpers to clean up the data structs be cleaner */
#define NDODATA_INTEGER(_KEY, _INT) { _KEY, BD_INT, { .integer = _INT } }
#define NDODATA_TIMESTAMP(_KEY, _TS) { _KEY, BD_TIMEVAL, { .timestamp = _TS } }
#define NDODATA_STRING(_KEY, _STR) { _KEY, BD_STRING, { .string = (_STR == NULL ? "" : _STR) } }
#define NDODATA_UNSIGNED_LONG(_KEY, _LONG) { _KEY, BD_UNSIGNED_LONG, { .unsigned_long = _LONG } }
#define NDODATA_FLOATING_POINT(_KEY, _FP) { _KEY, BD_FLOAT, { .floating_point = _FP } }

struct ndo_broker_data {
    int key;
    int datatype;
    union {
        int integer;
        struct timeval timestamp;
        char *string;
        unsigned long unsigned_long;
        double floating_point;
    } value;
};

void *ndomod_module_handle = NULL;
char *ndomod_instance_name = NULL;
char *ndomod_buffer_file = NULL;
char *ndomod_sink_name = NULL;
int ndomod_sink_type = NDO_SINK_UNIXSOCKET;
int ndomod_sink_tcp_port = NDO_DEFAULT_TCP_PORT;
int ndomod_sink_is_open = NDO_FALSE;
int ndomod_sink_previously_open = NDO_FALSE;
int ndomod_sink_fd = - 1;
time_t ndomod_sink_last_reconnect_attempt = 0L;
time_t ndomod_sink_last_reconnect_warning = 0L;
unsigned long ndomod_sink_connect_attempt = 0L;
unsigned long ndomod_sink_reconnect_interval = 15;
unsigned long ndomod_sink_reconnect_warning_interval = 900;
unsigned long ndomod_sink_rotation_interval = 3600;
char *ndomod_sink_rotation_command = NULL;
int ndomod_sink_rotation_timeout = 60;
int ndomod_allow_sink_activity = NDO_TRUE;
unsigned long ndomod_process_options = 0;
int ndomod_config_output_options = NDOMOD_CONFIG_DUMP_ALL;
unsigned long ndomod_sink_buffer_slots = 5000;
ndomod_sink_buffer sinkbuf;
int has_ver403_long_output = (CURRENT_OBJECT_STRUCTURE_VERSION >= 403);

extern int errno;

/**** NAGIOS VARIABLES ****/
extern command *command_list;
extern timeperiod *timeperiod_list;
extern contact *contact_list;
extern contactgroup *contactgroup_list;
extern host *host_list;
extern hostgroup *hostgroup_list;
extern service *service_list;
extern servicegroup *servicegroup_list;
extern hostescalation *hostescalation_list;
extern serviceescalation *serviceescalation_list;
extern hostdependency *hostdependency_list;
extern servicedependency *servicedependency_list;

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __nagios_object_structure_version;

extern int use_ssl;

static inline void ndomod_free_esc_buffers(char **ary, int num);

#define DEBUG_NDO 1

/* this function gets called when the module is loaded by the event broker */
int nebmodule_init(int flags, char *args, void *handle)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];

    /* save our handle */
    ndomod_module_handle = handle;

    /* log module info to the Nagios log file */
    snprintf(temp_buffer, sizeof(temp_buffer) - 1, 
        "ndomod: %s %s (%s) Copyright (c) 2009 Nagios Core Development Team and Community Contributors", 
        NDOMOD_NAME, NDOMOD_VERSION, NDOMOD_DATE);
    temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
    ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

    /* check Nagios object structure version */
    if (ndomod_check_nagios_object_version() == NDO_ERROR) {
        return - 1;
    }

    /* process arguments */
    if (ndomod_process_module_args(args) == NDO_ERROR) {
        ndomod_write_to_logs("ndomod: An error occurred while attempting to process module arguments.", NSLOG_INFO_MESSAGE);
        return - 1;
    }

    /* do some initialization stuff... */
    if (ndomod_init() == NDO_ERROR) {
        ndomod_write_to_logs("ndomod: An error occurred while attempting to initialize.", NSLOG_INFO_MESSAGE);
        return - 1;
    }

    return 0;
}


/* Shutdown and release our resources when the module is unloaded. */
int nebmodule_deinit(int flags, int reason)
{
    char msg[] = "ndomod: Shutdown complete."; /* A message for the core log. */
    ndomod_deinit();
    ndomod_write_to_logs(msg, NSLOG_INFO_MESSAGE);
    return 0;
}



/****************************************************************************/
/* INIT/DEINIT FUNCTIONS                                                    */
/****************************************************************************/

/* checks to make sure Nagios object version matches what we know about */
int ndomod_check_nagios_object_version(void)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];

    if (__nagios_object_structure_version != CURRENT_OBJECT_STRUCTURE_VERSION) {

        /* Temporary special case so newer ndomod can be used with slightly
         * older nagios in order to get longoutput on state changes */
        if (CURRENT_OBJECT_STRUCTURE_VERSION >= 403 && __nagios_object_structure_version == 402) {
            has_ver403_long_output = 0;
            return NDO_OK;
        }

        snprintf(temp_buffer, sizeof(temp_buffer) - 1, 
            "ndomod: I've been compiled with support for revision %d of the internal Nagios object structures,"
            " but the Nagios daemon is currently using revision %d."
            "  I'm going to unload so I don't cause any problems...\n", 
            CURRENT_OBJECT_STRUCTURE_VERSION, __nagios_object_structure_version);
        temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
        ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

        return NDO_ERROR;
    }

    return NDO_OK;
}


/* performs some initialization stuff */
int ndomod_init(void)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];
    time_t current_time;

    /* initialize some vars (needed for restarts of daemon - why, if the module gets reloaded ???) */
    ndomod_sink_is_open                = NDO_FALSE;
    ndomod_sink_previously_open        = NDO_FALSE;
    ndomod_sink_fd                     = - 1;
    ndomod_sink_last_reconnect_attempt = 0L;
    ndomod_sink_last_reconnect_warning = 0L;
    ndomod_allow_sink_activity         = NDO_TRUE;

    /* initialize data sink buffer */
    ndomod_sink_buffer_init(&sinkbuf, ndomod_sink_buffer_slots);

    /* read unprocessed data from buffer file */
    ndomod_load_unprocessed_data(ndomod_buffer_file);

    /* open data sink and say hello */
    /* 05/04/06 - modified to flush buffer items that may have been read in from file */
    ndomod_write_to_sink("\n", NDO_FALSE, NDO_TRUE);

    /* register callbacks */
    if (ndomod_register_callbacks() == NDO_ERROR) {
        return NDO_ERROR;
    }

    if (ndomod_sink_type == NDO_SINK_FILE) {

        /* make sure we have a rotation command defined... */
        if (ndomod_sink_rotation_command == NULL) {

            /* log an error message to the Nagios log file */
            snprintf(temp_buffer, sizeof(temp_buffer) - 1, "ndomod: Warning - No file rotation command defined.\n");
            temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
            ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
        }

        /* schedule a file rotation event */
        else {
            time(&current_time);
            schedule_new_event(EVENT_USER_FUNCTION, TRUE, current_time+ndomod_sink_rotation_interval, TRUE, ndomod_sink_rotation_interval, NULL, TRUE, (void *)ndomod_rotate_sink_file, NULL, 0);
        }
    }

    return NDO_OK;
}


/* Shutdown and release our resources when the module is unloaded. */
int ndomod_deinit(void)
{
    ndomod_deregister_callbacks();

    ndomod_save_unprocessed_data(ndomod_buffer_file);
    ndomod_sink_buffer_deinit(&sinkbuf);
    ndomod_goodbye_sink();
    ndomod_close_sink();

    ndomod_free_config_memory();

    return NDO_OK;
}



/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* process arguments that were passed to the module at startup */
int ndomod_process_module_args(char *args)
{
    char *ptr = NULL;
    char **arglist = NULL;
    char **newarglist = NULL;
    int argcount = 0;
    int memblocks = 64;
    int arg = 0;

    if (args == NULL) {
        return NDO_OK;
    }

    /* get all the var/val argument pairs */

    /* allocate some memory */
    arglist = (char **)malloc(memblocks*sizeof(char **));
    if (arglist == NULL) {
        return NDO_ERROR;
    }

    /* process all args */
    ptr = strtok(args, ",");
    while (ptr) {

        /* save the argument */
        arglist[argcount++] = strdup(ptr);

        /* allocate more memory if needed */
        if (!(argcount % memblocks)) {

            newarglist = (char **)realloc(arglist, (argcount+memblocks)*sizeof(char **));
            if (newarglist == NULL) {

                for (arg = 0; arg < argcount; arg++) {
                    free(arglist[argcount]);
                }

                free(arglist);
                return NDO_ERROR;
            }
            else {
                arglist = newarglist;
            }
        }

        ptr = strtok(NULL, ",");
    }

    /* terminate the arg list */
    arglist[argcount] = '\x0';

    /* process each argument */
    for (arg = 0; arg < argcount; arg++) {

        if (ndomod_process_config_var(arglist[arg]) == NDO_ERROR) {

            for (arg = 0; arg < argcount; arg++) {
                free(arglist[arg]);
            }

            free(arglist);
            return NDO_ERROR;
        }
    }

    /* free allocated memory */
    for (arg = 0; arg < argcount; arg++) {
        free(arglist[arg]);
    }

    free(arglist);

    return NDO_OK;
}


/* process all config vars in a file */
int ndomod_process_config_file(char *filename)
{
    ndo_mmapfile *thefile = NULL;
    char *buf             = NULL;
    int result            = NDO_OK;

    /* open the file */
    thefile = ndo_mmap_fopen(filename);
    if (thefile == NULL) {
        return NDO_ERROR;
    }

    /* process each line of the file */
    while (buf = ndo_mmap_fgets(thefile)) {

        /* skip comments */
        if (buf[0] == '#') {
            free(buf);
            continue;
        }

        /* skip blank lines */
        if (!strcmp(buf, "")) {
            free(buf);
            continue;
        }

        /* process the variable */
        result = ndomod_process_config_var(buf);

        /* free memory */
        free(buf);

        if (result != NDO_OK)
            break;
        }

    /* close the file */
    ndo_mmap_fclose(thefile);

    return result;
}


/* process a single module config variable */
int ndomod_process_config_var(char *arg)
{
    char *var = NULL;
    char *val = NULL;

    /* split var/val */
    var = strtok(arg, " = ");
    val = strtok(NULL, "\n");

    /* skip incomplete var/val pairs */
    if (var == NULL || val == NULL) {
        return NDO_OK;
    }

    /* strip var/val */
    ndomod_strip(var);
    ndomod_strip(val);

    /* process the variable... */

    if (!strcmp(var, "config_file")) {
        ndomod_process_config_file(val);
    }

    else if (!strcmp(var, "instance_name")) {
        ndomod_instance_name = strdup(val);
    }

    else if (!strcmp(var, "output")) {
        ndomod_sink_name = strdup(val);
    }

    else if (!strcmp(var, "output_type")) {
        if (!strcmp(val, "file")) {
            ndomod_sink_type = NDO_SINK_FILE;
        }
        else if (!strcmp(val, "tcpsocket")) {
            ndomod_sink_type = NDO_SINK_TCPSOCKET;
        }
        else {
            ndomod_sink_type = NDO_SINK_UNIXSOCKET;
        }
    }

    else if (!strcmp(var, "tcp_port")) {
        ndomod_sink_tcp_port = atoi(val);
    }

    else if (!strcmp(var, "output_buffer_items")) {
        ndomod_sink_buffer_slots = strtoul(val, NULL, 0);
    }

    else if (!strcmp(var, "reconnect_interval")) {
        ndomod_sink_reconnect_interval = strtoul(val, NULL, 0);
    }

    else if (!strcmp(var, "reconnect_warning_interval")) {
        ndomod_sink_reconnect_warning_interval = strtoul(val, NULL, 0);
    }

    else if (!strcmp(var, "file_rotation_interval")) {
        ndomod_sink_rotation_interval = strtoul(val, NULL, 0);
    }

    else if (!strcmp(var, "file_rotation_command")) {
        ndomod_sink_rotation_command = strdup(val);
    }

    else if (!strcmp(var, "file_rotation_timeout")) {
        ndomod_sink_rotation_timeout = atoi(val);
    }

    /* add bitwise processing opts */
    else if (!strcmp(var, "process_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_PROCESS_DATA;
    }
    else if (!strcmp(var, "timed_event_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_TIMED_EVENT_DATA;
    }
    else if (!strcmp(var, "log_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_LOG_DATA;
    }
    else if (!strcmp(var, "system_command_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SYSTEM_COMMAND_DATA;
    }
    else if (!strcmp(var, "event_handler_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_EVENT_HANDLER_DATA;
    }
    else if (!strcmp(var, "notification_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_NOTIFICATION_DATA;
    }
    else if (!strcmp(var, "service_check_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SERVICE_CHECK_DATA ;
    }
    else if (!strcmp(var, "host_check_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_HOST_CHECK_DATA;
    }
    else if (!strcmp(var, "comment_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_COMMENT_DATA;
    }
    else if (!strcmp(var, "downtime_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_DOWNTIME_DATA;
    }
    else if (!strcmp(var, "flapping_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_FLAPPING_DATA;
    }
    else if (!strcmp(var, "program_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_PROGRAM_STATUS_DATA;
    }
    else if (!strcmp(var, "host_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_HOST_STATUS_DATA;
    }
    else if (!strcmp(var, "service_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SERVICE_STATUS_DATA;
    }
    else if (!strcmp(var, "adaptive_program_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA;
    }
    else if (!strcmp(var, "adaptive_host_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_HOST_DATA;
    }
    else if (!strcmp(var, "adaptive_service_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA;
    }
    else if (!strcmp(var, "external_command_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA;
    }
    else if (!strcmp(var, "object_config_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_OBJECT_CONFIG_DATA;
    }
    else if (!strcmp(var, "main_config_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_MAIN_CONFIG_DATA;
    }
    else if (!strcmp(var, "aggregated_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_AGGREGATED_STATUS_DATA;
    }
    else if (!strcmp(var, "retention_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_RETENTION_DATA;
    }
    else if (!strcmp(var, "acknowledgement_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA;
    }
    else if (!strcmp(var, "statechange_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_STATECHANGE_DATA ;
    }
    else if (!strcmp(var, "state_change_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_STATECHANGE_DATA ;
    }
    else if (!strcmp(var, "contact_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_CONTACT_STATUS_DATA;
    }
    else if (!strcmp(var, "adaptive_contact_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA ;
    }

    /* data_processing_options will override individual values if set */
    else if (!strcmp(var, "data_processing_options")) {
        if (!strcmp(val, " - 1")) {
            ndomod_process_options = NDOMOD_PROCESS_EVERYTHING;
        }
        else {
            ndomod_process_options = strtoul(val, NULL, 0);
        }
    }

    else if (!strcmp(var, "config_output_options")) {
        ndomod_config_output_options = atoi(val);
    }

    else if (!strcmp(var, "buffer_file")) {
        ndomod_buffer_file = strdup(val);
    }

    else if (!strcmp(var, "use_ssl")) {
        if (strlen(val) == 1) {
            if (isdigit((int)val[strlen(val) - 1]) != NDO_FALSE) {
                use_ssl = atoi(val);
            }
            else {
                use_ssl = 0;
            }
        }
    }

    /* new processing options will be skipped if they're set to 0
    else {
        printf("Invalid ndomod config option: %s\n", var);
        return NDO_ERROR;
    }
    */

    return NDO_OK;
}

/* Frees any memory allocated for config options. */
static void ndomod_free_config_memory(void)
{
    my_free(ndomod_instance_name);
    my_free(ndomod_sink_name);
    my_free(ndomod_sink_rotation_command);
    my_free(ndomod_buffer_file);
}


/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

/* writes a string to Nagios logs */
int ndomod_write_to_logs(char *buf, int flags)
{
    if (buf == NULL) {
        return NDO_ERROR;
    }

    return write_to_all_logs(buf, flags);
}



/****************************************************************************/
/* DATA SINK FUNCTIONS                                                      */
/****************************************************************************/

/* (re)open data sink */
int ndomod_open_sink(void)
{
    int flags = 0;
    int ret   = 0;

    /* sink is already open... */
    if (ndomod_sink_is_open == NDO_TRUE) {
        return ndomod_sink_fd;
    }

    /* try and open sink */
    if (ndomod_sink_type == NDO_SINK_FILE) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    }

    ret = ndo_sink_open(ndomod_sink_name, 0, ndomod_sink_type, ndomod_sink_tcp_port, flags, &ndomod_sink_fd);
    if (ret == NDO_ERROR) {
        return NDO_ERROR;
    }

    /* mark the sink as being open */
    ndomod_sink_is_open = NDO_TRUE;

    /* mark the sink as having once been open */
    ndomod_sink_previously_open = NDO_TRUE;

    return NDO_OK;
}


/* (re)open data sink */
int ndomod_close_sink(void)
{
    /* sink is already closed... */
    if (ndomod_sink_is_open == NDO_FALSE) {
        return NDO_OK;
    }

    /* flush sink */
    ndo_sink_flush(ndomod_sink_fd);

    /* close sink */
    ndo_sink_close(ndomod_sink_fd);

    /* mark the sink as being closed */
    ndomod_sink_is_open = NDO_FALSE;

    return NDO_OK;
}


/* say hello */
int ndomod_hello_sink(int reconnect, int problem_disconnect)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];
    char *connection_type = NULL;
    char *connect_type = NULL;

    /* get the connection type string */
    if (ndomod_sink_type == NDO_SINK_FD || ndomod_sink_type == NDO_SINK_FILE) {
        connection_type = NDO_API_CONNECTION_FILE;
    }
    else if (ndomod_sink_type == NDO_SINK_TCPSOCKET) {
        connection_type = NDO_API_CONNECTION_TCPSOCKET;
    }
    else {
        connection_type = NDO_API_CONNECTION_UNIXSOCKET;
    }

    /* get the connect type string */
    if (reconnect == TRUE && problem_disconnect == TRUE) {
        connect_type = NDO_API_CONNECTTYPE_RECONNECT;
    }
    else {
        connect_type = NDO_API_CONNECTTYPE_INITIAL;
    }

    snprintf(temp_buffer, sizeof(temp_buffer) - 1,
        "\n\n%s\n%s: %d\n%s: %s\n%s: %s\n%s: %lu\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s\n\n",
        NDO_API_HELLO,
        NDO_API_PROTOCOL,
        NDO_API_PROTOVERSION,
        NDO_API_AGENT,
        NDOMOD_NAME,
        NDO_API_AGENTVERSION,
        NDOMOD_VERSION,
        NDO_API_STARTTIME,
        (unsigned long)time(NULL),
        NDO_API_DISPOSITION,
        NDO_API_DISPOSITION_REALTIME,
        NDO_API_CONNECTION,
        connection_type,
        NDO_API_CONNECTTYPE,
        connect_type,
        NDO_API_INSTANCENAME,
        (ndomod_instance_name == NULL ? "default" : ndomod_instance_name),
        NDO_API_STARTDATADUMP);

    temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

    ndomod_write_to_sink(temp_buffer, NDO_FALSE, NDO_FALSE);

    return NDO_OK;
}


/* say goodbye */
int ndomod_goodbye_sink(void)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];

    snprintf(temp_buffer, sizeof(temp_buffer) - 1,
        "\n%d\n%s: %lu\n%s\n\n",
        NDO_API_ENDDATADUMP,
        NDO_API_ENDTIME,
        (unsigned long)time(NULL),
        NDO_API_GOODBYE);

    temp_buffer[sizeof(temp_buffer) - 1] = '\x0';

    ndomod_write_to_sink(temp_buffer, NDO_FALSE, NDO_TRUE);

    return NDO_OK;
}


/* used to rotate data sink file on a regular basis */
int ndomod_rotate_sink_file(void *args)
{
    char *raw_command_line_3x       = NULL;
    char *processed_command_line_3x = NULL;
    int early_timeout               = FALSE;
    double exectime                 = 0.0;

    /* close sink */
    ndomod_goodbye_sink();
    ndomod_close_sink();

    /* we shouldn't write any data to the sink while we're rotating it... */
    ndomod_allow_sink_activity = NDO_FALSE;

    /****** ROTATE THE FILE *****/

    /* get the raw command line */
    get_raw_command_line(find_command(ndomod_sink_rotation_command), 
        ndomod_sink_rotation_command, 
        &raw_command_line_3x, 
        STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS);
    strip(raw_command_line_3x);

    /* process any macros in the raw command line */
    process_macros(raw_command_line_3x, &processed_command_line_3x, STRIP_ILLEGAL_MACRO_CHARS | ESCAPE_MACRO_CHARS);

    /* run the command */
    my_system(processed_command_line_3x, ndomod_sink_rotation_timeout, &early_timeout, &exectime, NULL, 0);

    /* allow data to be written to the sink */
    ndomod_allow_sink_activity = NDO_TRUE;

    /* re-open sink */
    ndomod_open_sink();
    ndomod_hello_sink(TRUE, FALSE);

    return NDO_OK;
}


/* writes data to sink */
int ndomod_write_to_sink(char *buf, int buffer_write, int flush_buffer)
{
    char *temp_buffer            = NULL;
    char *sbuf                   = NULL;
    int buflen                   = 0;
    int result                   = NDO_OK;
    time_t current_time;
    int reconnect                = NDO_FALSE;
    unsigned long items_to_flush = 0L;

    /* we have nothing to write... */
    if (buf == NULL) {
        return NDO_OK;
    }

    /* we shouldn't be messing with things... */
    if (ndomod_allow_sink_activity == NDO_FALSE) {
        return NDO_ERROR;
    }

    /* open the sink if necessary... */
    if (ndomod_sink_is_open == NDO_FALSE) {

        time(&current_time);

        /* are we reopening the sink? */
        if (ndomod_sink_previously_open == NDO_TRUE) {
            reconnect = NDO_TRUE;
        }

        /* (re)connect to the sink if its time */
        if (((unsigned long)current_time - ndomod_sink_reconnect_interval) > ndomod_sink_last_reconnect_attempt) {

            result = ndomod_open_sink();

            ndomod_sink_last_reconnect_attempt = current_time;

            ndomod_sink_connect_attempt++;

            /* sink was (re)opened... */
            if (result == NDO_OK) {

                if (reconnect == NDO_TRUE) {
                    asprintf(&temp_buffer, "ndomod: Successfully reconnected to data sink!  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items);
                    ndomod_hello_sink(TRUE, TRUE);
                }
                else {
                    if (sinkbuf.overflow == 0L) {
                        asprintf(&temp_buffer, "ndomod: Successfully connected to data sink.  %lu queued items to flush.", sinkbuf.items);
                    }
                    else {
                        asprintf(&temp_buffer, "ndomod: Successfully connected to data sink.  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items);
                    }
                    ndomod_hello_sink(FALSE, FALSE);
                }

                ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
                free(temp_buffer);
                temp_buffer = NULL;

                /* reset sink overflow */
                sinkbuf.overflow = 0L;
            }

            /* sink could not be (re)opened... */
            else {

                if (((unsigned long)current_time - ndomod_sink_reconnect_warning_interval) > ndomod_sink_last_reconnect_warning) {

                    if (reconnect == NDO_TRUE) {
                        asprintf(&temp_buffer, "ndomod: Still unable to reconnect to data sink.  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items);
                    }
                    else if (ndomod_sink_connect_attempt == 1) {
                        asprintf(&temp_buffer, "ndomod: Could not open data sink!  I'll keep trying, but some output may get lost...");
                    }
                    else {
                        asprintf(&temp_buffer, "ndomod: Still unable to connect to data sink.  %lu items lost, %lu queued items to flush.", sinkbuf.overflow, sinkbuf.items);
                    }

                    ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
                    free(temp_buffer);
                    temp_buffer = NULL;

                    ndomod_sink_last_reconnect_warning = current_time;
                }
            }
        }
    }

    /* we weren't able to (re)connect */
    if (ndomod_sink_is_open == NDO_FALSE) {

        /***** BUFFER OUTPUT FOR LATER *****/

        if (buffer_write == NDO_TRUE) {
            ndomod_sink_buffer_push(&sinkbuf, buf);
        }

        return NDO_ERROR;
    }


    /***** FLUSH BUFFERED DATA FIRST *****/

    items_to_flush = ndomod_sink_buffer_items(&sinkbuf);
    if (flush_buffer == NDO_TRUE && items_to_flush > 0) {

        while (ndomod_sink_buffer_items(&sinkbuf) > 0) {

            /* get next item from buffer */
            sbuf = ndomod_sink_buffer_peek(&sinkbuf);

            buflen = strlen(sbuf);
            result = ndo_sink_write(ndomod_sink_fd, sbuf, buflen);

            /* an error occurred... */
            if (result < 0) {

                /* sink problem! */
                if (errno != EAGAIN) {

                    /* close the sink */
                    ndomod_close_sink();

                    asprintf(&temp_buffer, "ndomod: Error writing to data sink!  Some output may get lost.  %lu queued items to flush.", sinkbuf.items);
                    ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
                    free(temp_buffer);
                    temp_buffer = NULL;

                    time(&current_time);
                    ndomod_sink_last_reconnect_attempt = current_time;
                    ndomod_sink_last_reconnect_warning = current_time;
                }

                /***** BUFFER ORIGINAL OUTPUT FOR LATER *****/

                if (buffer_write == NDO_TRUE) {
                    ndomod_sink_buffer_push(&sinkbuf, buf);
                }

                return NDO_ERROR;
            }

            /* buffer was written okay, so remove it from buffer */
            ndomod_sink_buffer_pop(&sinkbuf);
        }

        asprintf(&temp_buffer, "ndomod: Successfully flushed %lu queued items to data sink.", items_to_flush);
        ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
        free(temp_buffer);
        temp_buffer = NULL;
    }


    /***** WRITE ORIGINAL DATA *****/

    /* write the data */
    buflen = strlen(buf);
    result = ndo_sink_write(ndomod_sink_fd, buf, buflen);

    /* an error occurred... */
    if (result < 0) {

        /* sink problem! */
        if (errno != EAGAIN) {

            /* close the sink */
            ndomod_close_sink();

            time(&current_time);
            ndomod_sink_last_reconnect_attempt = current_time;
            ndomod_sink_last_reconnect_warning = current_time;

            asprintf(&temp_buffer, "ndomod: Error writing to data sink!  Some output may get lost...");
            ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
            free(temp_buffer);
            asprintf(&temp_buffer, "ndomod: Please check remote ndo2db log, database connection or SSL Parameters");
            ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
            free(temp_buffer);

            temp_buffer = NULL;
        }

        /***** BUFFER OUTPUT FOR LATER *****/

        if (buffer_write == NDO_TRUE) {
            ndomod_sink_buffer_push(&sinkbuf, buf);
        }

        return NDO_ERROR;
    }

    return NDO_OK;
}



/* save unprocessed data to buffer file */
int ndomod_save_unprocessed_data(char *f)
{
    FILE *fp   = NULL;
    char *buf  = NULL;
    char *ebuf = NULL;

    /* no file */
    if (f == NULL) {
        return NDO_OK;
    }

    /* open the file for writing */
    fp = fopen(f, "w");
    if (fp == NULL) {
        return NDO_ERROR;
    }

    /* save all buffered items */
    while (ndomod_sink_buffer_items(&sinkbuf) > 0) {

        /* get next item from buffer */
        buf = ndomod_sink_buffer_pop(&sinkbuf);

        /* escape the string */
        ebuf = ndo_escape_buffer(buf);

        /* write string to file */
        fputs(ebuf, fp);
        fputs("\n", fp);

        /* free memory */
        free(buf);
        buf = NULL;
        free(ebuf);
        ebuf = NULL;
    }

    fclose(fp);

    return NDO_OK;
}



/* load unprocessed data from buffer file */
int ndomod_load_unprocessed_data(char *f)
{
    ndo_mmapfile *thefile = NULL;
    char *ebuf = NULL;
    char *buf = NULL;

    /* open the file */
    thefile = ndo_mmap_fopen(f);
    if (thefile == NULL) {
        return NDO_ERROR;
    }

    /* process each line of the file */
    while (ebuf = ndo_mmap_fgets(thefile)) {

        /* unescape string */
        buf = ndo_unescape_buffer(ebuf);

        /* save the data to the sink buffer */
        ndomod_sink_buffer_push(&sinkbuf, buf);

        /* free memory */
        free(ebuf);
    }

    /* close the file */
    ndo_mmap_fclose(thefile);

    /* remove the file so we don't process it again in the future */
    unlink(f);

    return NDO_OK;
}



/* initializes sink buffer */
int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf, unsigned long maxitems)
{
    unsigned long x;

    if (sbuf == NULL || maxitems <= 0) {
        return NDO_ERROR;
    }

    /* allocate memory for the buffer */
    sbuf->buffer = (char **)malloc(sizeof(char *)*maxitems);
    if (sbuf->buffer == NULL) {
        return NDO_ERROR;
    }

    for (x = 0; x < maxitems; x++) {
        sbuf->buffer[x] = NULL;
    }

    sbuf->size     = 0L;
    sbuf->head     = 0L;
    sbuf->tail     = 0L;
    sbuf->items    = 0L;
    sbuf->maxitems = maxitems;
    sbuf->overflow = 0L;

    return NDO_OK;
}


/* deinitializes sink buffer */
int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf)
{
    unsigned long x;

    if (sbuf == NULL) {
        return NDO_ERROR;
    }

    /* free any allocated memory */
    for (x = 0; x < sbuf->maxitems; x++) {
        free(sbuf->buffer[x]);
    }

    free(sbuf->buffer);
    sbuf->buffer = NULL;

    return NDO_OK;
}


/* buffers output */
int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf, char *buf)
{
    if (sbuf == NULL || buf == NULL) {
        return NDO_ERROR;
    }

    /* no space to store buffer */
    if (sbuf->buffer == NULL || sbuf->items == sbuf->maxitems) {
        sbuf->overflow++;
        return NDO_ERROR;
    }

    /* store buffer */
    sbuf->buffer[sbuf->head] = strdup(buf);
    if (sbuf->buffer[sbuf->head] == NULL) {
        return NDO_ERROR;
    }

    sbuf->head = (sbuf->head + 1) % sbuf->maxitems;
    sbuf->items++;

    return NDO_OK;
}


/* gets and removes next item from buffer */
char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf)
{
    char *buf = NULL;

    if (sbuf == NULL) {
        return NULL;
    }

    if (sbuf->buffer == NULL) {
        return NULL;
    }

    if (sbuf->items == 0) {
        return NULL;
    }

    /* remove item from buffer */
    buf = sbuf->buffer[sbuf->tail];
    sbuf->buffer[sbuf->tail] = NULL;
    sbuf->tail = (sbuf->tail + 1) % sbuf->maxitems;
    sbuf->items--;

    return buf;
}


/* gets next items from buffer */
char *ndomod_sink_buffer_peek(ndomod_sink_buffer *sbuf)
{
    char *buf = NULL;

    if (sbuf == NULL) {
        return NULL;
    }

    if (sbuf->buffer == NULL) {
        return NULL;
    }

    buf = sbuf->buffer[sbuf->tail];

    return buf;
}


/* returns number of items buffered */
int ndomod_sink_buffer_items(ndomod_sink_buffer *sbuf)
{
    if (sbuf == NULL) {
        return 0;
    }
    else {
        return sbuf->items;
    }
}


/* gets number of items lost due to buffer overflow */
unsigned long ndomod_sink_buffer_get_overflow(ndomod_sink_buffer *sbuf)
{
    if (sbuf == NULL) {
        return 0;
    }
    else {
        return sbuf->overflow;
    }
}


/* sets number of items lost due to buffer overflow */
int ndomod_sink_buffer_set_overflow(ndomod_sink_buffer *sbuf, unsigned long num)
{
    if (sbuf == NULL) {
        return 0;
    }
    else {
        sbuf->overflow = num;
    }

    return sbuf->overflow;
}


/****************************************************************************/
/* CALLBACK FUNCTIONS                                                       */
/****************************************************************************/

#define NDO_REGISTER_CALLBACK(_OPT, _CBTYPE, _SUCCESS_MSG)                                                  \
    do {                                                                                                    \
        if ((result == NDO_OK) && (ndomod_process_options & _OPT)) {                                        \
            result = neb_register_callback(_CBTYPE, ndomod_module_handle, priority, ndomod_broker_data);    \
            if (result == NDO_OK) {                                                                         \
                asprintf(&msg, "ndo registered for " _SUCCESS_MSG " data\n");                               \
                ndomod_write_to_logs(msg, NSLOG_INFO_MESSAGE);                                              \
                free(msg);                                                                                  \
            }                                                                                               \
        }                                                                                                   \
    } while (0)

/* registers for callbacks */
int ndomod_register_callbacks(void)
{
    int priority = 0;
    int result   = NDO_OK;
    char *msg    = NULL;

    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_PROCESS_DATA, NEBCALLBACK_PROCESS_DATA, "process");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_TIMED_EVENT_DATA, NEBCALLBACK_TIMED_EVENT_DATA, "timed event");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_LOG_DATA, NEBCALLBACK_LOG_DATA, "log");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_SYSTEM_COMMAND_DATA, NEBCALLBACK_SYSTEM_COMMAND_DATA, "system command");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_EVENT_HANDLER_DATA, NEBCALLBACK_EVENT_HANDLER_DATA, "event handler");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_NOTIFICATION_DATA, NEBCALLBACK_NOTIFICATION_DATA, "notification");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_SERVICE_CHECK_DATA, NEBCALLBACK_SERVICE_CHECK_DATA, "service check");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_HOST_CHECK_DATA, NEBCALLBACK_HOST_CHECK_DATA, "host check");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_COMMENT_DATA, NEBCALLBACK_COMMENT_DATA, "comment");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_DOWNTIME_DATA, NEBCALLBACK_DOWNTIME_DATA, "downtime");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_FLAPPING_DATA, NEBCALLBACK_FLAPPING_DATA, "flapping");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_PROGRAM_STATUS_DATA, NEBCALLBACK_PROGRAM_STATUS_DATA, "program status");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_HOST_STATUS_DATA, NEBCALLBACK_HOST_STATUS_DATA, "host status");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_SERVICE_STATUS_DATA, NEBCALLBACK_SERVICE_STATUS_DATA, "service status");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA, NEBCALLBACK_ADAPTIVE_PROGRAM_DATA, "adaptive program");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_ADAPTIVE_HOST_DATA, NEBCALLBACK_ADAPTIVE_HOST_DATA, "adaptive host");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA, NEBCALLBACK_ADAPTIVE_SERVICE_DATA, "adaptive service");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA, NEBCALLBACK_EXTERNAL_COMMAND_DATA, "external command");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_AGGREGATED_STATUS_DATA, NEBCALLBACK_AGGREGATED_STATUS_DATA, "aggregated status");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_RETENTION_DATA, NEBCALLBACK_RETENTION_DATA, "retention");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA, NEBCALLBACK_ACKNOWLEDGEMENT_DATA, "acknowledgement");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_STATECHANGE_DATA, NEBCALLBACK_STATE_CHANGE_DATA, "state change");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_CONTACT_STATUS_DATA, NEBCALLBACK_CONTACT_STATUS_DATA, "contact status");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA, NEBCALLBACK_ADAPTIVE_CONTACT_DATA, "adaptive contact");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_CONTACT_NOTIFICATION_DATA, NEBCALLBACK_CONTACT_NOTIFICATION_DATA, "contact");
    NDO_REGISTER_CALLBACK(NDOMOD_PROCESS_CONTACT_NOTIFICATION_METHOD_DATA, NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, "contact notification");

    return result;
}


/* deregisters callbacks */
int ndomod_deregister_callbacks(void)
{
    neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_LOG_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_RETENTION_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA, ndomod_broker_data);

    return NDO_OK;
}

static void ndomod_enddata_serialize(ndo_dbuf *dbufp)
{
    char temp[64];

    snprintf(temp, sizeof(temp) - 1, "\n%d\n\n", NDO_API_ENDDATA);
    temp[sizeof(temp) - 1] = '\x0';
    ndo_dbuf_strcat(dbufp, temp);
}

#define NDO_TEMPLEN 64
static void ndomod_broker_data_serialize(ndo_dbuf *dbufp, int datatype,
        struct ndo_broker_data *bd, size_t bdsize, int add_enddata)
{
    char temp[NDO_TEMPLEN]               = { 0 };
    int x                                = 0;
    struct ndo_broker_data * broker_data = NULL;

    /* Start everything out with the broker data type */
    snprintf(temp, NDO_TEMPLEN, "\n%d:", datatype);
    ndo_dbuf_strcat(dbufp, temp);

    /* Add each value */
    for (x = 0, broker_data = bd; x < bdsize; x++, broker_data++) {

        switch(broker_data->datatype) {

        case BD_INT:
            snprintf(temp, NDO_TEMPLEN, 
                "\n%d = %d", 
                broker_data->key,
                broker_data->value.integer);
            break;

        case BD_TIMEVAL:
            snprintf(temp, NDO_TEMPLEN, 
                "\n%d = %ld.%06ld", 
                broker_data->key,
                broker_data->value.timestamp.tv_sec, 
                broker_data->value.timestamp.tv_usec);
            break;

        case BD_STRING:
            snprintf(temp, NDO_TEMPLEN, 
                "\n%d = ", 
                broker_data->key);
            break;

        case BD_UNSIGNED_LONG:
            snprintf(temp, NDO_TEMPLEN, 
                "\n%d = %lu", 
                broker_data->key,
                broker_data->value.unsigned_long);
            break;

        case BD_FLOAT:
            snprintf(temp, NDO_TEMPLEN, 
                "\n%d = %.5lf", 
                broker_data->key,
                broker_data->value.floating_point);
            break;
        }

        ndo_dbuf_strcat(dbufp, temp);
        if (broker_data->datatype == BD_STRING) {
            ndo_dbuf_strcat(dbufp, broker_data->value.string);
        }
    }

    /* Close everything out with an NDO_API_ENDDATA marker */
    if (add_enddata == TRUE) {
        ndomod_enddata_serialize(dbufp);
    }
}

static void ndomod_customvars_serialize(customvariablesmember *customvars,
    ndo_dbuf *dbufp)
{
    customvariablesmember * tmp         = NULL;
    char * cvname                       = NULL;
    char * cvvalue                      = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = customvars; tmp != NULL; tmp = tmp->next) {

        cvname = ndo_escape_buffer(tmp->variable_name);
        cvvalue = ndo_escape_buffer(tmp->variable_value);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN, 
            "\n%d = %s:%d:%s",
            NDO_DATA_CUSTOMVARIABLE, 
            (NULL == cvname) ? "" : cvname,
            tmp->has_been_modified,
            (NULL == cvvalue) ? "" : cvvalue);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(cvname);
        my_free(cvvalue);
    }
}

static void ndomod_contactgroups_serialize(contactgroupsmember *contactgroups,
    ndo_dbuf *dbufp) 
{
    contactgroupsmember *tmp            = NULL;
    char *groupname                     = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = contactgroups; tmp != NULL; tmp = tmp->next) {

        groupname = ndo_escape_buffer(tmp->group_name);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN, 
            "\n%d = %s",
            NDO_DATA_CONTACTGROUP,
            (NULL == groupname) ? "" : groupname);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(groupname);
    }
}

static void ndomod_contacts_serialize(contactsmember *contacts,
    ndo_dbuf *dbufp, int varnum) 
{
    contactsmember *tmp                 = NULL;
    char *contact_name                  = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = contacts; tmp != NULL; tmp = tmp->next) {

        contact_name = ndo_escape_buffer(tmp->contact_name);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN, 
            "\n%d = %s", 
            varnum,
            (NULL == contact_name) ? "" : contact_name);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(contact_name);
    }
}


static void ndomod_hosts_serialize(hostsmember *hosts, ndo_dbuf *dbufp,
        int varnum) 
{
    hostsmember *tmp                    = NULL;
    char *host_name                     = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = hosts; tmp != NULL; tmp = tmp->next) {

        host_name = ndo_escape_buffer(tmp->host_name);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN, 
            "\n%d = %s", varnum,
            (NULL == host_name) ? "" : host_name);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(host_name);
    }
}

static void ndomod_services_serialize(servicesmember *services, ndo_dbuf *dbufp,
        int varnum) 
{
    servicesmember *tmp                 = NULL;
    char *host_name                     = NULL;
    char *service_description           = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = services; tmp != NULL; tmp = tmp->next) {

        host_name = ndo_escape_buffer(tmp->host_name);
        service_description = ndo_escape_buffer(tmp->service_description);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
            "\n%d = %s;%s", 
            varnum,
            (NULL == host_name) ? "" : host_name,
            (NULL == service_description) ? "" : service_description);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(host_name);
        my_free(service_description);
    }
}

static void ndomod_commands_serialize(commandsmember *commands, ndo_dbuf *dbufp,
        int varnum)
{
    commandsmember *tmp                 = NULL;
    char *command                       = NULL;
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    for (tmp = commands; tmp != NULL; tmp = tmp->next) {

        command = ndo_escape_buffer(tmp->command);

        snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
            "\n%d = %s", 
            varnum,
            (command == NULL) ? "" : command);

        ndo_dbuf_strcat(dbufp, temp_buffer);

        my_free(command);
    }
}

/* handles brokered event data */
int ndomod_broker_data(int event_type, void *data)
{
    ndo_dbuf dbuf;
    int x                                                 = 0;
    char temp_buffer[NDOMOD_MAX_BUFLEN]                   = { 0 };
    size_t tbsize                                         = sizeof(temp_buffer);
    int write_to_sink                                     = NDO_TRUE;
    host *temp_host                                       = NULL;
    service *temp_service                                 = NULL;
    contact *temp_contact                                 = NULL;
    customvariablesmember *temp_customvar                 = NULL;
    scheduled_downtime *temp_downtime                     = NULL;
    nagios_comment *temp_comment                          = NULL;
    char *es[9]                                           = { NULL };
    nebstruct_process_data *procdata                      = NULL;
    nebstruct_timed_event_data *eventdata                 = NULL;
    nebstruct_log_data *logdata                           = NULL;
    nebstruct_system_command_data *cmddata                = NULL;
    nebstruct_event_handler_data *ehanddata               = NULL;
    nebstruct_notification_data *notdata                  = NULL;
    nebstruct_service_check_data *scdata                  = NULL;
    nebstruct_host_check_data *hcdata                     = NULL;
    nebstruct_comment_data *comdata                       = NULL;
    nebstruct_downtime_data *downdata                     = NULL;
    nebstruct_flapping_data *flapdata                     = NULL;
    nebstruct_program_status_data *psdata                 = NULL;
    nebstruct_host_status_data *hsdata                    = NULL;
    nebstruct_service_status_data *ssdata                 = NULL;
    nebstruct_adaptive_program_data *apdata               = NULL;
    nebstruct_adaptive_host_data *ahdata                  = NULL;
    nebstruct_adaptive_service_data *asdata               = NULL;
    nebstruct_external_command_data *ecdata               = NULL;
    nebstruct_aggregated_status_data *agsdata             = NULL;
    nebstruct_retention_data *rdata                       = NULL;
    nebstruct_contact_notification_data *cnotdata         = NULL;
    nebstruct_contact_notification_method_data *cnotmdata = NULL;
    nebstruct_acknowledgement_data *ackdata               = NULL;
    nebstruct_statechange_data *schangedata               = NULL;
    nebstruct_contact_status_data *csdata                 = NULL;
    nebstruct_adaptive_contact_data *acdata               = NULL;
    int last_state                                        = -1;
    int last_hard_state                                   = -1;

    if (data == NULL) {
        return 0;
    }

    /* should we handle this type of data? */
    switch(event_type) {

    case NEBCALLBACK_PROCESS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_PROCESS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_TIMED_EVENT_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_TIMED_EVENT_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_LOG_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_LOG_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_SYSTEM_COMMAND_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_SYSTEM_COMMAND_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_EVENT_HANDLER_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_EVENT_HANDLER_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_NOTIFICATION_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_SERVICE_CHECK_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_SERVICE_CHECK_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_HOST_CHECK_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_HOST_CHECK_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_COMMENT_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_COMMENT_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_DOWNTIME_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_DOWNTIME_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_FLAPPING_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_FLAPPING_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_PROGRAM_STATUS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_PROGRAM_STATUS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_HOST_STATUS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_HOST_STATUS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_SERVICE_STATUS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_SERVICE_STATUS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_CONTACT_STATUS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_CONTACT_STATUS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_ADAPTIVE_HOST_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_HOST_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_EXTERNAL_COMMAND_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_AGGREGATED_STATUS_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_AGGREGATED_STATUS_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_RETENTION_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_RETENTION_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA)) {
            return 0;
        }
        break;
    case NEBCALLBACK_STATE_CHANGE_DATA:
        if (!(ndomod_process_options & NDOMOD_PROCESS_STATECHANGE_DATA)) {
            return 0;
        }
        break;
    default:
        break;
    }

    /* initialize dynamic buffer (2KB chunk size) */
    ndo_dbuf_init(&dbuf, 2048);

    /* handle the event */
    switch(event_type) {

    case NEBCALLBACK_PROCESS_DATA:

        procdata = (nebstruct_process_data *)data;
        if (procdata->type == NEBTYPE_PROCESS_START) {
            ndomod_write_active_objects();
        }

        {
            struct ndo_broker_data process_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, procdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, procdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, procdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, procdata->timestamp),
                NDODATA_STRING(NDO_DATA_PROGRAMNAME, "Nagios"),
                NDODATA_STRING(NDO_DATA_PROGRAMVERSION, get_program_version()),
                NDODATA_STRING(NDO_DATA_PROGRAMDATE, get_program_modification_date()),
                NDODATA_UNSIGNED_LONG(NDO_DATA_PROCESSID, (unsigned long)getpid() )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_PROCESSDATA,
                process_data,
                ARRAY_SIZE(process_data), 
                TRUE);
        }

        break;

    case NEBCALLBACK_TIMED_EVENT_DATA:

        eventdata = (nebstruct_timed_event_data *)data;

        switch(eventdata->event_type) {

        case EVENT_SERVICE_CHECK:

            temp_service = (service *)eventdata->event_data;

            es[0] = ndo_escape_buffer(temp_service->host_name);
            es[1] = ndo_escape_buffer(temp_service->description);

            {
                struct ndo_broker_data timed_event_data[] = {
                    NDODATA_INTEGER(NDO_DATA_TYPE, eventdata->type),
                    NDODATA_INTEGER(NDO_DATA_FLAGS, eventdata->flags),
                    NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, eventdata->attr),
                    NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, eventdata->timestamp),
                    NDODATA_INTEGER(NDO_DATA_EVENTTYPE, eventdata->event_type),
                    NDODATA_INTEGER(NDO_DATA_RECURRING, eventdata->recurring),
                    NDODATA_UNSIGNED_LONG(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
                    NDODATA_STRING(NDO_DATA_HOST, es[0]),
                    NDODATA_STRING(NDO_DATA_SERVICE, es[1] )
                };

                ndomod_broker_data_serialize(&dbuf, 
                    NDO_API_TIMEDEVENTDATA,
                    timed_event_data,
                    ARRAY_SIZE(timed_event_data),
                    TRUE);
            }

            ndomod_free_esc_buffers(es, 2);

            break;

        case EVENT_HOST_CHECK:

            temp_host = (host *)eventdata->event_data;

            es[0] = ndo_escape_buffer(temp_host->name);

            {
                struct ndo_broker_data timed_event_data[] = {
                    NDODATA_INTEGER(NDO_DATA_TYPE, eventdata->type),
                    NDODATA_INTEGER(NDO_DATA_FLAGS, eventdata->flags),
                    NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, eventdata->attr),
                    NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, eventdata->timestamp),
                    NDODATA_INTEGER(NDO_DATA_EVENTTYPE, eventdata->event_type),
                    NDODATA_INTEGER(NDO_DATA_RECURRING, eventdata->recurring),
                    NDODATA_UNSIGNED_LONG(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
                    NDODATA_STRING(NDO_DATA_HOST, es[0] )
                };

                ndomod_broker_data_serialize(&dbuf, 
                    NDO_API_TIMEDEVENTDATA,
                    timed_event_data,
                    ARRAY_SIZE(timed_event_data),
                    TRUE);
            }

            ndomod_free_esc_buffers(es, 1);

            break;

        case EVENT_SCHEDULED_DOWNTIME:

            temp_downtime = find_downtime(ANY_DOWNTIME, (unsigned long)eventdata->event_data);

            if (temp_downtime != NULL) {
                es[0] = ndo_escape_buffer(temp_downtime->host_name);
                es[1] = ndo_escape_buffer(temp_downtime->service_description);
            }

            {
                struct ndo_broker_data timed_event_data[] = {
                    NDODATA_INTEGER(NDO_DATA_TYPE, eventdata->type),
                    NDODATA_INTEGER(NDO_DATA_FLAGS, eventdata->flags),
                    NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, eventdata->attr),
                    NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, eventdata->timestamp),
                    NDODATA_INTEGER(NDO_DATA_EVENTTYPE, eventdata->event_type),
                    NDODATA_INTEGER(NDO_DATA_RECURRING, eventdata->recurring),
                    NDODATA_UNSIGNED_LONG(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
                    NDODATA_STRING(NDO_DATA_HOST, es[0]),
                    NDODATA_STRING(NDO_DATA_SERVICE, es[1] )
                };

                ndomod_broker_data_serialize(&dbuf, 
                    NDO_API_TIMEDEVENTDATA,
                    timed_event_data,
                    ARRAY_SIZE(timed_event_data),
                    TRUE);
            }

            ndomod_free_esc_buffers(es, 2);

            break;

        default:

            {
                struct ndo_broker_data timed_event_data[] = {
                    NDODATA_INTEGER(NDO_DATA_TYPE, eventdata->type),
                    NDODATA_INTEGER(NDO_DATA_FLAGS, eventdata->flags),
                    NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, eventdata->attr),
                    NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, eventdata->timestamp),
                    NDODATA_INTEGER(NDO_DATA_EVENTTYPE, eventdata->event_type),
                    NDODATA_INTEGER(NDO_DATA_RECURRING, eventdata->recurring),
                    NDODATA_UNSIGNED_LONG(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
                };

                ndomod_broker_data_serialize(&dbuf, 
                    NDO_API_TIMEDEVENTDATA,
                    timed_event_data,
                    ARRAY_SIZE(timed_event_data),
                    TRUE);
            }

            break;
            }

        break;

    case NEBCALLBACK_LOG_DATA:

        logdata = (nebstruct_log_data *)data;

        {
            struct ndo_broker_data log_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, logdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, logdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, logdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, logdata->timestamp),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LOGENTRYTIME, logdata->entry_time),
                NDODATA_INTEGER(NDO_DATA_LOGENTRYTYPE, logdata->data_type),
                NDODATA_STRING(NDO_DATA_LOGENTRY, logdata->data),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_LOGDATA, 
                log_data,
                ARRAY_SIZE(log_data),
                TRUE);
        }

        break;

    case NEBCALLBACK_SYSTEM_COMMAND_DATA:

        cmddata = (nebstruct_system_command_data *)data;

        es[0] = ndo_escape_buffer(cmddata->command_line);
        es[1] = ndo_escape_buffer(cmddata->output);
        es[2] = ndo_escape_buffer(cmddata->output);

        {
            struct ndo_broker_data system_command_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, cmddata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, cmddata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, cmddata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, cmddata->timestamp),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, cmddata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, cmddata->end_time),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, cmddata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[0]),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, cmddata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, cmddata->execution_time),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, cmddata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[1]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[2] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_SYSTEMCOMMANDDATA,
                system_command_data, 
                ARRAY_SIZE(system_command_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 3);

        break;

    case NEBCALLBACK_EVENT_HANDLER_DATA:

        ehanddata = (nebstruct_event_handler_data *)data;

        es[0] = ndo_escape_buffer(ehanddata->host_name);
        es[1] = ndo_escape_buffer(ehanddata->service_description);
        es[2] = ndo_escape_buffer(ehanddata->command_name);
        es[3] = ndo_escape_buffer(ehanddata->command_args);
        es[4] = ndo_escape_buffer(ehanddata->command_line);
        es[5] = ndo_escape_buffer(ehanddata->output);
        /* Preparing if eventhandler will have long_output in the future */
        es[6] = ndo_escape_buffer(ehanddata->output);

        {
            struct ndo_broker_data event_handler_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ehanddata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ehanddata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ehanddata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ehanddata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, ehanddata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, ehanddata->state),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, ehanddata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, ehanddata->end_time),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, ehanddata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[3]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[4]),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, ehanddata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, ehanddata->execution_time),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, ehanddata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[6] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_EVENTHANDLERDATA,
                event_handler_data,
                ARRAY_SIZE(event_handler_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 7);

        break;

    case NEBCALLBACK_NOTIFICATION_DATA:

        notdata = (nebstruct_notification_data *)data;

        es[0] = ndo_escape_buffer(notdata->host_name);
        es[1] = ndo_escape_buffer(notdata->service_description);
        es[2] = ndo_escape_buffer(notdata->output);
        /* Preparing if notifications will have long_output in the future */
        es[3] = ndo_escape_buffer(notdata->output);
        es[4] = ndo_escape_buffer(notdata->ack_author);
        es[5] = ndo_escape_buffer(notdata->ack_data);

        {
            struct ndo_broker_data notification_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, notdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, notdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, notdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, notdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONTYPE, notdata->notification_type),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, notdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, notdata->end_time),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONREASON, notdata->reason_type),
                NDODATA_INTEGER(NDO_DATA_STATE, notdata->state),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[3]),
                NDODATA_STRING(NDO_DATA_ACKAUTHOR, es[4]),
                NDODATA_STRING(NDO_DATA_ACKDATA, es[5]),
                NDODATA_INTEGER(NDO_DATA_ESCALATED, notdata->escalated),
                NDODATA_INTEGER(NDO_DATA_CONTACTSNOTIFIED, notdata->contacts_notified )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_NOTIFICATIONDATA,
                notification_data,
                ARRAY_SIZE(notification_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 6);

        break;

    case NEBCALLBACK_SERVICE_CHECK_DATA:

        scdata = (nebstruct_service_check_data *)data;

        /* Nagios XI MOD */
        /* send only the data we really use */
        if (scdata->type != NEBTYPE_SERVICECHECK_PROCESSED) {
            break;
        }

        es[0] = ndo_escape_buffer(scdata->host_name);
        es[1] = ndo_escape_buffer(scdata->service_description);
        es[2] = ndo_escape_buffer(scdata->command_name);
        es[3] = ndo_escape_buffer(scdata->command_args);
        es[4] = ndo_escape_buffer(scdata->command_line);
        es[5] = ndo_escape_buffer(scdata->output);
        es[6] = ndo_escape_buffer(scdata->long_output);
        es[7] = ndo_escape_buffer(scdata->perf_data);

        {
            struct ndo_broker_data service_check_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, scdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, scdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, scdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, scdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, scdata->check_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, scdata->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, scdata->max_attempts),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, scdata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, scdata->state),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, scdata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[3]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[4]),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, scdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, scdata->end_time),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, scdata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, scdata->execution_time),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, scdata->latency),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, scdata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[6]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[7] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_SERVICECHECKDATA,
                service_check_data,
                ARRAY_SIZE(service_check_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 8);

        break;

    case NEBCALLBACK_HOST_CHECK_DATA:

        hcdata = (nebstruct_host_check_data *)data;

        /* Nagios XI MOD */
        /* send only the data we really use */
        if (hcdata->type != NEBTYPE_HOSTCHECK_PROCESSED) {
            break;
        }

        es[0] = ndo_escape_buffer(hcdata->host_name);
        es[1] = ndo_escape_buffer(hcdata->command_name);
        es[2] = ndo_escape_buffer(hcdata->command_args);
        es[3] = ndo_escape_buffer(hcdata->command_line);
        es[4] = ndo_escape_buffer(hcdata->output);
        es[5] = ndo_escape_buffer(hcdata->long_output);
        es[6] = ndo_escape_buffer(hcdata->perf_data);

        {
            struct ndo_broker_data host_check_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, hcdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, hcdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, hcdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, hcdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, hcdata->check_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, hcdata->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, hcdata->max_attempts),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, hcdata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, hcdata->state),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, hcdata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[1]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[3]),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, hcdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, hcdata->end_time),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, hcdata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, hcdata->execution_time),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, hcdata->latency),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, hcdata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[4]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[6] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_HOSTCHECKDATA,
                host_check_data,
                ARRAY_SIZE(host_check_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 7);

        break;

    case NEBCALLBACK_COMMENT_DATA:

        comdata = (nebstruct_comment_data *)data;

        es[0] = ndo_escape_buffer(comdata->host_name);
        es[1] = ndo_escape_buffer(comdata->service_description);
        es[2] = ndo_escape_buffer(comdata->author_name);
        es[3] = ndo_escape_buffer(comdata->comment_data);

        {
            struct ndo_broker_data comment_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, comdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, comdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, comdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, comdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMENTTYPE, comdata->comment_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENTRYTIME, (unsigned long)comdata->entry_time),
                NDODATA_STRING(NDO_DATA_AUTHORNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMENT, es[3]),
                NDODATA_INTEGER(NDO_DATA_PERSISTENT, comdata->persistent),
                NDODATA_INTEGER(NDO_DATA_SOURCE, comdata->source),
                NDODATA_INTEGER(NDO_DATA_ENTRYTYPE, comdata->entry_type),
                NDODATA_INTEGER(NDO_DATA_EXPIRES, comdata->expires),
                NDODATA_UNSIGNED_LONG(NDO_DATA_EXPIRATIONTIME, (unsigned long)comdata->expire_time),
                NDODATA_UNSIGNED_LONG(NDO_DATA_COMMENTID, comdata->comment_id )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_COMMENTDATA,
                comment_data,
                ARRAY_SIZE(comment_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 4);

        break;

    case NEBCALLBACK_DOWNTIME_DATA:

        downdata = (nebstruct_downtime_data *)data;

        es[0] = ndo_escape_buffer(downdata->host_name);
        es[1] = ndo_escape_buffer(downdata->service_description);
        es[2] = ndo_escape_buffer(downdata->author_name);
        es[3] = ndo_escape_buffer(downdata->comment_data);

        {
            struct ndo_broker_data downtime_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, downdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, downdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, downdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, downdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_DOWNTIMETYPE, downdata->downtime_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENTRYTIME, (unsigned long)downdata->entry_time),
                NDODATA_STRING(NDO_DATA_AUTHORNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMENT, es[3]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_STARTTIME, (unsigned long)downdata->start_time),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENDTIME, (unsigned long)downdata->end_time),
                NDODATA_INTEGER(NDO_DATA_FIXED, downdata->fixed),
                NDODATA_UNSIGNED_LONG(NDO_DATA_DURATION, (unsigned long)downdata->duration),
                NDODATA_UNSIGNED_LONG(NDO_DATA_TRIGGEREDBY, (unsigned long)downdata->triggered_by),
                NDODATA_UNSIGNED_LONG(NDO_DATA_DOWNTIMEID, (unsigned long)downdata->downtime_id),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_DOWNTIMEDATA,
                downtime_data,
                ARRAY_SIZE(downtime_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 4);

        break;

    case NEBCALLBACK_FLAPPING_DATA:

        flapdata = (nebstruct_flapping_data *)data;

        es[0] = ndo_escape_buffer(flapdata->host_name);
        es[1] = ndo_escape_buffer(flapdata->service_description);

        if (flapdata->flapping_type == HOST_FLAPPING) {
            temp_comment = find_host_comment(flapdata->comment_id);
        }
        else {
            temp_comment = find_service_comment(flapdata->comment_id);
        }

        {
            struct ndo_broker_data flapping_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, flapdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, flapdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, flapdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, flapdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_FLAPPINGTYPE, flapdata->flapping_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_FLOATING_POINT(NDO_DATA_PERCENTSTATECHANGE, flapdata->percent_change),
                NDODATA_FLOATING_POINT(NDO_DATA_HIGHTHRESHOLD, flapdata->high_threshold),
                NDODATA_FLOATING_POINT(NDO_DATA_LOWTHRESHOLD, flapdata->low_threshold),
                NDODATA_UNSIGNED_LONG(NDO_DATA_COMMENTTIME, (temp_comment == NULL) ? 0L : (unsigned long)temp_comment->entry_time),
                NDODATA_UNSIGNED_LONG(NDO_DATA_COMMENTID, flapdata->comment_id )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_FLAPPINGDATA,
                flapping_data,
                ARRAY_SIZE(flapping_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 2);

        break;

    case NEBCALLBACK_PROGRAM_STATUS_DATA:

        psdata = (nebstruct_program_status_data *)data;

        es[0] = ndo_escape_buffer(psdata->global_host_event_handler);
        es[1] = ndo_escape_buffer(psdata->global_service_event_handler);

        {
            struct ndo_broker_data program_status_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, psdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, psdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, psdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, psdata->timestamp),
                NDODATA_UNSIGNED_LONG(NDO_DATA_PROGRAMSTARTTIME, (unsigned long)psdata->program_start),
                NDODATA_INTEGER(NDO_DATA_PROCESSID, psdata->pid),
                NDODATA_INTEGER(NDO_DATA_DAEMONMODE, psdata->daemon_mode),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTCOMMANDCHECK, 0L),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTLOGROTATION, psdata->last_log_rotation),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONSENABLED, psdata->notifications_enabled),
                NDODATA_INTEGER(NDO_DATA_ACTIVESERVICECHECKSENABLED, psdata->active_service_checks_enabled),
                NDODATA_INTEGER(NDO_DATA_PASSIVESERVICECHECKSENABLED, psdata->passive_service_checks_enabled),
                NDODATA_INTEGER(NDO_DATA_ACTIVEHOSTCHECKSENABLED, psdata->active_host_checks_enabled),
                NDODATA_INTEGER(NDO_DATA_PASSIVEHOSTCHECKSENABLED, psdata->passive_host_checks_enabled),
                NDODATA_INTEGER(NDO_DATA_EVENTHANDLERSENABLED, psdata->event_handlers_enabled),
                NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONENABLED, psdata->flap_detection_enabled),
                NDODATA_INTEGER(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
                NDODATA_INTEGER(NDO_DATA_PROCESSPERFORMANCEDATA, psdata->process_performance_data),
                NDODATA_INTEGER(NDO_DATA_OBSESSOVERHOSTS, psdata->obsess_over_hosts),
                NDODATA_INTEGER(NDO_DATA_OBSESSOVERSERVICES, psdata->obsess_over_services),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, psdata->modified_host_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, psdata->modified_service_attributes),
                NDODATA_STRING(NDO_DATA_GLOBALHOSTEVENTHANDLER, es[0]),
                NDODATA_STRING(NDO_DATA_GLOBALSERVICEEVENTHANDLER, es[1]),
                };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_PROGRAMSTATUSDATA,
                program_status_data,
                ARRAY_SIZE(program_status_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 2);

        break;

    case NEBCALLBACK_HOST_STATUS_DATA:

        hsdata = (nebstruct_host_status_data *)data;
        temp_host = (host *)hsdata->object_ptr;

        if (temp_host == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_host->name);
        es[1] = ndo_escape_buffer(temp_host->plugin_output);
        es[2] = ndo_escape_buffer(temp_host->long_plugin_output);
        es[3] = ndo_escape_buffer(temp_host->perf_data);
        es[4] = ndo_escape_buffer(temp_host->event_handler);
        es[5] = ndo_escape_buffer(temp_host->check_command);
        es[6] = ndo_escape_buffer(temp_host->check_period);

        {
            struct ndo_broker_data host_status_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, hsdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, hsdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, hsdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, hsdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[1]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[3]),
                NDODATA_INTEGER(NDO_DATA_CURRENTSTATE, temp_host->current_state),
                NDODATA_INTEGER(NDO_DATA_HASBEENCHECKED, temp_host->has_been_checked),
                NDODATA_INTEGER(NDO_DATA_SHOULDBESCHEDULED, temp_host->should_be_scheduled),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, temp_host->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, temp_host->max_attempts),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTHOSTCHECK, (unsigned long)temp_host->last_check),
                NDODATA_UNSIGNED_LONG(NDO_DATA_NEXTHOSTCHECK, (unsigned long)temp_host->next_check),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, temp_host->check_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTSTATECHANGE, (unsigned long)temp_host->last_state_change),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTHARDSTATECHANGE, (unsigned long)temp_host->last_hard_state_change),
                NDODATA_INTEGER(NDO_DATA_LASTHARDSTATE, temp_host->last_hard_state),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEUP, (unsigned long)temp_host->last_time_up),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEDOWN, (unsigned long)temp_host->last_time_down),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEUNREACHABLE, (unsigned long)temp_host->last_time_unreachable),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, temp_host->state_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTHOSTNOTIFICATION, (unsigned long)temp_host->last_notification),
                NDODATA_UNSIGNED_LONG(NDO_DATA_NEXTHOSTNOTIFICATION, (unsigned long)temp_host->next_notification),
                NDODATA_INTEGER(NDO_DATA_NOMORENOTIFICATIONS, temp_host->no_more_notifications),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONSENABLED, temp_host->notifications_enabled),
                NDODATA_INTEGER(NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, temp_host->problem_has_been_acknowledged),
                NDODATA_INTEGER(NDO_DATA_ACKNOWLEDGEMENTTYPE, temp_host->acknowledgement_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTNOTIFICATIONNUMBER, temp_host->current_notification_number),
                NDODATA_INTEGER(NDO_DATA_PASSIVEHOSTCHECKSENABLED, temp_host->accept_passive_checks),
                NDODATA_INTEGER(NDO_DATA_EVENTHANDLERENABLED, temp_host->event_handler_enabled),
                NDODATA_INTEGER(NDO_DATA_ACTIVEHOSTCHECKSENABLED, temp_host->checks_enabled),
                NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONENABLED, temp_host->flap_detection_enabled),
                NDODATA_INTEGER(NDO_DATA_ISFLAPPING, temp_host->is_flapping),
                NDODATA_FLOATING_POINT(NDO_DATA_PERCENTSTATECHANGE, temp_host->percent_state_change),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, temp_host->latency),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, temp_host->execution_time),
                NDODATA_INTEGER(NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, temp_host->scheduled_downtime_depth),
                NDODATA_INTEGER(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
                NDODATA_INTEGER(NDO_DATA_PROCESSPERFORMANCEDATA, temp_host->process_performance_data),
                NDODATA_INTEGER(NDO_DATA_OBSESSOVERHOST, temp_host->obsess),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, temp_host->modified_attributes),
                NDODATA_STRING(NDO_DATA_EVENTHANDLER, es[4]),
                NDODATA_STRING(NDO_DATA_CHECKCOMMAND, es[5]),
                NDODATA_FLOATING_POINT(NDO_DATA_NORMALCHECKINTERVAL, (double)temp_host->check_interval),
                NDODATA_FLOATING_POINT(NDO_DATA_RETRYCHECKINTERVAL, (double)temp_host->retry_interval),
                NDODATA_STRING(NDO_DATA_HOSTCHECKPERIOD, es[6] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_HOSTSTATUSDATA,
                host_status_data,
                ARRAY_SIZE(host_status_data),
                FALSE);
        }

        ndomod_customvars_serialize(temp_host->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_free_esc_buffers(es, 7);

        break;

    case NEBCALLBACK_SERVICE_STATUS_DATA:

        ssdata = (nebstruct_service_status_data *)data;
        temp_service = (service *)ssdata->object_ptr;

        if (temp_service == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_service->host_name);
        es[1] = ndo_escape_buffer(temp_service->description);
        es[2] = ndo_escape_buffer(temp_service->plugin_output);
        es[3] = ndo_escape_buffer(temp_service->long_plugin_output);
        es[4] = ndo_escape_buffer(temp_service->perf_data);
        es[5] = ndo_escape_buffer(temp_service->event_handler);
        es[6] = ndo_escape_buffer(temp_service->check_command);
        es[7] = ndo_escape_buffer(temp_service->check_period);

        {
            struct ndo_broker_data service_status_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ssdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ssdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ssdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ssdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[3]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[4]),
                NDODATA_INTEGER(NDO_DATA_CURRENTSTATE, temp_service->current_state),
                NDODATA_INTEGER(NDO_DATA_HASBEENCHECKED, temp_service->has_been_checked),
                NDODATA_INTEGER(NDO_DATA_SHOULDBESCHEDULED, temp_service->should_be_scheduled),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, temp_service->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, temp_service->max_attempts),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTSERVICECHECK, (unsigned long)temp_service->last_check),
                NDODATA_UNSIGNED_LONG(NDO_DATA_NEXTSERVICECHECK, (unsigned long)temp_service->next_check),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, temp_service->check_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTSTATECHANGE, (unsigned long)temp_service->last_state_change),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTHARDSTATECHANGE, (unsigned long)temp_service->last_hard_state_change),
                NDODATA_INTEGER(NDO_DATA_LASTHARDSTATE, temp_service->last_hard_state),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEOK, (unsigned long)temp_service->last_time_ok),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEWARNING, (unsigned long)temp_service->last_time_warning),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMEUNKNOWN, (unsigned long)temp_service->last_time_unknown),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTTIMECRITICAL, (unsigned long)temp_service->last_time_critical),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, temp_service->state_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTSERVICENOTIFICATION, (unsigned long)temp_service->last_notification),
                NDODATA_UNSIGNED_LONG(NDO_DATA_NEXTSERVICENOTIFICATION, (unsigned long)temp_service->next_notification),
                NDODATA_INTEGER(NDO_DATA_NOMORENOTIFICATIONS, temp_service->no_more_notifications),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONSENABLED, temp_service->notifications_enabled),
                NDODATA_INTEGER(NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, temp_service->problem_has_been_acknowledged),
                NDODATA_INTEGER(NDO_DATA_ACKNOWLEDGEMENTTYPE, temp_service->acknowledgement_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTNOTIFICATIONNUMBER, temp_service->current_notification_number),
                NDODATA_INTEGER(NDO_DATA_PASSIVESERVICECHECKSENABLED, temp_service->accept_passive_checks),
                NDODATA_INTEGER(NDO_DATA_EVENTHANDLERENABLED, temp_service->event_handler_enabled),
                NDODATA_INTEGER(NDO_DATA_ACTIVESERVICECHECKSENABLED, temp_service->checks_enabled),
                NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONENABLED, temp_service->flap_detection_enabled),
                NDODATA_INTEGER(NDO_DATA_ISFLAPPING, temp_service->is_flapping),
                NDODATA_FLOATING_POINT(NDO_DATA_PERCENTSTATECHANGE, temp_service->percent_state_change),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, temp_service->latency),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, temp_service->execution_time),
                NDODATA_INTEGER(NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, temp_service->scheduled_downtime_depth),
                NDODATA_INTEGER(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
                NDODATA_INTEGER(NDO_DATA_PROCESSPERFORMANCEDATA, temp_service->process_performance_data),
                NDODATA_INTEGER(NDO_DATA_OBSESSOVERSERVICE, temp_service->obsess),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, temp_service->modified_attributes),
                NDODATA_STRING(NDO_DATA_EVENTHANDLER, es[5]),
                NDODATA_STRING(NDO_DATA_CHECKCOMMAND, es[6]),
                NDODATA_FLOATING_POINT(NDO_DATA_NORMALCHECKINTERVAL, (double)temp_service->check_interval),
                NDODATA_FLOATING_POINT(NDO_DATA_RETRYCHECKINTERVAL, (double)temp_service->retry_interval),
                NDODATA_STRING(NDO_DATA_SERVICECHECKPERIOD, es[7] )
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_SERVICESTATUSDATA,
                service_status_data, 
                ARRAY_SIZE(service_status_data), 
                FALSE);
        }

        ndomod_customvars_serialize(temp_service->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_free_esc_buffers(es, 8);

        break;

    case NEBCALLBACK_CONTACT_STATUS_DATA:

        csdata = (nebstruct_contact_status_data *)data;
        temp_contact = (contact *)csdata->object_ptr;

        if (temp_contact == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_contact->name);

        {
            struct ndo_broker_data contact_status_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, csdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, csdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, csdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, csdata->timestamp),
                NDODATA_STRING(NDO_DATA_CONTACTNAME, es[0]),
                NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
                NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTHOSTNOTIFICATION, (unsigned long)temp_contact->last_host_notification),
                NDODATA_UNSIGNED_LONG(NDO_DATA_LASTSERVICENOTIFICATION, (unsigned long)temp_contact->last_service_notification),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDCONTACTATTRIBUTES, temp_contact->modified_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, temp_contact->modified_host_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, temp_contact->modified_service_attributes),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_CONTACTSTATUSDATA,
                contact_status_data, 
                ARRAY_SIZE(contact_status_data), 
                FALSE);
        }

        ndomod_customvars_serialize(temp_contact->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_free_esc_buffers(es, 1);

        break;

    case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:

        apdata = (nebstruct_adaptive_program_data *)data;

        es[0] = ndo_escape_buffer(global_host_event_handler);
        es[1] = ndo_escape_buffer(global_service_event_handler);

        {
            struct ndo_broker_data adaptive_program_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, apdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, apdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, apdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, apdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMANDTYPE, apdata->command_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTE, apdata->modified_host_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, apdata->modified_host_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, apdata->modified_service_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, apdata->modified_service_attributes),
                NDODATA_STRING(NDO_DATA_GLOBALHOSTEVENTHANDLER, es[0]),
                NDODATA_STRING(NDO_DATA_GLOBALSERVICEEVENTHANDLER, es[1]),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ADAPTIVEPROGRAMDATA,
                adaptive_program_data, 
                ARRAY_SIZE(adaptive_program_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 2);

        break;

    case NEBCALLBACK_ADAPTIVE_HOST_DATA:

        ahdata = (nebstruct_adaptive_host_data *)data;
        temp_host = (host *)ahdata->object_ptr;

        if (temp_host == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_host->name);
        es[1] = ndo_escape_buffer(temp_host->event_handler);
        es[2] = ndo_escape_buffer(temp_host->check_command);

        {
            struct ndo_broker_data adaptive_host_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ahdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ahdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ahdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ahdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMANDTYPE, ahdata->command_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTE, ahdata->modified_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, ahdata->modified_attributes),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_EVENTHANDLER, es[1]),
                NDODATA_STRING(NDO_DATA_CHECKCOMMAND, es[2]),
                NDODATA_FLOATING_POINT(NDO_DATA_NORMALCHECKINTERVAL, temp_host->check_interval),
                NDODATA_FLOATING_POINT(NDO_DATA_RETRYCHECKINTERVAL, (double)temp_host->retry_interval),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, temp_host->max_attempts),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ADAPTIVEHOSTDATA,
                adaptive_host_data,
                ARRAY_SIZE(adaptive_host_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 3);

        break;

    case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:

        asdata = (nebstruct_adaptive_service_data *)data;
        temp_service = (service *)asdata->object_ptr;

        if (temp_service == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_service->host_name);
        es[1] = ndo_escape_buffer(temp_service->description);
        es[2] = ndo_escape_buffer(temp_service->event_handler);
        es[3] = ndo_escape_buffer(temp_service->check_command);

        {
            struct ndo_broker_data adaptive_service_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, asdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, asdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, asdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, asdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMANDTYPE, asdata->command_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, asdata->modified_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, asdata->modified_attributes),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_STRING(NDO_DATA_EVENTHANDLER, es[2]),
                NDODATA_STRING(NDO_DATA_CHECKCOMMAND, es[3]),
                NDODATA_FLOATING_POINT(NDO_DATA_NORMALCHECKINTERVAL, (double)temp_service->check_interval),
                NDODATA_FLOATING_POINT(NDO_DATA_RETRYCHECKINTERVAL, (double)temp_service->retry_interval),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, temp_service->max_attempts),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ADAPTIVESERVICEDATA,
                adaptive_service_data, 
                ARRAY_SIZE(adaptive_service_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 4);

        break;

    case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:

        acdata = (nebstruct_adaptive_contact_data *)data;
        temp_contact = (contact *)acdata->object_ptr;

        if (temp_contact == NULL) {
            ndo_dbuf_free(&dbuf);
            return 0;
        }

        es[0] = ndo_escape_buffer(temp_contact->name);

        {
            struct ndo_broker_data adaptive_contact_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, acdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, acdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, acdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, acdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMANDTYPE, acdata->command_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDCONTACTATTRIBUTE, acdata->modified_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDCONTACTATTRIBUTES, acdata->modified_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTE, acdata->modified_host_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDHOSTATTRIBUTES, acdata->modified_host_attributes),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, acdata->modified_service_attribute),
                NDODATA_UNSIGNED_LONG(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, acdata->modified_service_attributes),
                NDODATA_STRING(NDO_DATA_CONTACTNAME, es[0]),
                NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
                NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ADAPTIVECONTACTDATA,
                adaptive_contact_data, 
                ARRAY_SIZE(adaptive_contact_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 1);

        break;

    case NEBCALLBACK_EXTERNAL_COMMAND_DATA:

        ecdata = (nebstruct_external_command_data *)data;

        es[0] = ndo_escape_buffer(ecdata->command_string);
        es[1] = ndo_escape_buffer(ecdata->command_args);

        {
            struct ndo_broker_data external_command_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ecdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ecdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ecdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ecdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMANDTYPE, ecdata->command_type),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENTRYTIME, (unsigned long)ecdata->entry_time),
                NDODATA_STRING(NDO_DATA_COMMANDSTRING, es[0]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[1]),
                };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_EXTERNALCOMMANDDATA,
                external_command_data, 
                ARRAY_SIZE(external_command_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 2);

        break;

    case NEBCALLBACK_AGGREGATED_STATUS_DATA:

        agsdata = (nebstruct_aggregated_status_data *)data;

        {
            struct ndo_broker_data aggregated_status_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, agsdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, agsdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, agsdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, agsdata->timestamp),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_AGGREGATEDSTATUSDATA,
                aggregated_status_data, 
                ARRAY_SIZE(aggregated_status_data), 
                TRUE);
        }

        break;

    case NEBCALLBACK_RETENTION_DATA:

        rdata = (nebstruct_retention_data *)data;

        {
            struct ndo_broker_data retention_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, rdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, rdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, rdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, rdata->timestamp),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_RETENTIONDATA,
                retention_data,
                ARRAY_SIZE(retention_data), 
                TRUE);
        }

        break;

    case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:

        cnotdata = (nebstruct_contact_notification_data *)data;

        es[0] = ndo_escape_buffer(cnotdata->host_name);
        es[1] = ndo_escape_buffer(cnotdata->service_description);
        es[2] = ndo_escape_buffer(cnotdata->output);
        /* Preparing long output for the future */
        es[3] = ndo_escape_buffer(cnotdata->output);
        /* Preparing for long_output in the future */
        es[4] = ndo_escape_buffer(cnotdata->output);
        es[5] = ndo_escape_buffer(cnotdata->ack_author);
        es[6] = ndo_escape_buffer(cnotdata->ack_data);
        es[7] = ndo_escape_buffer(cnotdata->contact_name);

        {
            struct ndo_broker_data contact_notification_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, cnotdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, cnotdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, cnotdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, cnotdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONTYPE, cnotdata->notification_type),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, cnotdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, cnotdata->end_time),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_STRING(NDO_DATA_CONTACTNAME, es[7]),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONREASON, cnotdata->reason_type),
                NDODATA_INTEGER(NDO_DATA_STATE, cnotdata->state),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[3]),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[4]),
                NDODATA_STRING(NDO_DATA_ACKAUTHOR, es[5]),
                NDODATA_STRING(NDO_DATA_ACKDATA, es[6]),
            };

            ndomod_broker_data_serialize(&dbuf,
                NDO_API_CONTACTNOTIFICATIONDATA, 
                contact_notification_data,
                ARRAY_SIZE(contact_notification_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 8);

        break;

    case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:

        cnotmdata = (nebstruct_contact_notification_method_data *)data;

        es[0] = ndo_escape_buffer(cnotmdata->host_name);
        es[1] = ndo_escape_buffer(cnotmdata->service_description);
        es[2] = ndo_escape_buffer(cnotmdata->output);
        es[3] = ndo_escape_buffer(cnotmdata->ack_author);
        es[4] = ndo_escape_buffer(cnotmdata->ack_data);
        es[5] = ndo_escape_buffer(cnotmdata->contact_name);
        es[6] = ndo_escape_buffer(cnotmdata->command_name);
        es[7] = ndo_escape_buffer(cnotmdata->command_args);

        {
            struct ndo_broker_data contact_notification_method_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, cnotmdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, cnotmdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, cnotmdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, cnotmdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONTYPE, cnotmdata->notification_type),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, cnotmdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, cnotmdata->end_time),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_STRING(NDO_DATA_CONTACTNAME, es[5]),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[6]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[7]),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONREASON, cnotmdata->reason_type),
                NDODATA_INTEGER(NDO_DATA_STATE, cnotmdata->state),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_ACKAUTHOR, es[3]),
                NDODATA_STRING(NDO_DATA_ACKDATA, es[4]),
            };

            ndomod_broker_data_serialize(&dbuf,
                NDO_API_CONTACTNOTIFICATIONMETHODDATA,
                contact_notification_method_data,
                ARRAY_SIZE(contact_notification_method_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 8);

        break;

    case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:

        ackdata = (nebstruct_acknowledgement_data *)data;

        es[0] = ndo_escape_buffer(ackdata->host_name);
        es[1] = ndo_escape_buffer(ackdata->service_description);
        es[2] = ndo_escape_buffer(ackdata->author_name);
        es[3] = ndo_escape_buffer(ackdata->comment_data);

        {
            struct ndo_broker_data acknowledgement_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ackdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ackdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ackdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ackdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_ACKNOWLEDGEMENTTYPE, ackdata->acknowledgement_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_STRING(NDO_DATA_AUTHORNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMENT, es[3]),
                NDODATA_INTEGER(NDO_DATA_STATE, ackdata->state),
                NDODATA_INTEGER(NDO_DATA_STICKY, ackdata->is_sticky),
                NDODATA_INTEGER(NDO_DATA_PERSISTENT, ackdata->persistent_comment),
                NDODATA_INTEGER(NDO_DATA_NOTIFYCONTACTS, ackdata->notify_contacts),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACKNOWLEDGEMENTDATA,
                acknowledgement_data,
                ARRAY_SIZE(acknowledgement_data), 
                TRUE);
        }

        ndomod_free_esc_buffers(es, 4);

        break;

    case NEBCALLBACK_STATE_CHANGE_DATA:

        schangedata = (nebstruct_statechange_data *)data;

        if (schangedata->service_description == NULL) {

            temp_host = (host *)schangedata->object_ptr;
            if (temp_host == NULL) {
                ndo_dbuf_free(&dbuf);
                return 0;
            }

            last_state = temp_host->last_state;
            last_hard_state = temp_host->last_hard_state;
        }
        else {

            temp_service = (service *)schangedata->object_ptr;
            if (temp_service == NULL) {
                ndo_dbuf_free(&dbuf);
                return 0;
            }

            last_state = temp_service->last_state;
            last_hard_state = temp_service->last_hard_state;
        }

        es[0] = ndo_escape_buffer(schangedata->host_name);
        es[1] = ndo_escape_buffer(schangedata->service_description);
        es[2] = ndo_escape_buffer(schangedata->output);
        if (CURRENT_OBJECT_STRUCTURE_VERSION >= 403 && has_ver403_long_output) {
            es[3] = ndo_escape_buffer(schangedata->longoutput);
        }
        else {
            es[3] = ndo_escape_buffer(schangedata->output);
        }

        {
            struct ndo_broker_data state_change_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, schangedata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, schangedata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, schangedata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, schangedata->timestamp),
                NDODATA_INTEGER(NDO_DATA_STATECHANGETYPE, schangedata->statechange_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_STATECHANGE, TRUE),
                NDODATA_INTEGER(NDO_DATA_STATE, schangedata->state),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, schangedata->state_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, schangedata->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, schangedata->max_attempts),
                NDODATA_INTEGER(NDO_DATA_LASTSTATE, last_state),
                NDODATA_INTEGER(NDO_DATA_LASTHARDSTATE, last_hard_state),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[3]),
            };

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_STATECHANGEDATA,
                state_change_data,
                ARRAY_SIZE(state_change_data),
                TRUE);
        }

        ndomod_free_esc_buffers(es, 4);

        break;

    default:
        ndo_dbuf_free(&dbuf);
        return 0;
        break;
    }

    /* write data to sink */
    if (write_to_sink == NDO_TRUE) {
        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
    }

    /* free dynamic buffer */
    ndo_dbuf_free(&dbuf);

    /* POST PROCESSING... */

    switch(event_type) {

    case NEBCALLBACK_PROCESS_DATA:

        procdata = (nebstruct_process_data *)data;

        /* process has passed pre-launch config verification, so dump original config */
        if (procdata->type == NEBTYPE_PROCESS_START) {

            ndomod_write_config_files();
            ndomod_write_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
        }

        /* process is starting the event loop, so dump runtime vars */
        if (procdata->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {

            ndomod_write_runtime_variables();
        }

        break;

    case NEBCALLBACK_RETENTION_DATA:

        rdata = (nebstruct_retention_data *)data;

        /* retained config was just read, so dump it */
        if (rdata->type == NEBTYPE_RETENTIONDATA_ENDLOAD) {

            ndomod_write_config(NDOMOD_CONFIG_DUMP_RETAINED);
        }

        break;

    default:
        break;
    }

    return 0;
}



/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* dumps all configuration data to sink */
int ndomod_write_config(int config_type)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];
    struct timeval now;
    int result;

    if (!(ndomod_config_output_options & config_type)) {
        return NDO_OK;
    }

    gettimeofday(&now, NULL);

    /* record start of config dump */
    snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
        "\n\n%d:\n%d = %s\n%d = %ld.%06ld\n%d\n\n",
        NDO_API_STARTCONFIGDUMP,
        NDO_DATA_CONFIGDUMPTYPE,
        (config_type == NDOMOD_CONFIG_DUMP_ORIGINAL) ? NDO_API_CONFIGDUMP_ORIGINAL : NDO_API_CONFIGDUMP_RETAINED,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

/*  ndomod_write_active_objects(); */

    /* dump object config info */
    result = ndomod_write_object_config(config_type);
    if (result != NDO_OK) {
        return result;
    }

    /* record end of config dump */
    snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
        "\n\n%d:\n%d = %ld.%06ld\n%d\n\n",
        NDO_API_ENDCONFIGDUMP,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_API_ENDDATA);

    result = ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

    return result;
}


/*************************************************************
 * Get a list of all active objects, so the "is_active" flag *
 * can be set on them in batches, instead of one at a time.  *
 *************************************************************/
void ndomod_write_active_objects()
{
    ndo_dbuf dbuf;
    struct ndo_broker_data active_objects[256];
    struct timeval now;
    command *temp_command           = NULL;
    timeperiod *temp_timeperiod     = NULL;
    contact *temp_contact           = NULL;
    contactgroup *temp_contactgroup = NULL;
    host *temp_host                 = NULL;
    hostgroup *temp_hostgroup       = NULL;
    service *temp_service           = NULL;
    servicegroup *temp_servicegroup = NULL;
    char *name1                     = NULL;
    char *name2                     = NULL;
    int i                           = 0;
    int obj_count                   = 0;

    /* get current time */
    gettimeofday(&now, NULL);

    /* initialize dynamic buffer (2KB chunk size) */
    ndo_dbuf_init(&dbuf, 2048);


    /********************************************************************************
    command definitions
    ********************************************************************************/
    active_objects[0].key = NDO_DATA_ACTIVEOBJECTSTYPE;
    active_objects[0].datatype = BD_INT;
    active_objects[0].value.integer = NDO_API_COMMANDDEFINITION;

    obj_count = 1;

    for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

        name1 = ndo_escape_buffer(temp_command->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects,
                obj_count,
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    time period definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_TIMEPERIODDEFINITION;
    obj_count = 1;

    for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) {

        name1 = ndo_escape_buffer(temp_timeperiod->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    contact definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_CONTACTDEFINITION;
    obj_count = 1;

    for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) {
        name1 = ndo_escape_buffer(temp_contact->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    contact group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_CONTACTGROUPDEFINITION;
    obj_count = 1;

    for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) {

        name1 = ndo_escape_buffer(temp_contactgroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    host definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_HOSTDEFINITION;
    obj_count = 1;

    for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

        name1 = ndo_escape_buffer(temp_host->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    host group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_HOSTGROUPDEFINITION;
    obj_count = 1;

    for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

        name1 = ndo_escape_buffer(temp_hostgroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    service definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_SERVICEDEFINITION;
    obj_count = 1;

    for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

        name1 = ndo_escape_buffer(temp_service->host_name);
        if (name1 == NULL) {
            continue;
        }

        name2 = ndo_escape_buffer(temp_service->description);
        if (name2 == NULL) {
            free(name1);
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        ++obj_count;

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name2;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    service group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_SERVICEGROUPDEFINITION;
    obj_count = 1;

    for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL ; temp_servicegroup = temp_servicegroup->next) {

        name1 = ndo_escape_buffer(temp_servicegroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }
}


static inline void ndomod_free_esc_buffers(char **ary, int num)
{
    int i = (num < 0 ? 0 : num);
    while (i--) {

        if (ary[i] != NULL) {
            free(ary[i]);
        }

        ary[i] = NULL;
    }
}

#define OBJECTCONFIG_ES_ITEMS 16

/* dumps object configuration data to sink */
int ndomod_write_object_config(int config_type)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };
    char *es[OBJECTCONFIG_ES_ITEMS] = { NULL };
    ndo_dbuf dbuf;
    struct timeval now;
    int x                                         = 0;
    command *temp_command                         = NULL;
    timeperiod *temp_timeperiod                   = NULL;
    timerange *temp_timerange                     = NULL;
    contact *temp_contact                         = NULL;
    commandsmember *temp_commandsmember           = NULL;
    contactgroup *temp_contactgroup               = NULL;
    host *temp_host                               = NULL;
    hostsmember *temp_hostsmember                 = NULL;
    contactgroupsmember *temp_contactgroupsmember = NULL;
    hostgroup *temp_hostgroup                     = NULL;
    service *temp_service                         = NULL;
    servicegroup *temp_servicegroup               = NULL;
    hostescalation *temp_hostescalation           = NULL;
    serviceescalation *temp_serviceescalation     = NULL;
    hostdependency *temp_hostdependency           = NULL;
    servicedependency *temp_servicedependency     = NULL;


    int have_2d_coords                            = FALSE;
    int x_2d                                      = 0;
    int y_2d                                      = 0;
    int have_3d_coords                            = FALSE;
    double x_3d                                   = 0.0;
    double y_3d                                   = 0.0;
    double z_3d                                   = 0.0;
    double first_notification_delay               = 0.0;
    double retry_interval                         = 0.0;
    int notify_on_host_downtime                   = 0;
    int notify_on_service_downtime                = 0;
    int host_notifications_enabled                = 0;
    int service_notifications_enabled             = 0;
    int can_submit_commands                       = 0;
    int flap_detection_on_up                      = 0;
    int flap_detection_on_down                    = 0;
    int flap_detection_on_unreachable             = 0;
    int flap_detection_on_ok                      = 0;
    int flap_detection_on_warning                 = 0;
    int flap_detection_on_unknown                 = 0;
    int flap_detection_on_critical                = 0;

    customvariablesmember *temp_customvar         = NULL;
    contactsmember *temp_contactsmember           = NULL;
    servicesmember *temp_servicesmember           = NULL;

    if (!(ndomod_process_options & NDOMOD_PROCESS_OBJECT_CONFIG_DATA)) {
        return NDO_OK;
    }

    if (!(ndomod_config_output_options & config_type)) {
        return NDO_OK;
    }

    /* get current time */
    gettimeofday(&now, NULL);

    /* initialize dynamic buffer (2KB chunk size) */
    ndo_dbuf_init(&dbuf, 2048);


    /****** dump command config ******/
    for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

        es[0] = ndo_escape_buffer(temp_command->name);
        es[1] = ndo_escape_buffer(temp_command->command_line);

        struct ndo_broker_data command_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_COMMANDNAME, es[0]),
            NDODATA_STRING(NDO_DATA_COMMANDLINE, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_COMMANDDEFINITION, 
            command_definition,
            ARRAY_SIZE(command_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump timeperiod config ******/
    for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) {

        es[0] = ndo_escape_buffer(temp_timeperiod->name);
        es[1] = ndo_escape_buffer(temp_timeperiod->alias);

        struct ndo_broker_data timeperiod_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_TIMEPERIODNAME, es[0]),
            NDODATA_STRING(NDO_DATA_TIMEPERIODALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_TIMEPERIODDEFINITION, 
            timeperiod_definition, 
            ARRAY_SIZE(timeperiod_definition), 
            FALSE);

        /* dump timeranges for each day */
        for (x = 0; x < 7; x++) {
            for (temp_timerange = temp_timeperiod->days[x]; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

                snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
                    "\n%d = %d:%lu-%lu",
                    NDO_DATA_TIMERANGE,
                    x,
                    temp_timerange->range_start,
                    temp_timerange->range_end);
                
                ndo_dbuf_strcat(&dbuf, temp_buffer);
            }
        }

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump contact config ******/
    for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) {

        es[0] = ndo_escape_buffer(temp_contact->name);
        es[1] = ndo_escape_buffer(temp_contact->alias);
        es[2] = ndo_escape_buffer(temp_contact->email);
        es[3] = ndo_escape_buffer(temp_contact->pager);
        es[4] = ndo_escape_buffer(temp_contact->host_notification_period);
        es[5] = ndo_escape_buffer(temp_contact->service_notification_period);

        struct ndo_broker_data contact_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_CONTACTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_CONTACTALIAS, es[1]),
            NDODATA_STRING(NDO_DATA_EMAILADDRESS, es[2]),
            NDODATA_STRING(NDO_DATA_PAGERADDRESS, es[3]),
            NDODATA_STRING(NDO_DATA_HOSTNOTIFICATIONPERIOD, es[4]),
            NDODATA_STRING(NDO_DATA_SERVICENOTIFICATIONPERIOD, es[5]),
            NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_CANSUBMITCOMMANDS, temp_contact->can_submit_commands),

            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEUNKNOWN, flag_isset(temp_contact->service_notification_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEWARNING, flag_isset(temp_contact->service_notification_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICECRITICAL, flag_isset(temp_contact->service_notification_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICERECOVERY, flag_isset(temp_contact->service_notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEFLAPPING, flag_isset(temp_contact->service_notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEDOWNTIME, flag_isset(temp_contact->service_notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWN, flag_isset(temp_contact->host_notification_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTUNREACHABLE, flag_isset(temp_contact->host_notification_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTRECOVERY, flag_isset(temp_contact->host_notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTFLAPPING, flag_isset(temp_contact->host_notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWNTIME, flag_isset(temp_contact->host_notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_MINIMUMIMPORTANCE, temp_contact->minimum_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_CONTACTDEFINITION, 
            contact_definition, 
            ARRAY_SIZE(contact_definition), 
            FALSE);

        ndomod_free_esc_buffers(es, 6);

        /* dump addresses for each contact */
        for (x = 0; x < MAX_CONTACT_ADDRESSES; x++) {

            es[0] = ndo_escape_buffer(temp_contact->address[x]);

            snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
                "\n%d = %d:%s",
                NDO_DATA_CONTACTADDRESS,
                x + 1,
                (es[0] == NULL) ? "" : es[0]);

            ndo_dbuf_strcat(&dbuf, temp_buffer);

            free(es[0]);
        }

        /* dump host notification commands for each contact */
        ndomod_commands_serialize(temp_contact->host_notification_commands,
                &dbuf, NDO_DATA_HOSTNOTIFICATIONCOMMAND);

        /* dump service notification commands for each contact */
        ndomod_commands_serialize(temp_contact->service_notification_commands,
                &dbuf, NDO_DATA_SERVICENOTIFICATIONCOMMAND);

        /* dump customvars */
        ndomod_customvars_serialize(temp_contact->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndo_dbuf_free(&dbuf);
    }


    /****** dump contactgroup config ******/
    for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) {

        es[0] = ndo_escape_buffer(temp_contactgroup->group_name);
        es[1] = ndo_escape_buffer(temp_contactgroup->alias);

        struct ndo_broker_data contactgroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_CONTACTGROUPDEFINITION, 
            contactgroup_definition, 
            ARRAY_SIZE(contactgroup_definition), 
            FALSE);

        /* dump members for each contactgroup */
        ndomod_contacts_serialize(temp_contactgroup->members, &dbuf,
                NDO_DATA_CONTACTGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host config ******/
    for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

        es[0] = ndo_escape_buffer(temp_host->name);
        es[1] = ndo_escape_buffer(temp_host->alias);
        es[2] = ndo_escape_buffer(temp_host->address);
        es[3] = ndo_escape_buffer(temp_host->check_command);
        es[4] = ndo_escape_buffer(temp_host->event_handler);
        es[5] = ndo_escape_buffer(temp_host->notification_period);
        es[6] = ndo_escape_buffer(temp_host->check_period);
        es[7] = ndo_escape_buffer("");

        es[8] = ndo_escape_buffer(temp_host->notes);
        es[9] = ndo_escape_buffer(temp_host->notes_url);
        es[10] = ndo_escape_buffer(temp_host->action_url);
        es[11] = ndo_escape_buffer(temp_host->icon_image);
        es[12] = ndo_escape_buffer(temp_host->icon_image_alt);
        es[13] = ndo_escape_buffer(temp_host->vrml_image);
        es[14] = ndo_escape_buffer(temp_host->statusmap_image);
        es[15] = ndo_escape_buffer(temp_host->display_name);


        struct ndo_broker_data host_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DISPLAYNAME, es[15]),
            NDODATA_STRING(NDO_DATA_HOSTALIAS, es[1]),
            NDODATA_STRING(NDO_DATA_HOSTADDRESS, es[2]),
            NDODATA_STRING(NDO_DATA_HOSTCHECKCOMMAND, es[3]),
            NDODATA_STRING(NDO_DATA_HOSTEVENTHANDLER, es[4]),
            NDODATA_STRING(NDO_DATA_HOSTNOTIFICATIONPERIOD, es[5]),
            NDODATA_STRING(NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS, es[7]),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTCHECKINTERVAL, (double)temp_host->check_interval),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTRETRYINTERVAL, (double)temp_host->retry_interval),
            NDODATA_INTEGER(NDO_DATA_HOSTMAXCHECKATTEMPTS, temp_host->max_attempts),
            NDODATA_FLOATING_POINT(NDO_DATA_FIRSTNOTIFICATIONDELAY, temp_host->first_notification_delay),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTNOTIFICATIONINTERVAL, (double)temp_host->notification_interval),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWN, flag_isset(temp_host->notification_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTUNREACHABLE, flag_isset(temp_host->notification_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTRECOVERY, flag_isset(temp_host->notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTFLAPPING, flag_isset(temp_host->notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWNTIME, flag_isset(temp_host->notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_HOSTFLAPDETECTIONENABLED, temp_host->flap_detection_enabled),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUP, flag_isset(temp_host->flap_detection_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONDOWN, flag_isset(temp_host->flap_detection_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUNREACHABLE, flag_isset(temp_host->flap_detection_options, OPT_UNREACHABLE)),
            NDODATA_FLOATING_POINT(NDO_DATA_LOWHOSTFLAPTHRESHOLD, temp_host->low_flap_threshold),
            NDODATA_FLOATING_POINT(NDO_DATA_HIGHHOSTFLAPTHRESHOLD, temp_host->high_flap_threshold),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONUP, flag_isset(temp_host->stalking_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONDOWN, flag_isset(temp_host->stalking_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONUNREACHABLE, flag_isset(temp_host->stalking_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_HOSTFRESHNESSCHECKSENABLED, temp_host->check_freshness),
            NDODATA_INTEGER(NDO_DATA_HOSTFRESHNESSTHRESHOLD, temp_host->freshness_threshold),
            NDODATA_INTEGER(NDO_DATA_PROCESSHOSTPERFORMANCEDATA, temp_host->process_performance_data),
            NDODATA_INTEGER(NDO_DATA_ACTIVEHOSTCHECKSENABLED, temp_host->checks_enabled),
            NDODATA_INTEGER(NDO_DATA_PASSIVEHOSTCHECKSENABLED, temp_host->accept_passive_checks),
            NDODATA_INTEGER(NDO_DATA_HOSTEVENTHANDLERENABLED, temp_host->event_handler_enabled),
            NDODATA_INTEGER(NDO_DATA_RETAINHOSTSTATUSINFORMATION, temp_host->retain_status_information),
            NDODATA_INTEGER(NDO_DATA_RETAINHOSTNONSTATUSINFORMATION, temp_host->retain_nonstatus_information),
            NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_host->notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_HOSTFAILUREPREDICTIONENABLED, 0),
            NDODATA_INTEGER(NDO_DATA_OBSESSOVERHOST, temp_host->obsess),
            NDODATA_STRING(NDO_DATA_NOTES, es[8]),
            NDODATA_STRING(NDO_DATA_NOTESURL, es[9]),
            NDODATA_STRING(NDO_DATA_ACTIONURL, es[10]),
            NDODATA_STRING(NDO_DATA_ICONIMAGE, es[11]),
            NDODATA_STRING(NDO_DATA_ICONIMAGEALT, es[12]),
            NDODATA_STRING(NDO_DATA_VRMLIMAGE, es[13]),
            NDODATA_STRING(NDO_DATA_STATUSMAPIMAGE, es[14]),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPALIAS, es[1]),
            NDODATA_INTEGER(NDO_DATA_HAVE2DCOORDS, temp_host->have_2d_coords),
            NDODATA_INTEGER(NDO_DATA_X2D, temp_host->x_2d),
            NDODATA_INTEGER(NDO_DATA_Y2D, temp_host->y_2d),
            NDODATA_INTEGER(NDO_DATA_HAVE3DCOORDS, temp_host->have_3d_coords),
            NDODATA_FLOATING_POINT(NDO_DATA_X3D, temp_host->y_3d),
            NDODATA_FLOATING_POINT(NDO_DATA_Y3D, temp_host->y_3d),
            NDODATA_FLOATING_POINT(NDO_DATA_Z3D, temp_host->z_3d),
            NDODATA_INTEGER(NDO_DATA_IMPORTANCE, temp_host->hourly_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_HOSTDEFINITION, 
            host_definition, 
            ARRAY_SIZE(host_definition), 
            FALSE);

        /* dump parent hosts */
        ndomod_hosts_serialize(temp_host->parent_hosts, &dbuf,
                NDO_DATA_PARENTHOST);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_host->contact_groups, &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_host->contacts, &dbuf, NDO_DATA_CONTACT);

        /* dump customvars */
        ndomod_customvars_serialize(temp_host->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, OBJECTCONFIG_ES_ITEMS);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump hostgroup config ******/
    for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

        es[0] = ndo_escape_buffer(temp_hostgroup->group_name);
        es[1] = ndo_escape_buffer(temp_hostgroup->alias);

        struct ndo_broker_data hostgroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_HOSTGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_HOSTGROUPDEFINITION,
            hostgroup_definition, 
            ARRAY_SIZE(hostgroup_definition), 
            FALSE);

        /* dump members for each hostgroup */
        ndomod_hosts_serialize(temp_hostgroup->members, &dbuf,
                NDO_DATA_HOSTGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service config ******/
    for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

        es[0] = ndo_escape_buffer(temp_service->host_name);
        es[1] = ndo_escape_buffer(temp_service->description);
        es[2] = ndo_escape_buffer(temp_service->check_command);
        es[3] = ndo_escape_buffer(temp_service->event_handler);
        es[4] = ndo_escape_buffer(temp_service->notification_period);
        es[5] = ndo_escape_buffer(temp_service->check_period);
        es[6] = ndo_escape_buffer("");
        es[7] = ndo_escape_buffer(temp_service->notes);
        es[8] = ndo_escape_buffer(temp_service->notes_url);
        es[9] = ndo_escape_buffer(temp_service->action_url);
        es[10] = ndo_escape_buffer(temp_service->icon_image);
        es[11] = ndo_escape_buffer(temp_service->icon_image_alt);
        es[12] = ndo_escape_buffer(temp_service->display_name);

        struct ndo_broker_data service_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DISPLAYNAME, es[12]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_SERVICECHECKCOMMAND, es[2]),
            NDODATA_STRING(NDO_DATA_SERVICEEVENTHANDLER, es[3]),
            NDODATA_STRING(NDO_DATA_SERVICENOTIFICATIONPERIOD, es[4]),
            NDODATA_STRING(NDO_DATA_SERVICECHECKPERIOD, es[5]),
            NDODATA_STRING(NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS, es[6]),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICECHECKINTERVAL, (double)temp_service->check_interval),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICERETRYINTERVAL, (double)temp_service->retry_interval),
            NDODATA_INTEGER(NDO_DATA_MAXSERVICECHECKATTEMPTS, temp_service->max_attempts),
            NDODATA_FLOATING_POINT(NDO_DATA_FIRSTNOTIFICATIONDELAY, temp_service->first_notification_delay),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICENOTIFICATIONINTERVAL, (double)temp_service->notification_interval),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEUNKNOWN, flag_isset(temp_service->notification_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEWARNING, flag_isset(temp_service->notification_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICECRITICAL, flag_isset(temp_service->notification_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICERECOVERY, flag_isset(temp_service->notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEFLAPPING, flag_isset(temp_service->notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEDOWNTIME, flag_isset(temp_service->notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONOK, flag_isset(temp_service->stalking_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONWARNING, flag_isset(temp_service->stalking_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONUNKNOWN, flag_isset(temp_service->stalking_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONCRITICAL, flag_isset(temp_service->stalking_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_SERVICEISVOLATILE, temp_service->is_volatile),
            NDODATA_INTEGER(NDO_DATA_SERVICEFLAPDETECTIONENABLED, temp_service->flap_detection_enabled),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONOK, flag_isset(temp_service->flap_detection_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONWARNING, flag_isset(temp_service->flap_detection_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUNKNOWN, flag_isset(temp_service->flap_detection_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONCRITICAL, flag_isset(temp_service->flap_detection_options, OPT_CRITICAL)),
            NDODATA_FLOATING_POINT(NDO_DATA_LOWSERVICEFLAPTHRESHOLD, temp_service->low_flap_threshold),
            NDODATA_FLOATING_POINT(NDO_DATA_HIGHSERVICEFLAPTHRESHOLD, temp_service->high_flap_threshold),
            NDODATA_INTEGER(NDO_DATA_PROCESSSERVICEPERFORMANCEDATA, temp_service->process_performance_data),
            NDODATA_INTEGER(NDO_DATA_SERVICEFRESHNESSCHECKSENABLED, temp_service->check_freshness),
            NDODATA_INTEGER(NDO_DATA_SERVICEFRESHNESSTHRESHOLD, temp_service->freshness_threshold),
            NDODATA_INTEGER(NDO_DATA_PASSIVESERVICECHECKSENABLED, temp_service->accept_passive_checks),
            NDODATA_INTEGER(NDO_DATA_SERVICEEVENTHANDLERENABLED, temp_service->event_handler_enabled),
            NDODATA_INTEGER(NDO_DATA_ACTIVESERVICECHECKSENABLED, temp_service->checks_enabled),
            NDODATA_INTEGER(NDO_DATA_RETAINSERVICESTATUSINFORMATION, temp_service->retain_status_information),
            NDODATA_INTEGER(NDO_DATA_RETAINSERVICENONSTATUSINFORMATION, temp_service->retain_nonstatus_information),
            NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_service->notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_OBSESSOVERSERVICE, temp_service->obsess),
            NDODATA_INTEGER(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
            NDODATA_STRING(NDO_DATA_NOTES, es[7]),
            NDODATA_STRING(NDO_DATA_NOTESURL, es[8]),
            NDODATA_STRING(NDO_DATA_ACTIONURL, es[9]),
            NDODATA_STRING(NDO_DATA_ICONIMAGE, es[10]),
            NDODATA_STRING(NDO_DATA_ICONIMAGEALT, es[11]),
            NDODATA_INTEGER(NDO_DATA_IMPORTANCE, temp_service->hourly_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_SERVICEDEFINITION,
            service_definition, 
            ARRAY_SIZE(service_definition), 
            FALSE);

        /* dump parent services */
        ndomod_services_serialize(temp_service->parents, &dbuf,
                NDO_DATA_PARENTSERVICE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_service->contact_groups, &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_service->contacts, &dbuf,
                NDO_DATA_CONTACT);

        /* dump customvars */
        ndomod_customvars_serialize(temp_service->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 13);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump servicegroup config ******/
    for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

        es[0] = ndo_escape_buffer(temp_servicegroup->group_name);
        es[1] = ndo_escape_buffer(temp_servicegroup->alias);

        struct ndo_broker_data servicegroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_SERVICEGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_SERVICEGROUPDEFINITION,
            servicegroup_definition, 
            ARRAY_SIZE(servicegroup_definition), 
            FALSE);

        /* dump members for each servicegroup */
        ndomod_services_serialize(temp_servicegroup->members, &dbuf,
                NDO_DATA_SERVICEGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host escalation config ******/
    for (x = 0; x < num_objects.hostescalations; x++) {
        temp_hostescalation = hostescalation_ary[ x];
        es[0] = ndo_escape_buffer(temp_hostescalation->host_name);
        es[1] = ndo_escape_buffer(temp_hostescalation->escalation_period);

        struct ndo_broker_data hostescalation_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_ESCALATIONPERIOD, es[1]),
            NDODATA_INTEGER(NDO_DATA_FIRSTNOTIFICATION, temp_hostescalation->first_notification),
            NDODATA_INTEGER(NDO_DATA_LASTNOTIFICATION, temp_hostescalation->last_notification),
            NDODATA_FLOATING_POINT(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_hostescalation->notification_interval),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONRECOVERY, flag_isset(temp_hostescalation->escalation_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONDOWN, flag_isset(temp_hostescalation->escalation_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONUNREACHABLE, flag_isset(temp_hostescalation->escalation_options, OPT_UNREACHABLE)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_HOSTESCALATIONDEFINITION,
            hostescalation_definition,
            ARRAY_SIZE(hostescalation_definition), 
            FALSE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_hostescalation->contact_groups,
                &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_hostescalation->contacts, &dbuf,
                NDO_DATA_CONTACT);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service escalation config ******/
    for (x = 0; x < num_objects.serviceescalations; x++) {
        temp_serviceescalation = serviceescalation_ary[ x];

        es[0] = ndo_escape_buffer(temp_serviceescalation->host_name);
        es[1] = ndo_escape_buffer(temp_serviceescalation->description);
        es[2] = ndo_escape_buffer(temp_serviceescalation->escalation_period);

        struct ndo_broker_data serviceescalation_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_ESCALATIONPERIOD, es[2]),
            NDODATA_INTEGER(NDO_DATA_FIRSTNOTIFICATION, temp_serviceescalation->first_notification),
            NDODATA_INTEGER(NDO_DATA_LASTNOTIFICATION, temp_serviceescalation->last_notification),
            NDODATA_FLOATING_POINT(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_serviceescalation->notification_interval),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONRECOVERY, flag_isset(temp_serviceescalation->escalation_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONWARNING, flag_isset(temp_serviceescalation->escalation_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONUNKNOWN, flag_isset(temp_serviceescalation->escalation_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONCRITICAL, flag_isset(temp_serviceescalation->escalation_options, OPT_CRITICAL)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_SERVICEESCALATIONDEFINITION,
            serviceescalation_definition,
            ARRAY_SIZE(serviceescalation_definition), 
            FALSE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_serviceescalation->contact_groups,
                &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_serviceescalation->contacts, &dbuf,
                NDO_DATA_CONTACT);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 3);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host dependency config ******/
    for (x = 0; x < num_objects.hostdependencies; x++) {
        temp_hostdependency = hostdependency_ary[ x];

        es[0] = ndo_escape_buffer(temp_hostdependency->host_name);
        es[1] = ndo_escape_buffer(temp_hostdependency->dependent_host_name);
        es[2] = ndo_escape_buffer(temp_hostdependency->dependency_period);

        struct ndo_broker_data hostdependency_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DEPENDENTHOSTNAME, es[1]),
            NDODATA_INTEGER(NDO_DATA_DEPENDENCYTYPE, temp_hostdependency->dependency_type),
            NDODATA_INTEGER(NDO_DATA_INHERITSPARENT, temp_hostdependency->inherits_parent),
            NDODATA_STRING(NDO_DATA_DEPENDENCYPERIOD, es[2]),
            NDODATA_INTEGER(NDO_DATA_FAILONUP, flag_isset(temp_hostdependency->failure_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_FAILONDOWN, flag_isset(temp_hostdependency->failure_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_FAILONUNREACHABLE, flag_isset(temp_hostdependency->failure_options, OPT_UNREACHABLE)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_HOSTDEPENDENCYDEFINITION,
            hostdependency_definition,
            ARRAY_SIZE(hostdependency_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 3);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service dependency config ******/
    for (x = 0; x < num_objects.servicedependencies; x++) {
        temp_servicedependency = servicedependency_ary[ x];

        es[0] = ndo_escape_buffer(temp_servicedependency->host_name);
        es[1] = ndo_escape_buffer(temp_servicedependency->service_description);
        es[2] = ndo_escape_buffer(temp_servicedependency->dependent_host_name);
        es[3] = ndo_escape_buffer(temp_servicedependency->dependent_service_description);
        es[4] = ndo_escape_buffer(temp_servicedependency->dependency_period);

        struct ndo_broker_data servicedependency_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_DEPENDENTHOSTNAME, es[2]),
            NDODATA_STRING(NDO_DATA_DEPENDENTSERVICEDESCRIPTION, es[3]),
            NDODATA_INTEGER(NDO_DATA_DEPENDENCYTYPE, temp_servicedependency->dependency_type),
            NDODATA_INTEGER(NDO_DATA_INHERITSPARENT, temp_servicedependency->inherits_parent),
            NDODATA_STRING(NDO_DATA_DEPENDENCYPERIOD, es[4]),
            NDODATA_INTEGER(NDO_DATA_FAILONOK, flag_isset(temp_servicedependency->failure_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_FAILONWARNING, flag_isset(temp_servicedependency->failure_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_FAILONUNKNOWN, flag_isset(temp_servicedependency->failure_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_FAILONCRITICAL, flag_isset(temp_servicedependency->failure_options, OPT_CRITICAL)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_SERVICEDEPENDENCYDEFINITION, 
            servicedependency_definition,
            ARRAY_SIZE(servicedependency_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 5);
        ndo_dbuf_free(&dbuf);
    }

    return NDO_OK;
}



/* Dumps config files to the data sink. */
int ndomod_write_config_files(void)
{
    return ndomod_write_main_config_file();
}


/* dumps main config file data to sink */
int ndomod_write_main_config_file(void)
{
    struct timeval now;
    char fbuf[NDOMOD_MAX_BUFLEN] = { 0 };
    char *temp_buffer            = NULL;
    FILE *fp                     = NULL;
    char *var                    = NULL;
    char *val                    = NULL;

    /* get current time */
    gettimeofday(&now, NULL);

    asprintf(&temp_buffer,
        "\n%d:\n%d = %ld.%06ld\n"
        "%d = %s\n",
        NDO_API_MAINCONFIGFILEVARIABLES,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_DATA_CONFIGFILENAME,
        config_file);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write each var/val pair from config file */
    fp = fopen(config_file, "r");
    if (fp != NULL) {

        while (fgets(fbuf, sizeof(fbuf), fp)) {

            /* skip blank lines */
            if (fbuf[0] == '\x0' || fbuf[0] == '\n' || fbuf[0] == '\r') {
                continue;
            }

            strip(fbuf);

            /* skip comments */
            if (fbuf[0] == '#' || fbuf[0] == ';') {
                continue;
            }

            var = strtok(fbuf, " = ");
            if (var == NULL) {
                continue;
            }

            val = strtok(NULL, "\n");

            asprintf(&temp_buffer,
                 "%d = %s = %s\n",
                 NDO_DATA_CONFIGFILEVARIABLE,
                 var,
                 (val == NULL) ? "" : val);

            ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

            free(temp_buffer);
            temp_buffer = NULL;
        }

        fclose(fp);
    }

    asprintf(&temp_buffer,
        "%d\n\n",
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    return NDO_OK;
}

/* dumps runtime variables to sink */
int ndomod_write_runtime_variables(void)
{
    char *temp_buffer = NULL;
    struct timeval now;

    /* get current time */
    gettimeofday(&now, NULL);

    asprintf(&temp_buffer,
        "\n%d:\n%d = %ld.%06ld\n",
        NDO_API_RUNTIMEVARIABLES,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write out main config file name */
    asprintf(&temp_buffer,
        "%d = %s = %s\n",
        NDO_DATA_RUNTIMEVARIABLE,
        "config_file",
        config_file);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write out vars determined after startup */
    asprintf(&temp_buffer,
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lu\n"
        "%d = %s = %lu\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n",
        NDO_DATA_RUNTIMEVARIABLE,
            "total_services",
            scheduling_info.total_services,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_scheduled_services",
            scheduling_info.total_scheduled_services,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_hosts",
            scheduling_info.total_hosts,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_scheduled_hosts",
            scheduling_info.total_scheduled_hosts,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_services_per_host",
            scheduling_info.average_services_per_host,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_scheduled_services_per_host",
            scheduling_info.average_scheduled_services_per_host,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_check_interval_total",
            scheduling_info.service_check_interval_total,
        NDO_DATA_RUNTIMEVARIABLE,
            "host_check_interval_total",
            scheduling_info.host_check_interval_total,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_service_check_interval",
            scheduling_info.average_service_check_interval,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_host_check_interval",
            scheduling_info.average_host_check_interval,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_service_inter_check_delay",
            scheduling_info.average_service_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_host_inter_check_delay",
            scheduling_info.average_host_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_inter_check_delay",
            scheduling_info.service_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "host_inter_check_delay",
            scheduling_info.host_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_interleave_factor",
            scheduling_info.service_interleave_factor,
        NDO_DATA_RUNTIMEVARIABLE,
            "max_service_check_spread",
            scheduling_info.max_service_check_spread,
        NDO_DATA_RUNTIMEVARIABLE,
            "max_host_check_spread",
            scheduling_info.max_host_check_spread);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    asprintf(&temp_buffer,
        "%d\n\n",
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    return NDO_OK;
}
