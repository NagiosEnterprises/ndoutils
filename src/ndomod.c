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

/* include our project's header files */
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndomod.h"

/* include (minimum required) event broker header files */
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

/* include other Nagios header files for access to functions, data structs, etc. */
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

/* specify event broker API version (required) */
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

/* Phases that a ndomod_broker_*_data() function may process */
typedef enum bd_phase {
	bdp_preprocessing,		/* Pre-processing is the phase executed before
								data serialization takes place. Usually this
								is just to determine whether further
								processing should take place. */
	bdp_mainprocessing,		/* Main processing is the phase where data
								serialization is executed */
	bdp_postprocessing		/* Post-processing happens after data is
								serialized */
} bd_phase;

/* Possible return codes from the ndomod_broker_*_data() functions */
typedef enum bd_result {
	bdr_ok,			/* No error indicated, continue processing */
	bdr_stop,		/* No error indicated, but stop processing */
	bdr_ephase,		/* Bad phase passed */
	bdr_enoent		/* Entity (host, service, etc.) not found */
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


/* handles brokered event data */
void *ndomod_module_handle=NULL;
char *ndomod_instance_name=NULL;
char *ndomod_buffer_file=NULL;
char *ndomod_sink_name=NULL;
int ndomod_sink_type=NDO_SINK_UNIXSOCKET;
int ndomod_sink_tcp_port=NDO_DEFAULT_TCP_PORT;
int ndomod_sink_is_open=NDO_FALSE;
int ndomod_sink_previously_open=NDO_FALSE;
int ndomod_sink_fd=-1;
time_t ndomod_sink_last_reconnect_attempt=0L;
time_t ndomod_sink_last_reconnect_warning=0L;
unsigned long ndomod_sink_connect_attempt=0L;
unsigned long ndomod_sink_reconnect_interval=15;
unsigned long ndomod_sink_reconnect_warning_interval=900;
unsigned long ndomod_sink_rotation_interval=3600;
char *ndomod_sink_rotation_command=NULL;
int ndomod_sink_rotation_timeout=60;
int ndomod_allow_sink_activity=NDO_TRUE;
unsigned long ndomod_process_options=0;
int ndomod_config_output_options=NDOMOD_CONFIG_DUMP_ALL;
unsigned long ndomod_sink_buffer_slots=5000;
ndomod_sink_buffer sinkbuf;


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
#ifdef BUILD_NAGIOS_2X
extern hostextinfo *hostextinfo_list;
extern serviceextinfo *serviceextinfo_list;
#endif

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __nagios_object_structure_version;

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
int ndomod_check_nagios_object_version(void) {

	if (__nagios_object_structure_version == CURRENT_OBJECT_STRUCTURE_VERSION) {
		return NDO_OK;
	} else {
		ndomod_printf_to_logs(
				"ndomod: I've been compiled with support for revision %d of the internal Nagios object structures, but the Nagios daemon is currently using revision %d. I'm going to unload so I don't cause any problems...",
				CURRENT_OBJECT_STRUCTURE_VERSION, __nagios_object_structure_version);
		return NDO_ERROR;
	}
}


/* performs some initialization stuff */
int ndomod_init(void){

	/* initialize some vars (needed for restarts of daemon - why, if the module gets reloaded ???) */
	ndomod_sink_is_open=NDO_FALSE;
	ndomod_sink_previously_open=NDO_FALSE;
	ndomod_sink_fd=-1;
	ndomod_sink_last_reconnect_attempt=0L;
	ndomod_sink_last_reconnect_warning=0L;
	ndomod_allow_sink_activity=NDO_TRUE;

	/* initialize data sink buffer */
	ndomod_sink_buffer_init(&sinkbuf,ndomod_sink_buffer_slots);

	/* read unprocessed data from buffer file */
	ndomod_load_unprocessed_data(ndomod_buffer_file);

	/* open data sink and say hello */
	/* 05/04/06 - modified to flush buffer items that may have been read in from file */
	ndomod_write_to_sink("\n",NDO_FALSE,NDO_TRUE);

	/* register callbacks */
	if(ndomod_register_callbacks()==NDO_ERROR)
		return NDO_ERROR;

#if defined(BUILD_NAGIOS_2X) || defined(BUILD_NAGIOS_3X)
	/* See comment in ndomod_broker_process_data() about the Nagios Core 4
		implementation */
	if(ndomod_sink_type==NDO_SINK_FILE){

		/* make sure we have a rotation command defined... */
		if(ndomod_sink_rotation_command==NULL){

			/* log an error message to the Nagios log file */
			ndomod_printf_to_logs("ndomod: Warning - No file rotation command defined.");
		        }

		/* schedule a file rotation event */
		else{
			time_t current_time = time(NULL);
#ifdef BUILD_NAGIOS_2X
			schedule_new_event(EVENT_USER_FUNCTION,TRUE,current_time+ndomod_sink_rotation_interval,TRUE,ndomod_sink_rotation_interval,NULL,TRUE,(void *)ndomod_rotate_sink_file,NULL);
#else
			schedule_new_event(EVENT_USER_FUNCTION,TRUE,current_time+ndomod_sink_rotation_interval,TRUE,ndomod_sink_rotation_interval,NULL,TRUE,(void *)ndomod_rotate_sink_file,NULL,0);
#endif
		        }

	        }
#endif

	return NDO_OK;
        }


/* Shutdown and release our resources when the module is unloaded. */
int ndomod_deinit(void) {
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
static int ndomod_process_module_args(char *args) {
	char *ptr=NULL;
	char **arglist=NULL;
	char **newarglist=NULL;
	int argcount=0;
	int memblocks=64;
	int arg=0;

	if(args==NULL)
		return NDO_OK;


	/* get all the var/val argument pairs */

	/* allocate some memory */
        if((arglist=(char **)malloc(memblocks*sizeof(char **)))==NULL)
                return NDO_ERROR;

	/* process all args */
        ptr=strtok(args,",");
        while(ptr){

		/* save the argument */
                arglist[argcount++]=strdup(ptr);

		/* allocate more memory if needed */
                if(!(argcount%memblocks)){
                        if((newarglist=(char **)realloc(arglist,(argcount+memblocks)*sizeof(char **)))==NULL){
				for(arg=0;arg<argcount;arg++)
					free(arglist[argcount]);
				free(arglist);
				return NDO_ERROR;
			        }
			else
				arglist=newarglist;
                        }

                ptr=strtok(NULL,",");
                }

	/* terminate the arg list */
        arglist[argcount] = NULL;


	/* process each argument */
	for(arg=0;arg<argcount;arg++){
		if(ndomod_process_config_var(arglist[arg])==NDO_ERROR){
			for(arg=0;arg<argcount;arg++)
				free(arglist[arg]);
			free(arglist);
			return NDO_ERROR;
		        }
	        }

	/* free allocated memory */
	for(arg=0;arg<argcount;arg++)
		free(arglist[arg]);
	free(arglist);

	return NDO_OK;
        }


/* process all config vars in a file */
static int ndomod_process_config_file(const char *filename) {
	ndo_mmapfile *thefile=NULL;
	char *buf=NULL;
	int result=NDO_OK;

	/* open the file */
	if((thefile=ndo_mmap_fopen(filename))==NULL)
		return NDO_ERROR;

	/* process each line of the file */
	while((buf=ndo_mmap_fgets(thefile))){

		/* skip comments */
		if(buf[0]=='#'){
			free(buf);
			continue;
		        }

		/* skip blank lines */
		if(!strcmp(buf,"")){
			free(buf);
			continue;
		        }

		/* process the variable */
		result=ndomod_process_config_var(buf);

		/* free memory */
		free(buf);

		if(result!=NDO_OK)
			break;
	        }

	/* close the file */
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

/* Writes a string to Nagios Core logs. */
int ndomod_write_to_logs(char *buf, int flags) {
	return buf ? write_to_all_logs(buf, flags) : NDO_ERROR;
}

int ndomod_printf_to_logs(const char *fmt, ...) {
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

/* (re)open data sink */
int ndomod_open_sink(void){
	int flags=0;

	/* sink is already open... */
	if(ndomod_sink_is_open==NDO_TRUE)
		return ndomod_sink_fd;

	/* try and open sink */
	if(ndomod_sink_type==NDO_SINK_FILE)
		flags=O_WRONLY|O_CREAT|O_APPEND;
	if(ndo_sink_open(ndomod_sink_name,0,ndomod_sink_type,ndomod_sink_tcp_port,flags,&ndomod_sink_fd)==NDO_ERROR)
		return NDO_ERROR;

	/* mark the sink as being open */
	ndomod_sink_is_open=NDO_TRUE;

	/* mark the sink as having once been open */
	ndomod_sink_previously_open=NDO_TRUE;

	return NDO_OK;
        }


/* (re)open data sink */
int ndomod_close_sink(void){

	/* sink is already closed... */
	if(ndomod_sink_is_open==NDO_FALSE)
		return NDO_OK;

	/* flush sink */
	ndo_sink_flush(ndomod_sink_fd);

	/* close sink */
	ndo_sink_close(ndomod_sink_fd);

	/* mark the sink as being closed */
	ndomod_sink_is_open=NDO_FALSE;

	return NDO_OK;
        }


/* say hello */
int ndomod_hello_sink(int reconnect, int problem_disconnect){
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
int ndomod_goodbye_sink(void){
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
int ndomod_rotate_sink_file(void *args){
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
int ndomod_write_to_sink(const char *buf, int buffer_write, int flush_buffer){
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
int ndomod_save_unprocessed_data(const char *f){
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
int ndomod_load_unprocessed_data(const char *f){
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
int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf,unsigned long maxitems){
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
int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf){
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
int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf, const char *buf){

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
char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf){
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
char *ndomod_sink_buffer_peek(ndomod_sink_buffer *sbuf){
	char *buf=NULL;

	if(sbuf==NULL)
		return NULL;

	if(sbuf->buffer==NULL)
		return NULL;

	buf=sbuf->buffer[sbuf->tail];

	return buf;
        }


/* returns number of items buffered */
int ndomod_sink_buffer_items(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->items;
        }


/* gets number of items lost due to buffer overflow */
unsigned long ndomod_sink_buffer_get_overflow(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->overflow;
        }


/* sets number of items lost due to buffer overflow */
int ndomod_sink_buffer_set_overflow(ndomod_sink_buffer *sbuf, unsigned long num){

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
int ndomod_register_callbacks(void) {
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
int ndomod_deregister_callbacks(void){

	neb_deregister_callback(NEBCALLBACK_PROCESS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_LOG_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_COMMENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_RETENTION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA,ndomod_broker_data);
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
	neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA,ndomod_broker_data);
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
				ndo_dbuf_printf(dbuf, "\n%d=%ld.%ld", bd->key,
						bd->value.timestamp.tv_sec, bd->value.timestamp.tv_usec);
				break;
			case BD_STRING:
				ndo_dbuf_printf(dbuf, "\n%d=", bd->key);
				ndo_dbuf_strcat(dbuf, bd->value.string);
				break;
			case BD_STRING_ESCAPE:
				ndo_dbuf_printf(dbuf, "\n%d=", bd->key);
				ndo_dbuf_strcat_escaped(dbuf, bd->value.string);
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
		char *name = c->variable_name;
		char *value = c->variable_value;
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
		char *name = g->group_name;
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
		char *name = c->contact_name;
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
		char *name = h->host_name;
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
		char *name = s->host_name;
		char *desc = s->service_description;
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
		char *command = c->command;
		ndo_dbuf_printf(dbuf, "\n%d=", varnum);
		if (command && *command) ndo_dbuf_strcat_escaped(dbuf, command);
	}
}


int ndomod_broker_data(int event_type, void *data) {
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


static bd_result ndomod_broker_process_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_process_data *procdata = (nebstruct_process_data *)data;
#ifdef BUILD_NAGIOS_4X
	time_t current_time;
#endif

	struct ndo_broker_data process_data[] = {
		{ NDO_DATA_TYPE, BD_INT, { .integer = procdata->type }},
		{ NDO_DATA_FLAGS, BD_INT, { .integer = procdata->flags }},
		{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = procdata->attr }},
		{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
				{ .timestamp = procdata->timestamp }},
		{ NDO_DATA_PROGRAMNAME, BD_STRING, { .string = "Nagios" }},
		{ NDO_DATA_PROGRAMVERSION, BD_STRING,
				{ .string = get_program_version() }},
		{ NDO_DATA_PROGRAMDATE, BD_STRING,
				{ .string = get_program_modification_date() }},
		{ NDO_DATA_PROCESSID, BD_UNSIGNED_LONG,
				{ .unsigned_long = (unsigned long)getpid() }}
		};

printf("ndomod_broker_process_data() start\n");
	switch(phase) {
	case bdp_preprocessing:
printf("ndomod_broker_process_data() phase: Pre-Prossing Event Type: %d\n",
		procdata->type);
		switch(procdata->type) {
		case NEBTYPE_PROCESS_START:
#ifdef BUILD_NAGIOS_4X
			/* In Core 4, the file rotation event is schedule upon receipt of
				NEBTYPE_PROCESS_START because the scheduling queue is not
				initialized when ndomod_init() is called, as it was in
				previous versions of Core */
			if(ndomod_sink_type == NDO_SINK_FILE) {
				/* make sure we have a rotation command defined... */
				if(ndomod_sink_rotation_command == NULL) {
					/* log an error message to the Nagios log file */
					ndomod_printf_to_logs("ndomod: Warning - No file rotation command defined.");
					}
				/* schedule a file rotation event */
				else {
					time(&current_time);
					schedule_new_event(EVENT_USER_FUNCTION, TRUE,
							current_time + ndomod_sink_rotation_interval, TRUE,
							ndomod_sink_rotation_interval, NULL, TRUE,
							(void *)ndomod_rotate_sink_file, NULL, 0);
					}
				}
#endif
			break;
			}
		if(!(process_options & NDOMOD_PROCESS_PROCESS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		ndomod_broker_data_serialize(dbufp, NDO_API_PROCESSDATA, process_data,
				sizeof(process_data) / sizeof(process_data[ 0]), TRUE);
		break;
	case bdp_postprocessing:
		/* process has passed pre-launch config verification, so dump
				original config */
		if(procdata->type == NEBTYPE_PROCESS_START) {
			ndomod_write_config_files();
			ndomod_write_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
			}

		/* process is starting the event loop, so dump runtime vars */
		if(procdata->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {
			ndomod_write_runtime_variables();
			}
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_timed_event_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_timed_event_data *eventdata = (nebstruct_timed_event_data *)data;
	service *temp_service = NULL;
	host *temp_host = NULL;
	scheduled_downtime *temp_downtime = NULL;
	char *host_name = NULL;
	char *service_desc = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_TIMED_EVENT_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		switch(eventdata->event_type) {

		case EVENT_SERVICE_CHECK:
			temp_service = (service *)eventdata->event_data;

			host_name = ndo_escape_buffer(temp_service->host_name);
			service_desc = ndo_escape_buffer(temp_service->description);

			{
				struct ndo_broker_data timed_event_data[] = {
					{ NDO_DATA_TYPE, BD_INT, { .integer = eventdata->type }},
					{ NDO_DATA_FLAGS, BD_INT, { .integer = eventdata->flags }},
					{ NDO_DATA_ATTRIBUTES, BD_INT, 
							{ .integer = eventdata->attr }},
					{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
							{ .timestamp = eventdata->timestamp }},
					{ NDO_DATA_EVENTTYPE, BD_INT, 
							{ .integer = eventdata->event_type }},
					{ NDO_DATA_RECURRING, BD_INT, 
							{ .integer = eventdata->recurring }},
					{ NDO_DATA_RUNTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
							(unsigned long)eventdata->run_time }},
					{ NDO_DATA_HOST, BD_STRING, 
							{ .string = (host_name == NULL) ? "" : host_name }},
					{ NDO_DATA_SERVICE, BD_STRING, { .string =
							(service_desc == NULL) ? "" : service_desc }}
					};

				ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
						timed_event_data, 
						sizeof(timed_event_data) / sizeof(timed_event_data[ 0]),
						TRUE);
			}

			break;

		case EVENT_HOST_CHECK:
			temp_host = (host *)eventdata->event_data;

			host_name = ndo_escape_buffer(temp_host->name);

			{
				struct ndo_broker_data timed_event_data[] = {
					{ NDO_DATA_TYPE, BD_INT, { .integer = eventdata->type }},
					{ NDO_DATA_FLAGS, BD_INT, { .integer = eventdata->flags }},
					{ NDO_DATA_ATTRIBUTES, BD_INT, 
							{ .integer = eventdata->attr }},
					{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
							{ .timestamp = eventdata->timestamp }},
					{ NDO_DATA_EVENTTYPE, BD_INT, 
							{ .integer = eventdata->event_type }},
					{ NDO_DATA_RECURRING, BD_INT, 
							{ .integer = eventdata->recurring }},
					{ NDO_DATA_RUNTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
							(unsigned long)eventdata->run_time }},
					{ NDO_DATA_HOST, BD_STRING, 
							{ .string = (host_name == NULL) ? "" : host_name }}
					};

				ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
						timed_event_data, 
						sizeof(timed_event_data) / sizeof(timed_event_data[ 0]),
						TRUE);
			}

			break;

		case EVENT_SCHEDULED_DOWNTIME:
			temp_downtime = find_downtime(ANY_DOWNTIME,
					(unsigned long)eventdata->event_data);

			if(temp_downtime != NULL) {
				host_name = ndo_escape_buffer(temp_downtime->host_name);
				service_desc =
						ndo_escape_buffer(temp_downtime->service_description);
				}

			{
				struct ndo_broker_data timed_event_data[] = {
					{ NDO_DATA_TYPE, BD_INT, { .integer = eventdata->type }},
					{ NDO_DATA_FLAGS, BD_INT, { .integer = eventdata->flags }},
					{ NDO_DATA_ATTRIBUTES, BD_INT, 
							{ .integer = eventdata->attr }},
					{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
							{ .timestamp = eventdata->timestamp }},
					{ NDO_DATA_EVENTTYPE, BD_INT, 
							{ .integer = eventdata->event_type }},
					{ NDO_DATA_RECURRING, BD_INT, 
							{ .integer = eventdata->recurring }},
					{ NDO_DATA_RUNTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
							(unsigned long)eventdata->run_time }},
					{ NDO_DATA_HOST, BD_STRING, 
							{ .string = (host_name == NULL) ? "" : host_name }},
					{ NDO_DATA_SERVICE, BD_STRING, { .string =
							(service_desc == NULL) ? "" : service_desc }}
					};

				ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
						timed_event_data, 
						sizeof(timed_event_data) / sizeof(timed_event_data[ 0]),
						TRUE);
			}

			break;

		default:
			{
				struct ndo_broker_data timed_event_data[] = {
					{ NDO_DATA_TYPE, BD_INT, { .integer = eventdata->type }},
					{ NDO_DATA_FLAGS, BD_INT, { .integer = eventdata->flags }},
					{ NDO_DATA_ATTRIBUTES, BD_INT, 
							{ .integer = eventdata->attr }},
					{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
							{ .timestamp = eventdata->timestamp }},
					{ NDO_DATA_EVENTTYPE, BD_INT, 
							{ .integer = eventdata->event_type }},
					{ NDO_DATA_RECURRING, BD_INT, 
							{ .integer = eventdata->recurring }},
					{ NDO_DATA_RUNTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
							(unsigned long)eventdata->run_time }}
					};

				ndomod_broker_data_serialize(dbufp, NDO_API_TIMEDEVENTDATA,
						timed_event_data, 
						sizeof(timed_event_data) / sizeof(timed_event_data[ 0]),
						TRUE);
			}

			break;
	        }

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_log_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_log_data *logdata = (nebstruct_log_data *)data;

	struct ndo_broker_data log_data[] = {
		{ NDO_DATA_TYPE, BD_INT, { .integer = logdata->type }},
		{ NDO_DATA_FLAGS, BD_INT, { .integer = logdata->flags }},
		{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = logdata->attr }},
		{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, { .timestamp = logdata->timestamp }},
		{ NDO_DATA_LOGENTRYTIME, BD_UNSIGNED_LONG,
				{ .unsigned_long = logdata->entry_time }},
		{ NDO_DATA_LOGENTRYTYPE, BD_INT, { .integer = logdata->data_type }},
		{ NDO_DATA_LOGENTRY, BD_STRING, { .string = logdata->data }}
		};

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_LOG_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		ndomod_broker_data_serialize(dbufp, NDO_API_LOGDATA, log_data,
				sizeof(log_data) / sizeof(log_data[ 0]), TRUE);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_system_command_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_system_command_data *cmddata =
			(nebstruct_system_command_data *)data;
	char *command_line = NULL;
	char *output = NULL;
	char *long_output = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_SYSTEM_COMMAND_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		command_line = ndo_escape_buffer(cmddata->command_line);
		output = ndo_escape_buffer(cmddata->output);
		long_output = ndo_escape_buffer(cmddata->output);

		{
			struct ndo_broker_data system_command_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = cmddata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = cmddata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = cmddata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = cmddata->timestamp }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL,
						{ .timestamp = cmddata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL,
						{ .timestamp = cmddata->end_time }},
				{ NDO_DATA_TIMEOUT, BD_INT, { .integer = cmddata->timeout }},
				{ NDO_DATA_COMMANDLINE, BD_STRING, { .string =
						(command_line == NULL) ? "" : command_line }},
				{ NDO_DATA_EARLYTIMEOUT, BD_INT,
						{ .integer = cmddata->early_timeout }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = cmddata->execution_time }},
				{ NDO_DATA_RETURNCODE, BD_INT,
						{ .integer = cmddata->return_code }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_SYSTEMCOMMANDDATA,
					system_command_data, sizeof(system_command_data) / 
					sizeof(system_command_data[ 0]), TRUE);
		}

		if(NULL != command_line) free(command_line);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_event_handler_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_event_handler_data *ehanddata =
			(nebstruct_event_handler_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *command_name = NULL;
	char *command_args = NULL;
	char *command_line = NULL;
	char *output = NULL;
	char *long_output = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_EVENT_HANDLER_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(ehanddata->host_name);
		service_desc = ndo_escape_buffer(ehanddata->service_description);
		command_name = ndo_escape_buffer(ehanddata->command_name);
		command_args = ndo_escape_buffer(ehanddata->command_args);
		command_line = ndo_escape_buffer(ehanddata->command_line);
		output = ndo_escape_buffer(ehanddata->output);
		/* Preparing if eventhandler will have long_output in the future */
		long_output = ndo_escape_buffer(ehanddata->output);

		{
			struct ndo_broker_data event_handler_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = ehanddata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = ehanddata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = ehanddata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = ehanddata->timestamp }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_STATETYPE, BD_INT, 
						{ .integer = ehanddata->state_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = ehanddata->state }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL, 
						{ .timestamp = ehanddata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL, 
						{ .timestamp = ehanddata->end_time }},
				{ NDO_DATA_TIMEOUT, BD_INT, { .integer = ehanddata->timeout }},
				{ NDO_DATA_COMMANDNAME, BD_STRING, { .string =
						(command_name == NULL) ? "" : command_name }},
				{ NDO_DATA_COMMANDARGS, BD_STRING, { .string =
						(command_args == NULL) ? "" : command_args }},
				{ NDO_DATA_COMMANDLINE, BD_STRING, { .string =
						(command_line == NULL) ? "" : command_line }},
				{ NDO_DATA_EARLYTIMEOUT, BD_INT, 
						{ .integer = ehanddata->early_timeout }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = ehanddata->execution_time }},
				{ NDO_DATA_RETURNCODE, BD_INT, 
						{ .integer = ehanddata->return_code }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_EVENTHANDLERDATA,
					event_handler_data, 
					sizeof(event_handler_data) / sizeof(event_handler_data[ 0]),
					TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != command_name) free(command_name);
		if(NULL != command_args) free(command_args);
		if(NULL != command_line) free(command_line);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_notification_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_notification_data *notdata = (nebstruct_notification_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *output = NULL;
	char *long_output = NULL;
	char *ack_author = NULL;
	char *ack_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(notdata->host_name);
		service_desc = ndo_escape_buffer(notdata->service_description);
		output = ndo_escape_buffer(notdata->output);
		/* Preparing if notifications will have long_output in the future */
		long_output = ndo_escape_buffer(notdata->output);
		ack_author = ndo_escape_buffer(notdata->ack_author);
		ack_data = ndo_escape_buffer(notdata->ack_data);

		{
			struct ndo_broker_data notification_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = notdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = notdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = notdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = notdata->timestamp }},
				{ NDO_DATA_NOTIFICATIONTYPE, BD_INT, 
						{ .integer = notdata->notification_type }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL,
						{ .timestamp = notdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL,
						{ .timestamp = notdata->end_time }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_NOTIFICATIONREASON, BD_INT, 
						{ .integer = notdata->reason_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = notdata->state }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_ACKAUTHOR, BD_STRING, 
						{ .string = (ack_author == NULL) ? "" : ack_author }},
				{ NDO_DATA_ACKDATA, BD_STRING, 
						{ .string = (ack_data == NULL) ? "" : ack_data }},
				{ NDO_DATA_ESCALATED, BD_INT,
						{ .integer = notdata->escalated }},
				{ NDO_DATA_CONTACTSNOTIFIED, BD_INT, 
						{ .integer = notdata->contacts_notified }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_NOTIFICATIONDATA,
					notification_data, 
					sizeof(notification_data) / sizeof(notification_data[ 0]),
					TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != ack_author) free(ack_author);
		if(NULL != ack_data) free(ack_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_service_check_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_service_check_data *scdata = (nebstruct_service_check_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *command_name = NULL;
	char *command_args = NULL;
	char *command_line = NULL;
	char *output = NULL;
	char *long_output = NULL;
	char *perf_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_SERVICE_CHECK_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(scdata->host_name);
		service_desc = ndo_escape_buffer(scdata->service_description);
		command_name = ndo_escape_buffer(scdata->command_name);
		command_args = ndo_escape_buffer(scdata->command_args);
		command_line = ndo_escape_buffer(scdata->command_line);
		output = ndo_escape_buffer(scdata->output);
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		long_output = ndo_escape_buffer(scdata->long_output);
#endif
		perf_data = ndo_escape_buffer(scdata->perf_data);

		{
			struct ndo_broker_data service_check_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = scdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = scdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = scdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = scdata->timestamp }},
				{ NDO_DATA_HOST, BD_STRING, { .string =
						(host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_CHECKTYPE, BD_INT,
						{ .integer = scdata->check_type }},
				{ NDO_DATA_CURRENTCHECKATTEMPT, BD_INT, 
						{ .integer = scdata->current_attempt }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = scdata->max_attempts }},
				{ NDO_DATA_STATETYPE, BD_INT,
						{ .integer = scdata->state_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = scdata->state }},
				{ NDO_DATA_TIMEOUT, BD_INT, { .integer = scdata->timeout }},
				{ NDO_DATA_COMMANDNAME, BD_STRING, { .string =
						(command_name == NULL) ? "" : command_name }},
				{ NDO_DATA_COMMANDARGS, BD_STRING, { .string =
						(command_args == NULL) ? "" : command_args }},
				{ NDO_DATA_COMMANDLINE, BD_STRING, { .string =
						(command_line == NULL) ? "" : command_line }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL,
						{ .timestamp = scdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL,
						{ .timestamp = scdata->end_time }},
				{ NDO_DATA_EARLYTIMEOUT, BD_INT,
						{ .integer = scdata->early_timeout }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = scdata->execution_time }},
				{ NDO_DATA_LATENCY, BD_FLOAT,
						{ .floating_point = scdata->latency }},
				{ NDO_DATA_RETURNCODE, BD_INT,
						{ .integer = scdata->return_code }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_PERFDATA, BD_STRING, 
						{ .string = (perf_data == NULL) ? "" : perf_data }}
				};

			/* Nagios XI MOD */
			/* send only the data we really use */
			if(scdata->type == NEBTYPE_SERVICECHECK_PROCESSED) {
				ndomod_broker_data_serialize(dbufp, NDO_API_SERVICECHECKDATA,
						service_check_data, sizeof(service_check_data) /
						sizeof(service_check_data[ 0]), TRUE);
				}
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != command_name) free(command_name);
		if(NULL != command_args) free(command_args);
		if(NULL != command_line) free(command_line);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != perf_data) free(perf_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_host_check_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_host_check_data *hcdata = (nebstruct_host_check_data *)data;
	char *host_name = NULL;
	char *command_name = NULL;
	char *command_args = NULL;
	char *command_line = NULL;
	char *output = NULL;
	char *long_output = NULL;
	char *perf_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_HOST_CHECK_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(hcdata->host_name);
		command_name = ndo_escape_buffer(hcdata->command_name);
		command_args = ndo_escape_buffer(hcdata->command_args);
		command_line = ndo_escape_buffer(hcdata->command_line);
		output = ndo_escape_buffer(hcdata->output);
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		long_output = ndo_escape_buffer(hcdata->long_output);
#endif
		perf_data = ndo_escape_buffer(hcdata->perf_data);

		{
			struct ndo_broker_data host_check_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = hcdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = hcdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = hcdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = hcdata->timestamp }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_CHECKTYPE, BD_INT,
						{ .integer = hcdata->check_type }},
				{ NDO_DATA_CURRENTCHECKATTEMPT, BD_INT, 
						{ .integer = hcdata->current_attempt }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = hcdata->max_attempts }},
				{ NDO_DATA_STATETYPE, BD_INT,
						{ .integer = hcdata->state_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = hcdata->state }},
				{ NDO_DATA_TIMEOUT, BD_INT, { .integer = hcdata->timeout }},
				{ NDO_DATA_COMMANDNAME, BD_STRING, { .string =
						(command_name == NULL) ? "" : command_name }},
				{ NDO_DATA_COMMANDARGS, BD_STRING, { .string =
						(command_args == NULL) ? "" : command_args }},
				{ NDO_DATA_COMMANDLINE, BD_STRING, { .string =
						(command_line == NULL) ? "" : command_line }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL,
						{ .timestamp = hcdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL,
						{ .timestamp = hcdata->end_time }},
				{ NDO_DATA_EARLYTIMEOUT, BD_INT,
						{ .integer = hcdata->early_timeout }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = hcdata->execution_time }},
				{ NDO_DATA_LATENCY, BD_FLOAT,
						{ .floating_point = hcdata->latency }},
				{ NDO_DATA_RETURNCODE, BD_INT,
						{ .integer = hcdata->return_code }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_PERFDATA, BD_STRING, 
						{ .string = (perf_data == NULL) ? "" : perf_data }}
				};

			/* Nagios XI MOD */
			/* send only the data we really use */
			if(hcdata->type == NEBTYPE_HOSTCHECK_PROCESSED) {
				ndomod_broker_data_serialize(dbufp, NDO_API_HOSTCHECKDATA,
						host_check_data,
						sizeof(host_check_data) / sizeof(host_check_data[ 0]),
						TRUE);
				}
		}

		if(NULL != host_name) free(host_name);
		if(NULL != command_name) free(command_name);
		if(NULL != command_args) free(command_args);
		if(NULL != command_line) free(command_line);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != perf_data) free(perf_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_comment_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_comment_data *comdata = (nebstruct_comment_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *author_name = NULL;
	char *comment_text = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_COMMENT_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(comdata->host_name);
		service_desc = ndo_escape_buffer(comdata->service_description);
		author_name = ndo_escape_buffer(comdata->author_name);
		data = ndo_escape_buffer(comdata->comment_data);

		{
			struct ndo_broker_data comment_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = comdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = comdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = comdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = comdata->timestamp }},
				{ NDO_DATA_COMMENTTYPE, BD_INT,
						{ .integer = comdata->comment_type }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_ENTRYTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)comdata->entry_time }},
				{ NDO_DATA_AUTHORNAME, BD_STRING, 
						{ .string = (author_name == NULL) ? "" : author_name }},
				{ NDO_DATA_COMMENT, BD_STRING, { .string =
						(comment_text == NULL) ? "" : comment_text }},
				{ NDO_DATA_PERSISTENT, BD_INT,
						{ .integer = comdata->persistent }},
				{ NDO_DATA_SOURCE, BD_INT, { .integer = comdata->source }},
				{ NDO_DATA_ENTRYTYPE, BD_INT,
						{ .integer = comdata->entry_type }},
				{ NDO_DATA_EXPIRES, BD_INT, { .integer = comdata->expires }},
				{ NDO_DATA_EXPIRATIONTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)comdata->expire_time }},
				{ NDO_DATA_COMMENTID, BD_UNSIGNED_LONG, 
						{ .unsigned_long = comdata->comment_id }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_COMMENTDATA,
					comment_data, 
					sizeof(comment_data) / sizeof(comment_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != author_name) free(author_name);
		if(NULL != data) free(data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_downtime_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_downtime_data *downdata = (nebstruct_downtime_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *author_name = NULL;
	char *comment_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_DOWNTIME_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(downdata->host_name);
		service_desc = ndo_escape_buffer(downdata->service_description);
		author_name = ndo_escape_buffer(downdata->author_name);
		comment_data = ndo_escape_buffer(downdata->comment_data);

		{
			struct ndo_broker_data downtime_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = downdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = downdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = downdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = downdata->timestamp }},
				{ NDO_DATA_DOWNTIMETYPE, BD_INT, 
						{ .integer = downdata->downtime_type }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_ENTRYTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->entry_time }},
				{ NDO_DATA_AUTHORNAME, BD_STRING, 
						{ .string = (author_name == NULL) ? "" : author_name }},
				{ NDO_DATA_COMMENT, BD_STRING, { .string =
						(comment_data == NULL) ? "" : comment_data }},
				{ NDO_DATA_STARTTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->end_time }},
				{ NDO_DATA_FIXED, BD_INT, { .integer = downdata->fixed }},
				{ NDO_DATA_DURATION, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->duration }},
				{ NDO_DATA_TRIGGEREDBY, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->triggered_by }},
				{ NDO_DATA_DOWNTIMEID, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)downdata->downtime_id }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_DOWNTIMEDATA,
					downtime_data,
					sizeof(downtime_data) / sizeof(downtime_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != author_name) free(author_name);
		if(NULL != comment_data) free(comment_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_flapping_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_flapping_data *flapdata = (nebstruct_flapping_data *)data;
	comment *temp_comment = NULL;
	char *host_name = NULL;
	char *service_desc = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_FLAPPING_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(flapdata->host_name);
		service_desc = ndo_escape_buffer(flapdata->service_description);

		temp_comment = find_comment(flapdata->comment_id,
				flapdata->flapping_type == HOST_FLAPPING ? HOST_COMMENT :
				SERVICE_COMMENT);

		{
			struct ndo_broker_data flapping_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = flapdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = flapdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = flapdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = flapdata->timestamp }},
				{ NDO_DATA_FLAPPINGTYPE, BD_INT, 
						{ .integer = flapdata->flapping_type }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_PERCENTSTATECHANGE, BD_FLOAT, 
						{ .floating_point = flapdata->percent_change }},
				{ NDO_DATA_HIGHTHRESHOLD, BD_FLOAT, 
						{ .floating_point = flapdata->high_threshold }},
				{ NDO_DATA_LOWTHRESHOLD, BD_FLOAT, 
						{ .floating_point = flapdata->low_threshold }},
				{ NDO_DATA_COMMENTTIME, BD_UNSIGNED_LONG, { .unsigned_long = 
						(temp_comment == NULL) ? 0L :
						(unsigned long)temp_comment->entry_time }},
				{ NDO_DATA_COMMENTID, BD_UNSIGNED_LONG, 
						{ .unsigned_long = flapdata->comment_id }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_FLAPPINGDATA,
					flapping_data,
					sizeof(flapping_data) / sizeof(flapping_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_program_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_program_status_data *psdata =
			(nebstruct_program_status_data *)data;
	char *global_host_event_handler = NULL;
	char *global_service_event_handler = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_PROGRAM_STATUS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		global_host_event_handler =
				ndo_escape_buffer(psdata->global_host_event_handler);
		global_service_event_handler =
				ndo_escape_buffer(psdata->global_service_event_handler);

		{
			struct ndo_broker_data program_status_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = psdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = psdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = psdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = psdata->timestamp }},
				{ NDO_DATA_PROGRAMSTARTTIME, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
						(unsigned long)psdata->program_start }},
				{ NDO_DATA_PROCESSID, BD_INT, { .integer = psdata->pid }},
				{ NDO_DATA_DAEMONMODE, BD_INT,
						{ .integer = psdata->daemon_mode }},
				{ NDO_DATA_LASTCOMMANDCHECK, BD_UNSIGNED_LONG,
						{ .unsigned_long = 
#ifdef BUILD_NAGIOS_4X
							0L
#else
							(unsigned long)psdata->last_command_check
#endif
						}},
				{ NDO_DATA_LASTLOGROTATION, BD_UNSIGNED_LONG, 
						{ .unsigned_long = psdata->last_log_rotation }},
				{ NDO_DATA_NOTIFICATIONSENABLED, BD_INT, 
						{ .integer = psdata->notifications_enabled }},
				{ NDO_DATA_ACTIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer = psdata->active_service_checks_enabled }},
				{ NDO_DATA_PASSIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer = psdata->passive_service_checks_enabled }},
				{ NDO_DATA_ACTIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer = psdata->active_host_checks_enabled }},
				{ NDO_DATA_PASSIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer = psdata->passive_host_checks_enabled }},
				{ NDO_DATA_EVENTHANDLERSENABLED, BD_INT, 
						{ .integer = psdata->event_handlers_enabled }},
				{ NDO_DATA_FLAPDETECTIONENABLED, BD_INT, 
						{ .integer = psdata->flap_detection_enabled }},
				{ NDO_DATA_FAILUREPREDICTIONENABLED, BD_INT, { .integer = 
#ifdef BUILD_NAGIOS_4X
							0L
#else
							psdata->failure_prediction_enabled
#endif
						}},
				{ NDO_DATA_PROCESSPERFORMANCEDATA, BD_INT, 
						{ .integer = psdata->process_performance_data }},
				{ NDO_DATA_OBSESSOVERHOSTS, BD_INT, 
						{ .integer = psdata->obsess_over_hosts }},
				{ NDO_DATA_OBSESSOVERSERVICES, BD_INT, 
						{ .integer = psdata->obsess_over_services }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = psdata->modified_host_attributes }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
						psdata->modified_service_attributes }},
				{ NDO_DATA_GLOBALHOSTEVENTHANDLER, BD_STRING, 
						{ .string = (global_host_event_handler == NULL) ? ""
						: global_host_event_handler }},
				{ NDO_DATA_GLOBALSERVICEEVENTHANDLER, BD_STRING, 
						{ .string = (global_service_event_handler == NULL) ? ""
						: global_service_event_handler }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_PROGRAMSTATUSDATA,
					program_status_data, sizeof(program_status_data) /
					sizeof(program_status_data[ 0]), TRUE);
		}

		if(NULL != global_host_event_handler) free(global_host_event_handler);
		if(NULL != global_service_event_handler) free(global_service_event_handler);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_host_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_host_status_data *hsdata = (nebstruct_host_status_data *)data;
	host *temp_host = NULL;
	char *host_name = NULL;
	char *output = NULL;
	char *long_output = NULL;
	char *perf_data = NULL;
	char *event_handler = NULL;
	char *check_command = NULL;
	char *check_period = NULL;
	double retry_interval = 0.0;


	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_HOST_STATUS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_host = (host *)hsdata->object_ptr) == NULL) {
			return bdr_enoent;
			}

		host_name = ndo_escape_buffer(temp_host->name);
		output = ndo_escape_buffer(temp_host->plugin_output);
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		long_output = ndo_escape_buffer(temp_host->long_plugin_output);
#endif
		perf_data = ndo_escape_buffer(temp_host->perf_data);
		event_handler = ndo_escape_buffer(temp_host->event_handler);
#ifdef BUILD_NAGIOS_4X
		check_command = ndo_escape_buffer(temp_host->check_command);
#else
		check_command = ndo_escape_buffer(temp_host->host_check_command);
#endif
		check_period = ndo_escape_buffer(temp_host->check_period);

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		retry_interval = temp_host->retry_interval;
#endif

		{
			struct ndo_broker_data host_status_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = hsdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = hsdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = hsdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = hsdata->timestamp }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_PERFDATA, BD_STRING, 
						{ .string = (perf_data == NULL) ? "" : perf_data }},
				{ NDO_DATA_CURRENTSTATE, BD_INT, 
						{ .integer = temp_host->current_state }},
				{ NDO_DATA_HASBEENCHECKED, BD_INT, 
						{ .integer = temp_host->has_been_checked }},
				{ NDO_DATA_SHOULDBESCHEDULED, BD_INT, 
						{ .integer = temp_host->should_be_scheduled }},
				{ NDO_DATA_CURRENTCHECKATTEMPT, BD_INT, 
						{ .integer = temp_host->current_attempt }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = temp_host->max_attempts }},
				{ NDO_DATA_LASTHOSTCHECK, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)temp_host->last_check }},
				{ NDO_DATA_NEXTHOSTCHECK, BD_UNSIGNED_LONG, { .unsigned_long = 
						(unsigned long)temp_host->next_check }},
				{ NDO_DATA_CHECKTYPE, BD_INT, 
						{ .integer = temp_host->check_type }},
				{ NDO_DATA_LASTSTATECHANGE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_host->last_state_change }},
				{ NDO_DATA_LASTHARDSTATECHANGE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_host->last_hard_state_change }},
				{ NDO_DATA_LASTHARDSTATE, BD_INT, 
						{ .integer = temp_host->last_hard_state }},
				{ NDO_DATA_LASTTIMEUP, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_host->last_time_up }},
				{ NDO_DATA_LASTTIMEDOWN, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_host->last_time_down }},
				{ NDO_DATA_LASTTIMEUNREACHABLE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_host->last_time_unreachable }},
				{ NDO_DATA_STATETYPE, BD_INT, 
						{ .integer = temp_host->state_type }},
				{ NDO_DATA_LASTHOSTNOTIFICATION, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
#ifdef BUILD_NAGIOS_4X
			 				(unsigned long)temp_host->last_notification
#else
			 				(unsigned long)temp_host->last_host_notification
#endif
						}},
				{ NDO_DATA_NEXTHOSTNOTIFICATION, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
#ifdef BUILD_NAGIOS_4X
			 				(unsigned long)temp_host->next_notification
#else
			 				(unsigned long)temp_host->next_host_notification
#endif
						}},
				{ NDO_DATA_NOMORENOTIFICATIONS, BD_INT, 
						{ .integer = temp_host->no_more_notifications }},
				{ NDO_DATA_NOTIFICATIONSENABLED, BD_INT, 
						{ .integer = temp_host->notifications_enabled }},
				{ NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, BD_INT, 
						{ .integer = 
						temp_host->problem_has_been_acknowledged }},
				{ NDO_DATA_ACKNOWLEDGEMENTTYPE, BD_INT, 
						{ .integer = temp_host->acknowledgement_type }},
				{ NDO_DATA_CURRENTNOTIFICATIONNUMBER, BD_INT, 
						{ .integer = temp_host->current_notification_number }},
				{ NDO_DATA_PASSIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_host->accept_passive_checks
#else
			 				temp_host->accept_passive_host_checks
#endif
						}},
				{ NDO_DATA_EVENTHANDLERENABLED, BD_INT, 
						{ .integer = temp_host->event_handler_enabled }},
				{ NDO_DATA_ACTIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer = temp_host->checks_enabled }},
				{ NDO_DATA_FLAPDETECTIONENABLED, BD_INT, 
						{ .integer = temp_host->flap_detection_enabled }},
				{ NDO_DATA_ISFLAPPING, BD_INT, 
						{ .integer = temp_host->is_flapping }},
				{ NDO_DATA_PERCENTSTATECHANGE, BD_FLOAT, 
						{ .floating_point = temp_host->percent_state_change }},
				{ NDO_DATA_LATENCY, BD_FLOAT, 
						{ .floating_point = temp_host->latency }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = temp_host->execution_time }},
				{ NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, BD_INT, 
						{ .integer = temp_host->scheduled_downtime_depth }},
				{ NDO_DATA_FAILUREPREDICTIONENABLED, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				0
#else
			 				temp_host->failure_prediction_enabled
#endif
						}},
				{ NDO_DATA_PROCESSPERFORMANCEDATA, BD_INT, 
						{ .integer = temp_host->process_performance_data }},
				{ NDO_DATA_OBSESSOVERHOST, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				temp_host->obsess
#else
			 				temp_host->obsess_over_host
#endif
						}},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = temp_host->modified_attributes }},
				{ NDO_DATA_EVENTHANDLER, BD_STRING, { .string =
						(event_handler == NULL) ? "" : event_handler }},
				{ NDO_DATA_CHECKCOMMAND, BD_STRING, { .string =
						(check_command == NULL) ? "" : check_command }},
				{ NDO_DATA_NORMALCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_host->check_interval }},
				{ NDO_DATA_RETRYCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = (double)retry_interval }},
				{ NDO_DATA_HOSTCHECKPERIOD, BD_STRING, { .string =
						(check_period == NULL) ? "" : check_period }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_HOSTSTATUSDATA,
					host_status_data, 
					sizeof(host_status_data) / sizeof(host_status_data[ 0]),
					FALSE);
		}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		ndomod_customvars_serialize(temp_host->custom_variables, dbufp);
#endif

		ndomod_enddata_serialize(dbufp);

		if(NULL != host_name) free(host_name);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != perf_data) free(perf_data);
		if(NULL != event_handler) free(event_handler);
		if(NULL != check_command) free(check_command);
		if(NULL != check_period) free(check_period);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_service_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_service_status_data *ssdata =
			(nebstruct_service_status_data *)data;
	service *temp_service = NULL;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *output = NULL;
	char *long_output = NULL;
	char *perf_data = NULL;
	char *event_handler = NULL;
	char *check_command = NULL;
	char *check_period = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_SERVICE_STATUS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_service = (service *)ssdata->object_ptr) == NULL) {
			return bdr_enoent;
			}

		host_name = ndo_escape_buffer(temp_service->host_name);
		service_desc = ndo_escape_buffer(temp_service->description);
		output = ndo_escape_buffer(temp_service->plugin_output);
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		long_output = ndo_escape_buffer(temp_service->long_plugin_output);
#endif
		perf_data = ndo_escape_buffer(temp_service->perf_data);
		event_handler = ndo_escape_buffer(temp_service->event_handler);
#ifdef BUILD_NAGIOS_4X
		check_command = ndo_escape_buffer(temp_service->check_command);
#else
		check_command = ndo_escape_buffer(temp_service->service_check_command);
#endif
		check_period = ndo_escape_buffer(temp_service->check_period);

		{
			struct ndo_broker_data service_status_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = ssdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = ssdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = ssdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = ssdata->timestamp }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_PERFDATA, BD_STRING, 
						{ .string = (perf_data == NULL) ? "" : perf_data }},
				{ NDO_DATA_CURRENTSTATE, BD_INT, 
						{ .integer = temp_service->current_state }},
				{ NDO_DATA_HASBEENCHECKED, BD_INT, 
						{ .integer = temp_service->has_been_checked }},
				{ NDO_DATA_SHOULDBESCHEDULED, BD_INT, 
						{ .integer = temp_service->should_be_scheduled }},
				{ NDO_DATA_CURRENTCHECKATTEMPT, BD_INT, 
						{ .integer = temp_service->current_attempt }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = temp_service->max_attempts }},
				{ NDO_DATA_LASTSERVICECHECK, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_check }},
				{ NDO_DATA_NEXTSERVICECHECK, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->next_check }},
				{ NDO_DATA_CHECKTYPE, BD_INT, 
						{ .integer = temp_service->check_type }},
				{ NDO_DATA_LASTSTATECHANGE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_state_change }},
				{ NDO_DATA_LASTHARDSTATECHANGE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_hard_state_change }},
				{ NDO_DATA_LASTHARDSTATE, BD_INT, 
						{ .integer = temp_service->last_hard_state }},
				{ NDO_DATA_LASTTIMEOK, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_time_ok }},
				{ NDO_DATA_LASTTIMEWARNING, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_time_warning }},
				{ NDO_DATA_LASTTIMEUNKNOWN, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_time_unknown }},
				{ NDO_DATA_LASTTIMECRITICAL, BD_UNSIGNED_LONG, 
						{ .unsigned_long = 
						(unsigned long)temp_service->last_time_critical }},
				{ NDO_DATA_STATETYPE, BD_INT, 
						{ .integer = temp_service->state_type }},
				{ NDO_DATA_LASTSERVICENOTIFICATION, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
		 				(unsigned long)temp_service->last_notification }},
				{ NDO_DATA_NEXTSERVICENOTIFICATION, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
		 				(unsigned long)temp_service->next_notification }},
				{ NDO_DATA_NOMORENOTIFICATIONS, BD_INT, 
						{ .integer = temp_service->no_more_notifications }},
				{ NDO_DATA_NOTIFICATIONSENABLED, BD_INT, 
						{ .integer = temp_service->notifications_enabled }},
				{ NDO_DATA_PROBLEMHASBEENACKNOWLEDGED, BD_INT, 
						{ .integer = 
						temp_service->problem_has_been_acknowledged }},
				{ NDO_DATA_ACKNOWLEDGEMENTTYPE, BD_INT, 
						{ .integer = temp_service->acknowledgement_type }},
				{ NDO_DATA_CURRENTNOTIFICATIONNUMBER, BD_INT, 
						{ .integer = 
						temp_service->current_notification_number }},
				{ NDO_DATA_PASSIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_service->accept_passive_checks
#else
			 				temp_service->accept_passive_service_checks
#endif
						}},
				{ NDO_DATA_EVENTHANDLERENABLED, BD_INT, 
						{ .integer = temp_service->event_handler_enabled }},
				{ NDO_DATA_ACTIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer = temp_service->checks_enabled }},
				{ NDO_DATA_FLAPDETECTIONENABLED, BD_INT, 
						{ .integer = temp_service->flap_detection_enabled }},
				{ NDO_DATA_ISFLAPPING, BD_INT, 
						{ .integer = temp_service->is_flapping }},
				{ NDO_DATA_PERCENTSTATECHANGE, BD_FLOAT, 
						{ .floating_point = 
						temp_service->percent_state_change }},
				{ NDO_DATA_LATENCY, BD_FLOAT, 
						{ .floating_point = temp_service->latency }},
				{ NDO_DATA_EXECUTIONTIME, BD_FLOAT, 
						{ .floating_point = temp_service->execution_time }},
				{ NDO_DATA_SCHEDULEDDOWNTIMEDEPTH, BD_INT, 
						{ .integer = temp_service->scheduled_downtime_depth }},
				{ NDO_DATA_FAILUREPREDICTIONENABLED, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				0
#else
			 				temp_service->failure_prediction_enabled
#endif
						}},
				{ NDO_DATA_PROCESSPERFORMANCEDATA, BD_INT, 
						{ .integer = temp_service->process_performance_data }},
				{ NDO_DATA_OBSESSOVERSERVICE, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				temp_service->obsess
#else
			 				temp_service->obsess_over_service
#endif
						}},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = temp_service->modified_attributes }},
				{ NDO_DATA_EVENTHANDLER, BD_STRING, { .string =
						(event_handler == NULL) ? "" : event_handler }},
				{ NDO_DATA_CHECKCOMMAND, BD_STRING, { .string =
						(check_command == NULL) ? "" : check_command }},
				{ NDO_DATA_NORMALCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->check_interval }},
				{ NDO_DATA_RETRYCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->retry_interval }},
				{ NDO_DATA_SERVICECHECKPERIOD, BD_STRING, { .string =
						(check_period == NULL) ? "" : check_period }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_SERVICESTATUSDATA,
					service_status_data, sizeof(service_status_data) / 
					sizeof(service_status_data[ 0]), FALSE);
		}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		ndomod_customvars_serialize(temp_service->custom_variables, dbufp);
#endif

		ndomod_enddata_serialize(dbufp);

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != perf_data) free(perf_data);
		if(NULL != event_handler) free(event_handler);
		if(NULL != check_command) free(check_command);
		if(NULL != check_period) free(check_period);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_adaptive_program_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_adaptive_program_data *apdata =
			(nebstruct_adaptive_program_data *)data;
	char *global_host_event_handler = NULL;
	char *global_service_event_handler = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		global_host_event_handler =
				ndo_escape_buffer(global_host_event_handler);
		global_service_event_handler =
				ndo_escape_buffer(global_service_event_handler);
		{
			struct ndo_broker_data adaptive_program_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = apdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = apdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = apdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = apdata->timestamp }},
				{ NDO_DATA_COMMANDTYPE, BD_INT,
						{ .integer = apdata->command_type }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = apdata->modified_host_attribute }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = apdata->modified_host_attributes }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTE, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
						apdata->modified_service_attribute }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long =
						apdata->modified_service_attributes }},
				{ NDO_DATA_GLOBALHOSTEVENTHANDLER, BD_STRING, 
						{ .string = (global_host_event_handler == NULL) ? ""
						: global_host_event_handler }},
				{ NDO_DATA_GLOBALSERVICEEVENTHANDLER, BD_STRING, 
						{ .string = (global_service_event_handler == NULL) ? ""
						: global_service_event_handler }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVEPROGRAMDATA,
					adaptive_program_data, sizeof(adaptive_program_data) / 
					sizeof(adaptive_program_data[ 0]), TRUE);
		}

		if(NULL != global_host_event_handler) free(global_host_event_handler);
		if(NULL != global_service_event_handler) free(global_service_event_handler);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_adaptive_host_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_adaptive_host_data *ahdata = (nebstruct_adaptive_host_data *)data;
	host *temp_host = NULL;
	double retry_interval = 0.0;
	char *host_name = NULL;
	char *event_handler = NULL;
	char *check_command = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_ADAPTIVE_HOST_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_host = (host *)ahdata->object_ptr) == NULL ){
			return bdr_enoent;
			}

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		retry_interval = temp_host->retry_interval;
#endif

		host_name = ndo_escape_buffer(temp_host->name);
		event_handler = ndo_escape_buffer(temp_host->event_handler);
#ifdef BUILD_NAGIOS_4X
		check_command = ndo_escape_buffer(temp_host->check_command);
#else
		check_command = ndo_escape_buffer(temp_host->host_check_command);
#endif

		{
			struct ndo_broker_data adaptive_host_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = ahdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = ahdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = ahdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = ahdata->timestamp }},
				{ NDO_DATA_COMMANDTYPE, BD_INT, 
						{ .integer = ahdata->command_type }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = ahdata->modified_attribute }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = ahdata->modified_attributes }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL ) ? "" : host_name }},
				{ NDO_DATA_EVENTHANDLER, BD_STRING, { .string =
						(event_handler == NULL ) ? "" : event_handler }},
				{ NDO_DATA_CHECKCOMMAND, BD_STRING, { .string =
						(check_command == NULL ) ? "" : check_command }},
				{ NDO_DATA_NORMALCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = temp_host->check_interval }},
				{ NDO_DATA_RETRYCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = retry_interval }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = temp_host->max_attempts }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVEHOSTDATA,
					adaptive_host_data, 
					sizeof(adaptive_host_data) / sizeof(adaptive_host_data[ 0]),
					TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != event_handler) free(event_handler);
		if(NULL != check_command) free(check_command);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_adaptive_service_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_adaptive_service_data *asdata =
			(nebstruct_adaptive_service_data *)data;
	service *temp_service = NULL;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *event_handler = NULL;
	char *check_command = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_service = (service *)asdata->object_ptr) == NULL) {
			return bdr_enoent;
			}

		host_name = ndo_escape_buffer(temp_service->host_name);
		service_desc = ndo_escape_buffer(temp_service->description);
		event_handler = ndo_escape_buffer(temp_service->event_handler);
#ifdef BUILD_NAGIOS_4X
		check_command = ndo_escape_buffer(temp_service->check_command);
#else
		check_command = ndo_escape_buffer(temp_service->service_check_command);
#endif

		{
			struct ndo_broker_data adaptive_service_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = asdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = asdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = asdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = asdata->timestamp }},
				{ NDO_DATA_COMMANDTYPE, BD_INT, 
						{ .integer = asdata->command_type }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTE, BD_UNSIGNED_LONG, 
						{ .unsigned_long = asdata->modified_attribute }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG, 
						{ .unsigned_long = asdata->modified_attributes }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_EVENTHANDLER, BD_STRING, { .string =
						(event_handler == NULL) ? "" : event_handler }},
				{ NDO_DATA_CHECKCOMMAND, BD_STRING, { .string =
						(check_command == NULL) ? "" : check_command }},
				{ NDO_DATA_NORMALCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->check_interval }},
				{ NDO_DATA_RETRYCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->retry_interval }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = temp_service->max_attempts }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVESERVICEDATA,
					adaptive_service_data, sizeof(adaptive_service_data) / 
					sizeof(adaptive_service_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != event_handler) free(event_handler);
		if(NULL != check_command) free(check_command);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_external_command_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_external_command_data *ecdata =
			(nebstruct_external_command_data *)data;

	char *command_string = NULL;
	char *command_args = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		command_string = ndo_escape_buffer(ecdata->command_string);
		command_args = ndo_escape_buffer(ecdata->command_args);
		{
			struct ndo_broker_data external_command_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = ecdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = ecdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = ecdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = ecdata->timestamp }},
				{ NDO_DATA_COMMANDTYPE, BD_INT,
						{ .integer = ecdata->command_type }},
				{ NDO_DATA_ENTRYTIME, BD_UNSIGNED_LONG, 
						{ .unsigned_long = (unsigned long)ecdata->entry_time }},
				{ NDO_DATA_COMMANDSTRING, BD_STRING, { .string =
						(command_string == NULL) ? "" : command_string }},
				{ NDO_DATA_COMMANDARGS, BD_STRING, { .string =
						(command_args == NULL) ? "" : command_args }},
				};
			ndomod_broker_data_serialize(dbufp, NDO_API_EXTERNALCOMMANDDATA,
					external_command_data, sizeof(external_command_data) / 
					sizeof(external_command_data[ 0]), TRUE);
		}

		if(NULL != command_string) free(command_string);
		if(NULL != command_args) free(command_args);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_aggregated_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_aggregated_status_data *agsdata =
			(nebstruct_aggregated_status_data *)data;

	struct ndo_broker_data aggregated_status_data[] = {
		{ NDO_DATA_TYPE, BD_INT, { .integer = agsdata->type }},
		{ NDO_DATA_FLAGS, BD_INT, { .integer = agsdata->flags }},
		{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = agsdata->attr }},
		{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, { .timestamp = agsdata->timestamp }},
		};

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_AGGREGATED_STATUS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		ndomod_broker_data_serialize(dbufp, NDO_API_AGGREGATEDSTATUSDATA,
				aggregated_status_data, sizeof(aggregated_status_data) /
				sizeof(aggregated_status_data[ 0]), TRUE);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_retention_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_retention_data *rdata = (nebstruct_retention_data *)data;

	struct ndo_broker_data retention_data[] = {
		{ NDO_DATA_TYPE, BD_INT, { .integer = rdata->type }},
		{ NDO_DATA_FLAGS, BD_INT, { .integer = rdata->flags }},
		{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = rdata->attr }},
		{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, { .timestamp = rdata->timestamp }},
		};

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_RETENTION_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		ndomod_broker_data_serialize(dbufp, NDO_API_RETENTIONDATA,
				retention_data,
				sizeof(retention_data) / sizeof(retention_data[ 0]), TRUE);
		break;
	case bdp_postprocessing:
		/* retained config was just read, so dump it */
		if(rdata->type == NEBTYPE_RETENTIONDATA_ENDLOAD)
			ndomod_write_config(NDOMOD_CONFIG_DUMP_RETAINED);
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_contact_notification_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_contact_notification_data *cnotdata =
			(nebstruct_contact_notification_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *contact_name = NULL;
	char *output = NULL;
	/* Preparing long output for the future */
	char *long_output = NULL;
	/* Preparing for long_output in the future */
	char *output2 = NULL;
	char *ack_author = NULL;
	char *ack_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(cnotdata->host_name);
		service_desc = ndo_escape_buffer(cnotdata->service_description);
		contact_name = ndo_escape_buffer(cnotdata->contact_name);
		output = ndo_escape_buffer(cnotdata->output);
		/* Preparing long output for the future */
		long_output = ndo_escape_buffer(cnotdata->output);
		/* Preparing for long_output in the future */
		output2 = ndo_escape_buffer(cnotdata->output);
		ack_author = ndo_escape_buffer(cnotdata->ack_author);
		ack_data = ndo_escape_buffer(cnotdata->ack_data);

		{
			struct ndo_broker_data contact_notification_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = cnotdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = cnotdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = cnotdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = cnotdata->timestamp }},
				{ NDO_DATA_NOTIFICATIONTYPE, BD_INT, 
						{ .integer = cnotdata->notification_type }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL, 
						{ .timestamp = cnotdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL,
						{ .timestamp = cnotdata->end_time }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_CONTACTNAME, BD_STRING, { .string =
						(contact_name == NULL) ? "" : contact_name }},
				{ NDO_DATA_NOTIFICATIONREASON, BD_INT, 
						{ .integer = cnotdata->reason_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = cnotdata->state }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output2 == NULL) ? "" : output2 }},
				{ NDO_DATA_ACKAUTHOR, BD_STRING, 
						{ .string = (ack_author == NULL) ? "" : ack_author }},
				{ NDO_DATA_ACKDATA, BD_STRING, 
						{ .string = (ack_data == NULL) ? "" : ack_data }},
				};
				ndomod_broker_data_serialize(dbufp,
						NDO_API_CONTACTNOTIFICATIONDATA,
						contact_notification_data,
						sizeof(contact_notification_data) /
						sizeof(contact_notification_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != contact_name) free(contact_name);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		if(NULL != output2) free(output2);
		if(NULL != ack_author) free(ack_author);
		if(NULL != ack_data) free(ack_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_contact_notification_method_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_contact_notification_method_data *cnotmdata =
			(nebstruct_contact_notification_method_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *contact_name = NULL;
	char *command_name = NULL;
	char *command_args = NULL;
	char *output = NULL;
	char *ack_author = NULL;
	char *ack_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(cnotmdata->host_name);
		service_desc = ndo_escape_buffer(cnotmdata->service_description);
		contact_name = ndo_escape_buffer(cnotmdata->contact_name);
		command_name = ndo_escape_buffer(cnotmdata->command_name);
		command_args = ndo_escape_buffer(cnotmdata->command_args);
		output = ndo_escape_buffer(cnotmdata->output);
		ack_author = ndo_escape_buffer(cnotmdata->ack_author);
		ack_data = ndo_escape_buffer(cnotmdata->ack_data);

		{
			struct ndo_broker_data contact_notification_method_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = cnotmdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = cnotmdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = cnotmdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = cnotmdata->timestamp }},
				{ NDO_DATA_NOTIFICATIONTYPE, BD_INT, 
						{ .integer = cnotmdata->notification_type }},
				{ NDO_DATA_STARTTIME, BD_TIMEVAL, 
						{ .timestamp = cnotmdata->start_time }},
				{ NDO_DATA_ENDTIME, BD_TIMEVAL, 
						{ .timestamp = cnotmdata->end_time }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_CONTACTNAME, BD_STRING, { .string =
						(contact_name == NULL) ? "" : contact_name }},
				{ NDO_DATA_COMMANDNAME, BD_STRING, { .string =
						(command_name == NULL) ? "" : command_name }},
				{ NDO_DATA_COMMANDARGS, BD_STRING, { .string =
						(command_args == NULL) ? "" : command_args }},
				{ NDO_DATA_NOTIFICATIONREASON, BD_INT, 
						{ .integer = cnotmdata->reason_type }},
				{ NDO_DATA_STATE, BD_INT, { .integer = cnotmdata->state }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_ACKAUTHOR, BD_STRING, 
						{ .string = (ack_author == NULL) ? "" : ack_author }},
				{ NDO_DATA_ACKDATA, BD_STRING, 
						{ .string = (ack_data == NULL) ? "" : ack_data }},
				};

				ndomod_broker_data_serialize(dbufp,
						NDO_API_CONTACTNOTIFICATIONMETHODDATA,
						contact_notification_method_data,
						sizeof(contact_notification_method_data) /
						sizeof(contact_notification_method_data[ 0]), TRUE);
			}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != contact_name) free(contact_name);
		if(NULL != command_name) free(command_name);
		if(NULL != command_args) free(command_args);
		if(NULL != output) free(output);
		if(NULL != ack_author) free(ack_author);
		if(NULL != ack_data) free(ack_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_acknowledgement_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_acknowledgement_data *ackdata =
			(nebstruct_acknowledgement_data *)data;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *author_name = NULL;
	char *comment_data = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		host_name = ndo_escape_buffer(ackdata->host_name);
		service_desc = ndo_escape_buffer(ackdata->service_description);
		author_name = ndo_escape_buffer(ackdata->author_name);
		comment_data = ndo_escape_buffer(ackdata->comment_data);

		{
			struct ndo_broker_data acknowledgement_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = ackdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = ackdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, { .integer = ackdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = ackdata->timestamp }},
				{ NDO_DATA_ACKNOWLEDGEMENTTYPE, BD_INT, 
						{ .integer = ackdata->acknowledgement_type }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_AUTHORNAME, BD_STRING, 
						{ .string = (author_name == NULL) ? "" : author_name }},
				{ NDO_DATA_COMMENT, BD_STRING, { .string =
						(comment_data == NULL) ? "" : comment_data }},
				{ NDO_DATA_STATE, BD_INT, { .integer = ackdata->state }},
				{ NDO_DATA_STICKY, BD_INT, { .integer = ackdata->is_sticky }},
				{ NDO_DATA_PERSISTENT, BD_INT, 
						{ .integer = ackdata->persistent_comment }},
				{ NDO_DATA_NOTIFYCONTACTS, BD_INT, 
						{ .integer = ackdata->notify_contacts }},
				};

				ndomod_broker_data_serialize(dbufp, NDO_API_ACKNOWLEDGEMENTDATA,
						acknowledgement_data, sizeof(acknowledgement_data) /
						sizeof(acknowledgement_data[ 0]), TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != author_name) free(author_name);
		if(NULL != comment_data) free(comment_data);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_state_change_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_statechange_data *schangedata =
			(nebstruct_statechange_data *)data;
	host *temp_host = NULL;
	service *temp_service = NULL;
	int last_state = -1;
	int last_hard_state = -1;
	char *host_name = NULL;
	char *service_desc = NULL;
	char *output = NULL;
	char *long_output = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_STATE_CHANGE_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
#ifdef BUILD_NAGIOS_2X
		/* find host/service and get last state info */
		if(schangedata->service_description == NULL) {
			if((temp_host = find_host(schangedata->host_name)) == NULL) {
				return bdr_enoent;
				}
			last_state = temp_host->last_state;
			last_hard_state = temp_host->last_hard_state;
			}
		else{
			if((temp_service = find_service(schangedata->host_name,
					schangedata->service_description)) == NULL) {
				return bdr_enoent;
				}
			last_state = temp_service->last_state;
			last_hard_state = temp_service->last_hard_state;
			}
#else
		/* get the last state info */
		if(schangedata->service_description == NULL) {
			if((temp_host = (host *)schangedata->object_ptr) == NULL) {
				return bdr_enoent;
				}
			last_state = temp_host->last_state;
			last_hard_state = temp_host->last_hard_state;
			}
		else{
			if((temp_service = (service *)schangedata->object_ptr) == NULL) {
				return bdr_enoent;
				}
			last_state = temp_service->last_state;
			last_hard_state = temp_service->last_hard_state;
			}
#endif

		host_name = ndo_escape_buffer(schangedata->host_name);
		service_desc = ndo_escape_buffer(schangedata->service_description);
		output = ndo_escape_buffer(schangedata->output);
		/* Preparing for long_output in the future */
		long_output = ndo_escape_buffer(schangedata->output);

		{
			struct ndo_broker_data state_change_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = schangedata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = schangedata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT, 
						{ .integer = schangedata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = schangedata->timestamp }},
				{ NDO_DATA_STATECHANGETYPE, BD_INT, 
						{ .integer = schangedata->statechange_type }},
				{ NDO_DATA_HOST, BD_STRING, 
						{ .string = (host_name == NULL) ? "" : host_name }},
				{ NDO_DATA_SERVICE, BD_STRING, { .string =
						(service_desc == NULL) ? "" : service_desc }},
				{ NDO_DATA_STATECHANGE, BD_INT, { .integer = TRUE }},
				{ NDO_DATA_STATE, BD_INT, { .integer = schangedata->state }},
				{ NDO_DATA_STATETYPE, BD_INT, 
						{ .integer = schangedata->state_type }},
				{ NDO_DATA_CURRENTCHECKATTEMPT, BD_INT, 
						{ .integer = schangedata->current_attempt }},
				{ NDO_DATA_MAXCHECKATTEMPTS, BD_INT, 
						{ .integer = schangedata->max_attempts }},
				{ NDO_DATA_LASTSTATE, BD_INT, { .integer = last_state }},
				{ NDO_DATA_LASTHARDSTATE, BD_INT, 
						{ .integer = last_hard_state }},
				{ NDO_DATA_OUTPUT, BD_STRING, 
						{ .string = (output == NULL) ? "" : output }},
				{ NDO_DATA_LONGOUTPUT, BD_STRING, 
						{ .string = (long_output == NULL) ? "" : long_output }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_STATECHANGEDATA,
					state_change_data, 
					sizeof(state_change_data) / sizeof(state_change_data[ 0]), 
					TRUE);
		}

		if(NULL != host_name) free(host_name);
		if(NULL != service_desc) free(service_desc);
		if(NULL != output) free(output);
		if(NULL != long_output) free(long_output);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

#if defined(BUILD_NAGIOS_3X) || defined(BUILD_NAGIOS_4X)
static bd_result ndomod_broker_contact_status_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_contact_status_data *csdata =
			(nebstruct_contact_status_data *)data;
	contact *temp_contact = NULL;
	char *contact_name = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_CONTACT_STATUS_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_contact = (contact *)csdata->object_ptr) == NULL) {
			return bdr_enoent;
			}

		contact_name = ndo_escape_buffer(temp_contact->name);

		{
			struct ndo_broker_data contact_status_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = csdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = csdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT,
						{ .integer = csdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = csdata->timestamp }},
				{ NDO_DATA_CONTACTNAME, BD_STRING, { .string =
						(contact_name == NULL) ? "" : contact_name }},
				{ NDO_DATA_HOSTNOTIFICATIONSENABLED, BD_INT,
						{ .integer =
						temp_contact->host_notifications_enabled }},
				{ NDO_DATA_SERVICENOTIFICATIONSENABLED, BD_INT,
						{ .integer =
						temp_contact->service_notifications_enabled }},
				{ NDO_DATA_LASTHOSTNOTIFICATION, BD_UNSIGNED_LONG,
						{ .unsigned_long =
		 				(unsigned long)temp_contact->last_host_notification }},
				{ NDO_DATA_LASTSERVICENOTIFICATION, BD_UNSIGNED_LONG,
						{ .unsigned_long =
		 				(unsigned long)temp_contact->last_service_notification }},
				{ NDO_DATA_MODIFIEDCONTACTATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long = temp_contact->modified_attributes }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long =
						temp_contact->modified_host_attributes }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long =
						temp_contact->modified_service_attributes }}
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_CONTACTSTATUSDATA,
					contact_status_data, sizeof(contact_status_data) /
					sizeof(contact_status_data[ 0]), FALSE);
		}

		/* dump customvars */
		ndomod_customvars_serialize(temp_contact->custom_variables, dbufp);

		ndomod_enddata_serialize(dbufp);

		if(NULL != contact_name) free(contact_name);
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}

static bd_result ndomod_broker_adaptive_contact_data(bd_phase phase,
		unsigned long process_options, void *data, ndo_dbuf *dbufp) {

	nebstruct_adaptive_contact_data *acdata =
			(nebstruct_adaptive_contact_data *)data;
	contact *temp_contact = NULL;
	char *contact_name = NULL;

	switch(phase) {
	case bdp_preprocessing:
		if(!(process_options & NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA))
			return bdr_stop;
		break;
	case bdp_mainprocessing:
		if((temp_contact = (contact *)acdata->object_ptr) == NULL) {
			return bdr_enoent;
			}

		contact_name = ndo_escape_buffer(temp_contact->name);

		{
			struct ndo_broker_data adaptive_contact_data[] = {
				{ NDO_DATA_TYPE, BD_INT, { .integer = acdata->type }},
				{ NDO_DATA_FLAGS, BD_INT, { .integer = acdata->flags }},
				{ NDO_DATA_ATTRIBUTES, BD_INT,
						{ .integer = acdata->attr }},
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL,
						{ .timestamp = acdata->timestamp }},
				{ NDO_DATA_COMMANDTYPE, BD_INT,
						{ .integer = acdata->command_type }},
				{ NDO_DATA_MODIFIEDCONTACTATTRIBUTE, BD_UNSIGNED_LONG,
						{ .unsigned_long = acdata->modified_attribute }},
				{ NDO_DATA_MODIFIEDCONTACTATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long = acdata->modified_attributes }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTE, BD_UNSIGNED_LONG,
						{ .unsigned_long = acdata->modified_host_attribute }},
				{ NDO_DATA_MODIFIEDHOSTATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long = acdata->modified_host_attributes }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTE, BD_UNSIGNED_LONG,
						{ .unsigned_long =
						acdata->modified_service_attribute }},
				{ NDO_DATA_MODIFIEDSERVICEATTRIBUTES, BD_UNSIGNED_LONG,
						{ .unsigned_long =
						acdata->modified_service_attributes }},
				{ NDO_DATA_CONTACTNAME, BD_STRING, { .string =
						(contact_name == NULL) ? "" : contact_name }},
				{ NDO_DATA_HOSTNOTIFICATIONSENABLED, BD_INT, { .integer =
						temp_contact->host_notifications_enabled }},
				{ NDO_DATA_SERVICENOTIFICATIONSENABLED, BD_INT, { .integer =
						temp_contact->service_notifications_enabled }},
				};

			ndomod_broker_data_serialize(dbufp, NDO_API_ADAPTIVECONTACTDATA,
					adaptive_contact_data, sizeof(adaptive_contact_data) /
					sizeof(adaptive_contact_data[ 0]), TRUE);

			if(NULL != contact_name) free(contact_name);
		}
		break;
	case bdp_postprocessing:
		break;
	default:
		return bdr_ephase;
		break;
		}

	return bdr_ok;
	}
#endif


/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* dumps all configuration data to sink */
int ndomod_write_config(int config_type){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	struct timeval now;
	int result;

	if(!(ndomod_config_output_options & config_type))
		return NDO_OK;

	gettimeofday(&now,NULL);

	/* record start of config dump */
	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%d:\n%d=%s\n%d=%ld.%ld\n%d\n\n"
		 ,NDO_API_STARTCONFIGDUMP
		 ,NDO_DATA_CONFIGDUMPTYPE
		 ,(config_type==NDOMOD_CONFIG_DUMP_ORIGINAL)?NDO_API_CONFIGDUMP_ORIGINAL:NDO_API_CONFIGDUMP_RETAINED
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_API_ENDDATA
		);
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);

	/* dump object config info */
	result=ndomod_write_object_config(config_type);
	if(result!=NDO_OK)
		return result;

	/* record end of config dump */
	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%d:\n%d=%ld.%ld\n%d\n\n"
		 ,NDO_API_ENDCONFIGDUMP
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_API_ENDDATA
		);
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);

	return result;
        }


#define OBJECTCONFIG_ES_ITEMS 16

/* dumps object configuration data to sink */
int ndomod_write_object_config(int config_type){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	ndo_dbuf dbuf;
	struct timeval now;
	int x=0;
	char *es[OBJECTCONFIG_ES_ITEMS];
	command *temp_command=NULL;
	timeperiod *temp_timeperiod=NULL;
	timerange *temp_timerange=NULL;
	contact *temp_contact=NULL;
	contactgroup *temp_contactgroup=NULL;
	host *temp_host=NULL;
	hostgroup *temp_hostgroup=NULL;
	service *temp_service=NULL;
	servicegroup *temp_servicegroup=NULL;
	hostescalation *temp_hostescalation=NULL;
	serviceescalation *temp_serviceescalation=NULL;
	hostdependency *temp_hostdependency=NULL;
	servicedependency *temp_servicedependency=NULL;
#ifdef BUILD_NAGIOS_2X
	hostextinfo *temp_hostextinfo=NULL;
	serviceextinfo *temp_serviceextinfo=NULL;
#endif
	int have_2d_coords=FALSE;
	int x_2d=0;
	int y_2d=0;
	int have_3d_coords=FALSE;
	double x_3d=0.0;
	double y_3d=0.0;
	double z_3d=0.0;
	double first_notification_delay=0.0;
	double retry_interval=0.0;
	int notify_on_host_downtime=0;
	int notify_on_service_downtime=0;
	int host_notifications_enabled=0;
	int service_notifications_enabled=0;
	int can_submit_commands=0;
	int flap_detection_on_up=0;
	int flap_detection_on_down=0;
	int flap_detection_on_unreachable=0;
	int flap_detection_on_ok=0;
	int flap_detection_on_warning=0;
	int flap_detection_on_unknown=0;
	int flap_detection_on_critical=0;


	if(!(ndomod_process_options & NDOMOD_PROCESS_OBJECT_CONFIG_DATA))
		return NDO_OK;

	if(!(ndomod_config_output_options & config_type))
		return NDO_OK;

	/* get current time */
	gettimeofday(&now,NULL);

	/* initialize dynamic buffer (2KB chunk size) */
	ndo_dbuf_init(&dbuf,2048);

	/* initialize buffers */
	for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) es[x] = NULL;

	/****** dump command config ******/
	for (temp_command = command_list; temp_command; temp_command = temp_command->next) {

		es[0] = ndo_escape_buffer(temp_command->name);
		es[1] = ndo_escape_buffer(temp_command->command_line);

		{
			struct ndo_broker_data command_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, { .timestamp = now }},
				{ NDO_DATA_COMMANDNAME, BD_STRING, { .string = es[0] ? es[0] : "" }},
				{ NDO_DATA_COMMANDLINE, BD_STRING, { .string = es[1] ? es[1] : "" }},
			};

			ndomod_broker_data_serialize(&dbuf, NDO_API_COMMANDDEFINITION,
				command_definition, ARRAY_SIZE(command_definition), TRUE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* write data to sink */
		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);
		ndo_dbuf_free(&dbuf);
	}

	/****** dump timeperiod config ******/
	for (temp_timeperiod = timeperiod_list; temp_timeperiod; temp_timeperiod = temp_timeperiod->next) {

		es[0] = ndo_escape_buffer(temp_timeperiod->name);
		es[1] = ndo_escape_buffer(temp_timeperiod->alias);

		{
			struct ndo_broker_data timeperiod_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, { .timestamp = now }},
				{ NDO_DATA_TIMEPERIODNAME, BD_STRING, { .string = es[0] ? es[0] : "" }},
				{ NDO_DATA_TIMEPERIODALIAS, BD_STRING, { .string = es[1] ? es[1] : "" }},
			};

			ndomod_broker_data_serialize(&dbuf, NDO_API_TIMEPERIODDEFINITION,
				timeperiod_definition, ARRAY_SIZE(timeperiod_definition), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* dump timeranges for each day */
		for(x=0;x<7;x++){
			for(temp_timerange=temp_timeperiod->days[x];temp_timerange!=NULL;temp_timerange=temp_timerange->next){

				snprintf(temp_buffer,sizeof(temp_buffer)-1
					 ,"\n%d=%d:%lu-%lu"
					 ,NDO_DATA_TIMERANGE
					 ,x
					 ,temp_timerange->range_start
					 ,temp_timerange->range_end
					);
				temp_buffer[sizeof(temp_buffer)-1]='\x0';
				ndo_dbuf_strcat(&dbuf,temp_buffer);
			}
		}

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);
		ndo_dbuf_free(&dbuf);
	}


	/****** dump contact config ******/
	for(temp_contact=contact_list;temp_contact!=NULL;temp_contact=temp_contact->next){

		es[0]=ndo_escape_buffer(temp_contact->name);
		es[1]=ndo_escape_buffer(temp_contact->alias);
		es[2]=ndo_escape_buffer(temp_contact->email);
		es[3]=ndo_escape_buffer(temp_contact->pager);
		es[4]=ndo_escape_buffer(temp_contact->host_notification_period);
		es[5]=ndo_escape_buffer(temp_contact->service_notification_period);

#ifdef BUILD_NAGIOS_4X
		notify_on_service_downtime=flag_isset(temp_contact->service_notification_options,OPT_DOWNTIME);
		notify_on_host_downtime=flag_isset(temp_contact->host_notification_options,OPT_DOWNTIME);
		host_notifications_enabled=temp_contact->host_notifications_enabled;
		service_notifications_enabled=temp_contact->service_notifications_enabled;
		can_submit_commands=temp_contact->can_submit_commands;
#endif
#ifdef BUILD_NAGIOS_3X
		notify_on_service_downtime=temp_contact->notify_on_service_downtime;
		notify_on_host_downtime=temp_contact->notify_on_host_downtime;
		host_notifications_enabled=temp_contact->host_notifications_enabled;
		service_notifications_enabled=temp_contact->service_notifications_enabled;
		can_submit_commands=temp_contact->can_submit_commands;
#endif
#ifdef BUILD_NAGIOS_2X
		notify_on_service_downtime=0;
		notify_on_host_downtime=0;
		host_notifications_enabled=1;
		service_notifications_enabled=1;
		can_submit_commands=1;
#endif

		{
			struct ndo_broker_data contact_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_CONTACTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_CONTACTALIAS, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_EMAILADDRESS, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_PAGERADDRESS, BD_STRING, 
						{ .string = (es[3]==NULL) ? "" : es[3] }},
				{ NDO_DATA_HOSTNOTIFICATIONPERIOD, BD_STRING, 
						{ .string = (es[4]==NULL) ? "" : es[4] }},
				{ NDO_DATA_SERVICENOTIFICATIONPERIOD, BD_STRING, 
						{ .string = (es[5]==NULL) ? "" : es[5] }},
				{ NDO_DATA_SERVICENOTIFICATIONSENABLED, BD_INT, 
						{ .integer = service_notifications_enabled }},
				{ NDO_DATA_HOSTNOTIFICATIONSENABLED, BD_INT, 
						{ .integer = host_notifications_enabled }},
				{ NDO_DATA_CANSUBMITCOMMANDS, BD_INT, 
						{ .integer = can_submit_commands }},
				{ NDO_DATA_NOTIFYSERVICEUNKNOWN, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->service_notification_options,OPT_UNKNOWN)
#else
			 				temp_contact->notify_on_service_unknown
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEWARNING, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->service_notification_options,OPT_WARNING)
#else
			 				temp_contact->notify_on_service_warning
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICECRITICAL, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->service_notification_options,OPT_CRITICAL)
#else
			 				temp_contact->notify_on_service_critical
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICERECOVERY, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->service_notification_options,OPT_RECOVERY)
#else
			 				temp_contact->notify_on_service_recovery
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEFLAPPING, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->service_notification_options,OPT_FLAPPING)
#else
			 				temp_contact->notify_on_service_flapping
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEDOWNTIME, BD_INT, 
						{ .integer = notify_on_service_downtime }},
				{ NDO_DATA_NOTIFYHOSTDOWN, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->host_notification_options,
							OPT_DOWN)
#else
			 				temp_contact->notify_on_host_down
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTUNREACHABLE, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->host_notification_options,
							OPT_UNREACHABLE)
#else
			 				temp_contact->notify_on_host_unreachable
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTRECOVERY, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->host_notification_options,
							OPT_RECOVERY)
#else
			 				temp_contact->notify_on_host_recovery
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTFLAPPING, BD_INT, 
						{ .integer = 
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_contact->host_notification_options,
							OPT_FLAPPING)
#else
			 				temp_contact->notify_on_host_flapping
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTDOWNTIME, BD_INT, 
						{ .integer = notify_on_host_downtime }},
#ifdef BUILD_NAGIOS_4X
				{ NDO_DATA_MINIMUMIMPORTANCE, BD_INT, 
						{ .integer = temp_contact->minimum_value }},
#endif
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_CONTACTDEFINITION, 
					contact_definition, sizeof(contact_definition) / 
					sizeof(contact_definition[ 0]), FALSE);
		}

		/* Free the memory we allocated on this iteration. */
		for (x = 0; x < 6; x++) if (es[x]) free(es[x]);

		/* dump addresses for each contact */
		for(x=0;x<MAX_CONTACT_ADDRESSES;x++){

			es[0] = ndo_escape_buffer(temp_contact->address[x]);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"\n%d=%d:%s"
				 ,NDO_DATA_CONTACTADDRESS
				 ,x+1
				 ,es[0] ? es[0] : ""
			);

			if (es[0]) free(es[0]);

			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
		}

		/* dump host notification commands for each contact */
		ndomod_commands_serialize(temp_contact->host_notification_commands, 
				&dbuf, NDO_DATA_HOSTNOTIFICATIONCOMMAND);

		/* dump service notification commands for each contact */
		ndomod_commands_serialize(temp_contact->service_notification_commands, 
				&dbuf, NDO_DATA_SERVICENOTIFICATIONCOMMAND);

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* dump customvars */
		ndomod_customvars_serialize(temp_contact->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);
		ndo_dbuf_free(&dbuf);
	}


	/****** dump contactgroup config ******/
	for(temp_contactgroup=contactgroup_list;temp_contactgroup!=NULL;temp_contactgroup=temp_contactgroup->next){

		es[0] = ndo_escape_buffer(temp_contactgroup->group_name);
		es[1] = ndo_escape_buffer(temp_contactgroup->alias);

		{
			struct ndo_broker_data contactgroup_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_CONTACTGROUPNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_CONTACTGROUPALIAS, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_CONTACTGROUPDEFINITION, 
					contactgroup_definition, sizeof(contactgroup_definition) / 
					sizeof(contactgroup_definition[ 0]), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* dump members for each contactgroup */
		ndomod_contacts_serialize(temp_contactgroup->members, &dbuf, 
				NDO_DATA_CONTACTGROUPMEMBER);

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump host config ******/
	for(temp_host=host_list;temp_host!=NULL;temp_host=temp_host->next){

		es[0]=ndo_escape_buffer(temp_host->name);
		es[1]=ndo_escape_buffer(temp_host->alias);
		es[2]=ndo_escape_buffer(temp_host->address);
#ifdef BUILD_NAGIOS_4X
		es[3]=ndo_escape_buffer(temp_host->check_command);
#else
		es[3]=ndo_escape_buffer(temp_host->host_check_command);
#endif
		es[4]=ndo_escape_buffer(temp_host->event_handler);
		es[5]=ndo_escape_buffer(temp_host->notification_period);
		es[6]=ndo_escape_buffer(temp_host->check_period);
#ifdef BUILD_NAGIOS_4X
		es[7]=ndo_escape_buffer("");
#else
		es[7]=ndo_escape_buffer(temp_host->failure_prediction_options);
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		es[8]=ndo_escape_buffer(temp_host->notes);
		es[9]=ndo_escape_buffer(temp_host->notes_url);
		es[10]=ndo_escape_buffer(temp_host->action_url);
		es[11]=ndo_escape_buffer(temp_host->icon_image);
		es[12]=ndo_escape_buffer(temp_host->icon_image_alt);
		es[13]=ndo_escape_buffer(temp_host->vrml_image);
		es[14]=ndo_escape_buffer(temp_host->statusmap_image);
		have_2d_coords=temp_host->have_2d_coords;
		x_2d=temp_host->x_2d;
		y_2d=temp_host->y_2d;
		have_3d_coords=temp_host->have_3d_coords;
		x_3d=temp_host->x_3d;
		y_3d=temp_host->y_3d;
		z_3d=temp_host->z_3d;

		first_notification_delay=temp_host->first_notification_delay;
		retry_interval=temp_host->retry_interval;
#ifdef BUILD_NAGIOS_4X
		notify_on_host_downtime=flag_isset(temp_host->notification_options,OPT_DOWNTIME);
		flap_detection_on_up=flag_isset(temp_host->flap_detection_options,OPT_UP);
		flap_detection_on_down=flag_isset(temp_host->flap_detection_options,OPT_DOWN);
		flap_detection_on_unreachable=flag_isset(temp_host->flap_detection_options,OPT_UNREACHABLE);
#else
		notify_on_host_downtime=temp_host->notify_on_downtime;
		flap_detection_on_up=temp_host->flap_detection_on_up;
		flap_detection_on_down=temp_host->flap_detection_on_down;
		flap_detection_on_unreachable=temp_host->flap_detection_on_unreachable;
#endif
		es[15]=ndo_escape_buffer(temp_host->display_name);
#endif
#ifdef BUILD_NAGIOS_2X
		if((temp_hostextinfo=find_hostextinfo(temp_host->name))!=NULL){
			es[8]=ndo_escape_buffer(temp_hostextinfo->notes);
			es[9]=ndo_escape_buffer(temp_hostextinfo->notes_url);
			es[10]=ndo_escape_buffer(temp_hostextinfo->action_url);
			es[11]=ndo_escape_buffer(temp_hostextinfo->icon_image);
			es[12]=ndo_escape_buffer(temp_hostextinfo->icon_image_alt);
			es[13]=ndo_escape_buffer(temp_hostextinfo->vrml_image);
			es[14]=ndo_escape_buffer(temp_hostextinfo->statusmap_image);
			have_2d_coords=temp_hostextinfo->have_2d_coords;
			x_2d=temp_hostextinfo->x_2d;
			y_2d=temp_hostextinfo->y_2d;
			have_3d_coords=temp_hostextinfo->have_3d_coords;
			x_3d=temp_hostextinfo->x_3d;
			y_3d=temp_hostextinfo->y_3d;
			z_3d=temp_hostextinfo->z_3d;
			}
		else{
			es[8]=NULL;
			es[9]=NULL;
			es[10]=NULL;
			es[11]=NULL;
			es[12]=NULL;
			es[13]=NULL;
			es[14]=NULL;
			have_2d_coords=FALSE;
			x_2d=0;
			y_2d=0;
			have_3d_coords=FALSE;
			x_3d=0.0;
			y_3d=0.0;
			z_3d=0.0;
			}

		first_notification_delay=0.0;
		retry_interval=0.0;
		notify_on_host_downtime=0;
		flap_detection_on_up=1;
		flap_detection_on_down=1;
		flap_detection_on_unreachable=1;
		es[15]=ndo_escape_buffer(temp_host->name);
#endif

		{
			struct ndo_broker_data host_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_DISPLAYNAME, BD_STRING, 
						{ .string = (es[15]==NULL) ? "" : es[15] }},
				{ NDO_DATA_HOSTALIAS, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_HOSTADDRESS, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_HOSTCHECKCOMMAND, BD_STRING, 
						{ .string = (es[3]==NULL) ? "" : es[3] }},
				{ NDO_DATA_HOSTEVENTHANDLER, BD_STRING, 
						{ .string = (es[4]==NULL) ? "" : es[4] }},
				{ NDO_DATA_HOSTNOTIFICATIONPERIOD, BD_STRING, 
						{ .string = (es[5]==NULL) ? "" : es[5] }},
				{ NDO_DATA_HOSTCHECKPERIOD, BD_STRING, 
						{ .string = (es[6]==NULL) ? "" : es[6] }},
				{ NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS, BD_STRING, 
						{ .string = (es[7]==NULL) ? "" : es[7] }},
				{ NDO_DATA_HOSTCHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_host->check_interval }},
				{ NDO_DATA_HOSTRETRYINTERVAL, BD_FLOAT, 
						{ .floating_point = (double)retry_interval }},
				{ NDO_DATA_HOSTMAXCHECKATTEMPTS, BD_INT, 
						{ .integer = temp_host->max_attempts }},
				{ NDO_DATA_FIRSTNOTIFICATIONDELAY, BD_FLOAT, 
						{ .floating_point = first_notification_delay }},
				{ NDO_DATA_HOSTNOTIFICATIONINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_host->notification_interval }},
				{ NDO_DATA_NOTIFYHOSTDOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->notification_options,
							OPT_DOWN)
#else
			 				temp_host->notify_on_down
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTUNREACHABLE, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->notification_options,
							OPT_UNREACHABLE)
#else
			 				temp_host->notify_on_unreachable
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTRECOVERY, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->notification_options,
							OPT_RECOVERY)
#else
			 				temp_host->notify_on_recovery
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTFLAPPING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->notification_options,
							OPT_FLAPPING)
#else
			 				temp_host->notify_on_flapping
#endif
						}},
				{ NDO_DATA_NOTIFYHOSTDOWNTIME, BD_INT, 
						{ .integer = notify_on_host_downtime }},
				{ NDO_DATA_HOSTFLAPDETECTIONENABLED, BD_INT, 
						{ .integer = temp_host->flap_detection_enabled }},
				{ NDO_DATA_FLAPDETECTIONONUP, BD_INT, 
						{ .integer = flap_detection_on_up }},
				{ NDO_DATA_FLAPDETECTIONONDOWN, BD_INT, 
						{ .integer = flap_detection_on_down }},
				{ NDO_DATA_FLAPDETECTIONONUNREACHABLE, BD_INT, 
						{ .integer = flap_detection_on_unreachable }},
				{ NDO_DATA_LOWHOSTFLAPTHRESHOLD, BD_FLOAT, 
						{ .floating_point = temp_host->low_flap_threshold }},
				{ NDO_DATA_HIGHHOSTFLAPTHRESHOLD, BD_FLOAT, 
						{ .floating_point = temp_host->high_flap_threshold }},
				{ NDO_DATA_STALKHOSTONUP, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->stalking_options,
							OPT_UP)
#else
			 				temp_host->stalk_on_up
#endif
						}},
				{ NDO_DATA_STALKHOSTONDOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->stalking_options,
							OPT_DOWN)
#else
			 				temp_host->stalk_on_down
#endif
						}},
				{ NDO_DATA_STALKHOSTONUNREACHABLE, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_host->stalking_options,
							OPT_UNREACHABLE)
#else
			 				temp_host->stalk_on_unreachable
#endif
						}},
				{ NDO_DATA_HOSTFRESHNESSCHECKSENABLED, BD_INT, 
						{ .integer = temp_host->check_freshness }},
				{ NDO_DATA_HOSTFRESHNESSTHRESHOLD, BD_INT, 
						{ .integer = temp_host->freshness_threshold }},
				{ NDO_DATA_PROCESSHOSTPERFORMANCEDATA, BD_INT, 
						{ .integer = temp_host->process_performance_data }},
				{ NDO_DATA_ACTIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer = temp_host->checks_enabled }},
				{ NDO_DATA_PASSIVEHOSTCHECKSENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_host->accept_passive_checks
#else
			 				temp_host->accept_passive_host_checks
#endif
						}},
				{ NDO_DATA_HOSTEVENTHANDLERENABLED, BD_INT, 
						{ .integer = temp_host->event_handler_enabled }},
				{ NDO_DATA_RETAINHOSTSTATUSINFORMATION, BD_INT, 
						{ .integer = temp_host->retain_status_information }},
				{ NDO_DATA_RETAINHOSTNONSTATUSINFORMATION, BD_INT, 
						{ .integer = temp_host->retain_nonstatus_information }},
				{ NDO_DATA_HOSTNOTIFICATIONSENABLED, BD_INT, 
						{ .integer = temp_host->notifications_enabled }},
				{ NDO_DATA_HOSTFAILUREPREDICTIONENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				0
#else
			 				temp_host->failure_prediction_enabled
#endif
						}},
				{ NDO_DATA_OBSESSOVERHOST, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_host->obsess
#else
			 				temp_host->obsess_over_host
#endif
						}},
				{ NDO_DATA_NOTES, BD_STRING, 
						{ .string = (es[8]==NULL) ? "" : es[8] }},
				{ NDO_DATA_NOTESURL, BD_STRING, 
						{ .string = (es[9]==NULL) ? "" : es[9] }},
				{ NDO_DATA_ACTIONURL, BD_STRING, 
						{ .string = (es[10]==NULL) ? "" : es[10] }},
				{ NDO_DATA_ICONIMAGE, BD_STRING, 
						{ .string = (es[11]==NULL) ? "" : es[11] }},
				{ NDO_DATA_ICONIMAGEALT, BD_STRING, 
						{ .string = (es[12]==NULL) ? "" : es[12] }},
				{ NDO_DATA_VRMLIMAGE, BD_STRING, 
						{ .string = (es[13]==NULL) ? "" : es[13] }},
				{ NDO_DATA_STATUSMAPIMAGE, BD_STRING, 
						{ .string = (es[14]==NULL) ? "" : es[14] }},
				{ NDO_DATA_HAVE2DCOORDS, BD_INT, { .integer = have_2d_coords }},
				{ NDO_DATA_X2D, BD_INT, { .integer = x_2d }},
				{ NDO_DATA_Y2D, BD_INT, { .integer = y_2d }},
				{ NDO_DATA_HAVE3DCOORDS, BD_INT, { .integer = have_3d_coords }},
				{ NDO_DATA_X3D, BD_FLOAT, { .floating_point = x_3d }},
				{ NDO_DATA_Y3D, BD_FLOAT, { .floating_point = y_3d }},
				{ NDO_DATA_Z3D, BD_FLOAT, { .floating_point = z_3d }},
#ifdef BUILD_NAGIOS_4X
				{ NDO_DATA_IMPORTANCE, BD_INT, 
						{ .integer = temp_host->hourly_value }},
#endif
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTDEFINITION, 
					host_definition, sizeof(host_definition) / 
					sizeof(host_definition[ 0]), FALSE);
		}

		for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) my_free(es[x]);

		/* dump parent hosts */
		ndomod_hosts_serialize(temp_host->parent_hosts, &dbuf, 
				NDO_DATA_PARENTHOST);

		/* dump contactgroups */
		ndomod_contactgroups_serialize(temp_host->contact_groups, &dbuf);

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_host->contacts, &dbuf, NDO_DATA_CONTACT);
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* dump customvars */
		ndomod_customvars_serialize(temp_host->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump hostgroup config ******/
	for(temp_hostgroup=hostgroup_list;temp_hostgroup!=NULL;temp_hostgroup=temp_hostgroup->next){

		es[0] = ndo_escape_buffer(temp_hostgroup->group_name);
		es[1] = ndo_escape_buffer(temp_hostgroup->alias);

		{
			struct ndo_broker_data hostgroup_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTGROUPNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_HOSTGROUPALIAS, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_HOSTGROUPDEFINITION, 
					hostgroup_definition, sizeof(hostgroup_definition) / 
					sizeof(hostgroup_definition[ 0]), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* dump members for each hostgroup */
#ifdef BUILD_NAGIOS_2X
		ndomod_hosts_serialize_2x(temp_hostgroup->members, &dbuf, 
				NDO_DATA_HOSTGROUPMEMBER);
#else
		ndomod_hosts_serialize(temp_hostgroup->members, &dbuf, 
				NDO_DATA_HOSTGROUPMEMBER);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump service config ******/
	for(temp_service=service_list;temp_service!=NULL;temp_service=temp_service->next){

		es[0]=ndo_escape_buffer(temp_service->host_name);
		es[1]=ndo_escape_buffer(temp_service->description);
#ifdef BUILD_NAGIOS_4X
		es[2]=ndo_escape_buffer(temp_service->check_command);
#else
		es[2]=ndo_escape_buffer(temp_service->service_check_command);
#endif
		es[3]=ndo_escape_buffer(temp_service->event_handler);
		es[4]=ndo_escape_buffer(temp_service->notification_period);
		es[5]=ndo_escape_buffer(temp_service->check_period);
#ifdef BUILD_NAGIOS_4X
		es[6]=ndo_escape_buffer("");
#else
		es[6]=ndo_escape_buffer(temp_service->failure_prediction_options);
#endif
#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		es[7]=ndo_escape_buffer(temp_service->notes);
		es[8]=ndo_escape_buffer(temp_service->notes_url);
		es[9]=ndo_escape_buffer(temp_service->action_url);
		es[10]=ndo_escape_buffer(temp_service->icon_image);
		es[11]=ndo_escape_buffer(temp_service->icon_image_alt);

		first_notification_delay=temp_service->first_notification_delay;
#ifdef BUILD_NAGIOS_4X
		notify_on_service_downtime=flag_isset(temp_service->notification_options,OPT_DOWNTIME);
		flap_detection_on_ok=flag_isset(temp_service->flap_detection_options,OPT_OK);
		flap_detection_on_warning=flag_isset(temp_service->flap_detection_options,OPT_WARNING);
		flap_detection_on_unknown=flag_isset(temp_service->flap_detection_options,OPT_UNKNOWN);
		flap_detection_on_critical=flag_isset(temp_service->flap_detection_options,OPT_CRITICAL);
#else
		notify_on_service_downtime=temp_service->notify_on_downtime;
		flap_detection_on_ok=temp_service->flap_detection_on_ok;
		flap_detection_on_warning=temp_service->flap_detection_on_warning;
		flap_detection_on_unknown=temp_service->flap_detection_on_unknown;
		flap_detection_on_critical=temp_service->flap_detection_on_critical;
#endif
		es[12]=ndo_escape_buffer(temp_service->display_name);
#endif
#ifdef BUILD_NAGIOS_2X
		if((temp_serviceextinfo=find_serviceextinfo(temp_service->host_name,temp_service->description))!=NULL){
			es[7]=ndo_escape_buffer(temp_serviceextinfo->notes);
			es[8]=ndo_escape_buffer(temp_serviceextinfo->notes_url);
			es[9]=ndo_escape_buffer(temp_serviceextinfo->action_url);
			es[10]=ndo_escape_buffer(temp_serviceextinfo->icon_image);
			es[11]=ndo_escape_buffer(temp_serviceextinfo->icon_image_alt);
			}
		else{
			es[7]=NULL;
			es[8]=NULL;
			es[9]=NULL;
			es[10]=NULL;
			es[11]=NULL;
			}

		first_notification_delay=0.0;
		notify_on_service_downtime=0;
		flap_detection_on_ok=1;
		flap_detection_on_warning=1;
		flap_detection_on_unknown=1;
		flap_detection_on_critical=1;
		es[12]=ndo_escape_buffer(temp_service->description);
#endif

		{
			struct ndo_broker_data service_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING,
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_DISPLAYNAME, BD_STRING, 
						{ .string = (es[12]==NULL) ? "" : es[12] }},
				{ NDO_DATA_SERVICEDESCRIPTION, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_SERVICECHECKCOMMAND, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_SERVICEEVENTHANDLER, BD_STRING, 
						{ .string = (es[3]==NULL) ? "" : es[3] }},
				{ NDO_DATA_SERVICENOTIFICATIONPERIOD, BD_STRING, 
						{ .string = (es[4]==NULL) ? "" : es[4] }},
				{ NDO_DATA_SERVICECHECKPERIOD, BD_STRING, 
						{ .string = (es[5]==NULL) ? "" : es[5] }},
				{ NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS, BD_STRING, 
						{ .string = (es[6]==NULL) ? "" : es[6] }},
				{ NDO_DATA_SERVICECHECKINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->check_interval }},
				{ NDO_DATA_SERVICERETRYINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->retry_interval }},
				{ NDO_DATA_MAXSERVICECHECKATTEMPTS, BD_INT, 
						{ .integer = temp_service->max_attempts }},
				{ NDO_DATA_FIRSTNOTIFICATIONDELAY, BD_FLOAT, 
						{ .floating_point = first_notification_delay }},
				{ NDO_DATA_SERVICENOTIFICATIONINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_service->notification_interval }},
				{ NDO_DATA_NOTIFYSERVICEUNKNOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 			flag_isset(temp_service->notification_options,
						OPT_UNKNOWN)
#else
			 			temp_service->notify_on_unknown
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEWARNING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 			flag_isset(temp_service->notification_options,
						OPT_WARNING)
#else
			 			temp_service->notify_on_warning
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICECRITICAL, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 			flag_isset(temp_service->notification_options,
						OPT_CRITICAL)
#else
			 			temp_service->notify_on_critical
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICERECOVERY, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 			flag_isset(temp_service->notification_options,
						OPT_RECOVERY)
#else
			 			temp_service->notify_on_recovery
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEFLAPPING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 			flag_isset(temp_service->notification_options,
						OPT_FLAPPING)
#else
			 			temp_service->notify_on_flapping
#endif
						}},
				{ NDO_DATA_NOTIFYSERVICEDOWNTIME, BD_INT, 
						{ .integer = notify_on_service_downtime }},
				{ NDO_DATA_STALKSERVICEONOK, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_service->stalking_options,
							OPT_OK)
#else
			 				temp_service->stalk_on_ok
#endif
						}},
				{ NDO_DATA_STALKSERVICEONWARNING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_service->stalking_options,
							OPT_WARNING)
#else
			 				temp_service->stalk_on_warning
#endif
						}},
				{ NDO_DATA_STALKSERVICEONUNKNOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_service->stalking_options,
							OPT_UNKNOWN)
#else
			 				temp_service->stalk_on_unknown
#endif
						}},
				{ NDO_DATA_STALKSERVICEONCRITICAL, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_service->stalking_options,
							OPT_CRITICAL)
#else
			 				temp_service->stalk_on_critical
#endif
						}},
				{ NDO_DATA_SERVICEISVOLATILE, BD_INT, 
						{ .integer = temp_service->is_volatile }},
				{ NDO_DATA_SERVICEFLAPDETECTIONENABLED, BD_INT, 
						{ .integer = temp_service->flap_detection_enabled }},
				{ NDO_DATA_FLAPDETECTIONONOK, BD_INT, 
						{ .integer = flap_detection_on_ok }},
				{ NDO_DATA_FLAPDETECTIONONWARNING, BD_INT, 
						{ .integer = flap_detection_on_warning }},
				{ NDO_DATA_FLAPDETECTIONONUNKNOWN, BD_INT, 
						{ .integer = flap_detection_on_unknown }},
				{ NDO_DATA_FLAPDETECTIONONCRITICAL, BD_INT, 
						{ .integer = flap_detection_on_critical }},
				{ NDO_DATA_LOWSERVICEFLAPTHRESHOLD, BD_FLOAT, 
						{ .floating_point = temp_service->low_flap_threshold }},
				{ NDO_DATA_HIGHSERVICEFLAPTHRESHOLD, BD_FLOAT, 
						{ .floating_point = 
						temp_service->high_flap_threshold }},
				{ NDO_DATA_PROCESSSERVICEPERFORMANCEDATA, BD_INT, 
						{ .integer = temp_service->process_performance_data }},
				{ NDO_DATA_SERVICEFRESHNESSCHECKSENABLED, BD_INT, 
						{ .integer = temp_service->check_freshness }},
				{ NDO_DATA_SERVICEFRESHNESSTHRESHOLD, BD_INT, 
						{ .integer = temp_service->freshness_threshold }},
				{ NDO_DATA_PASSIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_service->accept_passive_checks
#else
			 				temp_service->accept_passive_service_checks
#endif
						}},
				{ NDO_DATA_SERVICEEVENTHANDLERENABLED, BD_INT, 
						{ .integer = temp_service->event_handler_enabled }},
				{ NDO_DATA_ACTIVESERVICECHECKSENABLED, BD_INT, 
						{ .integer = temp_service->checks_enabled }},
				{ NDO_DATA_RETAINSERVICESTATUSINFORMATION, BD_INT, 
						{ .integer = temp_service->retain_status_information }},
				{ NDO_DATA_RETAINSERVICENONSTATUSINFORMATION, BD_INT, 
						{ .integer = 
						temp_service->retain_nonstatus_information }},
				{ NDO_DATA_SERVICENOTIFICATIONSENABLED, BD_INT, 
						{ .integer = temp_service->notifications_enabled }},
				{ NDO_DATA_OBSESSOVERSERVICE, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				temp_service->obsess
#else
			 				temp_service->obsess_over_service
#endif
						}},
				{ NDO_DATA_FAILUREPREDICTIONENABLED, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				0
#else
			 				temp_service->failure_prediction_enabled
#endif
						}},
				{ NDO_DATA_NOTES, BD_STRING, 
						{ .string = (es[7]==NULL) ? "" : es[7] }},
				{ NDO_DATA_NOTESURL, BD_STRING, 
						{ .string = (es[8]==NULL) ? "" : es[8] }},
				{ NDO_DATA_ACTIONURL, BD_STRING, 
						{ .string = (es[9]==NULL) ? "" : es[9] }},
				{ NDO_DATA_ICONIMAGE, BD_STRING, 
						{ .string = (es[10]==NULL) ? "" : es[10] }},
				{ NDO_DATA_ICONIMAGEALT, BD_STRING, 
						{ .string = (es[11]==NULL) ? "" : es[11] }},
#ifdef BUILD_NAGIOS_4X
				{ NDO_DATA_IMPORTANCE, BD_INT, 
						{ .integer = temp_service->hourly_value }},
#endif
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEDEFINITION, 
					service_definition, sizeof(service_definition) / 
					sizeof(service_definition[ 0]), FALSE);
		}

		/* Free the memory we allocated on this iteration. */
		for (x = 0; x < OBJECTCONFIG_ES_ITEMS; x++) my_free(es[x]);

#ifdef BUILD_NAGIOS_4X
		/* dump parent services */
		ndomod_services_serialize(temp_service->parents, &dbuf, 
				NDO_DATA_PARENTSERVICE);
#endif

		/* dump contactgroups */
		ndomod_contactgroups_serialize(temp_service->contact_groups, &dbuf);

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_service->contacts, &dbuf, 
				NDO_DATA_CONTACT);
#endif

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		/* dump customvars */
		ndomod_customvars_serialize(temp_service->custom_variables, &dbuf);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump servicegroup config ******/
	for(temp_servicegroup=servicegroup_list;temp_servicegroup!=NULL;temp_servicegroup=temp_servicegroup->next){

		es[0]=ndo_escape_buffer(temp_servicegroup->group_name);
		es[1]=ndo_escape_buffer(temp_servicegroup->alias);

		{
			struct ndo_broker_data servicegroup_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_SERVICEGROUPNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_SERVICEGROUPALIAS, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				};

			ndomod_broker_data_serialize(&dbuf, NDO_API_SERVICEGROUPDEFINITION, 
					servicegroup_definition, sizeof(servicegroup_definition) / 
					sizeof(servicegroup_definition[ 0]), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* dump members for each servicegroup */
		ndomod_services_serialize(temp_servicegroup->members, &dbuf, 
				NDO_DATA_SERVICEGROUPMEMBER);

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump host escalation config ******/
#ifdef BUILD_NAGIOS_4X
	for(x=0; x<(int)num_objects.hostescalations; x++) {
		temp_hostescalation=hostescalation_ary[ x];
#else
	for(temp_hostescalation=hostescalation_list;temp_hostescalation!=NULL;temp_hostescalation=temp_hostescalation->next){
#endif
		es[0]=ndo_escape_buffer(temp_hostescalation->host_name);
		es[1]=ndo_escape_buffer(temp_hostescalation->escalation_period);

		{
			struct ndo_broker_data hostescalation_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_ESCALATIONPERIOD, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_FIRSTNOTIFICATION, BD_INT, 
						{ .integer = temp_hostescalation->first_notification }},
				{ NDO_DATA_LASTNOTIFICATION, BD_INT, 
						{ .integer = temp_hostescalation->last_notification }},
				{ NDO_DATA_NOTIFICATIONINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_hostescalation->notification_interval }},
				{ NDO_DATA_ESCALATEONRECOVERY, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostescalation->escalation_options,
							OPT_RECOVERY)
#else
			 				temp_hostescalation->escalate_on_recovery
#endif
						}},
				{ NDO_DATA_ESCALATEONDOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostescalation->escalation_options,
							OPT_DOWN)
#else
			 				temp_hostescalation->escalate_on_down
#endif
						}},
				{ NDO_DATA_ESCALATEONUNREACHABLE, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostescalation->escalation_options,
							OPT_UNREACHABLE)
#else
			 				temp_hostescalation->escalate_on_unreachable
#endif
						}},
				};

			ndomod_broker_data_serialize(&dbuf, 
					NDO_API_HOSTESCALATIONDEFINITION, 
					hostescalation_definition, 
					sizeof(hostescalation_definition) / 
					sizeof(hostescalation_definition[ 0]), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);

		/* dump contactgroups */
		ndomod_contactgroups_serialize(temp_hostescalation->contact_groups, 
				&dbuf);

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_hostescalation->contacts, &dbuf, 
				NDO_DATA_CONTACT);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump service escalation config ******/
#ifdef BUILD_NAGIOS_4X
	for(x=0; x<(int)num_objects.serviceescalations; x++) {
		temp_serviceescalation=serviceescalation_ary[ x];
#else
	for(temp_serviceescalation=serviceescalation_list;temp_serviceescalation!=NULL;temp_serviceescalation=temp_serviceescalation->next){
#endif

		es[0]=ndo_escape_buffer(temp_serviceescalation->host_name);
		es[1]=ndo_escape_buffer(temp_serviceescalation->description);
		es[2]=ndo_escape_buffer(temp_serviceescalation->escalation_period);

		{
			struct ndo_broker_data serviceescalation_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_SERVICEDESCRIPTION, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_ESCALATIONPERIOD, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_FIRSTNOTIFICATION, BD_INT, 
						{ .integer = 
						temp_serviceescalation->first_notification }},
				{ NDO_DATA_LASTNOTIFICATION, BD_INT, 
						{ .integer = 
						temp_serviceescalation->last_notification }},
				{ NDO_DATA_NOTIFICATIONINTERVAL, BD_FLOAT, 
						{ .floating_point = 
						(double)temp_serviceescalation->notification_interval }},
				{ NDO_DATA_ESCALATEONRECOVERY, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_serviceescalation->escalation_options,
							OPT_RECOVERY)
#else
			 				temp_serviceescalation->escalate_on_recovery
#endif
						}},
				{ NDO_DATA_ESCALATEONWARNING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_serviceescalation->escalation_options,
							OPT_WARNING)
#else
			 				temp_serviceescalation->escalate_on_warning
#endif
						}},

				{ NDO_DATA_ESCALATEONUNKNOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_serviceescalation->escalation_options,
							OPT_UNKNOWN)
#else
			 				temp_serviceescalation->escalate_on_unknown
#endif
						}},
				{ NDO_DATA_ESCALATEONCRITICAL, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_serviceescalation->escalation_options,
							OPT_CRITICAL)
#else
			 				temp_serviceescalation->escalate_on_critical
#endif
						}},
				};

			ndomod_broker_data_serialize(&dbuf, 
					NDO_API_SERVICEESCALATIONDEFINITION, 
					serviceescalation_definition, 
					sizeof(serviceescalation_definition) / 
					sizeof(serviceescalation_definition[ 0]), FALSE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);
		if (es[2]) free(es[2]);

		/* dump contactgroups */
		ndomod_contactgroups_serialize(temp_serviceescalation->contact_groups, 
				&dbuf);

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		ndomod_contacts_serialize(temp_serviceescalation->contacts, &dbuf, 
				NDO_DATA_CONTACT);
#endif

		ndomod_enddata_serialize(&dbuf);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump host dependency config ******/
#ifdef BUILD_NAGIOS_4X
	for(x=0; x<(int)num_objects.hostdependencies; x++) {
		temp_hostdependency=hostdependency_ary[ x];
#else
	for(temp_hostdependency=hostdependency_list;temp_hostdependency!=NULL;temp_hostdependency=temp_hostdependency->next){
#endif

		es[0]=ndo_escape_buffer(temp_hostdependency->host_name);
		es[1]=ndo_escape_buffer(temp_hostdependency->dependent_host_name);

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		es[2]=ndo_escape_buffer(temp_hostdependency->dependency_period);
#endif
#ifdef BUILD_NAGIOS_2X
		es[2]=NULL;
#endif

		{
			struct ndo_broker_data hostdependency_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_DEPENDENTHOSTNAME, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_DEPENDENCYTYPE, BD_INT, 
						{ .integer = temp_hostdependency->dependency_type }},
				{ NDO_DATA_INHERITSPARENT, BD_INT, 
						{ .integer = temp_hostdependency->inherits_parent }},
				{ NDO_DATA_DEPENDENCYPERIOD, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_FAILONUP, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostdependency->failure_options,
							OPT_UP)
#else
			 				temp_hostdependency->fail_on_up
#endif
						}},
				{ NDO_DATA_FAILONDOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostdependency->failure_options,
							OPT_DOWN)
#else
			 				temp_hostdependency->fail_on_down
#endif
						}},
				{ NDO_DATA_FAILONUNREACHABLE, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_hostdependency->failure_options,
							OPT_UNREACHABLE)
#else
			 				temp_hostdependency->fail_on_unreachable
#endif
						}},
				};

			ndomod_broker_data_serialize(&dbuf, 
					NDO_API_HOSTDEPENDENCYDEFINITION, 
					hostdependency_definition, 
					sizeof(hostdependency_definition) / 
					sizeof(hostdependency_definition[ 0]), TRUE);
		}

		if (es[0]) free(es[0]);
		if (es[1]) free(es[1]);
		if (es[2]) free(es[2]);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	/****** dump service dependency config ******/
#ifdef BUILD_NAGIOS_4X
	for(x=0; x<(int)num_objects.servicedependencies; x++) {
		temp_servicedependency=servicedependency_ary[ x];
#else
	for(temp_servicedependency=servicedependency_list;temp_servicedependency!=NULL;temp_servicedependency=temp_servicedependency->next){
#endif

		es[0]=ndo_escape_buffer(temp_servicedependency->host_name);
		es[1]=ndo_escape_buffer(temp_servicedependency->service_description);
		es[2]=ndo_escape_buffer(temp_servicedependency->dependent_host_name);
		es[3]=ndo_escape_buffer(temp_servicedependency->dependent_service_description);

#if ( defined( BUILD_NAGIOS_3X) || defined( BUILD_NAGIOS_4X))
		es[4]=ndo_escape_buffer(temp_servicedependency->dependency_period);
#endif
#ifdef BUILD_NAGIOS_2X
		es[4]=NULL;
#endif

		{
			struct ndo_broker_data servicedependency_definition[] = {
				{ NDO_DATA_TIMESTAMP, BD_TIMEVAL, 
						{ .timestamp = now }},
				{ NDO_DATA_HOSTNAME, BD_STRING, 
						{ .string = (es[0]==NULL) ? "" : es[0] }},
				{ NDO_DATA_SERVICEDESCRIPTION, BD_STRING, 
						{ .string = (es[1]==NULL) ? "" : es[1] }},
				{ NDO_DATA_DEPENDENTHOSTNAME, BD_STRING, 
						{ .string = (es[2]==NULL) ? "" : es[2] }},
				{ NDO_DATA_DEPENDENTSERVICEDESCRIPTION, BD_STRING, 
						{ .string = (es[3]==NULL) ? "" : es[3] }},
				{ NDO_DATA_DEPENDENCYTYPE, BD_INT, 
						{ .integer = temp_servicedependency->dependency_type }},
				{ NDO_DATA_INHERITSPARENT, BD_INT, 
						{ .integer = temp_servicedependency->inherits_parent }},
				{ NDO_DATA_DEPENDENCYPERIOD, BD_STRING, 
						{ .string = (es[4]==NULL) ? "" : es[4] }},
				{ NDO_DATA_FAILONOK, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_servicedependency->failure_options,
							OPT_OK)
#else
			 				temp_servicedependency->fail_on_ok
#endif
						}},
				{ NDO_DATA_FAILONWARNING, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_servicedependency->failure_options,
							OPT_WARNING)
#else
			 				temp_servicedependency->fail_on_warning
#endif
						}},
				{ NDO_DATA_FAILONUNKNOWN, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_servicedependency->failure_options,
							OPT_UNKNOWN)
#else
			 				temp_servicedependency->fail_on_unknown
#endif
						}},
				{ NDO_DATA_FAILONCRITICAL, BD_INT, 
						{ .integer =
#ifdef BUILD_NAGIOS_4X
			 				flag_isset(temp_servicedependency->failure_options,
							OPT_CRITICAL)
#else
			 				temp_servicedependency->fail_on_critical
#endif
						}},
				};

			ndomod_broker_data_serialize(&dbuf, 
					NDO_API_SERVICEDEPENDENCYDEFINITION, 
					servicedependency_definition, 
					sizeof(servicedependency_definition) / 
					sizeof(servicedependency_definition[ 0]), TRUE);
		}

		/* Free the memory we allocated on this iteration. */
		for (x = 0; x < 5; x++) if (es[x]) free(es[x]);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	}


	return NDO_OK;
}



/* dumps config files to data sink */
int ndomod_write_config_files(void){
	int result=NDO_OK;

	if((result=ndomod_write_main_config_file())==NDO_ERROR)
		return NDO_ERROR;

	if((result=ndomod_write_resource_config_files())==NDO_ERROR)
		return NDO_ERROR;

	return result;
        }



/* dumps main config file data to sink */
int ndomod_write_main_config_file(void){
	char fbuf[NDOMOD_MAX_BUFLEN];
	char *temp_buffer;
	struct timeval now;
	FILE *fp;
	char *var=NULL;
	char *val=NULL;

	/* get current time */
	gettimeofday(&now,NULL);

	asprintf(&temp_buffer
		 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n"
		 ,NDO_API_MAINCONFIGFILEVARIABLES
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_DATA_CONFIGFILENAME
		 ,config_file
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write each var/val pair from config file */
	if((fp=fopen(config_file,"r"))){

		while((fgets(fbuf,sizeof(fbuf),fp))){

			/* skip blank lines */
			if(fbuf[0]=='\x0' || fbuf[0]=='\n' || fbuf[0]=='\r')
				continue;

			strip(fbuf);

			/* skip comments */
			if(fbuf[0]=='#' || fbuf[0]==';')
				continue;

			if((var=strtok(fbuf,"="))==NULL)
				continue;
			val=strtok(NULL,"\n");

			asprintf(&temp_buffer
				 ,"%d=%s=%s\n"
				 ,NDO_DATA_CONFIGFILEVARIABLE
				 ,var
				 ,(val==NULL)?"":val
				);
			ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
			free(temp_buffer);
			temp_buffer=NULL;
		        }

		fclose(fp);
	        }

	ndomod_write_to_sink(STRINGIFY(NDO_API_ENDDATA)"\n\n", NDO_TRUE, NDO_TRUE);

	return NDO_OK;
        }



/* dumps all resource config files to sink */
int ndomod_write_resource_config_files(void){

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	return NDO_OK;
        }



/* dumps a single resource config file to sink */
int ndomod_write_resource_config_file(const char *filename){
	(void)filename; /* Unused, don't warn. */

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	return NDO_OK;
        }



/* dumps runtime variables to sink */
int ndomod_write_runtime_variables(void){
	char *temp_buffer=NULL;
	struct timeval now;

	/* get current time */
	gettimeofday(&now,NULL);

	asprintf(&temp_buffer
		 ,"\n%d:\n%d=%ld.%ld\n"
		 ,NDO_API_RUNTIMEVARIABLES
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write out main config file name */
	asprintf(&temp_buffer
		 ,"%d=%s=%s\n"
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"config_file"
		 ,config_file
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write out vars determined after startup */
	asprintf(&temp_buffer
		 ,"%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lu\n%d=%s=%lu\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n"
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_services"
		 ,scheduling_info.total_services
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_scheduled_services"
		 ,scheduling_info.total_scheduled_services
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_hosts"
		 ,scheduling_info.total_hosts
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_scheduled_hosts"
		 ,scheduling_info.total_scheduled_hosts
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_services_per_host"
		 ,scheduling_info.average_services_per_host
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_scheduled_services_per_host"
		 ,scheduling_info.average_scheduled_services_per_host
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_check_interval_total"
		 ,scheduling_info.service_check_interval_total
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"host_check_interval_total"
		 ,scheduling_info.host_check_interval_total
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_service_check_interval"
		 ,scheduling_info.average_service_check_interval
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_host_check_interval"
		 ,scheduling_info.average_host_check_interval
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_service_inter_check_delay"
		 ,scheduling_info.average_service_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_host_inter_check_delay"
		 ,scheduling_info.average_host_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_inter_check_delay"
		 ,scheduling_info.service_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"host_inter_check_delay"
		 ,scheduling_info.host_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_interleave_factor"
		 ,scheduling_info.service_interleave_factor
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"max_service_check_spread"
		 ,scheduling_info.max_service_check_spread
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"max_host_check_spread"
		 ,scheduling_info.max_host_check_spread
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	ndomod_write_to_sink(STRINGIFY(NDO_API_ENDDATA)"\n\n", NDO_TRUE, NDO_TRUE);

	return NDO_OK;
}
