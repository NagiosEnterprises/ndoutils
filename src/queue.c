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


int ndo2db_queue_init(int id) {
	key_t key = ftok(NDO_QUEUE_PATH, NDO_QUEUE_ID+id);

	if ((queue_id = msgget(key, IPC_CREAT | 0600)) < 0) {
		syslog(LOG_ERR, "Error: queue init error: %s", strerror(errno));
		return -1;
	}

	/* Register our message send timeout signal handler. */
	signal(SIGALRM, queue_alarm_handler);
	return 0;
}

int ndo2db_queue_free(void) {
	struct msqid_ds buf;

	if (msgctl(queue_id, IPC_RMID, &buf) < 0) {
		syslog(LOG_ERR, "Error: queue remove error: %s", strerror(errno));
		return -1;
	}

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
		syslog(LOG_ERR, "Warning: queue send error, retrying...");
	}
}

static void queue_alarm_handler(int sig) {
	(void)sig;
}

int ndo2db_queue_send(struct ndo2db_queue_msg *msg, size_t msgsz) {
	int errno_save = 0;
	int result = 0;

	msg->type = NDO_MSG_TYPE;

	alarm(1);
	result = msgsnd(queue_id, msg, msgsz, 0);
	if (result < 0 && errno == EINTR) {
		unsigned retrynum = 0;

		log_retry();

		do {
			alarm(1);
			result = msgsnd(queue_id, msg, msgsz, 0);
			++retrynum;
		} while (result < 0 && errno == EINTR && retrynum < MAX_RETRIES);

		if (result == 0) {
			syslog(LOG_ERR, "Message sent to queue after %u retries.", retrynum);
		}
		else if (result < 0 && errno == EINTR) {
			syslog(LOG_ERR, "Error: max retries exceeded sending message to queue. Kernel queue parameters may neeed to be tuned. See README.");
			errno = EINTR; /* Preserve errno == EINTR. */
		}
	}

	if (result < 0) {
		errno_save = errno;
		if (errno != EINTR) {
			syslog(LOG_ERR, "Error: queue send error: %s", strerror(errno));
		}
	}
	alarm(0);

	errno = errno_save;
	return result;
}

char* pop_from_queue(void) {
	struct ndo2db_queue_msg msg;
	char *buf;
	ssize_t received;
	size_t buf_size;

	received = msgrcv(queue_id, &msg, NDO_MAX_MSG_SIZE, NDO_MSG_TYPE, MSG_NOERROR);
	if (received < 0) {
		syslog(LOG_ERR, "Error: queue recv error.");
		received = 0;
	}

	buf_size = strnlen(msg.text, (size_t)received);
	buf = malloc(buf_size+1);
	strncpy(buf, msg.text, buf_size);
	buf[buf_size] = '\0';

	return buf;
}
ssize_t ndo2db_queue_recv(struct ndo2db_queue_msg *msg) {

	ssize_t received = msgrcv(queue_id, &msg, NDO_MAX_MSG_SIZE, NDO_MSG_TYPE, MSG_NOERROR);
	if (received < 0) {
		int errno_save = errno;
		syslog(LOG_ERR, "Error: queue recv error: %s", strerror(errno));
		errno = errno_save;
	}

	return received;
}
