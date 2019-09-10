/*

gcc -O0 -g3 bug-test.c
./a.out

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_OBJECT_INSERT 51
#define MAX_SQL_BUFFER ((MAX_OBJECT_INSERT * 150) + 8000)

int ndo_max_object_insert_count;

int func1()
{
    int var_count = 0;
    char var_query[MAX_SQL_BUFFER];
    char * var_query_base = "INSERT INTO nagios_customvariables (instance_id, object_id, config_type, has_been_modified, varname, varvalue) VALUES ";
    size_t var_query_base_len = strlen(var_query_base);
    size_t var_query_len = var_query_base_len;
    char * var_query_values = "(1,(SELECT object_id FROM nagios_objects WHERE objecttype_id = 1 AND name1 = ?),?,?,?,?),";
    size_t var_query_values_len = strlen(var_query_values);
    char * var_query_on_update = " ON DUPLICATE KEY UPDATE instance_id = VALUES(instance_id), object_id = VALUES(object_id), config_type = VALUES(config_type), has_been_modified = VALUES(has_been_modified), varname = VALUES(varname), varvalue = VALUES(varvalue)";
    size_t var_query_on_update_len = strlen(var_query_on_update);

    strcpy(var_query, var_query_base);

    while (1) {

        strcat(var_query, var_query_values);
        var_query_len += var_query_values_len;

        var_count++;

        if (var_count >= ndo_max_object_insert_count) {
            send_subquery(var_query, var_query_base_len, &var_query_len, var_query_on_update_len, var_query_on_update, 0, &var_count);
        }
    }
}


int send_subquery(char * query, size_t query_base_len, size_t * query_len, size_t query_on_update_len, char * query_on_update, int stmt, int * counter)
{
    strcpy(query + (* query_len) - 1, query_on_update);

    memset(query + query_base_len, 0, MAX_SQL_BUFFER - query_base_len);

    *query_len = 0;
    *counter = 0;
}


int main()
{
    ndo_max_object_insert_count = 50;
    func1();
}
