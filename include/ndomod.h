/************************************************************************
 *
 * NDOMOD.H - NDO NEB Module Include File
 * Copyright (c) 2005-2006 Ethan Galstad
 * Last Modified: 05-25-2006
 *
 ************************************************************************/

#ifndef _NDBXT_NDOMOD_H
#define _NDBXT_NDOMOD_H


/* this is needed for access to daemon's internal data */
#define NSCORE 1

typedef struct ndomod_sink_buffer_struct{
	char **buffer;
	unsigned long size;
	unsigned long head;
	unsigned long tail;
	unsigned long items;
	unsigned long maxitems;
	unsigned long overflow;
        }ndomod_sink_buffer;


#define NDOMOD_MAX_BUFLEN   4096


#define NDOMOD_PROCESS_PROCESS_DATA                   1
#define NDOMOD_PROCESS_TIMED_EVENT_DATA               2
#define NDOMOD_PROCESS_LOG_DATA                       4
#define NDOMOD_PROCESS_SYSTEM_COMMAND_DATA            8
#define NDOMOD_PROCESS_EVENT_HANDLER_DATA             16
#define NDOMOD_PROCESS_NOTIFICATION_DATA              32
#define NDOMOD_PROCESS_SERVICE_CHECK_DATA             64
#define NDOMOD_PROCESS_HOST_CHECK_DATA                128
#define NDOMOD_PROCESS_COMMENT_DATA                   256
#define NDOMOD_PROCESS_DOWNTIME_DATA                  512
#define NDOMOD_PROCESS_FLAPPING_DATA                  1024
#define NDOMOD_PROCESS_PROGRAM_STATUS_DATA            2048
#define NDOMOD_PROCESS_HOST_STATUS_DATA               4096
#define NDOMOD_PROCESS_SERVICE_STATUS_DATA            8192
#define NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA          16384
#define NDOMOD_PROCESS_ADAPTIVE_HOST_DATA             32768
#define NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA          65536
#define NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA          131072
#define NDOMOD_PROCESS_OBJECT_CONFIG_DATA             262144
#define NDOMOD_PROCESS_MAIN_CONFIG_DATA               524288
#define NDOMOD_PROCESS_AGGREGATED_STATUS_DATA         1048576
#define NDOMOD_PROCESS_RETENTION_DATA                 2097152
#define NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA           4194304
#define NDOMOD_PROCESS_STATECHANGE_DATA               8388608
#define NDOMOD_PROCESS_CONTACT_STATUS_DATA            16777216
#define NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA          33554432

#define NDOMOD_PROCESS_EVERYTHING                     67108863


#define NDOMOD_CONFIG_DUMP_NONE                       0
#define NDOMOD_CONFIG_DUMP_ORIGINAL                   1
#define NDOMOD_CONFIG_DUMP_RETAINED                   2
#define NDOMOD_CONFIG_DUMP_ALL                        3


int nebmodule_init(int,char *,void *);
int nebmodule_deinit(int,int);

int ndomod_init(void);
int ndomod_deinit(void);

int ndomod_check_nagios_object_version(void);

int ndomod_write_to_logs(char *,int);

int ndomod_process_module_args(char *);
int ndomod_process_config_var(char *);
int ndomod_process_config_file(char *);

int ndomod_open_sink(void);
int ndomod_close_sink(void);
int ndomod_write_to_sink(char *,int,int);
int ndomod_rotate_sink_file(void *);
int ndomod_hello_sink(int,int);
int ndomod_goodbye_sink(void);

int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf,unsigned long);
int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf);
int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf,char *);
char *ndomod_sink_buffer_peek(ndomod_sink_buffer *sbuf);
char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf);
int ndomod_sink_buffer_items(ndomod_sink_buffer *sbuf);
unsigned long ndomod_sink_buffer_get_overflow(ndomod_sink_buffer *sbuf);
int ndomod_sink_buffer_set_overflow(ndomod_sink_buffer *sbuf,unsigned long);

int ndomod_load_unprocessed_data(char *);
int ndomod_save_unprocessed_data(char *);

int ndomod_register_callbacks(void);
int ndomod_deregister_callbacks(void);

int ndomod_broker_data(int,void *);

int ndomod_write_config(int);
int ndomod_write_object_config(int);

int ndomod_write_config_files(void);
int ndomod_write_main_config_file(void);
int ndomod_write_resource_config_files(void);
int ndomod_write_resource_config_file(char *);

int ndomod_write_runtime_variables(void);

#endif
