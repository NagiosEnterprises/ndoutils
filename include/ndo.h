#ifndef NDO_H
#define NDO_H

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define NDO_OK 0
#define NDO_ERROR -1

#ifndef TRUE
#   define TRUE 1
#elif TRUE != 1
#   undef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#elif FALSE != 0
#   undef FALSE
#   define FALSE 0
#endif

#include "../include/nagios/objects.h"

void ndo_log(char * buffer);
int nebmodule_init(int flags, char * args, void * handle);
int nebmodule_deinit(int flags, int reason);
int ndo_process_arguments(char * args);
int ndo_initialize_database();
int ndo_initialize_prepared_statements();
void ndo_disconnect_database();
int ndo_register_callbacks();
int ndo_deregister_callbacks();


int ndo_handle_process(int type, void * d);
int ndo_handle_timed_event(int type, void * d);
int ndo_handle_log(int type, void * d);
int ndo_handle_system_command(int type, void * d);
int ndo_handle_event_handler(int type, void * d);
int ndo_handle_notification(int type, void * d);
int ndo_handle_service_check(int type, void * d);
int ndo_handle_host_check(int type, void * d);
int ndo_handle_comment(int type, void * d);
int ndo_handle_downtime(int type, void * d);
int ndo_handle_flapping(int type, void * d);
int ndo_handle_program_status(int type, void * d);
int ndo_handle_host_status(int type, void * d);
int ndo_handle_service_status(int type, void * d);
int ndo_handle_external_command(int type, void * d);
int ndo_handle_acknowledgement(int type, void * d);
int ndo_handle_state_change(int type, void * d);
int ndo_handle_contact_status(int type, void * d);
int ndo_handle_contact_notification(int type, void * d);
int ndo_handle_contact_notification_method(int type, void * d);

int ndo_handle_retention(int type, void * d);

int ndo_table_genocide();
int ndo_set_all_objects_inactive();
int ndo_write_active_objects();
int ndo_write_config_files();
int ndo_write_config(int type);
int ndo_write_runtime_variables();

int ndo_save_customvariables(int object_id, int config_type, customvariablesmember * vars);

void ndo_process_config_line(char * line);
int ndo_process_config_file();
int ndo_config_sanity_check();
char * ndo_strip(char * s);

int write_to_log(char * buffer, unsigned long l, time_t * t);

#define GET_NAME1    0
#define GET_NAME2    1
#define INSERT_NAME1 2
#define INSERT_NAME2 3

int ndo_get_object_id_name1(int insert, int object_type, char * name1);
int ndo_get_object_id_name2(int insert, int object_type, char * name1, char * name2);
int ndo_insert_object_id_name1(int object_type, char * name1);
int ndo_insert_object_id_name2(int object_type, char * name1, char * name2);



int ndo_write_commands(int config_type);

int ndo_write_timeperiods(int config_type);
int ndo_write_timeperiods(int config_type);
int ndo_write_timeperiod_timeranges(int * timeperiod_ids);

int ndo_write_contacts(int config_type);
int ndo_write_contact_addresses(int * contact_ids);
int ndo_write_contact_notificationcommands(int * contact_ids);
int ndo_write_contact_notificationcommands_cmd(commandsmember * cmd, int notification_type, int * contact_ids, int i);

int ndo_write_contactgroups(int config_type);
int ndo_write_contactgroup_members(int * contactgroup_ids);

int ndo_write_hosts(int config_type);
int ndo_write_host_parenthosts(int * host_ids);
int ndo_write_host_contactgroups(int * host_ids);
int ndo_write_host_contacts(int * host_ids);

int ndo_write_hostgroups(int config_type);
int ndo_write_hostgroup_members(int * hostgroup_ids);

int ndo_write_services(int config_type);
int ndo_write_service_parentservices(int * service_ids);
int ndo_write_service_contactgroups(int * service_ids);
int ndo_write_service_contacts(int * service_ids);

int ndo_write_servicegroups(int config_type);
int ndo_write_servicegroup_members(int * servicegroup_ids);


#define COUNT_OBJECTS(objvar, listvar, countvar) \
do { \
    while (objvar != NULL) { \
        countvar++; \
        objvar = objvar->next; \
    } \
    objvar = listvar; \
} while (0)




#define SAVE_CUSTOMVARIABLES(objvar, objidvar, objtype, customvars, i) \
do { \
    while (objvar != NULL) { \
        ndo_save_customvariables(objidvar[i], objtype, customvars); \
        i++; \
        objvar = objvar->next; \
    } \
} while(0)



#define NDO_PROCESS_PROCESS                         (1 << 0)
#define NDO_PROCESS_TIMED_EVENT                     (1 << 1)
#define NDO_PROCESS_LOG                             (1 << 2)
#define NDO_PROCESS_SYSTEM_COMMAND                  (1 << 3)
#define NDO_PROCESS_EVENT_HANDLER                   (1 << 4)
#define NDO_PROCESS_NOTIFICATION                    (1 << 5)
#define NDO_PROCESS_SERVICE_CHECK                   (1 << 6)
#define NDO_PROCESS_HOST_CHECK                      (1 << 7)
#define NDO_PROCESS_COMMENT                         (1 << 8)
#define NDO_PROCESS_DOWNTIME                        (1 << 9)
#define NDO_PROCESS_FLAPPING                        (1 << 10)
#define NDO_PROCESS_PROGRAM_STATUS                  (1 << 11)
#define NDO_PROCESS_HOST_STATUS                     (1 << 12)
#define NDO_PROCESS_SERVICE_STATUS                  (1 << 13)

#define NDO_PROCESS_EXTERNAL_COMMAND                (1 << 17)
#define NDO_PROCESS_OBJECT_CONFIG                   (1 << 18)
#define NDO_PROCESS_MAIN_CONFIG                     (1 << 19)
#define NDO_PROCESS_RETENTION                       (1 << 21)
#define NDO_PROCESS_ACKNOWLEDGEMENT                 (1 << 22)
#define NDO_PROCESS_STATE_CHANGE                    (1 << 23)
#define NDO_PROCESS_CONTACT_STATUS                  (1 << 24)

#define NDO_PROCESS_CONTACT_NOTIFICATION            (1 << 26)
#define NDO_PROCESS_CONTACT_NOTIFICATION_METHOD     (1 << 27)



#define NDO_OBJECTTYPE_HOST                1
#define NDO_OBJECTTYPE_SERVICE             2 
#define NDO_OBJECTTYPE_HOSTGROUP           3
#define NDO_OBJECTTYPE_SERVICEGROUP        4
#define NDO_OBJECTTYPE_HOSTESCALATION      5
#define NDO_OBJECTTYPE_SERVICEESCALATION   6
#define NDO_OBJECTTYPE_HOSTDEPENDENCY      7
#define NDO_OBJECTTYPE_SERVICEDEPENDENCY   8
#define NDO_OBJECTTYPE_TIMEPERIOD          9
#define NDO_OBJECTTYPE_CONTACT             10
#define NDO_OBJECTTYPE_CONTACTGROUP        11
#define NDO_OBJECTTYPE_COMMAND             12


#define NDO_CONFIG_DUMP_NONE     0
#define NDO_CONFIG_DUMP_ORIGINAL 1
#define NDO_CONFIG_DUMP_RETAINED 2
#define NDO_CONFIG_DUMP_ALL      3



/*************** LIVE DATA ATTRIBUTES ***************/

#define NDO_DATA_ACKAUTHOR                           5
#define NDO_DATA_ACKDATA                             6
#define NDO_DATA_ACKNOWLEDGEMENTTYPE                 7
#define NDO_DATA_ACTIVEHOSTCHECKSENABLED             8
#define NDO_DATA_ACTIVESERVICECHECKSENABLED          9
#define NDO_DATA_AUTHORNAME                          10
#define NDO_DATA_CHECKCOMMAND                        11
#define NDO_DATA_CHECKTYPE                           12
#define NDO_DATA_COMMANDARGS                         13
#define NDO_DATA_COMMANDLINE                         14
#define NDO_DATA_COMMANDSTRING                       15
#define NDO_DATA_COMMANDTYPE                         16
#define NDO_DATA_COMMENT                             17
#define NDO_DATA_COMMENTID                           18
#define NDO_DATA_COMMENTTIME                         19
#define NDO_DATA_COMMENTTYPE                         20
#define NDO_DATA_CONFIGFILENAME                      21
#define NDO_DATA_CONFIGFILEVARIABLE                  22
#define NDO_DATA_CONFIGVARIABLE                      23
#define NDO_DATA_CONTACTSNOTIFIED                    24
#define NDO_DATA_CURRENTCHECKATTEMPT                 25
#define NDO_DATA_CURRENTNOTIFICATIONNUMBER           26
#define NDO_DATA_CURRENTSTATE                        27
#define NDO_DATA_DAEMONMODE                          28
#define NDO_DATA_DOWNTIMEID                          29
#define NDO_DATA_DOWNTIMETYPE                        30
#define NDO_DATA_DURATION                            31
#define NDO_DATA_EARLYTIMEOUT                        32
#define NDO_DATA_ENDTIME                             33
#define NDO_DATA_ENTRYTIME                           34
#define NDO_DATA_ENTRYTYPE                           35
#define NDO_DATA_ESCALATED                           36
#define NDO_DATA_EVENTHANDLER                        37
#define NDO_DATA_EVENTHANDLERENABLED                 38
#define NDO_DATA_EVENTHANDLERSENABLED                39
#define NDO_DATA_EVENTHANDLERTYPE                    40
#define NDO_DATA_EVENTTYPE                           41
#define NDO_DATA_EXECUTIONTIME                       42
#define NDO_DATA_EXPIRATIONTIME                      43
#define NDO_DATA_EXPIRES                             44
#define NDO_DATA_FAILUREPREDICTIONENABLED            45
#define NDO_DATA_FIXED                               46
#define NDO_DATA_FLAPDETECTIONENABLED                47
#define NDO_DATA_FLAPPINGTYPE                        48
#define NDO_DATA_GLOBALHOSTEVENTHANDLER              49
#define NDO_DATA_GLOBALSERVICEEVENTHANDLER           50
#define NDO_DATA_HASBEENCHECKED                      51
#define NDO_DATA_HIGHTHRESHOLD                       52
#define NDO_DATA_HOST                                53
#define NDO_DATA_ISFLAPPING                          54
#define NDO_DATA_LASTCOMMANDCHECK                    55
#define NDO_DATA_LASTHARDSTATE                       56
#define NDO_DATA_LASTHARDSTATECHANGE                 57
#define NDO_DATA_LASTHOSTCHECK                       58
#define NDO_DATA_LASTHOSTNOTIFICATION                59
#define NDO_DATA_LASTLOGROTATION                     60
#define NDO_DATA_LASTSERVICECHECK                    61
#define NDO_DATA_LASTSERVICENOTIFICATION             62
#define NDO_DATA_LASTSTATECHANGE                     63
#define NDO_DATA_LASTTIMECRITICAL                    64
#define NDO_DATA_LASTTIMEDOWN                        65
#define NDO_DATA_LASTTIMEOK                          66
#define NDO_DATA_LASTTIMEUNKNOWN                     67
#define NDO_DATA_LASTTIMEUNREACHABLE                 68
#define NDO_DATA_LASTTIMEUP                          69
#define NDO_DATA_LASTTIMEWARNING                     70
#define NDO_DATA_LATENCY                             71
#define NDO_DATA_LOGENTRY                            72
#define NDO_DATA_LOGENTRYTIME                        73
#define NDO_DATA_LOGENTRYTYPE                        74
#define NDO_DATA_LOWTHRESHOLD                        75
#define NDO_DATA_MAXCHECKATTEMPTS                    76
#define NDO_DATA_MODIFIEDHOSTATTRIBUTE               77
#define NDO_DATA_MODIFIEDHOSTATTRIBUTES              78
#define NDO_DATA_MODIFIEDSERVICEATTRIBUTE            79
#define NDO_DATA_MODIFIEDSERVICEATTRIBUTES           80
#define NDO_DATA_NEXTHOSTCHECK                       81
#define NDO_DATA_NEXTHOSTNOTIFICATION                82
#define NDO_DATA_NEXTSERVICECHECK                    83
#define NDO_DATA_NEXTSERVICENOTIFICATION             84
#define NDO_DATA_NOMORENOTIFICATIONS                 85
#define NDO_DATA_NORMALCHECKINTERVAL                 86
#define NDO_DATA_NOTIFICATIONREASON                  87
#define NDO_DATA_NOTIFICATIONSENABLED                88
#define NDO_DATA_NOTIFICATIONTYPE                    89
#define NDO_DATA_NOTIFYCONTACTS                      90
#define NDO_DATA_OBSESSOVERHOST                      91
#define NDO_DATA_OBSESSOVERHOSTS                     92
#define NDO_DATA_OBSESSOVERSERVICE                   93
#define NDO_DATA_OBSESSOVERSERVICES                  94
#define NDO_DATA_OUTPUT                              95
#define NDO_DATA_PASSIVEHOSTCHECKSENABLED            96
#define NDO_DATA_PASSIVESERVICECHECKSENABLED         97
#define NDO_DATA_PERCENTSTATECHANGE                  98
#define NDO_DATA_PERFDATA                            99
#define NDO_DATA_PERSISTENT                          100
#define NDO_DATA_PROBLEMHASBEENACKNOWLEDGED          101
#define NDO_DATA_PROCESSID                           102
#define NDO_DATA_PROCESSPERFORMANCEDATA              103
#define NDO_DATA_PROGRAMDATE                         104
#define NDO_DATA_PROGRAMNAME                         105
#define NDO_DATA_PROGRAMSTARTTIME                    106
#define NDO_DATA_PROGRAMVERSION                      107
#define NDO_DATA_RECURRING                           108
#define NDO_DATA_RETRYCHECKINTERVAL                  109
#define NDO_DATA_RETURNCODE                          110
#define NDO_DATA_RUNTIME                             111
#define NDO_DATA_RUNTIMEVARIABLE                     112
#define NDO_DATA_SCHEDULEDDOWNTIMEDEPTH              113
#define NDO_DATA_SERVICE                             114
#define NDO_DATA_SHOULDBESCHEDULED                   115
#define NDO_DATA_SOURCE                              116
#define NDO_DATA_STARTTIME                           117
#define NDO_DATA_STATE                               118
#define NDO_DATA_STATECHANGE                         119
#define NDO_DATA_STATECHANGETYPE                     120
#define NDO_DATA_STATETYPE                           121
#define NDO_DATA_STICKY                              122
#define NDO_DATA_TIMEOUT                             123
#define NDO_DATA_TRIGGEREDBY                         124
#define NDO_DATA_LONGOUTPUT                          125

/*********** OBJECT CONFIG DATA ATTRIBUTES **********/

#define NDO_DATA_ACTIONURL                           126
#define NDO_DATA_COMMANDNAME                         127
#define NDO_DATA_CONTACTADDRESS                      128
#define NDO_DATA_CONTACTALIAS                        129
#define NDO_DATA_CONTACTGROUP                        130
#define NDO_DATA_CONTACTGROUPALIAS                   131
#define NDO_DATA_CONTACTGROUPMEMBER                  132
#define NDO_DATA_CONTACTGROUPNAME                    133
#define NDO_DATA_CONTACTNAME                         134
#define NDO_DATA_DEPENDENCYTYPE                      135
#define NDO_DATA_DEPENDENTHOSTNAME                   136
#define NDO_DATA_DEPENDENTSERVICEDESCRIPTION         137
#define NDO_DATA_EMAILADDRESS                        138
#define NDO_DATA_ESCALATEONCRITICAL                  139
#define NDO_DATA_ESCALATEONDOWN                      140
#define NDO_DATA_ESCALATEONRECOVERY                  141
#define NDO_DATA_ESCALATEONUNKNOWN                   142
#define NDO_DATA_ESCALATEONUNREACHABLE               143
#define NDO_DATA_ESCALATEONWARNING                   144
#define NDO_DATA_ESCALATIONPERIOD                    145
#define NDO_DATA_FAILONCRITICAL                      146
#define NDO_DATA_FAILONDOWN                          147
#define NDO_DATA_FAILONOK                            148
#define NDO_DATA_FAILONUNKNOWN                       149
#define NDO_DATA_FAILONUNREACHABLE                   150
#define NDO_DATA_FAILONUP                            151
#define NDO_DATA_FAILONWARNING                       152
#define NDO_DATA_FIRSTNOTIFICATION                   153
#define NDO_DATA_HAVE2DCOORDS                        154
#define NDO_DATA_HAVE3DCOORDS                        155
#define NDO_DATA_HIGHHOSTFLAPTHRESHOLD               156
#define NDO_DATA_HIGHSERVICEFLAPTHRESHOLD            157
#define NDO_DATA_HOSTADDRESS                         158
#define NDO_DATA_HOSTALIAS                           159
#define NDO_DATA_HOSTCHECKCOMMAND                    160
#define NDO_DATA_HOSTCHECKINTERVAL                   161
#define NDO_DATA_HOSTCHECKPERIOD                     162
#define NDO_DATA_HOSTEVENTHANDLER                    163
#define NDO_DATA_HOSTEVENTHANDLERENABLED             164
#define NDO_DATA_HOSTFAILUREPREDICTIONENABLED        165
#define NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS        166
#define NDO_DATA_HOSTFLAPDETECTIONENABLED            167
#define NDO_DATA_HOSTFRESHNESSCHECKSENABLED          168
#define NDO_DATA_HOSTFRESHNESSTHRESHOLD              169
#define NDO_DATA_HOSTGROUPALIAS                      170
#define NDO_DATA_HOSTGROUPMEMBER                     171
#define NDO_DATA_HOSTGROUPNAME                       172
#define NDO_DATA_HOSTMAXCHECKATTEMPTS                173
#define NDO_DATA_HOSTNAME                            174
#define NDO_DATA_HOSTNOTIFICATIONCOMMAND             175
#define NDO_DATA_HOSTNOTIFICATIONINTERVAL            176
#define NDO_DATA_HOSTNOTIFICATIONPERIOD              177
#define NDO_DATA_HOSTNOTIFICATIONSENABLED            178
#define NDO_DATA_ICONIMAGE                           179
#define NDO_DATA_ICONIMAGEALT                        180
#define NDO_DATA_INHERITSPARENT                      181
#define NDO_DATA_LASTNOTIFICATION                    182
#define NDO_DATA_LOWHOSTFLAPTHRESHOLD                183
#define NDO_DATA_LOWSERVICEFLAPTHRESHOLD             184
#define NDO_DATA_MAXSERVICECHECKATTEMPTS             185
#define NDO_DATA_NOTES                               186
#define NDO_DATA_NOTESURL                            187
#define NDO_DATA_NOTIFICATIONINTERVAL                188
#define NDO_DATA_NOTIFYHOSTDOWN                      189
#define NDO_DATA_NOTIFYHOSTFLAPPING                  190
#define NDO_DATA_NOTIFYHOSTRECOVERY                  191
#define NDO_DATA_NOTIFYHOSTUNREACHABLE               192
#define NDO_DATA_NOTIFYSERVICECRITICAL               193
#define NDO_DATA_NOTIFYSERVICEFLAPPING               194
#define NDO_DATA_NOTIFYSERVICERECOVERY               195
#define NDO_DATA_NOTIFYSERVICEUNKNOWN                196
#define NDO_DATA_NOTIFYSERVICEWARNING                197
#define NDO_DATA_PAGERADDRESS                        198
#define NDO_DATA_PARALLELIZESERVICECHECK             199   /* no longer used */
#define NDO_DATA_PARENTHOST                          200
#define NDO_DATA_PROCESSHOSTPERFORMANCEDATA          201
#define NDO_DATA_PROCESSSERVICEPERFORMANCEDATA       202
#define NDO_DATA_RETAINHOSTNONSTATUSINFORMATION      203
#define NDO_DATA_RETAINHOSTSTATUSINFORMATION         204
#define NDO_DATA_RETAINSERVICENONSTATUSINFORMATION   205
#define NDO_DATA_RETAINSERVICESTATUSINFORMATION      206
#define NDO_DATA_SERVICECHECKCOMMAND                 207
#define NDO_DATA_SERVICECHECKINTERVAL                208
#define NDO_DATA_SERVICECHECKPERIOD                  209
#define NDO_DATA_SERVICEDESCRIPTION                  210
#define NDO_DATA_SERVICEEVENTHANDLER                 211
#define NDO_DATA_SERVICEEVENTHANDLERENABLED          212
#define NDO_DATA_SERVICEFAILUREPREDICTIONENABLED     213
#define NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS     214
#define NDO_DATA_SERVICEFLAPDETECTIONENABLED         215
#define NDO_DATA_SERVICEFRESHNESSCHECKSENABLED       216
#define NDO_DATA_SERVICEFRESHNESSTHRESHOLD           217
#define NDO_DATA_SERVICEGROUPALIAS                   218
#define NDO_DATA_SERVICEGROUPMEMBER                  219
#define NDO_DATA_SERVICEGROUPNAME                    220
#define NDO_DATA_SERVICEISVOLATILE                   221
#define NDO_DATA_SERVICENOTIFICATIONCOMMAND          222
#define NDO_DATA_SERVICENOTIFICATIONINTERVAL         223
#define NDO_DATA_SERVICENOTIFICATIONPERIOD           224
#define NDO_DATA_SERVICENOTIFICATIONSENABLED         225
#define NDO_DATA_SERVICERETRYINTERVAL                226
#define NDO_DATA_SHOULDBEDRAWN                       227    /* no longer used */
#define NDO_DATA_STALKHOSTONDOWN                     228
#define NDO_DATA_STALKHOSTONUNREACHABLE              229
#define NDO_DATA_STALKHOSTONUP                       230
#define NDO_DATA_STALKSERVICEONCRITICAL              231
#define NDO_DATA_STALKSERVICEONOK                    232
#define NDO_DATA_STALKSERVICEONUNKNOWN               233
#define NDO_DATA_STALKSERVICEONWARNING               234
#define NDO_DATA_STATUSMAPIMAGE                      235
#define NDO_DATA_TIMEPERIODALIAS                     236
#define NDO_DATA_TIMEPERIODNAME                      237
#define NDO_DATA_TIMERANGE                           238
#define NDO_DATA_VRMLIMAGE                           239
#define NDO_DATA_X2D                                 240
#define NDO_DATA_X3D                                 241
#define NDO_DATA_Y2D                                 242
#define NDO_DATA_Y3D                                 243
#define NDO_DATA_Z3D                                 244

#define NDO_DATA_CONFIGDUMPTYPE                      245

#define NDO_DATA_FIRSTNOTIFICATIONDELAY              246
#define NDO_DATA_HOSTRETRYINTERVAL                   247
#define NDO_DATA_NOTIFYHOSTDOWNTIME                  248
#define NDO_DATA_NOTIFYSERVICEDOWNTIME               249
#define NDO_DATA_CANSUBMITCOMMANDS                   250
#define NDO_DATA_FLAPDETECTIONONUP                   251
#define NDO_DATA_FLAPDETECTIONONDOWN                 252
#define NDO_DATA_FLAPDETECTIONONUNREACHABLE          253
#define NDO_DATA_FLAPDETECTIONONOK                   254
#define NDO_DATA_FLAPDETECTIONONWARNING              255
#define NDO_DATA_FLAPDETECTIONONUNKNOWN              256
#define NDO_DATA_FLAPDETECTIONONCRITICAL             257
#define NDO_DATA_DISPLAYNAME                         258
#define NDO_DATA_DEPENDENCYPERIOD                    259
#define NDO_DATA_MODIFIEDCONTACTATTRIBUTE            260    /* LIVE DATA */
#define NDO_DATA_MODIFIEDCONTACTATTRIBUTES           261    /* LIVE DATA */
#define NDO_DATA_CUSTOMVARIABLE                      262
#define NDO_DATA_HASBEENMODIFIED                     263
#define NDO_DATA_CONTACT                             264
#define NDO_DATA_LASTSTATE                           265

/* Nagios Core 4 additions */
#define NDO_DATA_IMPORTANCE                          266
#define NDO_DATA_MINIMUMIMPORTANCE                   267
#define NDO_DATA_PARENTSERVICE                       268
#define NDO_DATA_ACTIVEOBJECTSTYPE                   269

#endif /* NDO_H */