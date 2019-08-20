
#define MYSQL_RESET_BIND() \
do { \
    memset(ndo_bind, 0, sizeof(ndo_bind)); \
    ndo_bind_i = 0; \
} while(0)


#define MYSQL_BIND() \
do { \
    ndo_return = mysql_stmt_bind_params(ndo_stmt, ndo_bind); \
    NDO_HANDLE_ERROR("Unable to bind parameters"); \
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
    ndo_tmp_str_len[ndo_bind_i]        = strlen(_buffer); \
    \
    ndo_bind[ndo_bind_i].buffer_type   = MYSQL_TYPE_STRING; \
    ndo_bind[ndo_bind_i].buffer_length = MAX_BIND_BUFFER; \
    ndo_bind[ndo_bind_i].buffer        = _buffer; \
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
    NDO_HANDLE_ERROR("Unable to prepare statement"); \
} while (0)


#define MYSQL_EXECUTE() \
do { \
    ndo_return = mysql_stmt_execute(ndo_stmt); \
    NDO_HANDLE_ERROR("Unable to execute statement"); \
} while (0)

