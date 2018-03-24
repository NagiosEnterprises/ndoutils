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


/****************************************************************************/
/* HASHING FUNCTIONS                                                        */
/****************************************************************************/

/* our basic 64 element lookup table */
static unsigned int ndo_hash_table[] = {
    0x69a03093, 0x5dfe9970, 0x36f71d91, 0x62bf5b2a,
    0x53fcac8a, 0x1ae50be7, 0x5ad28f83, 0x77695cb4,
    0x3b0e64d4, 0x1f27a85e, 0x6c6e0b0a, 0x2209a9f0,
    0x525cd481, 0x3a8c1903, 0x28bf8cf5, 0x62785dab,
    0x57cd4c33, 0x6de62d00, 0x274510dd, 0x10953293,
    0x14c888a3, 0x4ae79aaa, 0x1c32b588, 0x5573b029,
    0xa69448a0, 0x74cbb216, 0x5bb35f7b, 0x511f7814,
    0x7ef735ff, 0x41616000, 0x7446e56c, 0x4885f29f,
    0x3f1327d5, 0x4ebe59c7, 0x19a818f4, 0x522da2b5,
    0x6675fefe, 0x7425dd9f, 0x30e98d25, 0x37190a33,
    0x2c1c326d, 0x5879fda2, 0x2a23986b, 0x42edd7aa,
    0x47874e94, 0x23508db7, 0x12f2ce0a, 0x6aad41f1,
    0x4c2b4ea2, 0x6bce3fd1, 0x4072340f, 0x482f1dbc,
    0x2cddf1fe, 0x11b11751, 0x30db9351, 0x614ebac7,
    0x5b8916f1, 0x7dee79f7, 0x2e57dd95, 0x368bba57,
    0x5c42da69, 0x6507a03d, 0x4859e523, 0x7e04c0ca };

/* return a human readable/printable character with no special chars */
static inline char ndo_get_hash_char(unsigned int ch)
{
    register unsigned int tmp = ch % 62;

    /* 0-9 = 48-57, A-Z = 65-90, a-z = 97-122 */

    if (tmp < 10) {
        return tmp + 48;
    }

    if (tmp < 36) {
        /* 65 - 10 */
        return tmp + 55;
    }

    /* 97 - 26 -10 */
    return tmp + 61;
}

/* return a 64 character hash */
char * ndo_quick_hash(char * str)
{
    if (str == NULL || !strcmp(str, "")) {
        return strdup(NDO_EMPTY_HASH);
    }

    char * hash = calloc(65, sizeof(char));
    if (!hash) {
        return NULL;
    }

    /* i is used for str operations
       k is used for hash operations
       j is an intermediary for building */
    register unsigned int i = 0;
    register unsigned int j = 0;
    register unsigned int k = 0;;
    register char hash_complete = 0;
    register char string_complete = 0;

    /* initialize the hash */
    /* either fill the hash with the first 64 chars of str
       or repeat str until we've filled the 64 chars */
    for (k = 0; k < 64; k++) {

        hash[k] = str[i];

        /* reset str if we need to */
        if (str[i + 1] == '\0') {
            i = 0;
        } else {
            i++;
        }
    }

    i = j = k = 0;
    while (!string_complete || !hash_complete) {

        /* current j val xor with current char of hash * some lookup, bitshifted */
        j = j ^ (hash[k] * ndo_hash_table[(str[i] % 64)]) >> (i % 4);

        hash[k] = ndo_get_hash_char(j);

        /* if our current char is the end, reset the str */
        if (str[++i] == '\0') {
            i = 0;
            string_complete++;
        }

        /* if we've reached the end of the hash, reset */
        if (++k == 64) {
            hash_complete++;
            k = 0;
        }
    }

    return hash;
}




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
