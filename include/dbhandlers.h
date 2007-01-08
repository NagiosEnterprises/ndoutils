/************************************************************************
 *
 * DBHANDLERS.H - NDO2DB DB Handler Include File
 * Copyright (c) 2005-2006 Ethan Galstad
 * Last Modified: 05-25-2006
 *
 ************************************************************************/

#ifndef _NDO2DB_DBHANDLERS_H
#define _NDO2DB_DBHANDLERS_H

#include "ndo2db.h"

int ndo2db_get_object_id(ndo2db_idi *,int,char *,char *,unsigned long *);
int ndo2db_get_object_id_with_insert(ndo2db_idi *,int,char *,char *,unsigned long *);

int ndo2db_get_cached_object_ids(ndo2db_idi *);
int ndo2db_get_cached_object_id(ndo2db_idi *,int,char *,char *,unsigned long *);
int ndo2db_add_cached_object_id(ndo2db_idi *,int,char *,char *,unsigned long);
int ndo2db_free_cached_object_ids(ndo2db_idi *);

int ndo2db_object_hashfunc(const char *,const char *,int);
int ndo2db_compare_object_hashdata(const char *,const char *,const char *,const char *);

int ndo2db_set_all_objects_as_inactive(ndo2db_idi *);
int ndo2db_set_object_as_active(ndo2db_idi *,int,unsigned long);

int ndo2db_handle_logentry(ndo2db_idi *);
int ndo2db_handle_processdata(ndo2db_idi *);
int ndo2db_handle_timedeventdata(ndo2db_idi *);
int ndo2db_handle_logdata(ndo2db_idi *);
int ndo2db_handle_systemcommanddata(ndo2db_idi *);
int ndo2db_handle_eventhandlerdata(ndo2db_idi *);
int ndo2db_handle_notificationdata(ndo2db_idi *);
int ndo2db_handle_contactnotificationdata(ndo2db_idi *);
int ndo2db_handle_contactnotificationmethoddata(ndo2db_idi *);
int ndo2db_handle_servicecheckdata(ndo2db_idi *);
int ndo2db_handle_hostcheckdata(ndo2db_idi *);
int ndo2db_handle_commentdata(ndo2db_idi *);
int ndo2db_handle_downtimedata(ndo2db_idi *);
int ndo2db_handle_flappingdata(ndo2db_idi *);
int ndo2db_handle_programstatusdata(ndo2db_idi *);
int ndo2db_handle_hoststatusdata(ndo2db_idi *);
int ndo2db_handle_servicestatusdata(ndo2db_idi *);
int ndo2db_handle_contactstatusdata(ndo2db_idi *);
int ndo2db_handle_adaptiveprogramdata(ndo2db_idi *);
int ndo2db_handle_adaptivehostdata(ndo2db_idi *);
int ndo2db_handle_adaptiveservicedata(ndo2db_idi *);
int ndo2db_handle_adaptivecontactdata(ndo2db_idi *);
int ndo2db_handle_externalcommanddata(ndo2db_idi *);
int ndo2db_handle_aggregatedstatusdata(ndo2db_idi *);
int ndo2db_handle_retentiondata(ndo2db_idi *);
int ndo2db_handle_acknowledgementdata(ndo2db_idi *);
int ndo2db_handle_statechangedata(ndo2db_idi *);
int ndo2db_handle_configfilevariables(ndo2db_idi *,int);
int ndo2db_handle_configvariables(ndo2db_idi *);
int ndo2db_handle_runtimevariables(ndo2db_idi *);
int ndo2db_handle_configdumpstart(ndo2db_idi *);
int ndo2db_handle_configdumpend(ndo2db_idi *);
int ndo2db_handle_hostdefinition(ndo2db_idi *);
int ndo2db_handle_hostgroupdefinition(ndo2db_idi *);
int ndo2db_handle_servicedefinition(ndo2db_idi *);
int ndo2db_handle_servicegroupdefinition(ndo2db_idi *);
int ndo2db_handle_hostdependencydefinition(ndo2db_idi *);
int ndo2db_handle_servicedependencydefinition(ndo2db_idi *);
int ndo2db_handle_hostescalationdefinition(ndo2db_idi *);
int ndo2db_handle_serviceescalationdefinition(ndo2db_idi *);
int ndo2db_handle_commanddefinition(ndo2db_idi *);
int ndo2db_handle_timeperiodefinition(ndo2db_idi *);
int ndo2db_handle_contactdefinition(ndo2db_idi *);
int ndo2db_handle_contactgroupdefinition(ndo2db_idi *);

#endif
