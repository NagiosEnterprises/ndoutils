/************************************************************************
 *
 * PROTOAPI.H - NDO Protocol Definition
 * Copyright (c) 2005-2006 Ethan Galstad
 * Last Modified: 09-27-2006
 *
 ************************************************************************/

#ifndef _NDO_PROTOAPI_H
#define _NDO_PROTOAPI_H


/****************** PROTOCOL VERSION ***************/

#define NDO_API_PROTOVERSION                         2


/****************** CONTROL STRINGS ****************/

#define NDO_API_NONE                                 ""

#define NDO_API_HELLO                                "HELLO"
#define NDO_API_GOODBYE                              "GOODBYE"

#define NDO_API_PROTOCOL                             "PROTOCOL"
#define NDO_API_AGENT                                "AGENT"
#define NDO_API_AGENTVERSION                         "AGENTVERSION"
#define NDO_API_DISPOSITION                          "DISPOSITION"  /* archived or realtime */
#define NDO_API_CONNECTION                           "CONNECTION"   /* immediate or deferred */
#define NDO_API_CONNECTTYPE                          "CONNECTTYPE"  /* initial or reconnection */

#define NDO_API_DISPOSITION_ARCHIVED                 "ARCHIVED"
#define NDO_API_DISPOSITION_REALTIME                 "REALTIME"
#define NDO_API_CONNECTION_FILE                      "FILE"
#define NDO_API_CONNECTION_UNIXSOCKET                "UNIXSOCKET"
#define NDO_API_CONNECTION_TCPSOCKET                 "TCPSOCKET"
#define NDO_API_CONNECTTYPE_INITIAL                  "INITIAL"
#define NDO_API_CONNECTTYPE_RECONNECT                "RECONNECT"

#define NDO_API_STARTDATADUMP                        "STARTDATADUMP"
#define NDO_API_STARTTIME                            "STARTTIME"
#define NDO_API_ENDTIME                              "ENDTIME"

#define NDO_API_CONFIGDUMP_ORIGINAL                  "ORIGINAL"
#define NDO_API_CONFIGDUMP_RETAINED                  "RETAINED"

#define NDO_API_INSTANCENAME                         "INSTANCENAME"

#define NDO_API_STARTCONFIGDUMP                      900
#define NDO_API_ENDCONFIGDUMP                        901
#define NDO_API_ENDDATA                              999
#define NDO_API_ENDDATADUMP                          1000



/******************** DATA TYPES *******************/

#define NDO_API_LOGENTRY                             100

#define NDO_API_PROCESSDATA                          200
#define NDO_API_TIMEDEVENTDATA                       201
#define NDO_API_LOGDATA                              202
#define NDO_API_SYSTEMCOMMANDDATA                    203
#define NDO_API_EVENTHANDLERDATA                     204
#define NDO_API_NOTIFICATIONDATA                     205
#define NDO_API_SERVICECHECKDATA                     206
#define NDO_API_HOSTCHECKDATA                        207
#define NDO_API_COMMENTDATA                          208
#define NDO_API_DOWNTIMEDATA                         209
#define NDO_API_FLAPPINGDATA                         210
#define NDO_API_PROGRAMSTATUSDATA                    211
#define NDO_API_HOSTSTATUSDATA                       212
#define NDO_API_SERVICESTATUSDATA                    213
#define NDO_API_ADAPTIVEPROGRAMDATA                  214
#define NDO_API_ADAPTIVEHOSTDATA                     215
#define NDO_API_ADAPTIVESERVICEDATA                  216
#define NDO_API_EXTERNALCOMMANDDATA                  217
#define NDO_API_AGGREGATEDSTATUSDATA                 218
#define NDO_API_RETENTIONDATA                        219
#define NDO_API_CONTACTNOTIFICATIONDATA              220
#define NDO_API_CONTACTNOTIFICATIONMETHODDATA        221
#define NDO_API_ACKNOWLEDGEMENTDATA                  222
#define NDO_API_STATECHANGEDATA                      223
#define NDO_API_CONTACTSTATUSDATA                    224
#define NDO_API_ADAPTIVECONTACTDATA                  225

#define NDO_API_MAINCONFIGFILEVARIABLES              300
#define NDO_API_RESOURCECONFIGFILEVARIABLES          301
#define NDO_API_CONFIGVARIABLES                      302
#define NDO_API_RUNTIMEVARIABLES                     303

#define NDO_API_HOSTDEFINITION                       400
#define NDO_API_HOSTGROUPDEFINITION                  401
#define NDO_API_SERVICEDEFINITION                    402
#define NDO_API_SERVICEGROUPDEFINITION               403
#define NDO_API_HOSTDEPENDENCYDEFINITION             404
#define NDO_API_SERVICEDEPENDENCYDEFINITION          405
#define NDO_API_HOSTESCALATIONDEFINITION             406
#define NDO_API_SERVICEESCALATIONDEFINITION          407
#define NDO_API_COMMANDDEFINITION                    408
#define NDO_API_TIMEPERIODDEFINITION                 409
#define NDO_API_CONTACTDEFINITION                    410
#define NDO_API_CONTACTGROUPDEFINITION               411
#define NDO_API_HOSTEXTINFODEFINITION                412    /* no longer used */
#define NDO_API_SERVICEEXTINFODEFINITION             413    /* no longer used */


/************** COMMON DATA ATTRIBUTES **************/

#define NDO_MAX_DATA_TYPES                           266

#define NDO_DATA_NONE                                0

#define NDO_DATA_TYPE                                1
#define NDO_DATA_FLAGS                               2
#define NDO_DATA_ATTRIBUTES                          3
#define NDO_DATA_TIMESTAMP                           4


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

#endif
