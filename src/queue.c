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
#include <unistd.h>

#define RETRY_LOG_INTERVAL	600		/* Seconds */
#define MAX_RETRIES	20				/* Max number of times to retry sending message */

static time_t last_retry_log_time = (time_t)0;
static int queue_id = -1;


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
	char logmsg[1024];
	char curstats[1024];
	const char *logfmt = "Warning: Retrying message send. This can occur because you have too few messages allowed or too few total bytes allowed in message queues. %s See README for kernel tuning options.\n";
#if defined(__linux__)
	const char *statsfmt = "You are currently using %lu of %lu messages and %lu of %lu bytes in the queue.";
#endif

	time(&now);

	/* if we've never logged a retry message or we've exceeded the retry log interval */
	if ((now - last_retry_log_time) > RETRY_LOG_INTERVAL) {
		/* Get the message queue statistics */
		if (msgctl(queue_id, IPC_STAT, &queue_stats)) {
			sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_STAT: %d", errno);
			sprintf(logmsg, logfmt, curstats);
		} else {
#if defined(__linux__)
			/* Get the maximum number of messages allowed in a queue */
			msgmni = get_msgmni();
			if (msgmni < 0) {
				sprintf(curstats, "Unable to determine current message queue usage: error reading IPC_INFO: %d", errno);
			} else {
				sprintf(curstats, statsfmt, queue_stats.msg_qnum, 
						(unsigned long)msgmni, queue_stats.__msg_cbytes, 
						queue_stats.msg_qbytes);
			}
			sprintf(logmsg, logfmt, curstats);
#else
			sprintf(logmsg, logfmt, "");
#endif
		}
		syslog(LOG_ERR, logmsg);

		last_retry_log_time = now;
	} else {
		syslog(LOG_ERR, "Warning: queue send error, retrying...\n");
	}
}

void push_into_queue(const char* buf) {
	struct queue_msg msg;
	size_t size = strlen(buf);
	size = size < NDO_MAX_MSG_SIZE ? size : NDO_MAX_MSG_SIZE;
	strncpy(msg.text, buf, size);
	msg.type = NDO_MSG_TYPE;

	if (msgsnd(queue_id, &msg, size, IPC_NOWAIT) < 0) {
		if (errno == EAGAIN) {
			unsigned retrynum = 0;
			log_retry();
			/* added retry loop, data was being dropped if queue was full 5/22/2012 -MG */
			while (errno == EAGAIN && (retrynum++ < MAX_RETRIES)) {
#ifdef USE_NANOSLEEP
				/* Sleep half a second if we can. */
				struct timespec delay { (time_t)0, 500000000L };
				nanosleep(&delay, NULL);
#else
				/* Nap a whole second if we can't nanosleep(). */
				sleep(1);
#endif
				if (msgsnd(queue_id, &msg, size, IPC_NOWAIT) == 0) break;
			}
			if (retrynum < MAX_RETRIES) {
				syslog(LOG_ERR,"Message sent to queue after %u retries.\n", retrynum);
			} else {
				syslog(LOG_ERR,"Error: max retries exceeded sending message to queue. Kernel queue parameters may neeed to be tuned. See README.\n");
			}
		} else {
			syslog(LOG_ERR,"Error: queue send error.\n");
		}
	}
}

char* pop_from_queue(void) {
	struct queue_msg msg;
	char *buf;

	size_t n = msgrcv(queue_id, &msg, NDO_MAX_MSG_SIZE, NDO_MSG_TYPE, MSG_NOERROR);
	if (n < 0) {
		syslog(LOG_ERR,"Error: queue recv error.\n");
		n = 0;
	}

	int size = strnlen(msg.text, n);
	buf = malloc(size+1);
	strncpy(buf, msg.text, size);
	buf[size] = '\0';

	return buf;
}
