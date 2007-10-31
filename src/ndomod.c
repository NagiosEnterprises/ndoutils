/*****************************************************************************
 *
 * NDOMOD.C - Nagios Data Output Event Broker Module
 *
 * Copyright (c) 2005-2007 Ethan Galstad
 *
 * First Written: 05-19-2005
 * Last Modified: 10-31-2007
 *
 *****************************************************************************/

/* include our project's header files */
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndomod.h"

/* include (minimum required) event broker header files */
#ifdef BUILD_NAGIOS_2X
#include "../include/nagios-2x/nebstructs.h"
#include "../include/nagios-2x/nebmodules.h"
#include "../include/nagios-2x/nebcallbacks.h"
#include "../include/nagios-2x/broker.h"
#endif
#ifdef BUILD_NAGIOS_3X
#include "../include/nagios-3x/nebstructs.h"
#include "../include/nagios-3x/nebmodules.h"
#include "../include/nagios-3x/nebcallbacks.h"
#include "../include/nagios-3x/broker.h"
#endif

/* include other Nagios header files for access to functions, data structs, etc. */
#ifdef BUILD_NAGIOS_2X
#include "../include/nagios-2x/common.h"
#include "../include/nagios-2x/nagios.h"
#include "../include/nagios-2x/downtime.h"
#include "../include/nagios-2x/comments.h"
#endif
#ifdef BUILD_NAGIOS_3X
#include "../include/nagios-3x/common.h"
#include "../include/nagios-3x/nagios.h"
#include "../include/nagios-3x/downtime.h"
#include "../include/nagios-3x/comments.h"
#include "../include/nagios-3x/macros.h"
#endif

/* specify event broker API version (required) */
NEB_API_VERSION(CURRENT_NEB_API_VERSION)


#define NDOMOD_VERSION "1.4b7"
#define NDOMOD_NAME "NDOMOD"
#define NDOMOD_DATE "10-31-2007"


void *ndomod_module_handle=NULL;
char *ndomod_instance_name=NULL;
char *ndomod_buffer_file=NULL;
char *ndomod_sink_name=NULL;
int ndomod_sink_type=NDO_SINK_UNIXSOCKET;
int ndomod_sink_tcp_port=NDO_DEFAULT_TCP_PORT;
int ndomod_sink_is_open=NDO_FALSE;
int ndomod_sink_previously_open=NDO_FALSE;
int ndomod_sink_fd=-1;
time_t ndomod_sink_last_reconnect_attempt=0L;
time_t ndomod_sink_last_reconnect_warning=0L;
unsigned long ndomod_sink_connect_attempt=0L;
unsigned long ndomod_sink_reconnect_interval=15;
unsigned long ndomod_sink_reconnect_warning_interval=900;
unsigned long ndomod_sink_rotation_interval=3600;
char *ndomod_sink_rotation_command=NULL;
int ndomod_sink_rotation_timeout=60;
int ndomod_allow_sink_activity=NDO_TRUE;
unsigned long ndomod_process_options=NDOMOD_PROCESS_EVERYTHING;
int ndomod_config_output_options=NDOMOD_CONFIG_DUMP_ALL;
unsigned long ndomod_sink_buffer_slots=5000;
ndomod_sink_buffer sinkbuf;

extern int errno;

/**** NAGIOS VARIABLES ****/
extern command *command_list;
extern timeperiod *timeperiod_list;
extern contact *contact_list;
extern contactgroup *contactgroup_list;
extern host *host_list;
extern hostgroup *hostgroup_list;
extern service *service_list;
extern servicegroup *servicegroup_list;
extern hostescalation *hostescalation_list;
extern serviceescalation *serviceescalation_list;
extern hostdependency *hostdependency_list;
extern servicedependency *servicedependency_list;
#ifdef BUILD_NAGIOS_2X
extern hostextinfo *hostextinfo_list;
extern serviceextinfo *serviceextinfo_list;
#endif

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __nagios_object_structure_version;



#define DEBUG_NDO 1



/* this function gets called when the module is loaded by the event broker */
int nebmodule_init(int flags, char *args, void *handle){
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	/* save our handle */
	ndomod_module_handle=handle;

	/* log module info to the Nagios log file */
	snprintf(temp_buffer,sizeof(temp_buffer)-1,"ndomod: %s %s (%s) Copyright (c) 2005-2007 Ethan Galstad (nagios@nagios.org)",NDOMOD_NAME,NDOMOD_VERSION,NDOMOD_DATE);
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
	ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);

	/* check Nagios object structure version */
	if(ndomod_check_nagios_object_version()==NDO_ERROR)
		return -1;

	/* process arguments */
	if(ndomod_process_module_args(args)==NDO_ERROR)
		return -1;

	/* do some initialization stuff... */
	if(ndomod_init()==NDO_ERROR)
		return -1;

	return 0;
        }


/* this function gets called when the module is unloaded by the event broker */
int nebmodule_deinit(int flags, int reason){
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	/* do some shutdown stuff... */
	ndomod_deinit();
	
	/* log a message to the Nagios log file */
	snprintf(temp_buffer,sizeof(temp_buffer)-1,"ndomod: Shutdown complete.\n");
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
        ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);

	return 0;
        }



/****************************************************************************/
/* INIT/DEINIT FUNCTIONS                                                    */
/****************************************************************************/

/* checks to make sure Nagios object version matches what we know about */
int ndomod_check_nagios_object_version(void){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	
	if(__nagios_object_structure_version!=CURRENT_OBJECT_STRUCTURE_VERSION){

		snprintf(temp_buffer,sizeof(temp_buffer)-1,"ndomod: I've been compiled with support for revision %d of the internal Nagios object structures, but the Nagios daemon is currently using revision %d.  I'm going to unload so I don't cause any problems...\n",CURRENT_OBJECT_STRUCTURE_VERSION,__nagios_object_structure_version);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);

		return NDO_ERROR;
	        }

	return NDO_OK;
        }


/* performs some initialization stuff */
int ndomod_init(void){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	time_t current_time;

	/* initialize some vars (needed for restarts of daemon - why, if the module gets reloaded ???) */
	ndomod_sink_is_open=NDO_FALSE;
	ndomod_sink_previously_open=NDO_FALSE;
	ndomod_sink_fd=-1;
	ndomod_sink_last_reconnect_attempt=0L;
	ndomod_sink_last_reconnect_warning=0L;
	ndomod_allow_sink_activity=NDO_TRUE;

	/* initialize data sink buffer */
	ndomod_sink_buffer_init(&sinkbuf,ndomod_sink_buffer_slots);

	/* read unprocessed data from buffer file */
	ndomod_load_unprocessed_data(ndomod_buffer_file);

	/* open data sink and say hello */
	/* 05/04/06 - modified to flush buffer items that may have been read in from file */
	ndomod_write_to_sink("\n",NDO_FALSE,NDO_TRUE);

	/* register callbacks */
	if(ndomod_register_callbacks()==NDO_ERROR)
		return NDO_ERROR;

	if(ndomod_sink_type==NDO_SINK_FILE){

		/* make sure we have a rotation command defined... */
		if(ndomod_sink_rotation_command==NULL){

			/* log an error message to the Nagios log file */
			snprintf(temp_buffer,sizeof(temp_buffer)-1,"ndomod: Warning - No file rotation command defined.\n");
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
		        }

		/* schedule a file rotation event */
		else{
			time(&current_time);
#ifdef BUILD_NAGIOS_2X
			schedule_new_event(EVENT_USER_FUNCTION,TRUE,current_time+ndomod_sink_rotation_interval,TRUE,ndomod_sink_rotation_interval,NULL,TRUE,(void *)ndomod_rotate_sink_file,NULL);
#else
			schedule_new_event(EVENT_USER_FUNCTION,TRUE,current_time+ndomod_sink_rotation_interval,TRUE,ndomod_sink_rotation_interval,NULL,TRUE,(void *)ndomod_rotate_sink_file,NULL,0);
#endif
		        }

	        }

	return NDO_OK;
        }


/* performs some shutdown stuff */
int ndomod_deinit(void){

	/* deregister callbacks */
	ndomod_deregister_callbacks();

	/* save unprocessed data to buffer file */
	ndomod_save_unprocessed_data(ndomod_buffer_file);

	/* clear sink buffer */
	ndomod_sink_buffer_deinit(&sinkbuf);

	/* close data sink */
	ndomod_goodbye_sink();
	ndomod_close_sink();

	return NDO_OK;
        }



/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* process arguments that were passed to the module at startup */
int ndomod_process_module_args(char *args){
	char *ptr=NULL;
	char **arglist=NULL;
	char **newarglist=NULL;
	int argcount=0;
	int memblocks=64;
	int arg=0;

	if(args==NULL)
		return NDO_OK;


	/* get all the var/val argument pairs */

	/* allocate some memory */
        if((arglist=(char **)malloc(memblocks*sizeof(char **)))==NULL)
                return NDO_ERROR;

	/* process all args */
        ptr=strtok(args,",");
        while(ptr){

		/* save the argument */
                arglist[argcount++]=strdup(ptr);

		/* allocate more memory if needed */
                if(!(argcount%memblocks)){
                        if((newarglist=(char **)realloc(arglist,(argcount+memblocks)*sizeof(char **)))==NULL){
				for(arg=0;arg<argcount;arg++)
					free(arglist[argcount]);
				free(arglist);
				return NDO_ERROR;
			        }
			else
				arglist=newarglist;
                        }

                ptr=strtok(NULL,",");
                }

	/* terminate the arg list */
        arglist[argcount]='\x0';


	/* process each argument */
	for(arg=0;arg<argcount;arg++){
		if(ndomod_process_config_var(arglist[arg])==NDO_ERROR){
			for(arg=0;arg<argcount;arg++)
				free(arglist[arg]);
			free(arglist);
			return NDO_ERROR;
		        }
	        }

	/* free allocated memory */
	for(arg=0;arg<argcount;arg++)
		free(arglist[arg]);
	free(arglist);
	
	return NDO_OK;
        }


/* process all config vars in a file */
int ndomod_process_config_file(char *filename){
	ndo_mmapfile *thefile=NULL;
	char *buf=NULL;
	int result=NDO_OK;

	/* open the file */
	if((thefile=ndo_mmap_fopen(filename))==NULL)
		return NDO_ERROR;

	/* process each line of the file */
	while((buf=ndo_mmap_fgets(thefile))){

		/* skip comments */
		if(buf[0]=='#'){
			free(buf);
			continue;
		        }

		/* skip blank lines */
		if(!strcmp(buf,"")){
			free(buf);
			continue;
		        }

		/* process the variable */
		result=ndomod_process_config_var(buf);

		/* free memory */
		free(buf);

		if(result!=NDO_OK)
			break;
	        }

	/* close the file */
	ndo_mmap_fclose(thefile);

	return result;
        }


/* process a single module config variable */
int ndomod_process_config_var(char *arg){
	char *var=NULL;
	char *val=NULL;

	/* split var/val */
	var=strtok(arg,"=");
	val=strtok(NULL,"\n");

	/* skip incomplete var/val pairs */
	if(var==NULL || val==NULL)
		return NDO_OK;

	/* process the variable... */

	if(!strcmp(var,"config_file"))
		ndomod_process_config_file(val);

	else if(!strcmp(var,"instance_name"))
		ndomod_instance_name=strdup(val);

	else if(!strcmp(var,"output"))
		ndomod_sink_name=strdup(val);

	else if(!strcmp(var,"output_type")){
		if(!strcmp(val,"file"))
			ndomod_sink_type=NDO_SINK_FILE;
		else if(!strcmp(val,"tcpsocket"))
			ndomod_sink_type=NDO_SINK_TCPSOCKET;
		else
			ndomod_sink_type=NDO_SINK_UNIXSOCKET;
	        }

	else if(!strcmp(var,"tcp_port"))
		ndomod_sink_tcp_port=atoi(val);

	else if(!strcmp(var,"output_buffer_items"))
		ndomod_sink_buffer_slots=strtoul(val,NULL,0);

	else if(!strcmp(var,"reconnect_interval"))
		ndomod_sink_reconnect_interval=strtoul(val,NULL,0);

	else if(!strcmp(var,"reconnect_warning_interval"))
		ndomod_sink_reconnect_warning_interval=strtoul(val,NULL,0);

	else if(!strcmp(var,"file_rotation_interval"))
		ndomod_sink_rotation_interval=strtoul(val,NULL,0);

	else if(!strcmp(var,"file_rotation_command"))
		ndomod_sink_rotation_command=strdup(val);

	else if(!strcmp(var,"file_rotation_timeout"))
		ndomod_sink_rotation_timeout=atoi(val);

	else if(!strcmp(var,"data_processing_options")){
		if(!strcmp(val,"-1"))
			ndomod_process_options=NDOMOD_PROCESS_EVERYTHING;
		else
			ndomod_process_options=strtoul(val,NULL,0);
	        }

	else if(!strcmp(var,"config_output_options"))
		ndomod_config_output_options=atoi(val);

	else if(!strcmp(var,"buffer_file"))
		ndomod_buffer_file=strdup(val);

	else
		return NDO_ERROR;

	return NDO_OK;
        }



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

/* writes a string to Nagios logs */
int ndomod_write_to_logs(char *buf, int flags){

	if(buf==NULL)
		return NDO_ERROR;

	return write_to_all_logs(buf,flags);
	}



/****************************************************************************/
/* DATA SINK FUNCTIONS                                                      */
/****************************************************************************/

/* (re)open data sink */
int ndomod_open_sink(void){
	int flags=0;

	/* sink is already open... */
	if(ndomod_sink_is_open==NDO_TRUE)
		return ndomod_sink_fd;

	/* try and open sink */
	if(ndomod_sink_type==NDO_SINK_FILE)
		flags=O_WRONLY|O_CREAT|O_APPEND;
	if(ndo_sink_open(ndomod_sink_name,0,ndomod_sink_type,ndomod_sink_tcp_port,flags,&ndomod_sink_fd)==NDO_ERROR)
		return NDO_ERROR;

	/* mark the sink as being open */
	ndomod_sink_is_open=NDO_TRUE;

	/* mark the sink as having once been open */
	ndomod_sink_previously_open=NDO_TRUE;

	return NDO_OK;
        }


/* (re)open data sink */
int ndomod_close_sink(void){

	/* sink is already closed... */
	if(ndomod_sink_is_open==NDO_FALSE)
		return NDO_OK;

	/* flush sink */
	ndo_sink_flush(ndomod_sink_fd);

	/* close sink */
	ndo_sink_close(ndomod_sink_fd);

	/* mark the sink as being closed */
	ndomod_sink_is_open=NDO_FALSE;

	return NDO_OK;
        }


/* say hello */
int ndomod_hello_sink(int reconnect, int problem_disconnect){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	char *connection_type=NULL;
	char *connect_type=NULL;

	/* get the connection type string */
	if(ndomod_sink_type==NDO_SINK_FD || ndomod_sink_type==NDO_SINK_FILE)
		connection_type=NDO_API_CONNECTION_FILE;
	else if(ndomod_sink_type==NDO_SINK_TCPSOCKET)
		connection_type=NDO_API_CONNECTION_TCPSOCKET;
	else
		connection_type=NDO_API_CONNECTION_UNIXSOCKET;

	/* get the connect type string */
	if(reconnect==TRUE && problem_disconnect==TRUE)
		connect_type=NDO_API_CONNECTTYPE_RECONNECT;
	else
		connect_type=NDO_API_CONNECTTYPE_INITIAL;

	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%s\n%s: %d\n%s: %s\n%s: %s\n%s: %lu\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s\n\n"
		 ,NDO_API_HELLO
		 ,NDO_API_PROTOCOL
		 ,NDO_API_PROTOVERSION
		 ,NDO_API_AGENT
		 ,NDOMOD_NAME
		 ,NDO_API_AGENTVERSION
		 ,NDOMOD_VERSION
		 ,NDO_API_STARTTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_DISPOSITION
		 ,NDO_API_DISPOSITION_REALTIME
		 ,NDO_API_CONNECTION
		 ,connection_type
		 ,NDO_API_CONNECTTYPE
		 ,connect_type
		 ,NDO_API_INSTANCENAME
		 ,(ndomod_instance_name==NULL)?"default":ndomod_instance_name
		 ,NDO_API_STARTDATADUMP
		);

	temp_buffer[sizeof(temp_buffer)-1]='\x0';

	ndomod_write_to_sink(temp_buffer,NDO_FALSE,NDO_FALSE);

	return NDO_OK;
        }


/* say goodbye */
int ndomod_goodbye_sink(void){
	char temp_buffer[NDOMOD_MAX_BUFLEN];

	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n%d\n%s: %lu\n%s\n\n"
		 ,NDO_API_ENDDATADUMP
		 ,NDO_API_ENDTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_GOODBYE
		 );

	temp_buffer[sizeof(temp_buffer)-1]='\x0';

	ndomod_write_to_sink(temp_buffer,NDO_FALSE,NDO_TRUE);

	return NDO_OK;
        }


/* used to rotate data sink file on a regular basis */
int ndomod_rotate_sink_file(void *args){
	char raw_command_line[MAX_COMMAND_BUFFER];
	char *raw_command_line_3x=NULL;
	char processed_command_line[MAX_COMMAND_BUFFER];
	char *processed_command_line_3x=NULL;
	int early_timeout=FALSE;
	double exectime;

	/* close sink */
	ndomod_goodbye_sink();
	ndomod_close_sink();

	/* we shouldn't write any data to the sink while we're rotating it... */
	ndomod_allow_sink_activity=NDO_FALSE;


	/****** ROTATE THE FILE *****/

	/* get the raw command line */
#ifdef BUILD_NAGIOS_2X
	get_raw_command_line(ndomod_sink_rotation_command,raw_command_line,sizeof(raw_command_line),STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#else
	get_raw_command_line(find_command(ndomod_sink_rotation_command),ndomod_sink_rotation_command,&raw_command_line_3x,STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#endif
	strip(raw_command_line);

	/* process any macros in the raw command line */
#ifdef BUILD_NAGIOS_2X
	process_macros(raw_command_line,processed_command_line,(int)sizeof(processed_command_line),STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#else
	process_macros(raw_command_line,&processed_command_line_3x,STRIP_ILLEGAL_MACRO_CHARS|ESCAPE_MACRO_CHARS);
#endif

	/* run the command */
	my_system(processed_command_line,ndomod_sink_rotation_timeout,&early_timeout,&exectime,NULL,0);


	/* allow data to be written to the sink */
	ndomod_allow_sink_activity=NDO_TRUE;

	/* re-open sink */
	ndomod_open_sink();
	ndomod_hello_sink(TRUE,FALSE);

	return NDO_OK;
        }


/* writes data to sink */
int ndomod_write_to_sink(char *buf, int buffer_write, int flush_buffer){
	char *temp_buffer=NULL;
	char *sbuf=NULL;
	int buflen=0;
	int result=NDO_OK;
	time_t current_time;
	int reconnect=NDO_FALSE;
	unsigned long items_to_flush=0L;

	/* we have nothing to write... */
	if(buf==NULL)
		return NDO_OK;
	
	/* we shouldn't be messing with things... */
	if(ndomod_allow_sink_activity==NDO_FALSE)
		return NDO_ERROR;

	/* open the sink if necessary... */
	if(ndomod_sink_is_open==NDO_FALSE){

		time(&current_time);

		/* are we reopening the sink? */
		if(ndomod_sink_previously_open==NDO_TRUE)
			reconnect=NDO_TRUE;

		/* (re)connect to the sink if its time */
		if((unsigned long)((unsigned long)current_time-ndomod_sink_reconnect_interval)>(unsigned long)ndomod_sink_last_reconnect_attempt){

			result=ndomod_open_sink();

			ndomod_sink_last_reconnect_attempt=current_time;

			ndomod_sink_connect_attempt++;

			/* sink was (re)opened... */
			if(result==NDO_OK){

				if(reconnect==NDO_TRUE){
					asprintf(&temp_buffer,"ndomod: Successfully reconnected to data sink!  %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
					ndomod_hello_sink(TRUE,TRUE);
				        }
				else{
					if(sinkbuf.overflow==0L)
						asprintf(&temp_buffer,"ndomod: Successfully connected to data sink.  %lu queued items to flush.",sinkbuf.items);
					else
						asprintf(&temp_buffer,"ndomod: Successfully connected to data sink.  %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
					ndomod_hello_sink(FALSE,FALSE);
				        }

				ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
				free(temp_buffer);
				temp_buffer=NULL;

				/* reset sink overflow */
				sinkbuf.overflow=0L;
				}

			/* sink could not be (re)opened... */
			else{

				if((unsigned long)((unsigned long)current_time-ndomod_sink_reconnect_warning_interval)>(unsigned long)ndomod_sink_last_reconnect_warning){
					if(reconnect==NDO_TRUE)
						asprintf(&temp_buffer,"ndomod: Still unable to reconnect to data sink.  %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
					else if(ndomod_sink_connect_attempt==1)
						asprintf(&temp_buffer,"ndomod: Could not open data sink!  I'll keep trying, but some output may get lost...");
					else
						asprintf(&temp_buffer,"ndomod: Still unable to connect to data sink.  %lu items lost, %lu queued items to flush.",sinkbuf.overflow,sinkbuf.items);
					ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
					free(temp_buffer);
					temp_buffer=NULL;

					ndomod_sink_last_reconnect_warning=current_time;
					}
				}
			}
	        }

	/* we weren't able to (re)connect */
	if(ndomod_sink_is_open==NDO_FALSE){

		/***** BUFFER OUTPUT FOR LATER *****/

		if(buffer_write==NDO_TRUE)
			ndomod_sink_buffer_push(&sinkbuf,buf);

		return NDO_ERROR;
	        }


	/***** FLUSH BUFFERED DATA FIRST *****/

	if(flush_buffer==NDO_TRUE && (items_to_flush=ndomod_sink_buffer_items(&sinkbuf))>0){

		while(ndomod_sink_buffer_items(&sinkbuf)>0){

			/* get next item from buffer */
			sbuf=ndomod_sink_buffer_peek(&sinkbuf);

			buflen=strlen(sbuf);
			result=ndo_sink_write(ndomod_sink_fd,sbuf,buflen);

			/* an error occurred... */
			if(result<0){

				/* sink problem! */
				if(errno!=EAGAIN){
			
					/* close the sink */
					ndomod_close_sink();

					asprintf(&temp_buffer,"ndomod: Error writing to data sink!  Some output may get lost.  %lu queued items to flush.",sinkbuf.items);
					ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
					free(temp_buffer);
					temp_buffer=NULL;

					time(&current_time);
					ndomod_sink_last_reconnect_attempt=current_time;
					ndomod_sink_last_reconnect_warning=current_time;
		                        }

				/***** BUFFER ORIGINAL OUTPUT FOR LATER *****/

				if(buffer_write==NDO_TRUE)
					ndomod_sink_buffer_push(&sinkbuf,buf);
				
				return NDO_ERROR;
	                        }

			/* buffer was written okay, so remove it from buffer */
			ndomod_sink_buffer_pop(&sinkbuf);
		        }

		asprintf(&temp_buffer,"ndomod: Successfully flushed %lu queued items to data sink.",items_to_flush);
		ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
		free(temp_buffer);
		temp_buffer=NULL;
	        }


	/***** WRITE ORIGINAL DATA *****/

	/* write the data */
	buflen=strlen(buf);
	result=ndo_sink_write(ndomod_sink_fd,buf,buflen);

	/* an error occurred... */
	if(result<0){

		/* sink problem! */
		if(errno!=EAGAIN){

			/* close the sink */
			ndomod_close_sink();

			time(&current_time);
			ndomod_sink_last_reconnect_attempt=current_time;
			ndomod_sink_last_reconnect_warning=current_time;

			asprintf(&temp_buffer,"ndomod: Error writing to data sink!  Some output may get lost...");
			ndomod_write_to_logs(temp_buffer,NSLOG_INFO_MESSAGE);
			free(temp_buffer);
			temp_buffer=NULL;
		        }

		/***** BUFFER OUTPUT FOR LATER *****/

		if(buffer_write==NDO_TRUE)
			ndomod_sink_buffer_push(&sinkbuf,buf);

		return NDO_ERROR;
	        }

	return NDO_OK;
        }



/* save unprocessed data to buffer file */
int ndomod_save_unprocessed_data(char *f){
	FILE *fp=NULL;
	char *buf=NULL;
	char *ebuf=NULL;

	/* no file */
	if(f==NULL)
		return NDO_OK;

	/* open the file for writing */
	if((fp=fopen(f,"w"))==NULL)
		return NDO_ERROR;

	/* save all buffered items */
	while(ndomod_sink_buffer_items(&sinkbuf)>0){

		/* get next item from buffer */
		buf=ndomod_sink_buffer_pop(&sinkbuf);

		/* escape the string */
		ebuf=ndo_escape_buffer(buf);

		/* write string to file */
		fputs(ebuf,fp);
		fputs("\n",fp);

		/* free memory */
		free(buf);
		buf=NULL;
		free(ebuf);
		ebuf=NULL;
		}

	fclose(fp);

	return NDO_OK;
	}



/* load unprocessed data from buffer file */
int ndomod_load_unprocessed_data(char *f){
	ndo_mmapfile *thefile=NULL;
	char *ebuf=NULL;
	char *buf=NULL;

	/* open the file */
	if((thefile=ndo_mmap_fopen(f))==NULL)
		return NDO_ERROR;

	/* process each line of the file */
	while((ebuf=ndo_mmap_fgets(thefile))){

		/* unescape string */
		buf=ndo_unescape_buffer(ebuf);

		/* save the data to the sink buffer */
		ndomod_sink_buffer_push(&sinkbuf,buf);

		/* free memory */
		free(ebuf);
	        }

	/* close the file */
	ndo_mmap_fclose(thefile);

	/* remove the file so we don't process it again in the future */
	unlink(f);

	return NDO_OK;
	}



/* initializes sink buffer */
int ndomod_sink_buffer_init(ndomod_sink_buffer *sbuf,unsigned long maxitems){
	int x;

	if(sbuf==NULL || maxitems<=0)
		return NDO_ERROR;

	/* allocate memory for the buffer */
	if((sbuf->buffer=(char **)malloc(sizeof(char *)*maxitems))){
		for(x=0;x<maxitems;x++)
			sbuf->buffer[x]=NULL;
	        }

	sbuf->size=0L;
	sbuf->head=0L;
	sbuf->tail=0L;
	sbuf->items=0L;
	sbuf->maxitems=maxitems;
	sbuf->overflow=0L;

	return NDO_OK;
        }


/* deinitializes sink buffer */
int ndomod_sink_buffer_deinit(ndomod_sink_buffer *sbuf){
	int x;

	if(sbuf==NULL)
		return NDO_ERROR;

	/* free any allocated memory */
	for(x=0;x<sbuf->maxitems;x++)
		free(sbuf->buffer[x]);

	free(sbuf->buffer);
	sbuf->buffer=NULL;

	return NDO_OK;
        }


/* buffers output */
int ndomod_sink_buffer_push(ndomod_sink_buffer *sbuf,char *buf){

	if(sbuf==NULL || buf==NULL)
		return NDO_ERROR;

	/* no space to store buffer */
	if(sbuf->buffer==NULL || sbuf->items==sbuf->maxitems){
		sbuf->overflow++;
		return NDO_ERROR;
	        }

	/* store buffer */
	sbuf->buffer[sbuf->head]=strdup(buf);
	sbuf->head=(sbuf->head+1)%sbuf->maxitems;
	sbuf->items++;

	return NDO_OK;
        }


/* gets and removes next item from buffer */
char *ndomod_sink_buffer_pop(ndomod_sink_buffer *sbuf){
	char *buf=NULL;

	if(sbuf==NULL)
		return NULL;

	if(sbuf->buffer==NULL)
		return NULL;

	if(sbuf->items==0)
		return NULL;

	/* remove item from buffer */
	buf=sbuf->buffer[sbuf->tail];
	sbuf->buffer[sbuf->tail]=NULL;
	sbuf->tail=(sbuf->tail+1)%sbuf->maxitems;
	sbuf->items--;

	return buf;
        }


/* gets next items from buffer */
char *ndomod_sink_buffer_peek(ndomod_sink_buffer *sbuf){
	char *buf=NULL;

	if(sbuf==NULL)
		return NULL;

	if(sbuf->buffer==NULL)
		return NULL;

	buf=sbuf->buffer[sbuf->tail];

	return buf;
        }


/* returns number of items buffered */
int ndomod_sink_buffer_items(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->items;
        }


/* gets number of items lost due to buffer overflow */
unsigned long ndomod_sink_buffer_get_overflow(ndomod_sink_buffer *sbuf){

	if(sbuf==NULL)
		return 0;
	else
		return sbuf->overflow;
        }


/* sets number of items lost due to buffer overflow */
int ndomod_sink_buffer_set_overflow(ndomod_sink_buffer *sbuf, unsigned long num){

	if(sbuf==NULL)
		return 0;
	else
		sbuf->overflow=num;
	
	return sbuf->overflow;
        }



/****************************************************************************/
/* CALLBACK FUNCTIONS                                                       */
/****************************************************************************/

/* registers for callbacks */
int ndomod_register_callbacks(void){
	int priority=0;
	int result=NDO_OK;

	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_PROCESS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_TIMED_EVENT_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_LOG_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_EVENT_HANDLER_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_NOTIFICATION_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_SERVICE_CHECK_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_HOST_CHECK_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_COMMENT_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_DOWNTIME_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_FLAPPING_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_PROGRAM_STATUS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_HOST_STATUS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_SERVICE_STATUS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_RETENTION_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_STATE_CHANGE_DATA,ndomod_module_handle,priority,ndomod_broker_data);
#ifdef BUILD_NAGIOS_3X
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_CONTACT_STATUS_DATA,ndomod_module_handle,priority,ndomod_broker_data);
	if(result==NDO_OK)
		result=neb_register_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA,ndomod_module_handle,priority,ndomod_broker_data);
#endif

	return result;
        }


/* deregisters callbacks */
int ndomod_deregister_callbacks(void){

	neb_deregister_callback(NEBCALLBACK_PROCESS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_LOG_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_COMMENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_RETENTION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA,ndomod_broker_data);
#ifdef BUILD_NAGIOS_3X
	neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA,ndomod_broker_data);
	neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA,ndomod_broker_data);
#endif

	return NDO_OK;
        }


/* handles brokered event data */
int ndomod_broker_data(int event_type, void *data){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	ndo_dbuf dbuf;
	int write_to_sink=NDO_TRUE;
	host *temp_host=NULL;
	service *temp_service=NULL;
#ifdef BUILD_NAGIOS_3X
	contact *temp_contact=NULL;
#endif
	char *es[8];
	int x=0;
	scheduled_downtime *temp_downtime=NULL;
	comment *temp_comment=NULL;
	nebstruct_process_data *procdata=NULL;
	nebstruct_timed_event_data *eventdata=NULL;
	nebstruct_log_data *logdata=NULL;
	nebstruct_system_command_data *cmddata=NULL;
	nebstruct_event_handler_data *ehanddata=NULL;
	nebstruct_notification_data *notdata=NULL;
	nebstruct_service_check_data *scdata=NULL;
	nebstruct_host_check_data *hcdata=NULL;
	nebstruct_comment_data *comdata=NULL;
	nebstruct_downtime_data *downdata=NULL;
	nebstruct_flapping_data *flapdata=NULL;
	nebstruct_program_status_data *psdata=NULL;
	nebstruct_host_status_data *hsdata=NULL;
	nebstruct_service_status_data *ssdata=NULL;
	nebstruct_adaptive_program_data *apdata=NULL;
	nebstruct_adaptive_host_data *ahdata=NULL;
	nebstruct_adaptive_service_data *asdata=NULL;
	nebstruct_external_command_data *ecdata=NULL;
	nebstruct_aggregated_status_data *agsdata=NULL;
	nebstruct_retention_data *rdata=NULL;
	nebstruct_contact_notification_data *cnotdata=NULL;
	nebstruct_contact_notification_method_data *cnotmdata=NULL;
	nebstruct_acknowledgement_data *ackdata=NULL;
	nebstruct_statechange_data *schangedata=NULL;
#ifdef BUILD_NAGIOS_3X
	nebstruct_contact_status_data *csdata=NULL;
	nebstruct_adaptive_contact_data *acdata=NULL;
#endif
	double retry_interval=0.0;
	int last_state=-1;
	int last_hard_state=-1;
#ifdef BUILD_NAGIOS_3X
	customvariablesmember *temp_customvar=NULL;
#endif

	if(data==NULL)
		return 0;

	/* should we handle this type of data? */
	switch(event_type){

	case NEBCALLBACK_PROCESS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_PROCESS_DATA))
			return 0;
		break;
	case NEBCALLBACK_TIMED_EVENT_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_TIMED_EVENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_LOG_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_LOG_DATA))
			return 0;
		break;
	case NEBCALLBACK_SYSTEM_COMMAND_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_SYSTEM_COMMAND_DATA))
			return 0;
		break;
	case NEBCALLBACK_EVENT_HANDLER_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_EVENT_HANDLER_DATA))
			return 0;
		break;
	case NEBCALLBACK_NOTIFICATION_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_SERVICE_CHECK_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_SERVICE_CHECK_DATA))
			return 0;
		break;
	case NEBCALLBACK_HOST_CHECK_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_HOST_CHECK_DATA))
			return 0;
		break;
	case NEBCALLBACK_COMMENT_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_COMMENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_DOWNTIME_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_DOWNTIME_DATA))
			return 0;
		break;
	case NEBCALLBACK_FLAPPING_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_FLAPPING_DATA))
			return 0;
		break;
	case NEBCALLBACK_PROGRAM_STATUS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_PROGRAM_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_HOST_STATUS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_HOST_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_SERVICE_STATUS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_SERVICE_STATUS_DATA))
			return 0;
		break;
#ifdef BUILD_NAGIOS_3X
	case NEBCALLBACK_CONTACT_STATUS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_CONTACT_STATUS_DATA))
			return 0;
		break;
#endif
	case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_HOST_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_HOST_DATA))
			return 0;
		break;
	case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA))
			return 0;
		break;
#ifdef BUILD_NAGIOS_3X
	case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA))
			return 0;
		break;
#endif
	case NEBCALLBACK_EXTERNAL_COMMAND_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA))
			return 0;
		break;
	case NEBCALLBACK_AGGREGATED_STATUS_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_AGGREGATED_STATUS_DATA))
			return 0;
		break;
	case NEBCALLBACK_RETENTION_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_RETENTION_DATA))
			return 0;
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_NOTIFICATION_DATA))
			return 0;
		break;
	case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA))
			return 0;
		break;
	case NEBCALLBACK_STATE_CHANGE_DATA:
		if(!(ndomod_process_options & NDOMOD_PROCESS_STATECHANGE_DATA))
			return 0;
		break;
	default:
		break;
		}


	/* initialize escaped buffers */
	for(x=0;x<8;x++)
		es[x]=NULL;

	/* initialize dynamic buffer (2KB chunk size) */
	ndo_dbuf_init(&dbuf,2048);


	/* handle the event */
	switch(event_type){

	case NEBCALLBACK_PROCESS_DATA:

		procdata=(nebstruct_process_data *)data;

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%lu\n%d\n\n"
			 ,NDO_API_PROCESSDATA
			 ,NDO_DATA_TYPE
			 ,procdata->type
			 ,NDO_DATA_FLAGS
			 ,procdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,procdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,procdata->timestamp.tv_sec
			 ,procdata->timestamp.tv_usec
			 ,NDO_DATA_PROGRAMNAME
			 ,"Nagios"
			 ,NDO_DATA_PROGRAMVERSION
			 ,get_program_version()
			 ,NDO_DATA_PROGRAMDATE
			 ,get_program_modification_date()
			 ,NDO_DATA_PROCESSID
			 ,(unsigned long)getpid()
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_TIMED_EVENT_DATA:

		eventdata=(nebstruct_timed_event_data *)data;

		switch(eventdata->event_type){

		case EVENT_SERVICE_CHECK:
			temp_service=(service *)eventdata->event_data;

			es[0]=ndo_escape_buffer(temp_service->host_name);
			es[1]=ndo_escape_buffer(temp_service->description);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
				 ,NDO_API_TIMEDEVENTDATA
				 ,NDO_DATA_TYPE
				 ,eventdata->type
				 ,NDO_DATA_FLAGS
				 ,eventdata->flags
				 ,NDO_DATA_ATTRIBUTES
				 ,eventdata->attr
				 ,NDO_DATA_TIMESTAMP
				 ,eventdata->timestamp.tv_sec
				 ,eventdata->timestamp.tv_usec
				 ,NDO_DATA_EVENTTYPE
				 ,eventdata->event_type
				 ,NDO_DATA_RECURRING
				 ,eventdata->recurring
				 ,NDO_DATA_RUNTIME
				 ,(unsigned long)eventdata->run_time
				 ,NDO_DATA_HOST
				 ,(es[0]==NULL)?"":es[0]
				 ,NDO_DATA_SERVICE
				 ,(es[1]==NULL)?"":es[1]
				 ,NDO_API_ENDDATA
				);

			break;

		case EVENT_HOST_CHECK:
			temp_host=(host *)eventdata->event_data;

			es[0]=ndo_escape_buffer(temp_host->name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d\n\n"
				 ,NDO_API_TIMEDEVENTDATA
				 ,NDO_DATA_TYPE
				 ,eventdata->type
				 ,NDO_DATA_FLAGS
				 ,eventdata->flags
				 ,NDO_DATA_ATTRIBUTES
				 ,eventdata->attr
				 ,NDO_DATA_TIMESTAMP
				 ,eventdata->timestamp.tv_sec
				 ,eventdata->timestamp.tv_usec
				 ,NDO_DATA_EVENTTYPE
				 ,eventdata->event_type
				 ,NDO_DATA_RECURRING
				 ,eventdata->recurring
				 ,NDO_DATA_RUNTIME
				 ,(unsigned long)eventdata->run_time
				 ,NDO_DATA_HOST
				 ,(es[0]==NULL)?"":es[0]
				 ,NDO_API_ENDDATA
				);

			break;

		case EVENT_SCHEDULED_DOWNTIME:
			temp_downtime=find_downtime(ANY_DOWNTIME,(unsigned long)eventdata->event_data);

			if(temp_downtime!=NULL){
				es[0]=ndo_escape_buffer(temp_downtime->host_name);
				es[1]=ndo_escape_buffer(temp_downtime->service_description);
				}

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
				 ,NDO_API_TIMEDEVENTDATA
				 ,NDO_DATA_TYPE
				 ,eventdata->type
				 ,NDO_DATA_FLAGS
				 ,eventdata->flags
				 ,NDO_DATA_ATTRIBUTES
				 ,eventdata->attr
				 ,NDO_DATA_TIMESTAMP
				 ,eventdata->timestamp.tv_sec
				 ,eventdata->timestamp.tv_usec
				 ,NDO_DATA_EVENTTYPE
				 ,eventdata->event_type
				 ,NDO_DATA_RECURRING
				 ,eventdata->recurring
				 ,NDO_DATA_RUNTIME
				 ,(unsigned long)eventdata->run_time
				 ,NDO_DATA_HOST
				 ,(es[0]==NULL)?"":es[0]
				 ,NDO_DATA_SERVICE
				 ,(es[1]==NULL)?"":es[1]
				 ,NDO_API_ENDDATA
				);

			break;

		default:
			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%d\n%d=%lu\n%d\n\n"
				 ,NDO_API_TIMEDEVENTDATA
				 ,NDO_DATA_TYPE
				 ,eventdata->type
				 ,NDO_DATA_FLAGS
				 ,eventdata->flags
				 ,NDO_DATA_ATTRIBUTES
				 ,eventdata->attr
				 ,NDO_DATA_TIMESTAMP
				 ,eventdata->timestamp.tv_sec
				 ,eventdata->timestamp.tv_usec
				 ,NDO_DATA_EVENTTYPE
				 ,eventdata->event_type
				 ,NDO_DATA_RECURRING
				 ,eventdata->recurring
				 ,NDO_DATA_RUNTIME
				 ,(unsigned long)eventdata->run_time
				 ,NDO_API_ENDDATA
				);
			break;
		        }

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_LOG_DATA:

		logdata=(nebstruct_log_data *)data;

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%lu\n%d=%d\n%d=%s\n%d\n\n"
			 ,NDO_API_LOGDATA
			 ,NDO_DATA_TYPE
			 ,logdata->type
			 ,NDO_DATA_FLAGS
			 ,logdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,logdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,logdata->timestamp.tv_sec
			 ,logdata->timestamp.tv_usec
			 ,NDO_DATA_LOGENTRYTIME
			 ,logdata->entry_time
			 ,NDO_DATA_LOGENTRYTYPE
			 ,logdata->data_type
			 ,NDO_DATA_LOGENTRY
			 ,logdata->data
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_SYSTEM_COMMAND_DATA:

		cmddata=(nebstruct_system_command_data *)data;

		es[0]=ndo_escape_buffer(cmddata->command_line);
		es[1]=ndo_escape_buffer(cmddata->output);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%d\n%d=%.5lf\n%d=%d\n%d=%s\n%d\n\n"
			 ,NDO_API_SYSTEMCOMMANDDATA
			 ,NDO_DATA_TYPE
			 ,cmddata->type
			 ,NDO_DATA_FLAGS
			 ,cmddata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,cmddata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,cmddata->timestamp.tv_sec
			 ,cmddata->timestamp.tv_usec	
			 ,NDO_DATA_STARTTIME
			 ,cmddata->start_time.tv_sec
			 ,cmddata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,cmddata->end_time.tv_sec
			 ,cmddata->end_time.tv_usec
			 ,NDO_DATA_TIMEOUT
			 ,cmddata->timeout
			 ,NDO_DATA_COMMANDLINE
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_EARLYTIMEOUT
			 ,cmddata->early_timeout
			 ,NDO_DATA_EXECUTIONTIME
			 ,cmddata->execution_time
			 ,NDO_DATA_RETURNCODE
			 ,cmddata->return_code
			 ,NDO_DATA_OUTPUT
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_EVENT_HANDLER_DATA:

		ehanddata=(nebstruct_event_handler_data *)data;

		es[0]=ndo_escape_buffer(ehanddata->host_name);
		es[1]=ndo_escape_buffer(ehanddata->service_description);
		es[2]=ndo_escape_buffer(ehanddata->command_name);
		es[3]=ndo_escape_buffer(ehanddata->command_args);
		es[4]=ndo_escape_buffer(ehanddata->command_line);
		es[5]=ndo_escape_buffer(ehanddata->output);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%.5lf\n%d=%d\n%d=%s\n%d\n\n"
			 ,NDO_API_EVENTHANDLERDATA
			 ,NDO_DATA_TYPE
			 ,ehanddata->type
			 ,NDO_DATA_FLAGS
			 ,ehanddata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,ehanddata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,ehanddata->timestamp.tv_sec
			 ,ehanddata->timestamp.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_STATETYPE
			 ,ehanddata->state_type
			 ,NDO_DATA_STATE
			 ,ehanddata->state
			 ,NDO_DATA_STARTTIME
			 ,ehanddata->start_time.tv_sec
			 ,ehanddata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,ehanddata->end_time.tv_sec
			 ,ehanddata->end_time.tv_usec
			 ,NDO_DATA_TIMEOUT
			 ,ehanddata->timeout
			 ,NDO_DATA_COMMANDNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMANDARGS
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_COMMANDLINE
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_EARLYTIMEOUT
			 ,ehanddata->early_timeout
			 ,NDO_DATA_EXECUTIONTIME
			 ,ehanddata->execution_time
			 ,NDO_DATA_RETURNCODE
			 ,ehanddata->return_code
			 ,NDO_DATA_OUTPUT
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_NOTIFICATION_DATA:

		notdata=(nebstruct_notification_data *)data;

		es[0]=ndo_escape_buffer(notdata->host_name);
		es[1]=ndo_escape_buffer(notdata->service_description);
		es[2]=ndo_escape_buffer(notdata->output);
		es[3]=ndo_escape_buffer(notdata->ack_author);
		es[4]=ndo_escape_buffer(notdata->ack_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d\n\n"
			 ,NDO_API_NOTIFICATIONDATA
			 ,NDO_DATA_TYPE
			 ,notdata->type
			 ,NDO_DATA_FLAGS
			 ,notdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,notdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,notdata->timestamp.tv_sec
			 ,notdata->timestamp.tv_usec
			 ,NDO_DATA_NOTIFICATIONTYPE
			 ,notdata->notification_type
			 ,NDO_DATA_STARTTIME
			 ,notdata->start_time.tv_sec
			 ,notdata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,notdata->end_time.tv_sec
			 ,notdata->end_time.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_NOTIFICATIONREASON
			 ,notdata->reason_type
			 ,NDO_DATA_STATE
			 ,notdata->state
			 ,NDO_DATA_OUTPUT
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_ACKAUTHOR
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_ACKDATA
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_ESCALATED
			 ,notdata->escalated
			 ,NDO_DATA_CONTACTSNOTIFIED
			 ,notdata->contacts_notified
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_SERVICE_CHECK_DATA:

		scdata=(nebstruct_service_check_data *)data;

		es[0]=ndo_escape_buffer(scdata->host_name);
		es[1]=ndo_escape_buffer(scdata->service_description);
		es[2]=ndo_escape_buffer(scdata->command_name);
		es[3]=ndo_escape_buffer(scdata->command_args);
		es[4]=ndo_escape_buffer(scdata->command_line);
		es[5]=ndo_escape_buffer(scdata->output);
		es[6]=ndo_escape_buffer(scdata->perf_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_SERVICECHECKDATA
			 ,NDO_DATA_TYPE
			 ,scdata->type
			 ,NDO_DATA_FLAGS
			 ,scdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,scdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,scdata->timestamp.tv_sec
			 ,scdata->timestamp.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_CHECKTYPE
			 ,scdata->check_type
			 ,NDO_DATA_CURRENTCHECKATTEMPT
			 ,scdata->current_attempt
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,scdata->max_attempts
			 ,NDO_DATA_STATETYPE
			 ,scdata->state_type
			 ,NDO_DATA_STATE
			 ,scdata->state
			 ,NDO_DATA_TIMEOUT
			 ,scdata->timeout
			 ,NDO_DATA_COMMANDNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMANDARGS
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_COMMANDLINE
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_STARTTIME
			 ,scdata->start_time.tv_sec
			 ,scdata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,scdata->end_time.tv_sec
			 ,scdata->end_time.tv_usec
			 ,NDO_DATA_EARLYTIMEOUT
			 ,scdata->early_timeout
			 ,NDO_DATA_EXECUTIONTIME
			 ,scdata->execution_time
			 ,NDO_DATA_LATENCY
			 ,scdata->latency
			 ,NDO_DATA_RETURNCODE
			 ,scdata->return_code
			 ,NDO_DATA_OUTPUT
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_PERFDATA
			 ,(es[6]==NULL)?"":es[6]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_HOST_CHECK_DATA:

		hcdata=(nebstruct_host_check_data *)data;

		es[0]=ndo_escape_buffer(hcdata->host_name);
		es[1]=ndo_escape_buffer(hcdata->command_name);
		es[2]=ndo_escape_buffer(hcdata->command_args);
		es[3]=ndo_escape_buffer(hcdata->command_line);
		es[4]=ndo_escape_buffer(hcdata->output);
		es[5]=ndo_escape_buffer(hcdata->perf_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_HOSTCHECKDATA
			 ,NDO_DATA_TYPE
			 ,hcdata->type
			 ,NDO_DATA_FLAGS
			 ,hcdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,hcdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,hcdata->timestamp.tv_sec
			 ,hcdata->timestamp.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_CHECKTYPE
			 ,hcdata->check_type
			 ,NDO_DATA_CURRENTCHECKATTEMPT
			 ,hcdata->current_attempt
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,hcdata->max_attempts
			 ,NDO_DATA_STATETYPE
			 ,hcdata->state_type
			 ,NDO_DATA_STATE
			 ,hcdata->state
			 ,NDO_DATA_TIMEOUT
			 ,hcdata->timeout
			 ,NDO_DATA_COMMANDNAME
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_COMMANDARGS
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMANDLINE
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_STARTTIME
			 ,hcdata->start_time.tv_sec
			 ,hcdata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,hcdata->end_time.tv_sec
			 ,hcdata->end_time.tv_usec
			 ,NDO_DATA_EARLYTIMEOUT
			 ,hcdata->early_timeout
			 ,NDO_DATA_EXECUTIONTIME
			 ,hcdata->execution_time
			 ,NDO_DATA_LATENCY
			 ,hcdata->latency
			 ,NDO_DATA_RETURNCODE
			 ,hcdata->return_code
			 ,NDO_DATA_OUTPUT
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_PERFDATA
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_COMMENT_DATA:

		comdata=(nebstruct_comment_data *)data;

		es[0]=ndo_escape_buffer(comdata->host_name);
		es[1]=ndo_escape_buffer(comdata->service_description);
		es[2]=ndo_escape_buffer(comdata->author_name);
		es[3]=ndo_escape_buffer(comdata->comment_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%lu\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d\n\n"
			 ,NDO_API_COMMENTDATA
			 ,NDO_DATA_TYPE
			 ,comdata->type
			 ,NDO_DATA_FLAGS
			 ,comdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,comdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,comdata->timestamp.tv_sec
			 ,comdata->timestamp.tv_usec
			 ,NDO_DATA_COMMENTTYPE
			 ,comdata->comment_type
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_ENTRYTIME
			 ,(unsigned long)comdata->entry_time
			 ,NDO_DATA_AUTHORNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMENT
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_PERSISTENT
			 ,comdata->persistent
			 ,NDO_DATA_SOURCE
			 ,comdata->source
			 ,NDO_DATA_ENTRYTYPE
			 ,comdata->entry_type
			 ,NDO_DATA_EXPIRES
			 ,comdata->expires
			 ,NDO_DATA_EXPIRATIONTIME
			 ,(unsigned long)comdata->expire_time
			 ,NDO_DATA_COMMENTID
			 ,comdata->comment_id
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_DOWNTIME_DATA:

		downdata=(nebstruct_downtime_data *)data;

		es[0]=ndo_escape_buffer(downdata->host_name);
		es[1]=ndo_escape_buffer(downdata->service_description);
		es[2]=ndo_escape_buffer(downdata->author_name);
		es[3]=ndo_escape_buffer(downdata->comment_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%lu\n%d=%s\n%d=%s\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d\n\n"
			 ,NDO_API_DOWNTIMEDATA
			 ,NDO_DATA_TYPE
			 ,downdata->type
			 ,NDO_DATA_FLAGS
			 ,downdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,downdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,downdata->timestamp.tv_sec
			 ,downdata->timestamp.tv_usec
			 ,NDO_DATA_DOWNTIMETYPE
			 ,downdata->downtime_type
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_ENTRYTIME
			 ,downdata->entry_time
			 ,NDO_DATA_AUTHORNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMENT
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_STARTTIME
			 ,(unsigned long)downdata->start_time
			 ,NDO_DATA_ENDTIME
			 ,(unsigned long)downdata->end_time
			 ,NDO_DATA_FIXED
			 ,downdata->fixed
			 ,NDO_DATA_DURATION
			 ,(unsigned long)downdata->duration
			 ,NDO_DATA_TRIGGEREDBY
			 ,(unsigned long)downdata->triggered_by
			 ,NDO_DATA_DOWNTIMEID
			 ,(unsigned long)downdata->downtime_id
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_FLAPPING_DATA:

		flapdata=(nebstruct_flapping_data *)data;

		es[0]=ndo_escape_buffer(flapdata->host_name);
		es[1]=ndo_escape_buffer(flapdata->service_description);

		if(flapdata->flapping_type==HOST_FLAPPING)
			temp_comment=find_host_comment(flapdata->comment_id);
		else
			temp_comment=find_service_comment(flapdata->comment_id);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%lu\n%d=%lu\n%d\n\n"
			 ,NDO_API_FLAPPINGDATA
			 ,NDO_DATA_TYPE
			 ,flapdata->type
			 ,NDO_DATA_FLAGS
			 ,flapdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,flapdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,flapdata->timestamp.tv_sec
			 ,flapdata->timestamp.tv_usec
			 ,NDO_DATA_FLAPPINGTYPE
			 ,flapdata->flapping_type
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_PERCENTSTATECHANGE
			 ,flapdata->percent_change
			 ,NDO_DATA_HIGHTHRESHOLD
			 ,flapdata->high_threshold
			 ,NDO_DATA_LOWTHRESHOLD
			 ,flapdata->low_threshold
			 ,NDO_DATA_COMMENTTIME
			 ,(temp_comment==NULL)?0L:(unsigned long)temp_comment->entry_time
			 ,NDO_DATA_COMMENTID
			 ,flapdata->comment_id
			 ,NDO_API_ENDDATA
			);
	
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_PROGRAM_STATUS_DATA:

		psdata=(nebstruct_program_status_data *)data;

		es[0]=ndo_escape_buffer(psdata->global_host_event_handler);
		es[1]=ndo_escape_buffer(psdata->global_service_event_handler);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%lu\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_PROGRAMSTATUSDATA
			 ,NDO_DATA_TYPE
			 ,psdata->type
			 ,NDO_DATA_FLAGS
			 ,psdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,psdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,psdata->timestamp.tv_sec
			 ,psdata->timestamp.tv_usec
			 ,NDO_DATA_PROGRAMSTARTTIME
			 ,(unsigned long)psdata->program_start
			 ,NDO_DATA_PROCESSID
			 ,psdata->pid
			 ,NDO_DATA_DAEMONMODE
			 ,psdata->daemon_mode
			 ,NDO_DATA_LASTCOMMANDCHECK
			 ,(unsigned long)psdata->last_command_check
			 ,NDO_DATA_LASTLOGROTATION
			 ,(unsigned long)psdata->last_log_rotation
			 ,NDO_DATA_NOTIFICATIONSENABLED
			 ,psdata->notifications_enabled
			 ,NDO_DATA_ACTIVESERVICECHECKSENABLED
			 ,psdata->active_service_checks_enabled
			 ,NDO_DATA_PASSIVESERVICECHECKSENABLED
			 ,psdata->passive_service_checks_enabled
			 ,NDO_DATA_ACTIVEHOSTCHECKSENABLED
			 ,psdata->active_host_checks_enabled
			 ,NDO_DATA_PASSIVEHOSTCHECKSENABLED
			 ,psdata->passive_host_checks_enabled
			 ,NDO_DATA_EVENTHANDLERSENABLED
			 ,psdata->event_handlers_enabled
			 ,NDO_DATA_FLAPDETECTIONENABLED
			 ,psdata->flap_detection_enabled
			 ,NDO_DATA_FAILUREPREDICTIONENABLED
			 ,psdata->failure_prediction_enabled
			 ,NDO_DATA_PROCESSPERFORMANCEDATA
			 ,psdata->process_performance_data
			 ,NDO_DATA_OBSESSOVERHOSTS
			 ,psdata->obsess_over_hosts
			 ,NDO_DATA_OBSESSOVERSERVICES
			 ,psdata->obsess_over_services
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,psdata->modified_host_attributes
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,psdata->modified_service_attributes
			 ,NDO_DATA_GLOBALHOSTEVENTHANDLER
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_GLOBALSERVICEEVENTHANDLER
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_HOST_STATUS_DATA:

		hsdata=(nebstruct_host_status_data *)data;

		if((temp_host=(host *)hsdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}
		
		es[0]=ndo_escape_buffer(temp_host->name);
		es[1]=ndo_escape_buffer(temp_host->plugin_output);
		es[2]=ndo_escape_buffer(temp_host->perf_data);
		es[3]=ndo_escape_buffer(temp_host->event_handler);
		es[4]=ndo_escape_buffer(temp_host->host_check_command);
		es[5]=ndo_escape_buffer(temp_host->check_period);

#ifdef BUILLD_NAGIOS_3X
		retry_interval=temp_host->retry_interval;
#endif
#ifdef BUILD_NAGIOS_2X
		retry_interval=0.0;
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%s\n"
			 ,NDO_API_HOSTSTATUSDATA
			 ,NDO_DATA_TYPE
			 ,hsdata->type
			 ,NDO_DATA_FLAGS
			 ,hsdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,hsdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,hsdata->timestamp.tv_sec
			 ,hsdata->timestamp.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_OUTPUT
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_PERFDATA
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_CURRENTSTATE
			 ,temp_host->current_state
			 ,NDO_DATA_HASBEENCHECKED
			 ,temp_host->has_been_checked
			 ,NDO_DATA_SHOULDBESCHEDULED
			 ,temp_host->should_be_scheduled
			 ,NDO_DATA_CURRENTCHECKATTEMPT
			 ,temp_host->current_attempt
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,temp_host->max_attempts
			 ,NDO_DATA_LASTHOSTCHECK
			 ,(unsigned long)temp_host->last_check
			 ,NDO_DATA_NEXTHOSTCHECK
			 ,(unsigned long)temp_host->next_check
			 ,NDO_DATA_CHECKTYPE
			 ,temp_host->check_type
			 ,NDO_DATA_LASTSTATECHANGE
			 ,(unsigned long)temp_host->last_state_change
			 ,NDO_DATA_LASTHARDSTATECHANGE
			 ,(unsigned long)temp_host->last_hard_state_change
			 ,NDO_DATA_LASTHARDSTATE
			 ,temp_host->last_hard_state
			 ,NDO_DATA_LASTTIMEUP
			 ,(unsigned long)temp_host->last_time_up
			 ,NDO_DATA_LASTTIMEDOWN
			 ,(unsigned long)temp_host->last_time_down
			 ,NDO_DATA_LASTTIMEUNREACHABLE
			 ,(unsigned long)temp_host->last_time_unreachable
			 ,NDO_DATA_STATETYPE
			 ,temp_host->state_type
			 ,NDO_DATA_LASTHOSTNOTIFICATION
			 ,(unsigned long)temp_host->last_host_notification
			 ,NDO_DATA_NEXTHOSTNOTIFICATION
			 ,(unsigned long)temp_host->next_host_notification
			 ,NDO_DATA_NOMORENOTIFICATIONS
			 ,temp_host->no_more_notifications
			 ,NDO_DATA_NOTIFICATIONSENABLED
			 ,temp_host->notifications_enabled
			 ,NDO_DATA_PROBLEMHASBEENACKNOWLEDGED
			 ,temp_host->problem_has_been_acknowledged
			 ,NDO_DATA_ACKNOWLEDGEMENTTYPE
			 ,temp_host->acknowledgement_type
			 ,NDO_DATA_CURRENTNOTIFICATIONNUMBER
			 ,temp_host->current_notification_number
			 ,NDO_DATA_PASSIVEHOSTCHECKSENABLED
			 ,temp_host->accept_passive_host_checks
			 ,NDO_DATA_EVENTHANDLERENABLED
			 ,temp_host->event_handler_enabled
			 ,NDO_DATA_ACTIVEHOSTCHECKSENABLED
			 ,temp_host->checks_enabled
			 ,NDO_DATA_FLAPDETECTIONENABLED
			 ,temp_host->flap_detection_enabled
			 ,NDO_DATA_ISFLAPPING
			 ,temp_host->is_flapping
			 ,NDO_DATA_PERCENTSTATECHANGE
			 ,temp_host->percent_state_change
			 ,NDO_DATA_LATENCY
			 ,temp_host->latency
			 ,NDO_DATA_EXECUTIONTIME
			 ,temp_host->execution_time
			 ,NDO_DATA_SCHEDULEDDOWNTIMEDEPTH
			 ,temp_host->scheduled_downtime_depth
			 ,NDO_DATA_FAILUREPREDICTIONENABLED
			 ,temp_host->failure_prediction_enabled
			 ,NDO_DATA_PROCESSPERFORMANCEDATA
			 ,temp_host->process_performance_data
			 ,NDO_DATA_OBSESSOVERHOST
			 ,temp_host->obsess_over_host

			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,temp_host->modified_attributes
			 ,NDO_DATA_EVENTHANDLER
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_CHECKCOMMAND
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_NORMALCHECKINTERVAL
			 ,(double)temp_host->check_interval
			 ,NDO_DATA_RETRYCHECKINTERVAL
			 ,(double)retry_interval
			 ,NDO_DATA_HOSTCHECKPERIOD
			 ,(es[5]==NULL)?"":es[5]
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

#ifdef BUILD_NAGIOS_3X
		/* dump customvars */
		for(temp_customvar=temp_host->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}

			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);

			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
		        }
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_SERVICE_STATUS_DATA:

		ssdata=(nebstruct_service_status_data *)data;

		if((temp_service=(service *)ssdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}

		es[0]=ndo_escape_buffer(temp_service->host_name);
		es[1]=ndo_escape_buffer(temp_service->description);
		es[2]=ndo_escape_buffer(temp_service->plugin_output);
		es[3]=ndo_escape_buffer(temp_service->perf_data);
		es[4]=ndo_escape_buffer(temp_service->event_handler);
		es[5]=ndo_escape_buffer(temp_service->service_check_command);
		es[6]=ndo_escape_buffer(temp_service->check_period);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%d\n%d=%lu\n%d=%lu\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%.5lf\n%d=%.5lf\n%d=%.5lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%s\n"
			 ,NDO_API_SERVICESTATUSDATA
			 ,NDO_DATA_TYPE
			 ,ssdata->type
			 ,NDO_DATA_FLAGS
			 ,ssdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,ssdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,ssdata->timestamp.tv_sec
			 ,ssdata->timestamp.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_OUTPUT
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_PERFDATA
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_CURRENTSTATE
			 ,temp_service->current_state
			 ,NDO_DATA_HASBEENCHECKED
			 ,temp_service->has_been_checked
			 ,NDO_DATA_SHOULDBESCHEDULED
			 ,temp_service->should_be_scheduled
			 ,NDO_DATA_CURRENTCHECKATTEMPT
			 ,temp_service->current_attempt
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,temp_service->max_attempts
			 ,NDO_DATA_LASTSERVICECHECK
			 ,(unsigned long)temp_service->last_check
			 ,NDO_DATA_NEXTSERVICECHECK
			 ,(unsigned long)temp_service->next_check
			 ,NDO_DATA_CHECKTYPE
			 ,temp_service->check_type
			 ,NDO_DATA_LASTSTATECHANGE
			 ,(unsigned long)temp_service->last_state_change
			 ,NDO_DATA_LASTHARDSTATECHANGE
			 ,(unsigned long)temp_service->last_hard_state_change
			 ,NDO_DATA_LASTHARDSTATE
			 ,temp_service->last_hard_state
			 ,NDO_DATA_LASTTIMEOK
			 ,(unsigned long)temp_service->last_time_ok
			 ,NDO_DATA_LASTTIMEWARNING
			 ,(unsigned long)temp_service->last_time_warning
			 ,NDO_DATA_LASTTIMEUNKNOWN
			 ,(unsigned long)temp_service->last_time_unknown
			 ,NDO_DATA_LASTTIMECRITICAL
			 ,(unsigned long)temp_service->last_time_critical
			 ,NDO_DATA_STATETYPE
			 ,temp_service->state_type
			 ,NDO_DATA_LASTSERVICENOTIFICATION
			 ,(unsigned long)temp_service->last_notification
			 ,NDO_DATA_NEXTSERVICENOTIFICATION
			 ,(unsigned long)temp_service->next_notification
			 ,NDO_DATA_NOMORENOTIFICATIONS
			 ,temp_service->no_more_notifications
			 ,NDO_DATA_NOTIFICATIONSENABLED
			 ,temp_service->notifications_enabled
			 ,NDO_DATA_PROBLEMHASBEENACKNOWLEDGED
			 ,temp_service->problem_has_been_acknowledged
			 ,NDO_DATA_ACKNOWLEDGEMENTTYPE
			 ,temp_service->acknowledgement_type
			 ,NDO_DATA_CURRENTNOTIFICATIONNUMBER
			 ,temp_service->current_notification_number
			 ,NDO_DATA_PASSIVESERVICECHECKSENABLED
			 ,temp_service->accept_passive_service_checks
			 ,NDO_DATA_EVENTHANDLERENABLED
			 ,temp_service->event_handler_enabled
			 ,NDO_DATA_ACTIVESERVICECHECKSENABLED
			 ,temp_service->checks_enabled
			 ,NDO_DATA_FLAPDETECTIONENABLED
			 ,temp_service->flap_detection_enabled
			 ,NDO_DATA_ISFLAPPING
			 ,temp_service->is_flapping
			 ,NDO_DATA_PERCENTSTATECHANGE
			 ,temp_service->percent_state_change
			 ,NDO_DATA_LATENCY
			 ,temp_service->latency
			 ,NDO_DATA_EXECUTIONTIME
			 ,temp_service->execution_time
			 ,NDO_DATA_SCHEDULEDDOWNTIMEDEPTH
			 ,temp_service->scheduled_downtime_depth
			 ,NDO_DATA_FAILUREPREDICTIONENABLED
			 ,temp_service->failure_prediction_enabled
			 ,NDO_DATA_PROCESSPERFORMANCEDATA
			 ,temp_service->process_performance_data
			 ,NDO_DATA_OBSESSOVERSERVICE
			 ,temp_service->obsess_over_service

			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,temp_service->modified_attributes
			 ,NDO_DATA_EVENTHANDLER
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_CHECKCOMMAND
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_NORMALCHECKINTERVAL
			 ,(double)temp_service->check_interval
			 ,NDO_DATA_RETRYCHECKINTERVAL
			 ,(double)temp_service->retry_interval
			 ,NDO_DATA_SERVICECHECKPERIOD
			 ,(es[6]==NULL)?"":es[6]
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

#ifdef BUILD_NAGIOS_3X
		/* dump customvars */
		for(temp_customvar=temp_service->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}

			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);

			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
		        }
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

#ifdef BUILD_NAGIOS_3X
	case NEBCALLBACK_CONTACT_STATUS_DATA:

		csdata=(nebstruct_contact_status_data *)data;

		if((temp_contact=(contact *)csdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}

		es[0]=ndo_escape_buffer(temp_contact->name);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%s\n%d=%d\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n"
			 ,NDO_API_CONTACTSTATUSDATA
			 ,NDO_DATA_TYPE
			 ,csdata->type
			 ,NDO_DATA_FLAGS
			 ,csdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,csdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,csdata->timestamp.tv_sec
			 ,csdata->timestamp.tv_usec
			 ,NDO_DATA_CONTACTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_HOSTNOTIFICATIONSENABLED
			 ,temp_contact->host_notifications_enabled
			 ,NDO_DATA_SERVICENOTIFICATIONSENABLED
			 ,temp_contact->service_notifications_enabled
			 ,NDO_DATA_LASTHOSTNOTIFICATION
			 ,temp_contact->last_host_notification
			 ,NDO_DATA_LASTSERVICENOTIFICATION
			 ,temp_contact->last_service_notification

			 ,NDO_DATA_MODIFIEDCONTACTATTRIBUTES
			 ,temp_contact->modified_attributes
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,temp_contact->modified_host_attributes
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,temp_contact->modified_service_attributes
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		/* dump customvars */
		for(temp_customvar=temp_contact->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}

			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);

			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
		        }

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;
#endif

	case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:

		apdata=(nebstruct_adaptive_program_data *)data;

		es[0]=ndo_escape_buffer(global_host_event_handler);
		es[1]=ndo_escape_buffer(global_service_event_handler);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_ADAPTIVEPROGRAMDATA
			 ,NDO_DATA_TYPE,apdata->type
			 ,NDO_DATA_FLAGS
			 ,apdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,apdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,apdata->timestamp.tv_sec
			 ,apdata->timestamp.tv_usec
			 ,NDO_DATA_COMMANDTYPE
			 ,apdata->command_type
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTE
			 ,apdata->modified_host_attribute
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,apdata->modified_host_attributes
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTE
			 ,apdata->modified_service_attribute
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,apdata->modified_service_attributes
			 ,NDO_DATA_GLOBALHOSTEVENTHANDLER
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_GLOBALSERVICEEVENTHANDLER
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_HOST_DATA:

		ahdata=(nebstruct_adaptive_host_data *)data;

		if((temp_host=(host *)ahdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}

#ifdef BUILD_NAGIOS_2X
		retry_interval=0.0;
#endif
#ifdef BUILD_NAGIOS_3X
		retry_interval=temp_host->retry_interval;
#endif

		es[0]=ndo_escape_buffer(temp_host->name);
		es[1]=ndo_escape_buffer(temp_host->event_handler);
		es[2]=ndo_escape_buffer(temp_host->host_check_command);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d\n\n"
			 ,NDO_API_ADAPTIVEHOSTDATA
			 ,NDO_DATA_TYPE
			 ,ahdata->type
			 ,NDO_DATA_FLAGS
			 ,ahdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,ahdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,ahdata->timestamp.tv_sec
			 ,ahdata->timestamp.tv_usec
			 ,NDO_DATA_COMMANDTYPE
			 ,ahdata->command_type
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTE
			 ,ahdata->modified_attribute
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,ahdata->modified_attributes
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_EVENTHANDLER
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_CHECKCOMMAND
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_NORMALCHECKINTERVAL
			 ,(double)temp_host->check_interval
			 ,NDO_DATA_RETRYCHECKINTERVAL
			 ,retry_interval
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,temp_host->max_attempts
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:

		asdata=(nebstruct_adaptive_service_data *)data;

		if((temp_service=(service *)asdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}

		es[0]=ndo_escape_buffer(temp_service->host_name);
		es[1]=ndo_escape_buffer(temp_service->description);
		es[2]=ndo_escape_buffer(temp_service->event_handler);
		es[3]=ndo_escape_buffer(temp_service->service_check_command);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d\n\n"
			 ,NDO_API_ADAPTIVESERVICEDATA
			 ,NDO_DATA_TYPE
			 ,asdata->type
			 ,NDO_DATA_FLAGS
			 ,asdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,asdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,asdata->timestamp.tv_sec
			 ,asdata->timestamp.tv_usec
			 ,NDO_DATA_COMMANDTYPE
			 ,asdata->command_type
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTE
			 ,asdata->modified_attribute
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,asdata->modified_attributes
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_EVENTHANDLER
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_CHECKCOMMAND
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_NORMALCHECKINTERVAL
			 ,(double)temp_service->check_interval
			 ,NDO_DATA_RETRYCHECKINTERVAL
			 ,(double)temp_service->retry_interval
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,temp_service->max_attempts
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

#ifdef BUILD_NAGIOS_3X
	case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:

		acdata=(nebstruct_adaptive_contact_data *)data;

		if((temp_contact=(contact *)acdata->object_ptr)==NULL){
			ndo_dbuf_free(&dbuf);
			return 0;
			}

		es[0]=ndo_escape_buffer(temp_contact->name);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%lu\n%d=%s\n%d=%d\n%d=%d\n%d\n\n"
			 ,NDO_API_ADAPTIVECONTACTDATA
			 ,NDO_DATA_TYPE
			 ,acdata->type
			 ,NDO_DATA_FLAGS
			 ,acdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,acdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,acdata->timestamp.tv_sec
			 ,acdata->timestamp.tv_usec
			 ,NDO_DATA_COMMANDTYPE
			 ,acdata->command_type
			 ,NDO_DATA_MODIFIEDCONTACTATTRIBUTE
			 ,acdata->modified_attribute
			 ,NDO_DATA_MODIFIEDCONTACTATTRIBUTES
			 ,acdata->modified_attributes
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTE
			 ,acdata->modified_host_attribute
			 ,NDO_DATA_MODIFIEDHOSTATTRIBUTES
			 ,acdata->modified_host_attributes
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTE
			 ,acdata->modified_service_attribute
			 ,NDO_DATA_MODIFIEDSERVICEATTRIBUTES
			 ,acdata->modified_service_attributes
			 ,NDO_DATA_CONTACTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_HOSTNOTIFICATIONSENABLED
			 ,temp_contact->host_notifications_enabled
			 ,NDO_DATA_SERVICENOTIFICATIONSENABLED
			 ,temp_contact->service_notifications_enabled
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;
#endif

	case NEBCALLBACK_EXTERNAL_COMMAND_DATA:

		ecdata=(nebstruct_external_command_data *)data;

		es[0]=ndo_escape_buffer(ecdata->command_string);
		es[1]=ndo_escape_buffer(ecdata->command_args);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%lu\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_EXTERNALCOMMANDDATA
			 ,NDO_DATA_TYPE
			 ,ecdata->type
			 ,NDO_DATA_FLAGS
			 ,ecdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,ecdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,ecdata->timestamp.tv_sec
			 ,ecdata->timestamp.tv_usec
			 ,NDO_DATA_COMMANDTYPE
			 ,ecdata->command_type
			 ,NDO_DATA_ENTRYTIME
			 ,(unsigned long)ecdata->entry_time
			 ,NDO_DATA_COMMANDSTRING
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_COMMANDARGS
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_AGGREGATED_STATUS_DATA:

		agsdata=(nebstruct_aggregated_status_data *)data;

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d\n\n"
			 ,NDO_API_AGGREGATEDSTATUSDATA
			 ,NDO_DATA_TYPE
			 ,agsdata->type
			 ,NDO_DATA_FLAGS
			 ,agsdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,agsdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,agsdata->timestamp.tv_sec
			 ,agsdata->timestamp.tv_usec
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_RETENTION_DATA:

		rdata=(nebstruct_retention_data *)data;

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d\n\n"
			 ,NDO_API_RETENTIONDATA
			 ,NDO_DATA_TYPE
			 ,rdata->type
			 ,NDO_DATA_FLAGS
			 ,rdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,rdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,rdata->timestamp.tv_sec
			 ,rdata->timestamp.tv_usec
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:

		cnotdata=(nebstruct_contact_notification_data *)data;

		es[0]=ndo_escape_buffer(cnotdata->host_name);
		es[1]=ndo_escape_buffer(cnotdata->service_description);
		es[2]=ndo_escape_buffer(cnotdata->output);
		es[3]=ndo_escape_buffer(cnotdata->ack_author);
		es[4]=ndo_escape_buffer(cnotdata->ack_data);
		es[5]=ndo_escape_buffer(cnotdata->contact_name);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_CONTACTNOTIFICATIONDATA
			 ,NDO_DATA_TYPE
			 ,cnotdata->type
			 ,NDO_DATA_FLAGS
			 ,cnotdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,cnotdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,cnotdata->timestamp.tv_sec
			 ,cnotdata->timestamp.tv_usec
			 ,NDO_DATA_NOTIFICATIONTYPE
			 ,cnotdata->notification_type
			 ,NDO_DATA_STARTTIME
			 ,cnotdata->start_time.tv_sec
			 ,cnotdata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,cnotdata->end_time.tv_sec
			 ,cnotdata->end_time.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_CONTACTNAME
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_NOTIFICATIONREASON
			 ,cnotdata->reason_type
			 ,NDO_DATA_STATE
			 ,cnotdata->state
			 ,NDO_DATA_OUTPUT
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_ACKAUTHOR
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_ACKDATA
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:

		cnotmdata=(nebstruct_contact_notification_method_data *)data;

		es[0]=ndo_escape_buffer(cnotmdata->host_name);
		es[1]=ndo_escape_buffer(cnotmdata->service_description);
		es[2]=ndo_escape_buffer(cnotmdata->output);
		es[3]=ndo_escape_buffer(cnotmdata->ack_author);
		es[4]=ndo_escape_buffer(cnotmdata->ack_data);
		es[5]=ndo_escape_buffer(cnotmdata->contact_name);
		es[6]=ndo_escape_buffer(cnotmdata->command_name);
		es[7]=ndo_escape_buffer(cnotmdata->command_args);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%ld.%ld\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_CONTACTNOTIFICATIONMETHODDATA
			 ,NDO_DATA_TYPE
			 ,cnotmdata->type
			 ,NDO_DATA_FLAGS
			 ,cnotmdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,cnotmdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,cnotmdata->timestamp.tv_sec
			 ,cnotmdata->timestamp.tv_usec
			 ,NDO_DATA_NOTIFICATIONTYPE
			 ,cnotmdata->notification_type
			 ,NDO_DATA_STARTTIME
			 ,cnotmdata->start_time.tv_sec
			 ,cnotmdata->start_time.tv_usec
			 ,NDO_DATA_ENDTIME
			 ,cnotmdata->end_time.tv_sec
			 ,cnotmdata->end_time.tv_usec
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_CONTACTNAME
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_COMMANDNAME
			 ,(es[6]==NULL)?"":es[6]
			 ,NDO_DATA_COMMANDARGS
			 ,(es[7]==NULL)?"":es[7]
			 ,NDO_DATA_NOTIFICATIONREASON
			 ,cnotmdata->reason_type
			 ,NDO_DATA_STATE
			 ,cnotmdata->state
			 ,NDO_DATA_OUTPUT
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_ACKAUTHOR
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_ACKDATA
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:

		ackdata=(nebstruct_acknowledgement_data *)data;

		es[0]=ndo_escape_buffer(ackdata->host_name);
		es[1]=ndo_escape_buffer(ackdata->service_description);
		es[2]=ndo_escape_buffer(ackdata->author_name);
		es[3]=ndo_escape_buffer(ackdata->comment_data);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d\n\n"
			 ,NDO_API_ACKNOWLEDGEMENTDATA
			 ,NDO_DATA_TYPE
			 ,ackdata->type
			 ,NDO_DATA_FLAGS
			 ,ackdata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,ackdata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,ackdata->timestamp.tv_sec
			 ,ackdata->timestamp.tv_usec
			 ,NDO_DATA_ACKNOWLEDGEMENTTYPE
			 ,ackdata->acknowledgement_type
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_AUTHORNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_COMMENT
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_STATE
			 ,ackdata->state
			 ,NDO_DATA_STICKY
			 ,ackdata->is_sticky
			 ,NDO_DATA_PERSISTENT
			 ,ackdata->persistent_comment
			 ,NDO_DATA_NOTIFYCONTACTS
			 ,ackdata->notify_contacts
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	case NEBCALLBACK_STATE_CHANGE_DATA:

		schangedata=(nebstruct_statechange_data *)data;

#ifdef BUILD_NAGIOS_2X
		/* find host/service and get last state info */
		if(schangedata->service_description==NULL){
			if((temp_host=find_host(schangedata->host_name))==NULL){
				ndo_dbuf_free(&dbuf);
				return 0;
				}
			}
		else{
			if((temp_service=find_service(schangedata->host_name,schangedata->service_description))==NULL){
				ndo_dbuf_free(&dbuf);
				return 0;
				}
			last_state=temp_service->last_state;
			last_hard_state=temp_service->last_hard_state;
			}
#else
		/* get the last state info */
		if(schangedata->service_description==NULL){
			if((temp_host=(host *)schangedata->object_ptr)==NULL){
				ndo_dbuf_free(&dbuf);
				return 0;
				}
			last_state=temp_host->last_state;
			last_state=temp_host->last_hard_state;
			}
		else{
			if((temp_service=(service *)schangedata->object_ptr)==NULL){
				ndo_dbuf_free(&dbuf);
				return 0;
				}
			last_state=temp_service->last_state;
			last_hard_state=temp_service->last_hard_state;
			}
#endif

		es[0]=ndo_escape_buffer(schangedata->host_name);
		es[1]=ndo_escape_buffer(schangedata->service_description);
		es[2]=ndo_escape_buffer(schangedata->output);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%d\n%d=%d\n%d=%d\n%d=%ld.%ld\n%d=%d\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d\n\n"
			 ,NDO_API_STATECHANGEDATA
			 ,NDO_DATA_TYPE
			 ,schangedata->type
			 ,NDO_DATA_FLAGS
			 ,schangedata->flags
			 ,NDO_DATA_ATTRIBUTES
			 ,schangedata->attr
			 ,NDO_DATA_TIMESTAMP
			 ,schangedata->timestamp.tv_sec
			 ,schangedata->timestamp.tv_usec
			 ,NDO_DATA_STATECHANGETYPE
			 ,schangedata->statechange_type
			 ,NDO_DATA_HOST
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_STATECHANGE
			 ,TRUE
			 ,NDO_DATA_STATE
			 ,schangedata->state
			 ,NDO_DATA_STATETYPE
			 ,schangedata->state_type
			 ,NDO_DATA_CURRENTCHECKATTEMPT
			 ,schangedata->current_attempt
			 ,NDO_DATA_MAXCHECKATTEMPTS
			 ,schangedata->max_attempts
			 ,NDO_DATA_LASTSTATE
			 ,last_state
			 ,NDO_DATA_LASTHARDSTATE
			 ,last_hard_state
			 ,NDO_DATA_OUTPUT
			 ,es[2]
			 ,NDO_API_ENDDATA
			);

		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		break;

	default:
		ndo_dbuf_free(&dbuf);
		return 0;
		break;
	        }

	/* free escaped buffers */
	for(x=0;x<8;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/* write data to sink */
	if(write_to_sink==NDO_TRUE)
		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

	/* free dynamic buffer */
	ndo_dbuf_free(&dbuf);



	/* POST PROCESSING... */

	switch(event_type){

	case NEBCALLBACK_PROCESS_DATA:

		procdata=(nebstruct_process_data *)data;

		/* process has passed pre-launch config verification, so dump original config */
		if(procdata->type==NEBTYPE_PROCESS_START){
			ndomod_write_config_files();
			ndomod_write_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
		        }

		/* process is starting the event loop, so dump runtime vars */
		if(procdata->type==NEBTYPE_PROCESS_EVENTLOOPSTART){
			ndomod_write_runtime_variables();
		        }

		break;

	case NEBCALLBACK_RETENTION_DATA:

		rdata=(nebstruct_retention_data *)data;

		/* retained config was just read, so dump it */
		if(rdata->type==NEBTYPE_RETENTIONDATA_ENDLOAD)
			ndomod_write_config(NDOMOD_CONFIG_DUMP_RETAINED);

		break;

	default:
		break;
	        }

	return 0;
        }



/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* dumps all configuration data to sink */
int ndomod_write_config(int config_type){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	struct timeval now;
	int result;

	if(!(ndomod_config_output_options & config_type))
		return NDO_OK;

	gettimeofday(&now,NULL);

	/* record start of config dump */
	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%d:\n%d=%s\n%d=%ld.%ld\n%d\n\n"
		 ,NDO_API_STARTCONFIGDUMP
		 ,NDO_DATA_CONFIGDUMPTYPE
		 ,(config_type==NDOMOD_CONFIG_DUMP_ORIGINAL)?NDO_API_CONFIGDUMP_ORIGINAL:NDO_API_CONFIGDUMP_RETAINED
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_API_ENDDATA
		);
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);

	/* dump object config info */
	result=ndomod_write_object_config(config_type);
	if(result!=NDO_OK)
		return result;

	/* record end of config dump */
	snprintf(temp_buffer,sizeof(temp_buffer)-1
		 ,"\n\n%d:\n%d=%ld.%ld\n%d\n\n"
		 ,NDO_API_ENDCONFIGDUMP
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_API_ENDDATA
		);
	temp_buffer[sizeof(temp_buffer)-1]='\x0';
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);

	return result;
        }


#define OBJECTCONFIG_ES_ITEMS 15

/* dumps object configuration data to sink */
int ndomod_write_object_config(int config_type){
	char temp_buffer[NDOMOD_MAX_BUFLEN];
	ndo_dbuf dbuf;
	struct timeval now;
	int x=0;
	char *es[OBJECTCONFIG_ES_ITEMS];
	command *temp_command=NULL;
	timeperiod *temp_timeperiod=NULL;
	timerange *temp_timerange=NULL;
	contact *temp_contact=NULL;
	commandsmember *temp_commandsmember=NULL;
	contactgroup *temp_contactgroup=NULL;
	host *temp_host=NULL;
	hostsmember *temp_hostsmember=NULL;
	contactgroupsmember *temp_contactgroupsmember=NULL;
	hostgroup *temp_hostgroup=NULL;
	service *temp_service=NULL;
	servicegroup *temp_servicegroup=NULL;
	hostescalation *temp_hostescalation=NULL;
	serviceescalation *temp_serviceescalation=NULL;
	hostdependency *temp_hostdependency=NULL;
	servicedependency *temp_servicedependency=NULL;
#ifdef BUILD_NAGIOS_2X
	hostextinfo *temp_hostextinfo=NULL;
	serviceextinfo *temp_serviceextinfo=NULL;
	contactgroupmember *temp_contactgroupmember=NULL;
	hostgroupmember *temp_hostgroupmember=NULL;
	servicegroupmember *temp_servicegroupmember=NULL;
#endif
	int have_2d_coords=FALSE;
	int x_2d=0;
	int y_2d=0;
	int have_3d_coords=FALSE;
	double x_3d=0.0;
	double y_3d=0.0;
	double z_3d=0.0;
	double first_notification_delay=0.0;
	double retry_interval=0.0;
	int notify_on_host_downtime=0;
	int notify_on_service_downtime=0;
	int host_notifications_enabled=0;
	int service_notifications_enabled=0;
	int can_submit_commands=0;
	int flap_detection_on_up=0;
	int flap_detection_on_down=0;
	int flap_detection_on_unreachable=0;
	int flap_detection_on_ok=0;
	int flap_detection_on_warning=0;
	int flap_detection_on_unknown=0;
	int flap_detection_on_critical=0;
#ifdef BUILD_NAGIOS_3X
	customvariablesmember *temp_customvar=NULL;
	contactsmember *temp_contactsmember=NULL;
	servicesmember *temp_servicesmember=NULL;
#endif


	if(!(ndomod_process_options & NDOMOD_PROCESS_OBJECT_CONFIG_DATA))
		return NDO_OK;

	if(!(ndomod_config_output_options & config_type))
		return NDO_OK;

	/* get current time */
	gettimeofday(&now,NULL);

	/* initialize dynamic buffer (2KB chunk size) */
	ndo_dbuf_init(&dbuf,2048);

	/* initialize buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++)
		es[x]=NULL;

	/****** dump command config ******/
	for(temp_command=command_list;temp_command!=NULL;temp_command=temp_command->next){

		es[0]=ndo_escape_buffer(temp_command->name);
		es[1]=ndo_escape_buffer(temp_command->command_line);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d\n\n"
			 ,NDO_API_COMMANDDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_COMMANDNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_COMMANDLINE
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_API_ENDDATA
			);

		/* write data to sink */
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	        }

	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump timeperiod config ******/
	for(temp_timeperiod=timeperiod_list;temp_timeperiod!=NULL;temp_timeperiod=temp_timeperiod->next){

		es[0]=ndo_escape_buffer(temp_timeperiod->name);
		es[1]=ndo_escape_buffer(temp_timeperiod->alias);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
			 ,NDO_API_TIMEPERIODDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_TIMEPERIODNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_TIMEPERIODALIAS
			 ,(es[1]==NULL)?"":es[1]
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		/* dump timeranges for each day */
		for(x=0;x<7;x++){
			for(temp_timerange=temp_timeperiod->days[x];temp_timerange!=NULL;temp_timerange=temp_timerange->next){
				
				snprintf(temp_buffer,sizeof(temp_buffer)-1
					 ,"%d=%d:%lu-%lu\n"
					 ,NDO_DATA_TIMERANGE
					 ,x
					 ,temp_timerange->range_start
					 ,temp_timerange->range_end
					);
				temp_buffer[sizeof(temp_buffer)-1]='\x0';
				ndo_dbuf_strcat(&dbuf,temp_buffer);
			        }
		        }

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump contact config ******/
	for(temp_contact=contact_list;temp_contact!=NULL;temp_contact=temp_contact->next){

		es[0]=ndo_escape_buffer(temp_contact->name);
		es[1]=ndo_escape_buffer(temp_contact->alias);
		es[2]=ndo_escape_buffer(temp_contact->email);
		es[3]=ndo_escape_buffer(temp_contact->pager);
		es[4]=ndo_escape_buffer(temp_contact->host_notification_period);
		es[5]=ndo_escape_buffer(temp_contact->service_notification_period);

#ifdef BUILD_NAGIOS_3X
		notify_on_service_downtime=temp_contact->notify_on_service_downtime;
		notify_on_host_downtime=temp_contact->notify_on_host_downtime;
		host_notifications_enabled=temp_contact->host_notifications_enabled;
		service_notifications_enabled=temp_contact->service_notifications_enabled;
		can_submit_commands=temp_contact->can_submit_commands;
#endif
#ifdef BUILD_NAGIOS_2X
		notify_on_service_downtime=0;
		notify_on_host_downtime=0;
		host_notifications_enabled=1;
		service_notifications_enabled=1;
		can_submit_commands=1;
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
			 ,NDO_API_CONTACTDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_CONTACTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_CONTACTALIAS
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_EMAILADDRESS
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_PAGERADDRESS
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_HOSTNOTIFICATIONPERIOD
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_SERVICENOTIFICATIONPERIOD
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_SERVICENOTIFICATIONSENABLED
			 ,service_notifications_enabled
			 ,NDO_DATA_HOSTNOTIFICATIONSENABLED
			 ,host_notifications_enabled
			 ,NDO_DATA_CANSUBMITCOMMANDS
			 ,can_submit_commands
			 ,NDO_DATA_NOTIFYSERVICEUNKNOWN
			 ,temp_contact->notify_on_service_unknown
			 ,NDO_DATA_NOTIFYSERVICEWARNING
			 ,temp_contact->notify_on_service_warning
			 ,NDO_DATA_NOTIFYSERVICECRITICAL
			 ,temp_contact->notify_on_service_critical
			 ,NDO_DATA_NOTIFYSERVICERECOVERY
			 ,temp_contact->notify_on_service_recovery
			 ,NDO_DATA_NOTIFYSERVICEFLAPPING
			 ,temp_contact->notify_on_service_flapping
			 ,NDO_DATA_NOTIFYSERVICEDOWNTIME
			 ,notify_on_service_downtime
			 ,NDO_DATA_NOTIFYHOSTDOWN
			 ,temp_contact->notify_on_host_down
			 ,NDO_DATA_NOTIFYHOSTUNREACHABLE
			 ,temp_contact->notify_on_host_unreachable
			 ,NDO_DATA_NOTIFYHOSTRECOVERY
			 ,temp_contact->notify_on_host_recovery
			 ,NDO_DATA_NOTIFYHOSTFLAPPING
			 ,temp_contact->notify_on_host_flapping
			 ,NDO_DATA_NOTIFYHOSTDOWNTIME
			 ,notify_on_host_downtime
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;
	
		/* dump addresses for each contact */
		for(x=0;x<MAX_CONTACT_ADDRESSES;x++){
				
			es[0]=ndo_escape_buffer(temp_contact->address[x]);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%d:%s\n"
				 ,NDO_DATA_CONTACTADDRESS
				 ,x+1
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump host notification commands for each contact */
		for(temp_commandsmember=temp_contact->host_notification_commands;temp_commandsmember!=NULL;temp_commandsmember=temp_commandsmember->next){

			es[0]=ndo_escape_buffer(temp_commandsmember->command);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_HOSTNOTIFICATIONCOMMAND
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump service notification commands for each contact */
		for(temp_commandsmember=temp_contact->service_notification_commands;temp_commandsmember!=NULL;temp_commandsmember=temp_commandsmember->next){

			es[0]=ndo_escape_buffer(temp_commandsmember->command);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_SERVICENOTIFICATIONCOMMAND
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

#ifdef BUILD_NAGIOS_3X
		/* dump customvars */
		for(temp_customvar=temp_contact->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}
		        }
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump contactgroup config ******/
	for(temp_contactgroup=contactgroup_list;temp_contactgroup!=NULL;temp_contactgroup=temp_contactgroup->next){

		es[0]=ndo_escape_buffer(temp_contactgroup->group_name);
		es[1]=ndo_escape_buffer(temp_contactgroup->alias);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
			 ,NDO_API_CONTACTGROUPDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_CONTACTGROUPNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_CONTACTGROUPALIAS
			 ,(es[1]==NULL)?"":es[1]
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;
	
		/* dump members for each contactgroup */
#ifdef BUILD_NAGIOS_2X
		for(temp_contactgroupmember=temp_contactgroup->members;temp_contactgroupmember!=NULL;temp_contactgroupmember=temp_contactgroupmember->next)
#else
		for(temp_contactsmember=temp_contactgroup->members;temp_contactsmember!=NULL;temp_contactsmember=temp_contactsmember->next)
#endif
			{

#ifdef BUILD_NAGIOS_2X				
			es[0]=ndo_escape_buffer(temp_contactgroupmember->contact_name);
#else
			es[0]=ndo_escape_buffer(temp_contactsmember->contact_name);
#endif

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACTGROUPMEMBER
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump host config ******/
	for(temp_host=host_list;temp_host!=NULL;temp_host=temp_host->next){

		es[0]=ndo_escape_buffer(temp_host->name);
		es[1]=ndo_escape_buffer(temp_host->alias);
		es[2]=ndo_escape_buffer(temp_host->address);
		es[3]=ndo_escape_buffer(temp_host->host_check_command);
		es[4]=ndo_escape_buffer(temp_host->event_handler);
		es[5]=ndo_escape_buffer(temp_host->notification_period);
		es[6]=ndo_escape_buffer(temp_host->check_period);
		es[7]=ndo_escape_buffer(temp_host->failure_prediction_options);

#ifdef BUILD_NAGIOS_3X
		es[7]=ndo_escape_buffer(temp_host->notes);
		es[8]=ndo_escape_buffer(temp_host->notes_url);
		es[9]=ndo_escape_buffer(temp_host->action_url);
		es[10]=ndo_escape_buffer(temp_host->icon_image);
		es[11]=ndo_escape_buffer(temp_host->icon_image_alt);
		es[12]=ndo_escape_buffer(temp_host->vrml_image);
		es[13]=ndo_escape_buffer(temp_host->statusmap_image);
		have_2d_coords=temp_host->have_2d_coords;
		x_2d=temp_host->x_2d;
		y_2d=temp_host->y_2d;
		have_3d_coords=temp_host->have_3d_coords;
		x_3d=temp_host->x_3d;
		y_3d=temp_host->y_3d;
		z_3d=temp_host->z_3d;

		first_notification_delay=temp_host->first_notification_delay;
		retry_interval=temp_host->retry_interval;
		notify_on_host_downtime=temp_host->notify_on_downtime;
		flap_detection_on_up=temp_host->flap_detection_on_up;
		flap_detection_on_down=temp_host->flap_detection_on_down;
		flap_detection_on_unreachable=temp_host->flap_detection_on_unreachable;
		es[14]=ndo_escape_buffer(temp_host->display_name);
#endif
#ifdef BUILD_NAGIOS_2X
		if((temp_hostextinfo=find_hostextinfo(temp_host->name))!=NULL){
			es[7]=ndo_escape_buffer(temp_hostextinfo->notes);
			es[8]=ndo_escape_buffer(temp_hostextinfo->notes_url);
			es[9]=ndo_escape_buffer(temp_hostextinfo->action_url);
			es[10]=ndo_escape_buffer(temp_hostextinfo->icon_image);
			es[11]=ndo_escape_buffer(temp_hostextinfo->icon_image_alt);
			es[12]=ndo_escape_buffer(temp_hostextinfo->vrml_image);
			es[13]=ndo_escape_buffer(temp_hostextinfo->statusmap_image);
			have_2d_coords=temp_hostextinfo->have_2d_coords;
			x_2d=temp_hostextinfo->x_2d;
			y_2d=temp_hostextinfo->y_2d;
			have_3d_coords=temp_hostextinfo->have_3d_coords;
			x_3d=temp_hostextinfo->x_3d;
			y_3d=temp_hostextinfo->y_3d;
			z_3d=temp_hostextinfo->z_3d;
			}
		else{
			es[7]=NULL;
			es[8]=NULL;
			es[9]=NULL;
			es[10]=NULL;
			es[11]=NULL;
			es[12]=NULL;
			es[13]=NULL;
			have_2d_coords=FALSE;
			x_2d=0;
			y_2d=0;
			have_3d_coords=FALSE;
			x_3d=0.0;
			y_3d=0.0;
			z_3d=0.0;
			}

		first_notification_delay=0.0;
		retry_interval=0.0;
		notify_on_host_downtime=0;
		flap_detection_on_up=1;
		flap_detection_on_down=1;
		flap_detection_on_unreachable=1;
		es[14]=ndo_escape_buffer(temp_host->name);
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%lf\n"
			 ,NDO_API_HOSTDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_DISPLAYNAME
			 ,(es[14]==NULL)?"":es[14]
			 ,NDO_DATA_HOSTALIAS
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_HOSTADDRESS
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_HOSTCHECKCOMMAND
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_HOSTEVENTHANDLER
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_HOSTNOTIFICATIONPERIOD
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_HOSTCHECKPERIOD
			 ,(es[6]==NULL)?"":es[6]
			 ,NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS
			 ,(es[7]==NULL)?"":es[7]
			 ,NDO_DATA_HOSTCHECKINTERVAL
			 ,(double)temp_host->check_interval
			 ,NDO_DATA_HOSTRETRYINTERVAL
			 ,(double)retry_interval
			 ,NDO_DATA_HOSTMAXCHECKATTEMPTS
			 ,temp_host->max_attempts
			 ,NDO_DATA_FIRSTNOTIFICATIONDELAY
			 ,first_notification_delay
			 ,NDO_DATA_HOSTNOTIFICATIONINTERVAL
			 ,(double)temp_host->notification_interval
			 ,NDO_DATA_NOTIFYHOSTDOWN
			 ,temp_host->notify_on_down
			 ,NDO_DATA_NOTIFYHOSTUNREACHABLE
			 ,temp_host->notify_on_unreachable
			 ,NDO_DATA_NOTIFYHOSTRECOVERY
			 ,temp_host->notify_on_recovery
			 ,NDO_DATA_NOTIFYHOSTFLAPPING
			 ,temp_host->notify_on_flapping
			 ,NDO_DATA_NOTIFYHOSTDOWNTIME
			 ,notify_on_host_downtime
			 ,NDO_DATA_HOSTFLAPDETECTIONENABLED
			 ,temp_host->flap_detection_enabled
			 ,NDO_DATA_FLAPDETECTIONONUP
			 ,flap_detection_on_up
			 ,NDO_DATA_FLAPDETECTIONONDOWN
			 ,flap_detection_on_down
			 ,NDO_DATA_FLAPDETECTIONONUNREACHABLE
			 ,flap_detection_on_unreachable
			 ,NDO_DATA_LOWHOSTFLAPTHRESHOLD
			 ,temp_host->low_flap_threshold
			 ,NDO_DATA_HIGHHOSTFLAPTHRESHOLD
			 ,temp_host->high_flap_threshold
			 ,NDO_DATA_STALKHOSTONUP
			 ,temp_host->stalk_on_up
			 ,NDO_DATA_STALKHOSTONDOWN
			 ,temp_host->stalk_on_down
			 ,NDO_DATA_STALKHOSTONUNREACHABLE
			 ,temp_host->stalk_on_unreachable
			 ,NDO_DATA_HOSTFRESHNESSCHECKSENABLED
			 ,temp_host->check_freshness
			 ,NDO_DATA_HOSTFRESHNESSTHRESHOLD
			 ,temp_host->freshness_threshold
			 ,NDO_DATA_PROCESSHOSTPERFORMANCEDATA
			 ,temp_host->process_performance_data
			 ,NDO_DATA_ACTIVEHOSTCHECKSENABLED
			 ,temp_host->checks_enabled
			 ,NDO_DATA_PASSIVEHOSTCHECKSENABLED
			 ,temp_host->accept_passive_host_checks
			 ,NDO_DATA_HOSTEVENTHANDLERENABLED
			 ,temp_host->event_handler_enabled
			 ,NDO_DATA_RETAINHOSTSTATUSINFORMATION
			 ,temp_host->retain_status_information
			 ,NDO_DATA_RETAINHOSTNONSTATUSINFORMATION
			 ,temp_host->retain_nonstatus_information
			 ,NDO_DATA_HOSTNOTIFICATIONSENABLED
			 ,temp_host->notifications_enabled
			 ,NDO_DATA_HOSTFAILUREPREDICTIONENABLED
			 ,temp_host->failure_prediction_enabled
			 ,NDO_DATA_OBSESSOVERHOST
			 ,temp_host->obsess_over_host
			 ,NDO_DATA_NOTES
			 ,(es[7]==NULL)?"":es[7]
			 ,NDO_DATA_NOTESURL
			 ,(es[8]==NULL)?"":es[8]
			 ,NDO_DATA_ACTIONURL
			 ,(es[9]==NULL)?"":es[9]
			 ,NDO_DATA_ICONIMAGE
			 ,(es[10]==NULL)?"":es[10]
			 ,NDO_DATA_ICONIMAGEALT
			 ,(es[11]==NULL)?"":es[11]
			 ,NDO_DATA_VRMLIMAGE
			 ,(es[12]==NULL)?"":es[12]
			 ,NDO_DATA_STATUSMAPIMAGE
			 ,(es[13]==NULL)?"":es[13]
			 ,NDO_DATA_HAVE2DCOORDS
			 ,have_2d_coords
			 ,NDO_DATA_X2D
			 ,x_2d
			 ,NDO_DATA_Y2D
			 ,y_2d
			 ,NDO_DATA_HAVE3DCOORDS
			 ,have_3d_coords
			 ,NDO_DATA_X3D
			 ,x_3d
			 ,NDO_DATA_Y3D
			 ,y_3d
			 ,NDO_DATA_Z3D
			 ,z_3d
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;

		/* dump parent hosts */
		for(temp_hostsmember=temp_host->parent_hosts;temp_hostsmember!=NULL;temp_hostsmember=temp_hostsmember->next){
				
			es[0]=ndo_escape_buffer(temp_hostsmember->host_name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_PARENTHOST
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump contactgroups */
		for(temp_contactgroupsmember=temp_host->contact_groups;temp_contactgroupsmember!=NULL;temp_contactgroupsmember=temp_contactgroupsmember->next){
				
			es[0]=ndo_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACTGROUP
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		for(temp_contactsmember=temp_host->contacts;temp_contactsmember!=NULL;temp_contactsmember=temp_contactsmember->next){

			es[0]=ndo_escape_buffer(temp_contactsmember->contact_name);
				
			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACT
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
				
			free(es[0]);
			es[0]=NULL;
			}
#endif


#ifdef BUILD_NAGIOS_3X
		/* dump customvars */
		for(temp_customvar=temp_host->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}
		        }
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump hostgroup config ******/
	for(temp_hostgroup=hostgroup_list;temp_hostgroup!=NULL;temp_hostgroup=temp_hostgroup->next){

		es[0]=ndo_escape_buffer(temp_hostgroup->group_name);
		es[1]=ndo_escape_buffer(temp_hostgroup->alias);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
			 ,NDO_API_HOSTGROUPDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTGROUPNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_HOSTGROUPALIAS
			 ,(es[1]==NULL)?"":es[1]
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;
	
		/* dump members for each hostgroup */
#ifdef BUILD_NAGIOS_2X
		for(temp_hostgroupmember=temp_hostgroup->members;temp_hostgroupmember!=NULL;temp_hostgroupmember=temp_hostgroupmember->next)
#else
		for(temp_hostsmember=temp_hostgroup->members;temp_hostsmember!=NULL;temp_hostsmember=temp_hostsmember->next)
#endif
			{

#ifdef BUILD_NAGIOS_2X
			es[0]=ndo_escape_buffer(temp_hostgroupmember->host_name);
#else
			es[0]=ndo_escape_buffer(temp_hostsmember->host_name);
#endif

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_HOSTGROUPMEMBER
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump service config ******/
	for(temp_service=service_list;temp_service!=NULL;temp_service=temp_service->next){

		es[0]=ndo_escape_buffer(temp_service->host_name);
		es[1]=ndo_escape_buffer(temp_service->description);
		es[2]=ndo_escape_buffer(temp_service->service_check_command);
		es[3]=ndo_escape_buffer(temp_service->event_handler);
		es[4]=ndo_escape_buffer(temp_service->notification_period);
		es[5]=ndo_escape_buffer(temp_service->check_period);
		es[6]=ndo_escape_buffer(temp_service->failure_prediction_options);
#ifdef BUILD_NAGIOS_3X
		es[7]=ndo_escape_buffer(temp_service->notes);
		es[8]=ndo_escape_buffer(temp_service->notes_url);
		es[9]=ndo_escape_buffer(temp_service->action_url);
		es[10]=ndo_escape_buffer(temp_service->icon_image);
		es[11]=ndo_escape_buffer(temp_service->icon_image_alt);

		first_notification_delay=temp_service->first_notification_delay;
		notify_on_service_downtime=temp_service->notify_on_downtime;
		flap_detection_on_ok=temp_service->flap_detection_on_ok;
		flap_detection_on_warning=temp_service->flap_detection_on_warning;
		flap_detection_on_unknown=temp_service->flap_detection_on_unknown;
		flap_detection_on_critical=temp_service->flap_detection_on_critical;
		es[12]=ndo_escape_buffer(temp_service->display_name);
#endif
#ifdef BUILD_NAGIOS_2X
		if((temp_serviceextinfo=find_serviceextinfo(temp_service->host_name,temp_service->description))!=NULL){
			es[7]=ndo_escape_buffer(temp_serviceextinfo->notes);
			es[8]=ndo_escape_buffer(temp_serviceextinfo->notes_url);
			es[9]=ndo_escape_buffer(temp_serviceextinfo->action_url);
			es[10]=ndo_escape_buffer(temp_serviceextinfo->icon_image);
			es[11]=ndo_escape_buffer(temp_serviceextinfo->icon_image_alt);
			}
		else{
			es[7]=NULL;
			es[8]=NULL;
			es[9]=NULL;
			es[10]=NULL;
			es[11]=NULL;
			}

		first_notification_delay=0.0;
		notify_on_service_downtime=0;
		flap_detection_on_ok=1;
		flap_detection_on_warning=1;
		flap_detection_on_unknown=1;
		flap_detection_on_critical=1;
		es[12]=ndo_escape_buffer(temp_service->description);
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%lf\n%d=%lf\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%lf\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n"
			 ,NDO_API_SERVICEDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_DISPLAYNAME
			 ,(es[12]==NULL)?"":es[12]
			 ,NDO_DATA_SERVICEDESCRIPTION
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_SERVICECHECKCOMMAND
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_SERVICEEVENTHANDLER
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_SERVICENOTIFICATIONPERIOD
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_SERVICECHECKPERIOD
			 ,(es[5]==NULL)?"":es[5]
			 ,NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS
			 ,(es[6]==NULL)?"":es[6]
			 ,NDO_DATA_SERVICECHECKINTERVAL
			 ,(double)temp_service->check_interval
			 ,NDO_DATA_SERVICERETRYINTERVAL
			 ,(double)temp_service->retry_interval
			 ,NDO_DATA_MAXSERVICECHECKATTEMPTS
			 ,temp_service->max_attempts
			 ,NDO_DATA_FIRSTNOTIFICATIONDELAY
			 ,first_notification_delay
			 ,NDO_DATA_SERVICENOTIFICATIONINTERVAL
			 ,(double)temp_service->notification_interval
			 ,NDO_DATA_NOTIFYSERVICEUNKNOWN
			 ,temp_service->notify_on_unknown
			 ,NDO_DATA_NOTIFYSERVICEWARNING
			 ,temp_service->notify_on_warning
			 ,NDO_DATA_NOTIFYSERVICECRITICAL
			 ,temp_service->notify_on_critical
			 ,NDO_DATA_NOTIFYSERVICERECOVERY
			 ,temp_service->notify_on_recovery
			 ,NDO_DATA_NOTIFYSERVICEFLAPPING
			 ,temp_service->notify_on_flapping
			 ,NDO_DATA_NOTIFYSERVICEDOWNTIME
			 ,notify_on_service_downtime
			 ,NDO_DATA_STALKSERVICEONOK
			 ,temp_service->stalk_on_ok
			 ,NDO_DATA_STALKSERVICEONWARNING
			 ,temp_service->stalk_on_warning
			 ,NDO_DATA_STALKSERVICEONUNKNOWN
			 ,temp_service->stalk_on_unknown
			 ,NDO_DATA_STALKSERVICEONCRITICAL
			 ,temp_service->stalk_on_critical
			 ,NDO_DATA_SERVICEISVOLATILE
			 ,temp_service->is_volatile
			 ,NDO_DATA_SERVICEFLAPDETECTIONENABLED
			 ,temp_service->flap_detection_enabled
			 ,NDO_DATA_FLAPDETECTIONONOK
			 ,flap_detection_on_ok
			 ,NDO_DATA_FLAPDETECTIONONWARNING
			 ,flap_detection_on_warning
			 ,NDO_DATA_FLAPDETECTIONONUNKNOWN
			 ,flap_detection_on_unknown
			 ,NDO_DATA_FLAPDETECTIONONCRITICAL
			 ,flap_detection_on_critical
			 ,NDO_DATA_LOWSERVICEFLAPTHRESHOLD
			 ,temp_service->low_flap_threshold
			 ,NDO_DATA_HIGHSERVICEFLAPTHRESHOLD
			 ,temp_service->high_flap_threshold
			 ,NDO_DATA_PROCESSSERVICEPERFORMANCEDATA
			 ,temp_service->process_performance_data
			 ,NDO_DATA_SERVICEFRESHNESSCHECKSENABLED
			 ,temp_service->check_freshness
			 ,NDO_DATA_SERVICEFRESHNESSTHRESHOLD
			 ,temp_service->freshness_threshold
			 ,NDO_DATA_PASSIVESERVICECHECKSENABLED
			 ,temp_service->accept_passive_service_checks
			 ,NDO_DATA_SERVICEEVENTHANDLERENABLED
			 ,temp_service->event_handler_enabled
			 ,NDO_DATA_ACTIVESERVICECHECKSENABLED
			 ,temp_service->checks_enabled
			 ,NDO_DATA_RETAINSERVICESTATUSINFORMATION
			 ,temp_service->retain_status_information
			 ,NDO_DATA_RETAINSERVICENONSTATUSINFORMATION
			 ,temp_service->retain_nonstatus_information
			 ,NDO_DATA_SERVICENOTIFICATIONSENABLED
			 ,temp_service->notifications_enabled
			 ,NDO_DATA_OBSESSOVERSERVICE
			 ,temp_service->obsess_over_service
			 ,NDO_DATA_SERVICEFAILUREPREDICTIONENABLED
			 ,temp_service->failure_prediction_enabled
			 ,NDO_DATA_NOTES
			 ,(es[7]==NULL)?"":es[7]
			 ,NDO_DATA_NOTESURL
			 ,(es[8]==NULL)?"":es[8]
			 ,NDO_DATA_ACTIONURL
			 ,(es[9]==NULL)?"":es[9]
			 ,NDO_DATA_ICONIMAGE
			 ,(es[10]==NULL)?"":es[10]
			 ,NDO_DATA_ICONIMAGEALT
			 ,(es[11]==NULL)?"":es[11]
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;

		/* dump contactgroups */
		for(temp_contactgroupsmember=temp_service->contact_groups;temp_contactgroupsmember!=NULL;temp_contactgroupsmember=temp_contactgroupsmember->next){
				
			es[0]=ndo_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACTGROUP
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		for(temp_contactsmember=temp_service->contacts;temp_contactsmember!=NULL;temp_contactsmember=temp_contactsmember->next){

			es[0]=ndo_escape_buffer(temp_contactsmember->contact_name);
				
			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACT
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
				
			free(es[0]);
			es[0]=NULL;
			}
#endif

#ifdef BUILD_NAGIOS_3X
		/* dump customvars */
		for(temp_customvar=temp_service->custom_variables;temp_customvar!=NULL;temp_customvar=temp_customvar->next){
				
			es[0]=ndo_escape_buffer(temp_customvar->variable_name);
			es[1]=ndo_escape_buffer(temp_customvar->variable_value);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s:%d:%s\n"
				 ,NDO_DATA_CUSTOMVARIABLE
				 ,(es[0]==NULL)?"":es[0]
				 ,temp_customvar->has_been_modified
				 ,(es[1]==NULL)?"":es[1]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			for(x=0;x<2;x++){
				free(es[x]);
				es[x]=NULL;
				}
		        }
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump servicegroup config ******/
	for(temp_servicegroup=servicegroup_list;temp_servicegroup!=NULL;temp_servicegroup=temp_servicegroup->next){

		es[0]=ndo_escape_buffer(temp_servicegroup->group_name);
		es[1]=ndo_escape_buffer(temp_servicegroup->alias);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n"
			 ,NDO_API_SERVICEGROUPDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_SERVICEGROUPNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICEGROUPALIAS
			 ,(es[1]==NULL)?"":es[1]
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		free(es[1]);
		es[0]=NULL;
		es[1]=NULL;
	
		/* dump members for each servicegroup */
#ifdef BUILD_NAGIOS_2X
		for(temp_servicegroupmember=temp_servicegroup->members;temp_servicegroupmember!=NULL;temp_servicegroupmember=temp_servicegroupmember->next)
#else
		for(temp_servicesmember=temp_servicegroup->members;temp_servicesmember!=NULL;temp_servicesmember=temp_servicesmember->next)
#endif
			{

#ifdef BUILD_NAGIOS_2X
			es[0]=ndo_escape_buffer(temp_servicegroupmember->host_name);
			es[1]=ndo_escape_buffer(temp_servicegroupmember->service_description);
#else
			es[0]=ndo_escape_buffer(temp_servicesmember->host_name);
			es[1]=ndo_escape_buffer(temp_servicesmember->service_description);
#endif

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s;%s\n"
				 ,NDO_DATA_SERVICEGROUPMEMBER
				 ,(es[0]==NULL)?"":es[0]
				 ,(es[1]==NULL)?"":es[1]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			free(es[1]);
			es[0]=NULL;
			es[1]=NULL;
		        }

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump host escalation config ******/
	for(temp_hostescalation=hostescalation_list;temp_hostescalation!=NULL;temp_hostescalation=temp_hostescalation->next){

		es[0]=ndo_escape_buffer(temp_hostescalation->host_name);
		es[1]=ndo_escape_buffer(temp_hostescalation->escalation_period);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n"
			 ,NDO_API_HOSTESCALATIONDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_ESCALATIONPERIOD
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_FIRSTNOTIFICATION
			 ,temp_hostescalation->first_notification
			 ,NDO_DATA_LASTNOTIFICATION
			 ,temp_hostescalation->last_notification
			 ,NDO_DATA_NOTIFICATIONINTERVAL
			 ,(double)temp_hostescalation->notification_interval
			 ,NDO_DATA_ESCALATEONRECOVERY
			 ,temp_hostescalation->escalate_on_recovery
			 ,NDO_DATA_ESCALATEONDOWN
			 ,temp_hostescalation->escalate_on_down
			 ,NDO_DATA_ESCALATEONUNREACHABLE
			 ,temp_hostescalation->escalate_on_unreachable
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;
	
		/* dump contactgroups */
		for(temp_contactgroupsmember=temp_hostescalation->contact_groups;temp_contactgroupsmember!=NULL;temp_contactgroupsmember=temp_contactgroupsmember->next){
				
			es[0]=ndo_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACTGROUP
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		for(temp_contactsmember=temp_hostescalation->contacts;temp_contactsmember!=NULL;temp_contactsmember=temp_contactsmember->next){

			es[0]=ndo_escape_buffer(temp_contactsmember->contact_name);
				
			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACT
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
				
			free(es[0]);
			es[0]=NULL;
			}
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump service escalation config ******/
	for(temp_serviceescalation=serviceescalation_list;temp_serviceescalation!=NULL;temp_serviceescalation=temp_serviceescalation->next){

		es[0]=ndo_escape_buffer(temp_serviceescalation->host_name);
		es[1]=ndo_escape_buffer(temp_serviceescalation->description);
		es[2]=ndo_escape_buffer(temp_serviceescalation->escalation_period);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%lf\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
			 ,NDO_API_SERVICEESCALATIONDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICEDESCRIPTION
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_ESCALATIONPERIOD
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_FIRSTNOTIFICATION
			 ,temp_serviceescalation->first_notification
			 ,NDO_DATA_LASTNOTIFICATION
			 ,temp_serviceescalation->last_notification
			 ,NDO_DATA_NOTIFICATIONINTERVAL
			 ,(double)temp_serviceescalation->notification_interval
			 ,NDO_DATA_ESCALATEONRECOVERY
			 ,temp_serviceescalation->escalate_on_recovery
			 ,NDO_DATA_ESCALATEONWARNING
			 ,temp_serviceescalation->escalate_on_warning
			 ,NDO_DATA_ESCALATEONUNKNOWN
			 ,temp_serviceescalation->escalate_on_unknown
			 ,NDO_DATA_ESCALATEONCRITICAL
			 ,temp_serviceescalation->escalate_on_critical
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		free(es[0]);
		es[0]=NULL;
	
		/* dump contactgroups */
		for(temp_contactgroupsmember=temp_serviceescalation->contact_groups;temp_contactgroupsmember!=NULL;temp_contactgroupsmember=temp_contactgroupsmember->next){
				
			es[0]=ndo_escape_buffer(temp_contactgroupsmember->group_name);

			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACTGROUP
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);

			free(es[0]);
			es[0]=NULL;
		        }

		/* dump individual contacts (not supported in Nagios 2.x) */
#ifndef BUILD_NAGIOS_2X
		for(temp_contactsmember=temp_serviceescalation->contacts;temp_contactsmember!=NULL;temp_contactsmember=temp_contactsmember->next){

			es[0]=ndo_escape_buffer(temp_contactsmember->contact_name);
				
			snprintf(temp_buffer,sizeof(temp_buffer)-1
				 ,"%d=%s\n"
				 ,NDO_DATA_CONTACT
				 ,(es[0]==NULL)?"":es[0]
				);
			temp_buffer[sizeof(temp_buffer)-1]='\x0';
			ndo_dbuf_strcat(&dbuf,temp_buffer);
				
			free(es[0]);
			es[0]=NULL;
			}
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump host dependency config ******/
	for(temp_hostdependency=hostdependency_list;temp_hostdependency!=NULL;temp_hostdependency=temp_hostdependency->next){

		es[0]=ndo_escape_buffer(temp_hostdependency->host_name);
		es[1]=ndo_escape_buffer(temp_hostdependency->dependent_host_name);

#ifdef BUILD_NAGIOS_3X
		es[2]=ndo_escape_buffer(temp_hostdependency->dependency_period);
#endif
#ifdef BUILD_NAGIOS_2X
		es[2]=NULL;
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n"
			 ,NDO_API_HOSTDEPENDENCYDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_DEPENDENTHOSTNAME
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_DEPENDENCYTYPE
			 ,temp_hostdependency->dependency_type
			 ,NDO_DATA_INHERITSPARENT
			 ,temp_hostdependency->inherits_parent
			 ,NDO_DATA_DEPENDENCYPERIOD
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_FAILONUP
			 ,temp_hostdependency->fail_on_up
			 ,NDO_DATA_FAILONDOWN
			 ,temp_hostdependency->fail_on_down
			 ,NDO_DATA_FAILONUNREACHABLE
			 ,temp_hostdependency->fail_on_unreachable
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	/****** dump service dependency config ******/
	for(temp_servicedependency=servicedependency_list;temp_servicedependency!=NULL;temp_servicedependency=temp_servicedependency->next){

		es[0]=ndo_escape_buffer(temp_servicedependency->host_name);
		es[1]=ndo_escape_buffer(temp_servicedependency->service_description);
		es[2]=ndo_escape_buffer(temp_servicedependency->dependent_host_name);
		es[3]=ndo_escape_buffer(temp_servicedependency->dependent_service_description);

#ifdef BUILD_NAGIOS_3X
		es[4]=ndo_escape_buffer(temp_servicedependency->dependency_period);
#endif
#ifdef BUILD_NAGIOS_2X
		es[4]=NULL;
#endif

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n%d=%s\n%d=%s\n%d=%s\n%d=%d\n%d=%d\n%d=%s\n%d=%d\n%d=%d\n%d=%d\n%d=%d\n"
			 ,NDO_API_SERVICEDEPENDENCYDEFINITION
			 ,NDO_DATA_TIMESTAMP
			 ,now.tv_sec
			 ,now.tv_usec
			 ,NDO_DATA_HOSTNAME
			 ,(es[0]==NULL)?"":es[0]
			 ,NDO_DATA_SERVICEDESCRIPTION
			 ,(es[1]==NULL)?"":es[1]
			 ,NDO_DATA_DEPENDENTHOSTNAME
			 ,(es[2]==NULL)?"":es[2]
			 ,NDO_DATA_DEPENDENTSERVICEDESCRIPTION
			 ,(es[3]==NULL)?"":es[3]
			 ,NDO_DATA_DEPENDENCYTYPE
			 ,temp_servicedependency->dependency_type
			 ,NDO_DATA_INHERITSPARENT
			 ,temp_servicedependency->inherits_parent
			 ,NDO_DATA_DEPENDENCYPERIOD
			 ,(es[4]==NULL)?"":es[4]
			 ,NDO_DATA_FAILONOK
			 ,temp_servicedependency->fail_on_ok
			 ,NDO_DATA_FAILONWARNING
			 ,temp_servicedependency->fail_on_warning
			 ,NDO_DATA_FAILONUNKNOWN
			 ,temp_servicedependency->fail_on_unknown
			 ,NDO_DATA_FAILONCRITICAL
			 ,temp_servicedependency->fail_on_critical
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		snprintf(temp_buffer,sizeof(temp_buffer)-1
			 ,"%d\n\n"
			 ,NDO_API_ENDDATA
			);
		temp_buffer[sizeof(temp_buffer)-1]='\x0';
		ndo_dbuf_strcat(&dbuf,temp_buffer);

		ndomod_write_to_sink(dbuf.buf,NDO_TRUE,NDO_TRUE);

		ndo_dbuf_free(&dbuf);
	        }


	/* free buffers */
	for(x=0;x<OBJECTCONFIG_ES_ITEMS;x++){
		free(es[x]);
		es[x]=NULL;
	        }

	return NDO_OK;
        }



/* dumps config files to data sink */
int ndomod_write_config_files(void){
	int result=NDO_OK;

	if((result=ndomod_write_main_config_file())==NDO_ERROR)
		return NDO_ERROR;

	if((result=ndomod_write_resource_config_files())==NDO_ERROR)
		return NDO_ERROR;

	return result;
        }



/* dumps main config file data to sink */
int ndomod_write_main_config_file(void){
	char fbuf[NDOMOD_MAX_BUFLEN];
	char *temp_buffer;
	struct timeval now;
	FILE *fp;
	char *var=NULL;
	char *val=NULL;

	/* get current time */
	gettimeofday(&now,NULL);

	asprintf(&temp_buffer
		 ,"\n%d:\n%d=%ld.%ld\n%d=%s\n"
		 ,NDO_API_MAINCONFIGFILEVARIABLES
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		 ,NDO_DATA_CONFIGFILENAME
		 ,config_file
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write each var/val pair from config file */
	if((fp=fopen(config_file,"r"))){

		while((fgets(fbuf,sizeof(fbuf),fp))){

			/* skip blank lines */
			if(fbuf[0]=='\x0' || fbuf[0]=='\n' || fbuf[0]=='\r')
				continue;
			
			strip(fbuf);

			/* skip comments */
			if(fbuf[0]=='#' || fbuf[0]==';')
				continue;

			if((var=strtok(fbuf,"="))==NULL)
				continue;
			val=strtok(NULL,"\n");	
			
			asprintf(&temp_buffer
				 ,"%d=%s=%s\n"
				 ,NDO_DATA_CONFIGFILEVARIABLE
				 ,var
				 ,(val==NULL)?"":val
				);
			ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
			free(temp_buffer);
			temp_buffer=NULL;
		        }

		fclose(fp);
	        }

	asprintf(&temp_buffer
		 ,"%d\n\n"
		 ,NDO_API_ENDDATA
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	return NDO_OK;
        }



/* dumps all resource config files to sink */
int ndomod_write_resource_config_files(void){

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	return NDO_OK;
        }



/* dumps a single resource config file to sink */
int ndomod_write_resource_config_file(char *filename){

	/* TODO */
	/* loop through main config file to find all resource config files, and then process them */
	/* this should probably NOT be done, as the resource file is supposed to remain private... */

	return NDO_OK;
        }



/* dumps runtime variables to sink */
int ndomod_write_runtime_variables(void){
	char *temp_buffer=NULL;
	struct timeval now;

	/* get current time */
	gettimeofday(&now,NULL);

	asprintf(&temp_buffer
		 ,"\n%d:\n%d=%ld.%ld\n"
		 ,NDO_API_RUNTIMEVARIABLES
		 ,NDO_DATA_TIMESTAMP
		 ,now.tv_sec
		 ,now.tv_usec
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write out main config file name */
	asprintf(&temp_buffer
		 ,"%d=%s=%s\n"
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"config_file"
		 ,config_file
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	/* write out vars determined after startup */
	asprintf(&temp_buffer
		 ,"%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lu\n%d=%s=%lu\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%lf\n%d=%s=%d\n%d=%s=%d\n%d=%s=%d\n"
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_services"
		 ,scheduling_info.total_services
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_scheduled_services"
		 ,scheduling_info.total_scheduled_services
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_hosts"
		 ,scheduling_info.total_hosts
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"total_scheduled_hosts"
		 ,scheduling_info.total_scheduled_hosts
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_services_per_host"
		 ,scheduling_info.average_services_per_host
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_scheduled_services_per_host"
		 ,scheduling_info.average_scheduled_services_per_host
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_check_interval_total"
		 ,scheduling_info.service_check_interval_total
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"host_check_interval_total"
		 ,scheduling_info.host_check_interval_total
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_service_check_interval"
		 ,scheduling_info.average_service_check_interval
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_host_check_interval"
		 ,scheduling_info.average_host_check_interval
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_service_inter_check_delay"
		 ,scheduling_info.average_service_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"average_host_inter_check_delay"
		 ,scheduling_info.average_host_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_inter_check_delay"
		 ,scheduling_info.service_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"host_inter_check_delay"
		 ,scheduling_info.host_inter_check_delay
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"service_interleave_factor"
		 ,scheduling_info.service_interleave_factor
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"max_service_check_spread"
		 ,scheduling_info.max_service_check_spread
		 ,NDO_DATA_RUNTIMEVARIABLE
		 ,"max_host_check_spread"
		 ,scheduling_info.max_host_check_spread
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	asprintf(&temp_buffer
		 ,"%d\n\n"
		 ,NDO_API_ENDDATA
		);
	ndomod_write_to_sink(temp_buffer,NDO_TRUE,NDO_TRUE);
	free(temp_buffer);
	temp_buffer=NULL;

	return NDO_OK;
        }



