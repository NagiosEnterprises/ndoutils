/************************************************************************
 *
 * IO.H - Common I/O Functions
 * Copyright (c) 2005 Ethan Galstad
 * Last Modified: 05-19-2005
 *
 ************************************************************************/

#ifndef _NDO_IO_H
#define _NDO_IO_H

#include "config.h"


#define NDO_SINK_FILE         0
#define NDO_SINK_FD           1
#define NDO_SINK_UNIXSOCKET   2
#define NDO_SINK_TCPSOCKET    3

#define NDO_DEFAULT_TCP_PORT  5668


/* MMAPFILE structure - used for reading files via mmap() */
typedef struct ndo_mmapfile_struct{
	char *path;
	int mode;
	int fd;
	unsigned long file_size;
	unsigned long current_position;
	unsigned long current_line;
	void *mmap_buf;
        }ndo_mmapfile;


ndo_mmapfile *ndo_mmap_fopen(char *);
int ndo_mmap_fclose(ndo_mmapfile *);
char *ndo_mmap_fgets(ndo_mmapfile *);

int ndo_sink_open(char *,int,int,int,int,int *);
int ndo_sink_write(int,char *,int);
int ndo_sink_write_newline(int);
int ndo_sink_flush(int);
int ndo_sink_close(int);
int ndo_inet_aton(register const char *,struct in_addr *);

void ndo_strip_buffer(char *);
char *ndo_escape_buffer(char *);
char *ndo_unescape_buffer(char *);

#endif
