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

#ifndef _NDO_UTILS_H
#define _NDO_UTILS_H

#include <stddef.h>

/* my_free has been freed from bondage as a function */
#ifdef BUILD_NAGIOS_2X
#define my_free(ptr) { if(ptr) { free(ptr); ptr = NULL; } }
#else
#define my_free(ptr) do { if(ptr) { free(ptr); ptr = NULL; } } while(0)
#endif


typedef struct ndo_dbuf {
	char *buf;
	size_t used_size; /**< Strlen of used buffer. */
	size_t alloc_size; /**< Allocated buffer byte size. */
} ndo_dbuf;


int ndo_dbuf_init(ndo_dbuf *, size_t);
int ndo_dbuf_free(ndo_dbuf *);
int ndo_dbuf_reset(ndo_dbuf *);
int ndo_dbuf_strcat(ndo_dbuf *, const char *);
int ndo_dbuf_strcat_escaped(ndo_dbuf *, const char *);
int ndo_dbuf_printf(ndo_dbuf *db, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

int my_rename(char *, char *);

void ndomod_strip(char *);

#endif
