/**
 * @file dbstatements.c Database prepared statement support for ndo2db daemon
 */
/*
 * Copyright 2014 Nagios Core Development Team and Community Contributors
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

/* Headers from our project. */
#include "../include/config.h"
#include "../include/common.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndo2db.h"
#include "../include/db.h"
#include "../include/dbstatements.h"



/* Our prefixed table names (from db.c). */
extern char *ndo2db_db_tablenames[NDO2DB_NUM_DBTABLES];



/** Prepared statement identifiers. */
enum ndo2db_stmt_id {
	NDO2DB_STMT_SAVE_CUSTOMVARIABLES,
	NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS,

	NDO2DB_NUM_STMTS
};



/** Prepared statment handles. */
static MYSQL_STMT *ndo2db_stmts[NDO2DB_NUM_STMTS];

/** Prepared statement parameter bindings. */
static MYSQL_BIND *ndo2db_binds[NDO2DB_NUM_STMTS];

/** Static storage for prepared statement bound parameters. */
static signed char ndo2db_stmt_sc[2];
static unsigned int ndo2db_stmt_ui[2];
static char ndo2db_stmt_short_str[2][256];
static unsigned long ndo2db_stmt_short_strlen[2];



/**
 * ndo2db_stmt_init_*() prepared statement initialization function type.
 * @param idi Input data and DB connection handle.
 * @param dbuf Temporary dynamic buffer for printning statement templates.
 * @return NDO_OK on success, NDO_ERROR otherwise.
 */
typedef int (*ndo2db_stmt_initializer)(ndo2db_idi *idi, ndo_dbuf *dbuf);

/** Declare a prepared statement initialization function. */
#define NDO_DECLARE_STMT_INITIALIZER(f) \
	static int f(ndo2db_idi *idi, ndo_dbuf *dbuf)

NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_customvariables);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_customvariablestatus);

#undef NDO_DECLARE_STMT_INITIALIZER

/** Prepared statement initializer functions, ordered and indexable by
 * enum ndo2db_stmt_id values. */
static ndo2db_stmt_initializer ndo2db_stmt_initializers[] = {
	ndo2db_stmt_init_customvariables,
	ndo2db_stmt_init_customvariablestatus
};



/**
 * Prints an "INSERT INTO ... ON DUPLICATE KEY ..." statment template.
 * @param idi Input data and DB connection info.
 * @param dbuf Dynamic buffer for printing statment templates, reset by caller.
 * @param table Prefixed table name.
 * @param columns Array of column names to insert values for.
 * @param n Number of columns.
 * @param placeholders Prepared statement parameter placeholders. Some columns
 * may need more than just "?", e.g., "FROM_UNIXTIME(?)".
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 */
static int ndo2db_stmt_print_insert_update_template(ndo2db_idi *idi,
	ndo_dbuf *dbuf, const char *table, const char **columns, size_t n,
	const char *placeholders
);



/**
 * Copies a string v into storage bound to a prepared statement parameter.
 * The buffer to copy to is described by the MYSQL_BIND structure b. Strings
 * longer than the bind buffer will be truncated. In all cases the destination
 * bind buffer will be null-terminated and the bind structure updated with the
 * correct strlen.
 *
 * @param v The input string to copy from.
 * @param b The MYSQL_BIND for the parameter to copy to.
 */
#define COPY_BIND_STRING(v, b) \
	do { \
		unsigned long n_ = (unsigned long)strlen(v); \
		*b.length = (n_ < b.buffer_length) ? n_ : b.buffer_length - 1; \
		strncpy(b.buffer, v, (size_t)b.buffer_length); \
		((char *)b.buffer)[*b.length] = '\0'; \
	} while (0)

/* Evaluates to the number of elements in an array of known size. Note this is
 * for "type x[N]" with N a compile-time constant, not "type *x". */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif




/**
 * Initializes prepared statements once connected to the database and the
 * instance_id is available.
 */
int ndo2db_stmt_init_stmts(ndo2db_idi *idi) {
	/* Caller assures idi is non-null. */
	int i;
	ndo_dbuf dbuf;
	ndo_dbuf_init(&dbuf, 2048);

	for (i = 0; i < NDO2DB_NUM_STMTS; ++i) {
		/* Close (and free) any existing statement. */
		if (ndo2db_stmts[i]) mysql_stmt_close(ndo2db_stmts[i]);
		/* Get a new statement handle. */
		ndo2db_stmts[i] = mysql_stmt_init(&idi->dbinfo.mysql_conn);
		if (!ndo2db_stmts[i]) goto init_stmts_error_return;

		/* Reset our dbuf and free any existing binds, then prepare the statement
		 * and bind its input parameters. */
		ndo_dbuf_reset(&dbuf);
		my_free(ndo2db_binds[i]);
		if (ndo2db_stmt_initializers[i](idi, &dbuf) != NDO_OK) goto init_stmts_error_return;
	}

	ndo_dbuf_free(&dbuf); /* Be sure to free memory we've allocated. */
	return NDO_OK;

init_stmts_error_return:
	syslog(LOG_USER|LOG_ERR, "Error initializing statement %i.", i);
	syslog(LOG_USER|LOG_ERR, "mysql_error: %s", mysql_error(&idi->dbinfo.mysql_conn));
	ndo2db_stmt_free_stmts();

	ndo_dbuf_free(&dbuf);
	return NDO_ERROR;
}

/**
 * Frees resources allocated for prepared statements.
 */
int ndo2db_stmt_free_stmts(void) {
	int i;

	for (i = 0; i < NDO2DB_NUM_STMTS; ++i) {
		if (ndo2db_stmts[i]) {
			mysql_stmt_close(ndo2db_stmts[i]);
			ndo2db_stmts[i] = NULL;
		}
		my_free(ndo2db_binds[i]);
	}

	return NDO_OK;
}



/**
 * Executes a prepared statement.
 * @param idi Input data information structure.
 * @param id Preepared statement id to execute.
 * @return NDO_OK on success, NDO_ERROR on failure.
 */
static int ndo2db_stmt_execute(ndo2db_idi *idi, enum ndo2db_stmt_id id) {
	/* Caller assures idi is non-null. */
	/* If we're not connected, try and reconnect... */
	if (!idi->dbinfo.connected) {
		if (ndo2db_db_connect(idi) == NDO_ERROR) return NDO_ERROR;
		ndo2db_db_hello(idi);
		ndo2db_stmt_init_stmts(idi);
	}

	if (mysql_stmt_execute(ndo2db_stmts[id])) {
		syslog(LOG_USER|LOG_ERR, "Error: mysql_stmt_execute() failed for statement %d.", id);
		syslog(LOG_USER|LOG_ERR, "mysql_stmt_error: %s", mysql_stmt_error(ndo2db_stmts[id]));
		ndo2db_handle_db_error(idi);
		return NDO_ERROR;
	}

	return NDO_OK;
}







int ndo2db_stmt_save_customvariables(ndo2db_idi *idi, unsigned long o_id) {
	ndo2db_mbuf mbuf;
	char *name;
	char *modified;
	char *value;
	int status = NDO_OK;
	int i;
	MYSQL_BIND *binds = ndo2db_binds[NDO2DB_STMT_SAVE_CUSTOMVARIABLES];

	/* Save our object id and config type to the bound variable buffers. */
	*(unsigned int *)binds[0].buffer = (unsigned int)o_id;
	*(signed char *)binds[1].buffer = (signed char)idi->current_object_config_type;

	/* Save each custom variable. */
	mbuf = idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for (i = 0; i < mbuf.used_lines; ++i) {
		/* Skip empty buffers. */
		if (!mbuf.buffer[i]) continue;
		/* Extract the var name. */
		if (!(name = strtok(mbuf.buffer[i], ":")) || !*name) continue;
		/* Extract the has_been_modified status. */
		if (!(modified = strtok(NULL, ":"))) continue;
		/* Rest of the input string is the var value. */
		value = strtok(NULL, "\n");

		*(signed char *)binds[2].buffer = (signed char)atoi(modified);

		COPY_BIND_STRING(name, binds[3]);

		if (value) {
			COPY_BIND_STRING(value, binds[4]);
		}
		else {
			*binds[4].length = 0;
			*(char *)binds[4].buffer = '\0';
		}

		if (ndo2db_stmt_execute(idi, NDO2DB_STMT_SAVE_CUSTOMVARIABLES) != NDO_OK) {
			status = NDO_ERROR;
		}
	}

	return status;
}

int ndo2db_stmt_save_customvariablestatus(ndo2db_idi *idi, unsigned long o_id,
		unsigned long t) {
	ndo2db_mbuf mbuf;
	char *name;
	char *modified;
	char *value;
	int status = NDO_OK;
	int i;
	MYSQL_BIND *binds = ndo2db_binds[NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS];

	/* Save our object id and update time to the bound variable buffers. */
	*(unsigned int *)binds[0].buffer = (unsigned int)o_id;
	/* We use unsigned long elsewhere, but MYSQL_BIND doesn't have a type code
	 * for long. Since MySQL timestamps are 32-bit, and modern int is generally
	 * 32-bit, we'll use that. The MYSQL_BIND types seem to assume 1, 2, 4, 8 for
	 * sizeof() char, short, int and long long, respectively. This could be bad,
	 * bad news if/when sizeof(int) == 2. */
	*(unsigned int *)binds[1].buffer = (unsigned int)t;

	/* Save each custom variable. */
	mbuf = idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for (i = 0; i < mbuf.used_lines; ++i) {
		/* Skip empty buffers. */
		if (!mbuf.buffer[i]) continue;
		/* Extract the var name. */
		if (!(name = strtok(mbuf.buffer[i], ":")) || !*name) continue;
		/* Extract the has_been_modified status. */
		if (!(modified = strtok(NULL, ":"))) continue;
		/* Rest of the input string is the var value. */
		value = strtok(NULL, "\n");

		*(signed char *)binds[2].buffer = (signed char)atoi(modified);

		COPY_BIND_STRING(name, binds[3]);

		if (value) {
			COPY_BIND_STRING(value, binds[4]);
		}
		else {
			*binds[4].length = 0;
			*(char *)binds[4].buffer = '\0';
		}

		if (ndo2db_stmt_execute(idi, NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS) != NDO_OK) {
			status = NDO_ERROR;
		}
	}

	return status;
}

static int ndo2db_stmt_init_customvariables(ndo2db_idi *idi, ndo_dbuf *dbuf) {
	int status;
	MYSQL_BIND *binds;

	/* Print our full "INSERT INTO ... ON DUPLICATE KEY ..." template. */
	const char *columns[] = {
		"object_id",
		"config_type",
		"has_been_modified",
		"varname",
		"varvalue"
	};
	status = ndo2db_stmt_print_insert_update_template(idi, dbuf,
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES],
			columns, ARRAY_SIZE(columns), "?,?,?,?,?");
	if (status != NDO_OK) return status;

	/* Prepare our statement with the full template. */
	status = mysql_stmt_prepare(
			ndo2db_stmts[NDO2DB_STMT_SAVE_CUSTOMVARIABLES],
			dbuf->buf, (unsigned long)strlen(dbuf->buf));
	if (status != 0) return NDO_ERROR;

	/* Allocate our parameter bindings with null values. */
	binds = calloc(5, sizeof(MYSQL_BIND));
	if (!binds) return NDO_ERROR;

	/* object_id=%lu */
	binds[0].buffer_type = MYSQL_TYPE_LONG;
	binds[0].buffer = ndo2db_stmt_ui + 0;
	binds[0].is_unsigned = 1;
	/* config_type=%d => signed char */
	binds[1].buffer_type = MYSQL_TYPE_TINY;
	binds[1].buffer = ndo2db_stmt_sc + 0;
	/* has_been_modified=%d => signed char */
	binds[2].buffer_type = MYSQL_TYPE_TINY;
	binds[2].buffer = ndo2db_stmt_sc + 1;
	/* varname=%s => strlen() < 256 for VARCHAR(255) */
	binds[3].buffer_type = MYSQL_TYPE_STRING;
	binds[3].buffer = ndo2db_stmt_short_str + 0;
	binds[3].buffer_length = sizeof(ndo2db_stmt_short_str[0]);
	binds[3].length = ndo2db_stmt_short_strlen + 0;
	/* varvalue=%s => strlen() < 256 for VARCHAR(255) */
	binds[4].buffer_type = MYSQL_TYPE_STRING;
	binds[4].buffer = ndo2db_stmt_short_str + 1;
	binds[4].buffer_length = sizeof(ndo2db_stmt_short_str[1]);
	binds[4].length = ndo2db_stmt_short_strlen + 1;

	/* Store our binds where we can find them later. */
	ndo2db_binds[NDO2DB_STMT_SAVE_CUSTOMVARIABLES] = binds;

	/* Now bind our statement parameters. */
	status = mysql_stmt_bind_param(
			ndo2db_stmts[NDO2DB_STMT_SAVE_CUSTOMVARIABLES], binds);
	if (status != 0) return NDO_ERROR;

	return NDO_OK;
}

static int ndo2db_stmt_init_customvariablestatus(ndo2db_idi *idi, ndo_dbuf *dbuf) {
	int status;
	MYSQL_BIND *binds;

	/* Print our full "INSERT INTO ... ON DUPLICATE KEY ..." template. */
	const char *columns[] = {
		"object_id",
		"status_update_time",
		"has_been_modified",
		"varname",
		"varvalue"
	};
	status = ndo2db_stmt_print_insert_update_template(idi, dbuf,
			ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS],
			columns, ARRAY_SIZE(columns), "?,FROM_UNIXTIME(?),?,?,?");
	if (status != NDO_OK) return status;

	/* Prepare our statement with the full template. */
	status = mysql_stmt_prepare(
			ndo2db_stmts[NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS],
			dbuf->buf, (unsigned long)strlen(dbuf->buf));
	if (status != 0) return NDO_ERROR;

	/* Allocate our parameter bindings with null values. */
	binds = calloc(5, sizeof(MYSQL_BIND));
	if (!binds) return NDO_ERROR;

	/* object_id=%lu => unsigned int */
	binds[0].buffer_type = MYSQL_TYPE_LONG;
	binds[0].buffer = ndo2db_stmt_ui + 0;
	binds[0].is_unsigned = 1;
	/* status_update_time=FROM_UNIXTIME(%lu) => unsigned int */
	binds[1].buffer_type = MYSQL_TYPE_LONG;
	binds[1].buffer = ndo2db_stmt_ui + 1;
	binds[1].is_unsigned = 1;
	/* has_been_modified=%d => signed char */
	binds[2].buffer_type = MYSQL_TYPE_TINY;
	binds[2].buffer = ndo2db_stmt_sc + 0;
	/* varname=%s => strlen() < 256 for VARCHAR(255) */
	binds[3].buffer_type = MYSQL_TYPE_STRING;
	binds[3].buffer = ndo2db_stmt_short_str + 0;
	binds[3].buffer_length = sizeof(ndo2db_stmt_short_str[0]);
	binds[3].length = ndo2db_stmt_short_strlen + 0;
	/* varvalue=%s => strlen() < 256 for VARCHAR(255) */
	binds[4].buffer_type = MYSQL_TYPE_STRING;
	binds[4].buffer = ndo2db_stmt_short_str + 1;
	binds[4].buffer_length = sizeof(ndo2db_stmt_short_str[1]);
	binds[4].length = ndo2db_stmt_short_strlen + 1;

	/* Store our binds where we can find them later. */
	ndo2db_binds[NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS] = binds;

	/* Now bind our statement parameters. */
	status = mysql_stmt_bind_param(
			ndo2db_stmts[NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS], binds);
	if (status != 0) return NDO_ERROR;

	return NDO_OK;
}


static int ndo2db_stmt_print_insert_update_template(
	ndo2db_idi *idi,
	ndo_dbuf *dbuf,
	const char *table,
	const char **columns,
	size_t n,
	const char *placeholders
) {
	size_t i;
	int status;

	status = ndo_dbuf_printf(dbuf, "INSERT INTO %s (instance_id", table);
	if (status != NDO_OK) return status;

	for (i = 0; i < n; ++i) {
		status = ndo_dbuf_printf(dbuf, ",%s", columns[i]);
		if (status != NDO_OK) return status;
	}

	status = ndo_dbuf_printf(dbuf,
			") VALUES (%ld,%s) ON DUPLICATE KEY UPDATE instance_id=VALUES(instance_id)",
			idi->dbinfo.instance_id, placeholders);
	if (status != NDO_OK) return status;

	for (i = 0; i < n; ++i) {
		status = ndo_dbuf_printf(dbuf, ",%s=VALUES(%s)", columns[i], columns[i]);
		if (status != NDO_OK) return status;
	}

	return NDO_OK;
}
