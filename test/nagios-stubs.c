
int neb_register_callback(int callback_type, void * mod_handle, int priority, int (*callback_func)(int, void *))
{
    return NDO_OK;
}

int neb_deregister_callback(int callback_type, int (*callback_func)(int, void *))
{
    return NDO_OK;
}

char * get_program_version()
{
    return "1.0.0";
}

char * get_program_modification_date()
{
    return "2020-01-28";
}

struct scheduled_downtime * find_downtime(int i, unsigned long l)
{
    struct scheduled_downtime * dwn = malloc(sizeof(struct scheduled_downtime));
    return dwn;
}

int write_to_log(char * buffer, unsigned long data_type, time_t * timestamp)
{
    printf("%s\n", buffer);
}

nagios_comment * find_service_comment(unsigned long l)
{
    nagios_comment * comment = malloc(sizeof(nagios_comment));
    return comment;
}

nagios_comment * find_host_comment(unsigned long l)
{
    nagios_comment * comment = malloc(sizeof(nagios_comment));
    return comment;
}

int my_system_r(nagios_macros * mac, char * cmd, int timeout, int * early_timeout, double * exectime, char ** output, int max_output_length)
{
    return my_system_r_output;
}
