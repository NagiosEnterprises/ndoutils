/**
 * @file queue.h Simple message queue for ndo2db daemon
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

#ifndef NDOUTILS_INCLUDE_QUEUE_H
#define NDOUTILS_INCLUDE_QUEUE_H

#define NDO_QUEUE_PATH "."
#define NDO_QUEUE_ID 9504
#define NDO_MAX_MSG_SIZE 1024
#define NDO_MSG_TYPE 1

struct ndo2db_queue_msg {
    long type;
    char text[NDO_MAX_MSG_SIZE];
};

/* initialize new queue or open existing */
int get_queue_id(int id);
int ndo2db_queue_init(int id);

/* Releases the queue's system resources. */
int del_queue(void);
int ndo2db_queue_free(void);

/* Inserts a message into the queue. */
int ndo2db_queue_send(struct ndo2db_queue_msg *msg, size_t msgsz);

/* Receives (and removes) a message from the queue. */
char* pop_from_queue(void);
ssize_t ndo2db_queue_recv(struct ndo2db_queue_msg *msg);

#endif
