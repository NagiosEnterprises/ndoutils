/***************************************************************
 * DB.C - Datatabase routines for NDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad 
 *
 * Last Modified: 10-18-2007
 *
 **************************************************************/

/* include our project's header files */
#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndo2db.h"
#include "../include/dbhandlers.h"
#include "../include/db.h"

extern int errno;

extern ndo2db_dbconfig ndo2db_db_settings;
extern time_t ndo2db_db_last_checkin_time;

char *ndo2db_db_rawtablenames[NDO2DB_MAX_DBTABLES]={
	"instances",
	"conninfo",
	"objects",
	"objecttypes",
	"logentries",
	"systemcommands",
	"eventhandlers",
	"servicechecks",
	"hostchecks",
	"programstatus",
	"externalcommands",
	"servicestatus",
	"hoststatus",
	"processevents",
	"timedevents",
	"timedeventqueue",
	"flappinghistory",
	"commenthistory",
	"comments",
	"notifications",
	"contactnotifications",
	"contactnotificationmethods",
	"acknowledgements",
	"statehistory",
	"downtimehistory",
	"scheduleddowntime",
	"configfiles",
	"configfilevariables",
	"runtimevariables",
	"contactstatus",
	"customvariablestatus",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"commands",
	"timeperiods",
	"timeperiod_timeranges",
	"contactgroups",
	"contactgroup_members",
	"hostgroups",
	"hostgroup_members",
	"servicegroups",
	"servicegroup_members",
	"hostescalations",
	"hostescalation_contacts",
	"serviceescalations",
	"serviceescalation_contacts",
	"hostdependencies",
	"servicedependencies",
	"contacts",
	"contact_addresses",
	"contact_notificationcommands",
	"hosts",
	"host_parenthosts",
	"host_contacts",
	"services",
	"service_contacts",
	"customvariables",
	"host_contactgroups",
	"service_contactgroups",
	"hostescalation_contactgroups",
	"serviceescalation_contactgroups"
        };


char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];

/*
#define DEBUG_NDO2DB_QUERIES 1
*/

/****************************************************************************/
/* CONNECTION FUNCTIONS                                                     */
/****************************************************************************/

/* initialize database structures */
int ndo2db_db_init(ndo2db_idi *idi){
	register int x;

	if(idi==NULL)
		return NDO_ERROR;

	/* initialize db server type */
	idi->dbinfo.server_type=ndo2db_db_settings.server_type;

	/* initialize table names */
	for(x=0;x<NDO2DB_MAX_DBTABLES;x++){
		if((ndo2db_db_tablenames[x]=(char *)malloc(strlen(ndo2db_db_rawtablenames[x])+((ndo2db_db_settings.dbprefix==NULL)?0:strlen(ndo2db_db_settings.dbprefix))+1))==NULL)
			return NDO_ERROR;
		sprintf(ndo2db_db_tablenames[x],"%s%s",(ndo2db_db_settings.dbprefix==NULL)?"":ndo2db_db_settings.dbprefix,ndo2db_db_rawtablenames[x]);
	        }

	/* initialize other variables */
	idi->dbinfo.connected=NDO_FALSE;
	idi->dbinfo.error=NDO_FALSE;
	idi->dbinfo.instance_id=0L;
	idi->dbinfo.conninfo_id=0L;
	idi->dbinfo.latest_program_status_time=(time_t)0L;
	idi->dbinfo.latest_host_status_time=(time_t)0L;
	idi->dbinfo.latest_service_status_time=(time_t)0L;
	idi->dbinfo.latest_queued_event_time=(time_t)0L;
	idi->dbinfo.latest_realtime_data_time=(time_t)0L;
	idi->dbinfo.latest_comment_time=(time_t)0L;
	idi->dbinfo.clean_event_queue=NDO_FALSE;
	idi->dbinfo.last_notification_id=0L;
	idi->dbinfo.last_contact_notification_id=0L;
	idi->dbinfo.max_timedevents_age=ndo2db_db_settings.max_timedevents_age;
	idi->dbinfo.max_systemcommands_age=ndo2db_db_settings.max_systemcommands_age;
	idi->dbinfo.max_servicechecks_age=ndo2db_db_settings.max_servicechecks_age;
	idi->dbinfo.max_hostchecks_age=ndo2db_db_settings.max_hostchecks_age;
	idi->dbinfo.max_eventhandlers_age=ndo2db_db_settings.max_eventhandlers_age;
	idi->dbinfo.last_table_trim_time=(time_t)0L;
	idi->dbinfo.last_logentry_time=(time_t)0L;
	idi->dbinfo.last_logentry_data=NULL;
	idi->dbinfo.object_hashlist=NULL;

	/* initialize db structures, etc. */
	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		if(!mysql_init(&idi->dbinfo.mysql_conn)){
			syslog(LOG_USER|LOG_INFO,"Error: mysql_init() failed\n");
			return NDO_ERROR;
	                }
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		idi->dbinfo.pgsql_conn=NULL;
		idi->dbinfo.pgsql_result=NULL;
#endif
		break;
	default:
		break;
	        }

	return NDO_OK;
        }


/* clean up database structures */
int ndo2db_db_deinit(ndo2db_idi *idi){
	register int x;

	if(idi==NULL)
		return NDO_ERROR;

	/* free table names */
	for(x=0;x<NDO2DB_MAX_DBTABLES;x++){
		if(ndo2db_db_tablenames[x])
			free(ndo2db_db_tablenames[x]);
		ndo2db_db_tablenames[x]=NULL;
	        }

	/* free cached object ids */
	ndo2db_free_cached_object_ids(idi);

	return NDO_OK;
        }


/* connects to the database server */
int ndo2db_db_connect(ndo2db_idi *idi){
	char connect_string[NDO2DB_INPUT_BUFFER];
	int result=NDO_OK;

	if(idi==NULL)
		return NDO_ERROR;

	/* we're already connected... */
	if(idi->dbinfo.connected==NDO_TRUE)
		return NDO_OK;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		if(!mysql_real_connect(&idi->dbinfo.mysql_conn,ndo2db_db_settings.host,ndo2db_db_settings.username,ndo2db_db_settings.password,ndo2db_db_settings.dbname,ndo2db_db_settings.port,NULL,0)){

			mysql_close(&idi->dbinfo.mysql_conn);
			syslog(LOG_USER|LOG_INFO,"Error: Could not connect to MySQL database: %s",mysql_error(&idi->dbinfo.mysql_conn));
			result=NDO_ERROR;
			idi->disconnect_client=NDO_TRUE;
		        }
		else{
			idi->dbinfo.connected=NDO_TRUE;
			syslog(LOG_USER|LOG_INFO,"Successfully connected to MySQL database");
		        }
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		snprintf(connect_string,sizeof(connect_string)-1,"host='%s' port=%d dbname='%s' user='%s' password='%s'",ndo2db_db_settings.host,ndo2db_db_settings.port,ndo2db_db_settings.dbname,ndo2db_db_settings.username,ndo2db_db_settings.password);
		connect_string[sizeof(connect_string)-1]='\x0';
		idi->dbinfo.pgsql_conn=PQconnectdb(connect_string);

		if(PQstatus(idi->dbinfo.pgsql_conn)==CONNECTION_BAD){
			PQfinish(idi->dbinfo.pgsql_conn);
			syslog(LOG_USER|LOG_INFO,"Error: Could not connect to PostgreSQL database: %s",PQerrorMessage(idi->dbinfo.pgsql_conn));
			result=NDO_ERROR;
			idi->disconnect_client=NDO_TRUE;
		        }
		else{
			idi->dbinfo.connected=NDO_TRUE;
			syslog(LOG_USER|LOG_INFO,"Successfully connect to PostgreSQL database");
		        }
#endif
		break;
	default:
		break;
	        }

	return result;
        }


/* disconnects from the database server */
int ndo2db_db_disconnect(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	/* we're not connected... */
	if(idi->dbinfo.connected==NDO_FALSE)
		return NDO_OK;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		/* close the connection to the database server */		
		mysql_close(&idi->dbinfo.mysql_conn);
		syslog(LOG_USER|LOG_INFO,"Successfully disconnected from MySQL database");
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		/* close database connection and cleanup */
		if(PQstatus(idi->dbinfo.pgsql_conn)!=CONNECTION_BAD)
			PQfinish(idi->dbinfo.pgsql_conn);
		syslog(LOG_USER|LOG_INFO,"Successfully disconnected from PostgreSQL database");
#endif
		break;
	default:
		break;
	        }

	return NDO_OK;
        }


/* post-connect routines */
int ndo2db_db_hello(ndo2db_idi *idi){
	char *buf=NULL;
	char *ts=NULL;
	int result=NDO_OK;
	int have_instance=NDO_FALSE;
	time_t current_time;

	/* make sure we have an instance name */
	if(idi->instance_name==NULL)
		idi->instance_name=strdup("default");

	/* get existing instance */
	if(asprintf(&buf,"SELECT instance_id FROM %s WHERE instance_name='%s'",ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES],idi->instance_name)==-1)
		buf=NULL;
	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.mysql_result=mysql_store_result(&idi->dbinfo.mysql_conn);
			if((idi->dbinfo.mysql_row=mysql_fetch_row(idi->dbinfo.mysql_result))!=NULL){
				ndo2db_convert_string_to_unsignedlong(idi->dbinfo.mysql_row[0],&idi->dbinfo.instance_id);
				have_instance=NDO_TRUE;
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

	/* insert new instance if necessary */
	if(have_instance==NDO_FALSE){
		if(asprintf(&buf,"INSERT INTO %s SET instance_name='%s'",ndo2db_db_tablenames[NDO2DB_DBTABLE_INSTANCES],idi->instance_name)==-1)
			buf=NULL;
		if((result=ndo2db_db_query(idi,buf))==NDO_OK){
			switch(idi->dbinfo.server_type){
			case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
				idi->dbinfo.instance_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
				break;
			default:
				break;
			        }
	                }
		free(buf);
	        }
	
	ts=ndo2db_db_timet_to_sql(idi,idi->data_start_time);

	/* record initial connection information */
	if(asprintf(&buf,"INSERT INTO %s SET instance_id='%lu', connect_time=NOW(), last_checkin_time=NOW(), bytes_processed='0', lines_processed='0', entries_processed='0', agent_name='%s', agent_version='%s', disposition='%s', connect_source='%s', connect_type='%s', data_start_time=%s"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO]
		    ,idi->dbinfo.instance_id
		    ,idi->agent_name
		    ,idi->agent_version
		    ,idi->disposition
		    ,idi->connect_source
		    ,idi->connect_type
		    ,ts
		   )==-1)
		buf=NULL;
	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.conninfo_id=mysql_insert_id(&idi->dbinfo.mysql_conn);
#endif
			break;
		default:
			break;
		        }
	        }
	free(buf);
	free(ts);

	/* get cached object ids... */
	ndo2db_get_cached_object_ids(idi);

	/* get latest times from various tables... */
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_PROGRAMSTATUS],"status_update_time",(unsigned long *)&idi->dbinfo.latest_program_status_time);
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTSTATUS],"status_update_time",(unsigned long *)&idi->dbinfo.latest_host_status_time);
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICESTATUS],"status_update_time",(unsigned long *)&idi->dbinfo.latest_service_status_time);
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONTACTSTATUS],"status_update_time",(unsigned long *)&idi->dbinfo.latest_contact_status_time);
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTQUEUE],"queued_time",(unsigned long *)&idi->dbinfo.latest_queued_event_time);
	ndo2db_db_get_latest_data_time(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTS],"entry_time",(unsigned long *)&idi->dbinfo.latest_comment_time);

	/* calculate time of latest realtime data */
	idi->dbinfo.latest_realtime_data_time=(time_t)0L;
	if(idi->dbinfo.latest_program_status_time>idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time=idi->dbinfo.latest_program_status_time;
	if(idi->dbinfo.latest_host_status_time>idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time=idi->dbinfo.latest_host_status_time;
	if(idi->dbinfo.latest_service_status_time>idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time=idi->dbinfo.latest_service_status_time;
	if(idi->dbinfo.latest_contact_status_time>idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time=idi->dbinfo.latest_contact_status_time;
	if(idi->dbinfo.latest_queued_event_time>idi->dbinfo.latest_realtime_data_time)
		idi->dbinfo.latest_realtime_data_time=idi->dbinfo.latest_queued_event_time;

	/* get current time */
	/* make sure latest time stamp isn't in the future - this will cause problems if a backwards system time change occurs */
	time(&current_time);
	if(idi->dbinfo.latest_realtime_data_time>current_time)
		idi->dbinfo.latest_realtime_data_time=current_time;

	/* set flags to clean event queue, etc. */
	idi->dbinfo.clean_event_queue=NDO_TRUE;

	/* set misc data */
	idi->dbinfo.last_notification_id=0L;
	idi->dbinfo.last_contact_notification_id=0L;

	return result;
        }


/* pre-disconnect routines */
int ndo2db_db_goodbye(ndo2db_idi *idi){
	int result=NDO_OK;
	char *buf=NULL;
	char *ts=NULL;

	ts=ndo2db_db_timet_to_sql(idi,idi->data_end_time);

	/* record last connection information */
	if(asprintf(&buf,"UPDATE %s SET disconnect_time=NOW(), last_checkin_time=NOW(), data_end_time=%s, bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE conninfo_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO]
		    ,ts
		    ,idi->bytes_processed
		    ,idi->lines_processed
		    ,idi->entries_processed
		    ,idi->dbinfo.conninfo_id
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	free(ts);

	return result;
        }


/* checking routines */
int ndo2db_db_checkin(ndo2db_idi *idi){
	int result=NDO_OK;
	char *buf=NULL;

	/* record last connection information */
	if(asprintf(&buf,"UPDATE %s SET last_checkin_time=NOW(), bytes_processed='%lu', lines_processed='%lu', entries_processed='%lu' WHERE conninfo_id='%lu'"
		    ,ndo2db_db_tablenames[NDO2DB_DBTABLE_CONNINFO]
		    ,idi->bytes_processed
		    ,idi->lines_processed
		    ,idi->entries_processed
		    ,idi->dbinfo.conninfo_id
		   )==-1)
		buf=NULL;
	result=ndo2db_db_query(idi,buf);
	free(buf);

	time(&ndo2db_db_last_checkin_time);

	return result;
        }



/****************************************************************************/
/* MISC FUNCTIONS                                                           */
/****************************************************************************/

/* escape a string for a SQL statement */
char *ndo2db_db_escape_string(ndo2db_idi *idi, char *buf){
	register int x,y,z;
	char *newbuf=NULL;

	if(idi==NULL || buf==NULL)
		return NULL;

	z=strlen(buf);

	/* allocate space for the new string */
	if((newbuf=(char *)malloc((z*2)+1))==NULL)
		return NULL;

	/* escape characters */
	for(x=0,y=0;x<z;x++){

		if(idi->dbinfo.server_type==NDO2DB_DBSERVER_MYSQL){
			if(buf[x]=='\'' || buf[x]=='\"' || buf[x]=='*' || buf[x]=='\\' || buf[x]=='$' || buf[x]=='?' || buf[x]=='.' || buf[x]=='^' || buf[x]=='+' || buf[x]=='[' || buf[x]==']' || buf[x]=='(' || buf[x]==')')
				newbuf[y++]='\\';
		        }
		else if(idi->dbinfo.server_type==NDO2DB_DBSERVER_PGSQL){
			if(! (isspace(buf[x]) || isalnum(buf[x]) || (buf[x]=='_')) )
				newbuf[y++]='\\';
		        }

		newbuf[y++]=buf[x];
	        }

	/* terminate escape string */
	newbuf [y]='\0';

	return newbuf;
        }


/* SQL query conversion of time_t format to date/time format */
char *ndo2db_db_timet_to_sql(ndo2db_idi *idi, time_t t){
	char *buf=NULL;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		asprintf(&buf,"FROM_UNIXTIME(%lu)",(unsigned long)t);
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		asprintf(&buf,"FROM_UNIXTIME(%lu)",(unsigned long)t);
#endif
		break;
	default:
		break;
	        }
	
	return buf;
        }


/* SQL query conversion of date/time format to time_t format */
char *ndo2db_db_sql_to_timet(ndo2db_idi *idi, char *field){
	char *buf=NULL;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		asprintf(&buf,"UNIX_TIMESTAMP(%s)",(field==NULL)?"":field);
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		asprintf(&buf,"UNIX_TIMESTAMP(%s",(field==NULL)?"":field);
#endif
		break;
	default:
		break;
	        }
	
	return buf;
        }


/* executes a SQL statement */
int ndo2db_db_query(ndo2db_idi *idi, char *buf){
	int result=NDO_OK;
	int query_result=0;

	if(idi==NULL || buf==NULL)
		return NDO_ERROR;

	/* if we're not connected, try and reconnect... */
	if(idi->dbinfo.connected==NDO_FALSE){
		if(ndo2db_db_connect(idi)==NDO_ERROR)
			return NDO_ERROR;
		ndo2db_db_hello(idi);
	        }

#ifdef DEBUG_NDO2DB_QUERIES
	printf("%s\n\n",buf);
#endif

	ndo2db_log_debug_info(NDO2DB_DEBUGL_SQL,0,"%s\n",buf);

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		if((query_result=mysql_query(&idi->dbinfo.mysql_conn,buf))){
			syslog(LOG_USER|LOG_INFO,"Error: mysql_query() failed for '%s'\n",buf);
			result=NDO_ERROR;
		        }
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		idi->dbinfo.pgsql_result==PQexec(idi->dbinfo.pgsql_conn,buf);
		if((query_result=PQresultStatus(idi->dbinfo.pgsql_result))!=PGRES_COMMAND_OK){
			syslog(LOG_USER|LOG_INFO,"Error: PQexec() failed for '%s'\n",buf);
			PQclear(idi->dbinfo.pgsql_result);
			result=NDO_ERROR;
	                }
#endif
		break;
	default:
		break;
	        }

	/* handle errors */
	if(result==NDO_ERROR)
		ndo2db_handle_db_error(idi,query_result);

	return result;
        }


/* frees memory associated with a query */
int ndo2db_db_free_query(ndo2db_idi *idi){

	if(idi==NULL)
		return NDO_ERROR;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		PQclear(idi->dbinfo.pgsql_result);
#endif
		break;
	default:
		break;
	        }

	return NDO_OK;
        }


/* handles SQL query errors */
int ndo2db_handle_db_error(ndo2db_idi *idi, int query_result){
	int result=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* we're not currently connected... */
	if(idi->dbinfo.connected==NDO_FALSE)
		return NDO_OK;

	switch(idi->dbinfo.server_type){
	case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
		result=mysql_errno(&idi->dbinfo.mysql_conn);
		if(result==CR_SERVER_LOST || result==CR_SERVER_GONE_ERROR){
			syslog(LOG_USER|LOG_INFO,"Error: Connection to MySQL database has been lost!\n");
			ndo2db_db_disconnect(idi);
			idi->disconnect_client=NDO_TRUE;
		        }
#endif
		break;
	case NDO2DB_DBSERVER_PGSQL:
#ifdef USE_PGSQL
		result=PQstatus(idi->dbinfo.pgsql_conn);
		if(result!=CONNECTION_OK){
			syslog(LOG_USER|LOG_INFO,"Error: Connection to PostgreSQL database has been lost!\n");
			ndo2db_db_disconnect(idi);
			idi->disconnect_client=NDO_TRUE;
	                }
#endif
		break;
	default:
		break;
	        }

	return NDO_OK;
        }


/* clears data from a given table (current instance only) */
int ndo2db_db_clear_table(ndo2db_idi *idi, char *table_name){
	char *buf=NULL;
	int result=NDO_OK;

	if(idi==NULL || table_name==NULL)
		return NDO_ERROR;

	if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu'"
		    ,table_name
		    ,idi->dbinfo.instance_id
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);

	return result;
        }
		

/* gets latest data time value from a given table */
int ndo2db_db_get_latest_data_time(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long *t){
	char *buf=NULL;
	char *ts[1];
	int result=NDO_OK;

	if(idi==NULL || table_name==NULL || field_name==NULL || t==NULL)
		return NDO_ERROR;

	*t=(time_t)0L;
	ts[0]=ndo2db_db_sql_to_timet(idi,field_name);

	if(asprintf(&buf,"SELECT %s AS latest_time FROM %s WHERE instance_id='%lu' ORDER BY %s DESC LIMIT 0,1"
		    ,ts[0]
		    ,table_name
		    ,idi->dbinfo.instance_id
		    ,field_name
		   )==-1)
		buf=NULL;

	if((result=ndo2db_db_query(idi,buf))==NDO_OK){
		switch(idi->dbinfo.server_type){
		case NDO2DB_DBSERVER_MYSQL:
#ifdef USE_MYSQL
			idi->dbinfo.mysql_result=mysql_store_result(&idi->dbinfo.mysql_conn);
			if((idi->dbinfo.mysql_row=mysql_fetch_row(idi->dbinfo.mysql_result))!=NULL){
				ndo2db_convert_string_to_unsignedlong(idi->dbinfo.mysql_row[0],t);
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
	free(ts[0]);

	return result;
        }
		

/* trim/delete old data from a given table */
int ndo2db_db_trim_data_table(ndo2db_idi *idi, char *table_name, char *field_name, unsigned long t){
	char *buf=NULL;
	char *ts[1];
	int result=NDO_OK;

	if(idi==NULL || table_name==NULL || field_name==NULL)
		return NDO_ERROR;

	ts[0]=ndo2db_db_timet_to_sql(idi,(time_t)t);

	if(asprintf(&buf,"DELETE FROM %s WHERE instance_id='%lu' AND %s<%s"
		    ,table_name
		    ,idi->dbinfo.instance_id
		    ,field_name
		    ,ts[0]
		   )==-1)
		buf=NULL;

	result=ndo2db_db_query(idi,buf);
	free(buf);
	free(ts[0]);

	return result;
        }
		

/* performs some periodic table maintenance... */
int ndo2db_db_perform_maintenance(ndo2db_idi *idi){
	time_t current_time;

	/* get the current time */
	time(&current_time);

	/* trim tables */
	if(((unsigned long)current_time-60)>(unsigned long)idi->dbinfo.last_table_trim_time){
		if(idi->dbinfo.max_timedevents_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_TIMEDEVENTS],"scheduled_time",(time_t)((unsigned long)current_time-idi->dbinfo.max_timedevents_age));
		if(idi->dbinfo.max_systemcommands_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SYSTEMCOMMANDS],"start_time",(time_t)((unsigned long)current_time-idi->dbinfo.max_systemcommands_age));
		if(idi->dbinfo.max_servicechecks_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS],"start_time",(time_t)((unsigned long)current_time-idi->dbinfo.max_servicechecks_age));
		if(idi->dbinfo.max_hostchecks_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS],"start_time",(time_t)((unsigned long)current_time-idi->dbinfo.max_hostchecks_age));
		if(idi->dbinfo.max_eventhandlers_age>0L)
			ndo2db_db_trim_data_table(idi,ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS],"start_time",(time_t)((unsigned long)current_time-idi->dbinfo.max_eventhandlers_age));
		idi->dbinfo.last_table_trim_time=current_time;
	        }

	return NDO_OK;
        }
