/***************************************************************
 * LOG2NDO.C - Sends archived logs files to NDO2DB daemon
 *
 * Copyright (c) 2005-2007 Ethan Galstad 
 *
 * Last Modified: 10-31-2007
 *
 **************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/protoapi.h"

#define LOG2NDO_VERSION "1.4b7"
#define LOG2NDO_NAME "LOG2NDO"
#define LOG2NDO_DATE "10-31-2007"



int process_arguments(int,char **);

char *source_name=NULL;
char *dest_name=NULL;
char *instance_name=NULL;
int socket_type=NDO_SINK_UNIXSOCKET;
int tcp_port=0;
int show_version=NDO_FALSE;
int show_license=NDO_FALSE;
int show_help=NDO_FALSE;


int main(int argc, char **argv){
	ndo_mmapfile *thefile=NULL;
	char *connection_type=NULL;
	char *input=NULL;
	char *input2=NULL;
	int sd=2;
	char tempbuf[1024];
	int result=0;


	result=process_arguments(argc,argv);

        if(result!=NDO_OK || show_help==NDO_TRUE || show_license==NDO_TRUE || show_version==NDO_TRUE){

		if(result!=NDO_OK)
			printf("Incorrect command line arguments supplied\n");

		printf("\n");
		printf("%s %s\n",LOG2NDO_NAME,LOG2NDO_VERSION);
		printf("Copyright(c) 2005-2007 Ethan Galstad (nagios@nagios.org)\n");
		printf("Last Mofieid: %s\n",LOG2NDO_DATE);
		printf("License: GPL v2\n");
		printf("\n");
		printf("Sends the contents of an archived Nagios or NetSaint log file to STDOUT,\n");
		printf("a TCP socket, or a Unix domain socket in a format that is understood by the\n");
		printf("NDO2DB daemon.\n");
		printf("\n");
		printf("Usage: %s -s <source> -d <dest> -i <instance> [-t <type>] [-p <port>]\n",argv[0]);
		printf("\n");
		printf("<source>   = Name of the Nagios/NetSaint log file to read from.\n");
		printf("<dest>     = If destination is a TCP socket, the address/hostname to connect to.\n");
		printf("             If destination is a Unix domain socket, the path to the socket.\n");
		printf("             If destination is STDOUT (for redirection, etc), a single dash (-).\n");
		printf("<type>     = Specifies the type of destination socket.  Valid values include:\n");
		printf("                 tcp\n");
		printf("                 unix (default)\n");
		printf("<port>     = Port number to connect to if destination is TCP socket.\n");
		printf("\n");

		exit(1);
	        }

	/* send output to STDOUT rather than a socket */
	if(!strcmp(dest_name,"-")){
		sd=STDOUT_FILENO;
		socket_type=NDO_SINK_FD;
	        }

	/* open the file for reading */
	if((thefile=ndo_mmap_fopen(source_name))==NULL){
		perror("Unable to open source file for reading");
		exit(1);
	        }

	/* open the destination */
	if(ndo_sink_open(dest_name,sd,socket_type,tcp_port,0,&sd)==NDO_ERROR){
		ndo_mmap_fclose(thefile);
		exit(1);
	        }


	/***** SEND HEADER INFORMATION *****/

	/* get the connection type string */
	if(socket_type==NDO_SINK_FD || socket_type==NDO_SINK_FILE)
		connection_type=NDO_API_CONNECTION_FILE;
	else if(socket_type==NDO_SINK_TCPSOCKET)
		connection_type=NDO_API_CONNECTION_TCPSOCKET;
	else
		connection_type=NDO_API_CONNECTION_UNIXSOCKET;

	snprintf(tempbuf,sizeof(tempbuf)-1
		 ,"%s\n%s: %d\n%s: %s\n%s: %s\n%s: %lu\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s\n\n"
		 ,NDO_API_HELLO
		 ,NDO_API_PROTOCOL
		 ,NDO_API_PROTOVERSION
		 ,NDO_API_AGENT
		 ,LOG2NDO_NAME
		 ,NDO_API_AGENTVERSION
		 ,LOG2NDO_VERSION
		 ,NDO_API_STARTTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_DISPOSITION
		 ,NDO_API_DISPOSITION_ARCHIVED
		 ,NDO_API_CONNECTION
		 ,connection_type
		 ,NDO_API_CONNECTTYPE
		 ,NDO_API_CONNECTTYPE_INITIAL
		 ,NDO_API_INSTANCENAME
		 ,(instance_name==NULL)?"default":instance_name
		 ,NDO_API_STARTDATADUMP
		);
	tempbuf[sizeof(tempbuf)-1]='\x0';
	ndo_sink_write(sd,tempbuf,strlen(tempbuf));



	/***** SEND THE LOG CONTENTS *****/

	while((input=ndo_mmap_fgets(thefile))){

		/* strip and escape log entry */
		ndo_strip_buffer(input);
		if((input2=ndo_escape_buffer(input))==NULL){
			free(input);
			input2=NULL;
			continue;
		        }

		/* write log entry header */
		snprintf(tempbuf,sizeof(tempbuf)-1
			 ,"%d:\n%d=%s\n%d\n\n"
			 ,NDO_API_LOGENTRY
			 ,NDO_DATA_LOGENTRY
			 ,input2
			 ,NDO_API_ENDDATA
			);
		tempbuf[sizeof(tempbuf)-1]='\x0';
		ndo_sink_write(sd,tempbuf,strlen(tempbuf));

		/* free allocated memory */
		free(input);
		free(input2);
		input=NULL;
		input2=NULL;
	        }



	/***** SAY GOODBYE *****/

	snprintf(tempbuf,sizeof(tempbuf)-1,"\n%d\n%s: %lu\n%s\n"
		 ,NDO_API_ENDDATADUMP
		 ,NDO_API_ENDTIME
		 ,(unsigned long)time(NULL)
		 ,NDO_API_GOODBYE
		);
	tempbuf[sizeof(tempbuf)-1]='\x0';
	ndo_sink_write(sd,tempbuf,strlen(tempbuf));



	/* close the destination */
	ndo_sink_flush(sd);
	ndo_sink_close(sd);

	/* close the file */
	ndo_mmap_fclose(thefile);

	return 0;
        }


/* process command line arguments */
int process_arguments(int argc, char **argv){
	char optchars[32];
	int c=1;

#ifdef HAVE_GETOPT_H
	int option_index=0;
	static struct option long_options[]={
		{"source", required_argument, 0, 's'},
		{"dest", required_argument, 0, 'd'},
		{"instance", required_argument, 0, 'i'},
		{"type", required_argument, 0, 't'},
		{"port", required_argument, 0, 'p'},
		{"help", no_argument, 0, 'h'},
		{"license", no_argument, 0, 'l'},
		{"version", no_argument, 0, 'V'},
		{0, 0, 0, 0}
                };
#endif

	/* no options were supplied */
	if(argc<2){
		show_help=NDO_TRUE;
		return NDO_OK;
	        }

	snprintf(optchars,sizeof(optchars),"s:d:i:t:p:hlV");

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
			show_help=NDO_TRUE;
			break;
		case 'V':
			show_version=NDO_TRUE;
			break;
		case 'l':
			show_license=NDO_TRUE;
			break;
		case 't':
			if(!strcmp(optarg,"tcp"))
				socket_type=NDO_SINK_TCPSOCKET;
			else if(!strcmp(optarg,"unix"))
				socket_type=NDO_SINK_UNIXSOCKET;
			else
				return NDO_ERROR;
			break;
		case 'p':
			tcp_port=atoi(optarg);
			if(tcp_port<=0)
				return NDO_ERROR;
			break;
		case 's':
			source_name=strdup(optarg);
			break;
		case 'd':
			dest_name=strdup(optarg);
			break;
		case 'i':
			instance_name=strdup(optarg);
			break;
		default:
			return NDO_ERROR;
			break;
		        }
	        }

	/* make sure required args were supplied */
	if((source_name==NULL || dest_name==NULL || instance_name==NULL) && show_help==NDO_FALSE && show_version==NDO_FALSE  && show_license==NDO_FALSE)
		return NDO_ERROR;

	return NDO_OK;
        }

