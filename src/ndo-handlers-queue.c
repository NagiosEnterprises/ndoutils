
void enqueue(queue_node ** head, void * data, int type)
{
    if (head == NULL) {
        return;
    }

    if (*head == NULL) {
        *head = calloc(1, sizeof(**head));
        (*head)->data = data;
        (*head)->type = type;
        return;
    }

    queue_node * cur = *head;

    while (cur->next != NULL) {
        cur = cur->next;
    }

    cur->next = calloc(1, sizeof(*cur));
    cur->next->next = NULL;
    cur->next->data = data;
    cur->next->type = type;
}


void * dequeue(queue_node ** head, int * type)
{
    if (head == NULL || *head == NULL) {
        *type = -1;
        return NULL;
    }

    void * data = (*head)->data;
    *type = (*head)->type;

    if ((*head)->next == NULL) {
        free(*head);
        *head = NULL;
    }
    else {
        queue_node * cur_head = *head;
        *head = (*head)->next;
        free(cur_head);
    }

    return data;
}

queue_node * nebstruct_queue_timed_event = NULL;
queue_node * nebstruct_queue_event_handler_data = NULL;
queue_node * nebstruct_queue_host_check_data = NULL;
queue_node * nebstruct_queue_service_check_data = NULL;
queue_node * nebstruct_queue_comment_data = NULL;
queue_node * nebstruct_queue_downtime_data = NULL;
queue_node * nebstruct_queue_flapping_data = NULL;
queue_node * nebstruct_queue_host_status_data = NULL;
queue_node * nebstruct_queue_service_status_data = NULL;
queue_node * nebstruct_queue_contact_status_data = NULL;
queue_node * nebstruct_queue_notification = NULL;
queue_node * nebstruct_queue_acknowledgement_data = NULL;
queue_node * nebstruct_queue_state_change = NULL;

pthread_mutex_t queue_timed_event_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_event_handler_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_host_check_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_service_check_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_comment_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_downtime_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_flapping_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_host_status_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_service_status_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_contact_status_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_notification_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_acknowledgement_data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_state_change_mutex = PTHREAD_MUTEX_INITIALIZER;


int ndo_handle_queue_timed_event(int type, void * data)
{
    trace_func_handler(timed_event);
    pthread_mutex_lock(&queue_timed_event_mutex);
    enqueue(&nebstruct_queue_timed_event, data, type);
    pthread_mutex_unlock(&queue_timed_event_mutex);
    trace_return_ok();
}


int ndo_handle_queue_event_handler(int type, void * data)
{
    trace_func_handler(event_handler);
    pthread_mutex_lock(&queue_event_handler_data_mutex);
    enqueue(&nebstruct_queue_event_handler_data, data, type);
    pthread_mutex_unlock(&queue_event_handler_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_check(int type, void * data)
{
    trace_func_handler(host_check);
    pthread_mutex_lock(&queue_host_check_data_mutex);
    enqueue(&nebstruct_queue_host_check_data, data, type);
    pthread_mutex_unlock(&queue_host_check_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_check(int type, void * data)
{
    trace_func_handler(service_check);
    pthread_mutex_lock(&queue_service_check_data_mutex);
    enqueue(&nebstruct_queue_service_check_data, data, type);
    pthread_mutex_unlock(&queue_service_check_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_comment(int type, void * data)
{
    trace_func_handler(comment);
    pthread_mutex_lock(&queue_comment_data_mutex);
    enqueue(&nebstruct_queue_comment_data, data, type);
    pthread_mutex_unlock(&queue_comment_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_downtime(int type, void * data)
{
    trace_func_handler(downtime);
    pthread_mutex_lock(&queue_downtime_data_mutex);
    enqueue(&nebstruct_queue_downtime_data, data, type);
    pthread_mutex_unlock(&queue_downtime_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_flapping(int type, void * data)
{
    trace_func_handler(flapping);
    pthread_mutex_lock(&queue_flapping_data_mutex);
    enqueue(&nebstruct_queue_flapping_data, data, type);
    pthread_mutex_unlock(&queue_flapping_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_status(int type, void * data)
{
    trace_func_handler(host_status);
    pthread_mutex_lock(&queue_host_status_data_mutex);
    enqueue(&nebstruct_queue_host_status_data, data, type);
    pthread_mutex_unlock(&queue_host_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_status(int type, void * data)
{
    trace_func_handler(service_status);
    pthread_mutex_lock(&queue_service_status_data_mutex);
    enqueue(&nebstruct_queue_service_status_data, data, type);
    pthread_mutex_unlock(&queue_service_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_status(int type, void * data)
{
    trace_func_handler(contact_status);
    pthread_mutex_lock(&queue_contact_status_data_mutex);
    enqueue(&nebstruct_queue_contact_status_data, data, type);
    pthread_mutex_unlock(&queue_contact_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_notification(int type, void * data)
{
    trace_func_handler(notification);
    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_notification(int type, void * data)
{
    trace_func_handler(contact_notification);
    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_notification_method(int type, void * data)
{
    trace_func_handler(contact_notification_method);
    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_acknowledgement(int type, void * data)
{
    trace_func_handler(acknowledgement);
    pthread_mutex_lock(&queue_acknowledgement_data_mutex);
    enqueue(&nebstruct_queue_acknowledgement_data, data, type);
    pthread_mutex_unlock(&queue_acknowledgement_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_state_change(int type, void * data)
{
    trace_func_handler(statechange);
    pthread_mutex_lock(&queue_state_change_mutex);
    enqueue(&nebstruct_queue_state_change, data, type);
    pthread_mutex_unlock(&queue_state_change_mutex);
    trace_return_ok();
}
