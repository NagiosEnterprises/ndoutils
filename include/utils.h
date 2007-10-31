/************************************************************************
 *
 * UTILS.H - NDO utilities header file
 * Copyright (c) 2005-2007 Ethan Galstad
 * Last Modified: 10-29-2007
 *
 ************************************************************************/

#ifndef _NDO_UTILS_H
#define _NDO_UTILS_H

/* my_free has been freed from bondage as a function */
#define my_free(ptr) { if(ptr) { free(ptr); ptr = NULL; } }


typedef struct ndo_dbuf_struct{
	char *buf;
	unsigned long used_size;
	unsigned long allocated_size;
	unsigned long chunk_size;
        }ndo_dbuf;


int ndo_dbuf_init(ndo_dbuf *,int);
int ndo_dbuf_free(ndo_dbuf *);
int ndo_dbuf_strcat(ndo_dbuf *,char *);


int my_rename(char *,char *);

#endif
