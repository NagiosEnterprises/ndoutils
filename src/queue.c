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

#include "../include/config.h"
#include "../include/queue.h"
#include <sys/ipc.h>
#include <sys/msg.h>

#define RETRY_LOG_INTERVAL 600 /* Seconds */
#define MAX_RETRIES	20 /* Max number of times to retry sending message. */

static time_t last_retry_log_time = 0;
static int queue_id = -1;

static void queue_alarm_handler(int sig);


int del_queue(void) {
	struct msqid_ds buf;

	if (msgctl(queue_id, IPC_RMID, &buf) < 0) {
		syslog(LOG_ERR, "Error: queue remove error.\n");
		return -1;
	}

	return 0;
}

int get_queue_id(int id) {
	key_t key = ftok(NDO_QUEUE_PATH, NDO_QUEUE_ID+id);

	if ((queue_id = msgget(key, IPC_CREAT | 0600)) < 0) {
		syslog(LOG_ERR, "Error: queue init error.\n");
		return -1;
	}

	/* Register our message send timeout signal handler. */
	signal(SIGALRM, queue_alarm_handler);
	return 0;
}

static long get_msgmni(void) {
	FILE *fp;
	char buf[1024];
	char *endptr;

	if (!(fp = fopen("/proc/sys/kernel/msgmni", "r")))
		return -1;

	fgets(buf, sizeof(buf), fp);
	fclose(fp);
	return strtol(buf, &endptr, 10);
}

static void log_retry(void) {
	time_t now;
	struct msqid_ds queue_stats;
	long msgmni;
	char curstats[1024];
	/* #define these so the syslog formats are string literals and argument types
	 * can be checked. */
	#define LOGFMT "Warning: Retrying message send. This can occur because you have too few messages allowed or too few total bytes allowed in message queues. %s See README for kernel tuning options.\n"
#if defined(__linux__)
	#define STATSFMT "You are currently using %lu of %lu messages and %lu of %lu bytes in the queue."
#endif

	time(&now);

	/* if we've never logged a retry message or we've exceeded the retry log interval */
	if ((now - last_retry_log_time) > RETRY_LOG_INTERVAL) {
		/* Get the message queue statistics */
		if (msgctl(queue_id, IPC_STAT, &queue_stats)) {
			sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_STAT: %d", errno);
			syslog(LOG_ERR, LOGFMT, curstats);
		}
		else {
#if defined(__linux__)
			/* Get the maximum number of messages allowed in a queue */
			msgmni = get_msgmni();
			if (msgmni < 0) {
				sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_INFO: %d", errno);
			}
			else {
				sprintf(curstats, "You are currently using %lu of %lu messages and %lu of %lu bytes in the queue.",
						queue_stats.msg_qnum, (unsigned long)msgmni,
						queue_stats.__msg_cbytes, queue_stats.msg_qbytes);
			}
			syslog(LOG_ERR, LOGFMT, curstats);
#else
			syslog(LOG_ERR, LOGFMT, "");
#endif
		}

		last_retry_log_time = now;
	}
	else {
		syslog(LOG_ERR, "Warning: queue send error, retrying...\n");
	}
}

static void queue_alarm_handler(int sig) {
	(void)sig;
}

void push_into_queue(const char* buf) {
	struct queue_msg msg;
	size_t size = strlen(buf);
	size = size < NDO_MAX_MSG_SIZE ? size : NDO_MAX_MSG_SIZE;
	strncpy(msg.text, buf, size);
	msg.type = NDO_MSG_TYPE;

	alarm(1);
	if (msgsnd(queue_id, &msg, size, 0) < 0) {
		if (errno == EINTR) {
			unsigned retrynum = 0;

			log_retry();

			while (errno == EINTR && (retrynum++ < MAX_RETRIES)) {
				alarm(1);
				if (msgsnd(queue_id, &msg, size, 0) == 0) break;
			}
			if (retrynum < MAX_RETRIES) {
				syslog(LOG_ERR,"Message sent to queue after %u retries.\n", retrynum);
			}
			else {
				syslog(LOG_ERR,"Error: max retries exceeded sending message to queue. Kernel queue parameters may neeed to be tuned. See README.\n");
			}
		}
		else {
			syslog(LOG_ERR,"Error: queue send error.\n");
		}
	}
	alarm(0);
}

char* pop_from_queue(void) {
	struct queue_msg msg;
	char *buf;
	ssize_t received;
	size_t buf_size;

	received = msgrcv(queue_id, &msg, NDO_MAX_MSG_SIZE, NDO_MSG_TYPE, MSG_NOERROR);
	if (received < 0) {
		syslog(LOG_ERR,"Error: queue recv error.\n");
		received = 0;
	}

	buf_size = strnlen(msg.text, (size_t)received);
	buf = malloc(buf_size+1);
	strncpy(buf, msg.text, buf_size);
	buf[buf_size] = '\0';

	return buf;
}
