
int neb_register_callback(int callback_type, void *mod_handle, int priority, int (*callback_func)(int, void *))
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
    return "2019-08-20";
}

struct scheduled_downtime *find_downtime(int i, unsigned long l)
{
    struct scheduled_downtime * dwn = malloc(sizeof(struct scheduled_downtime));
    return dwn;
}

int write_to_log(char *buffer, unsigned long data_type, time_t *timestamp)
{
    printf("%s\n", buffer);
}