/**
 * @file ndomod.h Nagios Data Output Event Broker Module declarations
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
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

#ifndef NDOUTILS_INCLUDE_NDOMOD_H
#define NDOUTILS_INCLUDE_NDOMOD_H


/* this is needed for access to daemon's internal data */
#define NSCORE 1

typedef struct ndomod_sink_buffer {
	char **buffer;
	unsigned long head;
	unsigned long tail;
	unsigned long items;
	unsigned long maxitems;
	unsigned long overflow;
} ndomod_sink_buffer;


#define NDOMOD_MAX_BUFLEN   16384


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
#define NDOMOD_PROCESS_STATE_CHANGE_DATA              8388608
#define NDOMOD_PROCESS_CONTACT_STATUS_DATA            16777216
#define NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA          33554432

#define NDOMOD_PROCESS_EVERYTHING                     67108863


#define NDOMOD_CONFIG_DUMP_NONE                       0
#define NDOMOD_CONFIG_DUMP_ORIGINAL                   1
#define NDOMOD_CONFIG_DUMP_RETAINED                   2
#define NDOMOD_CONFIG_DUMP_ALL                        3


int nebmodule_init(int flags, char *args, void *handle);
int nebmodule_deinit(int flags, int reason);

static int ndomod_init(void);
static int ndomod_deinit(void);

static int ndomod_check_nagios_object_version(void);

static int ndomod_printf_to_logs(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static int ndomod_process_module_args(char *args);
static int ndomod_process_config_file(const char *filename);
static int ndomod_process_config_var(char *arg);
static void ndomod_free_config_memory(void);

static int ndomod_open_sink(void);
static int ndomod_close_sink(void);
static int ndomod_write_to_sink(const char *buf, int buffer_write, int flush_buffer);
static int ndomod_rotate_sink_file(void *args);
static int ndomod_hello_sink(int reconnect, int problem_disconnect);
static int ndomod_goodbye_sink(void);

static int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf, unsigned long maxitems);
static int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf);
static int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf, const char *buf);
static char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf);

static int ndomod_load_unprocessed_data(const char *f);
static int ndomod_save_unprocessed_data(const char *f);

static int ndomod_register_callbacks(void);
static int ndomod_deregister_callbacks(void);

static int ndomod_broker_data(int event_type, void *data);

static int ndomod_write_config(int config_type);
static int ndomod_write_object_config(int config_type);

static int ndomod_write_config_files(void);
static int ndomod_write_main_config_file(void);

static int ndomod_write_runtime_variables(void);

#endif
