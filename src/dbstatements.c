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
#include "../include/dbhandlers.h"
#include "../include/dbstatements.h"

/* Nagios headers. */
#ifdef BUILD_NAGIOS_2X
// #include "../include/nagios-2x/nagios.h"
#include "../include/nagios-2x/broker.h"
// #include "../include/nagios-2x/comments.h"
#endif
#ifdef BUILD_NAGIOS_3X
// #include "../include/nagios-3x/nagios.h"
#include "../include/nagios-3x/broker.h"
// #include "../include/nagios-3x/comments.h"
#endif
#ifdef BUILD_NAGIOS_4X
// #include "../include/nagios-4x/nagios.h"
#include "../include/nagios-4x/broker.h"
// #include "../include/nagios-4x/comments.h"
#endif



/* Our prefixed table names (from db.c). */
extern char *ndo2db_db_tablenames[NDO2DB_NUM_DBTABLES];



/** Short string buffer length. */
#define BIND_SHORT_STRING_LENGTH 256
/** Long string buffer length. */
#define BIND_LONG_STRING_LENGTH 65536

/** Prepared statement identifiers/indices and count. */
enum ndo2db_stmt_id {
	/** For when we want to say 'no statement'. The entry in ndo2db_stmts is not
	 * used, and will have empty/null values unless something is wrong. */
	NDO2DB_STMT_NONE = 0,
	NDO2DB_STMT_THE_FIFTH = NDO2DB_STMT_NONE,

	NDO2DB_STMT_GET_OBJ_ID,
	NDO2DB_STMT_GET_OBJ_ID_N2_NULL,
	NDO2DB_STMT_GET_OBJ_ID_INSERT,
	NDO2DB_STMT_GET_OBJ_IDS,
	NDO2DB_STMT_SET_OBJ_ACTIVE,

	NDO2DB_STMT_HANDLE_SERVICECHECK,
	NDO2DB_STMT_HANDLE_HOSTCHECK,
	NDO2DB_STMT_HANDLE_HOSTSTATUS,
	NDO2DB_STMT_HANDLE_SERVICESTATUS,

	NDO2DB_STMT_HANDLE_CONFIGFILE,
	NDO2DB_STMT_HANDLE_CONFIGFILEVARIABLE,
	NDO2DB_STMT_HANDLE_RUNTIMEVARIABLE,

	NDO2DB_STMT_HANDLE_HOST,
	NDO2DB_STMT_SAVE_HOSTPARENT,
	NDO2DB_STMT_SAVE_HOSTCONTACTGROUP,
	NDO2DB_STMT_SAVE_HOSTCONTACT,

	NDO2DB_STMT_HANDLE_HOSTGROUP,
	NDO2DB_STMT_SAVE_HOSTGROUPMEMBER,

	NDO2DB_STMT_HANDLE_SERVICE,
#ifdef BUILD_NAGIOS_4X
	NDO2DB_STMT_SAVE_SERVICEPARENT,
#endif
	NDO2DB_STMT_SAVE_SERVICECONTACTGROUP,
	NDO2DB_STMT_SAVE_SERVICECONTACT,

	NDO2DB_STMT_HANDLE_SERVICEGROUP,
	NDO2DB_STMT_SAVE_SERVICEGROUPMEMBER,

	NDO2DB_STMT_HANDLE_HOSTDEPENDENCY,
	NDO2DB_STMT_HANDLE_SERVICEDEPENDENCY,

	NDO2DB_STMT_HANDLE_HOSTESCALATION,
	NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACTGROUP,
	NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACT,

	NDO2DB_STMT_HANDLE_SERVICEESCALATION,
	NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACTGROUP,
	NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACT,

	NDO2DB_STMT_HANDLE_COMMAND,

	NDO2DB_STMT_HANDLE_TIMEPERIOD,
	NDO2DB_STMT_SAVE_TIMEPERIODRANGE,

	NDO2DB_STMT_HANDLE_CONTACT,
	NDO2DB_STMT_SAVE_CONTACTADDRESS,
	NDO2DB_STMT_SAVE_CONTACTNOTIFICATIONCOMMAND,

	NDO2DB_STMT_HANDLE_CONTACTGROUP,
	NDO2DB_STMT_SAVE_CONTACTGROUPMEMBER,

	NDO2DB_STMT_SAVE_CUSTOMVARIABLE,
	NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS,

	NDO2DB_NUM_STMTS
};

/** Input binding type codes for our use cases. */
enum bind_data_type {
	BIND_TYPE_I8, /* signed char bind */
	BIND_TYPE_I16, /* signed short bind */
	BIND_TYPE_I32, /* signed int bind */
	BIND_TYPE_U32, /* unsigned int bind */
	BIND_TYPE_DOUBLE, /* double bind */
	BIND_TYPE_SHORT_STRING, /* char[256] bind */
	BIND_TYPE_LONG_STRING, /* char{65536] bind */
	BIND_TYPE_FROM_UNIXTIME /* u32 bind, FROM_UNIXTIME(?) placeholder */
};

/** Additional binding flags for special cases. */
enum bind_flags {
	BIND_ONLY_INS = 1,
	BIND_MAYBE_NULL = 2,
	BIND_BUFFERED_INPUT = 4,
	BIND_CURRENT_CONFIG_TYPE = 8,
};

/** Bind info for template generation, binding, and data conversion. */
struct ndo2db_stmt_bind {
	/** Binding column name or NULL if not applicable. */
	const char *column;
	/** Binding and handling type information. */
	enum bind_data_type type;
	/** Data conversion index into idi->buffered_input, or -1 to skip auto
	 * data conversion of a parameter. */
	int bi_index;
	/** Additional flags. */
	enum bind_flags flags;
};

/** Prepared statement handle, bindings and parameter/result descriptions. */
struct ndo2db_stmt {
	/** Statement identifier and index. */
	enum ndo2db_stmt_id id;
	/** Prepared statment handle. */
	MYSQL_STMT *handle;

	/** Statement parameter information, assumed to live in static storage. */
	const struct ndo2db_stmt_bind *params;
	/** Prepared statement parameter bindings. Input data should be copied into
	 * the bound buffer using one of the COPY_TO_BIND or COPY_BIND_STRING_*
	 * macros. */
	MYSQL_BIND *param_binds;
	/** Count of parameters. */
	size_t np;

	/** Statement result information, assumed to live in static storage. */
	const struct ndo2db_stmt_bind *results;
	/** Prepared statement result bindings, or NULL for statements that don't
	 * generate a result set. Scalar data can be copied from the bound buffer
	 * using the COPY_FROM_BIND macro. String data can be accessed with a cast
	 * from void*. */
	MYSQL_BIND *result_binds;
	/** Count of results. */
	size_t nr;
};

/** Our prepared statements, indexed by enum ndo2db_stmt_id. */
static struct ndo2db_stmt ndo2db_stmts[NDO2DB_NUM_STMTS];


/** Static storage for bound parameters and results. */
static signed char ndo2db_stmt_bind_char[18];
static signed short ndo2db_stmt_bind_short[4];
static unsigned int ndo2db_stmt_bind_int[2];
static unsigned int ndo2db_stmt_bind_uint[14];
static double ndo2db_stmt_bind_double[5];
static char ndo2db_stmt_bind_short_str[13][256];
static char ndo2db_stmt_bind_long_str[2][65536];
/** Static storage for binding metadata. */
static unsigned long ndo2db_stmt_bind_length[13];
static my_bool ndo2db_stmt_bind_is_null[4];
static my_bool ndo2db_stmt_bind_error[4];


/**
 * ndo2db_stmt_init_*() prepared statement initialization function type.
 * This type exists to document the interface and define the initializer array,
 * it's not used otherwise.
 * @param idi Input data and DB connection handle.
 * @param dbuf Temporary dynamic buffer for printning statement templates.
 * @return NDO_OK on success, NDO_ERROR otherwise.
 */
typedef int (*ndo2db_stmt_initializer)(ndo2db_idi *idi, ndo_dbuf *dbuf);

/** Declare a prepared statement initialization function. */
#define NDO_DECLARE_STMT_INITIALIZER(f) \
	static int f(ndo2db_idi *idi, ndo_dbuf *dbuf)

NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_obj);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_servicecheck);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_hostcheck);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_hoststatus);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_servicestatus);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_configfile);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_configfilevariable);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_runtimevariable);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_host);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_hostgroup);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_service);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_servicegroup);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_hostdependency);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_servicedependency);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_hostescalation);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_serviceescalation);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_command);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_timeperiod);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_contact);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_contactgroup);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_customvariable);
NDO_DECLARE_STMT_INITIALIZER(ndo2db_stmt_init_customvariablestatus);

#undef NDO_DECLARE_STMT_INITIALIZER

/** Prepared statement initializer functions. Order in this list does not
 * matter: ndo2db_stmt_init_stmts() doesn't use statement ids, the initializers
 * themselves know the statement ids they need. After prefixing table names,
 * and connecting to the DB and obtaining and instance_id, executing all these
 * functions in order will initialize all ndo2db_stmts. */
static ndo2db_stmt_initializer ndo2db_stmt_initializers[] = {
	ndo2db_stmt_init_obj,
	/* ...NDO2DB_STMT_GET_OBJ_ID_N2_NULL */
	/* ...NDO2DB_STMT_GET_OBJ_ID_INSERT */
	/* ...NDO2DB_STMT_GET_OBJ_IDS */
	/* ...NDO2DB_STMT_SET_OBJ_ACTIVE */

	ndo2db_stmt_init_servicecheck,
	ndo2db_stmt_init_hostcheck,
	ndo2db_stmt_init_hoststatus,
	ndo2db_stmt_init_servicestatus,

	ndo2db_stmt_init_configfile,
	ndo2db_stmt_init_configfilevariable,
	ndo2db_stmt_init_runtimevariable,

	ndo2db_stmt_init_host,
	/* ...NDO2DB_STMT_SAVE_HOSTPARENT */
	/* ...NDO2DB_STMT_SAVE_HOSTCONTACTGROUP */
	/* ...NDO2DB_STMT_SAVE_HOSTCONTACT */

	ndo2db_stmt_init_hostgroup,
	/* ...NDO2DB_STMT_SAVE_HOSTGROUPMEMBER */

	ndo2db_stmt_init_service,
	/* ...NDO2DB_STMT_SAVE_SERVICEPARENT for BUILD_NAGIOS_4X */
	/* ...NDO2DB_STMT_SAVE_SERVICECONTACTGROUP */
	/* ...NDO2DB_STMT_SAVE_SERVICECONTACT */

	ndo2db_stmt_init_servicegroup,
	/* ...NDO2DB_STMT_SAVE_SERVICEGROUPMEMBER */

	ndo2db_stmt_init_hostdependency,
	ndo2db_stmt_init_servicedependency,

	ndo2db_stmt_init_hostescalation,
	/* ...NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACTGROUP */
	/* ...NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACT */

	ndo2db_stmt_init_serviceescalation,
	/* ...NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACTGROUP */
	/* ...NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACT */

	ndo2db_stmt_init_command,

	ndo2db_stmt_init_timeperiod,
	/* ...NDO2DB_STMT_SAVE_TIMEPERIODRANGE */

	ndo2db_stmt_init_contact,
	/* ...NDO2DB_STMT_SAVE_CONTACTADDRESS */
	/* ...NDO2DB_STMT_SAVE_CONTACTNOTIFICATIONCOMMAND */

	ndo2db_stmt_init_contactgroup,
	/* ...NDO2DB_STMT_SAVE_CONTACTGROUPMEMBER */

	ndo2db_stmt_init_customvariable,
	ndo2db_stmt_init_customvariablestatus
};


/**
 * Declares standard handler data: type, flags, attr, and tstamp.
 */
#define DECLARE_STD_DATA \
	int type, flags, attr; \
	struct timeval tstamp

/**
 * Converts standard handler data from idi->buffered_input.
 */
#define CONVERT_STD_DATA \
	ndo2db_convert_standard_data_elements(idi, &type, &flags, &attr, &tstamp)

/**
 * Declares and converts standard handler data.
 */
#define DECLARE_CONVERT_STD_DATA \
	DECLARE_STD_DATA; \
	CONVERT_STD_DATA

/**
 * Returns ND_OK if standard handler data tstamp older than most recent realtime
 * data.
 */
#define RETURN_OK_IF_STD_DATA_TOO_OLD \
	if (tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time) return NDO_OK

/**
 * Declares and converts standard handler data, returns if we've seen more
 * recent realtime data.
 */
#define DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD \
	DECLARE_CONVERT_STD_DATA; \
	RETURN_OK_IF_STD_DATA_TOO_OLD


/**
 * Copies a scalar to a bound buffer, casting as needed.
 *
 * @param v Source value.
 * @param b Destination MYSQL_BIND.
 * @param bt Destination bound buffer type to cast to.
 */
#define COPY_TO_BIND(v, b, bt) \
	*(bt *)(b).buffer = (bt)(v)

#define COPY_TO_BOUND_CHAR(v, b) COPY_TO_BIND(v, b, signed char);
#define COPY_TO_BOUND_SHORT(v, b) COPY_TO_BIND(v, b, short);
#define COPY_TO_BOUND_INT(v, b) COPY_TO_BIND(v, b, int);
#define COPY_TO_BOUND_UINT(v, b) COPY_TO_BIND(v, b, unsigned int);
#define COPY_TO_BOUND_DOUBLE(v, b) COPY_TO_BIND(v, b, double);


/**
 * Copies a scalar from a bound buffer, casting as needed.
 *
 * @param vt Destination value type to cast to.
 * @param b Source MYSQL_BIND.
 * @param bt Source bound buffer type to cast from.
 */
#define COPY_FROM_BIND(vt, b, bt) \
	(vt) *(bt *)(b).buffer

/**
 * Copies a non-null string v into storage bound to a prepared statement
 * parameter.
 * The destination buffer is described by the MYSQL_BIND structure b. Strings
 * longer than the bind buffer will be truncated. In all cases the destination
 * bind buffer will be null-terminated and the bind structure updated with the
 * correct strlen.
 *
 * @param v The input string to copy from.
 * @param b The MYSQL_BIND for the parameter to copy to.
 */
#define COPY_BIND_STRING_NOT_EMPTY(v, b) \
	do { \
		unsigned long n_ = (unsigned long)strlen((v)); \
		*(b).length = (n_ < (b).buffer_length) ? n_ : (b).buffer_length - 1; \
		strncpy((b).buffer, (v), (size_t)(b).buffer_length); \
		((char *)(b).buffer)[*(b).length] = '\0'; \
	} while (0)

/**
 * Copies a string v into storage bound to a prepared statement parameter,
 * defaulting to the empty string if v is null or empty.
 * The destination buffer is described by the MYSQL_BIND structure b. Strings
 * longer than the bind buffer will be truncated. In all cases the destination
 * bind buffer will be null-terminated and the bind structure updated with the
 * correct strlen.
 *
 * @param v The input string to copy from.
 * @param b The MYSQL_BIND for the parameter to copy to.
 */
#define COPY_BIND_STRING_OR_EMPTY(v, b) \
	do { \
		if ((v) && *(v)) { \
			COPY_BIND_STRING_NOT_EMPTY((v), (b)); \
		} \
		else { \
			*(b).length = 0; \
			((char *)(b).buffer)[0] = '\0'; \
		} \
	} while (0)

/**
 * Copies a string v into storage bound to a prepared statement parameter,
 * setting *b.is_null appropriately.
 * The destination buffer is described by the MYSQL_BIND structure b. Strings
 * longer than the bind buffer will be truncated. In all cases the destination
 * bind buffer will be null-terminated and the bind structure updated with the
 * correct strlen.
 *
 * @param v The input string to copy from.
 * @param b The MYSQL_BIND for the parameter to copy to.
 */
#define COPY_BIND_STRING_OR_NULL(v, b) \
	do { \
		COPY_BIND_STRING_OR_EMPTY((v), (b)); \
		*(b).is_null = !(v); \
	} while (0)


/**
 * Evaluates to the number of elements in an array of known size. Note this is
 * for "type x[N]" with N a compile-time constant, not "type *x".
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif


/**
 * Checks the status of an expression and resurns the status if not ok.
 */
#define CHK_OK(expression) \
	do { \
		int status_ = (expression); \
		if (status_ != NDO_OK) return status_; \
	} while (0)

/**
 * Saves the error status of an expression.
 */
#define SAVE_ERR(status, expression) \
	do { \
		int status_ = (expression); \
		if (status_ != NDO_OK) status = status_; \
	} while (0)




/**
 * Initializes our prepared statements once connected to the database and our
 * instance_id is available (the caller must ensure this).
 * @param idi Input data and DB connection info.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 */
int ndo2db_stmt_init_stmts(ndo2db_idi *idi) {
	/* Caller assures idi is non-null. */
	size_t i;
	ndo_dbuf dbuf;
	ndo_dbuf_init(&dbuf, 2048);
	int status = NDO_OK;

	for (i = 0; i < ARRAY_SIZE(ndo2db_stmt_initializers); ++i) {
		/* Skip any empty initializer slots, there shouldn't be any... */
		if (!ndo2db_stmt_initializers[i]) continue;
		/* Reset our dbuf, then initialize. */
		ndo_dbuf_reset(&dbuf);
		status = ndo2db_stmt_initializers[i](idi, &dbuf);
		if (status != NDO_OK) {
			syslog(LOG_USER|LOG_ERR, "ndo2db_stmt_initializers[%u] failed.", (unsigned)i);
			syslog(LOG_USER|LOG_ERR, "mysql_error: %s", mysql_error(&idi->dbinfo.mysql_conn));
			ndo2db_stmt_free_stmts();
			break;
		}
	}

	ndo_dbuf_free(&dbuf); /* Be sure to free memory we've allocated. */
	return status;
}


/**
 * Frees resources allocated for prepared statements.
 * @return Currently NDO_OK in all cases.
 */
int ndo2db_stmt_free_stmts(void) {
	/* Caller assures idi is non-null. */
	int i;

	for (i = 0; i < NDO2DB_NUM_STMTS; ++i) {
		if (ndo2db_stmts[i].handle) {
			mysql_stmt_close(ndo2db_stmts[i].handle);
			ndo2db_stmts[i].handle = NULL;
		}
		my_free(ndo2db_stmts[i].param_binds);
		my_free(ndo2db_stmts[i].result_binds);
	}

	return NDO_OK;
}




/**
 * Prints an "INSERT INTO ..." statment template.
 * @param idi Input data and DB connection info.
 * @param dbuf Dynamic buffer for printing statment templates, reset by caller.
 * @param table Prefixed table name.
 * @param params Column name and input datatype to bind for each parameter.
 * @param np Number of parameters.
 * @param up_on_dup Non-zero to add an "ON DUPLICATE KEY ..." clause.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 */
static int ndo2db_stmt_print_insert(
	ndo2db_idi *idi,
	ndo_dbuf *dbuf,
	const char *table,
	const struct ndo2db_stmt_bind *params,
	size_t np,
	my_bool up_on_dup
) {
	size_t i;

	CHK_OK(ndo_dbuf_printf(dbuf, "INSERT INTO %s (instance_id", table));

	for (i = 0; i < np; ++i) {
		CHK_OK(ndo_dbuf_printf(dbuf, ",%s", params[i].column));
	}

	CHK_OK(ndo_dbuf_printf(dbuf, ") VALUES (%lu", idi->dbinfo.instance_id));

	for (i = 0; i < np; ++i) {
		CHK_OK(ndo_dbuf_strcat(dbuf,
				params[i].type == BIND_TYPE_FROM_UNIXTIME ? ",FROM_UNIXTIME(?)" : ",?"));
	}

	CHK_OK(ndo_dbuf_strcat(dbuf, ")"));

	if (up_on_dup) {
		CHK_OK(ndo_dbuf_strcat(dbuf,
				" ON DUPLICATE KEY UPDATE instance_id=VALUES(instance_id)"));

		for (i = 0; i < np; ++i) {
			/* Skip update values for insert only parameters. */
			if (params[i].flags & BIND_ONLY_INS) continue;
			CHK_OK(ndo_dbuf_printf(dbuf,
					",%s=VALUES(%s)", params[i].column, params[i].column));
		}
	}

	return NDO_OK;
}



/**
 * Allocates, initializes and binds a prepared statment's input parameter
 * bindings. Frees any existing bindings for the statement.
 * @param stmt Statement to bind.
 * @return NDO_OK on success, NDO_ERROR otherwise.
 */
static int ndo2db_stmt_bind_params(struct ndo2db_stmt *stmt) {
	size_t i;
	size_t n_char = 0;
	size_t n_short = 0;
	size_t n_int = 0;
	size_t n_uint = 0;
	size_t n_double = 0;
	size_t n_short_str = 0;
	size_t n_long_str = 0;
	size_t n_len = 0;
	size_t n_null = 0;

	/* Allocate our parameter bindings with null values, free any existing. */
	if (stmt->param_binds) free(stmt->param_binds);
	stmt->param_binds = calloc(stmt->np, sizeof(MYSQL_BIND));
	if (!stmt->param_binds) return NDO_ERROR;

	/* Setup the bind description structures for the parameters. */
	for (i = 0; i < stmt->np; ++i) {
		MYSQL_BIND *bind = stmt->param_binds + i;

		switch (stmt->params[i].type) {

		case BIND_TYPE_I8:
			bind->buffer_type = MYSQL_TYPE_TINY;
			bind->buffer = ndo2db_stmt_bind_char + n_char++;
			break;

		case BIND_TYPE_I16:
			bind->buffer_type = MYSQL_TYPE_SHORT;
			bind->buffer = ndo2db_stmt_bind_short + n_short++;
			break;

		case BIND_TYPE_I32:
			bind->buffer_type = MYSQL_TYPE_LONG;
			bind->buffer = ndo2db_stmt_bind_int + n_int++;
			break;

		case BIND_TYPE_U32:
		case BIND_TYPE_FROM_UNIXTIME: /* Timestamps are bound as uint. */
			/* @todo Use uint32_t et al. We use unsigned long elsewhere (e.g.
			 * timestamps and ids), but sizeof(<short|int|long>) can vary.
			 * Since modern int is generally 32-bit, we'll use that. MySQL's docs
			 * seem to assume sizeof(<char|short|int|long long>) is 1|2|4|8. This
			 * could be bad if not so. */
			bind->buffer_type = MYSQL_TYPE_LONG;
			bind->buffer = ndo2db_stmt_bind_uint + n_uint++;
			bind->is_unsigned = 1;
			break;

		case BIND_TYPE_DOUBLE:
			bind->buffer_type = MYSQL_TYPE_DOUBLE;
			bind->buffer = ndo2db_stmt_bind_double + n_double++;
			break;

		case BIND_TYPE_SHORT_STRING:
			bind->buffer_type = MYSQL_TYPE_STRING;
			/* s[i] here, s+i would point at the wrong thing. */
			bind->buffer = ndo2db_stmt_bind_short_str[n_short_str++];
			bind->buffer_length = BIND_SHORT_STRING_LENGTH;
			bind->length = ndo2db_stmt_bind_length + n_len++;
			break;

		case BIND_TYPE_LONG_STRING:
			bind->buffer_type = MYSQL_TYPE_STRING;
			bind->buffer = ndo2db_stmt_bind_long_str[n_long_str++];
			bind->buffer_length = BIND_LONG_STRING_LENGTH;
			bind->length = ndo2db_stmt_bind_length + n_len++;
			break;

		default:
			syslog(LOG_USER|LOG_ERR, "Error: %s: stmts[%d]->params[%zu] has bad type %d.",
					"ndo2db_stmt_bind_params", stmt->id, i, stmt->params[i].type);
			return NDO_ERROR;
		}

		if (stmt->params[i].flags & BIND_MAYBE_NULL) {
			bind->is_null = ndo2db_stmt_bind_is_null + n_null++;
		}
	}

	/* Now bind our statement parameters. */
	return mysql_stmt_bind_param(stmt->handle, stmt->param_binds)
			? NDO_ERROR : NDO_OK;
}


/**
 * Allocates, initializes and binds a prepared statment's output result
 * bindings. Frees any existing result bindings for the statement.
 * @param stmt Statement to bind.
 * @return NDO_OK on success, NDO_ERROR otherwise.
 */
static int ndo2db_stmt_bind_results(struct ndo2db_stmt *stmt) {
	size_t i;
	size_t n_char = 0;
	size_t n_short = 0;
	// 	size_t n_int = 0;
	size_t n_uint = 0;
	size_t n_double = 0;
	size_t n_short_str = 0;
	size_t n_long_str = 0;

	/* Allocate our bindings with null values, free any existing. */
	if (stmt->result_binds) free(stmt->result_binds);
	stmt->result_binds = calloc(stmt->nr, sizeof(MYSQL_BIND));
	if (!stmt->result_binds) return NDO_ERROR;

	/* Setup the result bind descriptions. */
	for (i = 0; i < stmt->nr; ++i) {
		MYSQL_BIND *bind = stmt->result_binds + i;

		switch (stmt->results[i].type) {

		case BIND_TYPE_I8:
			bind->buffer_type = MYSQL_TYPE_TINY;
			bind->buffer = ndo2db_stmt_bind_char + n_char++;
			break;

		case BIND_TYPE_I16:
			bind->buffer_type = MYSQL_TYPE_SHORT;
			bind->buffer = ndo2db_stmt_bind_short + n_short++;
			break;

		case BIND_TYPE_U32: /* Equal to BIND_TYPE_ULONG */
		case BIND_TYPE_FROM_UNIXTIME: /* Timestamps are bound as unsigned int. */
			bind->buffer_type = MYSQL_TYPE_LONG;
			bind->buffer = ndo2db_stmt_bind_uint + n_uint++;
			bind->is_unsigned = 1;
			break;

		case BIND_TYPE_DOUBLE:
			bind->buffer_type = MYSQL_TYPE_DOUBLE;
			bind->buffer = ndo2db_stmt_bind_double + n_double++;
			break;

		case BIND_TYPE_SHORT_STRING:
			bind->buffer_type = MYSQL_TYPE_STRING;
			bind->buffer = ndo2db_stmt_bind_short_str[n_short_str++];
			bind->buffer_length = BIND_SHORT_STRING_LENGTH;
			break;

		case BIND_TYPE_LONG_STRING:
			bind->buffer_type = MYSQL_TYPE_STRING;
			bind->buffer = ndo2db_stmt_bind_long_str[n_long_str++];
			bind->buffer_length = BIND_LONG_STRING_LENGTH;
			break;

		default:
			syslog(LOG_USER|LOG_ERR, "Error: %s: stmts[%d]->results[%zu] has bad type %d.",
					"ndo2db_stmt_bind_results", stmt->id, i, stmt->results[i].type);
			return NDO_ERROR;
		}

		/* Every reult has these metadata. */
		bind->length = ndo2db_stmt_bind_length + i;
		bind->is_null = ndo2db_stmt_bind_is_null + i;
		bind->error = ndo2db_stmt_bind_error + i;
	}

	/* Now bind our statement results. */
	return mysql_stmt_bind_result(stmt->handle, stmt->result_binds)
			? NDO_ERROR : NDO_OK;
}


/**
 * Prepares and binds a statement.
 * @param idi Input data and DB connection info.
 * @param stmt_id Statement id to prepare.
 * @param template Statement SQL template.
 * @param len Template strlen.
 * @param params Column name and input datatype to bind for each parameter.
 * @param np Number of parameters.
 * @param results Column name and output datatype to bind for each result.
 * @param nr Number of results.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 * @post ndo2db_stmts[stmt_id].handle is the statment handle.
 * @post ndo2db_stmts[stmt_id].param_binds is the array of parameter bindings.
 * @post ndo2db_stmts[stmt_id].results is the array of result bindings.
 */
static int ndo2db_stmt_prepare_and_bind(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id,
	const char *template,
	const size_t len,
	const struct ndo2db_stmt_bind *params,
	const size_t np,
	const struct ndo2db_stmt_bind *results,
	const size_t nr
) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;

	/* Store our parameters/results, counts and id for later reference. */
	stmt->params = params;
	stmt->np = np;
	stmt->results = results;
	stmt->nr = nr;
	stmt->id = stmt_id;

	/* Prepare our statement with the template. */
	syslog(LOG_USER|LOG_INFO, "Statement %d template: %s", stmt->id, template);

	/* Close (and free) any existing statement and get a new statement handle. */
	if (stmt->handle) mysql_stmt_close(stmt->handle);
	stmt->handle = mysql_stmt_init(&idi->dbinfo.mysql_conn);
	if (!stmt->handle) return NDO_ERROR;

	/* Prepare our statement from the template. */
	if (mysql_stmt_prepare(stmt->handle, template, (unsigned long)len)) return NDO_ERROR;

	/* Setup parameter and result bindings. */
	if (np) CHK_OK(ndo2db_stmt_bind_params(stmt));
	if (nr) CHK_OK(ndo2db_stmt_bind_results(stmt));

	return NDO_OK;
}


/**
 * Prepares and binds an "INSERT INTO ..." statement.
 * @param idi Input data and DB connection info.
 * @param dbuf Dynamic buffer for printing statment templates.
 * @param stmt_id Statement id to prepare.
 * @param table_id Table name index for the statement.
 * @param params Column name and input datatype to bind for each parameter.
 * @param np Number of parameters.
 * @param up_on_dup Non-zero to add an "ON DUPLICATE KEY UPDATE ..." clause.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 * @post ndo2db_stmts[stmt_id].handle is the statment handle.
 * @post ndo2db_stmts[stmt_id].param_binds is the array of parameter bindings.
 */
static int ndo2db_stmt_prepare_insert(
	ndo2db_idi *idi,
	ndo_dbuf *dbuf,
	const enum ndo2db_stmt_id stmt_id,
	const int table_id,
	const struct ndo2db_stmt_bind *params,
	const size_t np,
	my_bool up_on_dup
) {
	/* Print our template with an "ON DUPLICATE KEY UPDATE..." if requested. */
	ndo_dbuf_reset(dbuf);
	CHK_OK(ndo2db_stmt_print_insert(idi, dbuf,
			ndo2db_db_tablenames[table_id], params, np, up_on_dup));

	/* Prepare our statement and bind its parameters. */
	return ndo2db_stmt_prepare_and_bind(idi, stmt_id,
			dbuf->buf, dbuf->used_size, params, np, NULL, 0);
}



/**
 * Converts and copies buffered input data to bound parameter storage. Only
 * parameters with a valid buffered input index will be converted, others must
 * be processed manually.
 * @param idi Input data and DB connection info.
 * @param stmt Statement to convert data for.
 * @return NDO_OK on success, or NDO_ERROR on error.
 * @note Data conversion and truncation errors are silently ignored, as this
 * was the behavior of the string-based handlers.
 */
static int ndo2db_stmt_process_buffered_input(
	ndo2db_idi *idi,
	struct ndo2db_stmt *stmt
) {
	size_t i;
	const struct ndo2db_stmt_bind *p = stmt->params;
	MYSQL_BIND *b = stmt->param_binds;
	char **bi = idi->buffered_input;

	/* Nothing to do if no parameters or binds. */
	if (!p || !b) return NDO_OK;

	for (i = 0; i < stmt->np; ++i, ++p, ++b) {

		/* Skip params with no buffered_input index. */
		if (!(p->flags & BIND_BUFFERED_INPUT)) continue;

		switch (p->type) {

		case BIND_TYPE_I8:
			if (p->flags & BIND_CURRENT_CONFIG_TYPE) {
				COPY_TO_BOUND_CHAR(idi->current_object_config_type, *b);
			}
			else {
				ndo2db_strtoschar(bi[p->bi_index], b->buffer);
			}
			break;

		case BIND_TYPE_I16:
			ndo2db_strtoshort(bi[p->bi_index], b->buffer);
			break;

		case BIND_TYPE_I32: /* Equal to BIND_TYPE_LONG */
			ndo2db_strtoint(bi[p->bi_index], b->buffer);
			break;

		case BIND_TYPE_U32: /* Equal to BIND_TYPE_ULONG */
		case BIND_TYPE_FROM_UNIXTIME: /* Timestamps are bound as unsigned int. */
			ndo2db_strtouint(bi[p->bi_index], b->buffer);
			break;

		case BIND_TYPE_DOUBLE:
			ndo2db_strtod(bi[p->bi_index], b->buffer);
			break;

		case BIND_TYPE_SHORT_STRING:
		case BIND_TYPE_LONG_STRING:
			COPY_BIND_STRING_OR_EMPTY(bi[p->bi_index], *b);
			break;

		default:
			syslog(LOG_USER|LOG_ERR, "Error: %s: stmts[%d]->params[%zu] has bad type %d.",
					"ndo2db_stmt_process_buffered_input", stmt->id, i, p->type);
			return NDO_ERROR;
		}
	}

	return NDO_OK;
}



/**
 * Executes a prepared statement.
 * @param idi Input data and DB connection info.
 * @param stmt Prepared statement to execute.
 * @return NDO_OK on success, NDO_ERROR on failure.
 */
static int ndo2db_stmt_execute(ndo2db_idi *idi, struct ndo2db_stmt *stmt) {
	/* Try to connect if we're not connected... */
	if (!idi->dbinfo.connected) {
		if (ndo2db_db_connect(idi) == NDO_ERROR) return NDO_ERROR;
		ndo2db_db_hello(idi);
		ndo2db_stmt_init_stmts(idi);
	}

	if (mysql_stmt_execute(stmt->handle)) {
		syslog(LOG_USER|LOG_ERR, "Error: mysql_stmt_execute() failed for statement %d.", stmt->id);
		syslog(LOG_USER|LOG_ERR, "mysql_stmt_error: %s", mysql_stmt_error(stmt->handle));
		ndo2db_handle_db_error(idi);
		return NDO_ERROR;
	}

	return NDO_OK;
}




/**
 * Fetches an existing object id from the cache or DB.
 * @param idi Input data and DB connection info.
 * @param object_type ndo2db object type code.
 * @param name1 Object name1.
 * @param name2 Object name2.
 * @param object_id Object id output.
 * @return NDO_OK with the object id in *object_id, otherwise an error code
 * (usually NDO_ERROR) and *object_id = 0.
 */
int ndo2db_get_obj_id(
	ndo2db_idi *idi,
	int object_type,
	const char *name1,
	const char *name2,
	unsigned long *object_id
) {
	/* Select the statement and binds to use. */
	const enum ndo2db_stmt_id stmt_id = (name2 && *name2) /* name2 not empty... */
			? NDO2DB_STMT_GET_OBJ_ID : NDO2DB_STMT_GET_OBJ_ID_N2_NULL;
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;
	MYSQL_BIND *result = stmt->result_binds + 0;

	*object_id = 0;

	/* Make empty names null. */
	if (name1 && !*name1) name1 = NULL;
	if (name2 && !*name2) name2 = NULL;
	/* No names means no object id. */
	if (!name1 && !name2) return NDO_OK;

	/* See if the object is already cached. */
	if (ndo2db_get_cached_obj_id(idi, object_type, name1, name2, object_id) == NDO_OK) {
		return NDO_OK;
	}

	/* Nothing cached so query. Copy input data to the parameter buffers... */
	COPY_TO_BOUND_CHAR(object_type, binds[0]);
	COPY_BIND_STRING_OR_EMPTY(name1, binds[1]);
	if (name2) COPY_BIND_STRING_OR_NULL(name2, binds[2]);
	/* ...execute the statement... */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));
	/* ...and fetch the first (only) result row to bound storage. */
	if (mysql_stmt_fetch(stmt->handle) || *result->error || *result->is_null) {
		return NDO_ERROR;
	}

	/** We hsve a good object_id by all our meaasures if we made it here. */
	*object_id = COPY_FROM_BIND(unsigned long, *result, unsigned int);
	return NDO_OK;
}


/**
 * Fetches an existing object id from the cache or DB. Inserts a new row if an
 * existing id is not found for non-empty object names.
 * @param idi Input data and DB connection info.
 * @param object_type ndo2db object type code.
 * @param name1 Object name1.
 * @param name2 Object name2.
 * @param object_id Object id output.
 * @return NDO_OK with the object id in *object_id, otherwise an error code
 * (usually NDO_ERROR) and *object_id = 0.
 */
int ndo2db_get_obj_id_with_insert(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long *object_id) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_GET_OBJ_ID_INSERT;
	MYSQL_BIND *binds = stmt->param_binds;
	*object_id = 0;

	/* Make empty names null. */
	if (name1 && !*name1) name1 = NULL;
	if (name2 && !*name2) name2 = NULL;
	/* No names means no object id. */
	if (!name1 && !name2) return NDO_OK;

	/* See if the object already exists. */
	if (ndo2db_get_obj_id(idi, object_type, name1, name2, object_id) == NDO_OK) {
		return NDO_OK;
	}

	/* No such object so insert. Copy input data to the parameter buffers... */
	COPY_TO_BOUND_CHAR(object_type, binds[0]);
	COPY_BIND_STRING_OR_EMPTY(name1, binds[1]);
	COPY_BIND_STRING_OR_NULL(name2, binds[2]);
	/* ...execute the statement and grab the inserted object id if successful. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));
	*object_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);

	/* Cache the object id for future reference. */
	return ndo2db_add_cached_obj_id(idi, object_type, name1, name2, *object_id);
}


/**
 * Fetches all objects for an instance from the DB.
 * @param idi Input data and DB connection info.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR. It is
 * possible for the object cache to be partially populated if an error occurs
 * while processing results.
 */
int ndo2db_get_cached_obj_ids(ndo2db_idi *idi) {
	int status;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_GET_OBJ_IDS;
	MYSQL_BIND *results = stmt->result_binds;

	/* Find all the object definitions we already have */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));

	/* Buffer the complete result set from the server. */
	if (mysql_stmt_store_result(stmt->handle)) {
		syslog(LOG_USER|LOG_ERR,
				"Error: mysql_stmt_store_result() failed for stmts[%d]: %s",
				stmt->id, mysql_stmt_error(stmt->handle));
		return NDO_ERROR;
	}

	/* Process each row of the result set until an error or end of data. */
	while ((status = mysql_stmt_fetch(stmt->handle)) == 0) {

		unsigned long object_id = COPY_FROM_BIND(unsigned long, results[0], unsigned int);
		int objecttype_id = COPY_FROM_BIND(int, results[1], signed char);
		/* name1 shouldn't be NULL, but check for thoroughness. */
		const char *name1 = (*results[2].is_null) ? NULL : results[2].buffer;
		/* name2 can be NULL... */
		const char *name2 = (*results[3].is_null) ? NULL : results[3].buffer;

		/* Add object to cached list */
		ndo2db_add_cached_obj_id(idi, objecttype_id, name1, name2, object_id);
	}

	/* Success if we're here because there was no more data. */
	return (status == MYSQL_NO_DATA) ? NDO_OK : NDO_ERROR;
}


int ndo2db_get_cached_obj_id(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long *object_id) {
	ndo2db_dbobject *o;
	int y;
	int h = ndo2db_obj_hashfunc(name1, name2, NDO2DB_OBJECT_HASHSLOTS);

#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT LOOKUP: type=%d, name1=%s, name2=%s\n",
			object_type, (name1 ? name1 : "NULL"), (name2 ? name2 : "NULL"));
#endif

	if (!idi->dbinfo.object_hashlist) {
		*object_id = 0;
		return NDO_ERROR;
	}

	for (o = idi->dbinfo.object_hashlist[h], y = 0; o; o = o->nexthash, ++y) {

#ifdef NDO2DB_DEBUG_CACHING
		printf("OBJECT LOOKUP LOOPING [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n",
				h, y, o->object_type, o->object_id,
				(o->name1 ? o->name1 : "NULL"), (o->name2 ? o->name2 : "NULL"));
#endif

		if (o->object_type == object_type &&
				ndo2db_compare_obj_hashdata(o->name1, o->name2, name1, name2) == 0
		) {
			/* We have a match! */
#ifdef NDO2DB_DEBUG_CACHING
			printf("OBJECT CACHE HIT\n");
#endif
			*object_id = o->object_id;
			return NDO_OK;
		}
	}

	/* No match was found :(. */
#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT CACHE MISS\n");
#endif
	*object_id = 0;
	return NDO_ERROR;
}


int ndo2db_add_cached_obj_id(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long object_id) {
	ndo2db_dbobject *curr;
	ndo2db_dbobject *prev;
	ndo2db_dbobject *o;
	int y;
	int h;

	/* Make empty names null. */
	if (name1 && !*name1) name1 = NULL;
	if (name2 && !*name2) name2 = NULL;
	/* No names means no object id. */
	if (!name1 && !name2) return NDO_OK;

#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT CACHE ADD: type=%d, id=%lu, name1=%s, name2=%s\n",
			object_type, object_id, (name1 ? name1 : "NULL"), (name2 ? name2 : "NULL"));
#endif

	/* Initialize the hash list if needed. */
	if (!idi->dbinfo.object_hashlist) {
		idi->dbinfo.object_hashlist = calloc(NDO2DB_OBJECT_HASHSLOTS,
				sizeof(ndo2db_dbobject *));
		if (!idi->dbinfo.object_hashlist) return NDO_ERROR;
	}

	/* Construct our new object. */
	o = malloc(sizeof(ndo2db_dbobject));
	if (!o) return NDO_ERROR;
	o->object_type = object_type;
	o->object_id = object_id;
	o->name1 = name1 ? strdup(name1) : NULL;
	o->name2 = name2 ? strdup(name2) : NULL;

	h = ndo2db_obj_hashfunc(o->name1, o->name2, NDO2DB_OBJECT_HASHSLOTS);

	for (prev = NULL, curr = idi->dbinfo.object_hashlist[h], y = 0;
			curr;
			prev = curr, curr = curr->nexthash, ++y) {
		int c = ndo2db_compare_obj_hashdata(curr->name1, curr->name2, o->name1, o->name2);
		if (c == 0) {
			/* There shouldn't be duplicates, and adding duplicates would hide
			 * objects since lookup would pick the first match in the list. */
#ifdef NDO2DB_DEBUG_CACHING
			printf("OBJECT CACHE DUPLICATE\n");
#endif
			if (o->name1) free(o->name1);
			if (o->name2) free(o->name2);
			free(o);
			return NDO_ERROR;
		}
		else if (c < 0) {
			break;
		}
	}

	if (prev) prev->nexthash = o;
	else idi->dbinfo.object_hashlist[h] = o;
	o->nexthash = curr;

	return NDO_OK;
}


int ndo2db_obj_hashfunc(const char *name1, const char *name2, int hashslots) {
	size_t i;
	unsigned int h = 0;

	if (name1) {
		const size_t n = strlen(name1);
		for (i = 0; i < n; ++i) h += name1[i];
	}

	if (name2) {
		const size_t n = strlen(name2);
		for (i = 0; i < n; ++i) h += name2[i];
	}

	return h % hashslots;
}


int ndo2db_compare_obj_hashdata(const char *v1a, const char *v1b,
		const char *v2a, const char *v2b) {

	/* Check first names. */
	int result = (!v1a && !v2a) ? 0
			: (!v1a) ? +1
			: (!v2a) ? -1
			: strcmp(v1a, v2a);

	/* Check second names if necessary */
	return (result != 0) ? result
			: (!v1b && !v2b) ? 0
			: (!v1b) ? +1
			: (!v2b) ? -1
			: strcmp(v1b, v2b);
}


int ndo2db_free_cached_objs(ndo2db_idi *idi) {

	if (idi->dbinfo.object_hashlist) {
		int x = 0;
		for (; x < NDO2DB_OBJECT_HASHSLOTS; ++x) {
			ndo2db_dbobject *curr = idi->dbinfo.object_hashlist[x];
			ndo2db_dbobject *next = NULL;
			for (; curr; curr = next) {
				next = curr->nexthash;
				free(curr->name1);
				free(curr->name2);
				free(curr);
			}
		}

		free(idi->dbinfo.object_hashlist);
		idi->dbinfo.object_hashlist = NULL;
	}

	return NDO_OK;
}


/**
 * Marks all objects inactive.
 */
int ndo2db_set_all_objs_inactive(ndo2db_idi *idi) {
	int status;
	char *buf = NULL;

	/* Since this is executed once when starting up a connection, preparing this
	 * would be much more expensive than one allocaing print here. */
	if (asprintf(&buf, "UPDATE %s SET is_active=0 WHERE instance_id=%lu",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS], idi->dbinfo.instance_id
	) < 0) return NDO_ERROR;

	status = ndo2db_db_query(idi, buf);
	free(buf);
	return status;
}


int ndo2db_set_obj_active(ndo2db_idi *idi, int object_type,
		unsigned long object_id) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_SET_OBJ_ACTIVE;

	COPY_TO_BOUND_UINT(object_id, stmt->param_binds[0]);
	COPY_TO_BOUND_CHAR(object_type, stmt->param_binds[1]);

	return ndo2db_stmt_execute(idi, stmt);
}




int ndo2db_stmt_handle_logentry(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_processdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_timedeventdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_logdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_systemcommanddata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_eventhandlerdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_notificationdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_contactnotificationdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_contactnotificationmethoddata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


static int ndo2db_stmt_save_hs_check(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id
) {
	struct timeval start_time;
	struct timeval end_time;
	unsigned long object_id = 0;
	unsigned long command_id = 0;
	MYSQL_BIND *binds = ndo2db_stmts[stmt_id].param_binds;
	char **bi = idi->buffered_input;

	const my_bool is_host_check = (stmt_id == NDO2DB_STMT_HANDLE_HOSTCHECK);
	const int object_type = is_host_check
			? NDO2DB_OBJECTTYPE_HOST : NDO2DB_OBJECTTYPE_SERVICE;
	const char *name1 = bi[NDO_DATA_HOST];
	const char *name2 = is_host_check ? NULL : bi[NDO_DATA_SERVICE];
	const char *cname = bi[NDO_DATA_COMMANDNAME];

	/* Convert timestamp, etc. */
	DECLARE_CONVERT_STD_DATA;

	if (
#if (defined(BUILD_NAGIOS_3X) || defined(BUILD_NAGIOS_4X))
			/* Skip precheck events, they're not useful to us. */
			type == NEBTYPE_SERVICECHECK_ASYNC_PRECHECK ||
			type == NEBTYPE_HOSTCHECK_ASYNC_PRECHECK ||
			type == NEBTYPE_HOSTCHECK_SYNC_PRECHECK ||
#endif
			(
					/* Only process initiated or processed service check data... */
					!is_host_check &&
					type != NEBTYPE_SERVICECHECK_INITIATE &&
					type != NEBTYPE_SERVICECHECK_PROCESSED
			)
	) {
		return NDO_OK;
	}

	/* Fetch our object id. */
	ndo2db_get_obj_id_with_insert(idi, object_type,
			name1, name2, &object_id);
	/* Fetch our command id if we have a command name. */
	if (cname && *cname) {
		ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
				cname, NULL, &command_id);
	}

	ndo2db_strtotv(bi[NDO_DATA_STARTTIME], &start_time);
	ndo2db_strtotv(bi[NDO_DATA_ENDTIME], &end_time);

	/* Covert/copy our input data to bound parameter storage. */
	COPY_TO_BOUND_UINT(object_id, binds[0]);
	COPY_TO_BOUND_UINT(command_id, binds[1]);
	COPY_TO_BOUND_UINT(start_time.tv_sec, binds[2]);
	COPY_TO_BOUND_INT(start_time.tv_usec, binds[3]);
	COPY_TO_BOUND_UINT(end_time.tv_sec, binds[4]);
	COPY_TO_BOUND_INT(end_time.tv_usec, binds[5]);
	/* Host checks have an additional 'is_raw_check' boolean column (SMALLINT). */
	if (is_host_check) {
		COPY_TO_BOUND_CHAR(
				(type == NEBTYPE_HOSTCHECK_RAW_START || type == NEBTYPE_HOSTCHECK_RAW_END),
				binds[6]);
	}
	ndo2db_stmt_process_buffered_input(idi, ndo2db_stmts + stmt_id);

	/* Now save the check. */
	return ndo2db_stmt_execute(idi, ndo2db_stmts + stmt_id);
}

int ndo2db_stmt_handle_servicecheckdata(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_check(idi, NDO2DB_STMT_HANDLE_SERVICECHECK);
}

int ndo2db_stmt_handle_hostcheckdata(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_check(idi, NDO2DB_STMT_HANDLE_HOSTCHECK);
}


int ndo2db_stmt_handle_commentdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_downtimedata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_flappingdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_programstatusdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


static int ndo2db_stmt_save_hs_status(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id,
	int obj_type,
	const char *obj_name1,
	const char *obj_name2,
	const char *ctp_name1
) {
	unsigned long object_id = 0;
	unsigned long check_timeperiod_object_id = 0;
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Fetch our object ids. */
	ndo2db_get_obj_id_with_insert(idi, obj_type,
			obj_name1, obj_name2, &object_id);
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			ctp_name1, NULL, &check_timeperiod_object_id);

	/* Covert/copy our input data to bound parameter storage. */
	COPY_TO_BOUND_UINT(object_id, binds[0]);
	COPY_TO_BOUND_UINT(tstamp.tv_sec, binds[1]);
	COPY_TO_BOUND_UINT(check_timeperiod_object_id, binds[2]);
	ndo2db_stmt_process_buffered_input(idi, stmt);

	/* Save the status... */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));

	/* ...and any custom variable statuses. */
	return ndo2db_stmt_save_customvariable_status(idi, object_id, tstamp.tv_sec);
}

int ndo2db_stmt_handle_hoststatusdata(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_status(
		idi,
		NDO2DB_STMT_HANDLE_HOSTSTATUS,
		NDO2DB_OBJECTTYPE_HOST,
		idi->buffered_input[NDO_DATA_HOST],
		NULL,
		idi->buffered_input[NDO_DATA_HOSTCHECKPERIOD]
	);
}

int ndo2db_stmt_handle_servicestatusdata(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_status(
		idi,
		NDO2DB_STMT_HANDLE_SERVICESTATUS,
		NDO2DB_OBJECTTYPE_SERVICE,
		idi->buffered_input[NDO_DATA_HOST],
		idi->buffered_input[NDO_DATA_SERVICE],
		idi->buffered_input[NDO_DATA_SERVICECHECKPERIOD]
	);
}


int ndo2db_stmt_handle_contactstatusdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_adaptiveprogramdata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_adaptivehostdata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_adaptiveservicedata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_adaptivecontactdata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_externalcommanddata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_aggregatedstatusdata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_retentiondata(ndo2db_idi *idi) {
	(void)idi;
	/* Ignored as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_acknowledgementdata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_statechangedata(ndo2db_idi *idi) {
	(void)idi;
	return NDO_OK;
}


int ndo2db_stmt_handle_configfilevariables(ndo2db_idi *idi,
		int configfile_type) {
	unsigned long configfile_id = 0;
	ndo2db_mbuf *mbuf;
	int i;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_CONFIGFILE;
	MYSQL_BIND *binds = stmt->param_binds;
	int status = NDO_OK;

	/* Declare and convert timestamp, etc. */
	DECLARE_CONVERT_STD_DATA;
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [1]\n");
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "TSTAMP: %lu   LATEST: %lu\n",
			(unsigned long)tstamp.tv_sec,
			(unsigned long)idi->dbinfo.latest_realtime_data_time);
	/* Don't store old data. */
	RETURN_OK_IF_STD_DATA_TOO_OLD;
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL, 0, "HANDLE_CONFIGFILEVARS [2]\n");

	/* Covert/copy our input data to bound parameter storage... */
	COPY_TO_BOUND_SHORT(configfile_type, binds[0]);
	ndo2db_stmt_process_buffered_input(idi, stmt);
	/* ...save the config file, and then fetch its id. */
	status = ndo2db_stmt_execute(idi, stmt);
	configfile_id = (status == NDO_OK)
			? (unsigned long)mysql_stmt_insert_id(stmt->handle) : 0;

	/* Save individual config file var=val pairs. */
	stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_CONFIGFILEVARIABLE;
	binds = stmt->param_binds;
	mbuf = idi->mbuf + NDO2DB_MBUF_CONFIGFILEVARIABLE;
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *var;
		const char *val;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the var name. */
		if (!(var = strtok(mbuf->buffer[i], "=")) || !*var) continue;
		/* Rest of the input string is the var value. */
		val = strtok(NULL, "\0");

		/* Covert/copy our input data to bound parameter storage... */
		COPY_TO_BOUND_UINT(configfile_id, binds[0]);
		COPY_BIND_STRING_NOT_EMPTY(var, binds[1]);
		COPY_BIND_STRING_OR_EMPTY(val, binds[2]);
		/* ...and save the variable. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}


int ndo2db_stmt_handle_configvariables(ndo2db_idi *idi) {
	(void)idi;
	/* No-op as per the string-based handler. */
	return NDO_OK;
}


int ndo2db_stmt_handle_runtimevariables(ndo2db_idi *idi) {
	int status = NDO_OK;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_RUNTIMEVARIABLE;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + NDO2DB_MBUF_RUNTIMEVARIABLE;
	int i;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Save individual runtime var=val pairs. */
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *var;
		const char *val;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the var name. */
		if (!(var = strtok(mbuf->buffer[i], "=")) || !*var) continue;
		/* Rest of the input string is the var value. */
		val = strtok(NULL, "\0");

		/* Copy our input data to bound parameter storage... */
		COPY_BIND_STRING_NOT_EMPTY(var, binds[0]);
		COPY_BIND_STRING_OR_EMPTY(val, binds[1]);
		/* ...and save the variable. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}


int ndo2db_stmt_handle_configdumpstart(ndo2db_idi *idi) {
	DECLARE_STD_DATA;
	const char *cdt = idi->buffered_input[NDO_DATA_CONFIGDUMPTYPE];

	/* Convert timestamp, etc. */
	int status = CONVERT_STD_DATA;

	/* Set config dump type: 1 retained, 0 original. */
	idi->current_object_config_type =
			(cdt && strcmp(cdt, NDO_API_CONFIGDUMP_RETAINED) == 0) ? 1 : 0;

	return status;
}


int ndo2db_stmt_handle_configdumpend(ndo2db_idi *idi) {
	(void)idi;
	/* No-op as per the string-based handler. */
	return NDO_OK;
}


/**
 * Saves one/many or parent/child id-to-id relations.
 */
static int ndo2db_stmt_save_relations(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id,
	const unsigned long one_id,
	const size_t mbuf_index,
	const int many_type,
	const char *many_token
) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + mbuf_index;
	int i;
	int status = NDO_OK;

	/* Store the 'one' or parent id to bound storage. */
	COPY_TO_BOUND_UINT(one_id, binds[0]);

	/* Save each 'many' or child id. */
	for (i = 0; i < mbuf->used_lines; i++) {
		const char *n1 = mbuf->buffer[i];
		const char *n2 = NULL;
		unsigned long many_id = 0;
		/* Skip empty names. */
		if (!n1 || !*n1) continue;
		/* Split the name into n1/n2 parts if we have a many_token. */
		if (many_token) {
			n1 = strtok(mbuf->buffer[i], many_token);
			n2 = strtok(NULL, "\0");
			/* Skip empty first names. */
			if (!n1 || !*n1) continue;
			/* Skip services with empty names. */
			if (many_type == NDO2DB_OBJECTTYPE_SERVICE && (!n2 || !*n2)) continue;
		}
		/* Get the 'many' or child id. */
		SAVE_ERR(status, ndo2db_get_obj_id_with_insert(idi, many_type,
				n1, n2, &many_id));
		COPY_TO_BOUND_UINT(many_id, binds[1]);
		/* Save the relation. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}


static int ndo2db_stmt_save_hs_definition(
	ndo2db_idi *idi,
	const int object_type,
	const enum ndo2db_stmt_id stmt_id,
	const size_t check_cmd_index,
	const size_t event_cmd_index,
	const size_t check_period_index,
	const size_t notif_period_index,
	const enum ndo2db_stmt_id parent_stmt_id,
	const size_t parent_mbuf_index,
	const enum ndo2db_stmt_id contact_group_stmt_id,
	const enum ndo2db_stmt_id contact_stmt_id
) {
	unsigned long object_id;
	unsigned long host_object_id;
	unsigned long check_command_id;
	unsigned long event_command_id;
	unsigned long check_timeperiod_id;
	unsigned long notif_timeperiod_id;
	const char *command_str;
	const char *check_args;
	const char *event_args;
	unsigned long row_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;
	char **bi = idi->buffered_input;
	size_t x = 0;
	int status = NDO_OK;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get the check command args and object id. */
	command_str = strtok(bi[check_cmd_index], "!");
	check_args = strtok(NULL, "\0");
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
			command_str, NULL, &check_command_id);

	/* Get the event handler command args and object id. */
	command_str = strtok(bi[event_cmd_index], "!");
	event_args = strtok(NULL, "\0");
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
			command_str, NULL, &event_command_id);

	/* Get our host object id. */
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_HOST,
			bi[NDO_DATA_HOSTNAME], NULL, &host_object_id);
	/* Fetch the service object id if this is a service, otherwise use the host
	 * object id as the definition object id. */
	if (object_type == NDO2DB_OBJECTTYPE_SERVICE) {
		ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_SERVICE,
				bi[NDO_DATA_HOSTNAME], bi[NDO_DATA_SERVICEDESCRIPTION], &object_id);
	}
	else {
		object_id = host_object_id;
	}

	/* Flag the object as being active. */
	ndo2db_set_obj_active(idi, object_type, object_id);

	/* Get the timeperiod object ids. */
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			bi[check_period_index], NULL, &check_timeperiod_id);
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			bi[notif_period_index], NULL, &notif_timeperiod_id);


	/* Covert/copy our input data to bound parameter storage. */
	COPY_TO_BOUND_UINT(host_object_id, binds[x]); ++x;
	COPY_TO_BOUND_UINT(check_command_id, binds[x]); ++x;
	COPY_BIND_STRING_OR_EMPTY(check_args, binds[x]); ++x;
	COPY_TO_BOUND_UINT(event_command_id, binds[x]); ++x;
	COPY_BIND_STRING_OR_EMPTY(event_args, binds[x]); ++x;
	COPY_TO_BOUND_UINT(check_timeperiod_id, binds[x]); ++x;
	COPY_TO_BOUND_UINT(notif_timeperiod_id, binds[x]); ++x;
	if (object_type == NDO2DB_OBJECTTYPE_SERVICE) COPY_TO_BOUND_UINT(object_id, binds[x]);
	ndo2db_stmt_process_buffered_input(idi, stmt);


	/* Save the definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt)); /* Do we want to continue on error? */
	row_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);


	/* Save parent hosts/services. Check if the statement exist for Nagios < 4X
	 * cases where there are no parent services. NDO2DB_STMT_NONE and/or zero
	 * (false) means no-op. */
	if (parent_stmt_id) {
		SAVE_ERR(status, ndo2db_stmt_save_relations(idi, parent_stmt_id,
				row_id, parent_mbuf_index, object_type,
				(object_type == NDO2DB_OBJECTTYPE_SERVICE ? ";" : NULL)));
	}

	/* Save contact groups. */
	SAVE_ERR(status, ndo2db_stmt_save_relations(idi, contact_group_stmt_id,
			row_id, NDO2DB_MBUF_CONTACTGROUP, NDO2DB_OBJECTTYPE_CONTACTGROUP, NULL));

	/* Save contacts. */
	SAVE_ERR(status, ndo2db_stmt_save_relations(idi, contact_stmt_id,
			row_id, NDO2DB_MBUF_CONTACT, NDO2DB_OBJECTTYPE_CONTACT, NULL));

	/* Save custom variables. */
	SAVE_ERR(status, ndo2db_stmt_save_customvariables(idi, object_id));

	return status;
}

int ndo2db_stmt_handle_hostdefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_definition(
		idi,
		NDO2DB_OBJECTTYPE_HOST,
		NDO2DB_STMT_HANDLE_HOST,
		NDO_DATA_HOSTCHECKCOMMAND,
		NDO_DATA_HOSTEVENTHANDLER,
		NDO_DATA_HOSTCHECKPERIOD,
		NDO_DATA_HOSTNOTIFICATIONPERIOD,
		NDO2DB_STMT_SAVE_HOSTPARENT,
		NDO2DB_MBUF_PARENTHOST,
		NDO2DB_STMT_SAVE_HOSTCONTACTGROUP,
		NDO2DB_STMT_SAVE_HOSTCONTACT
	);
}

int ndo2db_stmt_handle_servicedefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_definition(
		idi,
		NDO2DB_OBJECTTYPE_SERVICE,
		NDO2DB_STMT_HANDLE_SERVICE,
		NDO_DATA_SERVICECHECKCOMMAND,
		NDO_DATA_SERVICEEVENTHANDLER,
		NDO_DATA_SERVICECHECKPERIOD,
		NDO_DATA_SERVICENOTIFICATIONPERIOD,
#ifdef BUILD_NAGIOS_4X
		NDO2DB_STMT_SAVE_SERVICEPARENT,
		NDO2DB_MBUF_PARENTSERVICE,
#else
		NDO2DB_STMT_NONE, /* Plead the fifth, there is no statement in this case. */
		0,
#endif
		NDO2DB_STMT_SAVE_SERVICECONTACTGROUP,
		NDO2DB_STMT_SAVE_SERVICECONTACT
	);
}


static int ndo2db_stmt_save_hs_group_definition(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id group_stmt_id,
	const int group_type,
	const size_t group_index,
	const enum ndo2db_stmt_id member_stmt_id,
	const int member_type,
	const size_t member_index
) {
	unsigned long object_id;
	unsigned long row_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + group_stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get the group object id. */
	ndo2db_get_obj_id_with_insert(idi, group_type,
			idi->buffered_input[group_index], NULL, &object_id);
	/* Flag the object as active. */
	ndo2db_set_obj_active(idi, group_type, object_id);

	/* Covert/copy our input data to bound parameter storage. */
	COPY_TO_BOUND_UINT(object_id, binds[0]);
	ndo2db_stmt_process_buffered_input(idi, stmt);

	/* Save the definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt)); /* Do we want to continue on error? */
	row_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);

	/* Save group member relations. */
	return ndo2db_stmt_save_relations(idi, member_stmt_id,
			row_id, member_index, member_type,
			(member_type == NDO2DB_OBJECTTYPE_SERVICE ? ";" : NULL));
}

int ndo2db_stmt_handle_hostgroupdefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_group_definition(
		idi,
		NDO2DB_STMT_HANDLE_HOSTGROUP,
		NDO2DB_OBJECTTYPE_HOSTGROUP,
		NDO_DATA_HOSTGROUPNAME,
		NDO2DB_STMT_SAVE_HOSTGROUPMEMBER,
		NDO2DB_OBJECTTYPE_HOST,
		NDO2DB_MBUF_HOSTGROUPMEMBER
	);
}

int ndo2db_stmt_handle_servicegroupdefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_group_definition(
		idi,
		NDO2DB_STMT_HANDLE_SERVICEGROUP,
		NDO2DB_OBJECTTYPE_SERVICEGROUP,
		NDO_DATA_SERVICEGROUPNAME,
		NDO2DB_STMT_SAVE_SERVICEGROUPMEMBER,
		NDO2DB_OBJECTTYPE_SERVICE,
		NDO2DB_MBUF_SERVICEGROUPMEMBER
	);
}


static int ndo2db_stmt_save_hs_dependency_definition(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id,
	const int object_type,
	const char *object_name2,
	const char *dependent_name2
) {
	const char *object_name1 = idi->buffered_input[NDO_DATA_HOSTNAME];
	const char *dependent_name1 = idi->buffered_input[NDO_DATA_DEPENDENTHOSTNAME];
	const char *timeperiod_name1 = idi->buffered_input[NDO_DATA_DEPENDENCYPERIOD];
	unsigned long object_id;
	unsigned long dependent_id;
	unsigned long timeperiod_id;
	MYSQL_BIND *binds = ndo2db_stmts[stmt_id].param_binds;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our object ids. */
	ndo2db_get_obj_id_with_insert(idi, object_type,
			object_name1, object_name2, &object_id);
	ndo2db_get_obj_id_with_insert(idi, object_type,
			dependent_name1, dependent_name2, &dependent_id);
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			timeperiod_name1, NULL, &timeperiod_id);

	/* Covert/copy our input data to bound parameter storage.... */
	COPY_TO_BOUND_UINT(object_id, binds[0]);
	COPY_TO_BOUND_UINT(dependent_id, binds[1]);
	COPY_TO_BOUND_UINT(timeperiod_id, binds[2]);
	ndo2db_stmt_process_buffered_input(idi, ndo2db_stmts + stmt_id);

	/* ...and save the definition. */
	return ndo2db_stmt_execute(idi, ndo2db_stmts + stmt_id);
}

int ndo2db_stmt_handle_hostdependencydefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_dependency_definition(
		idi,
		NDO2DB_STMT_HANDLE_HOSTDEPENDENCY,
		NDO2DB_OBJECTTYPE_HOST,
		NULL,
		NULL
	);
}

int ndo2db_stmt_handle_servicedependencydefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_save_hs_dependency_definition(
		idi,
		NDO2DB_STMT_HANDLE_SERVICEDEPENDENCY,
		NDO2DB_OBJECTTYPE_SERVICE,
		idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION],
		idi->buffered_input[NDO_DATA_DEPENDENTSERVICEDESCRIPTION]
	);
}


static int ndo2db_stmt_hs_escalation_definition(
	ndo2db_idi *idi,
	const enum ndo2db_stmt_id stmt_id,
	const int object_type,
	const char *object_name2,
	const enum ndo2db_stmt_id contact_group_stmt_id,
	const enum ndo2db_stmt_id contact_stmt_id

) {
	unsigned long object_id;
	unsigned long timeperiod_id;
	unsigned long row_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + stmt_id;
	MYSQL_BIND *binds = stmt->param_binds;
	int status = NDO_OK;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our object ids. */
	ndo2db_get_obj_id_with_insert(idi, object_type,
			idi->buffered_input[NDO_DATA_HOSTNAME], object_name2, &object_id);
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_ESCALATIONPERIOD], NULL, &timeperiod_id);

	/* Covert/copy our input data to bound parameter storage. */
	COPY_TO_BOUND_UINT(object_id, binds[0]);
	COPY_TO_BOUND_UINT(timeperiod_id, binds[1]);
	ndo2db_stmt_process_buffered_input(idi, ndo2db_stmts + stmt_id);

	/* Save the definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt)); /* Do we want to continue on error? */
	row_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);

	/* Save contact groups. */
	SAVE_ERR(status, ndo2db_stmt_save_relations(idi, contact_group_stmt_id,
			row_id, NDO2DB_MBUF_CONTACTGROUP, NDO2DB_OBJECTTYPE_CONTACTGROUP, NULL));

	/* Save contacts. */
	SAVE_ERR(status, ndo2db_stmt_save_relations(idi, contact_stmt_id,
			row_id, NDO2DB_MBUF_CONTACT, NDO2DB_OBJECTTYPE_CONTACT, NULL));

	return status;
}

int ndo2db_stmt_handle_hostescalationdefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_hs_escalation_definition(
		idi,
		NDO2DB_STMT_HANDLE_HOSTESCALATION,
		NDO2DB_OBJECTTYPE_HOST,
		NULL,
		NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACTGROUP,
		NDO2DB_STMT_SAVE_HOSTESCALATIONCONTACT
	);
}

int ndo2db_stmt_handle_serviceescalationdefinition(ndo2db_idi *idi) {
	return ndo2db_stmt_hs_escalation_definition(
		idi,
		NDO2DB_STMT_HANDLE_SERVICEESCALATION,
		NDO2DB_OBJECTTYPE_SERVICE,
		idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION],
		NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACTGROUP,
		NDO2DB_STMT_SAVE_SERVICEESCALATIONCONTACT
	);
}


int ndo2db_stmt_handle_commanddefinition(ndo2db_idi *idi) {
	unsigned long object_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_COMMAND;
	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our command object id and set the object active. */
	ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
			idi->buffered_input[NDO_DATA_COMMANDNAME], NULL, &object_id);
	ndo2db_set_obj_active(idi, NDO2DB_OBJECTTYPE_COMMAND, object_id);

	/* Copy our object id and other input data to bound parameter storage... */
	COPY_TO_BOUND_UINT(object_id, stmt->param_binds[0]);
	ndo2db_stmt_process_buffered_input(idi, stmt);
	/* ...and save the command definition. */
	return ndo2db_stmt_execute(idi, stmt);
}


int ndo2db_stmt_handle_timeperiodefinition(ndo2db_idi *idi) {
	int status = NDO_OK;
	unsigned long object_id;
	unsigned long row_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_TIMEPERIOD;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + NDO2DB_MBUF_TIMERANGE;
	int i;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our command object id and set the object active. */
	ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			idi->buffered_input[NDO_DATA_TIMEPERIODNAME], NULL, &object_id);
	ndo2db_set_obj_active(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD, object_id);

	/* Copy our object id and other input data to bound parameter storage... */
	COPY_TO_BOUND_UINT(object_id, stmt->param_binds[0]);
	ndo2db_stmt_process_buffered_input(idi, stmt);
	/* ...then save the timeperiod definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));
	row_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);

	/* Get our timerange statement and binds, and store our timerange row id. */
	stmt = ndo2db_stmts + NDO2DB_STMT_SAVE_TIMEPERIODRANGE;
	binds = stmt->param_binds;
	COPY_TO_BOUND_UINT(row_id, binds[0]);
	/* Save each timerange. */
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *day;
		const char *start;
		const char *end;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the day, and start and end times. */
		if (!(day = strtok(mbuf->buffer[i], ":")) || !*day) continue;
		if (!(start = strtok(NULL, "-")) || !*start) continue;
		if (!(end = strtok(NULL, "\0")) || !*end) continue;
		/* Convert and copy our input data to bound parameter storage... */
		ndo2db_strtoschar(day, binds[1].buffer);
		ndo2db_strtouint(start, binds[2].buffer);
		ndo2db_strtouint(end, binds[3].buffer);
		/* ...and save the timerange. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}


static int ndo2db_stmt_save_contact_commands(
	ndo2db_idi *idi,
	const unsigned long contatct_id,
	const int notification_type,
	const size_t mbuf_index
) {
	int status = NDO_OK;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_SAVE_CONTACTNOTIFICATIONCOMMAND;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + mbuf_index;
	int i;

	COPY_TO_BOUND_UINT(contatct_id, binds[0]);
	COPY_TO_BOUND_CHAR(notification_type, binds[1]);

	/* Save each host notification command. */
	for (i = 0; i < mbuf->used_lines; ++i) {
		unsigned long cmd_id = 0;
		const char *cmd_name;
		const char *cmd_args;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the command name and arguments. */
		if (!(cmd_name = strtok(mbuf->buffer[i], "!")) || !*cmd_name) continue;
		cmd_args = strtok(NULL, "\0");
		/* Find the command id, skip this item if unsuccessful. */
		if (ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_COMMAND,
				cmd_name, NULL, &cmd_id) != NDO_OK || !cmd_id) {
			status = NDO_ERROR;
			continue;
		}
		/* Convert and copy our input data to bound parameter storage... */
		COPY_TO_BOUND_UINT(cmd_id, binds[2]);
		COPY_BIND_STRING_OR_EMPTY(cmd_args, binds[3]);
		/* ...and save the address. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}

int ndo2db_stmt_handle_contactdefinition(ndo2db_idi *idi) {
	unsigned long object_id;
	unsigned long host_timeperiod_id;
	unsigned long service_timeperiod_id;
	unsigned long contact_row_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_CONTACT;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf;
	char **bi = idi->buffered_input;
	int i;
	int status = NDO_OK;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our contact object id and set the object active. */
	ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_CONTACT,
			bi[NDO_DATA_CONTACTNAME], NULL, &object_id);
	ndo2db_set_obj_active(idi, NDO2DB_OBJECTTYPE_CONTACT, object_id);

	/* Get the timeperiod object ids. */
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			bi[NDO_DATA_HOSTNOTIFICATIONPERIOD], NULL, &host_timeperiod_id);
	ndo2db_get_obj_id_with_insert(idi, NDO2DB_OBJECTTYPE_TIMEPERIOD,
			bi[NDO_DATA_SERVICENOTIFICATIONPERIOD], NULL, &service_timeperiod_id);

	/* Copy our object ids and other input data to bound parameter storage... */
	COPY_TO_BOUND_UINT(object_id, stmt->param_binds[0]);
	COPY_TO_BOUND_UINT(host_timeperiod_id, stmt->param_binds[1]);
	COPY_TO_BOUND_UINT(service_timeperiod_id, stmt->param_binds[2]);
	ndo2db_stmt_process_buffered_input(idi, stmt);

	/* ...then save the contact definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));
	contact_row_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);


	/* Get our address statement and binds, store our contact row id... */
	stmt = ndo2db_stmts + NDO2DB_STMT_SAVE_CONTACTADDRESS;
	binds = stmt->param_binds;
	COPY_TO_BOUND_UINT(contact_row_id, binds[0]);
	/* ...and save each address. */
	mbuf = idi->mbuf + NDO2DB_MBUF_CONTACTADDRESS;
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *num;
		const char *adr;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the address number and value. */
		if (!(num = strtok(mbuf->buffer[i], ":")) || !*num) continue;
		if (!(adr = strtok(NULL, "\0")) || !*adr) continue;
		/* Convert and copy our input data to bound parameter storage... */
		ndo2db_strtoshort(num, binds[1].buffer);
		COPY_BIND_STRING_NOT_EMPTY(adr, binds[2]);
		/* ...and save the address. */
		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	/* Save host notification commands. */
	SAVE_ERR(status, ndo2db_stmt_save_contact_commands(idi, contact_row_id,
			HOST_NOTIFICATION, NDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND));

	/* Save service notification commands. */
	SAVE_ERR(status, ndo2db_stmt_save_contact_commands(idi, contact_row_id,
			SERVICE_NOTIFICATION, NDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND));

	/* Save custom variables. */
	SAVE_ERR(status, ndo2db_stmt_save_customvariables(idi, contact_row_id));

	return status;
}


int ndo2db_stmt_handle_contactgroupdefinition(ndo2db_idi *idi) {
	unsigned long object_id;
	unsigned long group_id;
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_HANDLE_CONTACTGROUP;

	DECLARE_CONVERT_STD_DATA_RETURN_OK_IF_TOO_OLD;

	/* Get our contact group object id and set the object active. */
	ndo2db_get_object_id_with_insert(idi, NDO2DB_OBJECTTYPE_CONTACTGROUP,
			idi->buffered_input[NDO_DATA_CONTACTGROUPNAME], NULL, &object_id);
	ndo2db_set_obj_active(idi, NDO2DB_OBJECTTYPE_CONTACTGROUP, object_id);

	/* Copy our object id and other input data to bound parameter storage... */
	COPY_TO_BOUND_UINT(object_id, stmt->param_binds[0]);
	ndo2db_stmt_process_buffered_input(idi, stmt);
	/* ...then save the contact group definition and get its insert id. */
	CHK_OK(ndo2db_stmt_execute(idi, stmt));
	group_id = (unsigned long)mysql_stmt_insert_id(stmt->handle);

	return ndo2db_stmt_save_relations(idi, NDO2DB_STMT_SAVE_CONTACTGROUPMEMBER,
			group_id, NDO2DB_MBUF_CONTACTGROUPMEMBER, NDO2DB_OBJECTTYPE_CONTACT, NULL);
}


int ndo2db_stmt_save_customvariables(ndo2db_idi *idi, unsigned long o_id) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_SAVE_CUSTOMVARIABLE;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + NDO2DB_MBUF_CUSTOMVARIABLE;
	int i;
	int status = NDO_OK;

	/* Save our object id and config type to the bound variable buffers. */
	COPY_TO_BOUND_UINT(o_id, binds[0]);
	COPY_TO_BOUND_CHAR(idi->current_object_config_type, binds[1]);

	/* Save each custom variable. */
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *name;
		const char *modified;
		const char *value;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the var name. */
		if (!(name = strtok(mbuf->buffer[i], ":")) || !*name) continue;
		/* Extract the has_been_modified status. */
		if (!(modified = strtok(NULL, ":"))) continue;
		/* Rest of the input string is the var value. */
		value = strtok(NULL, "\n");

		ndo2db_strtoschar(modified, binds[2].buffer);
		COPY_BIND_STRING_NOT_EMPTY(name, binds[3]);
		COPY_BIND_STRING_OR_EMPTY(value, binds[4]);

		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}


int ndo2db_stmt_save_customvariable_status(ndo2db_idi *idi, unsigned long o_id,
		unsigned long t) {
	struct ndo2db_stmt *stmt = ndo2db_stmts + NDO2DB_STMT_SAVE_CUSTOMVARIABLESTATUS;
	MYSQL_BIND *binds = stmt->param_binds;
	ndo2db_mbuf *mbuf = idi->mbuf + NDO2DB_MBUF_CUSTOMVARIABLE;
	int i;
	int status = NDO_OK;

	/* Save our object id and update time to the bound variable buffers. */
	COPY_TO_BOUND_UINT(o_id, binds[0]);
	COPY_TO_BOUND_UINT(t, binds[1]);

	/* Save each custom variable. */
	for (i = 0; i < mbuf->used_lines; ++i) {
		const char *name;
		const char *modified;
		const char *value;
		/* Skip empty buffers. */
		if (!mbuf->buffer[i]) continue;
		/* Extract the var name. */
		if (!(name = strtok(mbuf->buffer[i], ":")) || !*name) continue;
		/* Extract the has_been_modified status. */
		if (!(modified = strtok(NULL, ":"))) continue;
		/* Rest of the input string is the var value. */
		value = strtok(NULL, "\n");

		ndo2db_strtoschar(modified, binds[2].buffer);
		COPY_BIND_STRING_NOT_EMPTY(name, binds[3]);
		COPY_BIND_STRING_OR_EMPTY(value, binds[4]);

		SAVE_ERR(status, ndo2db_stmt_execute(idi, stmt));
	}

	return status;
}



/** Declare a parameter initializer with no auto convert or flags. */
#define INIT_PARAM(c, t) \
	{ c, BIND_TYPE_ ## t, -1, 0 }

/** Declare a parameter initializer list with flags and no auto convert. */
#define INIT_PARAM_F(c, t, f) \
	{ c, BIND_TYPE_ ## t, -1, f }

/** Declare a parameter initializer list with auto convert and no flags. */
#define INIT_PARAM_BI(c, t, i) \
	{ c, BIND_TYPE_ ## t, i, BIND_BUFFERED_INPUT }

/** Declare a parameter initializer list with auto convert and flags. */
#define INIT_PARAM_BIF(c, t, i, f) \
	{ c, BIND_TYPE_ ## t, i, (f)|BIND_BUFFERED_INPUT }

/** Call ndo2db_stmt_prepare_insert() with specified input params, using common
 * wariables, concatenating statement and table ids with common prefixes. */
#define PREPARE_INSERT_W_PARAMS(s, t, p) \
	ndo2db_stmt_prepare_insert(idi, dbuf, \
			NDO2DB_STMT_ ## s, NDO2DB_DBTABLE_ ## t, p, ARRAY_SIZE(p), 0)

/** Call ndo2db_stmt_prepare_insert() using common wariables, concatenating
 * statement and table ids with common prefixes. */
#define PREPARE_INSERT(s, t) \
	PREPARE_INSERT_W_PARAMS(s, t, params)

/** Call ndo2db_stmt_prepare_insert() with specified input params and
 * update_on_dup using common wariables, concatenating statement and table ids
 * with common prefixes. */
#define PREPARE_INSERT_UPDATE_W_PARAMS(s, t, p) \
	ndo2db_stmt_prepare_insert(idi, dbuf, \
			NDO2DB_STMT_ ## s, NDO2DB_DBTABLE_ ## t, p, ARRAY_SIZE(p), 1)

/** Call ndo2db_stmt_prepare_insert() with update_on_dup using common wariables,
 * concatenating statement and table ids with common prefixes. */
#define PREPARE_INSERT_UPDATE(s, t) \
	PREPARE_INSERT_UPDATE_W_PARAMS(s, t, params)


/**
 * Prepares and binds a SELECT statement for fetching object ids.
 * @param idi Input data and DB connection info.
 * @param dbuf Dynamic buffer for printing the statement template.
 * @param stmt_id Statement id to prepare.
 * @param params Column name and input datatype to bind for each parameter.
 * @param np Number of parameters.
 * @param results Column name and output datatype to bind for each result.
 * @param nr Number of results.
 * @param and_where Additional WHERE ... AND condition
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 * @post ndo2db_stmts[stmt_id].handle is the statment handle.
 * @post ndo2db_stmts[stmt_id].binds is the array of parameter bindings.
 * @post ndo2db_stmts[stmt_id].results is the array of result bindings.
 */
static int ndo2db_stmt_prepare_obj_id_select(
		ndo2db_idi *idi,
		ndo_dbuf *dbuf,
		const enum ndo2db_stmt_id stmt_id,
		const struct ndo2db_stmt_bind *params,
		const size_t np,
		const struct ndo2db_stmt_bind *results,
		const size_t nr,
		const char *and_where
) {
	size_t i;

	/* Print our full template. */
	ndo_dbuf_reset(dbuf);
	CHK_OK(ndo_dbuf_strcat(dbuf, "SELECT "));

	for (i = 0; i < nr; ++i) {
		CHK_OK(ndo_dbuf_printf(dbuf, (i ? ",%s" : "%s"), results[i].column));
	}

	CHK_OK(ndo_dbuf_printf(dbuf, " FROM %s WHERE instance_id=%lu",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS], idi->dbinfo.instance_id));

	if (and_where && *and_where) {
		CHK_OK(ndo_dbuf_printf(dbuf, " AND %s", and_where));
	}

	/* Prepare our statement, and bind its parameters and results. */
	return ndo2db_stmt_prepare_and_bind(idi, stmt_id,
			dbuf->buf, dbuf->used_size, params, np, results, nr);
}


static int ndo2db_stmt_init_obj(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	/* These param/result descriptions are shared by all five object id related
	 * statments, each statement still has its own MYSQL_BINDs. */
	static const struct ndo2db_stmt_bind binding_info[] = {
		INIT_PARAM("object_id", U32),
		INIT_PARAM("objecttype_id", I8),
		INIT_PARAM("name1", SHORT_STRING),
		INIT_PARAM_F("name2", SHORT_STRING, BIND_MAYBE_NULL),
	};
	const struct ndo2db_stmt_bind *params = binding_info + 1;
	const struct ndo2db_stmt_bind *results = binding_info + 0;

	/* Our SELECT for name2 IS NOT NULL cases.
	 * params: objecttype_id, name1, name2
	 * result: object_id
	 * The BINARY operator is a MySQL special for case sensitivity. */
	CHK_OK(ndo2db_stmt_prepare_obj_id_select(idi, dbuf,
			NDO2DB_STMT_GET_OBJ_ID, params, 3, results, 1,
			"objecttype_id=? AND BINARY name1=? AND BINARY name2=?"));

	/* Our SELECT for name2 IS NULL cases.
	* params: objecttype_id, name1
	* result: object_id */
	CHK_OK(ndo2db_stmt_prepare_obj_id_select(idi, dbuf,
			NDO2DB_STMT_GET_OBJ_ID_N2_NULL, params, 2, results, 1,
			"objecttype_id=? AND BINARY name1=? AND name2 IS NULL"));

	/* Our object id INSERT.
	 * params: objecttype_id, name1, name2
	 * no results */
	CHK_OK(ndo2db_stmt_prepare_insert(idi, dbuf,
			NDO2DB_STMT_GET_OBJ_ID_INSERT, NDO2DB_DBTABLE_OBJECTS, params, 3, 0));

	/* Our SELECT for loading all objects.
	 * no params
	 * results: object_id, objecttype_id, name1, name2 */
	CHK_OK(ndo2db_stmt_prepare_obj_id_select(idi, dbuf,
			NDO2DB_STMT_GET_OBJ_IDS, params, 0, results, 4, NULL));

	/* Our UPDATE for marking an object active.
	 * params: object_id, objecttype_id
	 * no results */
	ndo_dbuf_reset(dbuf);
	CHK_OK(ndo_dbuf_printf(dbuf,
			"UPDATE %s SET is_active=1 WHERE instance_id=%lu "
			"AND object_id=? AND objecttype_id=?",
			ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS], idi->dbinfo.instance_id));

	CHK_OK(ndo2db_stmt_prepare_and_bind(idi,
			NDO2DB_STMT_SET_OBJ_ACTIVE,
			dbuf->buf, dbuf->used_size, binding_info + 0, 2, NULL, 0));

	return NDO_OK;
}


static int ndo2db_stmt_init_servicecheck(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("service_object_id", U32),
		INIT_PARAM_F("command_object_id", U32, BIND_ONLY_INS),
		INIT_PARAM("start_time", FROM_UNIXTIME),
		INIT_PARAM("start_time_usec", I32),
		INIT_PARAM("end_time", FROM_UNIXTIME),
		INIT_PARAM("end_time_usec", I32),
		INIT_PARAM_BI("check_type", I8, NDO_DATA_CHECKTYPE),
		INIT_PARAM_BI("current_check_attempt", I16, NDO_DATA_CURRENTCHECKATTEMPT),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_MAXCHECKATTEMPTS),
		INIT_PARAM_BI("state", I8, NDO_DATA_STATE),
		INIT_PARAM_BI("state_type", I8, NDO_DATA_STATETYPE),
		INIT_PARAM_BI("timeout", I8, NDO_DATA_TIMEOUT),
		INIT_PARAM_BI("early_timeout", I8, NDO_DATA_EARLYTIMEOUT),
		INIT_PARAM_BI("execution_time", DOUBLE, NDO_DATA_EXECUTIONTIME),
		INIT_PARAM_BI("latency", DOUBLE, NDO_DATA_LATENCY),
		INIT_PARAM_BI("return_code", I16, NDO_DATA_RETURNCODE),
		INIT_PARAM_BI("output", SHORT_STRING, NDO_DATA_OUTPUT),
		INIT_PARAM_BI("long_output", LONG_STRING, NDO_DATA_LONGOUTPUT),
		INIT_PARAM_BI("perfdata", LONG_STRING, NDO_DATA_PERFDATA),
		INIT_PARAM_BIF("command_args", SHORT_STRING, NDO_DATA_COMMANDARGS, BIND_ONLY_INS),
		INIT_PARAM_BIF("command_line", SHORT_STRING, NDO_DATA_COMMANDLINE, BIND_ONLY_INS)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_SERVICECHECK, SERVICECHECKS);
}


static int ndo2db_stmt_init_hostcheck(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM_F("command_object_id", U32, BIND_ONLY_INS),
		INIT_PARAM("start_time", FROM_UNIXTIME),
		INIT_PARAM("start_time_usec", I32),
		INIT_PARAM("end_time", FROM_UNIXTIME),
		INIT_PARAM("end_time_usec", I32),
		INIT_PARAM("is_raw_check", I8),
		INIT_PARAM_BI("check_type", I8, NDO_DATA_CHECKTYPE),
		INIT_PARAM_BI("current_check_attempt", I16, NDO_DATA_CURRENTCHECKATTEMPT),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_MAXCHECKATTEMPTS),
		INIT_PARAM_BI("state", I8, NDO_DATA_STATE),
		INIT_PARAM_BI("state_type", I8, NDO_DATA_STATETYPE),
		INIT_PARAM_BI("timeout", I16, NDO_DATA_TIMEOUT),
		INIT_PARAM_BI("early_timeout", I8, NDO_DATA_EARLYTIMEOUT),
		INIT_PARAM_BI("execution_time", DOUBLE, NDO_DATA_EXECUTIONTIME),
		INIT_PARAM_BI("latency", DOUBLE, NDO_DATA_LATENCY),
		INIT_PARAM_BI("return_code", I16, NDO_DATA_RETURNCODE),
		INIT_PARAM_BI("output", SHORT_STRING, NDO_DATA_OUTPUT),
		INIT_PARAM_BI("long_output", LONG_STRING, NDO_DATA_LONGOUTPUT),
		INIT_PARAM_BI("perfdata", LONG_STRING, NDO_DATA_PERFDATA),
		INIT_PARAM_BIF("command_args", SHORT_STRING, NDO_DATA_COMMANDARGS, BIND_ONLY_INS),
		INIT_PARAM_BIF("command_line", SHORT_STRING, NDO_DATA_COMMANDLINE, BIND_ONLY_INS)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_HOSTCHECK, HOSTCHECKS);
}


static int ndo2db_stmt_init_hoststatus(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM("status_update_time", FROM_UNIXTIME),
		INIT_PARAM("check_timeperiod_object_id", U32),
		INIT_PARAM_BI("output", SHORT_STRING, NDO_DATA_OUTPUT),
		INIT_PARAM_BI("long_output", LONG_STRING, NDO_DATA_LONGOUTPUT),
		INIT_PARAM_BI("perfdata", LONG_STRING, NDO_DATA_PERFDATA),
		INIT_PARAM_BI("current_state", I8, NDO_DATA_CURRENTSTATE),
		INIT_PARAM_BI("has_been_checked", I8, NDO_DATA_HASBEENCHECKED),
		INIT_PARAM_BI("should_be_scheduled", I8, NDO_DATA_SHOULDBESCHEDULED),
		INIT_PARAM_BI("current_check_attempt", I16, NDO_DATA_CURRENTCHECKATTEMPT),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_MAXCHECKATTEMPTS),
		INIT_PARAM_BI("last_check", FROM_UNIXTIME, NDO_DATA_LASTHOSTCHECK),
		INIT_PARAM_BI("next_check", FROM_UNIXTIME, NDO_DATA_NEXTHOSTCHECK),
		INIT_PARAM_BI("check_type", I8, NDO_DATA_CHECKTYPE),
		INIT_PARAM_BI("last_state_change", FROM_UNIXTIME, NDO_DATA_LASTSTATECHANGE),
		INIT_PARAM_BI("last_hard_state_change", FROM_UNIXTIME, NDO_DATA_LASTHARDSTATECHANGE),
		INIT_PARAM_BI("last_hard_state", I8, NDO_DATA_LASTHARDSTATE),
		INIT_PARAM_BI("last_time_up", FROM_UNIXTIME, NDO_DATA_LASTTIMEUP),
		INIT_PARAM_BI("last_time_down", FROM_UNIXTIME, NDO_DATA_LASTTIMEDOWN),
		INIT_PARAM_BI("last_time_unreachable", FROM_UNIXTIME, NDO_DATA_LASTTIMEUNREACHABLE),
		INIT_PARAM_BI("state_type", I8, NDO_DATA_STATETYPE),
		INIT_PARAM_BI("last_notification", FROM_UNIXTIME, NDO_DATA_LASTHOSTNOTIFICATION),
		INIT_PARAM_BI("next_notification", FROM_UNIXTIME, NDO_DATA_NEXTHOSTNOTIFICATION),
		INIT_PARAM_BI("no_more_notifications", I8, NDO_DATA_NOMORENOTIFICATIONS),
		INIT_PARAM_BI("notifications_enabled", I8, NDO_DATA_NOTIFICATIONSENABLED),
		INIT_PARAM_BI("problem_has_been_acknowledged", I8, NDO_DATA_PROBLEMHASBEENACKNOWLEDGED),
		INIT_PARAM_BI("acknowledgement_type", I8, NDO_DATA_ACKNOWLEDGEMENTTYPE),
		INIT_PARAM_BI("current_notification_number", I16, NDO_DATA_CURRENTNOTIFICATIONNUMBER),
		INIT_PARAM_BI("passive_checks_enabled", I8, NDO_DATA_PASSIVEHOSTCHECKSENABLED),
		INIT_PARAM_BI("active_checks_enabled", I8, NDO_DATA_ACTIVEHOSTCHECKSENABLED),
		INIT_PARAM_BI("event_handler_enabled", I8, NDO_DATA_EVENTHANDLERENABLED),
		INIT_PARAM_BI("flap_detection_enabled", I8, NDO_DATA_FLAPDETECTIONENABLED),
		INIT_PARAM_BI("is_flapping", I8, NDO_DATA_ISFLAPPING),
		INIT_PARAM_BI("percent_state_change", DOUBLE, NDO_DATA_PERCENTSTATECHANGE),
		INIT_PARAM_BI("latency", DOUBLE, NDO_DATA_LATENCY),
		INIT_PARAM_BI("execution_time", DOUBLE, NDO_DATA_EXECUTIONTIME),
		INIT_PARAM_BI("scheduled_downtime_depth", I16, NDO_DATA_SCHEDULEDDOWNTIMEDEPTH),
		INIT_PARAM_BI("failure_prediction_enabled", I8, NDO_DATA_FAILUREPREDICTIONENABLED),
		INIT_PARAM_BI("process_performance_data", I8, NDO_DATA_PROCESSPERFORMANCEDATA),
		INIT_PARAM_BI("obsess_over_host", I8, NDO_DATA_OBSESSOVERHOST),
		INIT_PARAM_BI("modified_host_attributes", U32, NDO_DATA_MODIFIEDHOSTATTRIBUTES),
		INIT_PARAM_BI("event_handler", SHORT_STRING, NDO_DATA_EVENTHANDLER),
		INIT_PARAM_BI("check_command", SHORT_STRING,  NDO_DATA_CHECKCOMMAND),
		INIT_PARAM_BI("normal_check_interval", DOUBLE, NDO_DATA_NORMALCHECKINTERVAL),
		INIT_PARAM_BI("retry_check_interval", DOUBLE, NDO_DATA_RETRYCHECKINTERVAL)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_HOSTSTATUS, HOSTSTATUS);
}


static int ndo2db_stmt_init_servicestatus(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("service_object_id", U32),
		INIT_PARAM("status_update_time", FROM_UNIXTIME),
		INIT_PARAM("check_timeperiod_object_id", U32),
		INIT_PARAM_BI("output", SHORT_STRING, NDO_DATA_OUTPUT),
		INIT_PARAM_BI("long_output", LONG_STRING, NDO_DATA_LONGOUTPUT),
		INIT_PARAM_BI("perfdata", LONG_STRING, NDO_DATA_PERFDATA),
		INIT_PARAM_BI("current_state", I8, NDO_DATA_CURRENTSTATE),
		INIT_PARAM_BI("has_been_checked", I8, NDO_DATA_HASBEENCHECKED),
		INIT_PARAM_BI("should_be_scheduled", I8, NDO_DATA_SHOULDBESCHEDULED),
		INIT_PARAM_BI("current_check_attempt", I16, NDO_DATA_CURRENTCHECKATTEMPT),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_MAXCHECKATTEMPTS),
		INIT_PARAM_BI("last_check", FROM_UNIXTIME, NDO_DATA_LASTSERVICECHECK),
		INIT_PARAM_BI("next_check", FROM_UNIXTIME, NDO_DATA_NEXTSERVICECHECK),
		INIT_PARAM_BI("check_type", I8, NDO_DATA_CHECKTYPE),
		INIT_PARAM_BI("last_state_change", FROM_UNIXTIME, NDO_DATA_LASTSTATECHANGE),
		INIT_PARAM_BI("last_hard_state_change", FROM_UNIXTIME, NDO_DATA_LASTHARDSTATECHANGE),
		INIT_PARAM_BI("last_hard_state", I8, NDO_DATA_LASTHARDSTATE),
		INIT_PARAM_BI("last_time_ok", FROM_UNIXTIME, NDO_DATA_LASTTIMEOK),
		INIT_PARAM_BI("last_time_warning", FROM_UNIXTIME, NDO_DATA_LASTTIMEWARNING),
		INIT_PARAM_BI("last_time_unknown", FROM_UNIXTIME, NDO_DATA_LASTTIMEUNKNOWN),
		INIT_PARAM_BI("last_time_critical", FROM_UNIXTIME, NDO_DATA_LASTTIMECRITICAL),
		INIT_PARAM_BI("state_type", I8, NDO_DATA_STATETYPE),
		INIT_PARAM_BI("last_notification", FROM_UNIXTIME, NDO_DATA_LASTSERVICENOTIFICATION),
		INIT_PARAM_BI("next_notification", FROM_UNIXTIME, NDO_DATA_NEXTSERVICENOTIFICATION),
		INIT_PARAM_BI("no_more_notifications", I8, NDO_DATA_NOMORENOTIFICATIONS),
		INIT_PARAM_BI("notifications_enabled", I8, NDO_DATA_NOTIFICATIONSENABLED),
		INIT_PARAM_BI("problem_has_been_acknowledged", I8, NDO_DATA_PROBLEMHASBEENACKNOWLEDGED),
		INIT_PARAM_BI("acknowledgement_type", I8, NDO_DATA_ACKNOWLEDGEMENTTYPE),
		INIT_PARAM_BI("current_notification_number", I16, NDO_DATA_CURRENTNOTIFICATIONNUMBER),
		INIT_PARAM_BI("passive_checks_enabled", I8, NDO_DATA_PASSIVESERVICECHECKSENABLED),
		INIT_PARAM_BI("active_checks_enabled", I8, NDO_DATA_ACTIVESERVICECHECKSENABLED),
		INIT_PARAM_BI("event_handler_enabled", I8, NDO_DATA_EVENTHANDLERENABLED),
		INIT_PARAM_BI("flap_detection_enabled", I8, NDO_DATA_FLAPDETECTIONENABLED),
		INIT_PARAM_BI("is_flapping", I8, NDO_DATA_ISFLAPPING),
		INIT_PARAM_BI("percent_state_change", DOUBLE, NDO_DATA_PERCENTSTATECHANGE),
		INIT_PARAM_BI("latency", DOUBLE, NDO_DATA_LATENCY),
		INIT_PARAM_BI("execution_time", DOUBLE, NDO_DATA_EXECUTIONTIME),
		INIT_PARAM_BI("scheduled_downtime_depth", I16, NDO_DATA_SCHEDULEDDOWNTIMEDEPTH),
		INIT_PARAM_BI("failure_prediction_enabled", I8, NDO_DATA_FAILUREPREDICTIONENABLED),
		INIT_PARAM_BI("process_performance_data", I8, NDO_DATA_PROCESSPERFORMANCEDATA),
		INIT_PARAM_BI("obsess_over_service", I8, NDO_DATA_OBSESSOVERSERVICE),
		INIT_PARAM_BI("modified_service_attributes", U32, NDO_DATA_MODIFIEDSERVICEATTRIBUTES),
		INIT_PARAM_BI("event_handler", SHORT_STRING, NDO_DATA_EVENTHANDLER),
		INIT_PARAM_BI("check_command", SHORT_STRING,  NDO_DATA_CHECKCOMMAND),
		INIT_PARAM_BI("normal_check_interval", DOUBLE, NDO_DATA_NORMALCHECKINTERVAL),
		INIT_PARAM_BI("retry_check_interval", DOUBLE, NDO_DATA_RETRYCHECKINTERVAL)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_SERVICESTATUS, SERVICESTATUS);
}


static int ndo2db_stmt_init_configfile(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("configfile_type", I16),
		INIT_PARAM_BI("configfile_path", SHORT_STRING, NDO_DATA_CONFIGFILENAME)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_CONFIGFILE, CONFIGFILES);
}


static int ndo2db_stmt_init_configfilevariable(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("configfile_id", U32),
		INIT_PARAM("varname", SHORT_STRING),
		INIT_PARAM("varvalue", SHORT_STRING)
	};
	return PREPARE_INSERT(HANDLE_CONFIGFILEVARIABLE, CONFIGFILEVARIABLES);
}


static int ndo2db_stmt_init_runtimevariable(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("varname", SHORT_STRING),
		INIT_PARAM("varvalue", SHORT_STRING)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_RUNTIMEVARIABLE, RUNTIMEVARIABLES);
}


static int ndo2db_stmt_init_host(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind host_params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM("check_command_object_id", U32),
		INIT_PARAM("check_command_args", SHORT_STRING),
		INIT_PARAM("eventhandler_command_object_id", U32),
		INIT_PARAM("eventhandler_command_args", SHORT_STRING),
		INIT_PARAM("check_timeperiod_object_id", U32),
		INIT_PARAM("notification_timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_HOSTALIAS),
		INIT_PARAM_BI("display_name", SHORT_STRING, NDO_DATA_DISPLAYNAME),
		INIT_PARAM_BI("address", SHORT_STRING, NDO_DATA_HOSTADDRESS),
		INIT_PARAM_BI("failure_prediction_options", SHORT_STRING, NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS),
		INIT_PARAM_BI("check_interval", DOUBLE, NDO_DATA_HOSTCHECKINTERVAL),
		INIT_PARAM_BI("retry_interval", DOUBLE, NDO_DATA_HOSTRETRYINTERVAL),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_HOSTMAXCHECKATTEMPTS),
		INIT_PARAM_BI("first_notification_delay", DOUBLE, NDO_DATA_FIRSTNOTIFICATIONDELAY),
		INIT_PARAM_BI("notification_interval", DOUBLE, NDO_DATA_HOSTNOTIFICATIONINTERVAL),
		INIT_PARAM_BI("notify_on_down", I8, NDO_DATA_NOTIFYHOSTDOWN),
		INIT_PARAM_BI("notify_on_unreachable", I8, NDO_DATA_NOTIFYHOSTUNREACHABLE),
		INIT_PARAM_BI("notify_on_recovery", I8, NDO_DATA_NOTIFYHOSTRECOVERY),
		INIT_PARAM_BI("notify_on_flapping", I8, NDO_DATA_NOTIFYHOSTFLAPPING),
		INIT_PARAM_BI("notify_on_downtime", I8, NDO_DATA_NOTIFYHOSTDOWNTIME),
		INIT_PARAM_BI("stalk_on_up", I8, NDO_DATA_STALKHOSTONUP),
		INIT_PARAM_BI("stalk_on_down", I8, NDO_DATA_STALKHOSTONDOWN),
		INIT_PARAM_BI("stalk_on_unreachable", I8, NDO_DATA_STALKHOSTONUNREACHABLE),
		INIT_PARAM_BI("flap_detection_enabled", I8, NDO_DATA_HOSTFLAPDETECTIONENABLED),
		INIT_PARAM_BI("flap_detection_on_up", I8, NDO_DATA_FLAPDETECTIONONUP),
		INIT_PARAM_BI("flap_detection_on_down", I8, NDO_DATA_FLAPDETECTIONONDOWN),
		INIT_PARAM_BI("flap_detection_on_unreachable", I8, NDO_DATA_FLAPDETECTIONONUNREACHABLE),
		INIT_PARAM_BI("low_flap_threshold", DOUBLE, NDO_DATA_LOWHOSTFLAPTHRESHOLD),
		INIT_PARAM_BI("high_flap_threshold", DOUBLE, NDO_DATA_HIGHHOSTFLAPTHRESHOLD),
		INIT_PARAM_BI("process_performance_data", I8, NDO_DATA_PROCESSHOSTPERFORMANCEDATA),
		INIT_PARAM_BI("freshness_checks_enabled", I8, NDO_DATA_HOSTFRESHNESSCHECKSENABLED),
		INIT_PARAM_BI("freshness_threshold", I16, NDO_DATA_HOSTFRESHNESSTHRESHOLD),
		INIT_PARAM_BI("passive_checks_enabled", I8, NDO_DATA_PASSIVEHOSTCHECKSENABLED),
		INIT_PARAM_BI("event_handler_enabled", I8, NDO_DATA_HOSTEVENTHANDLERENABLED),
		INIT_PARAM_BI("active_checks_enabled", I8, NDO_DATA_ACTIVEHOSTCHECKSENABLED),
		INIT_PARAM_BI("retain_status_information", I8, NDO_DATA_RETAINHOSTSTATUSINFORMATION),
		INIT_PARAM_BI("retain_nonstatus_information", I8, NDO_DATA_RETAINHOSTNONSTATUSINFORMATION),
		INIT_PARAM_BI("notifications_enabled", I8, NDO_DATA_HOSTNOTIFICATIONSENABLED),
		INIT_PARAM_BI("obsess_over_host", I8, NDO_DATA_OBSESSOVERHOST),
		INIT_PARAM_BI("failure_prediction_enabled", I8, NDO_DATA_HOSTFAILUREPREDICTIONENABLED),
		INIT_PARAM_BI("notes", SHORT_STRING, NDO_DATA_NOTES),
		INIT_PARAM_BI("notes_url", SHORT_STRING, NDO_DATA_NOTESURL),
		INIT_PARAM_BI("action_url", SHORT_STRING, NDO_DATA_ACTIONURL),
		INIT_PARAM_BI("icon_image", SHORT_STRING, NDO_DATA_ICONIMAGE),
		INIT_PARAM_BI("icon_image_alt", SHORT_STRING, NDO_DATA_ICONIMAGEALT),
		INIT_PARAM_BI("vrml_image", SHORT_STRING, NDO_DATA_VRMLIMAGE),
		INIT_PARAM_BI("statusmap_image", SHORT_STRING, NDO_DATA_STATUSMAPIMAGE),
		INIT_PARAM_BI("have_2d_coords", I8, NDO_DATA_HAVE2DCOORDS),
		INIT_PARAM_BI("x_2d", I16, NDO_DATA_X2D),
		INIT_PARAM_BI("y_2d", I16, NDO_DATA_Y3D),
		INIT_PARAM_BI("have_3d_coords", I8, NDO_DATA_HAVE3DCOORDS),
		INIT_PARAM_BI("x_3d", DOUBLE, NDO_DATA_X3D),
		INIT_PARAM_BI("y_3d", DOUBLE, NDO_DATA_Y3D),
		INIT_PARAM_BI("z_3d", DOUBLE, NDO_DATA_Z3D)
#ifdef BUILD_NAGIOS_4X
		,INIT_PARAM_BI("importance", I32, NDO_DATA_IMPORTANCE)
#endif
	};
	static const struct ndo2db_stmt_bind parent_params[] = {
		INIT_PARAM("host_id", U32),
		INIT_PARAM("parent_host_object_id", U32)
	};
	static const struct ndo2db_stmt_bind contactgroup_params[] = {
		INIT_PARAM("host_id", U32),
		INIT_PARAM("contactgroup_object_id", U32)
	};
	static const struct ndo2db_stmt_bind contact_params[] = {
		INIT_PARAM("host_id", U32),
		INIT_PARAM("contact_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_HOST, HOSTS, host_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTPARENT, HOSTPARENTHOSTS, parent_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTCONTACTGROUP, HOSTCONTACTGROUPS, contactgroup_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTCONTACT, HOSTCONTACTS, contact_params));

	return NDO_OK;
}


static int ndo2db_stmt_init_hostgroup(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind group_params[] = {
		INIT_PARAM("hostgroup_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_HOSTGROUPALIAS)
	};
	static const struct ndo2db_stmt_bind member_params[] = {
		INIT_PARAM("hostgroup_id", U32),
		INIT_PARAM("host_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_HOSTGROUP, HOSTGROUPS, group_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTGROUPMEMBER, HOSTGROUPMEMBERS, member_params));

	return NDO_OK;
}


static int ndo2db_stmt_init_service(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind service_params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM("check_command_object_id", U32),
		INIT_PARAM("check_command_args", SHORT_STRING),
		INIT_PARAM("eventhandler_command_object_id", U32),
		INIT_PARAM("eventhandler_command_args", SHORT_STRING),
		INIT_PARAM("check_timeperiod_object_id", U32),
		INIT_PARAM("notification_timeperiod_object_id", U32),
		INIT_PARAM("service_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("display_name", SHORT_STRING, NDO_DATA_DISPLAYNAME),
		INIT_PARAM_BI("failure_prediction_options", SHORT_STRING, NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS),
		INIT_PARAM_BI("check_interval", DOUBLE, NDO_DATA_SERVICECHECKINTERVAL),
		INIT_PARAM_BI("retry_interval", DOUBLE, NDO_DATA_SERVICERETRYINTERVAL),
		INIT_PARAM_BI("max_check_attempts", I16, NDO_DATA_MAXSERVICECHECKATTEMPTS),
		INIT_PARAM_BI("first_notification_delay", DOUBLE, NDO_DATA_FIRSTNOTIFICATIONDELAY),
		INIT_PARAM_BI("notification_interval", DOUBLE, NDO_DATA_SERVICENOTIFICATIONINTERVAL),
		INIT_PARAM_BI("notify_on_warning", I8, NDO_DATA_NOTIFYSERVICEWARNING),
		INIT_PARAM_BI("notify_on_unknown", I8, NDO_DATA_NOTIFYSERVICEUNKNOWN),
		INIT_PARAM_BI("notify_on_critical", I8, NDO_DATA_NOTIFYSERVICECRITICAL),
		INIT_PARAM_BI("notify_on_recovery", I8, NDO_DATA_NOTIFYSERVICERECOVERY),
		INIT_PARAM_BI("notify_on_flapping", I8, NDO_DATA_NOTIFYSERVICEFLAPPING),
		INIT_PARAM_BI("notify_on_downtime", I8, NDO_DATA_NOTIFYSERVICEDOWNTIME),
		INIT_PARAM_BI("stalk_on_ok", I8, NDO_DATA_STALKSERVICEONOK),
		INIT_PARAM_BI("stalk_on_warning", I8, NDO_DATA_STALKSERVICEONWARNING),
		INIT_PARAM_BI("stalk_on_unknown", I8, NDO_DATA_STALKSERVICEONUNKNOWN),
		INIT_PARAM_BI("stalk_on_critical", I8, NDO_DATA_STALKSERVICEONCRITICAL),
		INIT_PARAM_BI("is_volatile", I8, NDO_DATA_SERVICEISVOLATILE),
		INIT_PARAM_BI("flap_detection_enabled", I8, NDO_DATA_SERVICEFLAPDETECTIONENABLED),
		INIT_PARAM_BI("flap_detection_on_ok", I8, NDO_DATA_FLAPDETECTIONONOK),
		INIT_PARAM_BI("flap_detection_on_warning", I8, NDO_DATA_FLAPDETECTIONONWARNING),
		INIT_PARAM_BI("flap_detection_on_unknown", I8, NDO_DATA_FLAPDETECTIONONUNKNOWN),
		INIT_PARAM_BI("flap_detection_on_critical", I8, NDO_DATA_FLAPDETECTIONONCRITICAL),
		INIT_PARAM_BI("low_flap_threshold", DOUBLE, NDO_DATA_LOWSERVICEFLAPTHRESHOLD),
		INIT_PARAM_BI("high_flap_threshold", DOUBLE, NDO_DATA_HIGHSERVICEFLAPTHRESHOLD),
		INIT_PARAM_BI("process_performance_data", I8, NDO_DATA_PROCESSSERVICEPERFORMANCEDATA),
		INIT_PARAM_BI("freshness_checks_enabled", I8, NDO_DATA_SERVICEFRESHNESSCHECKSENABLED),
		INIT_PARAM_BI("freshness_threshold", I16, NDO_DATA_SERVICEFRESHNESSTHRESHOLD),
		INIT_PARAM_BI("passive_checks_enabled", I8, NDO_DATA_PASSIVESERVICECHECKSENABLED),
		INIT_PARAM_BI("event_handler_enabled", I8, NDO_DATA_SERVICEEVENTHANDLERENABLED),
		INIT_PARAM_BI("active_checks_enabled", I8, NDO_DATA_ACTIVESERVICECHECKSENABLED),
		INIT_PARAM_BI("retain_status_information", I8, NDO_DATA_RETAINSERVICESTATUSINFORMATION),
		INIT_PARAM_BI("retain_nonstatus_information", I8, NDO_DATA_RETAINSERVICENONSTATUSINFORMATION),
		INIT_PARAM_BI("notifications_enabled", I8, NDO_DATA_SERVICENOTIFICATIONSENABLED),
		INIT_PARAM_BI("obsess_over_service", I8, NDO_DATA_OBSESSOVERSERVICE),
		INIT_PARAM_BI("failure_prediction_enabled", I8, NDO_DATA_SERVICEFAILUREPREDICTIONENABLED),
		INIT_PARAM_BI("notes", SHORT_STRING, NDO_DATA_NOTES),
		INIT_PARAM_BI("notes_url", SHORT_STRING, NDO_DATA_NOTESURL),
		INIT_PARAM_BI("action_url", SHORT_STRING, NDO_DATA_ACTIONURL),
		INIT_PARAM_BI("icon_image", SHORT_STRING, NDO_DATA_ICONIMAGE),
		INIT_PARAM_BI("icon_image_alt", SHORT_STRING, NDO_DATA_ICONIMAGEALT)
#ifdef BUILD_NAGIOS_4X
		,INIT_PARAM_BI("importance", I32, NDO_DATA_IMPORTANCE)
#endif
	};
#ifdef BUILD_NAGIOS_4X
	static const struct ndo2db_stmt_bind parent_params[] = {
		INIT_PARAM("service_id", U32),
		INIT_PARAM("parent_service_object_id", U32)
	};
#endif
	static const struct ndo2db_stmt_bind contactgroup_params[] = {
		INIT_PARAM("service_id", U32),
		INIT_PARAM("contactgroup_object_id", U32)
	};
	static const struct ndo2db_stmt_bind contact_params[] = {
		INIT_PARAM("service_id", U32),
		INIT_PARAM("contact_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_SERVICE, SERVICES, service_params));
#ifdef BUILD_NAGIOS_4X
	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICEPARENT, SERVICEPARENTSERVICES, parent_params));
#endif
	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICECONTACTGROUP, SERVICECONTACTGROUPS, contactgroup_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICECONTACT, SERVICECONTACTS, contact_params));

	return NDO_OK;
}


static int ndo2db_stmt_init_servicegroup(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind group_params[] = {
		INIT_PARAM("servicegroup_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_SERVICEGROUPALIAS)
	};
	static const struct ndo2db_stmt_bind member_params[] = {
		INIT_PARAM("servicegroup_id", U32),
		INIT_PARAM("service_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_SERVICEGROUP, SERVICEGROUPS, group_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICEGROUPMEMBER, SERVICEGROUPMEMBERS, member_params));

	return NDO_OK;
}


static int ndo2db_stmt_init_hostdependency(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM("dependent_host_object_id", U32),
		INIT_PARAM("timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("dependency_type", I8, NDO_DATA_DEPENDENCYTYPE),
		INIT_PARAM_BI("inherits_parent", I8, NDO_DATA_INHERITSPARENT),
		INIT_PARAM_BI("fail_on_up", I8, NDO_DATA_FAILONUP),
		INIT_PARAM_BI("fail_on_down", I8, NDO_DATA_FAILONDOWN),
		INIT_PARAM_BI("fail_on_unreachable", I8, NDO_DATA_FAILONUNREACHABLE)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_HOSTDEPENDENCY, HOSTDEPENDENCIES);
}


static int ndo2db_stmt_init_servicedependency(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("service_object_id", U32),
		INIT_PARAM("dependent_service_object_id", U32),
		INIT_PARAM("timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("dependency_type", I8, NDO_DATA_DEPENDENCYTYPE),
		INIT_PARAM_BI("inherits_parent", I8, NDO_DATA_INHERITSPARENT),
		INIT_PARAM_BI("fail_on_ok", I8, NDO_DATA_FAILONOK),
		INIT_PARAM_BI("fail_on_warning", I8, NDO_DATA_FAILONWARNING),
		INIT_PARAM_BI("fail_on_unknown", I8, NDO_DATA_FAILONUNKNOWN),
		INIT_PARAM_BI("fail_on_critical", I8, NDO_DATA_FAILONCRITICAL)
	};
	return PREPARE_INSERT_UPDATE(HANDLE_SERVICEDEPENDENCY, SERVICEDEPENDENCIES);
}


static int ndo2db_stmt_init_hostescalation(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind escalation_params[] = {
		INIT_PARAM("host_object_id", U32),
		INIT_PARAM("timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("first_notification", I16, NDO_DATA_FIRSTNOTIFICATION),
		INIT_PARAM_BI("last_notification", I16, NDO_DATA_LASTNOTIFICATION),
		INIT_PARAM_BI("notification_interval", DOUBLE, NDO_DATA_NOTIFICATIONINTERVAL),
		INIT_PARAM_BI("escalate_on_recovery", I8, NDO_DATA_ESCALATEONRECOVERY),
		INIT_PARAM_BI("escalate_on_down", I8, NDO_DATA_ESCALATEONDOWN),
		INIT_PARAM_BI("escalate_on_unreachable", I8, NDO_DATA_ESCALATEONUNREACHABLE)
	};
	static const struct ndo2db_stmt_bind contactgroup_params[] = {
		INIT_PARAM("hostescalation_id", U32),
		INIT_PARAM("contactgroup_object_id", U32)
	};
	static const struct ndo2db_stmt_bind contact_params[] = {
		INIT_PARAM("hostescalation_id", U32),
		INIT_PARAM("contact_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_HOSTESCALATION, HOSTESCALATIONS, escalation_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTESCALATIONCONTACTGROUP, HOSTESCALATIONCONTACTGROUPS,
			contactgroup_params));

	return PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_HOSTESCALATIONCONTACT, HOSTESCALATIONCONTACTS, contact_params);
}


static int ndo2db_stmt_init_serviceescalation(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind escalation_params[] = {
		INIT_PARAM("service_object_id", U32),
		INIT_PARAM("timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("first_notification", I16, NDO_DATA_FIRSTNOTIFICATION),
		INIT_PARAM_BI("last_notification", I16, NDO_DATA_LASTNOTIFICATION),
		INIT_PARAM_BI("notification_interval", DOUBLE, NDO_DATA_NOTIFICATIONINTERVAL),
		INIT_PARAM_BI("escalate_on_recovery", I8, NDO_DATA_ESCALATEONRECOVERY),
		INIT_PARAM_BI("escalate_on_warning", I8, NDO_DATA_ESCALATEONWARNING),
		INIT_PARAM_BI("escalate_on_unknown", I8, NDO_DATA_ESCALATEONUNKNOWN),
		INIT_PARAM_BI("escalate_on_critical", I8, NDO_DATA_ESCALATEONCRITICAL)
	};
	static const struct ndo2db_stmt_bind contactgroup_params[] = {
		INIT_PARAM("serviceescalation_id", U32),
		INIT_PARAM("contactgroup_object_id", U32)
	};
	static const struct ndo2db_stmt_bind contact_params[] = {
		INIT_PARAM("serviceescalation_id", U32),
		INIT_PARAM("contact_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			HANDLE_SERVICEESCALATION, SERVICEESCALATIONS, escalation_params));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICEESCALATIONCONTACTGROUP, SERVICEESCALATIONCONTACTGROUPS,
			contactgroup_params));

	return PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_SERVICEESCALATIONCONTACT, SERVICEESCALATIONCONTACTS, contact_params);
}


static int ndo2db_stmt_init_command(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("command_line", SHORT_STRING, NDO_DATA_COMMANDLINE)
	};

	return PREPARE_INSERT_UPDATE(HANDLE_COMMAND, COMMANDS);
}


static int ndo2db_stmt_init_timeperiod(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_TIMEPERIODALIAS)
	};
	static const struct ndo2db_stmt_bind range_params[] = {
		INIT_PARAM("timeperiod_id", U32),
		INIT_PARAM("day", I16),
		INIT_PARAM("start_sec", U32),
		INIT_PARAM("end_sec", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE(HANDLE_TIMEPERIOD, TIMEPERIODS));

	return PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_TIMEPERIODRANGE, TIMEPERIODTIMERANGES, range_params);
}


static int ndo2db_stmt_init_contact(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("contact_object_id", U32),
		INIT_PARAM("host_timeperiod_object_id", U32),
		INIT_PARAM("service_timeperiod_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_CONTACTALIAS),
		INIT_PARAM_BI("email_address", SHORT_STRING, NDO_DATA_EMAILADDRESS),
		INIT_PARAM_BI("pager_address", SHORT_STRING, NDO_DATA_PAGERADDRESS),
		INIT_PARAM_BI("host_notifications_enabled", I8, NDO_DATA_HOSTNOTIFICATIONSENABLED),
		INIT_PARAM_BI("service_notifications_enabled", I8, NDO_DATA_SERVICENOTIFICATIONSENABLED),
		INIT_PARAM_BI("can_submit_commands", I8, NDO_DATA_CANSUBMITCOMMANDS),
		INIT_PARAM_BI("notify_service_recovery", I8, NDO_DATA_NOTIFYSERVICERECOVERY),
		INIT_PARAM_BI("notify_service_warning", I8, NDO_DATA_NOTIFYSERVICEWARNING),
		INIT_PARAM_BI("notify_service_unknown", I8, NDO_DATA_NOTIFYSERVICEUNKNOWN),
		INIT_PARAM_BI("notify_service_critical", I8, NDO_DATA_NOTIFYSERVICECRITICAL),
		INIT_PARAM_BI("notify_service_flapping", I8, NDO_DATA_NOTIFYSERVICEFLAPPING),
		INIT_PARAM_BI("notify_service_downtime", I8, NDO_DATA_NOTIFYSERVICEDOWNTIME),
		INIT_PARAM_BI("notify_host_recovery", I8, NDO_DATA_NOTIFYHOSTRECOVERY),
		INIT_PARAM_BI("notify_host_down", I8, NDO_DATA_NOTIFYHOSTDOWN),
		INIT_PARAM_BI("notify_host_unreachable", I8, NDO_DATA_NOTIFYHOSTUNREACHABLE),
		INIT_PARAM_BI("notify_host_flapping", I8, NDO_DATA_NOTIFYHOSTFLAPPING),
		INIT_PARAM_BI("notify_host_downtime", I8, NDO_DATA_NOTIFYHOSTDOWNTIME)
#ifdef BUILD_NAGIOS_4X
		,INIT_PARAM_BI("minimum_importance", I32, NDO_DATA_MINIMUMIMPORTANCE)
#endif
	};
	static const struct ndo2db_stmt_bind address_params[] = {
		INIT_PARAM("contact_id", U32),
		INIT_PARAM("address_number", I16),
		INIT_PARAM("address", SHORT_STRING)
	};
	static const struct ndo2db_stmt_bind notif_params[] = {
		INIT_PARAM("contact_id", U32),
		INIT_PARAM("notification_type", I8),
		INIT_PARAM("command_object_id", U32),
		INIT_PARAM("command_args", SHORT_STRING)
	};

	CHK_OK(PREPARE_INSERT_UPDATE(HANDLE_CONTACT, CONTACTS));

	CHK_OK(PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_CONTACTADDRESS, CONTACTADDRESSES, address_params));

	return PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_CONTACTNOTIFICATIONCOMMAND, CONTACTNOTIFICATIONCOMMANDS,	notif_params);
}


static int ndo2db_stmt_init_contactgroup(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("contactgroup_object_id", U32),
		INIT_PARAM_F("config_type", I8, BIND_CURRENT_CONFIG_TYPE|BIND_BUFFERED_INPUT),
		INIT_PARAM_BI("alias", SHORT_STRING, NDO_DATA_CONTACTGROUPALIAS)
	};
	static const struct ndo2db_stmt_bind mamber_params[] = {
		INIT_PARAM("contactgroup_id", U32),
		INIT_PARAM("contact_object_id", U32)
	};

	CHK_OK(PREPARE_INSERT_UPDATE(HANDLE_CONTACTGROUP, CONTACTGROUPS));

	return PREPARE_INSERT_UPDATE_W_PARAMS(
			SAVE_CONTACTGROUPMEMBER, CONTACTGROUPMEMBERS, mamber_params);
}


static int ndo2db_stmt_init_customvariable(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("object_id", U32),
		INIT_PARAM("config_type", I8),
		INIT_PARAM("has_been_modified", I8),
		INIT_PARAM("varname", SHORT_STRING),
		INIT_PARAM("varvalue", SHORT_STRING)
	};
	return PREPARE_INSERT_UPDATE(SAVE_CUSTOMVARIABLE, CUSTOMVARIABLES);
}


static int ndo2db_stmt_init_customvariablestatus(ndo2db_idi *idi, ndo_dbuf *dbuf) {

	static const struct ndo2db_stmt_bind params[] = {
		INIT_PARAM("object_id", U32),
		INIT_PARAM("status_update_time", FROM_UNIXTIME),
		INIT_PARAM("has_been_modified", I8),
		INIT_PARAM("varname", SHORT_STRING),
		INIT_PARAM("varvalue", SHORT_STRING)
	};
	return PREPARE_INSERT_UPDATE(SAVE_CUSTOMVARIABLESTATUS, CUSTOMVARIABLESTATUS);
}
