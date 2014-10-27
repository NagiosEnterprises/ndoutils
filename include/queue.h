#ifndef _QUEUE_
#define _QUEUE_

#define NDO_QUEUE_PATH "."
#define NDO_QUEUE_ID 9504
#define NDO_MAX_MSG_SIZE 1024
#define NDO_MSG_TYPE 1

struct queue_msg
{
    long type;
    char text[NDO_MAX_MSG_SIZE];
};

/* remove queue from system */
int del_queue(void);

/* initialize new queue or open existing */
int get_queue_id(int id);

/* insert into queue */
void push_into_queue(const char*);

/* get and delete from queue */
char* pop_from_queue(void);

#endif /* _QUEUE_ */
