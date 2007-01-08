/************************************************************************
 *
 * UTILS.H - NDO utilities header file
 * Copyright (c) 2005 Ethan Galstad
 * Last Modified: 11-22-2005
 *
 ************************************************************************/

#ifndef _NDO_UTILS_H
#define _NDO_UTILS_H


typedef struct ndo_dbuf_struct{
	char *buf;
	unsigned long used_size;
	unsigned long allocated_size;
	unsigned long chunk_size;
        }ndo_dbuf;

int ndo_dbuf_init(ndo_dbuf *,int);
int ndo_dbuf_free(ndo_dbuf *);
int ndo_dbuf_strcat(ndo_dbuf *,char *);

#endif
