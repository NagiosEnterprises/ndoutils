
#define MYSQL_RESET_BIND() \
do { \
    memset(ndo_bind, 0, sizeof(ndo_bind)); \
    ndo_bind_i = 0; \
} while (0)


#define MYSQL_BIND() \
do { \
    ndo_return = mysql_stmt_bind_param(ndo_stmt, ndo_bind); \
    NDO_HANDLE_ERROR_BIND_STMT(ndo_stmt, ndo_bind); \
} while (0)


#define MYSQL_BIND_INT(_buffer) MYSQL_BIND_LONG(_buffer)

#define MYSQL_BIND_LONG(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_LONG; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_LONGLONG(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_TINY(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_TINY; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_SHORT(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_SHORT; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_FLOAT(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_FLOAT; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_DOUBLE(_buffer) \
do { \
    ndo_bind[ndo_bind_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    ndo_bind[ndo_bind_i].buffer      = &(_buffer); \
    ndo_bind_i++; \
} while (0)

#define MYSQL_BIND_STR(_buffer) \
do { \
\
    ndo_bind[ndo_bind_i].buffer_type   = MYSQL_TYPE_STRING; \
    ndo_bind[ndo_bind_i].buffer_length = MAX_BIND_BUFFER; \
\
    if (_buffer != NULL && strlen(_buffer) > 0) { \
\
        ndo_tmp_str_len[ndo_bind_i]        = strlen(_buffer); \
        ndo_bind[ndo_bind_i].buffer        = _buffer; \
    } \
    else { \
\
        ndo_tmp_str_len[ndo_bind_i]        = 0; \
        ndo_bind[ndo_bind_i].buffer        = ""; \
    } \
    ndo_bind[ndo_bind_i].length        = &(ndo_tmp_str_len[ndo_bind_i]); \
    \
    ndo_bind_i++; \
} while (0)




#define MYSQL_RESET_RESULT() \
do { \
    memset(ndo_result, 0, sizeof(ndo_result)); \
    ndo_result_i = 0; \
} while (0)


#define MYSQL_RESULT_BIND() \
do { \
    ndo_return = mysql_stmt_bind_result(ndo_stmt, ndo_result); \
    NDO_HANDLE_ERROR("Unable to bind result parameters"); \
} while (0)


#define MYSQL_BIND_RESULT_INT(_buffer) MYSQL_BIND_RESULT_LONG(_buffer)

#define MYSQL_BIND_RESULT_LONG(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_LONG; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_LONGLONG(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_TINY(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_TINY; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_SHORT(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_SHORT; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_FLOAT(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_FLOAT; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_DOUBLE(_buffer) \
do { \
    ndo_result[ndo_result_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    ndo_result[ndo_result_i].buffer      = &(_buffer); \
    ndo_result_i++; \
} while (0)

#define MYSQL_BIND_RESULT_STR(_buffer) \
do { \
    ndo_tmp_str_len[ndo_result_i]        = strlen(_buffer); \
    \
    ndo_result[ndo_result_i].buffer_type   = MYSQL_TYPE_STRING; \
    ndo_result[ndo_result_i].buffer_length = MAX_BIND_RESULT_BUFFER; \
    ndo_result[ndo_result_i].buffer        = _buffer; \
    ndo_result[ndo_result_i].length        = &(ndo_result_tmp_str_len[ndo_result_i]); \
    \
    ndo_result_i++; \
} while (0)



#define MYSQL_RESET_SQL() memset(ndo_query, 0, sizeof(ndo_query))


#define MYSQL_SET_SQL(_buffer) strncpy(ndo_query, _buffer, MAX_SQL_BUFFER)


#define MYSQL_PREPARE() \
do { \
    ndo_return = mysql_stmt_prepare(ndo_stmt, ndo_query, strlen(ndo_query)); \
    NDO_HANDLE_ERROR_PREPARE_STMT(ndo_stmt, ndo_query); \
} while (0)


#define MYSQL_EXECUTE() \
do { \
    ndo_return = mysql_stmt_execute(ndo_stmt); \
    NDO_HANDLE_ERROR_EXECUTE_STMT(ndo_stmt); \
} while (0)


#define MYSQL_SET_QUERY(query) \
do { \
    ndo_current_query = query; \
} while (0)









/****************************************************************/


#define NDO_HANDLE_ERROR_NEW(_which, err) NDO_HANDLE_ERROR_STMT(err, ndo_stmt_new[_which])


#define MYSQL_PREPARE_NEW(_which, _query) \
do { \
    ndo_return = mysql_stmt_prepare(ndo_stmt_new[_which], _query, strlen(_query)); \
    NDO_HANDLE_ERROR_PREPARE_STMT(ndo_stmt_new[_which], _query); \
} while (0)


#define MYSQL_EXECUTE_NEW(_which) \
do { \
    ndo_return = mysql_stmt_execute(ndo_stmt_new[_which]); \
    NDO_HANDLE_ERROR_EXECUTE_STMT(ndo_stmt_new[_which]); \
} while (0)



#define MYSQL_RESET_BIND_NEW(_which) \
do { \
    memset(ndo_bind_new[_which], 0, sizeof(ndo_bind_new[_which])); \
    ndo_bind_new_i[_which] = 0; \
} while (0)


#define MYSQL_BIND_NEW(_which) \
do { \
    ndo_return = mysql_stmt_bind_param(ndo_stmt_new[_which], ndo_bind_new[_which]); \
    NDO_HANDLE_ERROR_BIND_STMT(ndo_stmt_new[_which], ndo_bind_new[_which]); \
} while (0)


#define MYSQL_BIND_NEW_INT(_which, _buffer) MYSQL_BIND_NEW_LONG(_which, _buffer)

#define MYSQL_BIND_NEW_LONG(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_LONG; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new_i[_which]++; \
} while (0)

#define MYSQL_BIND_NEW_LONGLONG(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_LONGLONG; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new_i[_which]++; \
} while (0)

#define MYSQL_BIND_NEW_TINY(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_TINY; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new[_which]_i++; \
} while (0)

#define MYSQL_BIND_NEW_SHORT(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_SHORT; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new_i[_which]++; \
} while (0)

#define MYSQL_BIND_NEW_FLOAT(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_FLOAT; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new_i[_which]++; \
} while (0)

#define MYSQL_BIND_NEW_DOUBLE(_which, _buffer) \
do { \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type = MYSQL_TYPE_DOUBLE; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer      = &(_buffer); \
    ndo_bind_new_i[_which]++; \
} while (0)

#define MYSQL_BIND_NEW_STR(_which, _buffer) \
do { \
\
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_type   = MYSQL_TYPE_STRING; \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer_length = MAX_BIND_BUFFER; \
\
    if (_buffer != NULL && strlen(_buffer) > 0) { \
\
        ndo_tmp_str_len_new[_which][ndo_bind_new_i[_which]]        = strlen(_buffer); \
        ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer        = _buffer; \
    } \
    else { \
\
        ndo_tmp_str_len_new[_which][ndo_bind_new_i[_which]]        = 0; \
        ndo_bind_new[_which][ndo_bind_new_i[_which]].buffer        = ""; \
    } \
    ndo_bind_new[_which][ndo_bind_new_i[_which]].length        = &(ndo_tmp_str_len_new[_which][ndo_bind_new_i[_which]]); \
    \
    ndo_bind_new_i[_which]++; \
} while (0)


#define _MYSQL_BIND_NUMERICAL(type, bind, i, var) \
do { \
    bind.buffer_type = type; \
    bind.buffer = &(var); \
    i++; \
} while (0)

#define _MYSQL_BIND_STR(bind, i, var, len_var, len) \
do { \
    bind.buffer_type = MYSQL_TYPE_STRING; \
    bind.buffer_length = MAX_BIND_BUFFER; \
    if (var != NULL && len > 0) { \
        len_var = len; \
        bind.buffer = var; \
    } else { \
        len_var = 0; \
        bind.buffer = ""; \
    } \
    bind.length = &(len_var); \
    i++; \
} while (0)

#define _MYSQL_BIND_LONG(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_LONG, bind, i, var)
#define _MYSQL_BIND_INT(bind, i, var) _MYSQL_BIND_LONG(bind, i, var)
#define _MYSQL_BIND_DOUBLE(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_DOUBLE, bind, i, var)
#define _MYSQL_BIND_FLOAT(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_FLOAT, bind, i, var)
#define _MYSQL_BIND_SHORT(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_SHORT, bind, i, var)
#define _MYSQL_BIND_TINY(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_TINY, bind, i, var)
#define _MYSQL_BIND_LONGLONG(bind, i, var) _MYSQL_BIND_NUMERICAL(MYSQL_TYPE_LONGLONG, bind, i, var)

#define WRITE_BIND_LONG(stmt, var) _MYSQL_BIND_LONG(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_INT(stmt, var) WRITE_BIND_LONG(stmt, var)
#define WRITE_BIND_DOUBLE(stmt, var) _MYSQL_BIND_DOUBLE(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_FLOAT(stmt, var) _MYSQL_BIND_FLOAT(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_SHORT(stmt, var) _MYSQL_BIND_SHORT(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_TINY(stmt, var) _MYSQL_BIND_TINY(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)
#define WRITE_BIND_LONGLONG(stmt, var) _MYSQL_BIND_LONGLONG(ndo_write_bind[stmt][ndo_write_i[stmt]], ndo_write_i[stmt], var)

#define MYSQL_BIND_LONG(stmt, var) _MYSQL_BIND_LONG(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_INT(stmt, var) MYSQL_BIND_LONG(stmt, var)
#define MYSQL_BIND_DOUBLE(stmt, var) _MYSQL_BIND_DOUBLE(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_FLOAT(stmt, var) _MYSQL_BIND_FLOAT(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_SHORT(stmt, var) _MYSQL_BIND_SHORT(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_TINY(stmt, var) _MYSQL_BIND_TINY(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)
#define MYSQL_BIND_LONGLONG(stmt, var) _MYSQL_BIND_LONGLONG(ndo_sql[stmt].bind[ndo_sql[stmt].bind_i], ndo_sql[stmt].bind_i, var)