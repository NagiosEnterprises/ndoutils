/***************************************************************
 * UTILS.C - NDO Utils
 *
 * Copyright (c) 2005 Ethan Galstad 
 * First Written: 05-19-2005
 *
 *
 **************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/utils.h"


/****************************************************************************/
/* DYNAMIC BUFFER FUNCTIONS                                                 */
/****************************************************************************/

/* initializes a dynamic buffer */
int ndo_dbuf_init(ndo_dbuf *db, int chunk_size){

	if(db==NULL)
		return NDO_ERROR;

	db->buf=NULL;
	db->used_size=0L;
	db->allocated_size=0L;
	db->chunk_size=chunk_size;

	return NDO_OK;
        }


/* frees a dynamic buffer */
int ndo_dbuf_free(ndo_dbuf *db){

	if(db==NULL)
		return NDO_ERROR;

	if(db->buf!=NULL)
		free(db->buf);
	db->buf=NULL;
	db->used_size=0L;
	db->allocated_size=0L;

	return NDO_OK;
        }


/* dynamically expands a string */
int ndo_dbuf_strcat(ndo_dbuf *db, char *buf){
	char *newbuf=NULL;
	unsigned long buflen=0L;
	unsigned long new_size=0L;
	unsigned long memory_needed=0L;

	if(db==NULL || buf==NULL)
		return NDO_ERROR;

	/* how much memory should we allocate (if any)? */
	buflen=strlen(buf);
	new_size=db->used_size+buflen+1;

	/* we need more memory */
	if(db->allocated_size<new_size){

		memory_needed=((ceil(new_size/db->chunk_size)+1)*db->chunk_size);

		/* allocate memory to store old and new string */
		if((newbuf=(char *)realloc((void *)db->buf,(size_t)memory_needed))==NULL)
			return NDO_ERROR;

		/* update buffer pointer */
		db->buf=newbuf;

		/* update allocated size */
		db->allocated_size=memory_needed;

		/* terminate buffer */
		db->buf[db->used_size]='\x0';
	        }

	/* append the new string */
	strcat(db->buf,buf);

	/* update size allocated */
	db->used_size+=buflen;

	return NDO_OK;
        }
