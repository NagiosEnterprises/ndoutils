

void ndo_log(char * buffer);
int nebmodule_init(int flags, char * args, void * handle);
int nebmodule_deinit(int flags, int reason);
int ndo_process_arguments(char * args);
int ndo_initialize_database();
int ndo_initialize_prepared_statements();
void ndo_disconnect_database();
int ndo_register_callbacks();
int ndo_deregister_callbacks();


int ndo_handle_process(int type, void * d);
int ndo_handle_timed_event(int type, void * d);
int ndo_handle_log(int type, void * d);
int ndo_handle_system_command(int type, void * d);
int ndo_handle_event_handler(int type, void * d);
int ndo_handle_notification(int type, void * d);
int ndo_handle_service_check(int type, void * d);
int ndo_handle_host_check(int type, void * d);
int ndo_handle_comment(int type, void * d);
int ndo_handle_downtime(int type, void * d);
int ndo_handle_flapping(int type, void * d);
int ndo_handle_program_status(int type, void * d);
int ndo_handle_host_status(int type, void * d);
int ndo_handle_service_status(int type, void * d);
int ndo_handle_external_command(int type, void * d);
int ndo_handle_acknowledgement(int type, void * d);
int ndo_handle_state_change(int type, void * d);
int ndo_handle_contact_status(int type, void * d);
int ndo_handle_contact_notification(int type, void * d);
int ndo_handle_contact_notification_method(int type, void * d);


#define GET_NAME1    0
#define GET_NAME2    1
#define INSERT_NAME1 2
#define INSERT_NAME2 3

int ndo_get_object_id_name1(int insert, int object_type, char * name1);
int ndo_get_object_id_name2(int insert, int object_type, char * name1, char * name2);
int ndo_insert_object_id_name1(int object_type, char * name1);
int ndo_insert_object_id_name2(int object_type, char * name1, char * name2);


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
#define NDO_PROCESS_ACKNOWLEDGEMENT                 (1 << 22)
#define NDO_PROCESS_STATE_CHANGE                    (1 << 23)
#define NDO_PROCESS_CONTACT_STATUS                  (1 << 24)

#define NDO_PROCESS_CONTACT_NOTIFICATION            (1 << 26)
#define NDO_PROCESS_CONTACT_NOTIFICATION_METHOD     (1 << 27)


#define NDO_HANDLE_LOG 3