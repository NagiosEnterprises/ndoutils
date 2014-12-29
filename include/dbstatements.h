/**
 * @file dbstatements.h Database prepared statement declarations for ndo2db
 * daemon
 */
/*
 * Copyright 2014 Nagios Core Development Team and Community Contributors
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

#ifndef NDOUTILS_INCLUDE_DBSTATEMENTS_H
#define NDOUTILS_INCLUDE_DBSTATEMENTS_H

/* Bring in the ndo2db_idi type declaration. */
#include "ndo2db.h"


/**
 * Initializes prepared statements once connected to the database and the
 * instance_id is available.
 */
int ndo2db_stmt_init_stmts(ndo2db_idi *idi);

/**
 * Frees resources allocated for prepared statements.
 */
int ndo2db_stmt_free_stmts(void);


/* Statement handler functions. These are intended to function as the
 * corresponding ndo2db_handle_* functions. */
int ndo2db_stmt_save_customvariables(ndo2db_idi *idi, unsigned long o_id);
int ndo2db_stmt_save_customvariablestatus(ndo2db_idi *idi, unsigned long o_id,
		unsigned long t);

#endif
