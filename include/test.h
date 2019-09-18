
#ifndef NAGIOS_TEST_H_INCLUDED
#define NAGIOS_TEST_H_INCLUDED

#include "nagios/objects.h"

struct timeperiod test_tp;
struct host test_host;
struct service test_service;
struct contact test_contact;

void populate_all_objects();
void free_all_objects();

#endif