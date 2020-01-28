
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

char * nebstrdup(char * src)
{
    if (src == NULL) {
        return NULL;
    }
    return strdup(src);
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

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->command_name = nebstrdup(data->command_name);
    data->command_args = nebstrdup(data->command_args);
    data->command_line = nebstrdup(data->command_line);
    data->output = nebstrdup(data->output);

    pthread_mutex_lock(&queue_event_handler_mutex);
    enqueue(&nebstruct_queue_event_handler, data, type);
    pthread_mutex_unlock(&queue_event_handler_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_check(int type, void * d)
{
    trace_func_handler(host_check);
    nebstruct_host_check_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->command_name = nebstrdup(data->command_name);
    data->command_args = nebstrdup(data->command_args);
    data->command_line = nebstrdup(data->command_line);
    data->output = nebstrdup(data->output);
    data->long_output = nebstrdup(data->long_output);
    data->perf_data = nebstrdup(data->perf_data);

    pthread_mutex_lock(&queue_host_check_mutex);
    enqueue(&nebstruct_queue_host_check, data, type);
    pthread_mutex_unlock(&queue_host_check_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_check(int type, void * d)
{
    trace_func_handler(service_check);
    nebstruct_service_check_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->command_name = nebstrdup(data->command_name);
    data->command_args = nebstrdup(data->command_args);
    data->command_line = nebstrdup(data->command_line);

    pthread_mutex_lock(&queue_service_check_mutex);
    enqueue(&nebstruct_queue_service_check, data, type);
    pthread_mutex_unlock(&queue_service_check_mutex);
    trace_return_ok();
}


int ndo_handle_queue_comment(int type, void * d)
{
    trace_func_handler(comment);
    nebstruct_comment_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->author_name = nebstrdup(data->author_name);
    data->comment_data = nebstrdup(data->comment_data);

    pthread_mutex_lock(&queue_comment_mutex);
    enqueue(&nebstruct_queue_comment, data, type);
    pthread_mutex_unlock(&queue_comment_mutex);
    trace_return_ok();
}


int ndo_handle_queue_downtime(int type, void * d)
{
    trace_func_handler(downtime);
    nebstruct_downtime_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->author_name = nebstrdup(data->author_name);
    data->comment_data = nebstrdup(data->comment_data);

    pthread_mutex_lock(&queue_downtime_mutex);
    enqueue(&nebstruct_queue_downtime, data, type);
    pthread_mutex_unlock(&queue_downtime_mutex);
    trace_return_ok();
}


int ndo_handle_queue_flapping(int type, void * d)
{
    trace_func_handler(flapping);
    nebstruct_flapping_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);

    pthread_mutex_lock(&queue_flapping_mutex);
    enqueue(&nebstruct_queue_flapping, data, type);
    pthread_mutex_unlock(&queue_flapping_mutex);
    trace_return_ok();
}


int ndo_handle_queue_host_status(int type, void * d)
{
    trace_func_handler(host_status);
    nebstruct_host_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_host_status_mutex);
    enqueue(&nebstruct_queue_host_status, data, type);
    pthread_mutex_unlock(&queue_host_status_mutex);
    trace_return_ok();
}


int ndo_handle_queue_service_status(int type, void * d)
{
    trace_func_handler(service_status);
    nebstruct_service_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_service_status_mutex);
    enqueue(&nebstruct_queue_service_status, data, type);
    pthread_mutex_unlock(&queue_service_status_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_status(int type, void * d)
{
    trace_func_handler(contact_status);
    nebstruct_contact_status_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));
    pthread_mutex_lock(&queue_contact_status_mutex);
    enqueue(&nebstruct_queue_contact_status, data, type);
    pthread_mutex_unlock(&queue_contact_status_mutex);
    trace_return_ok();
}


int ndo_handle_queue_notification(int type, void * d)
{
    trace_func_handler(notification);
    nebstruct_notification_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->output = nebstrdup(data->output);
    data->ack_author = nebstrdup(data->ack_author);
    data->ack_data = nebstrdup(data->ack_data);

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

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->contact_name = nebstrdup(data->contact_name);
    data->output = nebstrdup(data->output);
    data->ack_author = nebstrdup(data->ack_author);
    data->ack_data = nebstrdup(data->ack_data);

    pthread_mutex_lock(&queue_notification_mutex);
    enqueue(&nebstruct_queue_notification, data, type);
    pthread_mutex_unlock(&queue_notification_mutex);
    trace_return_ok();
}


int ndo_handle_queue_contact_notification_method(int type, void * d)
{
    trace_func_handler(contact_notification_method);
    nebstruct_contact_notification_method_data * data = NULL;

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->contact_name = nebstrdup(data->contact_name);
    data->command_name = nebstrdup(data->command_name);
    data->command_args = nebstrdup(data->command_args);
    data->output = nebstrdup(data->output);
    data->ack_author = nebstrdup(data->ack_author);
    data->ack_data = nebstrdup(data->ack_data);

    nebstructcpy((void *)&data, d, sizeof(*data));
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

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->author_name = nebstrdup(data->author_name);
    data->comment_data = nebstrdup(data->comment_data);

    pthread_mutex_lock(&queue_acknowledgement_mutex);
    enqueue(&nebstruct_queue_acknowledgement, data, type);
    pthread_mutex_unlock(&queue_acknowledgement_mutex);
    trace_return_ok();
}


int ndo_handle_queue_statechange(int type, void * d)
{
    trace_func_handler(statechange);
    nebstruct_statechange_data * data = NULL;
    nebstructcpy((void *)&data, d, sizeof(*data));

    /* copy data before we add to queue so it's not pointing to data that changes later */
    data->host_name = nebstrdup(data->host_name);
    data->service_description = nebstrdup(data->service_description);
    data->output = nebstrdup(data->output);
    data->longoutput = nebstrdup(data->longoutput);

    pthread_mutex_lock(&queue_statechange_mutex);
    enqueue(&nebstruct_queue_statechange, data, type);
    pthread_mutex_unlock(&queue_statechange_mutex);
    trace_return_ok();
}
