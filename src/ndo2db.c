/***************************************************************
 * NDO2DB.C - NDO To Database Daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad 
 *
 * First Written: 05-19-2005
 * Last Modified: 10-31-2007
 *
 **************************************************************/

/*#define DEBUG_MEMORY 1*/

#ifdef DEBUG_MEMORY
#include <mcheck.h>
#endif

/* include our project's header files */
#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndo2db.h"
#include "../include/db.h"
#include "../include/dbhandlers.h"

#define NDO2DB_VERSION "1.4b7"
#define NDO2DB_NAME "NDO2DB"
#define NDO2DB_DATE "10-31-2007"


extern int errno;

char *ndo2db_config_file=NULL;
char *ndo2db_user=NULL;
char *ndo2db_group=NULL;
int ndo2db_sd=0;
int ndo2db_socket_type=NDO_SINK_UNIXSOCKET;
char *ndo2db_socket_name=NULL;
int ndo2db_tcp_port=NDO_DEFAULT_TCP_PORT;
int ndo2db_use_inetd=NDO_FALSE;
int ndo2db_show_version=NDO_FALSE;
int ndo2db_show_license=NDO_FALSE;
int ndo2db_show_help=NDO_FALSE;

ndo2db_dbconfig ndo2db_db_settings;
time_t ndo2db_db_last_checkin_time=0L;

char *ndo2db_debug_file=NULL;
int ndo2db_debug_level=NDO2DB_DEBUGL_NONE;
int ndo2db_debug_verbosity=NDO2DB_DEBUGV_BASIC;
FILE *ndo2db_debug_file_fp=NULL;
unsigned long ndo2db_max_debug_file_size=0L;

extern char *ndo2db_db_tablenames[NDO2DB_MAX_DBTABLES];



/*#define DEBUG_NDO2DB 1*/                        /* don't daemonize */
/*#define DEBUG_NDO2DB_EXIT_AFTER_CONNECTION 1*/    /* exit after first client disconnects */
/*#define DEBUG_NDO2DB2 1*/
/*#define NDO2DB_DEBUG_MBUF 1*/
/*
#ifdef NDO2DB_DEBUG_MBUF
unsigned long mbuf_bytes_allocated=0L;
unsigned long mbuf_data_allocated=0L;
#endif
*/


int main(int argc, char **argv){
	int db_supported=NDO_FALSE;
	int result=NDO_OK;

#ifdef DEBUG_MEMORY
	mtrace();
#endif

	result=ndo2db_process_arguments(argc,argv);

        if(result!=NDO_OK || ndo2db_show_help==NDO_TRUE || ndo2db_show_license==NDO_TRUE || ndo2db_show_version==NDO_TRUE){

		if(result!=NDO_OK)
			printf("Incorrect command line arguments supplied\n");

		printf("\n");
		printf("%s %s\n",NDO2DB_NAME,NDO2DB_VERSION);
		printf("Copyright(c) 2005-2007 Ethan Galstad (nagios@nagios.org)\n");
		printf("Last Modified: %s\n",NDO2DB_DATE);
		printf("License: GPL v2\n");
		printf("\n");
		printf("Stores Nagios event and configuration data to a database for later retrieval\n");
		printf("and processing.  Clients that are capable of sending data to the NDO2DB daemon\n");
		printf("include the LOG2NDO utility and NDOMOD event broker module.\n");
		printf("\n");
		printf("Usage: %s -c <config_file> [-i]\n",argv[0]);
		printf("\n");
		printf("-i  = Run under INETD/XINETD.\n");
		printf("\n");
		exit(1);
	        }

	/* initialize variables */
	ndo2db_initialize_variables();

	/* process config file */
	if(ndo2db_process_config_file(ndo2db_config_file)!=NDO_OK){
		printf("Error processing config file '%s'.\n",ndo2db_config_file);
		exit(1);
	        }
	
	/* open debug log */
	ndo2db_open_debug_log();

	/* make sure we're good to go */
	if(ndo2db_check_init_reqs()!=NDO_OK){
		printf("One or more required parameters is missing or incorrect.\n");
		exit(1);
	        }

	/* make sure we support the db option chosen... */
#ifdef USE_MYSQL
	if(ndo2db_db_settings.server_type==NDO2DB_DBSERVER_MYSQL)
		db_supported=NDO_TRUE;
#endif
#ifdef USE_PGSQL
	/* PostgreSQL support is not yet done... */
	/*
	if(ndo2db_db_settings.server_type==NDO2DB_DBSERVER_PGSQL)
		db_supported=NDO_TRUE;
	*/
#endif
	if(db_supported==NDO_FALSE){
		printf("Support for the specified database server is either not yet supported, or was not found on your system.\n");
		exit(1);
	        }

	/* initialize signal handling */
	signal(SIGQUIT,ndo2db_parent_sighandler);
	signal(SIGTERM,ndo2db_parent_sighandler);
	signal(SIGINT,ndo2db_parent_sighandler);
	signal(SIGSEGV,ndo2db_parent_sighandler);
	signal(SIGFPE,ndo2db_parent_sighandler);
	signal(SIGCHLD,ndo2db_parent_sighandler);

	/* drop privileges */
	ndo2db_drop_privileges(ndo2db_user,ndo2db_group);

	/* if we're running under inetd... */
	if(ndo2db_use_inetd==NDO_TRUE){

		/* redirect STDERR to /dev/null */
		close(2);
		open("/dev/null",O_WRONLY);

		/* handle the connection */
		ndo2db_handle_client_connection(0);
	        }

	/* standalone daemon... */
	else{

		/* create socket and wait for clients to connect */
		if(ndo2db_wait_for_connections()==NDO_ERROR)
			return 1;
	        }

	/* close debug log */
	ndo2db_close_debug_log();

	/* free memory */
	ndo2db_free_program_memory();

	return 0;
        }


/* process command line arguments */
int ndo2db_process_arguments(int argc, char **argv){
	char optchars[32];
	int c=1;

#ifdef HAVE_GETOPT_H
	int option_index=0;
	static struct option long_options[]={
		{"configfile", required_argument, 0, 'c'},
		{"inetd", no_argument, 0, 'i'},
		{"help", no_argument, 0, 'h'},
		{"license", no_argument, 0, 'l'},
		{"version", no_argument, 0, 'V'},
		{0, 0, 0, 0}
                };
#endif

	/* no options were supplied */
	if(argc<2){
		ndo2db_show_help=NDO_TRUE;
		return NDO_OK;
	        }

	snprintf(optchars,sizeof(optchars),"c:ihlV");

	while(1){
#ifdef HAVE_GETOPT_H
		c=getopt_long(argc,argv,optchars,long_options,&option_index);
#else
		c=getopt(argc,argv,optchars);
#endif
		if(c==-1 || c==EOF)
			break;

		/* process all arguments */
		switch(c){

		case '?':
		case 'h':
			ndo2db_show_help=NDO_TRUE;
			break;
		case 'V':
			ndo2db_show_version=NDO_TRUE;
			break;
		case 'l':
			ndo2db_show_license=NDO_TRUE;
			break;
		case 'c':
			ndo2db_config_file=strdup(optarg);
			break;
		case 'i':
			ndo2db_use_inetd=NDO_TRUE;
			break;
		default:
			return NDO_ERROR;
			break;
		        }
	        }

	/* make sure required args were supplied */
	if((ndo2db_config_file==NULL) && ndo2db_show_help==NDO_FALSE && ndo2db_show_version==NDO_FALSE  && ndo2db_show_license==NDO_FALSE)
		return NDO_ERROR;

	return NDO_OK;
        }



/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* process all config vars in a file */
int ndo2db_process_config_file(char *filename){
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
		result=ndo2db_process_config_var(buf);

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
int ndo2db_process_config_var(char *arg){
	char *var=NULL;
	char *val=NULL;

	/* split var/val */
	var=strtok(arg,"=");
	val=strtok(NULL,"\n");

	/* skip incomplete var/val pairs */
	if(var==NULL || val==NULL)
		return NDO_OK;

	/* process the variable... */

	if(!strcmp(var,"socket_type")){
		if(!strcmp(val,"tcp"))
			ndo2db_socket_type=NDO_SINK_TCPSOCKET;
		else
			ndo2db_socket_type=NDO_SINK_UNIXSOCKET;
	        }
	else if(!strcmp(var,"socket_name")){
		if((ndo2db_socket_name=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"tcp_port")){
		ndo2db_tcp_port=atoi(val);
	        }
	else if(!strcmp(var,"db_servertype")){
		if(!strcmp(val,"mysql"))
			ndo2db_db_settings.server_type=NDO2DB_DBSERVER_MYSQL;
		else if(!strcmp(val,"pgsql"))
			ndo2db_db_settings.server_type=NDO2DB_DBSERVER_PGSQL;
		else
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"db_host")){
		if((ndo2db_db_settings.host=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"db_port")){
		ndo2db_db_settings.port=atoi(val);
	        }
	else if(!strcmp(var,"db_user")){
		if((ndo2db_db_settings.username=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"db_pass")){
		if((ndo2db_db_settings.password=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"db_name")){
		if((ndo2db_db_settings.dbname=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"db_prefix")){
		if((ndo2db_db_settings.dbprefix=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"max_timedevents_age"))
		ndo2db_db_settings.max_timedevents_age=strtoul(val,NULL,0)*60;
	else if(!strcmp(var,"max_systemcommands_age"))
		ndo2db_db_settings.max_systemcommands_age=strtoul(val,NULL,0)*60;
	else if(!strcmp(var,"max_servicechecks_age"))
		ndo2db_db_settings.max_servicechecks_age=strtoul(val,NULL,0)*60;
	else if(!strcmp(var,"max_hostchecks_age"))
		ndo2db_db_settings.max_hostchecks_age=strtoul(val,NULL,0)*60;
	else if(!strcmp(var,"max_eventhandlers_age"))
		ndo2db_db_settings.max_eventhandlers_age=strtoul(val,NULL,0)*60;

	else if(!strcmp(var,"ndo2db_user"))
		ndo2db_user=strdup(val);
	else if(!strcmp(var,"ndo2db_group"))
		ndo2db_group=strdup(val);
		
	else if(!strcmp(var,"debug_file")){
		if((ndo2db_debug_file=strdup(val))==NULL)
			return NDO_ERROR;
	        }
	else if(!strcmp(var,"debug_level"))
		ndo2db_debug_level=atoi(val);
	else if(!strcmp(var,"debug_verbosity"))
		ndo2db_debug_verbosity=atoi(val);
	else if(!strcmp(var,"max_debug_file_size"))
		ndo2db_max_debug_file_size=strtoul(val,NULL,0);

	return NDO_OK;
        }


/* initialize variables */
int ndo2db_initialize_variables(void){
	
	ndo2db_db_settings.server_type=NDO2DB_DBSERVER_NONE;
	ndo2db_db_settings.host=NULL;
	ndo2db_db_settings.port=0;
	ndo2db_db_settings.username=NULL;
	ndo2db_db_settings.password=NULL;
	ndo2db_db_settings.dbname=NULL;
	ndo2db_db_settings.dbprefix=NULL;
	ndo2db_db_settings.max_timedevents_age=0L;
	ndo2db_db_settings.max_systemcommands_age=0L;
	ndo2db_db_settings.max_servicechecks_age=0L;
	ndo2db_db_settings.max_hostchecks_age=0L;
	ndo2db_db_settings.max_eventhandlers_age=0L;

	return NDO_OK;
        }



/****************************************************************************/
/* CLEANUP FUNCTIONS                                                       */
/****************************************************************************/

/* free program memory */
int ndo2db_free_program_memory(void){

	if(ndo2db_config_file){
		free(ndo2db_config_file);
		ndo2db_config_file=NULL;
		}
	if(ndo2db_user){
		free(ndo2db_user);
		ndo2db_user=NULL;
		}
	if(ndo2db_group){
		free(ndo2db_group);
		ndo2db_group=NULL;
		}
	if(ndo2db_socket_name){
		free(ndo2db_socket_name);
		ndo2db_socket_name=NULL;
		}
	if(ndo2db_db_settings.host){
		free(ndo2db_db_settings.host);
		ndo2db_db_settings.host=NULL;
		}
	if(ndo2db_db_settings.username){
		free(ndo2db_db_settings.username);
		ndo2db_db_settings.username=NULL;
		}
	if(ndo2db_db_settings.password){
		free(ndo2db_db_settings.password);
		ndo2db_db_settings.password=NULL;
		}
	if(ndo2db_db_settings.dbname){
		free(ndo2db_db_settings.dbname);
		ndo2db_db_settings.dbname=NULL;
		}
	if(ndo2db_db_settings.dbprefix){
		free(ndo2db_db_settings.dbprefix);
		ndo2db_db_settings.dbprefix=NULL;
		}
	if(ndo2db_debug_file){
		free(ndo2db_debug_file);
		ndo2db_debug_file=NULL;
		}

	return NDO_OK;
	}



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

int ndo2db_check_init_reqs(void){

	if(ndo2db_socket_type==NDO_SINK_UNIXSOCKET){
		if(ndo2db_socket_name==NULL){
			printf("No socket name specified.\n");
			return NDO_ERROR;
		        }
	        }

	return NDO_OK;
        }



/* drops privileges */
int ndo2db_drop_privileges(char *user, char *group){
	uid_t uid=-1;
	gid_t gid=-1;
	struct group *grp;
	struct passwd *pw;

	/* set effective group ID */
	if(group!=NULL){
		
		/* see if this is a group name */
		if(strspn(group,"0123456789")<strlen(group)){
			grp=(struct group *)getgrnam(group);
			if(grp!=NULL)
				gid=(gid_t)(grp->gr_gid);
			else
				syslog(LOG_ERR,"Warning: Could not get group entry for '%s'",group);
			endgrent();
		        }

		/* else we were passed the GID */
		else
			gid=(gid_t)atoi(group);

		/* set effective group ID if other than current EGID */
		if(gid!=getegid()){

			if(setgid(gid)==-1)
				syslog(LOG_ERR,"Warning: Could not set effective GID=%d",(int)gid);
		        }
	        }


	/* set effective user ID */
	if(user!=NULL){
		
		/* see if this is a user name */
		if(strspn(user,"0123456789")<strlen(user)){
			pw=(struct passwd *)getpwnam(user);
			if(pw!=NULL)
				uid=(uid_t)(pw->pw_uid);
			else
				syslog(LOG_ERR,"Warning: Could not get passwd entry for '%s'",user);
			endpwent();
		        }

		/* else we were passed the UID */
		else
			uid=(uid_t)atoi(user);
			
		/* set effective user ID if other than current EUID */
		if(uid!=geteuid()){

#ifdef HAVE_INITGROUPS
			/* initialize supplementary groups */
			if(initgroups(user,gid)==-1){
				if(errno==EPERM)
					syslog(LOG_ERR,"Warning: Unable to change supplementary groups using initgroups()");
				else{
					syslog(LOG_ERR,"Warning: Possibly root user failed dropping privileges with initgroups()");
					return NDO_ERROR;
			                }
	                        }
#endif

			if(setuid(uid)==-1)
				syslog(LOG_ERR,"Warning: Could not set effective UID=%d",(int)uid);
		        }
	        }

	return NDO_OK;
        }


int ndo2db_daemonize(void){
	pid_t pid=-1;

	/* fork */
	if((pid=fork())<0){
		perror("Fork error");
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	        }

	/* parent process goes away... */
	else if((int)pid!=0){
		ndo2db_free_program_memory();
		exit(0);
		}

	/* child forks again... */
	else{

		if((pid=fork())<0){
			perror("Fork error");
			ndo2db_cleanup_socket();
			return NDO_ERROR;
	                }

		/* first child process goes away.. */
		else if((int)pid!=0){
			ndo2db_free_program_memory();
			exit(0);
			}

		/* grandchild continues... */

		/* grandchild becomes session leader... */
		setsid();
	        }

        /* close existing stdin, stdout, stderr */
	close(0);
#ifndef DEBUG_NDO2DB
	close(1);
#endif
	close(2);

	/* re-open stdin, stdout, stderr with known values */
	open("/dev/null",O_RDONLY);
#ifndef DEBUG_NDO2DB
	open("/dev/null",O_WRONLY);
#endif
	open("/dev/null",O_WRONLY);

	return NDO_OK;
        }


int ndo2db_cleanup_socket(void){

	/* we're running under INETD */
	if(ndo2db_use_inetd==NDO_TRUE)
		return NDO_OK;

	/* close the socket */
	shutdown(ndo2db_sd,2);
	close(ndo2db_sd);

	/* unlink the file */
	if(ndo2db_socket_type==NDO_SINK_UNIXSOCKET)
		unlink(ndo2db_socket_name);

	return NDO_OK;
        }


void ndo2db_parent_sighandler(int sig){

	/* cleanup children that exit, so we don't have zombies */
	if(sig==SIGCHLD){
		waitpid(-1,NULL,WNOHANG);
		return;
	        }

	/* cleanup the socket */
	ndo2db_cleanup_socket();

	/* free memory */
	ndo2db_free_program_memory();

	exit(0);

	return;
        }


void ndo2db_child_sighandler(int sig){

	_exit(0);

	return;
        }


/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/


int ndo2db_wait_for_connections(void){
	int sd_flag=1;
	int new_sd=0;
	pid_t new_pid=-1;
	struct sockaddr_un server_address_u;
	struct sockaddr_in server_address_i;
	struct sockaddr_un client_address_u;
	struct sockaddr_in client_address_i;
	socklen_t client_address_length;

	/* TCP socket */
	if(ndo2db_socket_type==NDO_SINK_TCPSOCKET){

		/* create a socket */
		if(!(ndo2db_sd=socket(PF_INET,SOCK_STREAM,0))){
			perror("Cannot create socket");
			return NDO_ERROR;
		        }

		/* set the reuse address flag so we don't get errors when restarting */
		sd_flag=1;
		if(setsockopt(ndo2db_sd,SOL_SOCKET,SO_REUSEADDR,(char *)&sd_flag,sizeof(sd_flag))<0){
			printf("Could not set reuse address option on socket!\n");
			return NDO_ERROR;
	                }

		/* clear the address */
		bzero((char *)&server_address_i,sizeof(server_address_i));
		server_address_i.sin_family=AF_INET; 
		server_address_i.sin_addr.s_addr=INADDR_ANY;
		server_address_i.sin_port=htons(ndo2db_tcp_port);

		/* bind the socket */
		if((bind(ndo2db_sd,(struct sockaddr *)&server_address_i,sizeof(server_address_i)))){
			close(ndo2db_sd);
			perror("Could not bind socket");
			return NDO_ERROR;
	                }

		client_address_length=(socklen_t)sizeof(client_address_i);
	        }

	/* UNIX domain socket */
	else{

		/* create a socket */
		if(!(ndo2db_sd=socket(AF_UNIX,SOCK_STREAM,0))){
			perror("Cannot create socket");
			return NDO_ERROR;
	                }

		/* copy the socket path */
		strncpy(server_address_u.sun_path,ndo2db_socket_name,sizeof(server_address_u.sun_path)); 
		server_address_u.sun_family=AF_UNIX; 

		/* bind the socket */
		if((bind(ndo2db_sd,(struct sockaddr *)&server_address_u,SUN_LEN(&server_address_u)))){
			close(ndo2db_sd);
			perror("Could not bind socket");
			return NDO_ERROR;
	                }

		client_address_length=(socklen_t)sizeof(client_address_u);
	        }

	/* listen for connections */
	if((listen(ndo2db_sd,1))){
		perror("Cannot listen on socket");
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	        }


	/* daemonize */
#ifndef DEBUG_NDO2DB
	if(ndo2db_daemonize()!=NDO_OK)
		return NDO_ERROR;
#endif

	/* accept connections... */
	while(1){

		if((new_sd=accept(ndo2db_sd,(ndo2db_socket_type==NDO_SINK_TCPSOCKET)?(struct sockaddr *)&client_address_i:(struct sockaddr *)&client_address_u,(socklen_t *)&client_address_length))<0){
			perror("Accept error");
			ndo2db_cleanup_socket();
			return NDO_ERROR;
		        }

#ifndef DEBUG_NDO2DB
		/* fork... */
		new_pid=fork();

		switch(new_pid){
		case -1:
			/* parent simply prints an error message and keeps on going... */
			perror("Fork error");
			close(new_sd);
			break;

		case 0:
#endif
			/* child processes data... */
			ndo2db_handle_client_connection(new_sd);

			/* close socket when we're done */
			close(new_sd);
#ifndef DEBUG_NDO2DB
			return NDO_OK;
			break;

		default:
			/* parent keeps on going... */
			close(new_sd);
			break;
		        }
#endif

#ifdef DEBUG_NDO2DB_EXIT_AFTER_CONNECTION
		break;
#endif
	        }

	/* cleanup after ourselves */
	ndo2db_cleanup_socket();

	return NDO_OK;
        }


int ndo2db_handle_client_connection(int sd){
	ndo_dbuf dbuf;
	int dbuf_chunk=2048;
	ndo2db_idi idi;
	char buf[512];
	int result=0;
	int error=NDO_FALSE;

	/* open syslog facility */
	/*openlog("ndo2db",0,LOG_DAEMON);*/

	/* re-open debug log */
	ndo2db_close_debug_log();
	ndo2db_open_debug_log();

	/* reset signal handling */
	signal(SIGQUIT,ndo2db_child_sighandler);
	signal(SIGTERM,ndo2db_child_sighandler);
	signal(SIGINT,ndo2db_child_sighandler);
	signal(SIGSEGV,ndo2db_child_sighandler);
	signal(SIGFPE,ndo2db_child_sighandler);

	/* initialize input data information */
	ndo2db_idi_init(&idi);

	/* initialize dynamic buffer (2KB chunk size) */
	ndo_dbuf_init(&dbuf,dbuf_chunk);

	/* initialize database connection */
	ndo2db_db_init(&idi);
	ndo2db_db_connect(&idi);

	/* read all data from client */
	while(1){

		result=read(sd,buf,sizeof(buf)-1);

		/* bail out on hard errors */
		if(result==-1 && (errno!=EAGAIN && errno!=EINTR)){
			error=NDO_TRUE;
			break;
		        }

		/* zero bytes read means we lost the connection with the client */
		if(result==0){

			/* gracefully back out of current operation... */
			ndo2db_db_goodbye(&idi);

			break;
		        }

#ifdef DEBUG_NDO2DB2
		printf("BYTESREAD: %d\n",result);
#endif

		/* append data we just read to dynamic buffer */
		buf[result]='\x0';
		ndo_dbuf_strcat(&dbuf,buf);

		/* check for completed lines of input */
		ndo2db_check_for_client_input(&idi,&dbuf);

		/* should we disconnect the client? */
		if(idi.disconnect_client==NDO_TRUE){

			/* gracefully back out of current operation... */
			ndo2db_db_goodbye(&idi);

			break;
		        }
	        }

#ifdef DEBUG_NDO2DB2
	printf("BYTES: %lu, LINES: %lu\n",idi.bytes_processed,idi.lines_processed);
#endif

	/* free memory allocated to dynamic buffer */
	ndo_dbuf_free(&dbuf);

	/* disconnect from database */
	ndo2db_db_disconnect(&idi);
	ndo2db_db_deinit(&idi);

	/* free memory */
	ndo2db_free_input_memory(&idi);
	ndo2db_free_connection_memory(&idi);

	/* close syslog facility */
	/*closelog();*/

	if(error==NDO_TRUE)
		return NDO_ERROR;

	return NDO_OK;
        }


/* initializes structure for tracking data */
int ndo2db_idi_init(ndo2db_idi *idi){
	int x=0;

	if(idi==NULL)
		return NDO_ERROR;

	idi->disconnect_client=NDO_FALSE;
	idi->ignore_client_data=NDO_FALSE;
	idi->protocol_version=0;
	idi->instance_name=NULL;
	idi->buffered_input=NULL;
	idi->agent_name=NULL;
	idi->agent_version=NULL;
	idi->disposition=NULL;
	idi->connect_source=NULL;
	idi->connect_type=NULL;
	idi->current_input_section=NDO2DB_INPUT_SECTION_NONE;
	idi->current_input_data=NDO2DB_INPUT_DATA_NONE;
	idi->bytes_processed=0L;
	idi->lines_processed=0L;
	idi->entries_processed=0L;
	idi->current_object_config_type=NDO2DB_CONFIGTYPE_ORIGINAL;
	idi->data_start_time=0L;
	idi->data_end_time=0L;

	/* initialize mbuf */
	for(x=0;x<NDO2DB_MAX_MBUF_ITEMS;x++){
		idi->mbuf[x].used_lines=0;
		idi->mbuf[x].allocated_lines=0;
		idi->mbuf[x].buffer=NULL;
	        }
	
	return NDO_OK;
        }


/* checks for single lines of input from a client connection */
int ndo2db_check_for_client_input(ndo2db_idi *idi,ndo_dbuf *dbuf){
	char *buf=NULL;
	register int x;
	

	if(dbuf==NULL)
		return NDO_OK;
	if(dbuf->buf==NULL)
		return NDO_OK;

#ifdef DEBUG_NDO2DB2
	printf("RAWBUF: %s\n",dbuf->buf);
	printf("  USED1: %lu, BYTES: %lu, LINES: %lu\n",dbuf->used_size,idi->bytes_processed,idi->lines_processed);
#endif

	/* search for complete lines of input */
	for(x=0;dbuf->buf[x]!='\x0';x++){

		/* we found the end of a line */
		if(dbuf->buf[x]=='\n'){

#ifdef DEBUG_NDO2DB2
			printf("BUF[%d]='\\n'\n",x);
#endif

			/* handle this line of input */
			dbuf->buf[x]='\x0';
			if((buf=strdup(dbuf->buf))){
				ndo2db_handle_client_input(idi,buf);
				free(buf);
				buf=NULL;
				idi->lines_processed++;
				idi->bytes_processed+=(x+1);
			        }
		
			/* shift data back to front of buffer and adjust counters */
			memmove((void *)&dbuf->buf[0],(void *)&dbuf->buf[x+1],(size_t)((int)dbuf->used_size-x-1));
			dbuf->used_size-=(x+1);
			dbuf->buf[dbuf->used_size]='\x0';
			x=-1;
#ifdef DEBUG_NDO2DB2
			printf("  USED2: %lu, BYTES: %lu, LINES: %lu\n",dbuf->used_size,idi->bytes_processed,idi->lines_processed);
#endif
		        }
	        }

	return NDO_OK;
        }


/* handles a single line of input from a client connection */
int ndo2db_handle_client_input(ndo2db_idi *idi, char *buf){
	char *var=NULL;
	char *val=NULL;
	unsigned long data_type_long=0L;
	int data_type=NDO_DATA_NONE;
	int input_type=NDO2DB_INPUT_DATA_NONE;

#ifdef DEBUG_NDO2DB2
	printf("HANDLING: '%s'\n",buf);
#endif

	if(buf==NULL || idi==NULL)
		return NDO_ERROR;

	/* we're ignoring client data because of wrong protocol version, etc...  */
	if(idi->ignore_client_data==NDO_TRUE)
		return NDO_ERROR;

	/* skip empty lines */
	if(buf[0]=='\x0')
		return NDO_OK;

	switch(idi->current_input_section){

	case NDO2DB_INPUT_SECTION_NONE:

		var=strtok(buf,":");
		val=strtok(NULL,"\n");

		if(!strcmp(var,NDO_API_HELLO)){

			idi->current_input_section=NDO2DB_INPUT_SECTION_HEADER;
			idi->current_input_data=NDO2DB_INPUT_DATA_NONE;

			/* free old connection memory (necessary in some cases) */
			ndo2db_free_connection_memory(idi);
		        }

		break;

	case NDO2DB_INPUT_SECTION_HEADER:

		var=strtok(buf,":");
		val=strtok(NULL,"\n");

		if(!strcmp(var,NDO_API_STARTDATADUMP)){

			/* client is using wrong protocol version, bail out here... */
			if(idi->protocol_version!=NDO_API_PROTOVERSION){
				syslog(LOG_USER|LOG_INFO,"Error: Client protocol version %d is incompatible with server version %d.  Disconnecting client...",idi->protocol_version,NDO_API_PROTOVERSION);
				idi->disconnect_client=NDO_TRUE;
				idi->ignore_client_data=NDO_TRUE;
				return NDO_ERROR;
			        }

			idi->current_input_section=NDO2DB_INPUT_SECTION_DATA;

			/* save connection info to DB */
			ndo2db_db_hello(idi);
		        }

		else if(!strcmp(var,NDO_API_PROTOCOL))
			ndo2db_convert_string_to_int((val+1),&idi->protocol_version);

		else if(!strcmp(var,NDO_API_INSTANCENAME))
			idi->instance_name=strdup(val+1);

		else if(!strcmp(var,NDO_API_AGENT))
			idi->agent_name=strdup(val+1);

		else if(!strcmp(var,NDO_API_AGENTVERSION))
			idi->agent_version=strdup(val+1);

		else if(!strcmp(var,NDO_API_DISPOSITION))
			idi->disposition=strdup(val+1);

		else if(!strcmp(var,NDO_API_CONNECTION))
			idi->connect_source=strdup(val+1);

		else if(!strcmp(var,NDO_API_CONNECTTYPE))
			idi->connect_type=strdup(val+1);

		else if(!strcmp(var,NDO_API_STARTTIME))
			ndo2db_convert_string_to_unsignedlong((val+1),&idi->data_start_time);

		break;

	case NDO2DB_INPUT_SECTION_FOOTER:

		var=strtok(buf,":");
		val=strtok(NULL,"\n");

		/* client is saying goodbye... */
		if(!strcmp(var,NDO_API_GOODBYE))
			idi->current_input_section=NDO2DB_INPUT_SECTION_NONE;

		else if(!strcmp(var,NDO_API_ENDTIME))
			ndo2db_convert_string_to_unsignedlong((val+1),&idi->data_end_time);

		break;

	case NDO2DB_INPUT_SECTION_DATA:

		if(idi->current_input_data==NDO2DB_INPUT_DATA_NONE){

			var=strtok(buf,":");
			val=strtok(NULL,"\n");

			input_type=atoi(var);

			switch(input_type){

			/* we're reached the end of all of the data... */
			case NDO_API_ENDDATADUMP:
				idi->current_input_section=NDO2DB_INPUT_SECTION_FOOTER;
				idi->current_input_data=NDO2DB_INPUT_DATA_NONE;
				break;

			/* config dumps */
			case NDO_API_STARTCONFIGDUMP:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONFIGDUMPSTART;
				break;
			case NDO_API_ENDCONFIGDUMP:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONFIGDUMPEND;
				break;

			/* archived data */
			case NDO_API_LOGENTRY:
				idi->current_input_data=NDO2DB_INPUT_DATA_LOGENTRY;
				break;

			/* realtime data */
			case NDO_API_PROCESSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_PROCESSDATA;
				break;
			case NDO_API_TIMEDEVENTDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_TIMEDEVENTDATA;
				break;
			case NDO_API_LOGDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_LOGDATA;
				break;
			case NDO_API_SYSTEMCOMMANDDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA;
				break;
			case NDO_API_EVENTHANDLERDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_EVENTHANDLERDATA;
				break;
			case NDO_API_NOTIFICATIONDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_NOTIFICATIONDATA;
				break;
			case NDO_API_SERVICECHECKDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICECHECKDATA;
				break;
			case NDO_API_HOSTCHECKDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTCHECKDATA;
				break;
			case NDO_API_COMMENTDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_COMMENTDATA;
				break;
			case NDO_API_DOWNTIMEDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_DOWNTIMEDATA;
				break;
			case NDO_API_FLAPPINGDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_FLAPPINGDATA;
				break;
			case NDO_API_PROGRAMSTATUSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA;
				break;
			case NDO_API_HOSTSTATUSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTSTATUSDATA;
				break;
			case NDO_API_SERVICESTATUSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICESTATUSDATA;
				break;
			case NDO_API_CONTACTSTATUSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONTACTSTATUSDATA;
				break;
			case NDO_API_ADAPTIVEPROGRAMDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA;
				break;
			case NDO_API_ADAPTIVEHOSTDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA;
				break;
			case NDO_API_ADAPTIVESERVICEDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA;
				break;
			case NDO_API_ADAPTIVECONTACTDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA;
				break;
			case NDO_API_EXTERNALCOMMANDDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA;
				break;
			case NDO_API_AGGREGATEDSTATUSDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA;
				break;
			case NDO_API_RETENTIONDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_RETENTIONDATA;
				break;
			case NDO_API_CONTACTNOTIFICATIONDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA;
				break;
			case NDO_API_CONTACTNOTIFICATIONMETHODDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA;
				break;
			case NDO_API_ACKNOWLEDGEMENTDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA;
				break;
			case NDO_API_STATECHANGEDATA:
				idi->current_input_data=NDO2DB_INPUT_DATA_STATECHANGEDATA;
				break;

			/* config variables */
			case NDO_API_MAINCONFIGFILEVARIABLES:
				idi->current_input_data=NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES;
				break;
			case NDO_API_RESOURCECONFIGFILEVARIABLES:
				idi->current_input_data=NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES;
				break;
			case NDO_API_CONFIGVARIABLES:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONFIGVARIABLES;
				break;
			case NDO_API_RUNTIMEVARIABLES:
				idi->current_input_data=NDO2DB_INPUT_DATA_RUNTIMEVARIABLES;
				break;

			/* object configuration */
			case NDO_API_HOSTDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTDEFINITION;
				break;
			case NDO_API_HOSTGROUPDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION;
				break;
			case NDO_API_SERVICEDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICEDEFINITION;
				break;
			case NDO_API_SERVICEGROUPDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION;
				break;
			case NDO_API_HOSTDEPENDENCYDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION;
				break;
			case NDO_API_SERVICEDEPENDENCYDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION;
				break;
			case NDO_API_HOSTESCALATIONDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION;
				break;
			case NDO_API_SERVICEESCALATIONDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION;
				break;
			case NDO_API_COMMANDDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_COMMANDDEFINITION;
				break;
			case NDO_API_TIMEPERIODDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION;
				break;
			case NDO_API_CONTACTDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONTACTDEFINITION;
				break;
			case NDO_API_CONTACTGROUPDEFINITION:
				idi->current_input_data=NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION;
				break;
			case NDO_API_HOSTEXTINFODEFINITION:
				/* deprecated - merged with host definitions */
			case NDO_API_SERVICEEXTINFODEFINITION:
				/* deprecated - merged with service definitions */
			default:
				break;
			        }

			/* initialize input data */
			ndo2db_start_input_data(idi);
		        }

		/* we are processing some type of data already... */
		else{

			var=strtok(buf,"=");
			val=strtok(NULL,"\n");

			/* get the data type */
			data_type_long=strtoul(var,NULL,0);
				
			/* there was an error with the data type - throw it out */
			if(data_type_long==ULONG_MAX && errno==ERANGE)
				break;

			data_type=(int)data_type_long;

			/* the current data section is ending... */
			if(data_type==NDO_API_ENDDATA){

				/* finish current data processing */
				ndo2db_end_input_data(idi);

				idi->current_input_data=NDO2DB_INPUT_DATA_NONE;
		                }

			/* add data for already existing data type... */
			else{

				/* the data type is out of range - throw it out */
				if(data_type>NDO_MAX_DATA_TYPES){
#ifdef DEBUG_NDO2DB2
					printf("## DISCARD! LINE: %lu, TYPE: %d, VAL: %s\n",idi->lines_processed,data_type,val);
#endif
					break;
			                }

#ifdef DEBUG_NDO2DB2
				printf("LINE: %lu, TYPE: %d, VAL:%s\n",idi->lines_processed,data_type,val);
#endif
				ndo2db_add_input_data_item(idi,data_type,val);
		                }
		        }
		
		break;

	default:
		break;
	        }

	return NDO_OK;
        }


int ndo2db_start_input_data(ndo2db_idi *idi){
	int x;

	if(idi==NULL)
		return NDO_ERROR;

	/* sometimes ndo2db_end_input_data() isn't called, so free memory if we find it */
	ndo2db_free_input_memory(idi);

	/* allocate memory for holding buffered input */
	if((idi->buffered_input=(char **)malloc(sizeof(char *)*NDO_MAX_DATA_TYPES))==NULL)
		return NDO_ERROR;

	/* initialize buffered input slots */
	for(x=0;x<NDO_MAX_DATA_TYPES;x++)
		idi->buffered_input[x]=NULL;

	return NDO_OK;
        }


int ndo2db_add_input_data_item(ndo2db_idi *idi, int type, char *buf){
	char *newbuf=NULL;
	int mbuf_used=NDO_TRUE;

	if(idi==NULL)
		return NDO_ERROR;

	/* escape data if necessary */
	switch(type){

	case NDO_DATA_ACKAUTHOR:
	case NDO_DATA_ACKDATA:
	case NDO_DATA_AUTHORNAME:
	case NDO_DATA_CHECKCOMMAND:
	case NDO_DATA_COMMANDARGS:
	case NDO_DATA_COMMANDLINE:
	case NDO_DATA_COMMANDSTRING:
	case NDO_DATA_COMMENT:
	case NDO_DATA_EVENTHANDLER:
	case NDO_DATA_GLOBALHOSTEVENTHANDLER:
	case NDO_DATA_GLOBALSERVICEEVENTHANDLER:
	case NDO_DATA_HOST:
	case NDO_DATA_LOGENTRY:
	case NDO_DATA_OUTPUT:
	case NDO_DATA_PERFDATA:
	case NDO_DATA_SERVICE:
	case NDO_DATA_PROGRAMNAME:
	case NDO_DATA_PROGRAMVERSION:
	case NDO_DATA_PROGRAMDATE:

	case NDO_DATA_COMMANDNAME:
	case NDO_DATA_CONTACTADDRESS:
	case NDO_DATA_CONTACTALIAS:
	case NDO_DATA_CONTACTGROUP:
	case NDO_DATA_CONTACTGROUPALIAS:
	case NDO_DATA_CONTACTGROUPMEMBER:
	case NDO_DATA_CONTACTGROUPNAME:
	case NDO_DATA_CONTACTNAME:
	case NDO_DATA_DEPENDENTHOSTNAME:
	case NDO_DATA_DEPENDENTSERVICEDESCRIPTION:
	case NDO_DATA_EMAILADDRESS:
	case NDO_DATA_HOSTADDRESS:
	case NDO_DATA_HOSTALIAS:
	case NDO_DATA_HOSTCHECKCOMMAND:
	case NDO_DATA_HOSTCHECKPERIOD:
	case NDO_DATA_HOSTEVENTHANDLER:
	case NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS:
	case NDO_DATA_HOSTGROUPALIAS:
	case NDO_DATA_HOSTGROUPMEMBER:
	case NDO_DATA_HOSTGROUPNAME:
	case NDO_DATA_HOSTNAME:
	case NDO_DATA_HOSTNOTIFICATIONCOMMAND:
	case NDO_DATA_HOSTNOTIFICATIONPERIOD:
	case NDO_DATA_PAGERADDRESS:
	case NDO_DATA_PARENTHOST:
	case NDO_DATA_SERVICECHECKCOMMAND:
	case NDO_DATA_SERVICECHECKPERIOD:
	case NDO_DATA_SERVICEDESCRIPTION:
	case NDO_DATA_SERVICEEVENTHANDLER:
	case NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS:
	case NDO_DATA_SERVICEGROUPALIAS:
	case NDO_DATA_SERVICEGROUPMEMBER:
	case NDO_DATA_SERVICEGROUPNAME:
	case NDO_DATA_SERVICENOTIFICATIONCOMMAND:
	case NDO_DATA_SERVICENOTIFICATIONPERIOD:
	case NDO_DATA_TIMEPERIODALIAS:
	case NDO_DATA_TIMEPERIODNAME:
	case NDO_DATA_TIMERANGE:

	case NDO_DATA_ACTIONURL:
	case NDO_DATA_ICONIMAGE:
	case NDO_DATA_ICONIMAGEALT:
	case NDO_DATA_NOTES:
	case NDO_DATA_NOTESURL:
	case NDO_DATA_CUSTOMVARIABLE:
	case NDO_DATA_CONTACT:

		/* strings are escaped when they arrive */
		if(buf==NULL)
			newbuf=strdup("");
		else
			newbuf=strdup(buf);
		ndo_unescape_buffer(newbuf);
		break;

	default:

		/* data hasn't been escaped */
		if(buf==NULL)
			newbuf=strdup("");
		else
			newbuf=strdup(buf);
		break;
	        }

	/* check for errors */
	if(newbuf==NULL){
#ifdef DEBUG_NDO2DB
		printf("ALLOCATION ERROR\n");
#endif
		return NDO_ERROR;
	        }

	/* store the buffered data */
	switch(type){

	/* special case for data items that may appear multiple times */
	case NDO_DATA_CONTACTGROUP:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONTACTGROUP,newbuf);
		break;
	case NDO_DATA_CONTACTGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONTACTGROUPMEMBER,newbuf);
		break;
	case NDO_DATA_SERVICEGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_SERVICEGROUPMEMBER,newbuf);
		break;
	case NDO_DATA_HOSTGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_HOSTGROUPMEMBER,newbuf);
		break;
	case NDO_DATA_SERVICENOTIFICATIONCOMMAND:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND,newbuf);
		break;
	case NDO_DATA_HOSTNOTIFICATIONCOMMAND:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND,newbuf);
		break;
	case NDO_DATA_CONTACTADDRESS:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONTACTADDRESS,newbuf);
		break;
	case NDO_DATA_TIMERANGE:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_TIMERANGE,newbuf);
		break;
	case NDO_DATA_PARENTHOST:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_PARENTHOST,newbuf);
		break;
	case NDO_DATA_CONFIGFILEVARIABLE:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONFIGFILEVARIABLE,newbuf);
		break;
	case NDO_DATA_CONFIGVARIABLE:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONFIGVARIABLE,newbuf);
		break;
	case NDO_DATA_RUNTIMEVARIABLE:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_RUNTIMEVARIABLE,newbuf);
		break;
	case NDO_DATA_CUSTOMVARIABLE:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CUSTOMVARIABLE,newbuf);
		break;
	case NDO_DATA_CONTACT:
		ndo2db_add_input_data_mbuf(idi,type,NDO2DB_MBUF_CONTACT,newbuf);
		break;

	/* NORMAL DATA */
	/* normal data items appear only once per data type */
	default:

		mbuf_used=NDO_FALSE;

		/* if there was already a matching item, discard the old one */
		if(idi->buffered_input[type]!=NULL){
			free(idi->buffered_input[type]);
			idi->buffered_input[type]=NULL;
		        }

		/* save buffered item */
		idi->buffered_input[type]=newbuf;
	        }

	return NDO_OK;
        }



int ndo2db_add_input_data_mbuf(ndo2db_idi *idi, int type, int mbuf_slot, char *buf){
	int allocation_chunk=80;
	char **newbuffer=NULL;

	if(idi==NULL || buf==NULL)
		return NDO_ERROR;

	if(mbuf_slot>=NDO2DB_MAX_MBUF_ITEMS)
		return NDO_ERROR;

	/* create buffer */
	if(idi->mbuf[mbuf_slot].buffer==NULL){
#ifdef NDO2DB_DEBUG_MBUF
		mbuf_bytes_allocated+=sizeof(char *)*allocation_chunk;
		printf("MBUF INITIAL ALLOCATION (MBUF = %lu bytes)\n",mbuf_bytes_allocated);
#endif
		idi->mbuf[mbuf_slot].buffer=(char **)malloc(sizeof(char *)*allocation_chunk);
		idi->mbuf[mbuf_slot].allocated_lines+=allocation_chunk;
	        }

	/* expand buffer */
	if(idi->mbuf[mbuf_slot].used_lines==idi->mbuf[mbuf_slot].allocated_lines){
		newbuffer=(char **)realloc(idi->mbuf[mbuf_slot].buffer,sizeof(char *)*(idi->mbuf[mbuf_slot].allocated_lines+allocation_chunk));
		if(newbuffer==NULL)
			return NDO_ERROR;
#ifdef NDO2DB_DEBUG_MBUF
		mbuf_bytes_allocated+=sizeof(char *)*allocation_chunk;
		printf("MBUF RESIZED (MBUF = %lu bytes)\n",mbuf_bytes_allocated);
#endif
		idi->mbuf[mbuf_slot].buffer=newbuffer;
		idi->mbuf[mbuf_slot].allocated_lines+=allocation_chunk;
	        }

	/* store the data */
	if(idi->mbuf[mbuf_slot].buffer){
		idi->mbuf[mbuf_slot].buffer[idi->mbuf[mbuf_slot].used_lines]=buf;
		idi->mbuf[mbuf_slot].used_lines++;
	        }
	else
		return NDO_ERROR;

	return NDO_OK;
        }



int ndo2db_end_input_data(ndo2db_idi *idi){
	int result=NDO_OK;
	int x,y;

	if(idi==NULL)
		return NDO_ERROR;

	/* update db stats occassionally */
	if(ndo2db_db_last_checkin_time<(time(NULL)-60))
		ndo2db_db_checkin(idi);

#ifdef DEBUG_NDO2DB2
	printf("HANDLING TYPE: %d\n",idi->current_input_data);
#endif

	switch(idi->current_input_data){

	/* archived log entries */
	case NDO2DB_INPUT_DATA_LOGENTRY:
		result=ndo2db_handle_logentry(idi);
		break;

	/* realtime Nagios data */
	case NDO2DB_INPUT_DATA_PROCESSDATA:
		result=ndo2db_handle_processdata(idi);
		break;
	case NDO2DB_INPUT_DATA_TIMEDEVENTDATA:
		result=ndo2db_handle_timedeventdata(idi);
		break;
	case NDO2DB_INPUT_DATA_LOGDATA:
		result=ndo2db_handle_logdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA:
		result=ndo2db_handle_systemcommanddata(idi);
		break;
	case NDO2DB_INPUT_DATA_EVENTHANDLERDATA:
		result=ndo2db_handle_eventhandlerdata(idi);
		break;
	case NDO2DB_INPUT_DATA_NOTIFICATIONDATA:
		result=ndo2db_handle_notificationdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICECHECKDATA:
		result=ndo2db_handle_servicecheckdata(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTCHECKDATA:
		result=ndo2db_handle_hostcheckdata(idi);
		break;
	case NDO2DB_INPUT_DATA_COMMENTDATA:
		result=ndo2db_handle_commentdata(idi);
		break;
	case NDO2DB_INPUT_DATA_DOWNTIMEDATA:
		result=ndo2db_handle_downtimedata(idi);
		break;
	case NDO2DB_INPUT_DATA_FLAPPINGDATA:
		result=ndo2db_handle_flappingdata(idi);
		break;
	case NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA:
		result=ndo2db_handle_programstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTSTATUSDATA:
		result=ndo2db_handle_hoststatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICESTATUSDATA:
		result=ndo2db_handle_servicestatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTSTATUSDATA:
		result=ndo2db_handle_contactstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA:
		result=ndo2db_handle_adaptiveprogramdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA:
		result=ndo2db_handle_adaptivehostdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA:
		result=ndo2db_handle_adaptiveservicedata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA:
		result=ndo2db_handle_adaptivecontactdata(idi);
		break;
	case NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA:
		result=ndo2db_handle_externalcommanddata(idi);
		break;
	case NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA:
		result=ndo2db_handle_aggregatedstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_RETENTIONDATA:
		result=ndo2db_handle_retentiondata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA:
		result=ndo2db_handle_contactnotificationdata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA:
		result=ndo2db_handle_contactnotificationmethoddata(idi);
		break;
	case NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA:
		result=ndo2db_handle_acknowledgementdata(idi);
		break;
	case NDO2DB_INPUT_DATA_STATECHANGEDATA:
		result=ndo2db_handle_statechangedata(idi);
		break;

	/* config file and variable dumps */
	case NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES:
		result=ndo2db_handle_configfilevariables(idi,0);
		break;
	case NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES:
		result=ndo2db_handle_configfilevariables(idi,1);
		break;
	case NDO2DB_INPUT_DATA_CONFIGVARIABLES:
		result=ndo2db_handle_configvariables(idi);
		break;
	case NDO2DB_INPUT_DATA_RUNTIMEVARIABLES:
		result=ndo2db_handle_runtimevariables(idi);
		break;
	case NDO2DB_INPUT_DATA_CONFIGDUMPSTART:
		result=ndo2db_handle_configdumpstart(idi);
		break;
	case NDO2DB_INPUT_DATA_CONFIGDUMPEND:
		result=ndo2db_handle_configdumpend(idi);
		break;

	/* config definitions */
	case NDO2DB_INPUT_DATA_HOSTDEFINITION:
		result=ndo2db_handle_hostdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION:
		result=ndo2db_handle_hostgroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEDEFINITION:
		result=ndo2db_handle_servicedefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION:
		result=ndo2db_handle_servicegroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION:
		result=ndo2db_handle_hostdependencydefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION:
		result=ndo2db_handle_servicedependencydefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION:
		result=ndo2db_handle_hostescalationdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION:
		result=ndo2db_handle_serviceescalationdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_COMMANDDEFINITION:
		result=ndo2db_handle_commanddefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION:
		result=ndo2db_handle_timeperiodefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTDEFINITION:
		result=ndo2db_handle_contactdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION:
		result=ndo2db_handle_contactgroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTEXTINFODEFINITION:
		/* deprecated - merged with host definitions */
		break;
	case NDO2DB_INPUT_DATA_SERVICEEXTINFODEFINITION:
		/* deprecated - merged with service definitions */
		break;

	default:
		break;
	        }

	/* free input memory */
	ndo2db_free_input_memory(idi);

	/* adjust items processed */
	idi->entries_processed++;

	/* perform periodic maintenance... */
	ndo2db_db_perform_maintenance(idi);

	return result;
        }


/* free memory allocated to data input */
int ndo2db_free_input_memory(ndo2db_idi *idi){
	register int x=0;
	register int y=0;

	if(idi==NULL)
		return NDO_ERROR;

	/* free memory allocated to single-instance data buffers */
	if(idi->buffered_input){

		for(x=0;x<NDO_MAX_DATA_TYPES;x++){
			if(idi->buffered_input[x])
				free(idi->buffered_input[x]);
			idi->buffered_input[x]=NULL;
	                }

		free(idi->buffered_input);
		idi->buffered_input=NULL;
	        }

	/* free memory allocated to multi-instance data buffers */
	if(idi->mbuf){
		for(x=0;x<NDO2DB_MAX_MBUF_ITEMS;x++){
			if(idi->mbuf[x].buffer){
				for(y=0;y<idi->mbuf[x].used_lines;y++){
					if(idi->mbuf[x].buffer[y]){
						free(idi->mbuf[x].buffer[y]);
						idi->mbuf[x].buffer[y]=NULL;
						}
					}
				free(idi->mbuf[x].buffer);
				idi->mbuf[x].buffer=NULL;
				}
			idi->mbuf[x].used_lines=0;
			idi->mbuf[x].allocated_lines=0;
			}
		}

	return NDO_OK;
	}


/* free memory allocated to connection */
int ndo2db_free_connection_memory(ndo2db_idi *idi){

	if(idi->instance_name){
		free(idi->instance_name);
		idi->instance_name=NULL;
		}
	if(idi->agent_name){
		free(idi->agent_name);
		idi->agent_name=NULL;
		}
	if(idi->agent_version){
		free(idi->agent_version);
		idi->agent_version=NULL;
		}
	if(idi->disposition){
		free(idi->disposition);
		idi->disposition=NULL;
		}
	if(idi->connect_source){
		free(idi->connect_source);
		idi->connect_source=NULL;
		}
	if(idi->connect_type){
		free(idi->connect_type);
		idi->connect_type=NULL;
		}

	return NDO_OK;
	}
	


/****************************************************************************/
/* DATA TYPE CONVERTION ROUTINES                                            */
/****************************************************************************/

int ndo2db_convert_standard_data_elements(ndo2db_idi *idi, int *type, int *flags, int *attr, struct timeval *tstamp){
	int result1=NDO_OK;
	int result2=NDO_OK;
	int result3=NDO_OK;
	int result4=NDO_OK;
	
	result1=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TYPE],type);
	result2=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FLAGS],flags);
	result3=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ATTRIBUTES],attr);
	result4=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_TIMESTAMP],tstamp);

	if(result1==NDO_ERROR || result2==NDO_ERROR || result3==NDO_ERROR || result4==NDO_ERROR) 
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_convert_string_to_int(char *buf, int *i){

	if(buf==NULL)
		return NDO_ERROR;

	*i=atoi(buf);

	return NDO_OK;
        }


int ndo2db_convert_string_to_float(char *buf, float *f){
	char *endptr=NULL;

	if(buf==NULL)
		return NDO_ERROR;

#ifdef HAVE_STRTOF
	*f=strtof(buf,&endptr);
#else
	/* Solaris 8 doesn't have strtof() */
	*f=(float)strtod(buf,&endptr);
#endif

	if(*f==0 && (endptr==buf || errno==ERANGE))
		return NDO_ERROR;
	if(errno==ERANGE)
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_convert_string_to_double(char *buf, double *d){
	char *endptr=NULL;

	if(buf==NULL)
		return NDO_ERROR;

	*d=strtod(buf,&endptr);

	if(*d==0 && (endptr==buf || errno==ERANGE))
		return NDO_ERROR;
	if(errno==ERANGE)
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_convert_string_to_long(char *buf, long *l){
	char *endptr=NULL;

	if(buf==NULL)
		return NDO_ERROR;

	*l=strtol(buf,&endptr,0);

	if(*l==LONG_MAX && errno==ERANGE)
		return NDO_ERROR;
	if(*l==0L && endptr==buf)
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_convert_string_to_unsignedlong(char *buf, unsigned long *ul){
	char *endptr=NULL;

	if(buf==NULL)
		return NDO_ERROR;

	*ul=strtoul(buf,&endptr,0);

	if(*ul==ULONG_MAX && errno==ERANGE)
		return NDO_ERROR;
	if(*ul==0L && endptr==buf)
		return NDO_ERROR;

	return NDO_OK;
        }


int ndo2db_convert_string_to_timeval(char *buf, struct timeval *tv){
	char *newbuf=NULL;
	char *ptr=NULL;
	int result=NDO_OK;

	if(buf==NULL)
		return NDO_ERROR;

	tv->tv_sec=(time_t)0L;
	tv->tv_usec=(suseconds_t)0L;

	if((newbuf=strdup(buf))==NULL)
		return NDO_ERROR;

	ptr=strtok(newbuf,".");
	if((result=ndo2db_convert_string_to_unsignedlong(ptr,(unsigned long *)&tv->tv_sec))==NDO_OK){
		ptr=strtok(NULL,"\n");
		result=ndo2db_convert_string_to_unsignedlong(ptr,(unsigned long *)&tv->tv_usec);
	        }

	free(newbuf);

	if(result==NDO_ERROR)
		return NDO_ERROR;

	return NDO_OK;
        }



/****************************************************************************/
/* LOGGING ROUTINES                                                         */
/****************************************************************************/

/* opens the debug log for writing */
int ndo2db_open_debug_log(void){

	/* don't do anything if we're not debugging */
	if(ndo2db_debug_level==NDO2DB_DEBUGL_NONE)
		return NDO_OK;

	if((ndo2db_debug_file_fp=fopen(ndo2db_debug_file,"a+"))==NULL)
		return NDO_ERROR;

	return NDO_OK;
	}


/* closes the debug log */
int ndo2db_close_debug_log(void){

	if(ndo2db_debug_file_fp!=NULL)
		fclose(ndo2db_debug_file_fp);
	
	ndo2db_debug_file_fp=NULL;

	return NDO_OK;
	}


/* write to the debug log */
int ndo2db_log_debug_info(int level, int verbosity, const char *fmt, ...){
	va_list ap;
	char *temp_path=NULL;
	struct timeval current_time;

	if(!(ndo2db_debug_level==NDO2DB_DEBUGL_ALL || (level & ndo2db_debug_level)))
		return NDO_OK;

	if(verbosity>ndo2db_debug_verbosity)
		return NDO_OK;

	if(ndo2db_debug_file_fp==NULL)
		return NDO_ERROR;

	/* write the timestamp */
	gettimeofday(&current_time,NULL);
	fprintf(ndo2db_debug_file_fp,"[%lu.%06lu] [%03d.%d] [pid=%lu] ",current_time.tv_sec,current_time.tv_usec,level,verbosity,(unsigned long)getpid());

	/* write the data */
	va_start(ap,fmt);
	vfprintf(ndo2db_debug_file_fp,fmt,ap);
	va_end(ap);

	/* flush, so we don't have problems tailing or when fork()ing */
	fflush(ndo2db_debug_file_fp);

	/* if file has grown beyond max, rotate it */
	if((unsigned long)ftell(ndo2db_debug_file_fp)>ndo2db_max_debug_file_size && ndo2db_max_debug_file_size>0L){

		/* close the file */
		ndo2db_close_debug_log();
		
		/* rotate the log file */
		asprintf(&temp_path,"%s.old",ndo2db_debug_file);
		if(temp_path){

			/* unlink the old debug file */
			unlink(temp_path);

			/* rotate the debug file */
			my_rename(ndo2db_debug_file,temp_path);

			/* free memory */
			my_free(temp_path);
			}

		/* open a new file */
		ndo2db_open_debug_log();
		}

	return NDO_OK;
	}

