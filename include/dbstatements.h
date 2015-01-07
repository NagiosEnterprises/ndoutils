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
 * Initializes our prepared statements once connected to the database and our
 * instance_id is available.
 * @param idi Input data and DB connection info.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 */
int ndo2db_stmt_init_stmts(ndo2db_idi *idi);

/**
 * Frees resources allocated for prepared statements.
 * @return Currently NDO_OK in all cases.
 */
int ndo2db_stmt_free_stmts(void);


/**
 * Fetches an existing object id from the cache or DB.
 * @param idi Input data and DB connection info.
 * @param object_type ndo2db object type code.
 * @param name1 Object name1.
 * @param name2 Object name2.
 * @param object_id Object id output.
 * @return NDO_OK with the object id in *object_id if an object was found,
 * otherwise an error code (usually NDO_ERROR) and *object_id = 0.
 */
int ndo2db_get_obj_id(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long *object_id);
/**
 * Fetches an existing object id from the cache or DB. Inserts a new row if an
 * existing id is not found for non-empty object names.
 * @param idi Input data and DB connection info.
 * @param object_type ndo2db object type code.
 * @param name1 Object name1.
 * @param name2 Object name2.
 * @param object_id Object id output.
 * @return NDO_OK with the object id in *object_id if an object was found or
 * inserted, otherwise an error code (usually NDO_ERROR) and *object_id = 0.
 */
int ndo2db_get_obj_id_with_insert(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long *object_id);
/**
 * Fetches all objects for an instance from the DB.
 * @param idi Input data and DB connection info.
 * @return NDO_OK on success, an error code otherwise, usually NDO_ERROR.
 * @post It is possible for the object cache to be partially populated if an
 * error occurs while processing results.
 */
int ndo2db_get_cached_obj_ids(ndo2db_idi *idi);
/**
 * Fetches an existing object id from the cache.
 * @param idi Input data and DB connection info.
 * @param object_type ndo2db object type code.
 * @param name1 Object name1.
 * @param name2 Object name2.
 * @param object_id Object id output.
 * @return NDO_OK with the object id in *object_id if an object was found,
 * otherwise an error code (usually NDO_ERROR) and *object_id = 0.
 */
int ndo2db_get_cached_obj_id(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long *object_id);
/**
 * Adds an entry to the object cache.
 */
int ndo2db_add_cached_obj_id(ndo2db_idi *idi, int object_type,
		const char *name1, const char *name2, unsigned long object_id);
/**
 * Frees resources allocated for the object cache.
 */
int ndo2db_free_cached_objs(ndo2db_idi *idi);
/**
 * Calculates an object's hash value.
 */
int ndo2db_obj_hashfunc(const char *name1, const char *name2, int hashslots);
/**
 * Compares two objects' hash data.
 * @return 0 if v1 == v2; +1 if v1 > v2; -1 if v1 < v2.
 */
int ndo2db_compare_obj_hashdata(const char *v1a, const char *v1b,
		const char *v2a, const char *v2b);
/**
 * Sets all objects as inactive in the DB for the current instance.
 */
int ndo2db_set_all_obj_inactive(ndo2db_idi *idi);
/**
 * Sets an object as active in the DB for the current instance.
 */
int ndo2db_set_obj_active(ndo2db_idi *idi, int object_type,
		unsigned long object_id);


/* Statement handler functions. These are intended to function as the
 * corresponding ndo2db_handle_* functions. */
int ndo2db_stmt_handle_servicecheckdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_hostcheckdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_hoststatusdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_servicestatusdata(ndo2db_idi *idi);

int ndo2db_stmt_handle_contactstatusdata(ndo2db_idi *);

int ndo2db_stmt_handle_adaptiveprogramdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_adaptivehostdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_adaptiveservicedata(ndo2db_idi *idi);
int ndo2db_stmt_handle_adaptivecontactdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_aggregatedstatusdata(ndo2db_idi *idi);
int ndo2db_stmt_handle_retentiondata(ndo2db_idi *idi);

int ndo2db_stmt_handle_configfilevariables(ndo2db_idi *idi, int config_type);
int ndo2db_stmt_handle_configvariables(ndo2db_idi *idi);
int ndo2db_stmt_handle_runtimevariables(ndo2db_idi *idi);

int ndo2db_stmt_handle_configdumpstart(ndo2db_idi *idi);
int ndo2db_stmt_handle_configdumpend(ndo2db_idi *idi);

int ndo2db_stmt_handle_hostdefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_hostgroupdefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_hostdependencydefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_hostescalationdefinition(ndo2db_idi *idi);

int ndo2db_stmt_handle_servicedefinition(ndo2db_idi *);
int ndo2db_stmt_handle_servicegroupdefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_servicedependencydefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_serviceescalationdefinition(ndo2db_idi *idi);

int ndo2db_stmt_handle_commanddefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_timeperiodefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_contactdefinition(ndo2db_idi *idi);
int ndo2db_stmt_handle_contactgroupdefinition(ndo2db_idi *idi);

int ndo2db_stmt_save_customvariables(ndo2db_idi *idi, unsigned long o_id);
int ndo2db_stmt_save_customvariable_status(ndo2db_idi *idi, unsigned long o_id,
		unsigned long t);

#endif
