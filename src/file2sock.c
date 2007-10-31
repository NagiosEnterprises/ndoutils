/***************************************************************
 * FILE2SOCK.c - File to Socket Dump Utility
 *
 * Copyright (c) 20052-2007 Ethan Galstad 
 * License: GPL v2
 *
 * First Written: 05-13-2005
 * Last Modified: 10-31-2007
 *
 **************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"

#define FILE2SOCK_VERSION "1.4b7"
#define FILE2SOCK_NAME "FILE2SOCK"
#define FILE2SOCK_DATE "10-31-2007"


int process_arguments(int,char **);


char *source_name=NULL;
char *dest_name=NULL;
int socket_type=NDO_SINK_UNIXSOCKET;
int tcp_port=0;
int show_version=NDO_FALSE;
int show_license=NDO_FALSE;
int show_help=NDO_FALSE;

int main(int argc, char **argv){
	int sd=0;
	int fd=0;
	char ch[1];
	int result=0;


	result=process_arguments(argc,argv);

        if(result!=NDO_OK || show_help==NDO_TRUE || show_license==NDO_TRUE || show_version==NDO_TRUE){

		if(result!=NDO_OK)
			printf("Incorrect command line arguments supplied\n");

		printf("\n");
		printf("%s %s\n",FILE2SOCK_NAME,FILE2SOCK_VERSION);
		printf("Copyright(c) 2005-2007 Ethan Galstad (nagios@nagios.org)\n");
		printf("Last Modified: %s\n",FILE2SOCK_DATE);
		printf("License: GPL v2\n");
		printf("\n");
		printf("Sends the contents of a file to a TCP or UNIX domain socket.  The contents of\n");
		printf("the file are sent in their original format - no conversion, encapsulation, or\n");
		printf("other processing is done before sending the contents to the destination socket.\n");
		printf("\n");
		printf("Usage: %s -s <source> -d <dest> [-t <type>] [-p <port>]\n",argv[0]);
		printf("\n");
		printf("<source>   = Name of the file to read from.  Use '-' to read from stdin.\n");
		printf("<dest>     = If destination is a TCP socket, the address/hostname to connect to.\n");
		printf("             If destination is a Unix domain socket, the path to the socket.\n");
		printf("<type>     = Specifies the type of destination socket.  Valid values include:\n");
		printf("                 tcp\n");
		printf("                 unix (default)\n");
		printf("<port>     = Port number to connect to if destination is TCP socket.\n");
		printf("\n");

		exit(1);
	        }

	/* open the source file for reading */
	if(!strcmp(source_name,"-"))
		fd=STDIN_FILENO;
	else if((fd=open(source_name,O_RDONLY))==-1){
		perror("Unable to open source file for reading");
		exit(1);
	        }

	/* open data sink */
	if(ndo_sink_open(dest_name,sd,socket_type,tcp_port,0,&sd)==NDO_ERROR){
		perror("Cannot open destination socket");
		close(fd);
		exit(1);
	        }

	/* we're reading from stdin... */
#ifdef USE_SENDFILE
	if(fd==STDIN_FILENO){
#endif
		while((read(fd,&ch,1))){
			if(write(sd,&ch,1)==-1){
				perror("Error while writing to destination socket");
				result=1;
				break;
			        }
		        }
#ifdef USE_SENDFILE
	        }

	/* we're reading from a standard file... */
	else{

		/* get file size info */
		if(fstat(fd,&stat_buf)==-1){
			perror("fstat() error");
			result=1;
		        }

		/* send the file contents to the socket */
		else if(sendfile(sd,fd,&offset,stat_buf.st_size)==-1){
			perror("sendfile() error");
			result=1;
		        }
	        }
#endif

	/* close the data sink */
	ndo_sink_flush(sd);
	ndo_sink_close(sd);

	/* close the source file */
	close(fd);

	return result;
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

	snprintf(optchars,sizeof(optchars),"s:d:t:p:hlV");

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
		default:
			return NDO_ERROR;
			break;
		        }
	        }

	/* make sure required args were supplied */
	if((source_name==NULL || dest_name==NULL) && show_help==NDO_FALSE && show_version==NDO_FALSE  && show_license==NDO_FALSE)
		return NDO_ERROR;

	return NDO_OK;
        }

