/***************************************************************
 * IO.C - NDO I/O Functions
 *
 * Copyright (c) 2005-2006 Ethan Galstad 
 * First Written: 05-20-2006
 *
 *
 **************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"




/**************************************************************/
/****** MMAP()'ED FILE FUNCTIONS ******************************/
/**************************************************************/

/* open a file read-only via mmap() */
ndo_mmapfile *ndo_mmap_fopen(char *filename){
	ndo_mmapfile *new_mmapfile;
	int fd;
	void *mmap_buf;
	struct stat statbuf;
	int mode=O_RDONLY;

	/* allocate memory */
	if((new_mmapfile=(ndo_mmapfile *)malloc(sizeof(ndo_mmapfile)))==NULL)
		return NULL;

	/* open the file */
	if((fd=open(filename,mode))==-1){
		free(new_mmapfile);
		return NULL;
	        }

	/* get file info */
	if((fstat(fd,&statbuf))==-1){
		close(fd);
		free(new_mmapfile);
		return NULL;
	        }

	/* mmap() the file */
	if((mmap_buf=(void *)mmap(0,statbuf.st_size,PROT_READ,MAP_PRIVATE,fd,0))==MAP_FAILED){
		close(fd);
		free(new_mmapfile);
		return NULL;
	        }

	/* populate struct info for later use */
	/*new_mmapfile->path=strdup(filename);*/
	new_mmapfile->path=NULL;
	new_mmapfile->fd=fd;
	new_mmapfile->file_size=(unsigned long)(statbuf.st_size);
	new_mmapfile->current_position=0L;
	new_mmapfile->current_line=0L;
	new_mmapfile->mmap_buf=mmap_buf;

	return new_mmapfile;
        }


/* close a file originally opened via mmap() */
int ndo_mmap_fclose(ndo_mmapfile *temp_mmapfile){

	if(temp_mmapfile==NULL)
		return NDO_ERROR;

	/* un-mmap() the file */
	munmap(temp_mmapfile->mmap_buf,temp_mmapfile->file_size);

	/* close the file */
	close(temp_mmapfile->fd);

	/* free memory */
	if(temp_mmapfile->path!=NULL)
		free(temp_mmapfile->path);
	free(temp_mmapfile);
	
	return NDO_OK;
        }


/* gets one line of input from an mmap()'ed file */
char *ndo_mmap_fgets(ndo_mmapfile *temp_mmapfile){
	char *buf=NULL;
	unsigned long x=0L;
	int len=0;

	if(temp_mmapfile==NULL)
		return NULL;

	/* we've reached the end of the file */
	if(temp_mmapfile->current_position>=temp_mmapfile->file_size)
		return NULL;

	/* find the end of the string (or buffer) */
	for(x=temp_mmapfile->current_position;x<temp_mmapfile->file_size;x++){
		if(*((char *)(temp_mmapfile->mmap_buf)+x)=='\n'){
			x++;
			break;
			}
	        }

	/* calculate length of line we just read */
	len=(int)(x-temp_mmapfile->current_position);

	/* allocate memory for the new line */
	if((buf=(char *)malloc(len+1))==NULL)
		return NULL;

	/* copy string to newly allocated memory and terminate the string */
	memcpy(buf,((char *)(temp_mmapfile->mmap_buf)+temp_mmapfile->current_position),len);
	buf[len]='\x0';

	/* update the current position */
	temp_mmapfile->current_position=x;

	/* increment the current line */
	temp_mmapfile->current_line++;

	return buf;
        }




/**************************************************************/
/****** SOCKET FUNCTIONS **************************************/
/**************************************************************/


/* opens data sink */
int ndo_sink_open(char *name, int fd, int type, int port, int flags, int *nfd){
	struct sockaddr_un server_address_u;
	struct sockaddr_in server_address_i;
	struct hostent *hp=NULL;
	mode_t mode=S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int newfd=0;

	/* use file */
	if(type==NDO_SINK_FILE){
		if((newfd=open(name,flags,mode))==-1)
			return NDO_ERROR;
	        }

	/* use existing file descriptor */
	else if(type==NDO_SINK_FD){
		if(fd<0)
			return NDO_ERROR;
		else
			newfd=fd;
	        }

	/* we are sending output to a unix domain socket */
	else if(type==NDO_SINK_UNIXSOCKET){

		if(name==NULL)
			return NDO_ERROR;

		/* create a socket */
		if(!(newfd=socket(PF_UNIX,SOCK_STREAM,0)))
			return NDO_ERROR;

		/* copy the socket address/path */
		strncpy(server_address_u.sun_path,name,sizeof(server_address_u.sun_path)); 
		server_address_u.sun_family=AF_UNIX; 

		/* connect to the socket */
		if((connect(newfd,(struct sockaddr *)&server_address_u,SUN_LEN(&server_address_u)))){
			close(newfd);
			return NDO_ERROR;
			}
	        }

	/* we are sending output to a TCP socket */
	else if(type==NDO_SINK_TCPSOCKET){

		if(name==NULL)
			return NDO_ERROR;
		
		/* clear the address */
		bzero((char *)&server_address_i,sizeof(server_address_i));

		/* try to bypass using a DNS lookup if this is just an IP address */
		if(!ndo_inet_aton(name,&server_address_i.sin_addr)){

			/* else do a DNS lookup */
			if((hp=gethostbyname((const char *)name))==NULL)
				return NDO_ERROR;

			memcpy(&server_address_i.sin_addr,hp->h_addr,hp->h_length);
	                }

		/* create a socket */
		if(!(newfd=socket(PF_INET,SOCK_STREAM,0)))
			return NDO_ERROR;

		/* copy the host/ip address and port */
		server_address_i.sin_family=AF_INET; 
		server_address_i.sin_port=htons(port);

		/* connect to the socket */
		if((connect(newfd,(struct sockaddr *)&server_address_i,sizeof(server_address_i)))){
			close(newfd);
			return NDO_ERROR;
		        }
	        }

	/* unknown sink type */
	else
		return NDO_ERROR;

	/* save the new file descriptor */
	*nfd=newfd;

	return NDO_OK;
        }


/* writes to data sink */
int ndo_sink_write(int fd, char *buf, int buflen){
	int tbytes=0;
	int result=0;

	if(buf==NULL)
		return NDO_ERROR;
	if(buflen<=0)
		return 0;

	while(tbytes<buflen){

		/* try to write everything we have left */
		result=write(fd,buf+tbytes,buflen-tbytes);

		/* some kind of error occurred */
		if(result==-1){

			/* unless we encountered a recoverable error, bail out */
			if(errno!=EAGAIN && errno!=EINTR)
				return NDO_ERROR;
		        }

		/* update the number of bytes we've written */
		tbytes+=result;
	        }

	return tbytes;
        }


/* writes a newline to data sink */
int ndo_sink_write_newline(int fd){

	return ndo_sink_write(fd,"\n",1);
        }


/* flushes data sink */
int ndo_sink_flush(int fd){

	/* flush sink */
	fsync(fd);

	return NDO_OK;
        }


/* closes data sink */
int ndo_sink_close(int fd){

	/* no need to close STDOUT */
	if(fd==STDOUT_FILENO)
		return NDO_OK;

	/* close the socket */
	shutdown(fd,2);
	close(fd);

	return NDO_OK;
        }


/* This code was taken from Fyodor's nmap utility, which was originally taken from
   the GLIBC 2.0.6 libraries because Solaris doesn't contain the inet_aton() funtion. */
int ndo_inet_aton(register const char *cp, struct in_addr *addr){
	register unsigned int val;	/* changed from u_long --david */
	register int base, n;
	register char c;
	unsigned int parts[4];
	register unsigned int *pp = parts;

	c=*cp;

	for(;;){

		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit((int)c))
			return (0);
		val=0;
		base=10;

		if(c=='0'){
			c=*++cp;
			if(c=='x'||c=='X')
				base=16,c=*++cp;
			else
				base=8;
		        }

		for(;;){
			if(isascii((int)c) && isdigit((int)c)){
				val=(val*base)+(c -'0');
				c=*++cp;
			        } 
			else if(base==16 && isascii((int)c) && isxdigit((int)c)){
				val=(val<<4) | (c+10-(islower((int)c)?'a':'A'));
				c = *++cp;
			        } 
			else
				break;
		        }

		if(c=='.'){

			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if(pp>=parts+3)
				return (0);
			*pp++=val;
			c=*++cp;
		        } 
		else
			break;
	        }

	/* Check for trailing characters */
	if(c!='\0' && (!isascii((int)c) || !isspace((int)c)))
		return (0);

	/* Concoct the address according to the number of parts specified */
	n=pp-parts+1;
	switch(n){

	case 0:
		return (0);		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if(val>0xffffff)
			return (0);
		val|=parts[0]<<24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if(val>0xffff)
			return (0);
		val|=(parts[0]<< 24) | (parts[1]<<16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if(val>0xff)
			return (0);
		val|=(parts[0]<<24) | (parts[1]<<16) | (parts[2]<<8);
		break;
	        }

	if(addr)
		addr->s_addr=htonl(val);

	return (1);
        }


/******************************************************************/
/************************ STRING FUNCTIONS ************************/
/******************************************************************/

/* strip newline and carriage return characters from end of a string */
void ndo_strip_buffer(char *buffer){
	register int x;
	register int y;

	if(buffer==NULL || buffer[0]=='\x0')
		return;

	/* strip end of string */
	y=(int)strlen(buffer);
	for(x=y-1;x>=0;x--){
		if(buffer[x]=='\n' || buffer[x]=='\r' || buffer[x]==13)
			buffer[x]='\x0';
		else
			break;
	        }

	return;
	}


/* escape special characters in string */
char *ndo_escape_buffer(char *buffer){
	char *newbuf;
	register int x=0;
	register int y=0;
	register int len=0;

	if(buffer==NULL)
		return NULL;

	/* allocate memory for escaped string */
	if((newbuf=(char *)malloc((strlen(buffer)*2)+1))==NULL)
		return NULL;

	/* initialize string */
	newbuf[0]='\x0';

	len=(int)strlen(buffer);
	for(x=0;x<len;x++){
		if(buffer[x]=='\t'){
			newbuf[y++]='\\';
			newbuf[y++]='t';
		        }
		else if(buffer[x]=='\r'){
			newbuf[y++]='\\';
			newbuf[y++]='r';
		        }
		else if(buffer[x]=='\n'){
			newbuf[y++]='\\';
			newbuf[y++]='n';
		        }
		else if(buffer[x]=='\\'){
			newbuf[y++]='\\';
			newbuf[y++]='\\';
		        }
		else
			newbuf[y++]=buffer[x];
	        }

	/* terminate new string */
	newbuf[y++]='\x0';

	return newbuf;
        }


/* unescape special characters in string */
char *ndo_unescape_buffer(char *buffer){
	register int x=0;
	register int y=0;
	register int len=0;

	if(buffer==NULL)
		return NULL;

	len=(int)strlen(buffer);
	for(x=0;x<len;x++){
		if(buffer[x]=='\\'){
			if(buffer[x+1]=='\t')
				buffer[y++]='\t';
			else if(buffer[x+1]=='r')
				buffer[y++]='\r';
			else if(buffer[x+1]=='n')
				buffer[y++]='\n';
			else if(buffer[x+1]=='\\')
				buffer[y++]='\\';
			else
				buffer[y++]=buffer[x+1];
			x++;
		        }
		else
			buffer[y++]=buffer[x];
	        }

	/* terminate string */
	buffer[y++]='\x0';

	return buffer;
        }


