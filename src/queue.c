#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "../include/queue.h"

static int queue_id;
static const int queue_buff_size = sizeof(struct queue_msg) - sizeof(long);

void zero_string(char *str, int size) {
	int i;

	for (i = 0; i < size; i++) {
		str[i] = '\0';
	}
}

void del_queue() {
	struct msqid_ds buf;

	if (msgctl(queue_id,IPC_RMID,&buf) < 0) {
		syslog(LOG_ERR,"Error: queue remove error.\n");
	}
}

int get_queue_id(int id) {
	key_t key = ftok(NDO_QUEUE_PATH, NDO_QUEUE_ID+id);

	if ((queue_id = msgget(key, IPC_CREAT | 0600)) < 0) {
		syslog(LOG_ERR,"Error: queue init error.\n");
	}
}

void push_into_queue (char* buf) {
	struct queue_msg msg;
	int size;
	msg.type = NDO_MSG_TYPE;
	zero_string(msg.text, NDO_MAX_MSG_SIZE);

	strncpy(msg.text, buf, NDO_MAX_MSG_SIZE-1);
//	size = sizeof(struct queue_msg) - sizeof(long);

//	if (msgsnd(queue_id, &msg, size, IPC_NOWAIT) < 0) {
	if (msgsnd(queue_id, &msg, queue_buff_size, IPC_NOWAIT) < 0) {
		syslog(LOG_ERR,"Error: queue send error.\n");
        }
}

char* pop_from_queue() {
	struct queue_msg msg;
	char *buf;

	zero_string(msg.text, NDO_MAX_MSG_SIZE);

	if (msgrcv(queue_id, &msg, queue_buff_size, NDO_MSG_TYPE, MSG_NOERROR) < 0) {
		syslog(LOG_ERR,"Error: queue recv error.\n");
	}

	int size = strlen(msg.text);
	buf = (char*)calloc(size+1, sizeof(char));
	strncpy(buf, msg.text, size);
	buf[size] = '\0';

	return buf;
}

