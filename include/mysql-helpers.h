
/* error reporting / handling */

/*
    we use a global for returning from mysql functions so that we can standardize
    how errors are handled

    this is the same reason that almost any function utilizing the helper
    macros defined below have int return types
*/

#define NDO_HANDLE_ERROR_STMT(err, stmt) \
do { \
    if (ndo_return != 0) { \
        snprintf(ndo_error_msg, 1023, "ndo_return = %d (%s)", ndo_return, mysql_stmt_error(stmt)); \
        ndo_log(ndo_error_msg); \
        NDO_REPORT_ERROR(err); \
        return NDO_ERROR; \
    } \
} while (0)

#define NDO_HANDLE_ERROR_BIND_STMT(stmt, bind) \
do { \
    if (ndo_return != 0) { \
        int ndo_mysql_errno = *mysql_stmt_error(stmt); \
        if (ndo_mysql_errno == CR_SERVER_GONE_ERROR || ndo_mysql_errno == CR_SERVER_LOST) { \
            while (ndo_return != NDO_OK) { \
                sleep(1); \
                ndo_return = ndo_initialize_database(); \
            } \
            ndo_return = mysql_stmt_bind_param(stmt, bind); \
        } \
        NDO_HANDLE_ERROR_STMT("Unable to bind parameters", stmt); \
    } \
} while (0)

#define NDO_HANDLE_ERROR_RESULT_STMT(stmt, result) \
do { \
    if (ndo_return != 0) { \
        int ndo_mysql_errno = *mysql_stmt_error(stmt); \
        if (ndo_mysql_errno == CR_SERVER_GONE_ERROR || ndo_mysql_errno == CR_SERVER_LOST) { \
            while (ndo_return != NDO_OK) { \
                sleep(1); \
                ndo_return = ndo_initialize_database(); \
            } \
            ndo_return = mysql_stmt_bind_result(stmt, result); \
        } \
        NDO_HANDLE_ERROR_STMT("Unable to bind results", stmt); \
    } \
} while (0)

#define NDO_HANDLE_ERROR_STORE_STMT(stmt) \
do { \
    if (ndo_return != 0) { \
        int ndo_mysql_errno = *mysql_stmt_error(stmt); \
        if (ndo_mysql_errno == CR_SERVER_GONE_ERROR || ndo_mysql_errno == CR_SERVER_LOST) { \
            while (ndo_return != NDO_OK) { \
                sleep(1); \
                ndo_return = ndo_initialize_database(); \
            } \
            ndo_return = mysql_stmt_store_result(stmt); \
        } \
        NDO_HANDLE_ERROR_STMT("Unable to store results", stmt); \
    } \
} while (0)

#define NDO_HANDLE_ERROR_EXECUTE_STMT(stmt) \
do { \
    if (ndo_return != 0) { \
        int ndo_mysql_errno = *mysql_stmt_error(stmt); \
        if (ndo_mysql_errno == CR_SERVER_GONE_ERROR || ndo_mysql_errno == CR_SERVER_LOST) { \
            while (ndo_return != NDO_OK) { \
                sleep(1); \
                ndo_return = ndo_initialize_database(); \
            } \
            ndo_return = mysql_stmt_execute(stmt); \
        } \
        NDO_HANDLE_ERROR_STMT("Unable to execute statement", stmt); \
    } \
} while (0)

#define NDO_HANDLE_ERROR_PREPARE_STMT(stmt, query) \
do { \
    if (ndo_return != 0) { \
        int ndo_mysql_errno = *mysql_stmt_error(stmt); \
        if (ndo_mysql_errno == CR_SERVER_GONE_ERROR || ndo_mysql_errno == CR_SERVER_LOST) { \
            while (ndo_return != NDO_OK) { \
                sleep(1); \
                ndo_return = ndo_initialize_database(); \
            } \
            ndo_return = mysql_stmt_prepare(stmt, query, strlen(query)); \
        } \
        NDO_HANDLE_ERROR_STMT("Unable to prepare statement", stmt); \
    } \
} while (0)


/* mysql generic helpers */

#define _MYSQL_BIND_NUMERICAL(type, bind, i, var) \
do { \
    bind.buffer_type = type; \
    bind.buffer = &(var); \
    i++; \
} while (0)

#define _MYSQL_BIND_LONG(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_LONG, bind, i, var)
#define _MYSQL_BIND_INT(bind, i, var) _MYSQL_BIND_LONG(bind, i, var)
#define _MYSQL_BIND_DOUBLE(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_DOUBLE, bind, i, var)
#define _MYSQL_BIND_FLOAT(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_FLOAT, bind, i, var)
#define _MYSQL_BIND_SHORT(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_SHORT, bind, i, var)
#define _MYSQL_BIND_TINY(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_TINY, bind, i, var)
#define _MYSQL_BIND_LONGLONG(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_LONGLONG, bind, i, var)

#define _MYSQL_BIND_STR(bind, i, var, len_var, len) \
do { \
    bind.buffer_type = MYSQL_TYPE_STRING; \
    bind.buffer_length = MAX_BIND_BUFFER; \
    if (var != NULL) { \
        len_var = len; \
        bind.buffer = var; \
    } else { \
        len_var = 0; \
        bind.buffer = ""; \
    } \
    bind.length = &(len_var); \
    i++; \
} while (0)


#define _MYSQL_RESET_BIND(bind, size, i) \
do { \
    memset(bind, 0, size); \
    i = 0; \
} while (0)

#define _MYSQL_BIND(stmt, bind) \
do { \
    ndo_return = mysql_stmt_bind_param(stmt, bind); \
    NDO_HANDLE_ERROR_BIND_STMT(stmt, bind); \
} while (0)

#define _MYSQL_BIND_RESULT(stmt, result) \
do { \
    ndo_return = mysql_stmt_bind_result(stmt, result); \
    NDO_HANDLE_ERROR_RESULT_STMT(stmt, result); \
} while (0)

#define _MYSQL_STORE_RESULT(stmt) \
do { \
    ndo_return = mysql_stmt_store_result(stmt); \
    NDO_HANDLE_ERROR_STORE_STMT(stmt); \
} while (0)

#define _MYSQL_PREPARE(stmt, query) \
do { \
    ndo_return = mysql_stmt_prepare(stmt, query, strlen(query)); \
    NDO_HANDLE_ERROR_PREPARE_STMT(stmt, query); \
} while (0)

#define _MYSQL_EXECUTE(stmt) \
do { \
    ndo_return = mysql_stmt_execute(stmt); \
    NDO_HANDLE_ERROR_EXECUTE_STMT(stmt); \
} while (0)


/* startup/writing mysql helpers */

/*
    all of these queries and statements, etc. are contained in the 
    ndo_write_* family of global vars
*/

#define WRITE_BIND_LONG(stmt, var) _MYSQL_BIND_LONG(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_INT(stmt, var) WRITE_BIND_LONG(stmt, var)
#define WRITE_BIND_DOUBLE(stmt, var) _MYSQL_BIND_DOUBLE(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_FLOAT(stmt, var) _MYSQL_BIND_FLOAT(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_SHORT(stmt, var) _MYSQL_BIND_SHORT(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_TINY(stmt, var) _MYSQL_BIND_TINY(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_LONGLONG(stmt, var) _MYSQL_BIND_LONGLONG(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_STR(stmt, var) _MYSQL_BIND_STR(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var, ndo_write_tmp_len[stmt][ndo_write_i[stmt]], strlen(var))

#define WRITE_RESET_SQL() memset(ndo_query, 0, sizeof(ndo_query))
#define WRITE_SET_SQL(query) strcpy(ndo_query, query)
#define WRITE_RESET_BIND(stmt) _MYSQL_RESET_BIND(ndo_write_bind[stmt], sizeof(ndo_write_bind[stmt]), ndo_write_i[stmt])
#define WRITE_BIND(stmt) _MYSQL_BIND(ndo_write_stmt[stmt], ndo_write_bind[stmt])
#define WRITE_PREPARE(stmt) _MYSQL_PREPARE(ndo_write_stmt[stmt], ndo_query)
#define WRITE_EXECUTE(stmt) _MYSQL_EXECUTE(ndo_write_stmt[stmt])


/* handler and miscellaneous function mysql helpers */

/*
    all of these revolve around the usage of the ndo_sql global,
    which is a ndo_query_data struct
*/

#define MYSQL_BIND_LONG(stmt, var) _MYSQL_BIND_LONG(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_INT(stmt, var) MYSQL_BIND_LONG(stmt, var)
#define MYSQL_BIND_DOUBLE(stmt, var) _MYSQL_BIND_DOUBLE(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_FLOAT(stmt, var) _MYSQL_BIND_FLOAT(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_SHORT(stmt, var) _MYSQL_BIND_SHORT(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_TINY(stmt, var) _MYSQL_BIND_TINY(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_LONGLONG(stmt, var) _MYSQL_BIND_LONGLONG(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_STR(stmt, var) _MYSQL_BIND_STR(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var, ndo_sql[stmt].strlen[ndo_sql[stmt].bind_i], strlen(var))

#define MYSQL_RESULT_LONG(stmt, var) _MYSQL_BIND_LONG(ndo_sql[stmt].result[ndo_sql[stmt].result_i], ndo_sql[stmt].result_i, var)
#define MYSQL_RESULT_INT(stmt, var) MYSQL_RESULT_LONG(stmt, var)

/* #define MYSQL_RESET_BIND(stmt) memset(ndo_sql[stmt].bind, 0, (sizeof(MYSQL_BIND) * num_bindings[stmt])) */
#define MYSQL_RESET_BIND(stmt) ndo_sql[stmt].bind_i = 0
#define MYSQL_BIND(_stmt) _MYSQL_BIND(ndo_sql[_stmt].stmt, ndo_sql[_stmt].bind)
#define MYSQL_RESET_RESULT(stmt) ndo_sql[stmt].result_i = 0
#define MYSQL_BIND_RESULT(_stmt) _MYSQL_BIND_RESULT(ndo_sql[_stmt].stmt, ndo_sql[_stmt].result)
#define MYSQL_STORE_RESULT(_stmt) _MYSQL_STORE_RESULT(ndo_sql[_stmt].stmt)
#define MYSQL_FETCH(_stmt) mysql_stmt_fetch(ndo_sql[_stmt].stmt)
#define MYSQL_EXECUTE(_stmt) _MYSQL_EXECUTE(ndo_sql[_stmt].stmt)

/*
    these only work for the GENERIC ndo_sql data, as everything is else is set
    and prepared already
*/

#define GENERIC_RESET_SQL() memset(ndo_sql[GENERIC].query, 0, MAX_SQL_BUFFER)
#define GENERIC_SET_SQL(_query) strcpy(ndo_sql[GENERIC].query, _query)


/*
    the rest is just to make it easier to write and read queries
    that utilize the generic sql struct
*/

#define GENERIC_RESET_BIND() _MYSQL_RESET_BIND(ndo_sql[GENERIC].bind, num_bindings[GENERIC], ndo_sql[GENERIC].bind_i)
#define GENERIC_BIND() MYSQL_BIND(GENERIC)

#define GENERIC_EXECUTE() MYSQL_EXECUTE(GENERIC)
#define GENERIC_PREPARE() _MYSQL_PREPARE(ndo_sql[GENERIC].stmt, ndo_sql[GENERIC].query)

#define GENERIC_BIND_LONG(var) MYSQL_BIND_LONG(GENERIC, var)
#define GENERIC_BIND_INT(var) MYSQL_BIND_LONG(GENERIC, var)
#define GENERIC_BIND_DOUBLE(var) MYSQL_BIND_DOUBLE(GENERIC, var)
#define GENERIC_BIND_FLOAT(var) MYSQL_BIND_FLOAT(GENERIC, var)
#define GENERIC_BIND_SHORT(var) MYSQL_BIND_SHORT(GENERIC, var)
#define GENERIC_BIND_TINY(var) MYSQL_BIND_TINY(GENERIC, var)
#define GENERIC_BIND_LONGLONG(var) MYSQL_BIND_LONGLONG(GENERIC, var)
#define GENERIC_BIND_STR(var) MYSQL_BIND_STR(GENERIC, var)
