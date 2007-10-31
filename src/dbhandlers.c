/***************************************************************
 * DBHANDLERS.C - Data handler routines for NDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad 
 *
 * Last Modified: 10-31-2007
 *
 **************************************************************/

/* include our project's header files */
#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndo2db.h"
#include "../include/db.h"
#include "../include/dbhandlers.h"

/* Nagios header files */

#ifdef BUILD_NAGIOS_2X
#include "../include/nagios-2x/nagios.h"
#include "../include/nagios-2x/broker.h"
#include "../include/nagios-2x/comments.h"
#endif
#ifdef BUILD_NAGIOS_3X
#include "../include/nagios-3x/nagios.h"
#include "../include/nagios-3x/broker.h"
#include "../include/nagios-3x/comments.h"
#endif


extern int errno;

extern char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];



/****************************************************************************/
/* OBJECT ROUTINES                                                          */
/****************************************************************************/

int ndo2db_get_object_id(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id){
	int result=NDO_OK;
	unsigned long cached_object_id=0L;
	int found_object=NDO_FALSE;
	char *name1=NULL;
	char *name2=NULL;
	char *buf=NULL;
	char *buf1=NULL;
	char *buf2=NULL;
	char *es[2];

	/* make sure empty strings are set to null */
	name1=n1;
	name2=n2;
	if(name1 && !strcmp(name1,""))
		name1=NULL;
	if(name2 && !strcmp(name2,""))
		name2=NULL;

	/* null names mean no object id */
	if(name1==NULL && name2==NULL){
		*object_id=0L;
		return NDO_OK;
	        }

	/* see if the object already exists in cached lookup table */
	if(ndo2db_get_cached_object_id(idi,object_type,name1,name2,&cached_object_id)==NDO_OK){
		*object_id=cached_object_id;
		return NDO_OK;
	        }

	if(name1==NULL){
		es[0]=NULL;
		if(asprintf(&buf1,"name1 IS NULL")==-1)
			buf1=NULL;
	        }
	else{
		es[0]=ndo2db_db_escape_string(idi,name1);
		if(asprintf(&buf1,"name1='%s'",es[0])==-1)
			buf1=NULL;
	        }

	if(name2==NULL){
		es[1]=NULL;
		if(asprintf(&buf2,"name2 IS NULL")==-1)
			buf2=NULL;
	        }
	else{
		es[1]=ndo2db_db_escape_string(idi,name2);
		if(asprintf(&buf2,"name2='%s'",es[1])==-1)
			buf2=NULL;
	        }
	
	if(asprintf(&buf,"SELECT * FROM %s WHERE instance_id='%lu' AND objecttype_id='%d' AND %s AND %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]
		    ,idi->dbinfo.instance_id
		    ,object_type
		    ,buf1
		    ,buf2
		   )==-1)
		buf=NULL;
	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.mysql_result=mysql_store_result(&idi->dbinfo.mysql_conn);
			if((idi->dbinfo.mysql_row=mysql_fetch_row(idi->dbinfo.mysql_result))!=NULL){
				ndo2db_convert_string_to_unsignedlong(idi->dbinfo.mysql_row[0],object_id);
				found_object=NDO_TRUE;
			        }
			mysql_free_result(idi->dbinfo.mysql_result);
			idi->dbinfo.mysql_result=NULL;
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);

	/* free memory */
	free(buf1);
	free(buf2);
	free(es[0]);
	free(es[1]);

	if(found_object==NDO_FALSE)
		result=NDO_ERROR;

	return result;
        }


int ndo2db_get_object_id_with_insert(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long *object_id){
	int result=NDO_OK;
	char *buf=NULL;
	char *buf1=NULL;
	char *buf2=NULL;
	char *name1=NULL;
	char *name2=NULL;
	char *es[2];

	/* make sure empty strings are set to null */
	name1=n1;
	name2=n2;
	if(name1 && !strcmp(name1,""))
		name1=NULL;
	if(name2 && !strcmp(name2,""))
		name2=NULL;

	/* null names mean no object id */
	if(name1==NULL && name2==NULL){
		*object_id=0L;
		return NDO_OK;
	        }

	/* object already exists */
	if((result=ndo2db_get_object_id(idi,object_type,name1,name2,object_id))==NDO_OK)
		return NDO_OK;

	if(name1!=NULL){
		es[0]=ndo2db_db_escape_string(idi,name1);
		if(asprintf(&buf1,", name1='%s'",es[0])==-1)
			buf1=NULL;
	        }
	else
		es[0]=NULL;
	if(name2!=NULL){
		es[1]=ndo2db_db_escape_string(idi,name2);
		if(asprintf(&buf2,", name2='%s'",es[1])==-1)
			buf2=NULL;
	        }
	else
		es[1]=NULL;
	
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', objecttype_id='%d' %s %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]
		    ,idi->dbinfo.instance_id
		    ,object_type
		    ,(buf1==NULL)?"":buf1
		    ,(buf2==NULL)?"":buf2
		   )==-1)
		buf=NULL;
	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			*object_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);

	/* cache object id for later lookups */
	ndo2db_add_cached_object_id(idi,object_type,name1,name2,*object_id);

	/* free memory */
	free(buf1);
	free(buf2);
	free(es[0]);
	free(es[1]);

	return result;
        }



int ndo2db_get_cached_object_ids(ndo2db_idi *idi){
	int result=NDO_OK;
	unsigned long object_id=0L;
	int objecttype_id=0;
	char *buf=NULL;

	/* find all the object definitions we already have */
	if(asprintf(&buf,"SELECT object_id, objecttype_id, name1, name2 FROM %s WHERE instance_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]
		    ,idi->dbinfo.instance_id
		   )==-1)
		buf=NULL;

	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.mysql_result=mysql_store_result(&idi->dbinfo.mysql_conn);
			while((idi->dbinfo.mysql_row=mysql_fetch_row(idi->dbinfo.mysql_result))!=NULL){

				ndo2db_convert_string_to_unsignedlong(idi->dbinfo.mysql_row[0],&object_id);
				ndo2db_convert_string_to_int(idi->dbinfo.mysql_row[1],&objecttype_id);

				/* add object to cached list */
				ndo2db_add_cached_object_id(idi,objecttype_id,idi->dbinfo.mysql_row[2],idi->dbinfo.mysql_row[3],object_id);
			        }
			mysql_free_result(idi->dbinfo.mysql_result);
			idi->dbinfo.mysql_result=NULL;
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);

	return result;
        }



int ndo2db_get_cached_object_id(ndo2db_idi *idi, int object_type, char *name1, char *name2, unsigned long *object_id){
	int result=NDO_ERROR;
	int hashslot=0;
	int compare=0;
	ndo2db_dbobject *temp_object=NULL;
	int y=0;

	hashslot=ndo2db_object_hashfunc(name1,name2,NDO2DB_OBJECT_HASHSLOTS);
#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT LOOKUP: type=%d, name1=%s, name2=%s\n",object_type,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif

	if(idi->dbinfo.object_hashlist==NULL)
		return NDO_ERROR;

	for(temp_object=idi->dbinfo.object_hashlist[hashslot],y=0;temp_object!=NULL;temp_object=temp_object->nexthash,y++){
#ifdef NDO2DB_DEBUG_CACHING
		printf("OBJECT LOOKUP LOOPING [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n",hashslot,y,temp_object->object_type,temp_object->object_id,(temp_object->name1==NULL)?"NULL":temp_object->name1,(temp_object->name2==NULL)?"NULL":temp_object->name2);
#endif
		compare=ndo2db_compare_object_hashdata(temp_object->name1,temp_object->name2,name1,name2);
		if(compare==0 && temp_object->object_type==object_type)
			break;
	        }

	/* we have a match! */
	if(temp_object && (ndo2db_compare_object_hashdata(temp_object->name1,temp_object->name2,name1,name2)==0) && temp_object->object_type==object_type){
#ifdef NDO2DB_DEBUG_CACHING
		printf("OBJECT CACHE HIT [%d][%d]: type=%d, id=%lu, name1=%s, name2=%s\n",hashslot,y,object_type,temp_object->object_id,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif
		*object_id=temp_object->object_id;
		result=NDO_OK;
	        }
#ifdef NDO2DB_DEBUG_CACHING
	else{
		printf("OBJECT CACHE MISS: type=%d, name1=%s, name2=%s\n",object_type,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
	        }
#endif

	return result;
        }



int ndo2db_add_cached_object_id(ndo2db_idi *idi, int object_type, char *n1, char *n2, unsigned long object_id){
	int result=NDO_OK;
	ndo2db_dbobject *temp_object=NULL;
	ndo2db_dbobject *lastpointer=NULL;
	ndo2db_dbobject *new_object=NULL;
	int x=0;
	int y=0;
	int hashslot=0;
	int compare=0;
	char *name1=NULL;
	char *name2=NULL;

	/* make sure empty strings are set to null */
	name1=n1;
	name2=n2;
	if(name1 && !strcmp(name1,""))
		name1=NULL;
	if(name2 && !strcmp(name2,""))
		name2=NULL;

	/* null names mean no object id, so don't cache */
	if(name1==NULL && name2==NULL){
		return NDO_OK;
	        }

#ifdef NDO2DB_DEBUG_CACHING
	printf("OBJECT CACHE ADD: type=%d, id=%lu, name1=%s, name2=%s\n",object_type,object_id,(name1==NULL)?"NULL":name1,(name2==NULL)?"NULL":name2);
#endif

	/* initialize hash list if necessary */
	if(idi->dbinfo.object_hashlist==NULL){

		idi->dbinfo.object_hashlist=(ndo2db_dbobject **)malloc(sizeof(ndo2db_dbobject *)*NDO2DB_OBJECT_HASHSLOTS);
		if(idi->dbinfo.object_hashlist==NULL)
			return NDO_ERROR;
		
		for(x=0;x<NDO2DB_OBJECT_HASHSLOTS;x++)
			idi->dbinfo.object_hashlist[x]=NULL;
	        }

	/* allocate and populate new object */
	if((new_object=(ndo2db_dbobject *)malloc(sizeof(ndo2db_dbobject)))==NULL)
		return NDO_ERROR;
	new_object->object_type=object_type;
	new_object->object_id=object_id;
	new_object->name1=NULL;
	if(name1)
		new_object->name1=strdup(name1);
	new_object->name2=NULL;
	if(name2)
		new_object->name2=strdup(name2);

	hashslot=ndo2db_object_hashfunc(new_object->name1,new_object->name2,NDO2DB_OBJECT_HASHSLOTS);

	lastpointer=NULL;
	for(temp_object=idi->dbinfo.object_hashlist[hashslot],y=0;temp_object!=NULL;temp_object=temp_object->nexthash,y++){
		compare=ndo2db_compare_object_hashdata(temp_object->name1,temp_object->name2,new_object->name1,new_object->name2);
		if(compare<0)
			break;
		lastpointer=temp_object;
	        }

	if(lastpointer)
		lastpointer->nexthash=new_object;
	else
		idi->dbinfo.object_hashlist[hashslot]=new_object;
	new_object->nexthash=temp_object;

	return result;
        }



int ndo2db_object_hashfunc(const char *name1,const char *name2,int hashslots){
	unsigned int i,result;

	result=0;
	if(name1)
		for(i=0;i<strlen(name1);i++)
			result+=name1[i];

	if(name2)
		for(i=0;i<strlen(name2);i++)
			result+=name2[i];

	result=result%hashslots;

	return result;
        }



int ndo2db_compare_object_hashdata(const char *val1a, const char *val1b, const char *val2a, const char *val2b){
	int result=0;

	/* check first name */
	if(val1a==NULL && val2a==NULL)
		result=0;
	else if(val1a==NULL)
		result=1;
	else if(val2a==NULL)
		result=-1;
	else
		result=strcmp(val1a,val2a);

	/* check second name if necessary */
	if(result==0){
		if(val1b==NULL && val2b==NULL)
			result=0;
		else if(val1b==NULL)
			result=1;
		else if(val2b==NULL)
			result=-1;
		else
			return strcmp(val1b,val2b);
	        }

	return result;
        }



int ndo2db_free_cached_object_ids(ndo2db_idi *idi){
	int x=0;
	ndo2db_dbobject *temp_object=NULL;
	ndo2db_dbobject *next_object=NULL;

	if(idi==NULL)
		return NDO_OK;

	if(idi->dbinfo.object_hashlist){

		for(x=0;x<NDO2DB_OBJECT_HASHSLOTS;x++){
			for(temp_object=idi->dbinfo.object_hashlist[x];temp_object!=NULL;temp_object=next_object){
				next_object=temp_object->nexthash;
				free(temp_object->name1);
				free(temp_object->name2);
				free(temp_object);
			        }
		        }

		free(idi->dbinfo.object_hashlist);
		idi->dbinfo.object_hashlist=NULL;
	        }

	return NDO_OK;
        }



int ndo2db_set_all_objects_as_inactive(ndo2db_idi *idi){
	int result=NDO_OK;
	char *buf=NULL;

	/* mark all objects as being inactive */
	if(asprintf(&buf,"UPDATE %s SET is_active='0' WHERE instance_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]
		    ,idi->dbinfo.instance_id
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);

	return result;
         }



int ndo2db_set_object_as_active(ndo2db_idi *idi, int object_type, unsigned long object_id){
	int result=NDO_OK;
	char *buf=NULL;
	
	/* mark the object as being active */
	if(asprintf(&buf,"UPDATE %s SET is_active='1' WHERE instance_id='%lu' AND objecttype_id='%d' AND object_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_OBJECTS]
		    ,idi->dbinfo.instance_id
		    ,object_type
		    ,object_id
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);

	return result;
        }



/****************************************************************************/
/* ARCHIVED LOG DATA HANDLER                                                */
/****************************************************************************/

int ndo2db_handle_logentry(ndo2db_idi *idi){
	char *ptr=NULL;
	char *buf=NULL;
	char *es[1];
	time_t etime=0L;
	char *ts[1];
	unsigned long type=0L;
	int result=NDO_OK;
	int duplicate_record=NDO_FALSE;
	int len=0;
	int x=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* break log entry in pieces */
	if((ptr=strtok(idi->buffered_input[NDO_DATA_LOGENTRY],"]"))==NULL)
		return NDO_ERROR;
	if((ndo2db_convert_string_to_unsignedlong(ptr+1,(unsigned long *)&etime))==NDO_ERROR)
		return NDO_ERROR;
	ts[0]=ndo2db_db_timet_to_sql(idi,etime);
	if((ptr=strtok(NULL,"\x0"))==NULL)
		return NDO_ERROR;
	es[0]=ndo2db_db_escape_string(idi,(ptr+1));

	/* strip newline chars from end */
	len=strlen(es[0]);
	for(x=len-1;x>=0;x--){
		if(es[0][x]=='\n')
			es[0][x]='\x0';
		else
			break;
	        }

	/* what type of log entry is this? */
	type=0;

	/* make sure we aren't importing a duplicate log entry... */
	if(asprintf(&buf,"SELECT * FROM %s WHERE instance_id='%lu' AND logentry_time=%s AND logentry_data='%s'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES]
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,es[0]
		   )==-1)
		buf=NULL;
	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.mysql_result=mysql_store_result(&idi->dbinfo.mysql_conn);
			if((idi->dbinfo.mysql_row=mysql_fetch_row(idi->dbinfo.mysql_result))!=NULL)
				duplicate_record=NDO_TRUE;
			mysql_free_result(idi->dbinfo.mysql_result);
			idi->dbinfo.mysql_result=NULL;
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);

	/*if(duplicate_record==NDO_TRUE && idi->last_logentry_time!=etime){*/
	/*if(duplicate_record==NDO_TRUE && strcmp((es[0]==NULL)?"":es[0],idi->dbinfo.last_logentry_data)){*/
	if(duplicate_record==NDO_TRUE){
#ifdef NDO2DB_DEBUG
		printf("IGNORING DUPLICATE LOG RECORD!\n");
#endif
		return NDO_OK;
	        }

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', logentry_time=%s, entry_time=%s, entry_time_usec='0', logentry_type='%lu', logentry_data='%s', realtime_data='0', inferred_data_extracted='0'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES]
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,ts[0]
		    ,type
		    ,(es[0]==NULL)?"":es[0]
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* record timestamp of last log entry */
	idi->dbinfo.last_logentry_time=etime;

	/* save last log entry (for detecting duplicates) */
	if(idi->dbinfo.last_logentry_data)
		free(idi->dbinfo.last_logentry_data);
	idi->dbinfo.last_logentry_data=strdup((es[0]==NULL)?"":es[0]);

	/* free memory */
	free(ts[0]);
	free(es[0]);


	/* TODO - further processing of log entry to expand archived data... */


	return result;
        }



/****************************************************************************/
/* REALTIME DATA HANDLERS                                                   */
/****************************************************************************/

int ndo2db_handle_processdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long process_id;
	int result=NDO_OK;
	char *ts[1];
	char *es[3];
	int x=0;
	char *buf=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_PROCESSID],&process_id);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PROGRAMNAME]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PROGRAMVERSION]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PROGRAMDATE]);

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', event_type='%d', event_time=%s, event_time_usec='%lu', process_id='%lu', program_name='%s', program_version='%s', program_date='%s'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_PROCESSEVENTS]
		    ,idi->dbinfo.instance_id
		    ,type
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,process_id
		    ,es[0]
		    ,es[1]
		    ,es[2]
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* MORE PROCESSING.... */

	/* if process is starting up, clearstatus data, event queue, etc. */
	if(type==NEBTYPE_PROCESS_PRELAUNCH && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* clear realtime data */
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]);

		/* clear config data */
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES]);
		ndo2db_db_clear_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS]);

		/* flag all objects as being inactive */
		ndo2db_set_all_objects_as_inactive(idi);

#ifdef BAD_IDEA
		/* record a fake log entry to indicate that Nagios is starting - this normally occurs during the module's "blackout period" */
		if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', logentry_time=%s, logentry_type='%lu', logentry_data='Nagios %s starting... (PID=%lu)'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES]
			    ,idi->dbinfo.instance_id
			    ,ts[0]
			    ,NSLOG_PROCESS_INFO
			    ,es[1]
			    ,process_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
#endif		
	        }

	/* if process is shutting down or restarting, update process status data */
	if((type==NEBTYPE_PROCESS_SHUTDOWN || type==NEBTYPE_PROCESS_RESTART) && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		if(asprintf(&buf,"UPDATE %s SET program_end_time=%s, is_currently_running='0' WHERE instance_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS]
			    ,ts[0]
			    ,idi->dbinfo.instance_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* free memory */
	free(ts[0]);
	for(x=0;x<3;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_timedeventdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int event_type=0;
	unsigned long run_time=0L;
	int recurring_event=0;
	unsigned long object_id=0L;
	int result=NDO_OK;
	char *ts[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTTYPE],&event_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RECURRING],&recurring_event);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_RUNTIME],&run_time);

	/* skip sleep events.... */
	if(type==NEBTYPE_TIMEDEVENT_SLEEP){

		/* we could do some maintenance here if we wanted.... */

		return NDO_OK;
	        }

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,run_time);

	/* get the object id (if applicable) */
	if(event_type==EVENT_SERVICE_CHECK || (event_type==EVENT_SCHEDULED_DOWNTIME && idi->buffered_input[NDO_DATA_SERVICE]!=NULL && strcmp(idi->buffered_input[NDO_DATA_SERVICE],"")))
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(event_type==EVENT_HOST_CHECK || (event_type==EVENT_SCHEDULED_DOWNTIME && (idi->buffered_input[NDO_DATA_SERVICE]==NULL || !strcmp(idi->buffered_input[NDO_DATA_SERVICE],""))))
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);


	/* HISTORICAL TIMED EVENTS */

	/* save a record of timed events that get added */
	if(type==NEBTYPE_TIMEDEVENT_ADD){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', event_type='%d', queued_time=%s, queued_time_usec='%lu', scheduled_time=%s, recurring_event='%d', object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,event_type
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,ts[1]
			    ,recurring_event
			    ,object_id
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save a record of timed events that get executed.... */
	if(type==NEBTYPE_TIMEDEVENT_EXECUTE){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', event_type='%d', event_time=%s, event_time_usec='%lu', scheduled_time=%s, recurring_event='%d', object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,event_type
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,ts[1]
			    ,recurring_event
			    ,object_id
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save a record of timed events that get removed.... */
	if(type==NEBTYPE_TIMEDEVENT_REMOVE){

		/* save entry to db */
		if(asprintf(&buf,"UPDATE %s SET deletion_time=%s, deletion_time_usec='%lu' WHERE instance_id='%lu' AND event_type='%d' AND scheduled_time=%s AND recurring_event='%d' AND object_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS]
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,idi->dbinfo.instance_id
			    ,event_type
			    ,ts[1]
			    ,recurring_event
			    ,object_id
			   )==-1)
			buf=NULL;

		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* CURRENT TIMED EVENTS */

	/* remove (probably) expired events from the queue if client just connected */
	if(idi->dbinfo.clean_event_queue==NDO_TRUE && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		idi->dbinfo.clean_event_queue=NDO_FALSE;

		/* clear old entries from db */
		if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND scheduled_time<=%s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]
			    ,idi->dbinfo.instance_id
			    ,ts[0]
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* ADD QUEUED TIMED EVENTS */
	if(type==NEBTYPE_TIMEDEVENT_ADD  && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* save entry to db */
		if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', event_type='%d', queued_time=%s, queued_time_usec='%lu', scheduled_time=%s, recurring_event='%d', object_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]
			    ,idi->dbinfo.instance_id
			    ,event_type
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,ts[1]
			    ,recurring_event
			    ,object_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* REMOVE QUEUED TIMED EVENTS */
	if((type==NEBTYPE_TIMEDEVENT_REMOVE || type==NEBTYPE_TIMEDEVENT_EXECUTE)  && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* clear entry from db */
		if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND event_type='%d' AND scheduled_time=%s AND recurring_event='%d' AND object_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]
			    ,idi->dbinfo.instance_id
			    ,event_type
			    ,ts[1]
			    ,recurring_event
			    ,object_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);

		/* if we are executing a low-priority event, remove older events from the queue, as we know they've already been executed */
		/* THIS IS A HACK!  It shouldn't be necessary, but for some reason it is...  Otherwise not all events are removed from the queue. :-( */
		if(type==NEBTYPE_TIMEDEVENT_EXECUTE && (event_type==EVENT_SERVICE_CHECK || event_type==EVENT_HOST_CHECK)){

			/* clear entries from db */
			if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND scheduled_time<%s"
				    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE]
				    ,idi->dbinfo.instance_id
				    ,ts[1]
				   )==-1)
				buf=NULL;
			result=ndo2db_db_query(idi,buf);
			free(buf);
		        }

	        }

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);

	return NDO_OK;
        }


int ndo2db_handle_logdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	time_t etime=0L;
	unsigned long letype=0L;
	int result=NDO_OK;
	char *ts[2];
	char *es[1];
	char *buf=NULL;
	int len=0;
	int x=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert data */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LOGENTRYTYPE],&letype);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LOGENTRYTIME],(unsigned long *)&etime);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,etime);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_LOGENTRY]);

	/* strip newline chars from end */
	len=strlen(es[0]);
	for(x=len-1;x>=0;x--){
		if(es[0][x]=='\n')
			es[0][x]='\x0';
		else
			break;
	        }

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', logentry_time=%s, entry_time=%s, entry_time_usec='%lu', logentry_type='%lu', logentry_data='%s', realtime_data='1', inferred_data_extracted='1'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_LOGENTRIES]
		    ,idi->dbinfo.instance_id
		    ,ts[1]
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,letype
		    ,es[0]
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	free(es[0]);

	return NDO_OK;
        }


int ndo2db_handle_systemcommanddata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	struct timeval start_time;
	struct timeval end_time;
	int timeout=0;
	int early_timeout=0;
	double execution_time=0.0;
	int return_code=0;
	char *ts[2];
	char *es[2];
	char *buf=NULL;
	char *buf1=NULL;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* covert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', command_line='%s', timeout='%d', early_timeout='%d', execution_time='%lf', return_code='%d', output='%s'"
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,es[0]
		    ,timeout
		    ,early_timeout
		    ,execution_time
		    ,return_code
		    ,es[1]
		   )==-1)
		buf=NULL;

	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	/* free memory */
	free(ts[0]);
	free(ts[1]);
	free(es[0]);
	free(es[1]);

	return NDO_OK;
        }


int ndo2db_handle_eventhandlerdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[3];
	int x=0;
	int eventhandler_type=0;
	int state=0;
	int state_type=0;
	struct timeval start_time;
	struct timeval end_time;
	int timeout=0;
	int early_timeout=0;
	double execution_time=0.0;
	int return_code=0;
	unsigned long object_id=0L;
	unsigned long command_id=0L;
	char *buf=NULL;
	char *buf1=NULL;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* covert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERTYPE],&eventhandler_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* get the object id */
	if(eventhandler_type==SERVICE_EVENTHANDLER || eventhandler_type==GLOBAL_SERVICE_EVENTHANDLER)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	else
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* get the command id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', eventhandler_type='%d', object_id='%lu', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', command_object_id='%lu', command_args='%s', command_line='%s', timeout='%d', early_timeout='%d', execution_time='%lf', return_code='%d', output='%s'"
		    ,idi->dbinfo.instance_id
		    ,eventhandler_type
		    ,object_id
		    ,state
		    ,state_type
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,command_id
		    ,es[0]
		    ,es[1]
		    ,timeout
		    ,early_timeout
		    ,execution_time
		    ,return_code
		    ,es[2]
		   )==-1)
		buf=NULL;

	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	for(x=0;x<3;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_notificationdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int notification_type=0;
	int notification_reason=0;
	unsigned long object_id=0L;
	struct timeval start_time;
	struct timeval end_time;
	int state=0;
	int escalated=0;
	int contacts_notified=0;
	int result=NDO_OK;
	char *ts[2];
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONTYPE],&notification_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONREASON],&notification_reason);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATED],&escalated);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CONTACTSNOTIFIED],&contacts_notified);

	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* get the object id */
	if(notification_type==SERVICE_NOTIFICATION)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(notification_type==HOST_NOTIFICATION)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', notification_type='%d', notification_reason='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', object_id='%lu', state='%d', output='%s', escalated='%d', contacts_notified='%d'"
		    ,idi->dbinfo.instance_id
		    ,notification_type
		    ,notification_reason
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,object_id
		    ,state
		    ,es[0]
		    ,escalated
		    ,contacts_notified
		   )==-1)
		buf=NULL;

	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	/* run the query */
	result=ndo2db_db_query(idi,buf1);

	/* save the notification id for later use... */
	if(type==NEBTYPE_NOTIFICATION_START)
		idi->dbinfo.last_notification_id=0L;
	if(result==NDO_OK && type==NEBTYPE_NOTIFICATION_START){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.last_notification_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	free(es[0]);

	return NDO_OK;
        }


int ndo2db_handle_contactnotificationdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long contact_id=0L;
	struct timeval start_time;
	struct timeval end_time;
	int result=NDO_OK;
	char *ts[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */

	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* get the contact id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,idi->buffered_input[NDO_DATA_CONTACTNAME],NULL,&contact_id);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', notification_id='%lu', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', contact_object_id='%lu'"
		    ,idi->dbinfo.instance_id
		    ,idi->dbinfo.last_notification_id
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,contact_id
		   )==-1)
		buf=NULL;

	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	/* run the query */
	result=ndo2db_db_query(idi,buf1);

	/* save the contact notification id for later use... */
	if(type==NEBTYPE_CONTACTNOTIFICATION_START)
		idi->dbinfo.last_contact_notification_id=0L;
	if(result==NDO_OK && type==NEBTYPE_CONTACTNOTIFICATION_START){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.last_contact_notification_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);

	return NDO_OK;
        }


int ndo2db_handle_contactnotificationmethoddata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long command_id=0L;
	struct timeval start_time;
	struct timeval end_time;
	int result=NDO_OK;
	char *ts[2];
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */

	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);
	

	/* get the command id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', contactnotification_id='%lu', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', command_object_id='%lu', command_args='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->dbinfo.last_contact_notification_id
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,command_id
		    ,es[0]
		   )==-1)
		buf=NULL;

	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONMETHODS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	/* run the query */
	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	free(es[0]);

	return NDO_OK;
        }


int ndo2db_handle_servicecheckdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[4];
	int check_type=0;
	struct timeval start_time;
	struct timeval end_time;
	int current_check_attempt=0;
	int max_check_attempts=0;
	int state=0;
	int state_type=0;
	int timeout=0;
	int early_timeout=0;
	double execution_time=0.0;
	double latency=0.0;
	int return_code=0;
	unsigned long object_id=0L;
	unsigned long command_id=0L;
	char *buf=NULL;
	char *buf1=NULL;
	int x=0;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* only process some types of service checks... */
	if(type!=NEBTYPE_SERVICECHECK_INITIATE && type!=NEBTYPE_SERVICECHECK_PROCESSED)
		return NDO_OK;

#ifdef BUILD_NAGIOS_3X
	/* skip precheck events - they aren't useful to us */
	if(type==NEBTYPE_SERVICECHECK_ASYNC_PRECHECK)
		return NDO_OK;
#endif

	/* covert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
	es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);

	/* get the command id */
	if(idi->buffered_input[NDO_DATA_COMMANDNAME]!=NULL && strcmp(idi->buffered_input[NDO_DATA_COMMANDNAME],""))
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);
	else
		command_id=0L;

	/* save entry to db */
	if(asprintf(&buf1,"instance_id='%lu', service_object_id='%lu', check_type='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', perfdata='%s'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,check_type
		    ,current_check_attempt
		    ,max_check_attempts
		    ,state
		    ,state_type
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,timeout
		    ,early_timeout
		    ,execution_time
		    ,latency
		    ,return_code
		    ,es[2]
		    ,es[3]
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s, command_object_id='%lu', command_args='%s', command_line='%s' ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS]
		    ,buf1
		    ,command_id
		    ,es[0]
		    ,es[1]
		    ,buf1
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	for(x=0;x<4;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_hostcheckdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	char *ts[2];
	char *es[4];
	int check_type=0;
	int is_raw_check=0;
	struct timeval start_time;
	struct timeval end_time;
	int current_check_attempt=0;
	int max_check_attempts=0;
	int state=0;
	int state_type=0;
	int timeout=0;
	int early_timeout=0;
	double execution_time=0.0;
	double latency=0.0;
	int return_code=0;
	unsigned long object_id=0L;
	unsigned long command_id=0L;
	char *buf=NULL;
	char *buf1=NULL;
	int x=0;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* only process finished host checks... */
	/*
	if(type!=NEBTYPE_HOSTCHECK_PROCESSED)
		return NDO_OK;
	*/

#ifdef BUILD_NAGIOS_3X
	/* skip precheck events - they aren't useful to us */
	if(type==NEBTYPE_HOSTCHECK_ASYNC_PRECHECK || type==NEBTYPE_HOSTCHECK_SYNC_PRECHECK)
		return NDO_OK;
#endif

	/* covert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
	es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);

	ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* get the command id */
	if(idi->buffered_input[NDO_DATA_COMMANDNAME]!=NULL && strcmp(idi->buffered_input[NDO_DATA_COMMANDNAME],""))
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);
	else
		command_id=0L;

	/* is this a raw check? */
	if(type==NEBTYPE_HOSTCHECK_RAW_START || type==NEBTYPE_HOSTCHECK_RAW_END)
		is_raw_check=1;
	else
		is_raw_check=0;

	/* save entry to db */
	if(asprintf(&buf1,"instance_id='%lu', host_object_id='%lu', check_type='%d', is_raw_check='%d', current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', output='%s', perfdata='%s'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,check_type
		    ,is_raw_check
		    ,current_check_attempt
		    ,max_check_attempts
		    ,state
		    ,state_type
		    ,ts[0]
		    ,start_time.tv_usec
		    ,ts[1]
		    ,end_time.tv_usec
		    ,timeout
		    ,early_timeout
		    ,execution_time
		    ,latency
		    ,return_code
		    ,es[2]
		    ,es[3]
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s, command_object_id='%lu', command_args='%s', command_line='%s' ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS]
		    ,buf1
		    ,command_id
		    ,es[0]
		    ,es[1]
		    ,buf1
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<2;x++)
		free(ts[x]);
	for(x=0;x<4;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_commentdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int comment_type=0;
	int entry_type=0;
	unsigned long object_id=0L;
	unsigned long comment_time=0L;
	unsigned long internal_comment_id=0L;
	int is_persistent=0;
	int comment_source=0;
	int expires=0;
	unsigned long expire_time=0L;
	int result=NDO_OK;
	char *ts[3];
	char *es[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_COMMENTTYPE],&comment_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ENTRYTYPE],&entry_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PERSISTENT],&is_persistent);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SOURCE],&comment_source);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EXPIRES],&expires);

	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTID],&internal_comment_id);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME],&comment_time);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_EXPIRATIONTIME],&expire_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,comment_time);
	ts[2]=ndo2db_db_timet_to_sql(idi,expire_time);

	/* get the object id */
	if(comment_type==SERVICE_COMMENT)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(comment_type==HOST_COMMENT)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* ADD HISTORICAL COMMENTS */
	/* save a record of comments that get added (or get loaded and weren't previously recorded).... */
	if(type==NEBTYPE_COMMENT_ADD || type==NEBTYPE_COMMENT_LOAD){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', comment_type='%d', entry_type='%d', object_id='%lu', comment_time=%s, internal_comment_id='%lu', author_name='%s', comment_data='%s', is_persistent='%d', comment_source='%d', expires='%d', expiration_time=%s"
			    ,idi->dbinfo.instance_id
			    ,comment_type
			    ,entry_type
			    ,object_id
			    ,ts[1]
			    ,internal_comment_id
			    ,es[0]
			    ,es[1]
			    ,is_persistent
			    ,comment_source
			    ,expires
			    ,ts[2]
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s, entry_time=%s, entry_time_usec='%lu' ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTHISTORY]
			    ,buf
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* UPDATE HISTORICAL COMMENTS */
	/* mark records that have been deleted */
	if(type==NEBTYPE_COMMENT_DELETE){

		/* update db entry */
		if(asprintf(&buf,"UPDATE %s SET deletion_time=%s, deletion_time_usec='%lu' WHERE instance_id='%lu' AND comment_time=%s AND internal_comment_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTHISTORY]
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,idi->dbinfo.instance_id
			    ,ts[1]
			    ,internal_comment_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* ADD CURRENT COMMENTS */
	if((type==NEBTYPE_COMMENT_ADD || type==NEBTYPE_COMMENT_LOAD) && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', comment_type='%d', entry_type='%d', object_id='%lu', comment_time=%s, internal_comment_id='%lu', author_name='%s', comment_data='%s', is_persistent='%d', comment_source='%d', expires='%d', expiration_time=%s"
			    ,idi->dbinfo.instance_id
			    ,comment_type
			    ,entry_type
			    ,object_id
			    ,ts[1]
			    ,internal_comment_id
			    ,es[0]
			    ,es[1]
			    ,is_persistent
			    ,comment_source
			    ,expires
			    ,ts[2]
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s, entry_time=%s, entry_time_usec='%lu' ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS]
			    ,buf
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* REMOVE CURRENT COMMENTS */
	if(type==NEBTYPE_COMMENT_DELETE  && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* clear entry from db */
		if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND comment_time=%s AND internal_comment_id='%lu'"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS]
			    ,idi->dbinfo.instance_id
			    ,ts[1]
			    ,internal_comment_id
			   )==-1)
			buf=NULL;
		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* free memory */
	for(x=0;x<3;x++)
		free(ts[x]);
	for(x=0;x<2;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_downtimedata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int downtime_type=0;
	int fixed=0;
	unsigned long duration=0L;
	unsigned long internal_downtime_id=0L;
	unsigned long triggered_by=0L;
	unsigned long entry_time=0L;
	unsigned long start_time=0L;
	unsigned long end_time=0L;
	unsigned long object_id=0L;
	int result=NDO_OK;
	char *ts[4];
	char *es[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DOWNTIMETYPE],&downtime_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIXED],&fixed);

	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DURATION],&duration);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DOWNTIMEID],&internal_downtime_id);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_TRIGGEREDBY],&triggered_by);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME],&entry_time);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,entry_time);
	ts[2]=ndo2db_db_timet_to_sql(idi,start_time);
	ts[3]=ndo2db_db_timet_to_sql(idi,end_time);

	/* get the object id */
	if(downtime_type==SERVICE_DOWNTIME)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(downtime_type==HOST_DOWNTIME)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* HISTORICAL DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if(type==NEBTYPE_DOWNTIME_ADD || type==NEBTYPE_DOWNTIME_LOAD){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', downtime_type='%d', object_id='%lu', entry_time=%s, author_name='%s', comment_data='%s', internal_downtime_id='%lu', triggered_by_id='%lu', is_fixed='%d', duration='%lu', scheduled_start_time=%s, scheduled_end_time=%s"
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,es[0]
			    ,es[1]
			    ,internal_downtime_id
			    ,triggered_by
			    ,fixed
			    ,duration
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save a record of scheduled downtime that starts */
	if(type==NEBTYPE_DOWNTIME_START){

		/* save entry to db */
		if(asprintf(&buf,"UPDATE %s SET actual_start_time=%s, actual_start_time_usec='%lu', was_started='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY]
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,1
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* save a record of scheduled downtime that ends */
	if(type==NEBTYPE_DOWNTIME_STOP){

		/* save entry to db */
		if(asprintf(&buf,"UPDATE %s SET actual_end_time=%s, actual_end_time_usec='%lu', was_cancelled='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY]
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,(attr==NEBATTR_DOWNTIME_STOP_CANCELLED)?1:0
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }


	/* CURRENT DOWNTIME */

	/* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
	if((type==NEBTYPE_DOWNTIME_ADD || type==NEBTYPE_DOWNTIME_LOAD) && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* save entry to db */
		if(asprintf(&buf,"instance_id='%lu', downtime_type='%d', object_id='%lu', entry_time=%s, author_name='%s', comment_data='%s', internal_downtime_id='%lu', triggered_by_id='%lu', is_fixed='%d', duration='%lu', scheduled_start_time=%s, scheduled_end_time=%s"
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,es[0]
			    ,es[1]
			    ,internal_downtime_id
			    ,triggered_by
			    ,fixed
			    ,duration
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save a record of scheduled downtime that starts */
	if(type==NEBTYPE_DOWNTIME_START && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* save entry to db */
		if(asprintf(&buf,"UPDATE %s SET actual_start_time=%s, actual_start_time_usec='%lu', was_started='%d' WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
			    ,ts[0]
			    ,tstamp.tv_usec
			    ,1
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* remove completed or deleted downtime */
	if((type==NEBTYPE_DOWNTIME_STOP || type==NEBTYPE_DOWNTIME_DELETE) && tstamp.tv_sec>=idi->dbinfo.latest_realtime_data_time){

		/* save entry to db */
		if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND downtime_type='%d' AND object_id='%lu' AND entry_time=%s AND scheduled_start_time=%s AND scheduled_end_time=%s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SCHEDULEDDOWNTIME]
			    ,idi->dbinfo.instance_id
			    ,downtime_type
			    ,object_id
			    ,ts[1]
			    ,ts[2]
			    ,ts[3]
			   )==-1)
			buf=NULL;

		result=ndo2db_db_query(idi,buf);
		free(buf);
	        }

	/* free memory */
	for(x=0;x<4;x++)
		free(ts[x]);
	for(x=0;x<2;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_flappingdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int flapping_type=0;
	unsigned long object_id=0L;
	double percent_state_change=0.0;
	double low_threshold=0.0;
	double high_threshold=0.0;
	unsigned long comment_time=0L;
	unsigned long internal_comment_id=0L;
	int result=NDO_OK;
	char *ts[2];
	char *buf=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPPINGTYPE],&flapping_type);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE],&percent_state_change);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWTHRESHOLD],&low_threshold);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHTHRESHOLD],&high_threshold);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTTIME],&comment_time);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTID],&internal_comment_id);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,comment_time);

	/* get the object id (if applicable) */
	if(flapping_type==SERVICE_FLAPPING)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(flapping_type==HOST_FLAPPING)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', event_time=%s, event_time_usec='%lu', event_type='%d', reason_type='%d', flapping_type='%d', object_id='%lu', percent_state_change='%lf', low_threshold='%lf', high_threshold='%lf', comment_time=%s, internal_comment_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_FLAPPINGHISTORY]
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,type
		    ,attr
		    ,flapping_type
		    ,object_id
		    ,percent_state_change
		    ,low_threshold
		    ,high_threshold
		    ,ts[1]
		    ,internal_comment_id
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* free memory */
	free(ts[0]);
	free(ts[1]);

	return NDO_OK;
        }


int ndo2db_handle_programstatusdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long program_start_time=0L;
	unsigned long process_id=0L;
	int daemon_mode=0;
	unsigned long last_command_check=0L;
	unsigned long last_log_rotation=0L;
	int notifications_enabled=0;
	int active_service_checks_enabled=0;
	int passive_service_checks_enabled=0;
	int active_host_checks_enabled=0;
	int passive_host_checks_enabled=0;
	int event_handlers_enabled=0;
	int flap_detection_enabled=0;
	int failure_prediction_enabled=0;
	int process_performance_data=0;
	int obsess_over_hosts=0;
	int obsess_over_services=0;
	unsigned long modified_host_attributes=0L;
	unsigned long modified_service_attributes=0L;
	char *ts[4];
	char *es[2];
	char *buf=NULL;
	char *buf1=NULL;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_PROGRAMSTARTTIME],&program_start_time);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_PROCESSID],&process_id);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DAEMONMODE],&daemon_mode);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTCOMMANDCHECK],&last_command_check);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTLOGROTATION],&last_log_rotation);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED],&notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED],&active_service_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED],&passive_service_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED],&active_host_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED],&passive_host_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERSENABLED],&event_handlers_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED],&flap_detection_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED],&failure_prediction_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA],&process_performance_data);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOSTS],&obsess_over_hosts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICES],&obsess_over_services);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES],&modified_host_attributes);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES],&modified_service_attributes);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_GLOBALHOSTEVENTHANDLER]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_GLOBALSERVICEEVENTHANDLER]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,program_start_time);
	ts[2]=ndo2db_db_timet_to_sql(idi,last_command_check);
	ts[3]=ndo2db_db_timet_to_sql(idi,last_log_rotation);

	/* generate query string */
	if(asprintf(&buf1,"instance_id='%lu', status_update_time=%s, program_start_time=%s, is_currently_running='1', process_id='%lu', daemon_mode='%d', last_command_check=%s, last_log_rotation=%s, notifications_enabled='%d', active_service_checks_enabled='%d', passive_service_checks_enabled='%d', active_host_checks_enabled='%d', passive_host_checks_enabled='%d', event_handlers_enabled='%d', flap_detection_enabled='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_hosts='%d', obsess_over_services='%d', modified_host_attributes='%lu', modified_service_attributes='%lu', global_host_event_handler='%s', global_service_event_handler='%s'"
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,ts[1]
		    ,process_id
		    ,daemon_mode
		    ,ts[2]
		    ,ts[3]
		    ,notifications_enabled
		    ,active_service_checks_enabled
		    ,passive_service_checks_enabled
		    ,active_host_checks_enabled
		    ,passive_host_checks_enabled
		    ,event_handlers_enabled
		    ,flap_detection_enabled
		    ,failure_prediction_enabled
		    ,process_performance_data
		    ,obsess_over_hosts
		    ,obsess_over_services
		    ,modified_host_attributes
		    ,modified_service_attributes
		    ,es[0]
		    ,es[1]
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS]
		    ,buf1
		    ,buf1
		   )==-1)
		buf=NULL;

	/* save entry to db */
	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* free memory */
	free(ts[0]);
	free(ts[1]);
	free(ts[2]);
	free(ts[3]);
	free(es[0]);
	free(es[1]);

	return NDO_OK;
        }


int ndo2db_handle_hoststatusdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long last_check=0L;
	unsigned long next_check=0L;
	unsigned long last_state_change=0L;
	unsigned long last_hard_state_change=0L;
	unsigned long last_time_up=0L;
	unsigned long last_time_down=0L;
	unsigned long last_time_unreachable=0L;
	unsigned long last_notification=0L;
	unsigned long next_notification=0L;
	unsigned long modified_host_attributes=0L;
	double percent_state_change=0.0;
	double latency=0.0;
	double execution_time=0.0;
	int current_state=0;
	int has_been_checked=0;
	int should_be_scheduled=0;
	int current_check_attempt=0;
	int max_check_attempts=0;
	int check_type=0;
	int last_hard_state=0;
	int state_type=0;
	int no_more_notifications=0;
	int notifications_enabled=0;
	int problem_has_been_acknowledged=0;
	int acknowledgement_type=0;
	int current_notification_number=0;
	int passive_checks_enabled=0;
	int active_checks_enabled=0;
	int event_handler_enabled;
	int flap_detection_enabled=0;
	int is_flapping=0;
	int scheduled_downtime_depth=0;
	int failure_prediction_enabled=0;
	int process_performance_data;
	int obsess_over_host=0;
	double normal_check_interval=0.0;
	double retry_check_interval=0.0;
	char *ts[10];
	char *es[4];
	char *buf=NULL;
	char *buf1=NULL;
	unsigned long object_id=0L;
	unsigned long check_timeperiod_object_id=0L;
	int x=0;
	int result=NDO_OK;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHOSTCHECK],&last_check);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTHOSTCHECK],&next_check);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSTATECHANGE],&last_state_change);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHARDSTATECHANGE],&last_hard_state_change);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUP],&last_time_up);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEDOWN],&last_time_down);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUNREACHABLE],&last_time_unreachable);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHOSTNOTIFICATION],&last_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTHOSTNOTIFICATION],&next_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES],&modified_host_attributes);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE],&percent_state_change);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTSTATE],&current_state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HASBEENCHECKED],&has_been_checked);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SHOULDBESCHEDULED],&should_be_scheduled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE],&last_hard_state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOMORENOTIFICATIONS],&no_more_notifications);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED],&notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROBLEMHASBEENACKNOWLEDGED],&problem_has_been_acknowledged);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE],&acknowledgement_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTNOTIFICATIONNUMBER],&current_notification_number);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED],&passive_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED],&active_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERENABLED],&event_handler_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED],&flap_detection_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ISFLAPPING],&is_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SCHEDULEDDOWNTIMEDEPTH],&scheduled_downtime_depth);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED],&failure_prediction_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA],&process_performance_data);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOST],&obsess_over_host);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NORMALCHECKINTERVAL],&normal_check_interval);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_RETRYCHECKINTERVAL],&retry_check_interval);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_EVENTHANDLER]);
	es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_CHECKCOMMAND]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,last_check);
	ts[2]=ndo2db_db_timet_to_sql(idi,next_check);
	ts[3]=ndo2db_db_timet_to_sql(idi,last_state_change);
	ts[4]=ndo2db_db_timet_to_sql(idi,last_hard_state_change);
	ts[5]=ndo2db_db_timet_to_sql(idi,last_time_up);
	ts[6]=ndo2db_db_timet_to_sql(idi,last_time_down);
	ts[7]=ndo2db_db_timet_to_sql(idi,last_time_unreachable);
	ts[8]=ndo2db_db_timet_to_sql(idi,last_notification);
	ts[9]=ndo2db_db_timet_to_sql(idi,next_notification);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_HOSTCHECKPERIOD],NULL,&check_timeperiod_object_id);

	/* generate query string */
	if(asprintf(&buf1,"instance_id='%lu', host_object_id='%lu', status_update_time=%s, output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_up=%s, last_time_down=%s, last_time_unreachable=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_host='%d', modified_host_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,ts[0]
		    ,es[0]
		    ,es[1]
		    ,current_state
		    ,has_been_checked
		    ,should_be_scheduled
		    ,current_check_attempt
		    ,max_check_attempts
		    ,ts[1]
		    ,ts[2]
		    ,check_type
		    ,ts[3]
		    ,ts[4]
		    ,last_hard_state
		    ,ts[5]
		    ,ts[6]
		    ,ts[7]
		    ,state_type
		    ,ts[8]
		    ,ts[9]
		    ,no_more_notifications
		    ,notifications_enabled
		    ,problem_has_been_acknowledged
		    ,acknowledgement_type
		    ,current_notification_number
		    ,passive_checks_enabled
		    ,active_checks_enabled
		    ,event_handler_enabled
		    ,flap_detection_enabled
		    ,is_flapping
		    ,percent_state_change
		    ,latency
		    ,execution_time
		    ,scheduled_downtime_depth
		    ,failure_prediction_enabled
		    ,process_performance_data
		    ,obsess_over_host
		    ,modified_host_attributes
		    ,es[2]
		    ,es[3]
		    ,normal_check_interval
		    ,retry_check_interval
		    ,check_timeperiod_object_id
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS]
		    ,buf1
		    ,buf1
		   )==-1)
		buf=NULL;

	/* save entry to db */
	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<4;x++)
		free(es[x]);

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu', status_update_time=%s, has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,object_id
			    ,ts[0]
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* free memory */
	for(x=0;x<10;x++)
		free(ts[x]);

	return NDO_OK;
        }


int ndo2db_handle_servicestatusdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long last_check=0L;
	unsigned long next_check=0L;
	unsigned long last_state_change=0L;
	unsigned long last_hard_state_change=0L;
	unsigned long last_time_ok=0L;
	unsigned long last_time_warning=0L;
	unsigned long last_time_unknown=0L;
	unsigned long last_time_critical=0L;
	unsigned long last_notification=0L;
	unsigned long next_notification=0L;
	unsigned long modified_service_attributes=0L;
	double percent_state_change=0.0;
	double latency=0.0;
	double execution_time=0.0;
	int current_state=0;
	int has_been_checked=0;
	int should_be_scheduled=0;
	int current_check_attempt=0;
	int max_check_attempts=0;
	int check_type=0;
	int last_hard_state=0;
	int state_type=0;
	int no_more_notifications=0;
	int notifications_enabled=0;
	int problem_has_been_acknowledged=0;
	int acknowledgement_type=0;
	int current_notification_number=0;
	int passive_checks_enabled=0;
	int active_checks_enabled=0;
	int event_handler_enabled;
	int flap_detection_enabled=0;
	int is_flapping=0;
	int scheduled_downtime_depth=0;
	int failure_prediction_enabled=0;
	int process_performance_data;
	int obsess_over_service=0;
	double normal_check_interval=0.0;
	double retry_check_interval=0.0;
	char *ts[11];
	char *es[4];
	char *buf=NULL;
	char *buf1=NULL;
	unsigned long object_id=0L;
	unsigned long check_timeperiod_object_id=0L;
	int x=0;
	int result=NDO_OK;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSERVICECHECK],&last_check);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTSERVICECHECK],&next_check);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSTATECHANGE],&last_state_change);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHARDSTATECHANGE],&last_hard_state_change);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEOK],&last_time_ok);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEWARNING],&last_time_warning);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMEUNKNOWN],&last_time_unknown);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTTIMECRITICAL],&last_time_critical);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSERVICENOTIFICATION],&last_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_NEXTSERVICENOTIFICATION],&next_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES],&modified_service_attributes);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_PERCENTSTATECHANGE],&percent_state_change);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTSTATE],&current_state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HASBEENCHECKED],&has_been_checked);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SHOULDBESCHEDULED],&should_be_scheduled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE],&last_hard_state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOMORENOTIFICATIONS],&no_more_notifications);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONSENABLED],&notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROBLEMHASBEENACKNOWLEDGED],&problem_has_been_acknowledged);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE],&acknowledgement_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTNOTIFICATIONNUMBER],&current_notification_number);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED],&passive_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED],&active_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERENABLED],&event_handler_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONENABLED],&flap_detection_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ISFLAPPING],&is_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SCHEDULEDDOWNTIMEDEPTH],&scheduled_downtime_depth);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILUREPREDICTIONENABLED],&failure_prediction_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSPERFORMANCEDATA],&process_performance_data);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICE],&obsess_over_service);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NORMALCHECKINTERVAL],&normal_check_interval);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_RETRYCHECKINTERVAL],&retry_check_interval);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_EVENTHANDLER]);
	es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_CHECKCOMMAND]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,last_check);
	ts[2]=ndo2db_db_timet_to_sql(idi,next_check);
	ts[3]=ndo2db_db_timet_to_sql(idi,last_state_change);
	ts[4]=ndo2db_db_timet_to_sql(idi,last_hard_state_change);
	ts[5]=ndo2db_db_timet_to_sql(idi,last_time_ok);
	ts[6]=ndo2db_db_timet_to_sql(idi,last_time_warning);
	ts[7]=ndo2db_db_timet_to_sql(idi,last_time_unknown);
	ts[8]=ndo2db_db_timet_to_sql(idi,last_time_critical);
	ts[9]=ndo2db_db_timet_to_sql(idi,last_notification);
	ts[10]=ndo2db_db_timet_to_sql(idi,next_notification);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_SERVICECHECKPERIOD],NULL,&check_timeperiod_object_id);

	/* generate query string */
	if(asprintf(&buf1,"instance_id='%lu', service_object_id='%lu', status_update_time=%s, output='%s', perfdata='%s', current_state='%d', has_been_checked='%d', should_be_scheduled='%d', current_check_attempt='%d', max_check_attempts='%d', last_check=%s, next_check=%s, check_type='%d', last_state_change=%s, last_hard_state_change=%s, last_hard_state='%d', last_time_ok=%s, last_time_warning=%s, last_time_unknown=%s, last_time_critical=%s, state_type='%d', last_notification=%s, next_notification=%s, no_more_notifications='%d', notifications_enabled='%d', problem_has_been_acknowledged='%d', acknowledgement_type='%d', current_notification_number='%d', passive_checks_enabled='%d', active_checks_enabled='%d', event_handler_enabled='%d', flap_detection_enabled='%d', is_flapping='%d', percent_state_change='%lf', latency='%lf', execution_time='%lf', scheduled_downtime_depth='%d', failure_prediction_enabled='%d', process_performance_data='%d', obsess_over_service='%d', modified_service_attributes='%lu', event_handler='%s', check_command='%s', normal_check_interval='%lf', retry_check_interval='%lf', check_timeperiod_object_id='%lu'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,ts[0]
		    ,es[0]
		    ,es[1]
		    ,current_state
		    ,has_been_checked
		    ,should_be_scheduled
		    ,current_check_attempt
		    ,max_check_attempts
		    ,ts[1]
		    ,ts[2]
		    ,check_type
		    ,ts[3]
		    ,ts[4]
		    ,last_hard_state
		    ,ts[5]
		    ,ts[6]
		    ,ts[7]
		    ,ts[8]
		    ,state_type
		    ,ts[9]
		    ,ts[10]
		    ,no_more_notifications
		    ,notifications_enabled
		    ,problem_has_been_acknowledged
		    ,acknowledgement_type
		    ,current_notification_number
		    ,passive_checks_enabled
		    ,active_checks_enabled
		    ,event_handler_enabled
		    ,flap_detection_enabled
		    ,is_flapping
		    ,percent_state_change
		    ,latency
		    ,execution_time
		    ,scheduled_downtime_depth
		    ,failure_prediction_enabled
		    ,process_performance_data
		    ,obsess_over_service
		    ,modified_service_attributes
		    ,es[2]
		    ,es[3]
		    ,normal_check_interval
		    ,retry_check_interval
		    ,check_timeperiod_object_id
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS]
		    ,buf1
		    ,buf1
		   )==-1)
		buf=NULL;

	/* save entry to db */
	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* free memory */
	for(x=0;x<4;x++)
		free(es[x]);

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu', status_update_time=%s, has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,object_id
			    ,ts[0]
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* free memory */
	for(x=0;x<11;x++)
		free(ts[x]);

	return NDO_OK;
        }


int ndo2db_handle_contactstatusdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long last_host_notification=0L;
	unsigned long last_service_notification=0L;
	unsigned long modified_attributes=0L;
	unsigned long modified_host_attributes=0L;
	unsigned long modified_service_attributes=0L;
	int host_notifications_enabled=0;
	int service_notifications_enabled=0;
	char *es[2];
	char *ts[3];
	char *buf=NULL;
	char *buf1=NULL;
	unsigned long object_id=0L;
	int x=0;
	int result=NDO_OK;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec < idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* covert vars */
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTHOSTNOTIFICATION],&last_host_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_LASTSERVICENOTIFICATION],&last_service_notification);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDCONTACTATTRIBUTES],&modified_attributes);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDHOSTATTRIBUTES],&modified_host_attributes);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_MODIFIEDSERVICEATTRIBUTES],&modified_service_attributes);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED],&host_notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED],&service_notifications_enabled);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
	ts[1]=ndo2db_db_timet_to_sql(idi,last_host_notification);
	ts[2]=ndo2db_db_timet_to_sql(idi,last_service_notification);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,idi->buffered_input[NDO_DATA_CONTACTNAME],NULL,&object_id);

	/* generate query string */
	if(asprintf(&buf1,"instance_id='%lu', contact_object_id='%lu', status_update_time=%s, host_notifications_enabled='%d', service_notifications_enabled='%d', last_host_notification=%s, last_service_notification=%s, modified_attributes='%lu', modified_host_attributes='%lu', modified_service_attributes='%lu'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,ts[0]
		    ,host_notifications_enabled
		    ,service_notifications_enabled
		    ,ts[1]
		    ,ts[2]
		    ,modified_attributes
		    ,modified_host_attributes
		    ,modified_service_attributes
		   )==-1)
		buf1=NULL;

	if(asprintf(&buf,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS]
		    ,buf1
		    ,buf1
		   )==-1)
		buf=NULL;

	/* save entry to db */
	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(buf1);

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu',status_update_time=%s, has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,object_id
			    ,ts[0]
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLESTATUS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* free memory */
	for(x=0;x<3;x++)
		free(ts[x]);

	return NDO_OK;
        }


int ndo2db_handle_adaptiveprogramdata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_adaptivehostdata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_adaptiveservicedata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_adaptivecontactdata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_externalcommanddata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	char *ts=NULL;
	char *es[2];
	int command_type=0;
	unsigned long entry_time=0L;
	char *buf=NULL;
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* only handle start events */
	if(type!=NEBTYPE_EXTERNALCOMMAND_START)
		return NDO_OK;

	/* covert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_COMMANDTYPE],&command_type);
	result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME],&entry_time);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDSTRING]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);

	ts=ndo2db_db_timet_to_sql(idi,entry_time);

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', command_type='%d', entry_time=%s, command_name='%s', command_args='%s'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_EXTERNALCOMMANDS]
		    ,idi->dbinfo.instance_id
		    ,command_type
		    ,ts
		    ,es[0]
		    ,es[1]
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* free memory */
	free(ts);
	free(es[0]);
	free(es[1]);

	return NDO_OK;
        }


int ndo2db_handle_aggregatedstatusdata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_retentiondata(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* IGNORED */

	return NDO_OK;
        }


int ndo2db_handle_acknowledgementdata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int acknowledgement_type=0;
	int state=0;
	int is_sticky=0;
	int persistent_comment=0;
	int notify_contacts=0;
	unsigned long object_id=0L;
	int result=NDO_OK;
	char *ts[1];
	char *es[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACKNOWLEDGEMENTTYPE],&acknowledgement_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STICKY],&is_sticky);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PERSISTENT],&persistent_comment);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYCONTACTS],&notify_contacts);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_AUTHORNAME]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMENT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);

	/* get the object id */
	if(acknowledgement_type==SERVICE_ACKNOWLEDGEMENT)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	if(acknowledgement_type==HOST_ACKNOWLEDGEMENT)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* save entry to db */
	if(asprintf(&buf,"instance_id='%lu', entry_time=%s, entry_time_usec='%lu', acknowledgement_type='%d', object_id='%lu', state='%d', author_name='%s', comment_data='%s', is_sticky='%d', persistent_comment='%d', notify_contacts='%d'"
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,acknowledgement_type
		    ,object_id
		    ,state
		    ,es[0]
		    ,es[1]
		    ,is_sticky
		    ,persistent_comment
		    ,notify_contacts
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_ACKNOWLEDGEMENTS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;
	
	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	/* free memory */
	free(ts[0]);
	for(x=0;x<2;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_statechangedata(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int statechange_type=0;
	int state_change_occurred=0;
	int state=0;
	int state_type=0;
	int current_attempt=0;
	int max_attempts=0;
	int last_state=-1;
	int last_hard_state=-1;
	unsigned long object_id=0L;
	int result=NDO_OK;
	char *ts[1];
	char *es[1];
	char *buf=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* only process completed state changes */
	if(type!=NEBTYPE_STATECHANGE_END)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATECHANGETYPE],&statechange_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATECHANGE],&state_change_occurred);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_attempt);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_attempts);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTHARDSTATE],&last_hard_state);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTSTATE],&last_state);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);

	ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);

	/* get the object id */
	if(statechange_type==SERVICE_STATECHANGE)
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
	else
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

	/* save entry to db */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', state_time=%s, state_time_usec='%lu', object_id='%lu', state_change='%d', state='%d', state_type='%d', current_check_attempt='%d', max_check_attempts='%d', last_state='%d', last_hard_state='%d', output='%s'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_STATEHISTORY]
		    ,idi->dbinfo.instance_id
		    ,ts[0]
		    ,tstamp.tv_usec
		    ,object_id
		    ,state_change_occurred
		    ,state
		    ,state_type
		    ,current_attempt
		    ,max_attempts
		    ,last_state
		    ,last_hard_state
		    ,es[0]
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);

	/* free memory */
	free(ts[0]);
	free(es[0]);

	return NDO_OK;
        }



/****************************************************************************/
/* VARIABLE DATA HANDLERS                                                   */
/****************************************************************************/

int ndo2db_handle_configfilevariables(ndo2db_idi *idi, int configfile_type){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long configfile_id=0L;
	int result=NDO_OK;
	char *es[3];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	char *varname=NULL;
	char *varvalue=NULL;
	ndo2db_mbuf mbuf;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL,0,"HANDLE_CONFIGFILEVARS [1]\n");

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL,0,"HANDLE_CONFIGFILEVARS [2]\n");
	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL,0,"TSTAMP: %lu   LATEST: %lu\n",tstamp.tv_sec,idi->dbinfo.latest_realtime_data_time);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL,0,"HANDLE_CONFIGFILEVARS [3]\n");

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_CONFIGFILENAME]);

	/* add config file to db */
	if(asprintf(&buf,"instance_id='%lu', configfile_type='%d', configfile_path='%s'"
		    ,idi->dbinfo.instance_id
		    ,configfile_type
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILES]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			configfile_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	free(es[0]);

	/* save config file variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONFIGFILEVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get var name/val pair */
		varname=strtok(mbuf.buffer[x],"=");
		varvalue=strtok(NULL,"\x0");

		es[1]=ndo2db_db_escape_string(idi,varname);
		es[2]=ndo2db_db_escape_string(idi,varvalue);

		if(asprintf(&buf,"instance_id='%lu', configfile_id='%lu', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,configfile_id
			    ,es[1]
			    ,es[2]
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES]
			    ,buf
			   )==-1)
			buf1=NULL;
#ifdef REMOVED_10182007
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONFIGFILEVARIABLES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;
#endif

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);

		free(es[1]);
		free(es[2]);
	        }

	return NDO_OK;
        }



int ndo2db_handle_configvariables(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_handle_runtimevariables(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int result=NDO_OK;
	char *es[2];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	char *varname=NULL;
	char *varvalue=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* save config file variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_RUNTIMEVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get var name/val pair */
		varname=strtok(mbuf.buffer[x],"=");
		varvalue=strtok(NULL,"\x0");

		es[0]=ndo2db_db_escape_string(idi,varname);
		es[1]=ndo2db_db_escape_string(idi,varvalue);

		if(asprintf(&buf,"instance_id='%lu', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,es[0]
			    ,es[1]
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_RUNTIMEVARIABLES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);

		free(es[0]);
		free(es[1]);
	        }

	return NDO_OK;
        }



/****************************************************************************/
/* OBJECT DEFINITION DATA HANDLERS                                          */
/****************************************************************************/

int ndo2db_handle_configdumpstart(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	int result=NDO_OK;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* set config dump type */
	if(idi->buffered_input[NDO_DATA_CONFIGDUMPTYPE]!=NULL && !strcmp(idi->buffered_input[NDO_DATA_CONFIGDUMPTYPE],NDO_API_CONFIGDUMP_RETAINED))
		idi->current_object_config_type=1;
	else
		idi->current_object_config_type=0;

	return NDO_OK;
        }


int ndo2db_handle_configdumpend(ndo2db_idi *idi){

	return NDO_OK;
        }


int ndo2db_handle_hostdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long check_timeperiod_id=0L;
	unsigned long notification_timeperiod_id=0L;
	unsigned long check_command_id=0L;
	unsigned long eventhandler_command_id=0L;
	double check_interval=0.0;
	double retry_interval=0.0;
	int max_check_attempts=0;
	double first_notification_delay=0.0;
	double notification_interval=0.0;
	int notify_on_down=0;
	int notify_on_unreachable=0;
	int notify_on_recovery=0;
	int notify_on_flapping=0;
	int notify_on_downtime=0;
	int stalk_on_up=0;
	int stalk_on_down=0;
	int stalk_on_unreachable=0;
	int flap_detection_enabled=0;
	int flap_detection_on_up=0;
	int flap_detection_on_down=0;
	int flap_detection_on_unreachable=0;
	int process_performance_data=0;
	int freshness_checks_enabled=0;
	int freshness_threshold=0;
	int passive_checks_enabled=0;
	int event_handler_enabled=0;
	int active_checks_enabled=0;
	int retain_status_information=0;
	int retain_nonstatus_information=0;
	int notifications_enabled=0;
	int obsess_over_host=0;
	int failure_prediction_enabled=0;
	double low_flap_threshold=0.0;
	double high_flap_threshold=0.0;
	int have_2d_coords=0;
	int x_2d=0;
	int y_2d=0;
	int have_3d_coords=0;
	double x_3d=0.0;
	double y_3d=0.0;
	double z_3d=0.0;
	unsigned long host_id=0L;
	unsigned long member_id=0L;
	int result=NDO_OK;
	char *es[13];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;
	char *cmdptr=NULL;
	char *argptr=NULL;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTCHECKINTERVAL],&check_interval);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTRETRYINTERVAL],&retry_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTMAXCHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATIONDELAY],&first_notification_delay);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONINTERVAL],&notification_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWN],&notify_on_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTUNREACHABLE],&notify_on_unreachable);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTRECOVERY],&notify_on_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTFLAPPING],&notify_on_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWNTIME],&notify_on_downtime);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONUP],&stalk_on_up);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONDOWN],&stalk_on_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKHOSTONUNREACHABLE],&stalk_on_unreachable);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFLAPDETECTIONENABLED],&flap_detection_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUP],&flap_detection_on_up);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONDOWN],&flap_detection_on_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUNREACHABLE],&flap_detection_on_unreachable);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSHOSTPERFORMANCEDATA],&process_performance_data);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFRESHNESSCHECKSENABLED],&freshness_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFRESHNESSTHRESHOLD],&freshness_threshold);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVEHOSTCHECKSENABLED],&passive_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTEVENTHANDLERENABLED],&event_handler_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVEHOSTCHECKSENABLED],&active_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINHOSTSTATUSINFORMATION],&retain_status_information);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINHOSTNONSTATUSINFORMATION],&retain_nonstatus_information);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED],&notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERHOST],&obsess_over_host);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTFAILUREPREDICTIONENABLED],&failure_prediction_enabled);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWHOSTFLAPTHRESHOLD],&low_flap_threshold);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHHOSTFLAPTHRESHOLD],&high_flap_threshold);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HAVE2DCOORDS],&have_2d_coords);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_X2D],&x_2d);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_Y3D],&y_2d);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HAVE3DCOORDS],&have_3d_coords);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_X3D],&x_3d);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_Y3D],&y_3d);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_Z3D],&z_3d);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_HOSTADDRESS]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr=strtok(idi->buffered_input[NDO_DATA_HOSTCHECKCOMMAND],"!");
	argptr=strtok(NULL,"\x0");
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&check_command_id);
	es[2]=ndo2db_db_escape_string(idi,argptr);
	
	/* get the event handler command */
	cmdptr=strtok(idi->buffered_input[NDO_DATA_HOSTEVENTHANDLER],"!");
	argptr=strtok(NULL,"\x0");
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&eventhandler_command_id);
	es[3]=ndo2db_db_escape_string(idi,argptr);
	
	es[4]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_NOTES]);
	es[5]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_NOTESURL]);
	es[6]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ACTIONURL]);
	es[7]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ICONIMAGE]);
	es[8]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ICONIMAGEALT]);
	es[9]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_VRMLIMAGE]);
	es[10]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_STATUSMAPIMAGE]);
	es[11]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_DISPLAYNAME]);
	es[12]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_HOSTALIAS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOSTNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_HOST,object_id);

	/* get the timeperiod ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_HOSTCHECKPERIOD],NULL,&check_timeperiod_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONPERIOD],NULL,&notification_timeperiod_id);

 	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', host_object_id='%lu', alias='%s', display_name='%s', address='%s', check_command_object_id='%lu', check_command_args='%s', eventhandler_command_object_id='%lu', eventhandler_command_args='%s', check_timeperiod_object_id='%lu', notification_timeperiod_object_id='%lu', failure_prediction_options='%s', check_interval='%lf', retry_interval='%lf', max_check_attempts='%d', first_notification_delay='%lf', notification_interval='%lf', notify_on_down='%d', notify_on_unreachable='%d', notify_on_recovery='%d', notify_on_flapping='%d', notify_on_downtime='%d', stalk_on_up='%d', stalk_on_down='%d', stalk_on_unreachable='%d', flap_detection_enabled='%d', flap_detection_on_up='%d', flap_detection_on_down='%d', flap_detection_on_unreachable='%d', low_flap_threshold='%lf', high_flap_threshold='%lf', process_performance_data='%d', freshness_checks_enabled='%d', freshness_threshold='%d', passive_checks_enabled='%d', event_handler_enabled='%d', active_checks_enabled='%d', retain_status_information='%d', retain_nonstatus_information='%d', notifications_enabled='%d', obsess_over_host='%d', failure_prediction_enabled='%d', notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s', vrml_image='%s', statusmap_image='%s', have_2d_coords='%d', x_2d='%d', y_2d='%d', have_3d_coords='%d', x_3d='%lf', y_3d='%lf', z_3d='%lf'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,(es[12]==NULL)?"":es[12]
		    ,(es[11]==NULL)?"":es[11]
		    ,(es[0]==NULL)?"":es[0]
		    ,check_command_id
		    ,(es[2]==NULL)?"":es[2]
		    ,eventhandler_command_id
		    ,(es[3]==NULL)?"":es[3]
		    ,check_timeperiod_id
		    ,notification_timeperiod_id
		    ,(es[1]==NULL)?"":es[1]
		    ,check_interval
		    ,retry_interval
		    ,max_check_attempts
		    ,first_notification_delay
		    ,notification_interval
		    ,notify_on_down
		    ,notify_on_unreachable
		    ,notify_on_recovery
		    ,notify_on_flapping
		    ,notify_on_downtime
		    ,stalk_on_up
		    ,stalk_on_down
		    ,stalk_on_unreachable
		    ,flap_detection_enabled
		    ,flap_detection_on_up
		    ,flap_detection_on_down
		    ,flap_detection_on_unreachable
		    ,low_flap_threshold
		    ,high_flap_threshold
		    ,process_performance_data
		    ,freshness_checks_enabled
		    ,freshness_threshold
		    ,passive_checks_enabled
		    ,event_handler_enabled
		    ,active_checks_enabled
		    ,retain_status_information
		    ,retain_nonstatus_information
		    ,notifications_enabled
		    ,obsess_over_host
		    ,failure_prediction_enabled
		    ,(es[4]==NULL)?"":es[4]
		    ,(es[5]==NULL)?"":es[5]
		    ,(es[6]==NULL)?"":es[6]
		    ,(es[7]==NULL)?"":es[7]
		    ,(es[8]==NULL)?"":es[8]
		    ,(es[9]==NULL)?"":es[9]
		    ,(es[10]==NULL)?"":es[10]
		    ,have_2d_coords
		    ,x_2d
		    ,y_2d
		    ,have_3d_coords
		    ,x_3d
		    ,y_3d
		    ,z_3d
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			host_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	for(x=0;x<13;x++)
		free(es[x]);

	/* save parent hosts to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_PARENTHOST];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', host_id='%lu', parent_host_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,host_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTPARENTHOSTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save contact groups to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', host_id='%lu', contactgroup_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,host_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTGROUPS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save contacts to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACT];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', host_id='%lu', contact_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,host_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCONTACTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu', config_type='%d', has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,object_id
			    ,idi->current_object_config_type
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_hostgroupdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long group_id=0L;
	unsigned long member_id=0L;
	int result=NDO_OK;
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_HOSTGROUPALIAS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOSTGROUP,idi->buffered_input[NDO_DATA_HOSTGROUPNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_HOSTGROUP,object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', hostgroup_object_id='%lu', alias='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			group_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	free(es[0]);

	/* save hostgroup members to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_HOSTGROUPMEMBER];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', hostgroup_id='%lu', host_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,group_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTGROUPMEMBERS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_servicedefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long host_id=0L;
	unsigned long check_timeperiod_id=0L;
	unsigned long notification_timeperiod_id=0L;
	unsigned long check_command_id=0L;
	unsigned long eventhandler_command_id=0L;
	double check_interval=0.0;
	double retry_interval=0.0;
	int max_check_attempts=0;
	double first_notification_delay=0.0;
	double notification_interval=0.0;
	int notify_on_warning=0;
	int notify_on_unknown=0;
	int notify_on_critical=0;
	int notify_on_recovery=0;
	int notify_on_flapping=0;
	int notify_on_downtime=0;
	int stalk_on_ok=0;
	int stalk_on_warning=0;
	int stalk_on_unknown=0;
	int stalk_on_critical=0;
	int is_volatile=0;
	int flap_detection_enabled=0;
	int flap_detection_on_ok=0;
	int flap_detection_on_warning=0;
	int flap_detection_on_unknown=0;
	int flap_detection_on_critical=0;
	int process_performance_data=0;
	int freshness_checks_enabled=0;
	int freshness_threshold=0;
	int passive_checks_enabled=0;
	int event_handler_enabled=0;
	int active_checks_enabled=0;
	int retain_status_information=0;
	int retain_nonstatus_information=0;
	int notifications_enabled=0;
	int obsess_over_service=0;
	int failure_prediction_enabled=0;
	double low_flap_threshold=0.0;
	double high_flap_threshold=0.0;
	unsigned long service_id=0L;
	unsigned long member_id=0L;
	int result=NDO_OK;
	char *es[9];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;
	char *cmdptr=NULL;
	char *argptr=NULL;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICECHECKINTERVAL],&check_interval);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICERETRYINTERVAL],&retry_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXSERVICECHECKATTEMPTS],&max_check_attempts);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATIONDELAY],&first_notification_delay);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONINTERVAL],&notification_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEWARNING],&notify_on_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEUNKNOWN],&notify_on_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICECRITICAL],&notify_on_critical);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICERECOVERY],&notify_on_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEFLAPPING],&notify_on_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEDOWNTIME],&notify_on_downtime);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONOK],&stalk_on_ok);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONWARNING],&stalk_on_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONUNKNOWN],&stalk_on_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STALKSERVICEONCRITICAL],&stalk_on_critical);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEISVOLATILE],&is_volatile);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFLAPDETECTIONENABLED],&flap_detection_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONOK],&flap_detection_on_ok);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONWARNING],&flap_detection_on_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONUNKNOWN],&flap_detection_on_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAPDETECTIONONCRITICAL],&flap_detection_on_critical);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PROCESSSERVICEPERFORMANCEDATA],&process_performance_data);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFRESHNESSCHECKSENABLED],&freshness_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFRESHNESSTHRESHOLD],&freshness_threshold);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PASSIVESERVICECHECKSENABLED],&passive_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEEVENTHANDLERENABLED],&event_handler_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ACTIVESERVICECHECKSENABLED],&active_checks_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINSERVICESTATUSINFORMATION],&retain_status_information);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETAINSERVICENONSTATUSINFORMATION],&retain_nonstatus_information);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED],&notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_OBSESSOVERSERVICE],&obsess_over_service);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICEFAILUREPREDICTIONENABLED],&failure_prediction_enabled);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LOWSERVICEFLAPTHRESHOLD],&low_flap_threshold);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_HIGHSERVICEFLAPTHRESHOLD],&high_flap_threshold);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS]);

	/* get the check command */
	cmdptr=strtok(idi->buffered_input[NDO_DATA_SERVICECHECKCOMMAND],"!");
	argptr=strtok(NULL,"\x0");
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&check_command_id);
	es[1]=ndo2db_db_escape_string(idi,argptr);
	
	/* get the event handler command */
	cmdptr=strtok(idi->buffered_input[NDO_DATA_SERVICEEVENTHANDLER],"!");
	argptr=strtok(NULL,"\x0");
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&eventhandler_command_id);
	es[2]=ndo2db_db_escape_string(idi,argptr);
	
	es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_NOTES]);
	es[4]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_NOTESURL]);
	es[5]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ACTIONURL]);
	es[6]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ICONIMAGE]);
	es[7]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_ICONIMAGEALT]);
	es[8]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_DISPLAYNAME]);

	/* get the object ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOSTNAME],idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION],&object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOSTNAME],NULL,&host_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_SERVICE,object_id);

	/* get the timeperiod ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_SERVICECHECKPERIOD],NULL,&check_timeperiod_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONPERIOD],NULL,&notification_timeperiod_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', host_object_id='%lu', service_object_id='%lu', display_name='%s', check_command_object_id='%lu', check_command_args='%s', eventhandler_command_object_id='%lu', eventhandler_command_args='%s', check_timeperiod_object_id='%lu', notification_timeperiod_object_id='%lu', failure_prediction_options='%s', check_interval='%lf', retry_interval='%lf', max_check_attempts='%d', first_notification_delay='%lf', notification_interval='%lf', notify_on_warning='%d', notify_on_unknown='%d', notify_on_critical='%d', notify_on_recovery='%d', notify_on_flapping='%d', notify_on_downtime='%d', stalk_on_ok='%d', stalk_on_warning='%d', stalk_on_unknown='%d', stalk_on_critical='%d', is_volatile='%d', flap_detection_enabled='%d', flap_detection_on_ok='%d', flap_detection_on_warning='%d', flap_detection_on_unknown='%d', flap_detection_on_critical='%d', low_flap_threshold='%lf', high_flap_threshold='%lf', process_performance_data='%d', freshness_checks_enabled='%d', freshness_threshold='%d', passive_checks_enabled='%d', event_handler_enabled='%d', active_checks_enabled='%d', retain_status_information='%d', retain_nonstatus_information='%d', notifications_enabled='%d', obsess_over_service='%d', failure_prediction_enabled='%d', notes='%s', notes_url='%s', action_url='%s', icon_image='%s', icon_image_alt='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,host_id
		    ,object_id
		    ,(es[8]==NULL)?"":es[8]
		    ,check_command_id
		    ,(es[1]==NULL)?"":es[1]
		    ,eventhandler_command_id
		    ,(es[2]==NULL)?"":es[2]
		    ,check_timeperiod_id
		    ,notification_timeperiod_id
		    ,(es[0]==NULL)?"":es[0]
		    ,check_interval
		    ,retry_interval
		    ,max_check_attempts
		    ,first_notification_delay
		    ,notification_interval
		    ,notify_on_warning
		    ,notify_on_unknown
		    ,notify_on_critical
		    ,notify_on_recovery
		    ,notify_on_flapping
		    ,notify_on_downtime
		    ,stalk_on_ok
		    ,stalk_on_warning
		    ,stalk_on_unknown
		    ,stalk_on_critical
		    ,is_volatile
		    ,flap_detection_enabled
		    ,flap_detection_on_ok
		    ,flap_detection_on_warning
		    ,flap_detection_on_unknown
		    ,flap_detection_on_critical
		    ,low_flap_threshold
		    ,high_flap_threshold
		    ,process_performance_data
		    ,freshness_checks_enabled
		    ,freshness_threshold
		    ,passive_checks_enabled
		    ,event_handler_enabled
		    ,active_checks_enabled
		    ,retain_status_information
		    ,retain_nonstatus_information
		    ,notifications_enabled
		    ,obsess_over_service
		    ,failure_prediction_enabled
		    ,(es[3]==NULL)?"":es[3]
		    ,(es[4]==NULL)?"":es[4]
		    ,(es[5]==NULL)?"":es[5]
		    ,(es[6]==NULL)?"":es[6]
		    ,(es[7]==NULL)?"":es[7]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICES]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			service_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	for(x=0;x<9;x++)
		free(es[x]);

	/* save contact groups to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', service_id='%lu', contactgroup_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,service_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTGROUPS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save contacts to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACT];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', service_id='%lu', contact_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,service_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECONTACTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu', config_type='%d', has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,object_id
			    ,idi->current_object_config_type
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_servicegroupdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long group_id=0L;
	unsigned long member_id=0L;
	int result=NDO_OK;
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;
	char *hptr=NULL;
	char *sptr=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_SERVICEGROUPALIAS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICEGROUP,idi->buffered_input[NDO_DATA_SERVICEGROUPNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_SERVICEGROUP,object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', servicegroup_object_id='%lu', alias='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			group_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	free(es[0]);

	/* save members to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_SERVICEGROUPMEMBER];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* split the host/service name */
		hptr=strtok(mbuf.buffer[x],";");
		sptr=strtok(NULL,"\x0");

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,hptr,sptr,&member_id);

		if(asprintf(&buf,"instance_id='%d', servicegroup_id='%lu', service_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,group_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEGROUPMEMBERS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_hostdependencydefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long dependent_object_id=0L;
	unsigned long timeperiod_object_id=0L;
	int dependency_type=0;
	int inherits_parent=0;
	int fail_on_up=0;
	int fail_on_down=0;
	int fail_on_unreachable=0;
	int result=NDO_OK;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DEPENDENCYTYPE],&dependency_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_INHERITSPARENT],&inherits_parent);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUP],&fail_on_up);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONDOWN],&fail_on_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUNREACHABLE],&fail_on_unreachable);

	/* get the object ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOSTNAME],NULL,&object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_DEPENDENTHOSTNAME],NULL,&dependent_object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_DEPENDENCYPERIOD],NULL,&timeperiod_object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', host_object_id='%lu', dependent_host_object_id='%lu', dependency_type='%d', inherits_parent='%d', timeperiod_object_id='%lu', fail_on_up='%d', fail_on_down='%d', fail_on_unreachable='%d'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,dependent_object_id
		    ,dependency_type
		    ,inherits_parent
		    ,timeperiod_object_id
		    ,fail_on_up
		    ,fail_on_down
		    ,fail_on_unreachable
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTDEPENDENCIES]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	return NDO_OK;
        }


int ndo2db_handle_servicedependencydefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long dependent_object_id=0L;
	unsigned long timeperiod_object_id=0L;
	int dependency_type=0;
	int inherits_parent=0;
	int fail_on_ok=0;
	int fail_on_warning=0;
	int fail_on_unknown=0;
	int fail_on_critical=0;
	int result=NDO_OK;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DEPENDENCYTYPE],&dependency_type);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_INHERITSPARENT],&inherits_parent);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONOK],&fail_on_ok);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONWARNING],&fail_on_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONUNKNOWN],&fail_on_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FAILONCRITICAL],&fail_on_critical);

	/* get the object ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOSTNAME],idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION],&object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_DEPENDENTHOSTNAME],idi->buffered_input[NDO_DATA_DEPENDENTSERVICEDESCRIPTION],&dependent_object_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_DEPENDENCYPERIOD],NULL,&timeperiod_object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', service_object_id='%lu', dependent_service_object_id='%lu', dependency_type='%d', inherits_parent='%d', timeperiod_object_id='%lu', fail_on_ok='%d', fail_on_warning='%d', fail_on_unknown='%d', fail_on_critical='%d'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,dependent_object_id
		    ,dependency_type
		    ,inherits_parent
		    ,timeperiod_object_id
		    ,fail_on_ok
		    ,fail_on_warning
		    ,fail_on_unknown
		    ,fail_on_critical
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEDEPENDENCIES]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	return NDO_OK;
        }


int ndo2db_handle_hostescalationdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long timeperiod_id=0L;
	unsigned long escalation_id=0L;
	unsigned long member_id=0L;
	int first_notification=0;
	int last_notification=0;
	double notification_interval=0.0;
	int escalate_recovery=0;
	int escalate_down=0;
	int escalate_unreachable=0;
	int result=NDO_OK;
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATION],&first_notification);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTNOTIFICATION],&last_notification);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NOTIFICATIONINTERVAL],&notification_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONRECOVERY],&escalate_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONDOWN],&escalate_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONUNREACHABLE],&escalate_unreachable);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOSTNAME],NULL,&object_id);

	/* get the timeperiod id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_ESCALATIONPERIOD],NULL,&timeperiod_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', host_object_id='%lu', timeperiod_object_id='%lu', first_notification='%d', last_notification='%d', notification_interval='%lf', escalate_on_recovery='%d', escalate_on_down='%d', escalate_on_unreachable='%d'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,timeperiod_id
		    ,first_notification
		    ,last_notification
		    ,notification_interval
		    ,escalate_recovery
		    ,escalate_down
		    ,escalate_unreachable
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			escalation_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	/* save contact groups to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', hostescalation_id='%lu', contactgroup_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,escalation_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTGROUPS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save contacts to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACT];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', hostescalation_id='%lu', contact_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,escalation_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTESCALATIONCONTACTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_serviceescalationdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long timeperiod_id=0L;
	unsigned long escalation_id=0L;
	unsigned long member_id=0L;
	int first_notification=0;
	int last_notification=0;
	double notification_interval=0.0;
	int escalate_recovery=0;
	int escalate_warning=0;
	int escalate_unknown=0;
	int escalate_critical=0;
	int result=NDO_OK;
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIRSTNOTIFICATION],&first_notification);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_LASTNOTIFICATION],&last_notification);
	result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_NOTIFICATIONINTERVAL],&notification_interval);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONRECOVERY],&escalate_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONWARNING],&escalate_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONUNKNOWN],&escalate_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATEONCRITICAL],&escalate_critical);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOSTNAME],idi->buffered_input[NDO_DATA_SERVICEDESCRIPTION],&object_id);

	/* get the timeperiod id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_ESCALATIONPERIOD],NULL,&timeperiod_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', service_object_id='%lu', timeperiod_object_id='%lu', first_notification='%d', last_notification='%d', notification_interval='%lf', escalate_on_recovery='%d', escalate_on_warning='%d', escalate_on_unknown='%d', escalate_on_critical='%d'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,timeperiod_id
		    ,first_notification
		    ,last_notification
		    ,notification_interval
		    ,escalate_recovery
		    ,escalate_warning
		    ,escalate_unknown
		    ,escalate_critical
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			escalation_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	/* save contact groups to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTGROUP];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', serviceescalation_id='%lu', contactgroup_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,escalation_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTGROUPS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	/* save contacts to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACT];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', serviceescalation_id='%lu', contact_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,escalation_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICEESCALATIONCONTACTS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_commanddefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	int result=NDO_OK;
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_COMMAND,object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', object_id='%lu', config_type='%d', command_line='%s'"
		    ,idi->dbinfo.instance_id
		    ,object_id
		    ,idi->current_object_config_type
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMANDS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	result=ndo2db_db_query(idi,buf1);
	free(buf);
	free(buf1);

	for(x=0;x<1;x++)
		free(es[x]);

	return NDO_OK;
        }


int ndo2db_handle_timeperiodefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long timeperiod_id=0L;
	char *dayptr=NULL;
	char *startptr=NULL;
	char *endptr=NULL;
	int day=0;
	unsigned long start_sec=0L;
	unsigned long end_sec=0L;
	int result=NDO_OK;
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_TIMEPERIODALIAS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_TIMEPERIODNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', timeperiod_object_id='%lu', alias='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			timeperiod_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	free(es[0]);

	/* save timeranges to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_TIMERANGE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get var name/val pair */
		dayptr=strtok(mbuf.buffer[x],":");
		startptr=strtok(NULL,"-");
		endptr=strtok(NULL,"\x0");

		if(startptr==NULL || endptr==NULL)
			continue;

		day=atoi(dayptr);
		start_sec=strtoul(startptr,NULL,0);
		end_sec=strtoul(endptr,NULL,0);

		if(asprintf(&buf,"instance_id='%d', timeperiod_id='%lu', day='%d', start_sec='%lu', end_sec='%lu'"
			    ,idi->dbinfo.instance_id
			    ,timeperiod_id
			    ,day
			    ,start_sec
			    ,end_sec
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEPERIODTIMERANGES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_contactdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long contact_id=0L;
	unsigned long host_timeperiod_id=0L;
	unsigned long service_timeperiod_id=0L;
	int host_notifications_enabled=0;
	int service_notifications_enabled=0;
	int can_submit_commands=0;
	int notify_service_recovery=0;
	int notify_service_warning=0;
	int notify_service_unknown=0;
	int notify_service_critical=0;
	int notify_service_flapping=0;
	int notify_service_downtime=0;
	int notify_host_recovery=0;
	int notify_host_down=0;
	int notify_host_unreachable=0;
	int notify_host_flapping=0;
	int notify_host_downtime=0;
	unsigned long command_id=0L;
	int result=NDO_OK;
	char *es[3];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;
	char *numptr=NULL;
	char *addressptr=NULL;
	int address_number=0;
	char *cmdptr=NULL;
	char *argptr=NULL;
	char *ptr1=NULL;
	char *ptr2=NULL;
	char *ptr3=NULL;
	int has_been_modified=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	/* convert vars */
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONSENABLED],&host_notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONSENABLED],&service_notifications_enabled);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CANSUBMITCOMMANDS],&can_submit_commands);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEWARNING],&notify_service_warning);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEUNKNOWN],&notify_service_unknown);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICECRITICAL],&notify_service_critical);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICERECOVERY],&notify_service_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEFLAPPING],&notify_service_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYSERVICEDOWNTIME],&notify_service_downtime);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWN],&notify_host_down);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTUNREACHABLE],&notify_host_unreachable);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTRECOVERY],&notify_host_recovery);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTFLAPPING],&notify_host_flapping);
	result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFYHOSTDOWNTIME],&notify_host_downtime);

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_CONTACTALIAS]);
	es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_EMAILADDRESS]);
	es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PAGERADDRESS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,idi->buffered_input[NDO_DATA_CONTACTNAME],NULL,&contact_id);

	/* get the timeperiod ids */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_HOSTNOTIFICATIONPERIOD],NULL,&host_timeperiod_id);
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_TIMEPERIOD,idi->buffered_input[NDO_DATA_SERVICENOTIFICATIONPERIOD],NULL,&service_timeperiod_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_CONTACT,contact_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', contact_object_id='%lu', alias='%s', email_address='%s', pager_address='%s', host_timeperiod_object_id='%lu', service_timeperiod_object_id='%lu', host_notifications_enabled='%d', service_notifications_enabled='%d', can_submit_commands='%d', notify_service_recovery='%d', notify_service_warning='%d', notify_service_unknown='%d', notify_service_critical='%d', notify_service_flapping='%d', notify_service_downtime='%d', notify_host_recovery='%d', notify_host_down='%d', notify_host_unreachable='%d', notify_host_flapping='%d', notify_host_downtime='%d'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,contact_id
		    ,es[0]
		    ,es[1]
		    ,es[2]
		    ,host_timeperiod_id
		    ,service_timeperiod_id
		    ,host_notifications_enabled
		    ,service_notifications_enabled
		    ,can_submit_commands
		    ,notify_service_recovery
		    ,notify_service_warning
		    ,notify_service_unknown
		    ,notify_service_critical
		    ,notify_service_flapping
		    ,notify_service_downtime
		    ,notify_host_recovery
		    ,notify_host_down
		    ,notify_host_unreachable
		    ,notify_host_flapping
		    ,notify_host_downtime
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			contact_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	for(x=0;x<3;x++)
		free(es[x]);

	/* save addresses to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		numptr=strtok(mbuf.buffer[x],":");
		addressptr=strtok(NULL,"\x0");

		if(numptr==NULL || addressptr==NULL)
			continue;

		address_number=atoi(numptr);
		es[0]=ndo2db_db_escape_string(idi,addressptr);

		if(asprintf(&buf,"instance_id='%d', contact_id='%lu', address_number='%d', address='%s'"
			    ,idi->dbinfo.instance_id
			    ,contact_id
			    ,address_number
			    ,es[0]
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTADDRESSES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);

		free(es[0]);
	        }

	/* save host notification commands to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		cmdptr=strtok(mbuf.buffer[x],"!");
		argptr=strtok(NULL,"\x0");

		if(numptr==NULL)
			continue;

		/* find the command */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&command_id);

		es[0]=ndo2db_db_escape_string(idi,argptr);

		if(asprintf(&buf,"instance_id='%d', contact_id='%lu', notification_type='%d', command_object_id='%lu', command_args='%s'"
			    ,idi->dbinfo.instance_id
			    ,contact_id
			    ,HOST_NOTIFICATION
			    ,command_id
			    ,(es[0]==NULL)?"":es[0]
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);

		free(es[0]);
	        }

	/* save service notification commands to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTADDRESS];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		cmdptr=strtok(mbuf.buffer[x],"!");
		argptr=strtok(NULL,"\x0");

		if(numptr==NULL)
			continue;

		/* find the command */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,cmdptr,NULL,&command_id);

		es[0]=ndo2db_db_escape_string(idi,argptr);

		if(asprintf(&buf,"instance_id='%d', contact_id='%lu', notification_type='%d', command_object_id='%lu', command_args='%s'"
			    ,idi->dbinfo.instance_id
			    ,contact_id
			    ,SERVICE_NOTIFICATION
			    ,command_id
			    ,(es[0]==NULL)?"":es[0]
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTNOTIFICATIONCOMMANDS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);

		free(es[0]);
	        }

	/* save custom variables to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CUSTOMVARIABLE];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		if((ptr1=strtok(mbuf.buffer[x],":"))==NULL)
			continue;
		es[0]=strdup(ptr1);
		if((ptr2=strtok(NULL,":"))==NULL)
			continue;
		has_been_modified=atoi(ptr2);
		ptr3=strtok(NULL,"\n");
		es[1]=strdup((ptr3==NULL)?"":ptr3);

		if(asprintf(&buf,"instance_id='%d', object_id='%lu', config_type='%d', has_been_modified='%d', varname='%s', varvalue='%s'"
			    ,idi->dbinfo.instance_id
			    ,contact_id
			    ,idi->current_object_config_type
			    ,has_been_modified
			    ,(es[0]==NULL)?"":es[0]
			    ,(es[1]==NULL)?"":es[1]
			   )==-1)
			buf=NULL;

		free(es[0]);
		free(es[1]);
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CUSTOMVARIABLES]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }


int ndo2db_handle_contactgroupdefinition(ndo2db_idi *idi){
	int type,flags,attr;
	struct timeval tstamp;
	unsigned long object_id=0L;
	unsigned long group_id=0L;
	unsigned long member_id=0L;
	int result=NDO_OK;
	char *es[1];
	int x=0;
	char *buf=NULL;
	char *buf1=NULL;
	ndo2db_mbuf mbuf;

	if(idi==NULL)
		return NDO_ERROR;

	/* convert timestamp, etc */
	result=ndo2db_convert_standard_data_elements(idi,&type,&flags,&attr,&tstamp);

	/* don't store old data */
	if(tstamp.tv_sec<idi->dbinfo.latest_realtime_data_time)
		return NDO_OK;

	es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_CONTACTGROUPALIAS]);

	/* get the object id */
	result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,idi->buffered_input[NDO_DATA_CONTACTGROUPNAME],NULL,&object_id);

	/* flag the object as being active */
	ndo2db_set_object_as_active(idi,NDO2DB_OBJECTTYPE_CONTACTGROUP,object_id);

	/* add definition to db */
	if(asprintf(&buf,"instance_id='%lu', config_type='%d', contactgroup_object_id='%lu', alias='%s'"
		    ,idi->dbinfo.instance_id
		    ,idi->current_object_config_type
		    ,object_id
		    ,es[0]
		   )==-1)
		buf=NULL;
	
	if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPS]
		    ,buf
		    ,buf
		   )==-1)
		buf1=NULL;

	if((result=ndo2db_db_query(idi,buf1))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			group_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
	                }
	        }
	free(buf);
	free(buf1);

	free(es[0]);

	/* save contact group members to db */
	mbuf=idi->mbuf[NDO2DB_MBUF_CONTACTGROUPMEMBER];
	for(x=0;x<mbuf.used_lines;x++){

		if(mbuf.buffer[x]==NULL)
			continue;

		/* get the object id of the member */
		result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_CONTACT,mbuf.buffer[x],NULL,&member_id);

		if(asprintf(&buf,"instance_id='%d', contactgroup_id='%lu', contact_object_id='%lu'"
			    ,idi->dbinfo.instance_id
			    ,group_id
			    ,member_id
			   )==-1)
			buf=NULL;
	
		if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
			    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTGROUPMEMBERS]
			    ,buf
			    ,buf
			   )==-1)
			buf1=NULL;

		result=ndo2db_db_query(idi,buf1);
		free(buf);
		free(buf1);
	        }

	return NDO_OK;
        }
