/**
 * @file utils.c Miscellaneous common utility functions for NDO
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * This file is part of NDOUtils.
 *
 * NDOUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NDOUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NDOUtils. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/config.h"
#include "../include/common.h"
#include "../include/utils.h"
#include <assert.h>



/****************************************************************************/
/* DYNAMIC BUFFER FUNCTIONS                                                 */
/****************************************************************************/

/* Initializes a dynamic buffer. */
int ndo_dbuf_init(ndo_dbuf *db, size_t alloc_size) {

	if (!db) return NDO_ERROR;

	db->buf = NULL;
	db->used_size = 0;
	db->alloc_size = alloc_size;

	return NDO_OK;
}


/* Frees a dynamic buffer's internal memory and clears it fields. */
int ndo_dbuf_free(ndo_dbuf *db) {

	if (!db) return NDO_ERROR;

	my_free(db->buf);
	db->used_size = 0;
	db->alloc_size = 0;

	return NDO_OK;
}


/**
 * Resets a dynamic buffer without freeing its internal memory. Useful for
 * avoiding repeated memory allocation/deallocation in sequences of string
 * building operations.
 * @param db The dynamic buffer.
 * @return NDO_ERROR if db if NULL, NDO_OK otherwise.
 */
int ndo_dbuf_reset(ndo_dbuf *db) {

	if (!db) return NDO_ERROR;

	if (db->buf) db->buf[0] = '\0';
	db->used_size = 0;

	return NDO_OK;
}


/**
 * Ensures enough memory is available to append a string to a dynamic buffer,
 * (re)allocating the internal buffer as needed.
 * @param db The dynamic buffer.
 * @param requested_strlen Requested strlen to reserve.
 * @return NDO_ERROR when memory allocation fails, NDO_OK otherwise.
 */
static int ndo_dbuf_reserve_strlen(ndo_dbuf *db, size_t requested_strlen) {
	/* We need at least this much memory. */
	const size_t required_size = db->used_size + requested_strlen + 1;

	if (!db->buf || db->alloc_size < required_size) {
		char *newbuf = NULL;
		/* We need more memory, calculate how big to size the buffer. */
		size_t alloc_size = db->alloc_size ? db->alloc_size : 1;
		while (alloc_size < required_size) alloc_size *= 2;

		/* (Re)allocate memory to store old and new strings. */
		newbuf = realloc(db->buf, alloc_size);
		if (!newbuf) return NDO_ERROR;

		db->buf = newbuf;
		db->alloc_size = alloc_size;
	}

	return NDO_OK;
}


/* Concatentates a string to the buffer, dynamically expanding as needed. */
int ndo_dbuf_strcat(ndo_dbuf *db, const char *s) {
	size_t len = 0;

	if (!db || !s) return NDO_ERROR;

	/* Make sure we have enough room for the new string. */
	len = strlen(s);
	if (ndo_dbuf_reserve_strlen(db, len) != NDO_OK) return NDO_ERROR;

	/* Append the new string. */
	memcpy(db->buf + db->used_size, s, len+1);
	db->used_size += len;

	return NDO_OK;
}


/* Concatentates a string to the buffer, escaping the same characters as
 * ndo_escape_buffer(), dynamically expanding the buffer as needed. */
int ndo_dbuf_strcat_escaped(ndo_dbuf *db, const char *s) {
	size_t len;
	size_t d;

	if (!db || !s) return NDO_ERROR;

	/* Make sure we have enough room for the new string. */
	len = 2*strlen(s);
	if (ndo_dbuf_reserve_strlen(db, len) != NDO_OK) return NDO_ERROR;

	/* Append the new string, escaping as we go. */
	for (d = db->used_size; *s; ++s) {
		switch (*s) {
			case '\t': db->buf[d++] = '\\', db->buf[d++] = 't'; break;
			case '\r': db->buf[d++] = '\\', db->buf[d++] = 'r'; break;
			case '\n': db->buf[d++] = '\\', db->buf[d++] = 'n'; break;
			case '\\': db->buf[d++] = '\\', db->buf[d++] = '\\'; break;
			default: db->buf[d++] = *s; break;
		}
	}

	db->buf[d] = '\0'; /* Null terminate, but don't count the null byte. */
	db->used_size = d;

	return NDO_OK;
}


/* Prints to a dynamic buffer, sort of like asprintf(). */
int ndo_dbuf_printf(ndo_dbuf *db, const char *fmt, ...) {
	char * free_buf = NULL;
	size_t free_size = 0;
	int print_len = 0;
	va_list ap;

	if (!db || !fmt) return NDO_ERROR;

	/* If we don't have a buffer allocated yet, the free size is zero. */
	free_size = db->buf ? db->alloc_size - db->used_size : 0;
	/* A pointer to the start of data, or NULL if no buffer yet. */
	free_buf = db->buf + db->used_size;

	va_start(ap, fmt);
	/* Try to print to our buffer. If free_size is zero db->buf can be null and
	 * [v]snprintf() returns the string length that would have been printed. */
	print_len = vsnprintf(free_buf, free_size, fmt, ap);
	va_end(ap);

	if (print_len >= (int)free_size) {
		/* Overflow, make sure we have enough room for the string. */
		if (ndo_dbuf_reserve_strlen(db, print_len) != NDO_OK) return NDO_ERROR;

		free_size = db->alloc_size - db->used_size;
		assert((int)free_size > print_len);
		free_buf = db->buf + db->used_size;

		va_start(ap, fmt);
		/* Try to print to our buffer. If free_size is zero db->buf can be null and
		 * [v]snprintf() returns the string length that would have been printed. */
		print_len = vsnprintf(free_buf, free_size, fmt, ap);
		va_end(ap);
		assert((int)free_size > print_len);
	}

	/* Add the count of new characters. */
	if (print_len > 0) db->used_size += print_len;
	/* Keep things  null terminated. */
	if (db->buf) db->buf[db->used_size] = '\0';

	return print_len < 0 ? NDO_ERROR :  NDO_OK;
}



/******************************************************************/
/************************* FILE FUNCTIONS *************************/
/******************************************************************/

/* renames a file - works across filesystems (Mike Wiacek) */
int my_rename(char *source, char *dest){
	char buffer[1024]={0};
	int rename_result=0;
	int source_fd=-1;
	int dest_fd=-1;
	int bytes_read=0;


	/* make sure we have something */
	if(source==NULL || dest==NULL)
		return -1;

	/* first see if we can rename file with standard function */
	rename_result=rename(source,dest);

	/* handle any errors... */
	if(rename_result==-1){

		/* an error occurred because the source and dest files are on different filesystems */
		if(errno==EXDEV){

			/* open destination file for writing */
			if((dest_fd=open(dest,O_WRONLY|O_TRUNC|O_CREAT|O_APPEND,0644))>0){

				/* open source file for reading */
				if((source_fd=open(source,O_RDONLY,0644))>0){

					while((bytes_read=read(source_fd,buffer,sizeof(buffer)))>0)
						write(dest_fd,buffer,bytes_read);

					close(source_fd);
					close(dest_fd);
				
					/* delete the original file */
					unlink(source);

					/* reset result since we successfully copied file */
					rename_result=0;
					}

				else{
					close(dest_fd);
					return rename_result;
					}
				}
			}

		else{
			return rename_result;
			}
	        }

	return rename_result;
        }




/******************************************************************/
/************************ STRING FUNCTIONS ************************/
/******************************************************************/

/* strip newline, carriage return, and tab characters from beginning and end of a string */
void ndomod_strip(char *buffer){
	register int x=0;
	register int y=0;
	register int z=0;

	if(buffer==NULL || buffer[0]=='\x0')
		return;

	/* strip end of string */
	y=(int)strlen(buffer);
	for(x=y-1;x>=0;x--){
		if(buffer[x]==' ' || buffer[x]=='\n' || buffer[x]=='\r' || buffer[x]=='\t' || buffer[x]==13)
			buffer[x]='\x0';
		else
			break;
	        }
	/* save last position for later... */
	z=x;

	/* strip beginning of string (by shifting) */
	for(x=0;;x++){
		if(buffer[x]==' ' || buffer[x]=='\n' || buffer[x]=='\r' || buffer[x]=='\t' || buffer[x]==13)
			continue;
		else
			break;
	        }
	if(x>0){
		/* new length of the string after we stripped the end */
		y=z+1;
		
		/* shift chars towards beginning of string to remove leading whitespace */
		for(z=x;z<y;z++)
			buffer[z-x]=buffer[z];
		buffer[y-x]='\x0';
	        }

	return;
	}
