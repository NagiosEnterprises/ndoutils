/**
 * @file utils.h Miscellaneous common utility functionns for NDO
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

#ifndef NDO_UTILS_H_INCLUDED
#define NDO_UTILS_H_INCLUDED

/* my_free has been freed from bondage as a function */
#ifdef BUILD_NAGIOS_2X
#define my_free(ptr) { if(ptr) { free(ptr); ptr = NULL; } }
#else
#define my_free(ptr) do { if(ptr) { free(ptr); ptr = NULL; } } while(0)
#endif


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

void ndomod_strip(char *);

#endif
