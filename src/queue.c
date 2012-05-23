#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "../include/queue.h"
#include <errno.h>
#include <time.h>

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
	struct timespec delay;
	int sleep_time;

	strncpy(msg.text, buf, NDO_MAX_MSG_SIZE-1);

	if (msgsnd(queue_id, &msg, queue_buff_size, IPC_NOWAIT) < 0) {
			syslog(LOG_ERR,"Error: queue send error, retrying...\n");
			/* added retry loop, data was being dropped if queue was full 5/22/2012 -MG */
			while(errno == EAGAIN) {
					if(msgsnd(queue_id, &msg, queue_buff_size, IPC_NOWAIT)==0)
							break;
					#ifdef USE_NANOSLEEP
						delay.tv_sec=(time_t)sleep_time;
						delay.tv_nsec=(long)((sleep_time-(double)delay.tv_sec)*1000000000);
						nanosleep(&delay,NULL);
					#else
						delay.tv_sec=(time_t)sleep_time;
						if(delay.tv_sec==0L)
							delay.tv_sec=1;
							delay.tv_nsec=0L;
							sleep((unsigned int)delay.tv_sec);
					#endif 		
				}
				syslog(LOG_ERR,"Message sent to queue\n");
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

