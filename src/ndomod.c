/**
 * @file ndomod.c Nagios Data Output Event Broker Module
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * Last Modified: 02-28-2014
 *
 * This file is part of NDOUtils.
 *
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

/* Include headers from our project. */
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndomod.h"

/* Include (minimum required) event broker header files. */
#ifdef BUILD_NAGIOS_2X
#include "../include/nagios-2x/nebstructs.h"
#include "../include/nagios-2x/nebmodules.h"
#include "../include/nagios-2x/nebcallbacks.h"
#include "../include/nagios-2x/broker.h"
#endif
#ifdef BUILD_NAGIOS_3X
#include "../include/nagios-3x/nebstructs.h"
#include "../include/nagios-3x/nebmodules.h"
#include "../include/nagios-3x/nebcallbacks.h"
#include "../include/nagios-3x/broker.h"
#endif
#ifdef BUILD_NAGIOS_4X
#include "../include/nagios-4x/nebstructs.h"
#include "../include/nagios-4x/nebmodules.h"
#include "../include/nagios-4x/nebcallbacks.h"
#include "../include/nagios-4x/broker.h"
#endif

/* Include other Nagios header files for access to functions, data structs, etc. */
#ifdef BUILD_NAGIOS_2X
#include "../include/nagios-2x/common.h"
#include "../include/nagios-2x/nagios.h"
#include "../include/nagios-2x/downtime.h"
#include "../include/nagios-2x/comments.h"
#endif
#ifdef BUILD_NAGIOS_3X
#include "../include/nagios-3x/common.h"
#include "../include/nagios-3x/nagios.h"
#include "../include/nagios-3x/downtime.h"
#include "../include/nagios-3x/comments.h"
#include "../include/nagios-3x/macros.h"
#endif
#ifdef BUILD_NAGIOS_4X
#include "../include/nagios-4x/common.h"
#include "../include/nagios-4x/nagios.h"
#include "../include/nagios-4x/downtime.h"
#include "../include/nagios-4x/comments.h"
#include "../include/nagios-4x/macros.h"
#endif

/* Specify event broker API version (required). */
NEB_API_VERSION(CURRENT_NEB_API_VERSION)


#define NDOMOD_VERSION "2.0.0"
#define NDOMOD_NAME "NDOMOD"
#define NDOMOD_DATE "02-28-2014"


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)


typedef enum bd_datatype {
	BD_INT,
	BD_TIMEVAL,
	BD_STRING,
	BD_STRING_ESCAPE,
	BD_UNSIGNED_LONG,
	BD_FLOAT,
} bd_datatype;

struct ndo_broker_data {
	int	key;
	bd_datatype datatype;
	union {
		int	integer;
		struct timeval timestamp;
		const char *string;
		unsigned long unsigned_long;
		double floating_point;
	} value;
};

/* Macros to declare struct ndo_broker_data static initializer lists. */
#define INIT_BD_STRUCT(K, T, U, V) { K, T, { U = V } }
#define INIT_BD_I(K, V)  INIT_BD_STRUCT(K, BD_INT, .integer, V)
#define INIT_BD_TV(K, V) INIT_BD_STRUCT(K, BD_TIMEVAL, .timestamp, V)
#define INIT_BD_S(K, V)  INIT_BD_STRUCT(K, BD_STRING, .string, V)
#define INIT_BD_SE(K, V) INIT_BD_STRUCT(K, BD_STRING_ESCAPE, .string, V)
#define INIT_BD_UL(K, V) INIT_BD_STRUCT(K, BD_UNSIGNED_LONG, .unsigned_long, V)
#define INIT_BD_F(K, V)  INIT_BD_STRUCT(K, BD_FLOAT, .floating_point, V)

/** Phases that a ndomod_broker_*_data() function may process. */
typedef enum bd_phase {
	/** Pre processing executed before data serialization, usually just to
	 * determine whether further processing should take place. */
	bdp_preprocessing,
	/** Main processing is where data is serialized. */
	bdp_mainprocessing,
	/** Post processing happens after data is serialized. */
	bdp_postprocessing
} bd_phase;

/** Possible return codes from the ndomod_broker_*_data() functions. */
typedef enum bd_result {
	bdr_ok, /**< No error indicated, continue processing. */
	bdr_stop, /**< No error indicated, but stop processing. */
	bdr_ephase, /**< Bad phase passed. */
	bdr_enoent /**< Entity (host, service, etc.) not found. */
} bd_result;

/**
 * ndomod_broker_*_data() callback function type.
 * @param p The processing phase to execute.
 * @param o NDO module process options.
 * @param d Data to be handled.
 * @param b Dynamic string buffer to serialize data into.
 * @return bdr_ok, bdr_stop, bdr_ephase or bdr_enoent.
 */
typedef bd_result (*bd_callback)(bd_phase p, unsigned long o, void *d, ndo_dbuf *b);

#define NDO_DECLARE_BD_CALLBACK(f) \
	static bd_result f(bd_phase p, unsigned long o, void *d, ndo_dbuf *b)

NDO_DECLARE_BD_CALLBACK(ndomod_broker_process_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_timed_event_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_log_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_system_command_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_event_handler_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_notification_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_service_check_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_host_check_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_comment_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_downtime_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_flapping_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_program_status_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_host_status_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_service_status_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_adaptive_program_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_adaptive_host_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_adaptive_service_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_external_command_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_aggregated_status_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_retention_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_contact_notification_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_contact_notification_method_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_acknowledgement_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_state_change_data);
#if defined(BUILD_NAGIOS_3X) || defined(BUILD_NAGIOS_4X)
NDO_DECLARE_BD_CALLBACK(ndomod_broker_contact_status_data);
NDO_DECLARE_BD_CALLBACK(ndomod_broker_adaptive_contact_data);
#endif

#undef NDO_DECLARE_BD_CALLBACK

bd_callback ndomod_broker_data_funcs[] = {
#if defined(BUILD_NAGIOS_2X) || defined(BUILD_NAGIOS_3X)
	NULL, /* NEBCALLBACK_RESERVED0 */
	NULL, /* NEBCALLBACK_RESERVED1 */
	NULL, /* NEBCALLBACK_RESERVED2 */
	NULL, /* NEBCALLBACK_RESERVED3 */
	NULL, /* NEBCALLBACK_RESERVED4 */
	NULL, /* NEBCALLBACK_RAW_DATA */
	NULL, /* NEBCALLBACK_NEB_DATA */
#endif
	/* NEBCALLBACK_<ABC>_DATA => ndomod_broker_<abc>_data */
	/* i.e NEBCALLBACK_PROCESS_DATA => ndomod_broker_process_data */
	ndomod_broker_process_data,
	ndomod_broker_timed_event_data,
	ndomod_broker_log_data,
	ndomod_broker_system_command_data,
	ndomod_broker_event_handler_data,
	ndomod_broker_notification_data,
	ndomod_broker_service_check_data,
	ndomod_broker_host_check_data,
	ndomod_broker_comment_data,
	ndomod_broker_downtime_data,
	ndomod_broker_flapping_data,
	ndomod_broker_program_status_data,
	ndomod_broker_host_status_data,
	ndomod_broker_service_status_data,
	ndomod_broker_adaptive_program_data,
	ndomod_broker_adaptive_host_data,
	ndomod_broker_adaptive_service_data,
	ndomod_broker_external_command_data,
	ndomod_broker_aggregated_status_data,
	ndomod_broker_retention_data,
	ndomod_broker_contact_notification_data,
	ndomod_broker_contact_notification_method_data,
	ndomod_broker_acknowledgement_data,
	ndomod_broker_state_change_data,
#if defined(BUILD_NAGIOS_3X) || defined(BUILD_NAGIOS_4X)
	ndomod_broker_contact_status_data,
	ndomod_broker_adaptive_contact_data,
#endif
};

#define EVENT_HANDLER_COUNT ARRAY_SIZE(ndomod_broker_data_funcs)


/* Our NEB module handle. */
void *ndomod_module_handle = NULL;
char *ndomod_instance_name = NULL;
char *ndomod_buffer_file = NULL;
char *ndomod_sink_name = NULL;
int ndomod_sink_type = NDO_SINK_UNIXSOCKET;
int ndomod_sink_tcp_port = NDO_DEFAULT_TCP_PORT;
int ndomod_sink_is_open = NDO_FALSE;
int ndomod_sink_previously_open = NDO_FALSE;
int ndomod_sink_fd = -1;
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


/* Nagios Core variables, READ ONLY!!! */
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
#ifdef BUILD_NAGIOS_2X
extern hostextinfo *hostextinfo_list;
extern serviceextinfo *serviceextinfo_list;
#endif

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __nagios_object_structure_version;

/* This one lives in ndoutils io.c. */
extern int use_ssl;



/* Setup our module when loaded by the event broker. */
int nebmodule_init(int flags, char *args, void *handle) {
	(void)flags; /* Unused, don't warn. */

	/* Cache our NEB module handle */
	ndomod_module_handle = handle;

	/* Say hi to the Nagios Core log. */
	ndomod_printf_to_logs("ndomod: "NDOMOD_NAME" "NDOMOD_VERSION" ("NDOMOD_DATE
			") Copyright 2009-2014 Nagios Core Development Team and Community Contributors");

	/* Ensure our Nagios object structure version matches Core. */
	if (ndomod_check_nagios_object_version() == NDO_ERROR) {
		return -1;
	}

	/* Process our arguments (and configuration). */
	if (ndomod_process_module_args(args) == NDO_ERROR) {
		ndomod_printf_to_logs(
				"ndomod: An error occurred while attempting to process module arguments.");
		return -1;
	}

	/* do some initialization stuff... */
	if (ndomod_init() == NDO_ERROR) {
		ndomod_printf_to_logs("ndomod: An error occurred while attempting to initialize.");
		return -1;
	}

	return 0;
}


/* Shutdown and release our resources when the module is unloaded. */
int nebmodule_deinit(int flags, int reason) {
	(void)flags; /* Unused, don't warn. */
	(void)reason; /* Unused, don't warn. */

	ndomod_deinit();
	ndomod_printf_to_logs("ndomod: Shutdown complete.");
	return 0;
}



/****************************************************************************/
/* INIT/DEINIT FUNCTIONS                                                    */
/****************************************************************************/

/* Checks to make sure Nagios object version matches what we know about. */
static int ndomod_check_nagios_object_version(void) {

	if (__nagios_object_structure_version == CURRENT_OBJECT_STRUCTURE_VERSION) {
		return NDO_OK;
	} else {
		ndomod_printf_to_logs(
				"ndomod: I've been compiled with support for revision %d of the internal Nagios object structures, but the Nagios daemon is currently using revision %d. I'm going to unload so I don't cause any problems...",
				CURRENT_OBJECT_STRUCTURE_VERSION, __nagios_object_structure_version);
		return NDO_ERROR;
	}
}


/* Initialize our data sink and callbacks after we have our module config. */
static int ndomod_init(void) {

	/* (Re)initialize some vars (needed for restarts of daemon - why, if the module gets reloaded ???) */
	ndomod_sink_is_open = NDO_FALSE;
	ndomod_sink_previously_open = NDO_FALSE;
	ndomod_sink_fd = -1;
	ndomod_sink_last_reconnect_attempt = 0L;
	ndomod_sink_last_reconnect_warning = 0L;
	ndomod_allow_sink_activity = NDO_TRUE;

	/* Initialize our data sink buffer and load unprocessed data. */
	ndomod_sink_buffer_init(&sinkbuf, ndomod_sink_buffer_slots);
	ndomod_load_unprocessed_data(ndomod_buffer_file);
	/* The sink will be opened on first write. Here we flush items that may have
	 * been read from the buffer file, but don't buffer this 'line'. */
	ndomod_write_to_sink("\n", NDO_FALSE, NDO_TRUE);

	/* Register our callbacks. */
	if (ndomod_register_callbacks() != NDO_OK) return NDO_ERROR;

#if defined(BUILD_NAGIOS_2X) || defined(BUILD_NAGIOS_3X)
	/* See ndomod_broker_process_data() for Nagios Core 4 implementation. */
	if (ndomod_sink_type == NDO_SINK_FILE) {
		if (!ndomod_sink_rotation_command) {
			/* Log an error if we don't have a rotation command... */
			ndomod_printf_to_logs("ndomod: Warning - No file rotation command defined.");
		} else {
			/* ...otherwise schedule a file rotation event. */
			time_t current_time = time(NULL);
			schedule_new_event(EVENT_USER_FUNCTION, TRUE,
					time(NULL) + ndomod_sink_rotation_interval,
					TRUE, ndomod_sink_rotation_interval, NULL,
					TRUE, ndomod_rotate_sink_file, NULL
#ifndef BUILD_NAGIOS_2X
					, 0
#endif
			);
		}
	}
#endif

	return NDO_OK;
}


/* Shutdown and release our resources when the module is unloaded. */
static int ndomod_deinit(void) {
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

/* Process arguments that were passed to the module at startup. */
static int ndomod_process_module_args(char *args) {
	char *s = NULL;
	char *lasts = NULL;

	if (!args) return NDO_OK;

	/* Process all arguments, hopefully in the form of valid var=val pairs. */
	for (s = strtok_r(args, ",", &lasts); s; s = strtok_r(NULL, ",", &lasts)) {
		int result = ndomod_process_config_var(s);
		if (result != NDO_OK) return result;
	}

	return NDO_OK;
}


/* Process all config vars in a file. */
static int ndomod_process_config_file(const char *filename) {
	ndo_mmapfile *thefile = NULL;
	char *buf = NULL;
	int result = NDO_OK;

	/* Open our file. */
	if (!(thefile = ndo_mmap_fopen(filename))) return NDO_ERROR;

	/* Read, process, and free each line. */
	for  (; result == NDO_OK && (buf = ndo_mmap_fgets(thefile)); free(buf)) {
		/* Skip comments and lank lines... */
		if (buf[0] != '\0' && buf[0] != '#') {
			/* ...otherwise process the variable. */
			result = ndomod_process_config_var(buf);
		}
	}

	/* Close the file. */
	ndo_mmap_fclose(thefile);

	return result;
}


/* A macro to check and handle boolean processing options. */
#define NDO_HANDLE_PROC_OPT(n, o) \
	do if (!strcmp(var, n) && !strcmp(val, "1")) { \
		ndomod_process_options |= NDOMOD_PROCESS_## o ##_DATA; \
		return NDO_OK; \
	} while (0)

/* A macro to check and copy string options. Returns from the invoking context
 * if the config var matches n. If successful, memory allocated
 * for the value will need to be freed when the module is unloaded. */
#define NDO_HANDLE_STRING_OPT(n, v) \
	do if (!strcmp(var, n)) { \
		if ((v = strdup(val))) { \
			return NDO_OK; \
		} else { \
			ndomod_printf_to_logs("ndomod: Error copying option string for '%s'.", n); \
			return NDO_ERROR; \
		} \
	} while (0)

/* A macro to handle strtoul() with a return from invoking context on error. */
#define NDO_HANDLE_STRTOUL_OPT_VAL(n, v) \
	do { \
		errno = 0; \
		v = strtoul(val, NULL, 0); \
		if (errno) { \
			ndomod_printf_to_logs("ndomod: Error converting value for '%s'.", n); \
			return NDO_ERROR; \
		} \
	} while (0)

/* A macro to handle strtoul() with a return from invoking context on error. */
#define NDO_HANDLE_STRTOUL_OPT(n, v) \
	do if (!strcmp(var, n)) { \
		NDO_HANDLE_STRTOUL_OPT_VAL(n, v); \
		return NDO_OK; \
	} while (0)

/* Process a single module config variable */
static int ndomod_process_config_var(char *arg) {
	char *var = NULL;
	char *val = NULL;

	/* Split and strip var/val. */
	ndomod_strip(var = strtok(arg,"="));
	ndomod_strip(val = strtok(NULL,"\n"));

	/* Skip empty var or val. */
	if (!var || !*var || !val || !*val) return NDO_OK;

	/* Process the variable... */

	if (!strcmp(var, "config_file")) return ndomod_process_config_file(val);

	NDO_HANDLE_STRING_OPT("instance_name", ndomod_instance_name);

	NDO_HANDLE_STRING_OPT("output", ndomod_sink_name);

	if (!strcmp(var, "output_type")) {
		if (!strcmp(val, "file")) {
			ndomod_sink_type = NDO_SINK_FILE;
		} else if (!strcmp(val, "tcpsocket")) {
			ndomod_sink_type = NDO_SINK_TCPSOCKET;
		} else {
			ndomod_sink_type = NDO_SINK_UNIXSOCKET;
		}
		return NDO_OK;
	}

	NDO_HANDLE_STRTOUL_OPT("tcp_port", ndomod_sink_tcp_port);

	NDO_HANDLE_STRTOUL_OPT("output_buffer_items", ndomod_sink_buffer_slots);

	NDO_HANDLE_STRTOUL_OPT("reconnect_interval", ndomod_sink_reconnect_interval);

	NDO_HANDLE_STRTOUL_OPT("reconnect_warning_interval", ndomod_sink_reconnect_warning_interval);

	NDO_HANDLE_STRTOUL_OPT("file_rotation_interval", ndomod_sink_rotation_interval);

	NDO_HANDLE_STRING_OPT("file_rotation_command", ndomod_sink_rotation_command);

	NDO_HANDLE_STRTOUL_OPT("file_rotation_timeout", ndomod_sink_rotation_timeout);

	/* Add boolean processing opts */
	NDO_HANDLE_PROC_OPT("process_data", PROCESS);
	NDO_HANDLE_PROC_OPT("timed_event_data", TIMED_EVENT);
	NDO_HANDLE_PROC_OPT("log_data", LOG);
	NDO_HANDLE_PROC_OPT("system_command_data", SYSTEM_COMMAND);
	NDO_HANDLE_PROC_OPT("event_handler_data", EVENT_HANDLER);
	NDO_HANDLE_PROC_OPT("notification_data", NOTIFICATION);
	NDO_HANDLE_PROC_OPT("service_check_data", SERVICE_CHECK);
	NDO_HANDLE_PROC_OPT("host_check_data", HOST_CHECK);
	NDO_HANDLE_PROC_OPT("comment_data", COMMENT);
	NDO_HANDLE_PROC_OPT("downtime_data", DOWNTIME);
	NDO_HANDLE_PROC_OPT("flapping_data", FLAPPING);
	NDO_HANDLE_PROC_OPT("program_status_data", PROGRAM_STATUS);
	NDO_HANDLE_PROC_OPT("host_status_data", HOST_STATUS);
	NDO_HANDLE_PROC_OPT("service_status_data", SERVICE_STATUS);
	NDO_HANDLE_PROC_OPT("adaptive_program_data", ADAPTIVE_PROGRAM);
	NDO_HANDLE_PROC_OPT("adaptive_host_data", ADAPTIVE_HOST);
	NDO_HANDLE_PROC_OPT("adaptive_service_data", ADAPTIVE_SERVICE);
	NDO_HANDLE_PROC_OPT("external_command_data", EXTERNAL_COMMAND);
	NDO_HANDLE_PROC_OPT("object_config_data", OBJECT_CONFIG);
	NDO_HANDLE_PROC_OPT("main_config_data", MAIN_CONFIG);
	NDO_HANDLE_PROC_OPT("aggregated_status_data", AGGREGATED_STATUS);
	NDO_HANDLE_PROC_OPT("retention_data", RETENTION);
	NDO_HANDLE_PROC_OPT("acknowledgement_data", ACKNOWLEDGEMENT);
	NDO_HANDLE_PROC_OPT("state_change_data", STATE_CHANGE);
	NDO_HANDLE_PROC_OPT("contact_status_data", CONTACT_STATUS);
	NDO_HANDLE_PROC_OPT("adaptive_contact_data", ADAPTIVE_CONTACT);

	/* data_processing_options overrides individual values previously set. */
	if (!strcmp(var, "data_processing_options")) {
		if (!strcmp(val, "-1")) {
			ndomod_process_options = NDOMOD_PROCESS_EVERYTHING;
		} else {
			NDO_HANDLE_STRTOUL_OPT_VAL("data_processing_options", ndomod_process_options);
		}
		return NDO_OK;
	}

	NDO_HANDLE_STRTOUL_OPT("config_output_options", ndomod_config_output_options);

	NDO_HANDLE_STRING_OPT("buffer_file", ndomod_buffer_file);

	if (!strcmp(var, "use_ssl")) {
		if (strlen(val) == 1) use_ssl = (val[0] == '1');
		return NDO_OK;
	}

	/*
	printf("ndomod: Invalid ndomod config option: %s\n", var);
	*/
	return NDO_OK;
}

/* Cleanup our macros after processing our config. */
#undef NDO_HANDLE_PROC_OPT
#undef NDO_HANDLE_STRING_OPT
#undef NDO_HANDLE_STRTOUL_OPT_VAL
#undef NDO_HANDLE_STRTOUL_OPT

/* Frees any memory allocated for config options. */
static void ndomod_free_config_memory(void) {
	my_free(ndomod_instance_name);
	my_free(ndomod_sink_name);
	my_free(ndomod_sink_rotation_command);
	my_free(ndomod_buffer_file);
}



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

/* Print to Nagios Core logs via a temporary static buffer. */
static int ndomod_printf_to_logs(const char *fmt, ...) {
	char msg[NDOMOD_MAX_BUFLEN];
	int n;
	va_list ap;

	va_start(ap, fmt);
	n = vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	if (n < 0) return NDO_ERROR;
	if (n >= (int)sizeof(msg)) strcpy(msg+sizeof(msg)-4, "...");
	else msg[sizeof(msg)-1] = '\0';

	return write_to_all_logs(msg, NSLOG_INFO_MESSAGE);
}



/****************************************************************************/
/* DATA SINK FUNCTIONS                                                      */
/****************************************************************************/

/* (Re)opens our data sink. */
static int ndomod_open_sink(void) {
	int result;

	if (ndomod_sink_is_open) return ndomod_sink_fd;

	/* Try and open the sink. */
	result = ndo_sink_open(ndomod_sink_name,
			0,
			ndomod_sink_type,
			ndomod_sink_tcp_port,
			(ndomod_sink_type == NDO_SINK_FILE) ? O_WRONLY|O_CREAT|O_APPEND : 0,
			&ndomod_sink_fd
	);
	if (result != NDO_OK) return NDO_ERROR;

	ndomod_sink_is_open = NDO_TRUE; /* We've opened the sink. */
	ndomod_sink_previously_open = NDO_TRUE; /* The sink has been open once. */

	return NDO_OK;
}


/* Closes our data sink. */
static int ndomod_close_sink(void) {

	if (!ndomod_sink_is_open) return NDO_OK;

	/* Flush and close the sink. */
	ndo_sink_flush(ndomod_sink_fd);
	ndo_sink_close(ndomod_sink_fd);
	ndomod_sink_is_open = NDO_FALSE;

	return NDO_OK;
}


/* say hello */
static int ndomod_hello_sink(int reconnect, int problem_disconnect) {
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	const char *connection_type=NULL;
	const char *connect_type=NULL;

	/* get the connection type string */
	if(ndomod_sink_type==NDO_SINK_FD || ndomod_sink_type==NDO_SINK_FILE)
		connection_type=NDO_API_CONNECTION_FILE;
	else if(ndomod_sink_type==NDO_SINK_TCPSOCKET)
		connection_type=NDO_API_CONNECTION_TCPSOCKET;
	else
		connection_type=NDO_API_CONNECTION_UNIXSOCKET;

	/* get the connect type string */
	if(reconnect==TRUE && problem_disconnect==TRUE)
		connect_type=NDO_API_CONNECTTYPE_RECONNECT;
	else
		connect_type=NDO_API_CONNECTTYPE_INITIAL;

	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%s\n%s: %d\n%s: %s\n%s: %s\n%s: %lu\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s\n\n"
		 ,NDO_API_HELLO
		 ,NDO_API_PROTOCOL
		 ,NDO_API_PROTOVERSION
		 ,NDO_API_AGENT
		 ,NDOMOD_NAME
		 ,NDO_API_AGENTVERSION
		 ,NDOMOD_VERSION
		 ,NDO_API_STARTTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_DISPOSITION
		 ,NDO_API_DISPOSITION_REALTIME
		 ,NDO_API_CONNECTION
		 ,connection_type
		 ,NDO_API_CONNECTTYPE
		 ,connect_type
		 ,NDO_API_INSTANCENAME
		 ,(ndomod_instance_name==NULL)?"default":ndomod_instance_name
		 ,NDO_API_STARTDATADUMP
		);

	temp_buffer[sizeof(temp_buffer)-1]='\x0';

	ndomod_write_to_sink(temp_buffer,NDO_FALSE,NDO_FALSE);

	return NDO_OK;
}


/* say goodbye */
static int ndomod_goodbye_sink(void) {
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n%d\n%s: %lu\n%s\n\n"
		 ,NDO_API_ENDDATADUMP
		 ,NDO_API_ENDTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_GOODBYE
		 );

	temp_buffer[sizeof(temp_buffer)-1]='\x0';

	ndomod_write_to_sink(temp_buffer,NDO_FALSE,NDO_TRUE);

	return NDO_OK;
}


/* used to rotate data sink file on a regular basis */
static int ndomod_rotate_sink_file(void *args) {
	(void)args; /* Unused, don't warn. */

#ifdef BUILD_NAGIOS_2X
	char raw_command_line[MAX_COMMAND_BUFFER];
	char processed_command_line[MAX_COMMAND_BUFFER];
#else
	char *raw_command_line_3x=NULL;
	char *processed_command_line_3x=NULL;
#endif
	int early_timeout=FALSE;
	double exectime;

	/* close sink */
	ndomod_goodbye_sink();
	ndomod_close_sink();

	/* we shouldn't write any data to the sink while we're rotating it... */
	ndomod_allow_sink_activity=NDO_FALSE;


	/****** ROTATE THE FILE *****/

	/* get the raw command line */
#ifdef BUILD_NAGIOS_2X
	get_raw_command_line(ndomod_sink_rotation_command,raw_command_line,sizeof(raw_command_line),STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
	strip(raw_command_line);
#else
	get_raw_command_line(find_command(ndomod_sink_rotation_command),ndomod_sink_rotation_command,&raw_command_line_3x,STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
	strip(raw_command_line_3x);
#endif

	/* process any macros in the raw command line */
#ifdef BUILD_NAGIOS_2X
	process_macros(raw_command_line,processed_command_line,(int)sizeof(processed_command_line),STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#else
	process_macros(raw_command_line_3x,&processed_command_line_3x,STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#endif

	/* run the command */
#ifdef BUILD_NAGIOS_2X
	my_system(processed_command_line,ndomod_sink_rotation_timeout,&early_timeout,&exectime,NULL,0);
#else
	my_system(processed_command_line_3x,ndomod_sink_rotation_timeout,&early_timeout,&exectime,NULL,0);
#endif


	/* allow data to be written to the sink */
	ndomod_allow_sink_activity=NDO_TRUE;

	/* re-open sink */
	ndomod_open_sink();
	ndomod_hello_sink(TRUE,FALSE);

	return NDO_OK;
}


/* writes data to sink */
static int ndomod_write_to_sink(const char *buf, int buffer_write, int flush_buffer) {
	char *sbuf=NULL;
	int buflen=0;
	int result=NDO_OK;
	time_t current_time;
	int reconnect=NDO_FALSE;
	unsigned long items_to_flush=0L;

	/* we have nothing to write... */
	if(buf==NULL)
		return NDO_OK;

	/* we shouldn't be messing with things... */
	if(ndomod_allow_sink_activity==NDO_FALSE)
		return NDO_ERROR;

	/* open the sink if necessary... */
	if(ndomod_sink_is_open==NDO_FALSE){

		time(&current_time);

		/* are we reopening the sink? */
		if(ndomod_sink_previously_open==NDO_TRUE)
			reconnect=NDO_TRUE;

		/* (re)connect to the sink if its time */
		if((unsigned long)((unsigned long)current_time-ndomod_sink_reconnect_interval)>(unsigned long)ndomod_sink_last_reconnect_attempt){

			result=ndomod_open_sink();

			ndomod_sink_last_reconnect_attempt=current_time;

			ndomod_sink_connect_attempt++;

			/* sink was (re)opened... */
			if(result==NDO_OK){

				if(reconnect==NDO_TRUE){
					ndomod_hello_sink(TRUE,TRUE);
					ndomod_printf_to_logs("ndomod: Successfully reconnected to data sink! %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
				        }
				else{
					ndomod_hello_sink(FALSE,FALSE);
					if(sinkbuf.overflow==0L)
						ndomod_printf_to_logs("ndomod: Successfully connected to data sink. %lu queued items to flush.",sinkbuf.items);
					else
						ndomod_printf_to_logs("ndomod: Successfully connected to data sink. %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
				        }

				/* reset sink overflow */
				sinkbuf.overflow=0L;
				}

			/* sink could not be (re)opened... */
			else{

				if((unsigned long)((unsigned long)current_time-ndomod_sink_reconnect_warning_interval)>(unsigned long)ndomod_sink_last_reconnect_warning){
					if(reconnect==NDO_TRUE)
						ndomod_printf_to_logs("ndomod: Still unable to reconnect to data sink. %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
					else if(ndomod_sink_connect_attempt==1)
						ndomod_printf_to_logs("ndomod: Could not open data sink! I'll keep trying, but some output may get lost...");
					else
						ndomod_printf_to_logs("ndomod: Still unable to connect to data sink. %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);

					ndomod_sink_last_reconnect_warning=current_time;
					}
				}
			}
	        }

	/* we weren't able to (re)connect */
	if(ndomod_sink_is_open==NDO_FALSE){

		/***** BUFFER OUTPUT FOR LATER *****/

		if(buffer_write==NDO_TRUE)
			ndomod_sink_buffer_push(&sinkbuf,buf);

		return NDO_ERROR;
	        }


	/***** FLUSH BUFFERED DATA FIRST *****/

	if(flush_buffer==NDO_TRUE && (items_to_flush=ndomod_sink_buffer_items(&sinkbuf))>0){

		while(ndomod_sink_buffer_items(&sinkbuf)>0){

			/* get next item from buffer */
			sbuf=ndomod_sink_buffer_peek(&sinkbuf);

			buflen=strlen(sbuf);
			result=ndo_sink_write(ndomod_sink_fd,sbuf,buflen);

			/* an error occurred... */
			if(result<0){

				/* sink problem! */
				if(errno!=EAGAIN){

					/* close the sink */
					ndomod_close_sink();

					ndomod_printf_to_logs("ndomod: Error writing to data sink! Some output may get lost. %lu queued items to flush.",sinkbuf.items);

					time(&current_time);
					ndomod_sink_last_reconnect_attempt=current_time;
					ndomod_sink_last_reconnect_warning=current_time;
		                        }

				/***** BUFFER ORIGINAL OUTPUT FOR LATER *****/

				if(buffer_write==NDO_TRUE)
					ndomod_sink_buffer_push(&sinkbuf,buf);

				return NDO_ERROR;
	                        }

			/* buffer was written okay, so remove it from buffer */
			ndomod_sink_buffer_pop(&sinkbuf);
		        }

		ndomod_printf_to_logs("ndomod: Successfully flushed %lu queued items to data sink.",items_to_flush);
	        }


	/***** WRITE ORIGINAL DATA *****/

	/* write the data */
	buflen=strlen(buf);
	result=ndo_sink_write(ndomod_sink_fd,buf,buflen);

	/* an error occurred... */
	if(result<0){

		/* sink problem! */
		if(errno!=EAGAIN){

			/* close the sink */
			ndomod_close_sink();

			time(&current_time);
			ndomod_sink_last_reconnect_attempt=current_time;
			ndomod_sink_last_reconnect_warning=current_time;

			ndomod_printf_to_logs("ndomod: Error writing to data sink! Some output may get lost...");
			ndomod_printf_to_logs("ndomod: Please check remote ndo2db log, database connection or SSL parameters.");
		        }

		/***** BUFFER OUTPUT FOR LATER *****/

		if(buffer_write==NDO_TRUE)
			ndomod_sink_buffer_push(&sinkbuf,buf);

		return NDO_ERROR;
	        }

	return NDO_OK;
}



/* save unprocessed data to buffer file */
static int ndomod_save_unprocessed_data(const char *f) {
	FILE *fp=NULL;
	char *buf=NULL;
	char *ebuf=NULL;

	/* no file */
	if(f==NULL)
		return NDO_OK;

	/* open the file for writing */
	if((fp=fopen(f,"w"))==NULL)
		return NDO_ERROR;

	/* save all buffered items */
	while(ndomod_sink_buffer_items(&sinkbuf)>0){

		/* get next item from buffer */
		buf=ndomod_sink_buffer_pop(&sinkbuf);

		/* escape the string */
		ebuf=ndo_escape_buffer(buf);

		/* write string to file */
		fputs(ebuf,fp);
		fputs("\n",fp);

		/* free memory */
		free(buf);
		buf=NULL;
		free(ebuf);
		ebuf=NULL;
		}

	fclose(fp);

	return NDO_OK;
}


/* load unprocessed data from buffer file */
static int ndomod_load_unprocessed_data(const char *f) {
	ndo_mmapfile *thefile=NULL;
	char *ebuf=NULL;
	char *buf=NULL;

	/* open the file */
	if((thefile=ndo_mmap_fopen(f))==NULL)
		return NDO_ERROR;

	/* process each line of the file */
	while((ebuf=ndo_mmap_fgets(thefile))){

		/* unescape string */
		buf=ndo_unescape_buffer(ebuf);

		/* save the data to the sink buffer */
		ndomod_sink_buffer_push(&sinkbuf,buf);

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
static int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf, unsigned long maxitems) {
	unsigned long x;

	if(sbuf==NULL || maxitems<=0)
		return NDO_ERROR;

	/* allocate memory for the buffer */
	if((sbuf->buffer=(char **)malloc(sizeof(char *)*maxitems))){
		for(x=0;x<maxitems;x++)
			sbuf->buffer[x]=NULL;
	        }

	sbuf->size=0L;
	sbuf->head=0L;
	sbuf->tail=0L;
	sbuf->items=0L;
	sbuf->maxitems=maxitems;
	sbuf->overflow=0L;

	return NDO_OK;
        }


/* deinitializes sink buffer */
static int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf){
	unsigned long x;

	if(sbuf==NULL)
		return NDO_ERROR;

	/* free any allocated memory */
	for(x=0;x<sbuf->maxitems;x++)
		free(sbuf->buffer[x]);

	free(sbuf->buffer);
	sbuf->buffer=NULL;

	return NDO_OK;
        }


/* buffers output */
static int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf, const char *buf){

	if(sbuf==NULL || buf==NULL)
		return NDO_ERROR;

	/* no space to store buffer */
	if(sbuf->buffer==NULL || sbuf->items==sbuf->maxitems){
		sbuf->overflow++;
		return NDO_ERROR;
	        }

	/* store buffer */
	sbuf->buffer[sbuf->head]=strdup(buf);
	sbuf->head=(sbuf->head+1)%sbuf->maxitems;
	sbuf->items++;

	return NDO_OK;
        }


/* gets and removes next item from buffer */
static char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf){
	char *buf=NULL;

	if(sbuf==NULL)
		return NULL;

	if(sbuf->buffer==NULL)
		return NULL;

	if(sbuf->items==0)
		return NULL;

	/* remove item from buffer */
	buf=sbuf->buffer[sbuf->tail];
	sbuf->buffer[sbuf->tail]=NULL;
	sbuf->tail=(sbuf->tail+1)%sbuf->maxitems;
	sbuf->items--;

	return buf;
        }


/* gets next items from buffer */
static char *ndomod_sink_buffer_peek(ndomod_sink_buffer *sbuf){
	char *buf=NULL;

	if(sbuf==NULL)
		return NULL;

	if(sbuf->buffer==NULL)
		return NULL;

	buf=sbuf->buffer[sbuf->tail];

	return buf;
        }


/* returns number of items buffered */
static int ndomod_sink_buffer_items(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->items;
        }


/* gets number of items lost due to buffer overflow */
static unsigned long ndomod_sink_buffer_get_overflow(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->overflow;
        }


/* sets number of items lost due to buffer overflow */
static int ndomod_sink_buffer_set_overflow(ndomod_sink_buffer *sbuf, unsigned long num){

	if(sbuf==NULL)
		return 0;
	else
		sbuf->overflow=num;

	return sbuf->overflow;
        }



/****************************************************************************/
/* CALLBACK FUNCTIONS                                                       */
/****************************************************************************/

/* A handy macro to DRY out the callback registration. */
#define NDO_REG_CALLBACK(dtc, dtn) \
	do { \
		int result = neb_register_callback(dtc, \
				ndomod_module_handle, 0, ndomod_broker_data); \
		if (result != NDO_OK) { \
			ndomod_printf_to_logs("ndomod: Error registering for %s data.", dtn); \
			return result; \
		} else { \
			ndomod_printf_to_logs("ndomod: Registered for %s data.", dtn); \
		} \
	} while (0)

#define NDO_REG_OPTIONAL_CALLBACK(dtc, dtn) \
	if (ndomod_process_options & NDOMOD_PROCESS_## dtc ##_DATA) \
		NDO_REG_CALLBACK(NEBCALLBACK_## dtc ##_DATA, dtn)

/* Registers callbacks for the events we process. */
static int ndomod_register_callbacks(void) {
	NDO_REG_OPTIONAL_CALLBACK(PROCESS, "process");
	NDO_REG_OPTIONAL_CALLBACK(TIMED_EVENT, "timed event");
	NDO_REG_OPTIONAL_CALLBACK(LOG, "log");
	NDO_REG_OPTIONAL_CALLBACK(SYSTEM_COMMAND, "system command");
	NDO_REG_OPTIONAL_CALLBACK(EVENT_HANDLER, "event handler");
	NDO_REG_OPTIONAL_CALLBACK(NOTIFICATION, "notification");
	NDO_REG_OPTIONAL_CALLBACK(SERVICE_CHECK, "service check");
	NDO_REG_OPTIONAL_CALLBACK(HOST_CHECK, "host check");
	NDO_REG_OPTIONAL_CALLBACK(COMMENT, "comment");
	NDO_REG_OPTIONAL_CALLBACK(DOWNTIME, "downtime");
	NDO_REG_OPTIONAL_CALLBACK(FLAPPING, "flapping");
	NDO_REG_OPTIONAL_CALLBACK(PROGRAM_STATUS, "program status");
	NDO_REG_OPTIONAL_CALLBACK(HOST_STATUS, "host status");
	NDO_REG_OPTIONAL_CALLBACK(SERVICE_STATUS, "service status");
	NDO_REG_OPTIONAL_CALLBACK(ADAPTIVE_PROGRAM, "adaptive program");
	NDO_REG_OPTIONAL_CALLBACK(ADAPTIVE_HOST, "adaptive host");
	NDO_REG_OPTIONAL_CALLBACK(ADAPTIVE_SERVICE, "adaptive service");
	NDO_REG_OPTIONAL_CALLBACK(EXTERNAL_COMMAND, "external command");
	NDO_REG_OPTIONAL_CALLBACK(AGGREGATED_STATUS, "aggregated status");
	NDO_REG_OPTIONAL_CALLBACK(RETENTION, "retention");
	// No current options for these two.
	NDO_REG_CALLBACK(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, "contact notification");
	NDO_REG_CALLBACK(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, "contact notification method");
	NDO_REG_OPTIONAL_CALLBACK(ACKNOWLEDGEMENT, "acknowledgement");
	NDO_REG_OPTIONAL_CALLBACK(STATE_CHANGE, "state change");
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
	NDO_REG_OPTIONAL_CALLBACK(CONTACT_STATUS, "contact status");
	NDO_REG_OPTIONAL_CALLBACK(ADAPTIVE_CONTACT, "adaptive contact");
#endif

	return NDO_OK;
}

/* Cleanup our macros after registering our callbacks. */
#undef NDO_REG_OPTIONAL_CALLBACK
#undef NDO_REG_CALLBACK


/* deregisters callbacks */
static int ndomod_deregister_callbacks(void) {

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
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
	neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA, ndomod_broker_data);
#endif

	return NDO_OK;
}


static void ndomod_enddata_serialize(ndo_dbuf *dbuf) {
	ndo_dbuf_strcat(dbuf, "\n"STRINGIFY(NDO_API_ENDDATA)"\n\n");
}

static void ndomod_broker_data_serialize(ndo_dbuf *dbuf, int datatype,
		struct ndo_broker_data *bd, size_t bdsize, int add_enddata) {

	/* Start out with the broker data type. */
	ndo_dbuf_printf(dbuf, "\n%d:", datatype);

	/* Add each value from the input array. */
	for (; bdsize--; bd++) {
		switch (bd->datatype) {
			case BD_INT:
				ndo_dbuf_printf(dbuf, "\n%d=%d", bd->key, bd->value.integer);
				break;
			case BD_TIMEVAL:
				ndo_dbuf_printf(dbuf, "\n%d=%ld.%06ld", bd->key,
						(long)bd->value.timestamp.tv_sec, (long)bd->value.timestamp.tv_usec);
				break;
			case BD_STRING:
				ndo_dbuf_printf(dbuf, "\n%d=", bd->key);
				if (bd->value.string && *bd->value.string) {
					ndo_dbuf_strcat(dbuf, bd->value.string);
				}
				break;
			case BD_STRING_ESCAPE:
				ndo_dbuf_printf(dbuf, "\n%d=", bd->key);
				if (bd->value.string && *bd->value.string) {
					ndo_dbuf_strcat_escaped(dbuf, bd->value.string);
				}
				break;
			case BD_UNSIGNED_LONG:
				ndo_dbuf_printf(dbuf, "\n%d=%lu", bd->key, bd->value.unsigned_long);
				break;
			case BD_FLOAT:
				ndo_dbuf_printf(dbuf, "\n%d=%.5lf", bd->key, bd->value.floating_point);
				break;
		}
	}

	/* Close out with an NDO_API_ENDDATA marker if requested. */
	if (add_enddata) ndomod_enddata_serialize(dbuf);
}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
static void ndomod_customvars_serialize(customvariablesmember *c,
		ndo_dbuf *dbuf) {

	for (; c; c = c->next) {
		const char *name = c->variable_name;
		const char *value = c->variable_value;
		ndo_dbuf_strcat(dbuf, "\n"STRINGIFY(NDO_DATA_CUSTOMVARIABLE)"=");
		if (name && *name) ndo_dbuf_strcat_escaped(dbuf, name);
		ndo_dbuf_strcat(dbuf, c->has_been_modified ? ":1:" : ":0:");
		if (value && *value) ndo_dbuf_strcat_escaped(dbuf, value);
	}
}
#endif

static void ndomod_contactgroups_serialize(contactgroupsmember *g,
		ndo_dbuf *dbuf) {

	for (; g; g = g->next) {
		const char *name = g->group_name;
		ndo_dbuf_strcat(dbuf, "\n"STRINGIFY(NDO_DATA_CONTACTGROUP)"=");
		if (name && *name) ndo_dbuf_strcat_escaped(dbuf, name);
	}
}

#ifdef BUILD_NAGIOS_2X
static void ndomod_contacts_serialize(contactgroupmember *contacts,
	ndo_dbuf *dbufp, int varnum) {

	contactgroupmember *temp_contactgroupmember = NULL;
	char *contact_name;
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	for(temp_contactgroupmember = contacts; temp_contactgroupmember != NULL;
			temp_contactgroupmember=temp_contactgroupmember->next) {

		contact_name = ndo_escape_buffer(temp_contactgroupmember->contact_name);

		snprintf(temp_buffer, sizeof(temp_buffer)-1, "\n%d=%s", varnum, 
				(NULL == contact_name) ? "" : contact_name);
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ndo_dbuf_strcat(dbufp, temp_buffer);

		if(NULL != contact_name) free(contact_name);
		}
	}
#else
static void ndomod_contacts_serialize(contactsmember *c, ndo_dbuf *dbuf,
		int varnum) {

	for (; c; c = c->next) {
		const char *name = c->contact_name;
		ndo_dbuf_printf(dbuf, "\n%d=", varnum);
		if (name && *name) ndo_dbuf_strcat_escaped(dbuf, name);
	}
}
#endif

#ifdef BUILD_NAGIOS_2X
static void ndomod_hosts_serialize_2x(hostgroupmember *hosts, ndo_dbuf *dbufp, 
		int varnum) {

	hostgroupmember *temp_hostgroupmember=NULL;
	char *host_name;
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	for(temp_hostgroupmember = hosts; temp_hostgroupmember != NULL;
			temp_hostgroupmember = temp_hostgroupmember->next) {

		host_name = ndo_escape_buffer(temp_hostgroupmember->host_name);

		snprintf(temp_buffer, sizeof(temp_buffer)-1, "\n%d=%s", varnum,
				(NULL == host_name) ? "" : host_name);
		temp_buffer[sizeof(temp_buffer)-1] =  '\x0';
		ndo_dbuf_strcat(dbufp, temp_buffer);

		if(NULL != host_name) free(host_name);
		}
	}
#endif

static void ndomod_hosts_serialize(hostsmember *h, ndo_dbuf *dbuf,
		int varnum) {

	for (; h; h = h->next) {
		const char *name = h->host_name;
		ndo_dbuf_printf(dbuf, "\n%d=", varnum);
		if (name && *name) ndo_dbuf_strcat_escaped(dbuf, name);
	}
}

#ifdef BUILD_NAGIOS_2X
static void ndomod_services_serialize(servicegroupmember *services, 
		ndo_dbuf *dbufp, int varnum) {

	servicegroupmember *temp_servicegroupmember = NULL;
	char *host_name;
	char *service_description;
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	for(temp_servicegroupmember = services; temp_servicegroupmember != NULL;
			temp_servicegroupmember = temp_servicegroupmember->next) {

		host_name = ndo_escape_buffer(temp_servicegroupmember->host_name);
		service_description = ndo_escape_buffer(temp_servicegroupmember->service_description);

		snprintf(temp_buffer, sizeof(temp_buffer)-1, "\n%d=%s;%s", varnum,
				(NULL == host_name) ? "" : host_name,
				(NULL == service_description) ? "" : service_description);
		temp_buffer[sizeof(temp_buffer)-1] = '\x0';
		ndo_dbuf_strcat(dbufp, temp_buffer);

		if(NULL != host_name) free(host_name);
		if(NULL != service_description) free(service_description);
		}
	}
#else
static void ndomod_services_serialize(servicesmember *s, ndo_dbuf *dbuf,
		int varnum) {

	for (; s; s = s->next) {
		const char *name = s->host_name;
		const char *desc = s->service_description;
		ndo_dbuf_printf(dbuf, "\n%d=", varnum);
		if (name && *name) ndo_dbuf_strcat_escaped(dbuf, name);
		ndo_dbuf_strcat(dbuf, ";");
		if (desc && *desc) ndo_dbuf_strcat_escaped(dbuf, desc);
	}
}
#endif

static void ndomod_commands_serialize(commandsmember *c, ndo_dbuf *dbuf,
		int varnum) {

	for (; c; c = c->next) {
		const char *command = c->command;
		ndo_dbuf_printf(dbuf, "\n%d=", varnum);
		if (command && *command) ndo_dbuf_strcat_escaped(dbuf, command);
	}
}


static int ndomod_broker_data(int event_type, void *data) {
	ndo_dbuf dbuf;
	bd_callback handler;

	/* We need data to process and a function to process it. */
	if (!data) return 0;
	if (event_type < 0 || (size_t)event_type >= EVENT_HANDLER_COUNT) return 0;
	if (!(handler = ndomod_broker_data_funcs[event_type])) return 0;

	/* Pre-processing. */
	switch (handler(bdp_preprocessing, ndomod_process_options, data, NULL)) {
		case bdr_ok:
			break;
		case bdr_stop:
		case bdr_ephase:
		case bdr_enoent:
			return 0;
	}

	/* A dynamic buffer, 2KB chunk size. */
	ndo_dbuf_init(&dbuf, 2048);

	/* Main processing. */
	switch(handler(bdp_mainprocessing, ndomod_process_options, data, &dbuf)) {
		case bdr_ok:
			break;
		case bdr_stop:
		case bdr_ephase:
			/* Shouldn't happen */
			break;
		case bdr_enoent:
			ndo_dbuf_free(&dbuf);
			return 0;
	}

	/* Sink the buffer and then free its memory. */
	ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
	ndo_dbuf_free(&dbuf);

	/* Post-processing, ignore the return for now... */
	(void)handler(bdp_postprocessing, ndomod_process_options, data, &dbuf);

	return 0;
}


/**
 * A macro for defining static initializer lists for the common nebstruct_*
 * type, flags, attr and timestamp members.
 */
#define INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(D) \
INIT_BD_I(NDO_DATA_TYPE, D->type), \
INIT_BD_I(NDO_DATA_FLAGS, D->flags), \
INIT_BD_I(NDO_DATA_ATTRIBUTES, D->attr), \
INIT_BD_TV(NDO_DATA_TIMESTAMP, D->timestamp)

static bd_result ndomod_broker_process_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_process_data *procdata = data;

printf("ndomod_broker_process_data() start\n");
	switch (phase) {

		case bdp_preprocessing:
printf("ndomod_broker_process_data() preprocessing event type: %d\n", procdata->type);
#ifdef BUILD_NAGIOS_4X
			/* In Core 4, the file rotation event is scheduled upon receipt of
			 * NEBTYPE_PROCESS_START because the scheduling queue is not initialized
			 * when ndomod_init() is called, as in previous versions. */
			if (procdata->type == NEBTYPE_PROCESS_START && ndomod_sink_type == NDO_SINK_FILE) {
				if (!ndomod_sink_rotation_command) {
					/* Log an error to Core if we don't have a rotation command... */
					ndomod_printf_to_logs("ndomod: Warning - No file rotation command defined.");
				} else {
					/* ...otherwise, schedule a file rotation event. */
					time_t rotate_at_time = time(NULL) + ndomod_sink_rotation_interval;
					schedule_new_event(EVENT_USER_FUNCTION, TRUE, rotate_at_time, TRUE,
							ndomod_sink_rotation_interval, NULL, TRUE,
							ndomod_rotate_sink_file, NULL, 0);
				}
			}
#endif
			if (!(process_options & NDOMOD_PROCESS_PROCESS_DATA))
				return bdr_stop;
			break;

		case bdp_mainprocessing:
			{
				struct ndo_broker_data process_data[] = {
					INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(procdata),
					INIT_BD_S(NDO_DATA_PROGRAMNAME, "Nagios"),
					INIT_BD_S(NDO_DATA_PROGRAMVERSION, get_program_version()),
					INIT_BD_S(NDO_DATA_PROGRAMDATE, get_program_modification_date()),
					INIT_BD_UL(NDO_DATA_PROCESSID, (unsigned long)getpid())
				};
				ndomod_broker_data_serialize(dbufp, NDO_API_PROCESSDATA, process_data,
						ARRAY_SIZE(process_data), TRUE);
			}
			break;

		case bdp_postprocessing:
			/* Dump original config after pre-launch config verification. */
			if (procdata->type == NEBTYPE_PROCESS_START) {
				ndomod_write_config_files();
				ndomod_write_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
			}
			/* Dump runtime vars on event loop startup. */
			if (procdata->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {
				ndomod_write_runtime_variables();
			}
			break;

		default:
			return bdr_ephase;
	}

	return bdr_ok;
}

/**
 * Many ndomod_broker_*_data() handlers have a common structure: a switch on
 * bd_phase phase; a case bdp_preprocessing that returns bdr_stop if the data
 * type flag T isn't enabled in process_options; a case bdp_mainprocessing that
 * serializes data; a do-nothing case bdp_postprocessing; and a default return
 * bdr_ephase. This macro handles the preamble before the bdp_mainprocessing
 * case body. Use it with the corresponding postamble macro like so:
 *   NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_SOME_DATA) {
 *     // bdp_mainprocessing case body here...
 *   } NDOMOD_BD_COMMON_SWITCH_POST;
 */
#define NDOMOD_BD_COMMON_SWITCH_PRE(T) \
	switch (phase) { \
		case bdp_preprocessing: \
			if (!(process_options & T)) return bdr_stop; \
			break; \
		case bdp_mainprocessing:

/**
 * Handle the ndomod_broker_*_data() common switch postamble.
 */
#define NDOMOD_BD_COMMON_SWITCH_POST \
			break; \
		case bdp_postprocessing: break; \
		default: return bdr_ephase; \
	} return bdr_ok

static bd_result ndomod_broker_timed_event_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_TIMED_EVENT_DATA) {

		nebstruct_timed_event_data *eventdata = data;
		service *temp_service = NULL;
		host *temp_host = NULL;
		scheduled_downtime *temp_downtime = NULL;
		const char *host_name = NULL;
		const char *service_desc = NULL;

		switch (eventdata->event_type) {

			case EVENT_SERVICE_CHECK:
				temp_service = eventdata->event_data;
				if (temp_service) {
					host_name = temp_service->host_name;
					service_desc = temp_service->description;
				}
				{
					struct ndo_broker_data timed_event_data[] = {
						INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(eventdata),
						INIT_BD_I(NDO_DATA_EVENTTYPE, eventdata->event_type),
						INIT_BD_I(NDO_DATA_RECURRING, eventdata->recurring),
						INIT_BD_UL(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
						INIT_BD_SE(NDO_DATA_HOST, host_name),
						INIT_BD_SE(NDO_DATA_SERVICE, service_desc)
					};
					ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
							timed_event_data, ARRAY_SIZE(timed_event_data), TRUE);
				}
				break;

			case EVENT_HOST_CHECK:
				temp_host = eventdata->event_data;
				if (temp_host) {
					host_name = temp_host->name;
				}
				{
					struct ndo_broker_data timed_event_data[] = {
						INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(eventdata),
						INIT_BD_I(NDO_DATA_EVENTTYPE, eventdata->event_type),
						INIT_BD_I(NDO_DATA_RECURRING, eventdata->recurring),
						INIT_BD_UL(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
						INIT_BD_SE(NDO_DATA_HOST, host_name)
					};
					ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
							timed_event_data, ARRAY_SIZE(timed_event_data), TRUE);
				}
				break;

			case EVENT_SCHEDULED_DOWNTIME:
				temp_downtime = find_downtime(ANY_DOWNTIME,
						(unsigned long)eventdata->event_data);
				if (temp_downtime) {
					host_name = temp_downtime->host_name;
					service_desc = temp_downtime->service_description;
				}
				{
					struct ndo_broker_data timed_event_data[] = {
						INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(eventdata),
						INIT_BD_I(NDO_DATA_EVENTTYPE, eventdata->event_type),
						INIT_BD_I(NDO_DATA_RECURRING, eventdata->recurring),
						INIT_BD_UL(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time),
						INIT_BD_SE(NDO_DATA_HOST, host_name),
						INIT_BD_SE(NDO_DATA_SERVICE, service_desc)
					};
					ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
							timed_event_data, ARRAY_SIZE(timed_event_data), TRUE);
				}
				break;

			default:
				{
					struct ndo_broker_data timed_event_data[] = {
						INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(eventdata),
						INIT_BD_I(NDO_DATA_EVENTTYPE, eventdata->event_type),
						INIT_BD_I(NDO_DATA_RECURRING, eventdata->recurring),
						INIT_BD_UL(NDO_DATA_RUNTIME, (unsigned long)eventdata->run_time)
					};
					ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
							timed_event_data, ARRAY_SIZE(timed_event_data), TRUE);
				}
				break;
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_log_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_LOG_DATA) {

		nebstruct_log_data *logdata = data;

		struct ndo_broker_data log_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(logdata),
			INIT_BD_UL(NDO_DATA_LOGENTRYTIME, logdata->entry_time),
			INIT_BD_I(NDO_DATA_LOGENTRYTYPE, logdata->data_type),
			/* Log data strings are not escaped... */
			INIT_BD_S(NDO_DATA_LOGENTRY, logdata->data)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_LOGDATA, log_data,
				ARRAY_SIZE(log_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_system_command_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_SYSTEM_COMMAND_DATA) {

		nebstruct_system_command_data *cmddata = data;

		struct ndo_broker_data system_command_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(cmddata),
			INIT_BD_TV(NDO_DATA_STARTTIME, cmddata->start_time),
			INIT_BD_TV(NDO_DATA_ENDTIME, cmddata->end_time),
			INIT_BD_I(NDO_DATA_TIMEOUT, cmddata->timeout),
			INIT_BD_SE(NDO_DATA_COMMANDLINE, cmddata->command_line),
			INIT_BD_I(NDO_DATA_EARLYTIMEOUT, cmddata->early_timeout),
			INIT_BD_F(NDO_DATA_EXECUTIONTIME, cmddata->execution_time),
			INIT_BD_I(NDO_DATA_RETURNCODE, cmddata->return_code),
			INIT_BD_SE(NDO_DATA_OUTPUT, cmddata->output),
			/* Preparing if system command will have long_output in the future. */
			INIT_BD_SE(NDO_DATA_LONGOUTPUT, cmddata->output)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_SYSTEMCOMMANDDATA,
				system_command_data, ARRAY_SIZE(system_command_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_event_handler_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_EVENT_HANDLER_DATA) {

		nebstruct_event_handler_data *ehanddata = data;

		struct ndo_broker_data event_handler_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(ehanddata),
			INIT_BD_SE(NDO_DATA_HOST, ehanddata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, ehanddata->service_description),
			INIT_BD_I(NDO_DATA_STATETYPE, ehanddata->state_type),
			INIT_BD_I(NDO_DATA_STATE, ehanddata->state),
			INIT_BD_TV(NDO_DATA_STARTTIME, ehanddata->start_time),
			INIT_BD_TV(NDO_DATA_ENDTIME, ehanddata->end_time),
			INIT_BD_I(NDO_DATA_TIMEOUT, ehanddata->timeout),
			INIT_BD_SE(NDO_DATA_COMMANDNAME, ehanddata->command_name),
			INIT_BD_SE(NDO_DATA_COMMANDARGS, ehanddata->command_args),
			INIT_BD_SE(NDO_DATA_COMMANDLINE, ehanddata->command_line),
			INIT_BD_I(NDO_DATA_EARLYTIMEOUT, ehanddata->early_timeout),
			INIT_BD_F(NDO_DATA_EXECUTIONTIME, ehanddata->execution_time),
			INIT_BD_I(NDO_DATA_RETURNCODE, ehanddata->return_code),
			INIT_BD_SE(NDO_DATA_OUTPUT, ehanddata->output),
			/* Preparing if eventhandler will have long_output in the future. */
			INIT_BD_SE(NDO_DATA_LONGOUTPUT, ehanddata->output)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_EVENTHANDLERDATA,
				event_handler_data, ARRAY_SIZE(event_handler_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_notification_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_NOTIFICATION_DATA) {

		nebstruct_notification_data *notdata = data;

		struct ndo_broker_data notification_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(notdata),
			INIT_BD_I(NDO_DATA_NOTIFICATIONTYPE, notdata->notification_type),
			INIT_BD_TV(NDO_DATA_STARTTIME, notdata->start_time),
			INIT_BD_TV(NDO_DATA_ENDTIME, notdata->end_time),
			INIT_BD_SE(NDO_DATA_HOST, notdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, notdata->service_description),
			INIT_BD_I(NDO_DATA_NOTIFICATIONREASON, notdata->reason_type),
			INIT_BD_I(NDO_DATA_STATE, notdata->state),
			INIT_BD_SE(NDO_DATA_OUTPUT, notdata->output),
			/* Preparing if notifications will have long_output in the future. */
			INIT_BD_SE(NDO_DATA_LONGOUTPUT, notdata->output),
			INIT_BD_SE(NDO_DATA_ACKAUTHOR, notdata->ack_author),
			INIT_BD_SE(NDO_DATA_ACKDATA, notdata->ack_data),
			INIT_BD_I(NDO_DATA_ESCALATED, notdata->escalated),
			INIT_BD_I(NDO_DATA_CONTACTSNOTIFIED, notdata->contacts_notified)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_NOTIFICATIONDATA,
				notification_data, ARRAY_SIZE(notification_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_service_check_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_SERVICE_CHECK_DATA) {

		nebstruct_service_check_data *scdata = data;

		/* Modified originally for Nagios XI to send only processed checks. */
		if (scdata->type == NEBTYPE_SERVICECHECK_PROCESSED) {
			struct ndo_broker_data service_check_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(scdata),
				INIT_BD_SE(NDO_DATA_HOST, scdata->host_name),
				INIT_BD_SE(NDO_DATA_SERVICE, scdata->service_description),
				INIT_BD_I(NDO_DATA_CHECKTYPE, scdata->check_type),
				INIT_BD_I(NDO_DATA_CURRENTCHECKATTEMPT, scdata->current_attempt),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, scdata->max_attempts),
				INIT_BD_I(NDO_DATA_STATETYPE, scdata->state_type),
				INIT_BD_I(NDO_DATA_STATE, scdata->state),
				INIT_BD_I(NDO_DATA_TIMEOUT, scdata->timeout),
				INIT_BD_SE(NDO_DATA_COMMANDNAME, scdata->command_name),
				INIT_BD_SE(NDO_DATA_COMMANDARGS, scdata->command_args),
				INIT_BD_SE(NDO_DATA_COMMANDLINE, scdata->command_line),
				INIT_BD_TV(NDO_DATA_STARTTIME, scdata->start_time),
				INIT_BD_TV(NDO_DATA_ENDTIME, scdata->end_time),
				INIT_BD_I(NDO_DATA_EARLYTIMEOUT, scdata->early_timeout),
				INIT_BD_F(NDO_DATA_EXECUTIONTIME, scdata->execution_time),
				INIT_BD_F(NDO_DATA_LATENCY, scdata->latency),
				INIT_BD_I(NDO_DATA_RETURNCODE, scdata->return_code),
				INIT_BD_SE(NDO_DATA_OUTPUT, scdata->output),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, scdata->long_output),
#else
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, NULL),
#endif
				INIT_BD_SE(NDO_DATA_PERFDATA, scdata->perf_data)
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_SERVICECHECKDATA,
					service_check_data, ARRAY_SIZE(service_check_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_host_check_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_HOST_CHECK_DATA) {

		nebstruct_host_check_data *hcdata = data;

		/* Modified originally for Nagios XI to send only processed checks. */
		if (hcdata->type == NEBTYPE_HOSTCHECK_PROCESSED) {
			struct ndo_broker_data host_check_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(hcdata),
				INIT_BD_SE(NDO_DATA_HOST, hcdata->host_name),
				INIT_BD_I(NDO_DATA_CHECKTYPE, hcdata->check_type),
				INIT_BD_I(NDO_DATA_CURRENTCHECKATTEMPT, hcdata->current_attempt),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, hcdata->max_attempts),
				INIT_BD_I(NDO_DATA_STATETYPE, hcdata->state_type),
				INIT_BD_I(NDO_DATA_STATE, hcdata->state),
				INIT_BD_I(NDO_DATA_TIMEOUT, hcdata->timeout),
				INIT_BD_SE(NDO_DATA_COMMANDNAME, hcdata->command_name),
				INIT_BD_SE(NDO_DATA_COMMANDARGS, hcdata->command_args),
				INIT_BD_SE(NDO_DATA_COMMANDLINE, hcdata->command_line),
				INIT_BD_TV(NDO_DATA_STARTTIME, hcdata->start_time),
				INIT_BD_TV(NDO_DATA_ENDTIME, hcdata->end_time),
				INIT_BD_I(NDO_DATA_EARLYTIMEOUT, hcdata->early_timeout),
				INIT_BD_F(NDO_DATA_EXECUTIONTIME, hcdata->execution_time),
				INIT_BD_F(NDO_DATA_LATENCY, hcdata->latency),
				INIT_BD_I(NDO_DATA_RETURNCODE, hcdata->return_code),
				INIT_BD_SE(NDO_DATA_OUTPUT, hcdata->output),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, hcdata->long_output),
#else
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, NULL),
#endif
				INIT_BD_SE(NDO_DATA_PERFDATA, hcdata->perf_data)
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_HOSTCHECKDATA,
					host_check_data, ARRAY_SIZE(host_check_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_comment_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_COMMENT_DATA) {

		nebstruct_comment_data *comdata = data;
		char *comment_text = NULL;
		/* @todo This assignment to data (preserved from before refactoring) is
		* never referenced later, and should likely be an assignment to
		* comment_text to be used as NDO_DATA_COMMENT, otherwise we never pass the
		* actual comment_data! Resolve this later as a separate bug. */
		data = comdata->comment_data;

		struct ndo_broker_data comment_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(comdata),
			INIT_BD_I(NDO_DATA_COMMENTTYPE, comdata->comment_type),
			INIT_BD_SE(NDO_DATA_HOST, comdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, comdata->service_description),
			INIT_BD_UL(NDO_DATA_ENTRYTIME, (unsigned long)comdata->entry_time),
			INIT_BD_SE(NDO_DATA_AUTHORNAME, comdata->author_name),
			INIT_BD_SE(NDO_DATA_COMMENT, comment_text), /* !!! comdata->comment_data ??? */
			INIT_BD_I(NDO_DATA_PERSISTENT, comdata->persistent),
			INIT_BD_I(NDO_DATA_SOURCE, comdata->source),
			INIT_BD_I(NDO_DATA_ENTRYTYPE, comdata->entry_type),
			INIT_BD_I(NDO_DATA_EXPIRES, comdata->expires),
			INIT_BD_UL(NDO_DATA_EXPIRATIONTIME, (unsigned long)comdata->expire_time),
			INIT_BD_UL(NDO_DATA_COMMENTID, comdata->comment_id)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_COMMENTDATA,
				comment_data, ARRAY_SIZE(comment_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_downtime_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_DOWNTIME_DATA) {

		nebstruct_downtime_data *downdata = data;

		struct ndo_broker_data downtime_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(downdata),
			INIT_BD_I(NDO_DATA_DOWNTIMETYPE, downdata->downtime_type),
			INIT_BD_SE(NDO_DATA_HOST, downdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, downdata->service_description),
			INIT_BD_UL(NDO_DATA_ENTRYTIME, (unsigned long)downdata->entry_time),
			INIT_BD_SE(NDO_DATA_AUTHORNAME, downdata->author_name),
			INIT_BD_SE(NDO_DATA_COMMENT, downdata->comment_data),
			INIT_BD_UL(NDO_DATA_STARTTIME, (unsigned long)downdata->start_time),
			INIT_BD_UL(NDO_DATA_ENDTIME, (unsigned long)downdata->end_time),
			INIT_BD_I(NDO_DATA_FIXED, downdata->fixed),
			INIT_BD_UL(NDO_DATA_DURATION, (unsigned long)downdata->duration),
			INIT_BD_UL(NDO_DATA_TRIGGEREDBY, (unsigned long)downdata->triggered_by),
			INIT_BD_UL(NDO_DATA_DOWNTIMEID, (unsigned long)downdata->downtime_id)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_DOWNTIMEDATA,
				downtime_data, ARRAY_SIZE(downtime_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_flapping_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_FLAPPING_DATA) {

		nebstruct_flapping_data *flapdata = data;

		comment *temp_comment = find_comment(flapdata->comment_id,
				(flapdata->flapping_type == HOST_FLAPPING) ? HOST_COMMENT :
						SERVICE_COMMENT);

		struct ndo_broker_data flapping_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(flapdata),
			INIT_BD_I(NDO_DATA_FLAPPINGTYPE, flapdata->flapping_type),
			INIT_BD_SE(NDO_DATA_HOST, flapdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, flapdata->service_description),
			INIT_BD_F(NDO_DATA_PERCENTSTATECHANGE, flapdata->percent_change),
			INIT_BD_F(NDO_DATA_HIGHTHRESHOLD, flapdata->high_threshold),
			INIT_BD_F(NDO_DATA_LOWTHRESHOLD, flapdata->low_threshold),
			INIT_BD_UL(NDO_DATA_COMMENTTIME, (temp_comment) ? (unsigned long)temp_comment->entry_time : 0L),
			INIT_BD_UL(NDO_DATA_COMMENTID, flapdata->comment_id)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_FLAPPINGDATA,
				flapping_data, ARRAY_SIZE(flapping_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_program_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_PROGRAM_STATUS_DATA) {

		nebstruct_program_status_data *psdata = data;

		struct ndo_broker_data program_status_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(psdata),
			INIT_BD_UL(NDO_DATA_PROGRAMSTARTTIME, (unsigned long)psdata->program_start),
			INIT_BD_I(NDO_DATA_PROCESSID, psdata->pid),
			INIT_BD_I(NDO_DATA_DAEMONMODE, psdata->daemon_mode),
#ifdef BUILD_NAGIOS_4X
			INIT_BD_UL(NDO_DATA_LASTCOMMANDCHECK, 0L),
#else
			INIT_BD_UL(NDO_DATA_LASTCOMMANDCHECK, (unsigned long)psdata->last_command_check),
#endif
			INIT_BD_UL(NDO_DATA_LASTLOGROTATION, psdata->last_log_rotation),
			INIT_BD_I(NDO_DATA_NOTIFICATIONSENABLED, psdata->notifications_enabled),
			INIT_BD_I(NDO_DATA_ACTIVESERVICECHECKSENABLED, psdata->active_service_checks_enabled),
			INIT_BD_I(NDO_DATA_PASSIVESERVICECHECKSENABLED, psdata->passive_service_checks_enabled),
			INIT_BD_I(NDO_DATA_ACTIVEHOSTCHECKSENABLED, psdata->active_host_checks_enabled),
			INIT_BD_I(NDO_DATA_PASSIVEHOSTCHECKSENABLED, psdata->passive_host_checks_enabled),
			INIT_BD_I(NDO_DATA_EVENTHANDLERSENABLED, psdata->event_handlers_enabled),
			INIT_BD_I(NDO_DATA_FLAPDETECTIONENABLED, psdata->flap_detection_enabled),
#ifdef BUILD_NAGIOS_4X
			INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
#else
			INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, psdata->failure_prediction_enabled),
#endif
			INIT_BD_I(NDO_DATA_PROCESSPERFORMANCEDATA, psdata->process_performance_data),
			INIT_BD_I(NDO_DATA_OBSESSOVERHOSTS, psdata->obsess_over_hosts),
			INIT_BD_I(NDO_DATA_OBSESSOVERSERVICES, psdata->obsess_over_services),
			INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, psdata->modified_host_attributes),
			INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, psdata->modified_service_attributes),
			INIT_BD_SE(NDO_DATA_GLOBALHOSTEVENTHANDLER, psdata->global_host_event_handler),
			INIT_BD_SE(NDO_DATA_GLOBALSERVICEEVENTHANDLER, psdata->global_service_event_handler),
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_PROGRAMSTATUSDATA,
				program_status_data, ARRAY_SIZE(program_status_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_host_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_HOST_STATUS_DATA) {

		nebstruct_host_status_data *hsdata = data;
		host *temp_host = hsdata->object_ptr;
		if (!temp_host) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data host_status_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(hsdata),
				INIT_BD_SE(NDO_DATA_HOST, temp_host->name),
				INIT_BD_SE(NDO_DATA_OUTPUT, temp_host->plugin_output),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, temp_host->long_plugin_output),
#else
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, NULL),
#endif
				INIT_BD_SE(NDO_DATA_PERFDATA, temp_host->perf_data),
				INIT_BD_I(NDO_DATA_CURRENTSTATE, temp_host->current_state),
				INIT_BD_I(NDO_DATA_HASBEENCHECKED, temp_host->has_been_checked),
				INIT_BD_I(NDO_DATA_SHOULDBESCHEDULED, temp_host->should_be_scheduled),
				INIT_BD_I(NDO_DATA_CURRENTCHECKATTEMPT, temp_host->current_attempt),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, temp_host->max_attempts),
				INIT_BD_UL(NDO_DATA_LASTHOSTCHECK, (unsigned long)temp_host->last_check),
				INIT_BD_UL(NDO_DATA_NEXTHOSTCHECK, (unsigned long)temp_host->next_check),
				INIT_BD_I(NDO_DATA_CHECKTYPE, temp_host->check_type),
				INIT_BD_UL(NDO_DATA_LASTSTATECHANGE, (unsigned long)temp_host->last_state_change),
				INIT_BD_UL(NDO_DATA_LASTHARDSTATECHANGE, (unsigned long)temp_host->last_hard_state_change),
				INIT_BD_I(NDO_DATA_LASTHARDSTATE, temp_host->last_hard_state),
				INIT_BD_UL(NDO_DATA_LASTTIMEUP, (unsigned long)temp_host->last_time_up),
				INIT_BD_UL(NDO_DATA_LASTTIMEDOWN, (unsigned long)temp_host->last_time_down),
				INIT_BD_UL(NDO_DATA_LASTTIMEUNREACHABLE, (unsigned long)temp_host->last_time_unreachable),
				INIT_BD_I(NDO_DATA_STATETYPE, temp_host->state_type),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_UL(NDO_DATA_LASTHOSTNOTIFICATION, (unsigned long)temp_host->last_notification),
				INIT_BD_UL(NDO_DATA_NEXTHOSTNOTIFICATION, (unsigned long)temp_host->next_notification),
#else
				INIT_BD_UL(NDO_DATA_LASTHOSTNOTIFICATION, (unsigned long)temp_host->last_host_notification),
				INIT_BD_UL(NDO_DATA_NEXTHOSTNOTIFICATION, (unsigned long)temp_host->next_host_notification),
#endif
				INIT_BD_I(NDO_DATA_NOMORENOTIFICATIONS, temp_host->no_more_notifications),
				INIT_BD_I(NDO_DATA_NOTIFICATIONSENABLED, temp_host->notifications_enabled),
				INIT_BD_I(NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, temp_host->problem_has_been_acknowledged),
				INIT_BD_I(NDO_DATA_ACKNOWLEDGEMENTTYPE, temp_host->acknowledgement_type),
				INIT_BD_I(NDO_DATA_CURRENTNOTIFICATIONNUMBER, temp_host->current_notification_number),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_PASSIVEHOSTCHECKSENABLED, temp_host->accept_passive_checks),
#else
				INIT_BD_I(NDO_DATA_PASSIVEHOSTCHECKSENABLED, temp_host->accept_passive_host_checks),
#endif
				INIT_BD_I(NDO_DATA_EVENTHANDLERENABLED, temp_host->event_handler_enabled),
				INIT_BD_I(NDO_DATA_ACTIVEHOSTCHECKSENABLED, temp_host->checks_enabled),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONENABLED, temp_host->flap_detection_enabled),
				INIT_BD_I(NDO_DATA_ISFLAPPING, temp_host->is_flapping),
				INIT_BD_F(NDO_DATA_PERCENTSTATECHANGE, temp_host->percent_state_change),
				INIT_BD_F(NDO_DATA_LATENCY, temp_host->latency),
				INIT_BD_F(NDO_DATA_EXECUTIONTIME, temp_host->execution_time),
				INIT_BD_I(NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, temp_host->scheduled_downtime_depth),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
#else
				INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, temp_host->failure_prediction_enabled),
#endif
				INIT_BD_I(NDO_DATA_PROCESSPERFORMANCEDATA, temp_host->process_performance_data),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_OBSESSOVERHOST, temp_host->obsess),
#else
				INIT_BD_I(NDO_DATA_OBSESSOVERHOST, temp_host->obsess_over_host),
#endif
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, temp_host->modified_attributes),
				INIT_BD_SE(NDO_DATA_EVENTHANDLER, temp_host->event_handler),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_host->check_command),
#else
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_host->host_check_command),
#endif
				INIT_BD_F(NDO_DATA_NORMALCHECKINTERVAL, temp_host->check_interval),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, temp_host->retry_interval),
#else
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, 0.0),
#endif
				INIT_BD_SE(NDO_DATA_HOSTCHECKPERIOD, temp_host->check_period)
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_HOSTSTATUSDATA,
					host_status_data, ARRAY_SIZE(host_status_data), FALSE);
		}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		ndomod_customvars_serialize(temp_host->custom_variables, dbufp);
#endif
		ndomod_enddata_serialize(dbufp);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_service_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_SERVICE_STATUS_DATA) {

		nebstruct_service_status_data *ssdata = data;
		service *temp_service = ssdata->object_ptr;
		if (!temp_service) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data service_status_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(ssdata),
				INIT_BD_SE(NDO_DATA_HOST, temp_service->host_name),
				INIT_BD_SE(NDO_DATA_SERVICE, temp_service->description),
				INIT_BD_SE(NDO_DATA_OUTPUT, temp_service->plugin_output),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, temp_service->long_plugin_output),
#else
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, NULL),
#endif
				INIT_BD_SE(NDO_DATA_PERFDATA, temp_service->perf_data),
				INIT_BD_I(NDO_DATA_CURRENTSTATE, temp_service->current_state),
				INIT_BD_I(NDO_DATA_HASBEENCHECKED, temp_service->has_been_checked),
				INIT_BD_I(NDO_DATA_SHOULDBESCHEDULED, temp_service->should_be_scheduled),
				INIT_BD_I(NDO_DATA_CURRENTCHECKATTEMPT, temp_service->current_attempt),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, temp_service->max_attempts),
				INIT_BD_UL(NDO_DATA_LASTSERVICECHECK, (unsigned long)temp_service->last_check),
				INIT_BD_UL(NDO_DATA_NEXTSERVICECHECK, (unsigned long)temp_service->next_check),
				INIT_BD_I(NDO_DATA_CHECKTYPE, temp_service->check_type),
				INIT_BD_UL(NDO_DATA_LASTSTATECHANGE, (unsigned long)temp_service->last_state_change),
				INIT_BD_UL(NDO_DATA_LASTHARDSTATECHANGE, (unsigned long)temp_service->last_hard_state_change),
				INIT_BD_I(NDO_DATA_LASTHARDSTATE, temp_service->last_hard_state),
				INIT_BD_UL(NDO_DATA_LASTTIMEOK, (unsigned long)temp_service->last_time_ok),
				INIT_BD_UL(NDO_DATA_LASTTIMEWARNING, (unsigned long)temp_service->last_time_warning),
				INIT_BD_UL(NDO_DATA_LASTTIMEUNKNOWN, (unsigned long)temp_service->last_time_unknown),
				INIT_BD_UL(NDO_DATA_LASTTIMECRITICAL, (unsigned long)temp_service->last_time_critical),
				INIT_BD_I(NDO_DATA_STATETYPE, temp_service->state_type),
				INIT_BD_UL(NDO_DATA_LASTSERVICENOTIFICATION, (unsigned long)temp_service->last_notification),
				INIT_BD_UL(NDO_DATA_NEXTSERVICENOTIFICATION, (unsigned long)temp_service->next_notification),
				INIT_BD_I(NDO_DATA_NOMORENOTIFICATIONS, temp_service->no_more_notifications),
				INIT_BD_I(NDO_DATA_NOTIFICATIONSENABLED, temp_service->notifications_enabled),
				INIT_BD_I(NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, temp_service->problem_has_been_acknowledged),
				INIT_BD_I(NDO_DATA_ACKNOWLEDGEMENTTYPE, temp_service->acknowledgement_type),
				INIT_BD_I(NDO_DATA_CURRENTNOTIFICATIONNUMBER, temp_service->current_notification_number),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_PASSIVESERVICECHECKSENABLED, temp_service->accept_passive_checks),
#else
				INIT_BD_I(NDO_DATA_PASSIVESERVICECHECKSENABLED, temp_service->accept_passive_service_checks),
#endif
				INIT_BD_I(NDO_DATA_EVENTHANDLERENABLED, temp_service->event_handler_enabled),
				INIT_BD_I(NDO_DATA_ACTIVESERVICECHECKSENABLED, temp_service->checks_enabled),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONENABLED, temp_service->flap_detection_enabled),
				INIT_BD_I(NDO_DATA_ISFLAPPING, temp_service->is_flapping),
				INIT_BD_F(NDO_DATA_PERCENTSTATECHANGE, temp_service->percent_state_change),
				INIT_BD_F(NDO_DATA_LATENCY, temp_service->latency),
				INIT_BD_F(NDO_DATA_EXECUTIONTIME, temp_service->execution_time),
				INIT_BD_I(NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, temp_service->scheduled_downtime_depth),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
#else
				INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, temp_service->failure_prediction_enabled),
#endif
				INIT_BD_I(NDO_DATA_PROCESSPERFORMANCEDATA, temp_service->process_performance_data),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_OBSESSOVERSERVICE, temp_service->obsess),
#else
				INIT_BD_I(NDO_DATA_OBSESSOVERSERVICE, temp_service->obsess_over_service),
#endif
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, temp_service->modified_attributes),
				INIT_BD_SE(NDO_DATA_EVENTHANDLER, temp_service->event_handler),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_service->check_command),
#else
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_service->service_check_command),
#endif
				INIT_BD_F(NDO_DATA_NORMALCHECKINTERVAL, temp_service->check_interval),
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, temp_service->retry_interval),
				INIT_BD_SE(NDO_DATA_SERVICECHECKPERIOD, temp_service->check_period)
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_SERVICESTATUSDATA,
					service_status_data, ARRAY_SIZE(service_status_data), FALSE);
		}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		ndomod_customvars_serialize(temp_service->custom_variables, dbufp);
#endif
		ndomod_enddata_serialize(dbufp);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_adaptive_program_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA) {

		nebstruct_adaptive_program_data *apdata = data;
		const char *global_host_event_handler = NULL; /* @todo ??? */
		const char *global_service_event_handler = NULL; /* @todo ??? */

		struct ndo_broker_data adaptive_program_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(apdata),
			INIT_BD_I(NDO_DATA_COMMANDTYPE, apdata->command_type),
			INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTE, apdata->modified_host_attribute),
			INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, apdata->modified_host_attributes),
			INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, apdata->modified_service_attribute),
			INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, apdata->modified_service_attributes),
			INIT_BD_SE(NDO_DATA_GLOBALHOSTEVENTHANDLER, global_host_event_handler), /* ??? */
			INIT_BD_SE(NDO_DATA_GLOBALSERVICEEVENTHANDLER, global_service_event_handler), /* ??? */
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVEPROGRAMDATA,
				adaptive_program_data, ARRAY_SIZE(adaptive_program_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_adaptive_host_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_ADAPTIVE_HOST_DATA) {

		nebstruct_adaptive_host_data *ahdata = data;
		host *temp_host = ahdata->object_ptr;
		if (!temp_host) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data adaptive_host_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(ahdata),
				INIT_BD_I(NDO_DATA_COMMANDTYPE, ahdata->command_type),
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTE, ahdata->modified_attribute),
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, ahdata->modified_attributes),
				INIT_BD_SE(NDO_DATA_HOST, temp_host->name),
				INIT_BD_SE(NDO_DATA_EVENTHANDLER, temp_host->event_handler),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_host->check_command),
#else
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_host->host_check_command),
#endif
				INIT_BD_F(NDO_DATA_NORMALCHECKINTERVAL, temp_host->check_interval),
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, temp_host->retry_interval),
#else
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, 0.0),
#endif
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, temp_host->max_attempts),
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVEHOSTDATA,
					adaptive_host_data, ARRAY_SIZE(adaptive_host_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_adaptive_service_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA) {

		nebstruct_adaptive_service_data *asdata = data;
		service *temp_service = asdata->object_ptr;
		if (!temp_service) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data adaptive_service_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(asdata),
				INIT_BD_I(NDO_DATA_COMMANDTYPE, asdata->command_type),
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, asdata->modified_attribute),
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, asdata->modified_attributes),
				INIT_BD_SE(NDO_DATA_HOST, temp_service->host_name),
				INIT_BD_SE(NDO_DATA_SERVICE, temp_service->description),
				INIT_BD_SE(NDO_DATA_EVENTHANDLER, temp_service->event_handler),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_service->check_command),
#else
				INIT_BD_SE(NDO_DATA_CHECKCOMMAND, temp_service->service_check_command),
#endif
				INIT_BD_F(NDO_DATA_NORMALCHECKINTERVAL, temp_service->check_interval),
				INIT_BD_F(NDO_DATA_RETRYCHECKINTERVAL, temp_service->retry_interval),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, temp_service->max_attempts),
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVESERVICEDATA,
					adaptive_service_data, ARRAY_SIZE(adaptive_service_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_external_command_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA) {

		nebstruct_external_command_data *ecdata = data;

		struct ndo_broker_data external_command_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(ecdata),
			INIT_BD_I(NDO_DATA_COMMANDTYPE, ecdata->command_type),
			INIT_BD_UL(NDO_DATA_ENTRYTIME, (unsigned long)ecdata->entry_time),
			INIT_BD_SE(NDO_DATA_COMMANDSTRING, ecdata->command_string),
			INIT_BD_SE(NDO_DATA_COMMANDARGS, ecdata->command_args),
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_EXTERNALCOMMANDDATA,
				external_command_data, ARRAY_SIZE(external_command_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_aggregated_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_AGGREGATED_STATUS_DATA) {

		nebstruct_aggregated_status_data *agsdata = data;

		struct ndo_broker_data aggregated_status_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(agsdata)
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_AGGREGATEDSTATUSDATA,
				aggregated_status_data, ARRAY_SIZE(aggregated_status_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_retention_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_retention_data *rdata = data;

	switch (phase) {
		case bdp_preprocessing:
			if (!(process_options & NDOMOD_PROCESS_RETENTION_DATA))
				return bdr_stop;
			break;

		case bdp_mainprocessing:
			{
				struct ndo_broker_data retention_data[] = {
					INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(rdata),
				};
				ndomod_broker_data_serialize(dbufp, NDO_API_RETENTIONDATA,
						retention_data, ARRAY_SIZE(retention_data), TRUE);
			}
			break;

		case bdp_postprocessing:
			/* If retained config was just read, dump it. */
			if (rdata->type == NEBTYPE_RETENTIONDATA_ENDLOAD) {
				ndomod_write_config(NDOMOD_CONFIG_DUMP_RETAINED);
			}
			break;

		default:
			return bdr_ephase;
	}

	return bdr_ok;
}

static bd_result ndomod_broker_contact_notification_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_NOTIFICATION_DATA) {

		nebstruct_contact_notification_data *cnotdata = data;

		struct ndo_broker_data contact_notification_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(cnotdata),
			INIT_BD_I(NDO_DATA_NOTIFICATIONTYPE, cnotdata->notification_type),
			INIT_BD_TV(NDO_DATA_STARTTIME, cnotdata->start_time),
			INIT_BD_TV(NDO_DATA_ENDTIME, cnotdata->end_time),
			INIT_BD_SE(NDO_DATA_HOST, cnotdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, cnotdata->service_description),
			INIT_BD_SE(NDO_DATA_CONTACTNAME, cnotdata->contact_name),
			INIT_BD_I(NDO_DATA_NOTIFICATIONREASON, cnotdata->reason_type),
			INIT_BD_I(NDO_DATA_STATE, cnotdata->state),
			INIT_BD_SE(NDO_DATA_OUTPUT, cnotdata->output),
			/* Preparing long output for the future */
			INIT_BD_SE(NDO_DATA_LONGOUTPUT, cnotdata->output),
			/* Preparing for long_output in the future */
			INIT_BD_SE(NDO_DATA_OUTPUT, cnotdata->output), /* @todo Dup NDO_DATA_OUTPUT ??? */
			INIT_BD_SE(NDO_DATA_ACKAUTHOR, cnotdata->ack_author),
			INIT_BD_SE(NDO_DATA_ACKDATA, cnotdata->ack_data),
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_CONTACTNOTIFICATIONDATA,
				contact_notification_data, ARRAY_SIZE(contact_notification_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_contact_notification_method_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_NOTIFICATION_DATA) {

		nebstruct_contact_notification_method_data *cnotmdata = data;

		struct ndo_broker_data contact_notification_method_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(cnotmdata),
			INIT_BD_I(NDO_DATA_NOTIFICATIONTYPE, cnotmdata->notification_type),
			INIT_BD_TV(NDO_DATA_STARTTIME, cnotmdata->start_time),
			INIT_BD_TV(NDO_DATA_ENDTIME, cnotmdata->end_time),
			INIT_BD_SE(NDO_DATA_HOST, cnotmdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, cnotmdata->service_description),
			INIT_BD_SE(NDO_DATA_CONTACTNAME, cnotmdata->contact_name),
			INIT_BD_SE(NDO_DATA_COMMANDNAME, cnotmdata->command_name),
			INIT_BD_SE(NDO_DATA_COMMANDARGS, cnotmdata->command_args),
			INIT_BD_I(NDO_DATA_NOTIFICATIONREASON, cnotmdata->reason_type),
			INIT_BD_I(NDO_DATA_STATE, cnotmdata->state),
			INIT_BD_SE(NDO_DATA_OUTPUT, cnotmdata->output),
			INIT_BD_SE(NDO_DATA_ACKAUTHOR, cnotmdata->ack_author),
			INIT_BD_SE(NDO_DATA_ACKDATA, cnotmdata->ack_data),
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_CONTACTNOTIFICATIONMETHODDATA,
				contact_notification_method_data,
				ARRAY_SIZE(contact_notification_method_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_acknowledgement_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA) {

		nebstruct_acknowledgement_data *ackdata = data;

		struct ndo_broker_data acknowledgement_data[] = {
			INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(ackdata),
			INIT_BD_I(NDO_DATA_ACKNOWLEDGEMENTTYPE, ackdata->acknowledgement_type),
			INIT_BD_SE(NDO_DATA_HOST, ackdata->host_name),
			INIT_BD_SE(NDO_DATA_SERVICE, ackdata->service_description),
			INIT_BD_SE(NDO_DATA_AUTHORNAME, ackdata->author_name),
			INIT_BD_SE(NDO_DATA_COMMENT, ackdata->comment_data),
			INIT_BD_I(NDO_DATA_STATE, ackdata->state),
			INIT_BD_I(NDO_DATA_STICKY, ackdata->is_sticky),
			INIT_BD_I(NDO_DATA_PERSISTENT, ackdata->persistent_comment),
			INIT_BD_I(NDO_DATA_NOTIFYCONTACTS, ackdata->notify_contacts),
		};
		ndomod_broker_data_serialize(dbufp, NDO_API_ACKNOWLEDGEMENTDATA,
				acknowledgement_data, ARRAY_SIZE(acknowledgement_data), TRUE);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_state_change_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_STATE_CHANGE_DATA) {

		nebstruct_statechange_data *schangedata = data;
		int last_state = -1;
		int last_hard_state = -1;

#ifdef BUILD_NAGIOS_2X
		/* Find host/service and get last state info */
		if (!schangedata->service_description) {
			host *temp_host = find_host(schangedata->host_name);
			if (!temp_host) {
				return bdr_enoent;
			}
			last_state = temp_host->last_state;
			last_hard_state = temp_host->last_hard_state;
		} else {
			service *temp_service = find_service(schangedata->host_name,
					schangedata->service_description);
			if (!temp_service) {
				return bdr_enoent;
			}
			last_state = temp_service->last_state;
			last_hard_state = temp_service->last_hard_state;
		}
#else
		/* Get the last state info */
		if (!schangedata->service_description) {
			host *temp_host = schangedata->object_ptr;
			if (!temp_host) {
				return bdr_enoent;
			}
			last_state = temp_host->last_state;
			last_hard_state = temp_host->last_hard_state;
		} else {
			service *temp_service = schangedata->object_ptr;
			if (!temp_service) {
				return bdr_enoent;
			}
			last_state = temp_service->last_state;
			last_hard_state = temp_service->last_hard_state;
		}
#endif
		{
			struct ndo_broker_data state_change_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(schangedata),
				INIT_BD_I(NDO_DATA_STATECHANGETYPE, schangedata->statechange_type),
				INIT_BD_SE(NDO_DATA_HOST, schangedata->host_name),
				INIT_BD_SE(NDO_DATA_SERVICE, schangedata->service_description),
				INIT_BD_I(NDO_DATA_STATECHANGE, TRUE),
				INIT_BD_I(NDO_DATA_STATE, schangedata->state),
				INIT_BD_I(NDO_DATA_STATETYPE, schangedata->state_type),
				INIT_BD_I(NDO_DATA_CURRENTCHECKATTEMPT, schangedata->current_attempt),
				INIT_BD_I(NDO_DATA_MAXCHECKATTEMPTS, schangedata->max_attempts),
				INIT_BD_I(NDO_DATA_LASTSTATE, last_state),
				INIT_BD_I(NDO_DATA_LASTHARDSTATE, last_hard_state),
				INIT_BD_SE(NDO_DATA_OUTPUT, schangedata->output),
				/* Preparing for long_output in the future */
				INIT_BD_SE(NDO_DATA_LONGOUTPUT, schangedata->output),
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_STATECHANGEDATA,
					state_change_data, sizeof(state_change_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

#if defined(BUILD_NAGIOS_3X) || defined(BUILD_NAGIOS_4X)
static bd_result ndomod_broker_contact_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_CONTACT_STATUS_DATA) {

		nebstruct_contact_status_data *csdata = data;
		contact *temp_contact = csdata->object_ptr;
		if (!temp_contact) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data contact_status_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(csdata),
				INIT_BD_SE(NDO_DATA_CONTACTNAME, temp_contact->name),
				INIT_BD_I(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
				INIT_BD_I(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
				INIT_BD_UL(NDO_DATA_LASTHOSTNOTIFICATION, (unsigned long)temp_contact->last_host_notification),
				INIT_BD_UL(NDO_DATA_LASTSERVICENOTIFICATION, (unsigned long)temp_contact->last_service_notification),
				INIT_BD_UL(NDO_DATA_MODIFIEDCONTACTATTRIBUTES, temp_contact->modified_attributes),
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, temp_contact->modified_host_attributes),
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, temp_contact->modified_service_attributes)
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_CONTACTSTATUSDATA,
					contact_status_data, ARRAY_SIZE(contact_status_data), FALSE);
		}

		/* dump customvars */
		ndomod_customvars_serialize(temp_contact->custom_variables, dbufp);

		ndomod_enddata_serialize(dbufp);

	} NDOMOD_BD_COMMON_SWITCH_POST;
}

static bd_result ndomod_broker_adaptive_contact_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	NDOMOD_BD_COMMON_SWITCH_PRE(NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA) {

		nebstruct_adaptive_contact_data *acdata = data;
		contact *temp_contact = acdata->object_ptr;
		if (!temp_contact) {
			return bdr_enoent;
		}
		{
			struct ndo_broker_data adaptive_contact_data[] = {
				INIT_BD_TYPE_FLAGS_ATTRIBUTES_TIMESTAMP(acdata),
				INIT_BD_I(NDO_DATA_COMMANDTYPE, acdata->command_type),
				INIT_BD_UL(NDO_DATA_MODIFIEDCONTACTATTRIBUTE, acdata->modified_attribute),
				INIT_BD_UL(NDO_DATA_MODIFIEDCONTACTATTRIBUTES, acdata->modified_attributes),
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTE, acdata->modified_host_attribute),
				INIT_BD_UL(NDO_DATA_MODIFIEDHOSTATTRIBUTES, acdata->modified_host_attributes),
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTE, acdata->modified_service_attribute),
				INIT_BD_UL(NDO_DATA_MODIFIEDSERVICEATTRIBUTES, acdata->modified_service_attributes),
				INIT_BD_SE(NDO_DATA_CONTACTNAME, temp_contact->name),
				INIT_BD_I(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
				INIT_BD_I(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
			};
			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVECONTACTDATA,
					adaptive_contact_data, ARRAY_SIZE(adaptive_contact_data), TRUE);
		}

	} NDOMOD_BD_COMMON_SWITCH_POST;
}
#endif


/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* Dumps all configuration data to the sink. */
static int ndomod_write_config(int config_type) {
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	struct timeval now;
#ifdef DEBUG_TIME_CONFIG_DUMP
	struct timeval start;
	struct timeval stop;
	struct timeval elapsed;
#endif
	int result;

	if (!(ndomod_config_output_options & config_type)) return NDO_OK;

	gettimeofday(&now, NULL);

	/* Start the config dump. */
	snprintf(temp_buffer, sizeof(temp_buffer)
			,"\n\n%d:\n%d=%s\n%d=%ld.%06ld\n%d\n\n"
			,NDO_API_STARTCONFIGDUMP
			,NDO_DATA_CONFIGDUMPTYPE
			,(config_type == NDOMOD_CONFIG_DUMP_ORIGINAL)
					? NDO_API_CONFIGDUMP_ORIGINAL : NDO_API_CONFIGDUMP_RETAINED
			,NDO_DATA_TIMESTAMP
			,now.tv_sec
			,now.tv_usec
			,NDO_API_ENDDATA
	);
	temp_buffer[sizeof(temp_buffer)-1] = '\0';
	ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

	/* Dump the object config. */
#ifdef DEBUG_TIME_CONFIG_DUMP
	gettimeofday(&start, NULL);
#endif

	result = ndomod_write_object_config(config_type);

#ifdef DEBUG_TIME_CONFIG_DUMP
	gettimeofday(&stop, NULL);
	elapsed.tv_sec = stop.tv_sec - start.tv_sec;
	elapsed.tv_usec = stop.tv_usec - start.tv_usec;
	if (elapsed.tv_usec < 0) elapsed.tv_sec--, elapsed.tv_usec += 1000000;
	printf("%s: start: %ld.%06ld, stop: %ld.%06ld, elapsed: %ld.%06ld sec.\n",
			__func__,
			(long)start.tv_sec, (long)start.tv_usec,
			(long)stop.tv_sec, (long)stop.tv_usec,
			(long)elapsed.tv_sec, (long)elapsed.tv_usec
	);
#endif

	if (result != NDO_OK) return result;

	/* End the config dump. */
	snprintf(temp_buffer, sizeof(temp_buffer)
			,"\n\n%d:\n%d=%ld.%06ld\n%d\n\n"
			,NDO_API_ENDCONFIGDUMP
			,NDO_DATA_TIMESTAMP
			,now.tv_sec
			,now.tv_usec
			,NDO_API_ENDDATA
	);
	temp_buffer[sizeof(temp_buffer)-1] = '\0';
	ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

	return NDO_OK;
}


/* Dumps object configuration data to the sink. */
static int ndomod_write_object_config(int config_type) {
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	ndo_dbuf dbuf;
	struct timeval now;
	int x = 0;
	command *temp_command = NULL;
	timeperiod *temp_timeperiod = NULL;
	contact *temp_contact = NULL;
	contactgroup *temp_contactgroup = NULL;
	host *temp_host = NULL;
	hostgroup *temp_hostgroup = NULL;
	service *temp_service = NULL;
	servicegroup *temp_servicegroup = NULL;
	hostescalation *temp_hostescalation = NULL;
	serviceescalation *temp_serviceescalation = NULL;
	hostdependency *temp_hostdependency = NULL;
	servicedependency *temp_servicedependency = NULL;


	if (!(ndomod_process_options & NDOMOD_PROCESS_OBJECT_CONFIG_DATA)) return NDO_OK;

	if (!(ndomod_config_output_options & config_type)) return NDO_OK;

	/* Get current time. */
	gettimeofday(&now, NULL);

	/* Initialize our dynamic buffer (2KB chunk size). */
	ndo_dbuf_init(&dbuf, 2048);


	/* Command config. */
	for (temp_command = command_list; temp_command; temp_command = temp_command->next) {
		{
			struct ndo_broker_data command_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_COMMANDNAME, temp_command->name),
				INIT_BD_SE(NDO_DATA_COMMANDLINE, temp_command->command_line),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_COMMANDDEFINITION,
					command_definition, ARRAY_SIZE(command_definition), TRUE);
		}

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE); /* Sink the data... */
		ndo_dbuf_reset(&dbuf); /* ...and reset our dynamic buffer to empty. */

	} /* Command config. */


	/* Timeperiod config. */
	for (temp_timeperiod = timeperiod_list; temp_timeperiod; temp_timeperiod = temp_timeperiod->next) {
		{
			struct ndo_broker_data timeperiod_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_TIMEPERIODNAME, temp_timeperiod->name),
				INIT_BD_SE(NDO_DATA_TIMEPERIODALIAS, temp_timeperiod->alias),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_TIMEPERIODDEFINITION,
					timeperiod_definition, ARRAY_SIZE(timeperiod_definition), FALSE);
		}

		/* Timeranges for each day. */
		for (x = 0; x < 7; x++) {
			timerange *temp_timerange = temp_timeperiod->days[x];
			for (; temp_timerange; temp_timerange = temp_timerange->next) {
				ndo_dbuf_printf(&dbuf, "\n"STRINGIFY(NDO_DATA_TIMERANGE)"=%d:%lu-%lu",
						x,
						temp_timerange->range_start,
						temp_timerange->range_end
				);
			}
		}
		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE); /* Sink the data... */
		ndo_dbuf_reset(&dbuf); /* ...and reset our dynamic buffer to empty. */

	} /* Timeperiod config. */


	/* Contact config. */
	for (temp_contact = contact_list; temp_contact; temp_contact=temp_contact->next) {

#ifdef BUILD_NAGIOS_4X
		int notify_on_service_downtime = flag_isset(temp_contact->service_notification_options, OPT_DOWNTIME);
		int notify_on_host_downtime = flag_isset(temp_contact->host_notification_options, OPT_DOWNTIME);
		int host_notifications_enabled = temp_contact->host_notifications_enabled;
		int service_notifications_enabled = temp_contact->service_notifications_enabled;
		int can_submit_commands = temp_contact->can_submit_commands;

		int notify_on_service_unknown = flag_isset(temp_contact->service_notification_options, OPT_UNKNOWN);
		int notify_on_service_warning = flag_isset(temp_contact->service_notification_options, OPT_WARNING);
		int notify_on_service_critical = flag_isset(temp_contact->service_notification_options, OPT_CRITICAL);
		int notify_on_service_recovery = flag_isset(temp_contact->service_notification_options, OPT_RECOVERY);
		int notify_on_service_flapping = flag_isset(temp_contact->service_notification_options, OPT_FLAPPING);

		int notify_on_host_down = flag_isset(temp_contact->host_notification_options, OPT_DOWN);
		int notify_on_host_unreachable = flag_isset(temp_contact->host_notification_options, OPT_UNREACHABLE);
		int notify_on_host_recovery = flag_isset(temp_contact->host_notification_options, OPT_RECOVERY);
		int notify_on_host_flapping = flag_isset(temp_contact->host_notification_options, OPT_FLAPPING);
#else
		int notify_on_service_unknown = temp_contact->notify_on_service_unknown;
		int notify_on_service_warning = temp_contact->notify_on_service_warning;
		int notify_on_service_critical = temp_contact->notify_on_service_critical;
		int notify_on_service_recovery = temp_contact->notify_on_service_recovery;
		int notify_on_service_flapping = temp_contact->notify_on_service_flapping;

		int notify_on_host_down = temp_contact->notify_on_host_down;
		int notify_on_host_unreachable = temp_contact->notify_on_host_unreachable;
		int notify_on_host_recovery = temp_contact->notify_on_host_recovery;
		int notify_on_host_flapping = temp_contact->notify_on_host_flapping;
#endif
#ifdef BUILD_NAGIOS_3X
		int notify_on_service_downtime = temp_contact->notify_on_service_downtime;
		int notify_on_host_downtime = temp_contact->notify_on_host_downtime;
		int host_notifications_enabled = temp_contact->host_notifications_enabled;
		int service_notifications_enabled = temp_contact->service_notifications_enabled;
		int can_submit_commands = temp_contact->can_submit_commands;
#endif
#ifdef BUILD_NAGIOS_2X
		int notify_on_service_downtime = 0;
		int notify_on_host_downtime = 0;
		int host_notifications_enabled = 1;
		int service_notifications_enabled = 1;
		int can_submit_commands = 1;
#endif

		{
			struct ndo_broker_data contact_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_CONTACTNAME, temp_contact->name),
				INIT_BD_SE(NDO_DATA_CONTACTALIAS, temp_contact->alias),
				INIT_BD_SE(NDO_DATA_EMAILADDRESS, temp_contact->email),
				INIT_BD_SE(NDO_DATA_PAGERADDRESS, temp_contact->pager),
				INIT_BD_SE(NDO_DATA_HOSTNOTIFICATIONPERIOD, temp_contact->host_notification_period),
				INIT_BD_SE(NDO_DATA_SERVICENOTIFICATIONPERIOD, temp_contact->service_notification_period),
				INIT_BD_I(NDO_DATA_SERVICENOTIFICATIONSENABLED, service_notifications_enabled),
				INIT_BD_I(NDO_DATA_HOSTNOTIFICATIONSENABLED, host_notifications_enabled),
				INIT_BD_I(NDO_DATA_CANSUBMITCOMMANDS, can_submit_commands),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEUNKNOWN, notify_on_service_unknown),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEWARNING, notify_on_service_warning),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICECRITICAL, notify_on_service_critical),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICERECOVERY, notify_on_service_recovery),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEFLAPPING, notify_on_service_flapping),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEDOWNTIME, notify_on_service_downtime),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTDOWN, notify_on_host_down),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTUNREACHABLE, notify_on_host_unreachable),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTRECOVERY, notify_on_host_recovery),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTFLAPPING, notify_on_host_flapping),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTDOWNTIME, notify_on_host_downtime),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_MINIMUMIMPORTANCE, temp_contact->minimum_value),
#endif
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_CONTACTDEFINITION, 
					contact_definition, ARRAY_SIZE(contact_definition), FALSE);
		}

		/* Addresses for each contact. */
		for (x = 0; x < MAX_CONTACT_ADDRESSES; x++) {
			const char *address = temp_contact->address[x];
			ndo_dbuf_printf(&dbuf, "\n"STRINGIFY(NDO_DATA_CONTACTADDRESS)"=%d:", x+1);
			if (address) ndo_dbuf_strcat_escaped(&dbuf, address);
		}

		/* Host notification commands for each contact. */
		ndomod_commands_serialize(temp_contact->host_notification_commands, 
				&dbuf, NDO_DATA_HOSTNOTIFICATIONCOMMAND);

		/* Service notification commands for each contact. */
		ndomod_commands_serialize(temp_contact->service_notification_commands, 
				&dbuf, NDO_DATA_SERVICENOTIFICATIONCOMMAND);

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* Custom variables. */
		ndomod_customvars_serialize(temp_contact->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Contact config. */


	/* Contactgroup config. */
	for (temp_contactgroup = contactgroup_list; temp_contactgroup; temp_contactgroup = temp_contactgroup->next) {
		{
			struct ndo_broker_data contactgroup_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_CONTACTGROUPNAME, temp_contactgroup->group_name),
				INIT_BD_SE(NDO_DATA_CONTACTGROUPALIAS, temp_contactgroup->alias),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_CONTACTGROUPDEFINITION,
					contactgroup_definition, ARRAY_SIZE(contactgroup_definition), FALSE);
		}

		/* Members of each contactgroup. */
		ndomod_contacts_serialize(temp_contactgroup->members, &dbuf, 
				NDO_DATA_CONTACTGROUPMEMBER);

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Contactgroup config. */


	/* Host config. */
	for (temp_host = host_list; temp_host; temp_host = temp_host->next) {

#ifdef BUILD_NAGIOS_4X
		const char *check_command = temp_host->check_command;
		const char *failure_prediction_options = "";
		int notify_on_down = flag_isset(temp_host->notification_options, OPT_DOWN);
		int notify_on_unreachable = flag_isset(temp_host->notification_options, OPT_UNREACHABLE);
		int notify_on_recovery = flag_isset(temp_host->notification_options, OPT_RECOVERY);
		int notify_on_flapping = flag_isset(temp_host->notification_options, OPT_FLAPPING);
		int stalk_on_up = flag_isset(temp_host->stalking_options, OPT_UP);
		int stalk_on_down = flag_isset(temp_host->stalking_options, OPT_DOWN);
		int stalk_on_unreachable = flag_isset(temp_host->stalking_options, OPT_UNREACHABLE);
		int accept_passive_checks = temp_host->accept_passive_checks;
		int failure_prediction_enabled = 0;
		int obsess = temp_host->obsess;
#else
		const char *check_command = temp_host->host_check_command;
		const char *failure_prediction_options = temp_host->failure_prediction_options;
		int notify_on_down = temp_host->notify_on_down;
		int notify_on_unreachable = temp_host->notify_on_unreachable;
		int notify_on_recovery = temp_host->notify_on_recovery;
		int notify_on_flapping = temp_host->notify_on_flapping;
		int stalk_on_up = temp_host->stalk_on_up;
		int stalk_on_down = temp_host->stalk_on_down;
		int stalk_on_unreachable = temp_host->stalk_on_unreachable;
		int accept_passive_checks = temp_host->accept_passive_host_checks;
		int failure_prediction_enabled = temp_host->failure_prediction_enabled;
		int obsess = temp_host->obsess_over_host;
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		const char *display_name = temp_host->display_name;

		const char *notes = temp_host->notes;
		const char *notes_url = temp_host->notes_url;
		const char *action_url = temp_host->action_url;
		const char *icon_image = temp_host->icon_image;
		const char *icon_image_alt = temp_host->icon_image_alt;
		const char *vrml_image = temp_host->vrml_image;
		const char *statusmap_image = temp_host->statusmap_image;

		int have_2d_coords = temp_host->have_2d_coords;
		double x_2d = temp_host->x_2d;
		double y_2d = temp_host->y_2d;
		int have_3d_coords = temp_host->have_3d_coords;
		double x_3d = temp_host->x_3d;
		double y_3d = temp_host->y_3d;
		double z_3d = temp_host->z_3d;

		double first_notification_delay = temp_host->first_notification_delay;
		double retry_interval = temp_host->retry_interval;
#ifdef BUILD_NAGIOS_4X
		int notify_on_host_downtime = flag_isset(temp_host->notification_options, OPT_DOWNTIME);
		int flap_detection_on_up = flag_isset(temp_host->flap_detection_options, OPT_UP);
		int flap_detection_on_down = flag_isset(temp_host->flap_detection_options, OPT_DOWN);
		int flap_detection_on_unreachable = flag_isset(temp_host->flap_detection_options, OPT_UNREACHABLE);
#else
		int notify_on_host_downtime = temp_host->notify_on_downtime;
		int flap_detection_on_up = temp_host->flap_detection_on_up;
		int flap_detection_on_down = temp_host->flap_detection_on_down;
		int flap_detection_on_unreachable = temp_host->flap_detection_on_unreachable;
#endif
#endif

#ifdef BUILD_NAGIOS_2X
		const char *display_name = temp_host->name;

		double first_notification_delay = 0.0;
		double retry_interval = 0.0;
		int notify_on_host_downtime = 0;
		int flap_detection_on_up = 1;
		int flap_detection_on_down = 1;
		int flap_detection_on_unreachable = 1;

		const char *notes = NULL;
		const char *notes_url = NULL;
		const char *action_url = NULL;
		const char *icon_image = NULL;
		const char *icon_image_alt = NULL;
		const char *vrml_image = NULL;
		const char *statusmap_image = NULL;

		int have_2d_coords = NDO_FALSE;
		double x_2d = 0.0;
		double y_2d = 0.0;
		int have_3d_coords = NDO_FALSE;
		double x_3d = 0.0;
		double y_3d = 0.0;
		double z_3d = 0.0;

		hostextinfo *temp_hostextinfo = find_hostextinfo(temp_host->name);
		if (temp_hostextinfo) {
			notes = temp_hostextinfo->notes;
			notes_url = temp_hostextinfo->notes_url;
			action_url = temp_hostextinfo->action_url;
			icon_image = temp_hostextinfo->icon_image;
			icon_image_alt = temp_hostextinfo->icon_image_alt;
			vrml_image = temp_hostextinfo->vrml_image;
			statusmap_image = temp_hostextinfo->statusmap_image;

			have_2d_coords = temp_hostextinfo->have_2d_coords;
			x_2d = temp_hostextinfo->x_2d;
			y_2d = temp_hostextinfo->y_2d;
			have_3d_coords = temp_hostextinfo->have_3d_coords;
			x_3d = temp_hostextinfo->x_3d;
			y_3d = temp_hostextinfo->y_3d;
			z_3d = temp_hostextinfo->z_3d;
		}
#endif

		{
			struct ndo_broker_data host_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_host->name),
				INIT_BD_SE(NDO_DATA_DISPLAYNAME, display_name),
				INIT_BD_SE(NDO_DATA_HOSTALIAS, temp_host->alias),
				INIT_BD_SE(NDO_DATA_HOSTADDRESS, temp_host->address),
				INIT_BD_SE(NDO_DATA_HOSTCHECKCOMMAND, check_command),
				INIT_BD_SE(NDO_DATA_HOSTEVENTHANDLER, temp_host->event_handler),
				INIT_BD_SE(NDO_DATA_HOSTNOTIFICATIONPERIOD, temp_host->notification_period),
				INIT_BD_SE(NDO_DATA_HOSTCHECKPERIOD, temp_host->check_period),
				INIT_BD_SE(NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS, failure_prediction_options),
				INIT_BD_F(NDO_DATA_HOSTCHECKINTERVAL, (double)temp_host->check_interval),
				INIT_BD_F(NDO_DATA_HOSTRETRYINTERVAL, (double)retry_interval),
				INIT_BD_I(NDO_DATA_HOSTMAXCHECKATTEMPTS, temp_host->max_attempts),
				INIT_BD_F(NDO_DATA_FIRSTNOTIFICATIONDELAY, first_notification_delay),
				INIT_BD_F(NDO_DATA_HOSTNOTIFICATIONINTERVAL, (double)temp_host->notification_interval),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTDOWN, notify_on_down),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTUNREACHABLE, notify_on_unreachable),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTRECOVERY, notify_on_recovery),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTFLAPPING, notify_on_flapping),
				INIT_BD_I(NDO_DATA_NOTIFYHOSTDOWNTIME, notify_on_host_downtime),
				INIT_BD_I(NDO_DATA_HOSTFLAPDETECTIONENABLED, temp_host->flap_detection_enabled),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONUP, flap_detection_on_up),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONDOWN, flap_detection_on_down),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONUNREACHABLE, flap_detection_on_unreachable),
				INIT_BD_F(NDO_DATA_LOWHOSTFLAPTHRESHOLD, temp_host->low_flap_threshold),
				INIT_BD_F(NDO_DATA_HIGHHOSTFLAPTHRESHOLD, temp_host->high_flap_threshold),
				INIT_BD_I(NDO_DATA_STALKHOSTONUP, stalk_on_up),
				INIT_BD_I(NDO_DATA_STALKHOSTONDOWN, stalk_on_down),
				INIT_BD_I(NDO_DATA_STALKHOSTONUNREACHABLE, stalk_on_unreachable),
				INIT_BD_I(NDO_DATA_HOSTFRESHNESSCHECKSENABLED, temp_host->check_freshness),
				INIT_BD_I(NDO_DATA_HOSTFRESHNESSTHRESHOLD, temp_host->freshness_threshold),
				INIT_BD_I(NDO_DATA_PROCESSHOSTPERFORMANCEDATA, temp_host->process_performance_data),
				INIT_BD_I(NDO_DATA_ACTIVEHOSTCHECKSENABLED, temp_host->checks_enabled),
				INIT_BD_I(NDO_DATA_PASSIVEHOSTCHECKSENABLED, accept_passive_checks),
				INIT_BD_I(NDO_DATA_HOSTEVENTHANDLERENABLED, temp_host->event_handler_enabled),
				INIT_BD_I(NDO_DATA_RETAINHOSTSTATUSINFORMATION, temp_host->retain_status_information),
				INIT_BD_I(NDO_DATA_RETAINHOSTNONSTATUSINFORMATION, temp_host->retain_nonstatus_information),
				INIT_BD_I(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_host->notifications_enabled),
				INIT_BD_I(NDO_DATA_HOSTFAILUREPREDICTIONENABLED, failure_prediction_enabled),
				INIT_BD_I(NDO_DATA_OBSESSOVERHOST, obsess),
				INIT_BD_SE(NDO_DATA_NOTES, notes),
				INIT_BD_SE(NDO_DATA_NOTESURL, notes_url),
				INIT_BD_SE(NDO_DATA_ACTIONURL, action_url),
				INIT_BD_SE(NDO_DATA_ICONIMAGE, icon_image),
				INIT_BD_SE(NDO_DATA_ICONIMAGEALT, icon_image_alt),
				INIT_BD_SE(NDO_DATA_VRMLIMAGE, vrml_image),
				INIT_BD_SE(NDO_DATA_STATUSMAPIMAGE, statusmap_image),
				INIT_BD_I(NDO_DATA_HAVE2DCOORDS, have_2d_coords),
				INIT_BD_I(NDO_DATA_X2D, x_2d),
				INIT_BD_I(NDO_DATA_Y2D, y_2d),
				INIT_BD_I(NDO_DATA_HAVE3DCOORDS, have_3d_coords),
				INIT_BD_F(NDO_DATA_X3D, x_3d),
				INIT_BD_F(NDO_DATA_Y3D, y_3d),
				INIT_BD_F(NDO_DATA_Z3D, z_3d),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_IMPORTANCE, temp_host->hourly_value),
#endif
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTDEFINITION, 
					host_definition, ARRAY_SIZE(host_definition), FALSE);
		}

		/* Parent hosts. */
		ndomod_hosts_serialize(temp_host->parent_hosts, &dbuf, NDO_DATA_PARENTHOST);

		/* Contactgroups. */
		ndomod_contactgroups_serialize(temp_host->contact_groups, &dbuf);

		/* Individual contacts (not supported in Nagios 2.x). */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_host->contacts, &dbuf, NDO_DATA_CONTACT);
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* Custom variables. */
		ndomod_customvars_serialize(temp_host->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Host config. */


	/* Hostgroup config. */
	for (temp_hostgroup = hostgroup_list; temp_hostgroup; temp_hostgroup = temp_hostgroup->next) {
		{
			struct ndo_broker_data hostgroup_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTGROUPNAME, temp_hostgroup->group_name),
				INIT_BD_SE(NDO_DATA_HOSTGROUPALIAS, temp_hostgroup->alias),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTGROUPDEFINITION, 
					hostgroup_definition, ARRAY_SIZE(hostgroup_definition), FALSE);
		}

		/* Members of each hostgroup. */
#ifdef BUILD_NAGIOS_2X
		ndomod_hosts_serialize_2x(temp_hostgroup->members, &dbuf, 
				NDO_DATA_HOSTGROUPMEMBER);
#else
		ndomod_hosts_serialize(temp_hostgroup->members, &dbuf, 
				NDO_DATA_HOSTGROUPMEMBER);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Hostgroup config. */


	/* Service config. */
	for (temp_service = service_list; temp_service; temp_service = temp_service->next) {

#ifdef BUILD_NAGIOS_4X
		const char *check_command = temp_service->check_command;
		const char *failure_prediction_options = "";
		int notify_on_unknown = flag_isset(temp_service->notification_options, OPT_UNKNOWN);
		int notify_on_warning = flag_isset(temp_service->notification_options, OPT_WARNING);
		int notify_on_critical = flag_isset(temp_service->notification_options, OPT_CRITICAL);
		int notify_on_recovery = flag_isset(temp_service->notification_options, OPT_RECOVERY);
		int notify_on_flapping = flag_isset(temp_service->notification_options, OPT_FLAPPING);
		int stalk_on_ok = flag_isset(temp_service->stalking_options, OPT_OK);
		int stalk_on_warning = flag_isset(temp_service->stalking_options, OPT_WARNING);
		int stalk_on_unknown = flag_isset(temp_service->stalking_options, OPT_UNKNOWN);
		int stalk_on_critical = flag_isset(temp_service->stalking_options, OPT_CRITICAL);
		int accept_passive_checks = temp_service->accept_passive_checks;
		int obsess = temp_service->obsess;
		int failure_prediction_enabled = 0;
#else
		const char *check_command = temp_service->service_check_command;
		const char *failure_prediction_options = temp_service->failure_prediction_options;
		int notify_on_unknown = temp_service->notify_on_unknown;
		int notify_on_warning = temp_service->notify_on_warning;
		int notify_on_critical = temp_service->notify_on_critical;
		int notify_on_recovery = temp_service->notify_on_recovery;
		int notify_on_flapping = temp_service->notify_on_flapping;
		int stalk_on_ok = temp_service->stalk_on_ok;
		int stalk_on_warning = temp_service->stalk_on_warning;
		int stalk_on_unknown = temp_service->stalk_on_unknown;
		int stalk_on_critical = temp_service->stalk_on_critical;
		int accept_passive_checks = temp_service->accept_passive_service_checks;
		int obsess = temp_service->obsess_over_service;
		int failure_prediction_enabled = temp_service->failure_prediction_enabled;
#endif

#if (defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		const char *display_name = temp_service->display_name;

		const char *notes = temp_service->notes;
		const char *notes_url = temp_service->notes_url;
		const char *action_url = temp_service->action_url;
		const char *icon_image = temp_service->icon_image;
		const char *icon_image_alt = temp_service->icon_image_alt;

		double first_notification_delay = temp_service->first_notification_delay;
#ifdef BUILD_NAGIOS_4X
		int notify_on_service_downtime = flag_isset(temp_service->notification_options, OPT_DOWNTIME);
		int flap_detection_on_ok = flag_isset(temp_service->flap_detection_options, OPT_OK);
		int flap_detection_on_warning = flag_isset(temp_service->flap_detection_options, OPT_WARNING);
		int flap_detection_on_unknown = flag_isset(temp_service->flap_detection_options, OPT_UNKNOWN);
		int flap_detection_on_critical = flag_isset(temp_service->flap_detection_options, OPT_CRITICAL);
#else
		int notify_on_service_downtime = temp_service->notify_on_downtime;
		int flap_detection_on_ok = temp_service->flap_detection_on_ok;
		int flap_detection_on_warning = temp_service->flap_detection_on_warning;
		int flap_detection_on_unknown = temp_service->flap_detection_on_unknown;
		int flap_detection_on_critical = temp_service->flap_detection_on_critical;
#endif
#endif

#ifdef BUILD_NAGIOS_2X
		const char *display_name = temp_service->description;

		double first_notification_delay = 0.0;
		int notify_on_service_downtime = 0;
		int flap_detection_on_ok = 1;
		int flap_detection_on_warning = 1;
		int flap_detection_on_unknown = 1;
		int flap_detection_on_critical = 1;

		const char *notes = NULL;
		const char *notes_url = NULL;
		const char *action_url = NULL;
		const char *icon_image = NULL;
		const char *icon_image_alt = NULL;

		serviceextinfo *temp_serviceextinfo = find_serviceextinfo(
				temp_service->host_name, temp_service->description);
		if (temp_serviceextinfo) {
			notes = temp_serviceextinfo->notes;
			notes_url = temp_serviceextinfo->notes_url;
			action_url = temp_serviceextinfo->action_url;
			icon_image = temp_serviceextinfo->icon_image;
			icon_image_alt = temp_serviceextinfo->icon_image_alt;
		}
#endif

		{
			struct ndo_broker_data service_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_service->host_name),
				INIT_BD_SE(NDO_DATA_DISPLAYNAME, display_name),
				INIT_BD_SE(NDO_DATA_SERVICEDESCRIPTION, temp_service->description),
				INIT_BD_SE(NDO_DATA_SERVICECHECKCOMMAND, check_command),
				INIT_BD_SE(NDO_DATA_SERVICEEVENTHANDLER, temp_service->event_handler),
				INIT_BD_SE(NDO_DATA_SERVICENOTIFICATIONPERIOD, temp_service->notification_period),
				INIT_BD_SE(NDO_DATA_SERVICECHECKPERIOD, temp_service->check_period),
				INIT_BD_SE(NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS, failure_prediction_options),
				INIT_BD_F(NDO_DATA_SERVICECHECKINTERVAL, (double)temp_service->check_interval),
				INIT_BD_F(NDO_DATA_SERVICERETRYINTERVAL, (double)temp_service->retry_interval),
				INIT_BD_I(NDO_DATA_MAXSERVICECHECKATTEMPTS, temp_service->max_attempts),
				INIT_BD_F(NDO_DATA_FIRSTNOTIFICATIONDELAY, first_notification_delay),
				INIT_BD_F(NDO_DATA_SERVICENOTIFICATIONINTERVAL, (double)temp_service->notification_interval),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEUNKNOWN, notify_on_unknown),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEWARNING, notify_on_warning),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICECRITICAL, notify_on_critical),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICERECOVERY, notify_on_recovery),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEFLAPPING, notify_on_flapping),
				INIT_BD_I(NDO_DATA_NOTIFYSERVICEDOWNTIME, notify_on_service_downtime),
				INIT_BD_I(NDO_DATA_STALKSERVICEONOK, stalk_on_ok),
				INIT_BD_I(NDO_DATA_STALKSERVICEONWARNING, stalk_on_warning),
				INIT_BD_I(NDO_DATA_STALKSERVICEONUNKNOWN, stalk_on_unknown),
				INIT_BD_I(NDO_DATA_STALKSERVICEONCRITICAL, stalk_on_critical),
				INIT_BD_I(NDO_DATA_SERVICEISVOLATILE, temp_service->is_volatile),
				INIT_BD_I(NDO_DATA_SERVICEFLAPDETECTIONENABLED, temp_service->flap_detection_enabled),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONOK, flap_detection_on_ok),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONWARNING, flap_detection_on_warning),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONUNKNOWN, flap_detection_on_unknown),
				INIT_BD_I(NDO_DATA_FLAPDETECTIONONCRITICAL, flap_detection_on_critical),
				INIT_BD_F(NDO_DATA_LOWSERVICEFLAPTHRESHOLD, temp_service->low_flap_threshold),
				INIT_BD_F(NDO_DATA_HIGHSERVICEFLAPTHRESHOLD, temp_service->high_flap_threshold),
				INIT_BD_I(NDO_DATA_PROCESSSERVICEPERFORMANCEDATA, temp_service->process_performance_data),
				INIT_BD_I(NDO_DATA_SERVICEFRESHNESSCHECKSENABLED, temp_service->check_freshness),
				INIT_BD_I(NDO_DATA_SERVICEFRESHNESSTHRESHOLD, temp_service->freshness_threshold),
				INIT_BD_I(NDO_DATA_PASSIVESERVICECHECKSENABLED, accept_passive_checks),
				INIT_BD_I(NDO_DATA_SERVICEEVENTHANDLERENABLED, temp_service->event_handler_enabled),
				INIT_BD_I(NDO_DATA_ACTIVESERVICECHECKSENABLED, temp_service->checks_enabled),
				INIT_BD_I(NDO_DATA_RETAINSERVICESTATUSINFORMATION, temp_service->retain_status_information),
				INIT_BD_I(NDO_DATA_RETAINSERVICENONSTATUSINFORMATION, temp_service->retain_nonstatus_information),
				INIT_BD_I(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_service->notifications_enabled),
				INIT_BD_I(NDO_DATA_OBSESSOVERSERVICE, obsess),
				INIT_BD_I(NDO_DATA_FAILUREPREDICTIONENABLED, failure_prediction_enabled),
				INIT_BD_SE(NDO_DATA_NOTES, notes),
				INIT_BD_SE(NDO_DATA_NOTESURL, notes_url),
				INIT_BD_SE(NDO_DATA_ACTIONURL, action_url),
				INIT_BD_SE(NDO_DATA_ICONIMAGE, icon_image),
				INIT_BD_SE(NDO_DATA_ICONIMAGEALT, icon_image_alt),
#ifdef BUILD_NAGIOS_4X
				INIT_BD_I(NDO_DATA_IMPORTANCE, temp_service->hourly_value),
#endif
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEDEFINITION, 
					service_definition, ARRAY_SIZE(service_definition), FALSE);
		}

#ifdef BUILD_NAGIOS_4X
		/* Parent services of the service. */
		ndomod_services_serialize(temp_service->parents, &dbuf, 
				NDO_DATA_PARENTSERVICE);
#endif

		/* Contactgroups for the service. */
		ndomod_contactgroups_serialize(temp_service->contact_groups, &dbuf);

		/* Individual contacts (not supported in Nagios 2.x). */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_service->contacts, &dbuf, 
				NDO_DATA_CONTACT);
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* Custom variables. */
		ndomod_customvars_serialize(temp_service->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Service config. */


	/* Servicegroup config. */
	for (temp_servicegroup = servicegroup_list; temp_servicegroup; temp_servicegroup = temp_servicegroup->next) {
		{
			struct ndo_broker_data servicegroup_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_SERVICEGROUPNAME, temp_servicegroup->group_name),
				INIT_BD_SE(NDO_DATA_SERVICEGROUPALIAS, temp_servicegroup->alias),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEGROUPDEFINITION, 
					servicegroup_definition, ARRAY_SIZE(servicegroup_definition), FALSE);
		}

		/* Members of each servicegroup */
		ndomod_services_serialize(temp_servicegroup->members, &dbuf, 
				NDO_DATA_SERVICEGROUPMEMBER);

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Servicegroup config. */


	/* Host escalation config. */
#ifdef BUILD_NAGIOS_4X
	for (x = 0; x < (int)num_objects.hostescalations; x++) {
		temp_hostescalation = hostescalation_ary[x];
#else
	for (temp_hostescalation = hostescalation_list; temp_hostescalation; temp_hostescalation = temp_hostescalation->next) {
#endif
		{
#ifdef BUILD_NAGIOS_4X
			int escalate_on_recovery = flag_isset(temp_hostescalation->escalation_options, OPT_RECOVERY);
			int escalate_on_down = flag_isset(temp_hostescalation->escalation_options, OPT_DOWN);
			int escalate_on_unreachable = flag_isset(temp_hostescalation->escalation_options, OPT_UNREACHABLE);
#else
			int escalate_on_recovery = temp_hostescalation->escalate_on_recovery;
			int escalate_on_down = temp_hostescalation->escalate_on_down;
			int escalate_on_unreachable = temp_hostescalation->escalate_on_unreachable;
#endif
			struct ndo_broker_data hostescalation_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_hostescalation->host_name),
				INIT_BD_SE(NDO_DATA_ESCALATIONPERIOD, temp_hostescalation->escalation_period),
				INIT_BD_I(NDO_DATA_FIRSTNOTIFICATION, temp_hostescalation->first_notification),
				INIT_BD_I(NDO_DATA_LASTNOTIFICATION, temp_hostescalation->last_notification),
				INIT_BD_F(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_hostescalation->notification_interval),
				INIT_BD_I(NDO_DATA_ESCALATEONRECOVERY, escalate_on_recovery),
				INIT_BD_I(NDO_DATA_ESCALATEONDOWN, escalate_on_down),
				INIT_BD_I(NDO_DATA_ESCALATEONUNREACHABLE, escalate_on_unreachable),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTESCALATIONDEFINITION,
					hostescalation_definition, ARRAY_SIZE(hostescalation_definition), FALSE);
		}

		/* Contactgroups for the escalation. */
		ndomod_contactgroups_serialize(temp_hostescalation->contact_groups, &dbuf);

		/* Individual contacts (not supported in Nagios 2.x). */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_hostescalation->contacts, &dbuf, NDO_DATA_CONTACT);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Host escalation config. */


	/* Service escalation config. */
#ifdef BUILD_NAGIOS_4X
	for (x = 0; x < (int)num_objects.serviceescalations; x++) {
		temp_serviceescalation = serviceescalation_ary[x];
#else
	for (temp_serviceescalation = serviceescalation_list; temp_serviceescalation; temp_serviceescalation = temp_serviceescalation->next) {
#endif
		{
#ifdef BUILD_NAGIOS_4X
			int escalate_on_recovery = flag_isset(temp_serviceescalation->escalation_options, OPT_RECOVERY);
			int escalate_on_warning = flag_isset(temp_serviceescalation->escalation_options, OPT_WARNING);
			int escalate_on_unknown = flag_isset(temp_serviceescalation->escalation_options, OPT_UNKNOWN);
			int escalate_on_critical = flag_isset(temp_serviceescalation->escalation_options, OPT_CRITICAL);
#else
			int escalate_on_recovery = temp_serviceescalation->escalate_on_recovery;
			int escalate_on_warning = temp_serviceescalation->escalate_on_warning;
			int escalate_on_unknown = temp_serviceescalation->escalate_on_unknown;
			int escalate_on_critical = temp_serviceescalation->escalate_on_critical;
#endif
			struct ndo_broker_data serviceescalation_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_serviceescalation->host_name),
				INIT_BD_SE(NDO_DATA_SERVICEDESCRIPTION, temp_serviceescalation->description),
				INIT_BD_SE(NDO_DATA_ESCALATIONPERIOD, temp_serviceescalation->escalation_period),
				INIT_BD_I(NDO_DATA_FIRSTNOTIFICATION, temp_serviceescalation->first_notification),
				INIT_BD_I(NDO_DATA_LASTNOTIFICATION, temp_serviceescalation->last_notification),
				INIT_BD_F(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_serviceescalation->notification_interval),
				INIT_BD_I(NDO_DATA_ESCALATEONRECOVERY, escalate_on_recovery),
				INIT_BD_I(NDO_DATA_ESCALATEONWARNING, escalate_on_warning),
				INIT_BD_I(NDO_DATA_ESCALATEONUNKNOWN, escalate_on_unknown),
				INIT_BD_I(NDO_DATA_ESCALATEONCRITICAL, escalate_on_critical),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEESCALATIONDEFINITION,
					serviceescalation_definition, ARRAY_SIZE(serviceescalation_definition), FALSE);
		}

		/* Contactgroups for the escalation. */
		ndomod_contactgroups_serialize(temp_serviceescalation->contact_groups, &dbuf);

		/* Individual contacts (not supported in Nagios 2.x). */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_serviceescalation->contacts, &dbuf, NDO_DATA_CONTACT);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Service escalation config. */


	/* Host dependency config. */
#ifdef BUILD_NAGIOS_4X
	for (x = 0; x < (int)num_objects.hostdependencies; x++) {
		temp_hostdependency = hostdependency_ary[x];
#else
	for (temp_hostdependency = hostdependency_list; temp_hostdependency; temp_hostdependency = temp_hostdependency->next) {
#endif
		{
#ifdef BUILD_NAGIOS_4X
			int fail_on_up = flag_isset(temp_hostdependency->failure_options, OPT_UP);
			int fail_on_down = flag_isset(temp_hostdependency->failure_options, OPT_DOWN);
			int fail_on_unreachable = flag_isset(temp_hostdependency->failure_options, OPT_UNREACHABLE);
#else
			int fail_on_up = temp_hostdependency->fail_on_up;
			int fail_on_down = temp_hostdependency->fail_on_down;
			int fail_on_unreachable = temp_hostdependency->fail_on_unreachable;
#endif
#if (defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
			const char *dependency_period = temp_hostdependency->dependency_period;
#endif
#ifdef BUILD_NAGIOS_2X
			const char *dependency_period = NULL;
#endif
			struct ndo_broker_data hostdependency_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_hostdependency->host_name),
				INIT_BD_SE(NDO_DATA_DEPENDENTHOSTNAME, temp_hostdependency->dependent_host_name),
				INIT_BD_I(NDO_DATA_DEPENDENCYTYPE, temp_hostdependency->dependency_type),
				INIT_BD_I(NDO_DATA_INHERITSPARENT, temp_hostdependency->inherits_parent),
				INIT_BD_SE(NDO_DATA_DEPENDENCYPERIOD, dependency_period),
				INIT_BD_I(NDO_DATA_FAILONUP, fail_on_up),
				INIT_BD_I(NDO_DATA_FAILONDOWN, fail_on_down),
				INIT_BD_I(NDO_DATA_FAILONUNREACHABLE, fail_on_unreachable),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTDEPENDENCYDEFINITION,
					hostdependency_definition, ARRAY_SIZE(hostdependency_definition), TRUE);
		}

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Host dependency config. */


	/* Service dependency config. */
#ifdef BUILD_NAGIOS_4X
	for (x = 0; x < (int)num_objects.servicedependencies; x++) {
		temp_servicedependency = servicedependency_ary[x];
#else
	for (temp_servicedependency = servicedependency_list; temp_servicedependency; temp_servicedependency = temp_servicedependency->next) {
#endif
		{
#ifdef BUILD_NAGIOS_4X
			int fail_on_ok = flag_isset(temp_servicedependency->failure_options, OPT_OK);
			int fail_on_warning = flag_isset(temp_servicedependency->failure_options, OPT_WARNING);
			int fail_on_unknown = flag_isset(temp_servicedependency->failure_options, OPT_UNKNOWN);
			int fail_on_critical = flag_isset(temp_servicedependency->failure_options, OPT_CRITICAL);
#else
			int fail_on_ok = temp_servicedependency->fail_on_ok;
			int fail_on_warning = temp_servicedependency->fail_on_warning;
			int fail_on_unknown = temp_servicedependency->fail_on_unknown;
			int fail_on_critical = temp_servicedependency->fail_on_critical;
#endif
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
			const char *dependency_period = temp_servicedependency->dependency_period;
#endif
#ifdef BUILD_NAGIOS_2X
			const char *dependency_period = NULL;
#endif
			struct ndo_broker_data servicedependency_definition[] = {
				INIT_BD_TV(NDO_DATA_TIMESTAMP, now),
				INIT_BD_SE(NDO_DATA_HOSTNAME, temp_servicedependency->host_name),
				INIT_BD_SE(NDO_DATA_SERVICEDESCRIPTION, temp_servicedependency->service_description),
				INIT_BD_SE(NDO_DATA_DEPENDENTHOSTNAME, temp_servicedependency->dependent_host_name),
				INIT_BD_SE(NDO_DATA_DEPENDENTSERVICEDESCRIPTION, temp_servicedependency->dependent_service_description),
				INIT_BD_I(NDO_DATA_DEPENDENCYTYPE, temp_servicedependency->dependency_type),
				INIT_BD_I(NDO_DATA_INHERITSPARENT, temp_servicedependency->inherits_parent),
				INIT_BD_SE(NDO_DATA_DEPENDENCYPERIOD, dependency_period),
				INIT_BD_I(NDO_DATA_FAILONOK, fail_on_ok),
				INIT_BD_I(NDO_DATA_FAILONWARNING, fail_on_warning),
				INIT_BD_I(NDO_DATA_FAILONUNKNOWN, fail_on_unknown),
				INIT_BD_I(NDO_DATA_FAILONCRITICAL, fail_on_critical),
			};
			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEDEPENDENCYDEFINITION,
					servicedependency_definition, ARRAY_SIZE(servicedependency_definition), TRUE);
		}

		ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
		ndo_dbuf_reset(&dbuf);

	} /* Service dependency config. */


	/* Release our dynamic buffer's internal memory. */
	ndo_dbuf_free(&dbuf);

	return NDO_OK;
}



/* Dumps config files to the data sink. */
static int ndomod_write_config_files(void) {
	return ndomod_write_main_config_file();
	/* @note: Previously a stub to sink resource config files was called here.
	 * This wont be implemented since resource files should remain private. */
}



/* Dumps main config file data to the sink. */
static int ndomod_write_main_config_file(void) {
	ndo_dbuf dbuf;
	struct timeval now;
	FILE *fp;
	char *var = NULL;
	char *val = NULL;

	/* Get current time. */
	gettimeofday(&now, NULL);

	/* Initialize our dynamic buffer (2KB chunk size). */
	ndo_dbuf_init(&dbuf, 2048);

	/* Start out the config variable dump. */
	ndo_dbuf_printf(&dbuf, "\n%d:\n%d=%ld.%06ld\n%d=%s\n",
			NDO_API_MAINCONFIGFILEVARIABLES,
			NDO_DATA_TIMESTAMP, now.tv_sec, now.tv_usec,
			NDO_DATA_CONFIGFILENAME, config_file
	);
	ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

	/* Write each var/val pair from config file. */
	if ((fp = fopen(config_file, "r"))) {

		char fbuf[NDOMOD_MAX_BUFLEN];
		while ((fgets(fbuf, sizeof(fbuf), fp))) {

			strip(fbuf);
			switch (fbuf[0]) {

				/* Skip blank lines and comments. */
				case '\0': case '\n': case '\r': case '#': case ';':
					continue;

				default:
					if (!(var = strtok(fbuf, "="))) continue;
					val = strtok(NULL, "\n");

					ndo_dbuf_reset(&dbuf);
					ndo_dbuf_printf(&dbuf, "%d=%s=%s\n",
							NDO_DATA_CONFIGFILEVARIABLE, var, val ? val : ""
					);
					ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
					break;
			}
		}

		fclose(fp);
	}

	/* End the config variable dump. */
	ndomod_write_to_sink(STRINGIFY(NDO_API_ENDDATA)"\n\n", NDO_TRUE, NDO_TRUE);

	ndo_dbuf_free(&dbuf);

	return NDO_OK;
}



/* Dumps runtime variables to the sink. */
static int ndomod_write_runtime_variables(void) {
	struct timeval now;
	ndo_dbuf dbuf;

	/* Get current time. */
	gettimeofday(&now, NULL);

	/* Initialize our dynamic buffer (2KB chunk size). */
	ndo_dbuf_init(&dbuf, 2048);

	/* Start out the variable dump. */
	ndo_dbuf_printf(&dbuf, "\n%d:\n%d=%ld.%06ld\n",
			NDO_API_RUNTIMEVARIABLES,
			NDO_DATA_TIMESTAMP, now.tv_sec, now.tv_usec
	);
	/* Add our main config file name. */
	ndo_dbuf_printf(&dbuf, "%d=config_file=%s\n",
			NDO_DATA_RUNTIMEVARIABLE, config_file
	);
	/* Sink the data and reset to an empty buffer. */
	ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
	ndo_dbuf_reset(&dbuf);

	/* A macro to print scheduling_info variables to our dbuf. */
	#define NDO_RUNVAR_SCHED_PRINT(f, v) \
		ndo_dbuf_printf(&dbuf, "%d=%s=%" #f "\n", \
				NDO_DATA_RUNTIMEVARIABLE, #v, scheduling_info.v)

	/* Add our values determined after startup. */
	NDO_RUNVAR_SCHED_PRINT(d, total_services);
	NDO_RUNVAR_SCHED_PRINT(d, total_scheduled_services);
	NDO_RUNVAR_SCHED_PRINT(d, total_hosts);
	NDO_RUNVAR_SCHED_PRINT(d, total_scheduled_hosts);
	NDO_RUNVAR_SCHED_PRINT(lf, average_services_per_host);
	NDO_RUNVAR_SCHED_PRINT(lf, average_scheduled_services_per_host);
	NDO_RUNVAR_SCHED_PRINT(lu, service_check_interval_total);
	NDO_RUNVAR_SCHED_PRINT(lu, host_check_interval_total);
	NDO_RUNVAR_SCHED_PRINT(lf, average_service_check_interval);
	NDO_RUNVAR_SCHED_PRINT(lf, average_host_check_interval);
	NDO_RUNVAR_SCHED_PRINT(lf, average_service_inter_check_delay);
	NDO_RUNVAR_SCHED_PRINT(lf, average_host_inter_check_delay);
	NDO_RUNVAR_SCHED_PRINT(lf, service_inter_check_delay);
	NDO_RUNVAR_SCHED_PRINT(lf, host_inter_check_delay);
	NDO_RUNVAR_SCHED_PRINT(d, service_interleave_factor);
	NDO_RUNVAR_SCHED_PRINT(d, max_service_check_spread);
	NDO_RUNVAR_SCHED_PRINT(d, max_host_check_spread);

	/* We don't need this anymore. */
	#undef NDO_RUNVAR_SCHED_PRINT

	/* End the variable dump and sink the data. */
	ndo_dbuf_strcat(&dbuf, STRINGIFY(NDO_API_ENDDATA)"\n\n");
	ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

	ndo_dbuf_free(&dbuf);

	return NDO_OK;
}
