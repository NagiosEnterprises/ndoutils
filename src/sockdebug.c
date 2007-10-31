/***************************************************************
 * SOCKDEBUG.C - Socket Debugging Utility
 *
 * Copyright (c) 2005-2007 Ethan Galstad 
 *
 * First Written: 05-13-2005
 * Last Modified: 10-31-2007
 *
 **************************************************************/

#include "../include/config.h"

#define SOCKDEBUG_VERSION "1.4b7"
#define SOCKDEBUG_NAME "SOCKDEBUG"
#define SOCKDEBUG_DATE "10-31-2007"


int cleanup_socket(int,char *);
void sighandler(int);

int sd=0;
char *socketname=NULL;

int main(int argc, char **argv){
	int new_sd=0;
#ifdef HANDLE_MULTI
	int pid=0;
#endif
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	socklen_t client_address_length;
	char buf[1];


	if(argc<2){
		printf("%s %s\n",SOCKDEBUG_NAME,SOCKDEBUG_VERSION);
		printf("Copyright(c) 2005-2007 Ethan Galstad\n");
		printf("Last Modified: %s\n",SOCKDEBUG_DATE);
		printf("License: GPL v2\n");
		printf("\n");
		printf("Usage: %s <socketname>\n",argv[0]);
		printf("\n");
		printf("Creates a UNIX domain socket with name <socketname>, waits for a\n");
		printf("client to connect and then prints all received data to stdout.\n");
		printf("Only one client connection is processed at any given time.\n");
		exit(1);
	        }

	/* initialize signal handling */
	signal(SIGQUIT,sighandler);
	signal(SIGTERM,sighandler);
	signal(SIGINT,sighandler);

	socketname=strdup(argv[1]);
	if(socketname==NULL){
		perror("Could not dup socket name");
		exit(1);
	         }

	/* create a socket */
	if(!(sd=socket(AF_UNIX,SOCK_STREAM,0))){
		perror("Cannot create socket");
		exit(1);
	        }

	/* bind the socket */
	strncpy(server_address.sun_path,socketname,sizeof(server_address.sun_path)); 
	server_address.sun_family=AF_UNIX; 
	if((bind(sd,(struct sockaddr *)&server_address,SUN_LEN(&server_address)))){
		perror("Could not bind socket");
		exit(1);
	        }

	/* listen for connections */
	if((listen(sd,1))){
		perror("Cannot listen on socket");
		cleanup_socket(sd,socketname);
		exit(1);
	        }

	client_address_length=(socklen_t)sizeof(client_address);

	/* accept connections... */
	while(1){

		if((new_sd=accept(sd,(struct sockaddr *)&client_address,(socklen_t *)&client_address_length))<0){
			perror("Accept error");
			cleanup_socket(sd,socketname);
			exit(1);
		        }

#ifdef HANDLE_MULTI
		/* fork... */
		pid=fork();

		switch(pid){
		case -1:
			perror("Fork error");
			cleanup_socket(sd,socketname);
			exit(1);
			break;

		case 0:
			/* print all data from socket to the screen */
			while((read(new_sd,buf,sizeof(buf)))){
				printf("%c",buf[0]);
			        }
			exit(0);
			break;

		default:
			close(new_sd);
			break;
		        }

#else
		/* print all data from socket to the screen */
		while((read(new_sd,buf,sizeof(buf)))){
			printf("%c",buf[0]);
		        }

		close(new_sd);
#endif
		
	        }

	/* cleanup after ourselves */
	cleanup_socket(sd,socketname);

	return 0;
        }


int cleanup_socket(int s, char *f){

	/* close the socket */
	shutdown(s,2);
	close(s);

	/* unlink the file */
	unlink(f);

	return 0;
        }



void sighandler(int sig){

	/* close the socket */
	shutdown(sd,2);
	close(sd);

	/* unlink the file */
	unlink(socketname);

	exit(0);

	return;
        }
