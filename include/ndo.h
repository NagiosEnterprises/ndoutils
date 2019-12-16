#ifndef NDO_H
#define NDO_H


/* include important headers */
#include "../include/config.h"
#include "../include/nagios/objects.h"
#include <mysql.h>

/* ndo specifics */
#define DEFAULT_STARTUP_HASH_SCRIPT_PATH "/usr/local/nagios/bin/ndo-startup-hash.sh"
#define MAX_OBJECT_INSERT 100
#define MAX_SQL_BINDINGS (MAX_OBJECT_INSERT * 50)
#define MAX_BIND_BUFFER BUFSZ_XXL
/* any given query_values string (in startup funcs) is ~120 chars
   and the query_base + query_on_update longest string (write_services) is ~4k
   so we pad a bit and hope we never go over this */
#define MAX_SQL_BUFFER ((MAX_OBJECT_INSERT * 150) + 8000)

/* standard macros */
#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define NDO_OK 0
#define NDO_ERROR -1

#ifndef TRUE
#   define TRUE 1
#elif TRUE != 1
#   undef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#elif FALSE != 0
#   undef FALSE
#   define FALSE 0
#endif


/* helper macros */
/* only use STRLIT_LEN on a char array, not a pointer */
#define STRLIT_LEN(s) ((sizeof(s) / sizeof(s[0])) - 1)
#define UPDATE_QUERY_X_POS(_query, _cursor, _xpos, _cond) _query[_cursor + _xpos] = (_cond ? '1' : '0')


/* generic buffer sizes */
#define BUFSZ_TINY  (1 << 6)  /* 64 */
#define BUFSZ_SMOL  (1 << 7)  /* 128 */
#define BUFSZ_MED   (1 << 8)  /* 256 */
#define BUFSZ_LARGE (1 << 10) /* 1024 */
#define BUFSZ_XL    (1 << 11) /* 2048 */
#define BUFSZ_XXL   (1 << 12) /* 4096 */
#define BUFSZ_EPIC  (1 << 13) /* 8192 */


/* debugging/trace logging */
#ifdef DEBUG
# define TRACE(fmt, ...) ndo_debug(TRUE, fmt, __VA_ARGS__)
#else
# define TRACE(fmt, ...) (void)0
#endif

#define trace(fmt, ...) TRACE("%s():%d - " fmt, __func__, __LINE__, __VA_ARGS__)
#define trace_info(msg) trace("%s", msg)

#define trace_func_void() \
do { \
    trace("%s", "begin function (void args)"); \
    ndo_debug_stack_frames++; \
} while (0)

#define trace_func_args(fmt, ...) \
do { \
    trace(fmt, __VA_ARGS__); \
    ndo_debug_stack_frames++; \
} while (0)

#define trace_func_handler(_struct) \
do { \
    trace("type=%d, data(type=%d,f=%d,a=%d,t=%ld.%06ld)", type, ((nebstruct_## _struct ##_data *)d)->type, ((nebstruct_## _struct ##_data *)d)->flags, ((nebstruct_## _struct ##_data *)d)->attr, ((nebstruct_## _struct ##_data *)d)->timestamp.tv_sec, ((nebstruct_## _struct ##_data *)d)->timestamp.tv_usec); \
    ndo_debug_stack_frames++; \
} while (0)

#define trace_return_void() \
do { \
    ndo_debug_stack_frames--; \
    trace("%s", "returning void"); \
    return; \
} while (0)

#define trace_return_void_cond(condition) \
do { \
    ndo_debug_stack_frames--; \
    trace("(%s), returning void", condition); \
    return; \
} while (0)

#define trace_return_error() \
do { \
    ndo_debug_stack_frames--; \
    trace("%s", "returning ERROR"); \
    return NDO_ERROR; \
} while (0)

#define trace_return_error_cond(condition) \
do { \
    ndo_debug_stack_frames--; \
    trace("(%s), returning ERROR", condition); \
    return NDO_ERROR; \
} while (0)

#define trace_return_ok() \
do { \
    ndo_debug_stack_frames--; \
    trace("%s", "returning OK"); \
    return NDO_OK; \
} while (0)

#define trace_return_ok_cond(condition) \
do { \
    ndo_debug_stack_frames--; \
    trace("(%s), returning OK", condition); \
    return NDO_OK; \
} while (0)

#define trace_return_null() \
do { \
    ndo_debug_stack_frames--; \
    trace("%s", "returning NULL"); \
    return NULL; \
} while (0)

#define trace_return_null_cond(condition) \
do { \
    ndo_debug_stack_frames--; \
    trace("(%s), returning NULL", condition); \
    return NULL; \
} while (0)

#define trace_return(fmtid, value) \
do { \
    ndo_debug_stack_frames--; \
    trace("returning with value: " fmtid, value); \
    return value; \
} while (0)


/* error logging */
#define NDO_REPORT_ERROR(err) \
do { \
    snprintf(ndo_error_msg, BUFSZ_LARGE - 1, "%s(%s:%d): %s", __func__, __FILE__, __LINE__, err); \
    ndo_log(ndo_error_msg); \
} while (0)

/* Utility statements - these exist on both connections */
#define GENERIC 0
#define GET_OBJECT_ID_NAME1 1
#define GET_OBJECT_ID_NAME2 2
#define INSERT_OBJECT_ID_NAME1 3
#define INSERT_OBJECT_ID_NAME2 4
/* handler statements - used on both main thread and on startup thread */
#define HANDLE_LOG_DATA 5
#define HANDLE_PROCESS 6
#define HANDLE_PROCESS_SHUTDOWN 7
#define HANDLE_TIMEDEVENT_ADD 8
#define HANDLE_TIMEDEVENT_REMOVE 9
#define HANDLE_TIMEDEVENT_EXECUTE 10
#define HANDLE_SYSTEM_COMMAND 11
#define HANDLE_EVENT_HANDLER 12
#define HANDLE_HOST_CHECK 13
#define HANDLE_SERVICE_CHECK 14
#define HANDLE_COMMENT_ADD 15
#define HANDLE_COMMENT_HISTORY_ADD 16
#define HANDLE_COMMENT_DELETE 17
#define HANDLE_COMMENT_HISTORY_DELETE 18
#define HANDLE_DOWNTIME_ADD 19
#define HANDLE_DOWNTIME_HISTORY_ADD 20
#define HANDLE_DOWNTIME_START 21
#define HANDLE_DOWNTIME_HISTORY_START 22
#define HANDLE_DOWNTIME_STOP 23
#define HANDLE_DOWNTIME_HISTORY_STOP 24
#define HANDLE_FLAPPING 25
#define HANDLE_PROGRAM_STATUS 26
#define HANDLE_HOST_STATUS 27
#define HANDLE_SERVICE_STATUS 28
#define HANDLE_CONTACT_STATUS 29
#define HANDLE_NOTIFICATION 30
#define HANDLE_CONTACT_NOTIFICATION 31
#define HANDLE_CONTACT_NOTIFICATION_METHOD 32
#define HANDLE_EXTERNAL_COMMAND 33
#define HANDLE_ACKNOWLEDGEMENT 34
#define HANDLE_STATE_CHANGE 35
#define HANDLE_OBJECT_WRITING 36
/* statements for startup thread only */
#define WRITE_HANDLE_OBJECT_WRITING 37
#define WRITE_ACTIVE_OBJECTS 38
#define WRITE_CUSTOMVARS 39
#define WRITE_CONTACT_ADDRESSES 40
#define WRITE_CONTACT_NOTIFICATIONCOMMANDS 41
#define WRITE_HOST_PARENTHOSTS 42
#define WRITE_HOST_CONTACTGROUPS 43
#define WRITE_HOST_CONTACTS 44
#define WRITE_SERVICE_PARENTSERVICES 45
#define WRITE_SERVICE_CONTACTGROUPS 46
#define WRITE_SERVICE_CONTACTS 47
#define WRITE_CONTACTS 48
#define WRITE_HOSTS 49
#define WRITE_SERVICES 50
#define WRITE_CONFIG 51

/* WRITE queries are only created during invocation, whereas handler queries are allocated near the beginning of each thread */
#define NUM_INITIALIZED_QUERIES 37
#define NUM_QUERIES 52


/* ndo processing information */
#define NDO_PROCESS_PROCESS                         (1 << 0)
#define NDO_PROCESS_TIMED_EVENT                     (1 << 1)
#define NDO_PROCESS_LOG                             (1 << 2)
#define NDO_PROCESS_SYSTEM_COMMAND                  (1 << 3)
#define NDO_PROCESS_EVENT_HANDLER                   (1 << 4)
#define NDO_PROCESS_NOTIFICATION                    (1 << 5)
#define NDO_PROCESS_SERVICE_CHECK                   (1 << 6)
#define NDO_PROCESS_HOST_CHECK                      (1 << 7)
#define NDO_PROCESS_COMMENT                         (1 << 8)
#define NDO_PROCESS_DOWNTIME                        (1 << 9)
#define NDO_PROCESS_FLAPPING                        (1 << 10)
#define NDO_PROCESS_PROGRAM_STATUS                  (1 << 11)
#define NDO_PROCESS_HOST_STATUS                     (1 << 12)
#define NDO_PROCESS_SERVICE_STATUS                  (1 << 13)

#define NDO_PROCESS_EXTERNAL_COMMAND                (1 << 17)
#define NDO_PROCESS_OBJECT_CONFIG                   (1 << 18)
#define NDO_PROCESS_MAIN_CONFIG                     (1 << 19)
#define NDO_PROCESS_RETENTION                       (1 << 21)
#define NDO_PROCESS_ACKNOWLEDGEMENT                 (1 << 22)
#define NDO_PROCESS_STATE_CHANGE                    (1 << 23)
#define NDO_PROCESS_CONTACT_STATUS                  (1 << 24)

#define NDO_PROCESS_CONTACT_NOTIFICATION            (1 << 26)
#define NDO_PROCESS_CONTACT_NOTIFICATION_METHOD     (1 << 27)


/* ndo configuration dumping/processing definitions */
#define NDO_CONFIG_DUMP_NONE     0
#define NDO_CONFIG_DUMP_ORIGINAL 1
#define NDO_CONFIG_DUMP_RETAINED 2
#define NDO_CONFIG_DUMP_ALL      3


/* database objecttype_ids */
#define NDO_OBJECTTYPE_HOST                1
#define NDO_OBJECTTYPE_SERVICE             2 
#define NDO_OBJECTTYPE_HOSTGROUP           3
#define NDO_OBJECTTYPE_SERVICEGROUP        4
#define NDO_OBJECTTYPE_HOSTESCALATION      5
#define NDO_OBJECTTYPE_SERVICEESCALATION   6
#define NDO_OBJECTTYPE_HOSTDEPENDENCY      7
#define NDO_OBJECTTYPE_SERVICEDEPENDENCY   8
#define NDO_OBJECTTYPE_TIMEPERIOD          9
#define NDO_OBJECTTYPE_CONTACT             10
#define NDO_OBJECTTYPE_CONTACTGROUP        11
#define NDO_OBJECTTYPE_COMMAND             12


/* useful for keeping all of one thread's data in
   one place */
typedef struct {
    MYSQL * conn;
    int connected;
    int reconnect_counter;
    char * query[NUM_QUERIES];
    MYSQL_STMT * stmt[NUM_QUERIES];
    MYSQL_BIND * bind[NUM_QUERIES];
    MYSQL_BIND * result[NUM_QUERIES];
    unsigned long * strlen[NUM_QUERIES];
    unsigned long * result_strlen[NUM_QUERIES];
    int bind_i[NUM_QUERIES];
    int result_i[NUM_QUERIES];
} ndo_query_context;

/* nodes for startup handler queueing
   while object definitions are being written to db */
typedef struct queue_node_t {
    struct queue_node_t * next;
    void * data;
    int type;
} queue_node;

void enqueue(queue_node ** head, void * data, int type);
void * dequeue(queue_node ** head, int * type);

/* function declarations */
void initialize_bindings_array();
int initialize_stmt_data();
int deinitialize_stmt_data();

void ndo_log(char * buffer);
void ndo_debug(int write_to_log, const char * fmt, ...);
int nebmodule_init(int flags, char * args, void * handle);
int nebmodule_deinit(int flags, int reason);
int ndo_process_arguments(char * args);
int ndo_initialize_mysql_connection();
int ndo_initialize_database(ndo_query_context *q_ctx);
int ndo_initialize_prepared_statements();
void ndo_disconnect_database(ndo_query_context *q_ctx);

int ndo_register_static_callbacks();
int ndo_register_queue_callbacks();
int ndo_deregister_callbacks();

int ndo_process_file(ndo_query_context *q_ctx, char * file, int (* process_line_cb)(ndo_query_context *q_ctx, char * line));
int ndo_process_file_lines(ndo_query_context *q_ctx, char * contents, int (* process_line_cb)(ndo_query_context *q_ctx, char * line));

int ndo_process_ndo_config_file(char * config_file_contents);
int ndo_process_ndo_config_line(ndo_query_context *q_ctx, char * line);

int ndo_process_nagios_config_line(ndo_query_context *q_ctx, char * line);
int ndo_process_nagios_config(ndo_query_context *q_ctx);

int ndo_handle_process(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_timed_event(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_log(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_system_command(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_event_handler(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_notification(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_service_check(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_host_check(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_comment(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_downtime(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_flapping(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_program_status(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_host_status(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_service_status(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_external_command(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_acknowledgement(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_statechange(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_contact_status(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_contact_notification(ndo_query_context *q_ctx, int type, void * d);
int ndo_handle_contact_notification_method(ndo_query_context *q_ctx, int type, void * d);

int ndo_neb_handle_process(int type, void * d);
int ndo_neb_handle_timed_event(int type, void * d);
int ndo_neb_handle_log(int type, void * d);
int ndo_neb_handle_system_command(int type, void * d);
int ndo_neb_handle_event_handler(int type, void * d);
int ndo_neb_handle_notification(int type, void * d);
int ndo_neb_handle_service_check(int type, void * d);
int ndo_neb_handle_host_check(int type, void * d);
int ndo_neb_handle_comment(int type, void * d);
int ndo_neb_handle_downtime(int type, void * d);
int ndo_neb_handle_flapping(int type, void * d);
int ndo_neb_handle_program_status(int type, void * d);
int ndo_neb_handle_host_status(int type, void * d);
int ndo_neb_handle_service_status(int type, void * d);
int ndo_neb_handle_external_command(int type, void * d);
int ndo_neb_handle_acknowledgement(int type, void * d);
int ndo_neb_handle_statechange(int type, void * d);
int ndo_neb_handle_contact_status(int type, void * d);
int ndo_neb_handle_contact_notification(int type, void * d);
int ndo_neb_handle_contact_notification_method(int type, void * d);

int ndo_handle_queue_timed_event(int type, void * d);
int ndo_handle_queue_event_handler(int type, void * d);
int ndo_handle_queue_host_check(int type, void * d);
int ndo_handle_queue_service_check(int type, void * d);
int ndo_handle_queue_comment(int type, void * d);
int ndo_handle_queue_downtime(int type, void * d);
int ndo_handle_queue_flapping(int type, void * d);
int ndo_handle_queue_host_status(int type, void * d);
int ndo_handle_queue_service_status(int type, void * d);
int ndo_handle_queue_contact_status(int type, void * d);
int ndo_handle_queue_notification(int type, void * d);
int ndo_handle_queue_contact_notification(int type, void * d);
int ndo_handle_queue_contact_notification_method(int type, void * d);
int ndo_handle_queue_acknowledgement(int type, void * d);
int ndo_handle_queue_statechange(int type, void * d);

int ndo_empty_queue_timed_event(ndo_query_context *q_ctx);
int ndo_empty_queue_event_handler(ndo_query_context *q_ctx);
int ndo_empty_queue_host_check(ndo_query_context *q_ctx);
int ndo_empty_queue_service_check(ndo_query_context *q_ctx);
int ndo_empty_queue_comment(ndo_query_context *q_ctx);
int ndo_empty_queue_downtime(ndo_query_context *q_ctx);
int ndo_empty_queue_flapping(ndo_query_context *q_ctx);
int ndo_empty_queue_host_status(ndo_query_context *q_ctx);
int ndo_empty_queue_service_status(ndo_query_context *q_ctx);
int ndo_empty_queue_contact_status(ndo_query_context *q_ctx);
int ndo_empty_queue_acknowledgement(ndo_query_context *q_ctx);
int ndo_empty_queue_statechange(ndo_query_context *q_ctx);
int ndo_empty_queue_notification(ndo_query_context *q_ctx);
int ndo_empty_queue_contact_notification(ndo_query_context *q_ctx);
int ndo_empty_queue_contact_notification_method(ndo_query_context *q_ctx);

int ndo_handle_retention(int type, void * d);

void * ndo_startup_thread(void * args);

int ndo_table_genocide();
int ndo_set_all_objects_inactive();
int ndo_begin_active_objects(int run_count);
int ndo_end_active_objects();
int ndo_set_object_active(int object_id, int config_type, void * next);

int ndo_write_stmt_init();
int ndo_write_stmt_close();

int ndo_write_config_files();
int ndo_write_config(int type);
int ndo_write_runtime_variables();

int ndo_write_object_config(ndo_query_context *q_ctx, int config_type);

int ndo_empty_startup_queues(ndo_query_context *q_ctx);
int ndo_empty_queue_timed_event();

void ndo_process_config_line(char * line);
int ndo_process_config_file();
int ndo_config_sanity_check();
char * ndo_strip(char * s);

int write_to_log(char * buffer, unsigned long l, time_t * t);

void ndo_calculate_startup_hash();

long ndo_get_object_id_name1(ndo_query_context *q_ctx, int insert, int object_type, char * name1);
long ndo_get_object_id_name2(ndo_query_context *q_ctx, int insert, int object_type, char * name1, char * name2);
long ndo_insert_object_id_name1(ndo_query_context *q_ctx, int object_type, char * name1);
long ndo_insert_object_id_name2(ndo_query_context *q_ctx, int object_type, char * name1, char * name2);

int send_subquery(ndo_query_context *q_ctx, int stmt, int * counter, char * query, char * query_on_update, size_t * query_len, size_t query_base_len, size_t query_on_update_len);

int ndo_write_commands(ndo_query_context *q_ctx, int config_type);
int ndo_write_timeperiods(ndo_query_context *q_ctx, int config_type);
int ndo_write_timeperiods(ndo_query_context *q_ctx, int config_type);
int ndo_write_timeperiod_timeranges(ndo_query_context *q_ctx, int * timeperiod_ids);
int ndo_write_contacts(ndo_query_context *q_ctx, int config_type);
int ndo_write_contact_objects(ndo_query_context *q_ctx, int config_type);
int ndo_write_contactgroups(ndo_query_context *q_ctx, int config_type);
int ndo_write_contactgroup_members(ndo_query_context *q_ctx, int * contactgroup_ids);
int ndo_write_hosts(ndo_query_context *q_ctx, int config_type);
int ndo_write_hosts_objects(ndo_query_context *q_ctx, int config_type);
int ndo_write_hostgroups(ndo_query_context *q_ctx, int config_type);
int ndo_write_hostgroup_members(ndo_query_context *q_ctx, int * hostgroup_ids);
int ndo_write_services(ndo_query_context *q_ctx, int config_type);
int ndo_write_services_objects(ndo_query_context *q_ctx, int config_type);
int ndo_write_servicegroups(ndo_query_context *q_ctx, int config_type);
int ndo_write_servicegroup_members(ndo_query_context *q_ctx, int * servicegroup_ids);
int ndo_write_hostescalations(ndo_query_context *q_ctx, int config_type);
int ndo_write_hostescalation_contactgroups(ndo_query_context *q_ctx, int * hostescalation_ids);
int ndo_write_hostescalation_contacts(ndo_query_context *q_ctx, int * hostescalation_ids);
int ndo_write_serviceescalations(ndo_query_context *q_ctx, int config_type);
int ndo_write_serviceescalation_contactgroups(ndo_query_context *q_ctx, int * serviceescalation_ids);
int ndo_write_serviceescalation_contacts(ndo_query_context *q_ctx, int * serviceescalation_ids);
int ndo_write_hostdependencies(ndo_query_context *q_ctx, int config_type);
int ndo_write_servicedependencies(ndo_query_context *q_ctx, int config_type);





#endif /* NDO_H */