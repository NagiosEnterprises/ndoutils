
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


void nebstructcpy(void ** dest, const void * src, size_t n)
{
    void * ptr = calloc(1, n);
    memcpy(ptr, src, n);
    *dest = ptr;
}


int ndo_handle_queue_timed_event(int type, void * d)
{
    trace_func_handler(timed_event);
    nebstruct_timed_event_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_timed_event_mutex);
    enqueue(&nebstruct_queue_timed_event, data, type);
    pthread_mutex_unlock(&queue_timed_event_mutex);
    trace_return_ok();
}


int ndo_handle_queue_event_handler(int type, void * d)
{
    trace_func_handler(event_handler);
    nebstruct_event_handler_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_event_handler_data_mutex);
    enqueue(&nebstruct_queue_event_handler_data, data, type);
    pthread_mutex_unlock(&queue_event_handler_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_check(int type, void * d)
{
    trace_func_handler(host_check);
    nebstruct_host_check_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_host_check_data_mutex);
    enqueue(&nebstruct_queue_host_check_data, data, type);
    pthread_mutex_unlock(&queue_host_check_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_check(int type, void * d)
{
    trace_func_handler(service_check);
    nebstruct_service_check_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_service_check_data_mutex);
    enqueue(&nebstruct_queue_service_check_data, data, type);
    pthread_mutex_unlock(&queue_service_check_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_comment(int type, void * d)
{
    trace_func_handler(comment);
    nebstruct_comment_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_comment_data_mutex);
    enqueue(&nebstruct_queue_comment_data, data, type);
    pthread_mutex_unlock(&queue_comment_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_downtime(int type, void * d)
{
    trace_func_handler(downtime);
    nebstruct_downtime_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_downtime_data_mutex);
    enqueue(&nebstruct_queue_downtime_data, data, type);
    pthread_mutex_unlock(&queue_downtime_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_flapping(int type, void * d)
{
    trace_func_handler(flapping);
    nebstruct_flapping_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_flapping_data_mutex);
    enqueue(&nebstruct_queue_flapping_data, data, type);
    pthread_mutex_unlock(&queue_flapping_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_status(int type, void * d)
{
    trace_func_handler(host_status);
    nebstruct_host_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_host_status_data_mutex);
    enqueue(&nebstruct_queue_host_status_data, data, type);
    pthread_mutex_unlock(&queue_host_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_status(int type, void * d)
{
    trace_func_handler(service_status);
    nebstruct_service_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_service_status_data_mutex);
    enqueue(&nebstruct_queue_service_status_data, data, type);
    pthread_mutex_unlock(&queue_service_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_status(int type, void * d)
{
    trace_func_handler(contact_status);
    nebstruct_contact_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_contact_status_data_mutex);
    enqueue(&nebstruct_queue_contact_status_data, data, type);
    pthread_mutex_unlock(&queue_contact_status_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_notification(int type, void * d)
{
    trace_func_handler(notification);
    nebstruct_notification_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    if (data->type == NEBTYPE_NOTIFICATION_START) {
        ndo_notification_
    }

    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_notification(int type, void * d)
{
    trace_func_handler(contact_notification);
    nebstruct_contact_notification_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    if (data->type == NEBTYPE_CONTACTNOTIFICATION_START) {
        ndo_notification_
    }

    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_notification_method(int type, void * d)
{
    trace_func_handler(contact_notification_method);
    nebstruct_contact_notification_method_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    if (data->type == NEBTYPE_CONTACTNOTIFICATIONMETHOD_START) {
        ndo_notification_
    }    

    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_acknowledgement(int type, void * d)
{
    trace_func_handler(acknowledgement);
    nebstruct_acknowledgement_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_acknowledgement_data_mutex);
    enqueue(&nebstruct_queue_acknowledgement_data, data, type);
    pthread_mutex_unlock(&queue_acknowledgement_data_mutex);
    trace_return_ok();
}


int ndo_handle_queue_state_change(int type, void * d)
{
    trace_func_handler(statechange);
    nebstruct_statechange_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_state_change_mutex);
    enqueue(&nebstruct_queue_state_change, data, type);
    pthread_mutex_unlock(&queue_state_change_mutex);
    trace_return_ok();
}
