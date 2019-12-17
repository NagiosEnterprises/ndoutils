
/*
    if you want to see the expanded source, you can either:

        `make expanded`

    or:

        `gcc -E src/ndo-startup-queue.c | clang-format`
*/


int ndo_empty_startup_queues(ndo_query_context *q_ctx)
{
    ndo_empty_queue_timed_event(q_ctx);
    ndo_empty_queue_event_handler(q_ctx);
    ndo_empty_queue_host_check(q_ctx);
    ndo_empty_queue_service_check(q_ctx);
    ndo_empty_queue_comment(q_ctx);
    ndo_empty_queue_downtime(q_ctx);
    ndo_empty_queue_flapping(q_ctx);
    ndo_empty_queue_host_status(q_ctx);
    ndo_empty_queue_service_status(q_ctx);
    ndo_empty_queue_contact_status(q_ctx);
    ndo_empty_queue_acknowledgement(q_ctx);
    ndo_empty_queue_statechange(q_ctx);

    ndo_empty_queue_notification(q_ctx);
}


/* le sigh... this just makes it way easier */
#define EMPTY_QUEUE_FUNCTION(_type, _callback) \
int ndo_empty_queue_## _type(ndo_query_context *q_ctx) \
{ \
    trace_func_void(); \
\
    nebstruct_## _type ##_data * data = NULL; \
    int type = 0; \
\
    /* first we deregister our queue callback and make sure that the */ \
    /* data goes straight into the database so that our queue doesn't get */ \
    /* backed up */ \
    neb_deregister_callback(_callback, ndo_handle_queue_## _type); \
    neb_register_callback(_callback, ndo_handle, 0, ndo_neb_handle_## _type); \
\
    while (TRUE) { \
\
        /* we probably don't need the lock and unlock because of the unreg/reg */ \
        /* prior to here, but i'm not willing to take that chance without */ \
        /* testing */ \
        pthread_mutex_lock(&queue_## _type ##_mutex); \
        data = dequeue(&nebstruct_queue_## _type, &type); \
        pthread_mutex_unlock(&queue_## _type ##_mutex); \
\
        /* the queue is empty */ \
        if (data == NULL || type == -1) { \
            break; \
        } \
\
        ndo_handle_## _type(q_ctx, type, data); \
        free(data); \
        data = NULL; \
    } \
\
    trace_return_ok(); \
}


EMPTY_QUEUE_FUNCTION(timed_event, NEBCALLBACK_TIMED_EVENT_DATA)


EMPTY_QUEUE_FUNCTION(event_handler, NEBCALLBACK_EVENT_HANDLER_DATA)


EMPTY_QUEUE_FUNCTION(host_check, NEBCALLBACK_HOST_CHECK_DATA)


EMPTY_QUEUE_FUNCTION(service_check, NEBCALLBACK_SERVICE_CHECK_DATA)


EMPTY_QUEUE_FUNCTION(comment, NEBCALLBACK_COMMENT_DATA)


EMPTY_QUEUE_FUNCTION(downtime, NEBCALLBACK_DOWNTIME_DATA)


EMPTY_QUEUE_FUNCTION(flapping, NEBCALLBACK_FLAPPING_DATA)


EMPTY_QUEUE_FUNCTION(host_status, NEBCALLBACK_HOST_STATUS_DATA)


EMPTY_QUEUE_FUNCTION(service_status, NEBCALLBACK_SERVICE_STATUS_DATA)


EMPTY_QUEUE_FUNCTION(contact_status, NEBCALLBACK_CONTACT_STATUS_DATA)


EMPTY_QUEUE_FUNCTION(acknowledgement, NEBCALLBACK_ACKNOWLEDGEMENT_DATA)


EMPTY_QUEUE_FUNCTION(statechange, NEBCALLBACK_STATE_CHANGE_DATA)


/* so, the reason this one doesn't use the prototype is because the order of
   all three callbacks that notification encapsulates is actually very important

   if they aren't executed in the exact order, then the linking ids will be
   wrong - since we don't use relational ids (e.g.: find the ids based on some
   proper linking) and instead rely on mysql_insert_id - which is fine, IF
   THEY'RE EXECUTED IN ORDER. */
int ndo_empty_queue_notification(ndo_query_context *q_ctx)
{
    trace_func_void();

    void * data = NULL;
    int type = -1;

    /* unlike the EMPTY_QUEUE_FUNCTION() prototype, we can't deregister and
       then register the new ones UNTIL the queue is proven empty. if we do
       that, then we run the risk of messing up some notification ids or
       whatever */
    while (TRUE) {

        /* again, unlike the EMPTY_QUEUE_FUNCTION() prototype, the mutex
           locking and unlocking is ABSOLUTELY NECESSARY here - since we're
           popping queue members WHILE they are potentially still being added */
        pthread_mutex_lock(&queue_notification_mutex);
        data = dequeue(&nebstruct_queue_notification, &type);
        pthread_mutex_unlock(&queue_notification_mutex);

        if (data == NULL || type == -1) {

            /* there may be some contention here between deregistering and re-registering
               i don't know of a good way to solve this - or if it's really even a problem in practice
               maybe have some blocking mechanism in core to see if ndo-3 is present and if so, add some blocking
               in place */
            neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle_queue_notification);
            neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle_queue_contact_notification);
            neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle_queue_contact_notification_method);

            neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA, ndo_handle, 0, ndo_neb_handle_notification);
            neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndo_handle, 0, ndo_neb_handle_contact_notification);
            neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndo_handle, 0, ndo_neb_handle_contact_notification_method);

            break;
        }
        else if (type == NEBCALLBACK_NOTIFICATION_DATA) {
            ndo_handle_notification(q_ctx, type, data);
        }
        else if (type == NEBCALLBACK_CONTACT_NOTIFICATION_DATA) {
            ndo_handle_contact_notification(q_ctx, type, data);
        }
        else if (type == NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA) {
            ndo_handle_contact_notification_method(q_ctx, type, data);
        }

        free(data);
        data = NULL;
    }

    trace_return_ok();
}
