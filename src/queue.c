/**
 * @file queue.c Simple message queue for ndo2db daemon
 */
/*
 * Copyright 2012-2014 Nagios Core Development Team and Community Contributors
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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "../include/config.h"
#include "../include/queue.h"
#include <errno.h>
#include <time.h>

#define RETRY_LOG_INTERVAL	600		/* Seconds */
#define MAX_RETRIES	20				/* Max number of times to retry sending message */

static time_t last_retry_log_time = ( time_t)0;
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
		syslog(LOG_ERR, "%s", "Error: queue remove error.\n");
	}
}

int get_queue_id(int id) {
	key_t key = ftok(NDO_QUEUE_PATH, NDO_QUEUE_ID+id);

	if ((queue_id = msgget(key, IPC_CREAT | 0600)) < 0) {
		syslog(LOG_ERR, "%s", "Error: queue init error.\n");
	}
}

long get_msgmni( void) {
	FILE *	fp;
	char	buf[ 1024];
	char *	endptr;
	long	msgmni;

	if( !( fp = fopen( "/proc/sys/kernel/msgmni", "r"))) {
		return -1;
		}
	else {
		fgets( buf, sizeof( buf), fp);
		msgmni = strtol((const char *)buf, &endptr, 10);
		return msgmni;
		}
}

void log_retry( void) {

	time_t			now;
	struct msqid_ds	queue_stats;
	FILE *			fp;
	long			msgmni;
	char 			logmsg[ 1024];
	char 			curstats[ 1024];
	const char *	logfmt = "Warning: Retrying message send. This can occur because you have too few messages allowed or too few total bytes allowed in message queues. %s See README for kernel tuning options.\n";
#if defined( __linux__)
	const char *	statsfmt = "You are currently using %lu of %lu messages and %lu of %lu bytes in the queue.";
#endif

	time( &now);

	/* if we've never logged a retry message or we've exceeded the retry log interval */
	if(( now - last_retry_log_time) > RETRY_LOG_INTERVAL) {
		/* Get the message queue statistics */
		if(msgctl(queue_id, IPC_STAT, &queue_stats)) {
			sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_STAT: %d", errno);
			sprintf(logmsg, logfmt, curstats);
			syslog(LOG_ERR, "%s", logmsg);
			}
		else {
#if defined( __linux__)
			/* Get the maximum number of messages allowed in a queue */
			msgmni = get_msgmni();
			if( msgmni < 0) {
				sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_INFO: %d", errno);
				sprintf(logmsg, logfmt, curstats);
				syslog(LOG_ERR, "%s", logmsg);
				}
			else {
				sprintf(curstats, statsfmt, queue_stats.msg_qnum,
						(unsigned long)msgmni, queue_stats.__msg_cbytes,
						queue_stats.msg_qbytes);
				sprintf(logmsg, logfmt, curstats);
				syslog(LOG_ERR, "%s", logmsg);
				}
#else
			sprintf(logmsg, logfmt, "");
			syslog(LOG_ERR, "%s", logmsg);
#endif
			}
		last_retry_log_time = now;
		}
	else {
		syslog(LOG_ERR, "%s", "Warning: queue send error, retrying...\n");
		}
}

void push_into_queue (char* buf) {
	struct queue_msg msg;
	int size;
	msg.type = NDO_MSG_TYPE;
	zero_string(msg.text, NDO_MAX_MSG_SIZE);
	struct timespec delay;
	unsigned retrynum = 0;

	strncpy(msg.text, buf, NDO_MAX_MSG_SIZE-1);

	if (msgsnd(queue_id, &msg, queue_buff_size, IPC_NOWAIT) < 0) {
		if (EAGAIN == errno) {
			log_retry();
			/* added retry loop, data was being dropped if queue was full 5/22/2012 -MG */
			while((EAGAIN == errno) && ( retrynum++ < MAX_RETRIES)) {
					if(msgsnd(queue_id, &msg, queue_buff_size, IPC_NOWAIT)==0)
							break;
					#ifdef USE_NANOSLEEP
						delay.tv_sec = 0;
						delay.tv_nsec = 250000000;
						nanosleep(&delay,NULL);
					#else
						sleep(1);
					#endif
				}
				if (retrynum < MAX_RETRIES) {
					syslog(LOG_ERR, "%s", "Message sent to queue.\n");
					}
				else {
					syslog(LOG_ERR, "%s", "Error: max retries exceeded sending message to queue. Kernel queue parameters may need to be tuned. See README.\n");
				}
			}
		else {
			syslog(LOG_ERR, "%s", "Error: queue send error.\n");
			}
		}

}

char* pop_from_queue() {
	struct queue_msg msg;
	char *buf;

	zero_string(msg.text, NDO_MAX_MSG_SIZE);

	if (msgrcv(queue_id, &msg, queue_buff_size, NDO_MSG_TYPE, MSG_NOERROR) < 0) {
		syslog(LOG_ERR, "%s", "Error: queue recv error.\n");
	}

	int size = strlen(msg.text);
	buf = (char*)calloc(size+1, sizeof(char));
	strncpy(buf, msg.text, size);
	buf[size] = '\0';

	return buf;
}

