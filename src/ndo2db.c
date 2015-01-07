/**
 * @file ndo2db.c Nagios Data Output to Database Daemon
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * Last Modified: 01-07-2015
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

/*#define DEBUG_MEMORY 1*/
#ifdef DEBUG_MEMORY
#include <mcheck.h>
#endif

/* Headers from our project. */
#include "../include/config.h"
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndo2db.h"
#include "../include/db.h"
#include "../include/dbhandlers.h"
#include "../include/dbstatements.h"
#include "../include/queue.h"

#ifdef HAVE_SSL
#include "../include/dh.h"
#endif

#define NDO2DB_VERSION "2.1b1"
#define NDO2DB_NAME "NDO2DB"
#define NDO2DB_DATE "01-07-2015"

#ifdef HAVE_SSL
SSL_METHOD *meth;
SSL_CTX *ctx;
int allow_weak_random_seed = NDO_FALSE;
#endif
extern int use_ssl;


char *ndo2db_config_file = NULL;
char *lock_file = NULL;
char *ndo2db_user = NULL;
char *ndo2db_group = NULL;
int ndo2db_sd = 0;
int ndo2db_socket_type = NDO_SINK_UNIXSOCKET;
char *ndo2db_socket_name = NULL;
int ndo2db_tcp_port = NDO_DEFAULT_TCP_PORT;
int ndo2db_use_inetd = NDO_FALSE;
int ndo2db_show_version = NDO_FALSE;
int ndo2db_show_license = NDO_FALSE;
int ndo2db_show_help = NDO_FALSE;

ndo2db_dbconfig ndo2db_db_settings;
time_t ndo2db_db_last_checkin_time = 0;

char *ndo2db_debug_file = NULL;
int ndo2db_debug_level = NDO2DB_DEBUGL_NONE;
int ndo2db_debug_verbosity = NDO2DB_DEBUGV_BASIC;
FILE *ndo2db_debug_file_fp = NULL;
unsigned long ndo2db_max_debug_file_size = 0;



/*#define DEBUG_NDO2DB 1*/ /* don't daemonize */
/*#define DEBUG_NDO2DB_EXIT_AFTER_CONNECTION 1*/ /* exit after first client disconnects */
/*#define DEBUG_NDO2DB2 1*/
/*#define NDO2DB_DEBUG_MBUF 1*/
/*
#ifdef NDO2DB_DEBUG_MBUF
unsigned long mbuf_bytes_allocated = 0;
unsigned long mbuf_data_allocated = 0;
#endif
*/


int main(int argc, char **argv) {
	int db_supported = NDO_FALSE;
	int result = NDO_OK;

#ifdef DEBUG_MEMORY
	mtrace();
#endif

#ifdef HAVE_SSL
	DH *dh;
	char seedfile[FILENAME_MAX];
	int i;
	int c;
#endif

	result = ndo2db_process_arguments(argc, argv);

	if (result != NDO_OK || ndo2db_show_help || ndo2db_show_license
			|| ndo2db_show_version) {

		if (result != NDO_OK) {
			printf("Incorrect command line arguments supplied\n");
		}

		printf("\n");
		printf("%s %s\n", NDO2DB_NAME, NDO2DB_VERSION);
		printf("Copyright 2009-2014 Nagios Core Development Team and Community Contributors\n");
		printf("Copyright 2005-2008 Ethan Galstad\n");
		printf("Last Modified: %s\n", NDO2DB_DATE);
		printf("License: GPL v2\n");
#ifdef HAVE_SSL
		printf("SSL/TLS Available: Anonymous DH Mode, OpenSSL 0.9.6 or higher required\n");
#endif
		printf("\n");
		printf("Stores Nagios event and configuration data to a database for later retrieval\n");
		printf("and processing. Clients that are capable of sending data to the NDO2DB daemon\n");
		printf("include the LOG2NDO utility and NDOMOD event broker module.\n");
		printf("\n");
		printf("Usage: %s -c <config_file> [-i]\n", argv[0]);
		printf("\n");
		printf("-i  = Run under INETD/XINETD.\n");
		printf("\n");
		exit(EXIT_FAILURE);
	}

	/* initialize variables */
	ndo2db_initialize_variables();

	/* process config file */
	if (ndo2db_process_config_file(ndo2db_config_file) != NDO_OK) {
		printf("Error processing config file '%s'.\n", ndo2db_config_file);
		exit(EXIT_FAILURE);
	}

#ifdef HAVE_SSL
	/* initialize SSL */
	if (use_ssl) {
		SSL_library_init();
		SSLeay_add_ssl_algorithms();
		meth = SSLv23_server_method();
		SSL_load_error_strings();

		/* use week random seed if necessary */
		if (allow_weak_random_seed && (RAND_status() == 0)) {

			if (RAND_file_name(seedfile, sizeof(seedfile)-1)) {
				if (RAND_load_file(seedfile, -1)) RAND_write_file(seedfile);
			}

			if (RAND_status() == 0) {
				syslog(LOG_ERR, "Warning: SSL/TLS uses a weak random seed which is highly discouraged");
				srand(time(NULL));
				for (i = 0; i < 500 && RAND_status() == 0; i++) {
					for (c = 0; c < sizeof(seedfile); c += sizeof(int)) {
						*((int *)(seedfile+c)) = rand();
					}
					RAND_seed(seedfile, sizeof(seedfile));
				}
			}
		}
		if (!(ctx = SSL_CTX_new(meth))) {
			syslog(LOG_ERR, "Error: could not create SSL context.\n");
			exit(EXIT_FAILURE);
		}

		/* ADDED 01/19/2004: use only TLSv1 protocol */
		SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

		/* use anonymous DH ciphers */
		SSL_CTX_set_cipher_list(ctx, "ADH");
		dh = get_dh512();
		SSL_CTX_set_tmp_dh(ctx, dh);
		DH_free(dh);
		syslog(LOG_INFO, "INFO: SSL/TLS initialized. All network traffic will be encrypted.");
	}
	else {
		syslog(LOG_INFO, "INFO: SSL/TLS NOT initialized. Network encryption DISABLED.");
	}
	/*Fin Hack SSL*/
#endif

	/* make sure we're good to go */
	if (ndo2db_check_init_reqs() != NDO_OK) {
		printf("One or more required parameters is missing or incorrect.\n");
		goto main_error_exit;
	}

	/* make sure we support the db option chosen... */
#ifdef USE_MYSQL
	if (ndo2db_db_settings.server_type == NDO2DB_DBSERVER_MYSQL) {
		db_supported = NDO_TRUE;
	}
#endif

	if (!db_supported) {
		printf("Support for the specified database server is either not yet supported, or was not found on your system.\n");
		goto main_error_exit;
	}

	/* initialize signal handling */
	signal(SIGQUIT, ndo2db_parent_sighandler);
	signal(SIGTERM, ndo2db_parent_sighandler);
	signal(SIGINT, ndo2db_parent_sighandler);
	signal(SIGSEGV, ndo2db_parent_sighandler);
	signal(SIGFPE, ndo2db_parent_sighandler);
	signal(SIGCHLD, ndo2db_parent_sighandler);

	/* drop privileges */
	ndo2db_drop_privileges(ndo2db_user, ndo2db_group);

	/* open debug log */
	ndo2db_open_debug_log();

	/* if we're running under inetd... */
	if (ndo2db_use_inetd) {

		/* redirect STDERR to /dev/null */
		close(2);
		open("/dev/null", O_WRONLY);

		/* handle the connection */
		ndo2db_handle_client_connection(0);
	}

	/* standalone daemon... */
	else {
		/* create socket and wait for clients to connect */
		if (ndo2db_wait_for_connections() == NDO_ERROR) goto main_error_exit;
	}

	/* close debug log */
	ndo2db_close_debug_log();
	/* free memory */
	ndo2db_free_program_memory();

#ifdef HAVE_SSL
	if (use_ssl) SSL_CTX_free(ctx);
#endif
	exit(EXIT_SUCCESS); /* return 0 */

main_error_exit:
#ifdef HAVE_SSL
	if (use_ssl) SSL_CTX_free(ctx);
#endif
	exit(EXIT_FAILURE);
}


/* process command line arguments */
int ndo2db_process_arguments(int argc, char **argv) {
#ifdef HAVE_GETOPT_H
	int option_index = 0;
	static struct option long_options[] = {
		{"configfile", required_argument, 0, 'c'},
		{"inetd", no_argument, 0, 'i'},
		{"help", no_argument, 0, 'h'},
		{"license", no_argument, 0, 'l'},
		{"version", no_argument, 0, 'V'},
		{0, 0, 0, 0}
	};
#endif

	/* no options were supplied */
	if (argc < 2) {
		ndo2db_show_help = NDO_TRUE;
		return NDO_OK;
	}

	while (1) {
#ifdef HAVE_GETOPT_H
		int c = getopt_long(argc, argv, "c:ihlV", long_options, &option_index);
#else
		int c = getopt(argc, argv, "c:ihlV");
#endif
		if (c == -1 || c == EOF) break;

		/* process all arguments */
		switch (c) {
		case '?':
		case 'h':
			ndo2db_show_help = NDO_TRUE;
			break;
		case 'V':
			ndo2db_show_version = NDO_TRUE;
			break;
		case 'l':
			ndo2db_show_license = NDO_TRUE;
			break;
		case 'c':
			ndo2db_config_file = strdup(optarg);
			break;
		case 'i':
			ndo2db_use_inetd = NDO_TRUE;
			break;
		default:
			return NDO_ERROR;
			break;
		}
	}

	/* make sure required args were supplied */
	return (ndo2db_config_file || ndo2db_show_help || ndo2db_show_version
			|| ndo2db_show_license)
		? NDO_OK : NDO_ERROR;
}



/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* Process all config vars in a file. */
int ndo2db_process_config_file(const char *filename) {
	ndo_mmapfile *thefile = NULL;
	char *buf = NULL;
	int result = NDO_OK;

	/* Open our file. */
	if (!(thefile = ndo_mmap_fopen(filename))) return NDO_ERROR;

	/* Read, process, and free each line. */
	for (; result == NDO_OK && (buf = ndo_mmap_fgets(thefile)); free(buf)) {
		/* Skip comments and blank lines... */
		if (buf[0] != '\0' && buf[0] != '#') {
			/* ...otherwise process the variable. */
			result = ndo2db_process_config_var(buf);
		}
	}

	/* Close the file. */
	ndo_mmap_fclose(thefile);

	return result;
}


/* A macro to check and copy string options. Returns from the invoking context
 * if the config var matches n. Memory allocated for the value will need to
 * later be freed. */
#define NDO_HANDLE_STRING_OPT(n, v) \
	do if (strcmp(var, n) == 0) { \
		if ((v = strdup(val))) { \
			return NDO_OK; \
		} \
		else { \
			printf("Error copying option string for '%s'.", n); \
			return NDO_ERROR; \
		} \
	} while (0)

/* A macro to handle strtoul() with a return from invoking context on error. */
#define NDO_HANDLE_STRTOUL_OPT_VAL(n, v) \
	do { \
		char *end = NULL; \
		errno = 0; \
		v = strtoul(val, &end, 0); \
		if (errno || end == val) { \
			printf("ndomod: Error converting value for '%s'.", n); \
			return NDO_ERROR; \
		} \
	} while (0)

/* A macro to check and handle strtoul() options with a return from invoking
 * context on match. */
#define NDO_HANDLE_STRTOUL_OPT(n, v) \
	do if (strcmp(var, n) == 0) { \
		NDO_HANDLE_STRTOUL_OPT_VAL(n, v); \
		return NDO_OK; \
	} while (0)

/* A macro to check and handle strtoul() minutes options with a return from
 * invoking context on match. This is similar to NDO_HANDLE_STRTOUL_OPT(), but
 * multiplies the option value by 60 to convert minutes to seconds. */
#define NDO_HANDLE_MINUTES_OPT(n, v) \
	do if (strcmp(var, n) == 0) { \
		NDO_HANDLE_STRTOUL_OPT_VAL(n, v); \
		v *= 60; \
		return NDO_OK; \
	} while (0)

/* Process a single config variable. */
int ndo2db_process_config_var(char *arg) {
	char *var = NULL;
	char *val = NULL;

	/* Split and strip var/val */
	ndomod_strip(var = strtok(arg, "="));
	ndomod_strip(val = strtok(NULL, "\n"));

	/* Skip incomplete var or val. */
	if (!var || !val) return NDO_OK;

	/* Process the variable... */

	NDO_HANDLE_STRING_OPT("lock_file", lock_file);

	if (strcmp(var, "socket_type") == 0) {
		if (strcmp(val, "tcp") == 0) {
			ndo2db_socket_type = NDO_SINK_TCPSOCKET;
		}
		else {
			ndo2db_socket_type = NDO_SINK_UNIXSOCKET;
		}
		return NDO_OK;
	}

	NDO_HANDLE_STRING_OPT("socket_name", ndo2db_socket_name);

	NDO_HANDLE_STRTOUL_OPT("tcp_port", ndo2db_tcp_port);

	if (strcmp(var, "db_servertype") == 0) {
		if (strcmp(val, "mysql") == 0) {
			ndo2db_db_settings.server_type = NDO2DB_DBSERVER_MYSQL;
		}
		else {
			return NDO_ERROR;
		}
		return NDO_OK;
	}

	NDO_HANDLE_STRING_OPT("db_host", ndo2db_db_settings.host);
	NDO_HANDLE_STRTOUL_OPT("db_port", ndo2db_db_settings.port);
	NDO_HANDLE_STRING_OPT("db_socket", ndo2db_db_settings.socket);
	NDO_HANDLE_STRING_OPT("db_user", ndo2db_db_settings.username);
	NDO_HANDLE_STRING_OPT("db_pass", ndo2db_db_settings.password);
	NDO_HANDLE_STRING_OPT("db_name", ndo2db_db_settings.dbname);
	NDO_HANDLE_STRING_OPT("db_prefix", ndo2db_db_settings.dbprefix);

	NDO_HANDLE_MINUTES_OPT("max_timedevents_age", ndo2db_db_settings.max_timedevents_age);
	NDO_HANDLE_MINUTES_OPT("max_systemcommands_age", ndo2db_db_settings.max_systemcommands_age);
	NDO_HANDLE_MINUTES_OPT("max_servicechecks_age", ndo2db_db_settings.max_servicechecks_age);
	NDO_HANDLE_MINUTES_OPT("max_hostchecks_age", ndo2db_db_settings.max_hostchecks_age);
	NDO_HANDLE_MINUTES_OPT("max_eventhandlers_age", ndo2db_db_settings.max_eventhandlers_age);
	NDO_HANDLE_MINUTES_OPT("max_externalcommands_age", ndo2db_db_settings.max_externalcommands_age);
	NDO_HANDLE_MINUTES_OPT("max_logentries_age", ndo2db_db_settings.max_logentries_age);
	NDO_HANDLE_MINUTES_OPT("max_notifications_age", ndo2db_db_settings.max_notifications_age);
	/* Accept max_contactnotifications too (a typo in old configs). */
	NDO_HANDLE_MINUTES_OPT("max_contactnotifications", ndo2db_db_settings.max_contactnotifications_age);
	NDO_HANDLE_MINUTES_OPT("max_contactnotifications_age", ndo2db_db_settings.max_contactnotifications_age);
	/* Accept max_contactnotificationmethods too (a typo in old configs). */
	NDO_HANDLE_MINUTES_OPT("max_contactnotificationmethods", ndo2db_db_settings.max_contactnotificationmethods_age);
	NDO_HANDLE_MINUTES_OPT("max_contactnotificationmethods_age", ndo2db_db_settings.max_contactnotificationmethods_age);
	NDO_HANDLE_MINUTES_OPT("max_acknowledgements_age", ndo2db_db_settings.max_acknowledgements_age);

	NDO_HANDLE_MINUTES_OPT("table_trim_interval", ndo2db_db_settings.table_trim_interval);

	NDO_HANDLE_STRING_OPT("ndo2db_user", ndo2db_user);
	NDO_HANDLE_STRING_OPT("ndo2db_group", ndo2db_group);

	NDO_HANDLE_STRING_OPT("debug_file", ndo2db_debug_file);
	NDO_HANDLE_STRTOUL_OPT("debug_level", ndo2db_debug_level);
	NDO_HANDLE_STRTOUL_OPT("debug_verbosity", ndo2db_debug_verbosity);
	NDO_HANDLE_STRTOUL_OPT("max_debug_file_size", ndo2db_max_debug_file_size);

	if (strcmp(var, "use_ssl") == 0) {
		if (strlen(val) == 1) use_ssl = (val[0] == '1');
		return NDO_OK;
	}

	return NDO_OK;
}


/* Initializes DB settings to default (null) values. */
int ndo2db_initialize_variables(void) {

	ndo2db_db_settings.server_type = NDO2DB_DBSERVER_NONE;
	ndo2db_db_settings.host = NULL;
	ndo2db_db_settings.port = 0;
	ndo2db_db_settings.socket = NULL;
	ndo2db_db_settings.username = NULL;
	ndo2db_db_settings.password = NULL;
	ndo2db_db_settings.dbname = NULL;
	ndo2db_db_settings.dbprefix = NULL;
	ndo2db_db_settings.max_timedevents_age = 0;
	ndo2db_db_settings.max_systemcommands_age = 0;
	ndo2db_db_settings.max_servicechecks_age = 0;
	ndo2db_db_settings.max_hostchecks_age = 0;
	ndo2db_db_settings.max_eventhandlers_age = 0;
	ndo2db_db_settings.max_externalcommands_age = 0;
	ndo2db_db_settings.max_notifications_age = 0;
	ndo2db_db_settings.max_contactnotifications_age = 0;
	ndo2db_db_settings.max_contactnotificationmethods_age = 0;
	ndo2db_db_settings.max_logentries_age = 0;
	ndo2db_db_settings.max_acknowledgements_age = 0;
	ndo2db_db_settings.table_trim_interval = NDO2DB_DEFAULT_TABLE_TRIM_INTERVAL;

	return NDO_OK;
}



/****************************************************************************/
/* CLEANUP FUNCTIONS                                                       */
/****************************************************************************/

/* Frees program memory allocated for configuration variables. */
int ndo2db_free_program_memory(void) {

	my_free(ndo2db_config_file);
	my_free(ndo2db_user);
	my_free(ndo2db_group);
	my_free(ndo2db_socket_name);
	my_free(ndo2db_db_settings.host);
	my_free(ndo2db_db_settings.socket);
	my_free(ndo2db_db_settings.username);
	my_free(ndo2db_db_settings.password);
	my_free(ndo2db_db_settings.dbname);
	my_free(ndo2db_db_settings.dbprefix);
	my_free(ndo2db_debug_file);

	return NDO_OK;
}



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

int ndo2db_check_init_reqs(void) {

	if (ndo2db_socket_type == NDO_SINK_UNIXSOCKET) {
		if (!ndo2db_socket_name) {
			printf("No socket name specified.\n");
			return NDO_ERROR;
		}
	}

	return NDO_OK;
}



/* Drops privileges to ndo2db_user:ndo2db_group. */
int ndo2db_drop_privileges(char *user, char *group) {
	uid_t uid = -1;
	gid_t gid = -1;

	/* Set our effective group ID. */
	if (group) {

		/* See if we have a group name... */
		if (strspn(group, "0123456789") < strlen(group)) {
			struct group *grp = getgrnam(group);
			if (grp) {
				gid = grp->gr_gid;
			}
			else {
				syslog(LOG_ERR, "Warning: Could not get group entry for '%s'", group);
			}
			endgrent();
		}
		/* ...else we have a numeric GID. */
		else {
			gid = atoi(group);
		}

		/* Set our group ID if different than current EGID. */
		if (gid != getegid() && setgid(gid) == -1) {
				syslog(LOG_ERR, "Warning: Could not set effective GID=%d", (int)gid);
		}
	}

	/* Similarly, set our effective user ID. */
	if (user) {

		/* See if we have a user name... */
		if (strspn(user, "0123456789") < strlen(user)) {
			struct passwd *pw = getpwnam(user);
			if (pw) {
				uid = pw->pw_uid;
			}
			else {
				syslog(LOG_ERR, "Warning: Could not get passwd entry for '%s'", user);
			}
			endpwent();
		}
		/* ...otherwise we have a numeric UID. */
		else {
			uid = atoi(user);
		}

		/* Set our user ID if different than current EUID. */
		if (uid != geteuid()) {
#ifdef HAVE_INITGROUPS
			/* Initialize supplementary groups. */
			if (initgroups(user, gid) == -1) {
				if (errno == EPERM) {
					syslog(LOG_ERR, "Warning: Unable to change supplementary groups using initgroups()");
				}
				else {
					syslog(LOG_ERR, "Warning: Possibly root user failed dropping privileges with initgroups()");
					return NDO_ERROR;
				}
			}
#endif
			if (setuid(uid) == -1)
				syslog(LOG_ERR, "Warning: Could not set effective UID=%d", (int)uid);
		}
	}

	return NDO_OK;
}


int ndo2db_daemonize(void) {
	pid_t pid = -1;
	int lockfile = 0;
	struct flock lock;
	int val = 0;
	char buf[256];

	umask(S_IWGRP|S_IWOTH);

	/* Get a lock on the lockfile */
	if (lock_file) {
		lockfile = open(lock_file, O_RDWR|O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
		if (lockfile < 0) {
			fprintf(stderr, "Failed to obtain lock on file %s: %s\n", lock_file, strerror(errno));
			ndo2db_cleanup_socket();
			return NDO_ERROR;
		}

		/* See if we can read the contents of the lockfile */
		if ((val = read(lockfile, buf, 10)) < 0) {
			perror("Lockfile exists but cannot be read");
			ndo2db_cleanup_socket();
			return NDO_ERROR;
		}

		/* Place a file lock on the lock file. */
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		if (fcntl(lockfile, F_SETLK, &lock) < 0) {
			if (errno == EACCES || errno == EAGAIN) {
				fcntl(lockfile, F_GETLK, &lock);
				fprintf(stderr, "Lockfile '%s' looks like its already held by another instance (%d): %s\nBailing out...\n",
						lock_file, (int)lock.l_pid, strerror(errno));
			}
			else {
				fprintf(stderr, "Cannot lock lockfile '%s': %s\nBailing out...\n",
						lock_file, strerror(errno));
			}
			ndo2db_cleanup_socket();
			return NDO_ERROR;
		}
	}

	/* Fork. */
	pid = fork();
	if (pid < 0) {
		perror("Fork error");
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	}
	/* Original parent process goes away... */
	if (pid > 0) {
		ndo2db_free_program_memory();
		exit(0);
	}

	/* New child forks again... */
	pid = fork();
	if (pid < 0) {
		perror("Fork error");
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	}
	/* First child process goes away.. */
	if (pid > 0) {
		ndo2db_free_program_memory();
		exit(0);
	}

	/* Grandchild continues and becomes session leader... */
	setsid();


	if (lock_file) {
		int n;
		/* Write our PID to the lockfile... */
		lseek(lockfile, 0, SEEK_SET);
		if (ftruncate(lockfile, 0)) {
			syslog(LOG_ERR, "Warning: Unable to truncate lockfile (errno %d): %s",
					errno, strerror(errno));
		}
		sprintf(buf, "%d\n", (int)getpid());
		n = (int)strlen(buf);
		if (write(lockfile, buf, n) < n) {
			syslog(LOG_ERR, "Warning: Unable to write pid to lockfile (errno %d): %s",
					errno, strerror(errno));
		}

		/* Keep the lock file open while program is executing... */
		val = fcntl(lockfile, F_GETFD, 0);
		val |= FD_CLOEXEC;
		fcntl(lockfile, F_SETFD, val);
	}

	/* Close existing stdin, stdout, stderr. */
	close(0);
#ifndef DEBUG_NDO2DB
	close(1);
#endif
	close(2);

	/* Re-open stdin, stdout, stderr with known values. */
	open("/dev/null", O_RDONLY);
#ifndef DEBUG_NDO2DB
	open("/dev/null", O_WRONLY);
#endif
	open("/dev/null", O_WRONLY);

	return NDO_OK;
}


int ndo2db_cleanup_socket(void) {

	/* No socket to cleanup if we're running under inetd. */
	if (ndo2db_use_inetd) return NDO_OK;

	/* Close the socket. */
	shutdown(ndo2db_sd, 2);
	close(ndo2db_sd);

	/* Unlink UNIX domain sockets. */
	if (ndo2db_socket_type == NDO_SINK_UNIXSOCKET) unlink(ndo2db_socket_name);
	/* Unlink our lock file so it will go away. */
	if (lock_file) unlink(lock_file);

	return NDO_OK;
}


void ndo2db_parent_sighandler(int sig) {

	switch (sig) {
	case SIGTERM:
	case SIGINT:
		/* forward signal to all members of this group of processes. */
		kill(0, sig);
		break;
	case SIGCHLD:
		/* Cleanup children that exit, so we don't have zombies. */
		while (waitpid(-1, NULL, WNOHANG) > 0);
		return;
	default:
		/* Nothing special to do here, continue on to shutdown code below, although
		 * this old printf() (that shouldn't be called from a signal-handler
		 * anyway...) seems to indicate that we are ignoring other signals. */
		/* printf("Caught the Signal '%d' but don't care about this.\n", sig); */
		break;
	}

	/* Cleanup the socket (the functions used are async-signal-safe). */
	ndo2db_cleanup_socket();

	/* free memory */
	/* @todo: This needs to be moved elswhere, free() is not async-signal-safe. */
	ndo2db_free_program_memory();

	/* @todo: Should be _exit(0); or set a flag to exit(0) from the mainline. */
	exit(0);

	return;
}


void ndo2db_child_sighandler(int sig) {
	(void)sig; /* Unused, don't warn. */
	_exit(0);
}


/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/


int ndo2db_wait_for_connections(void) {
	int sd_flag = 1;
	int new_sd = 0;
	pid_t new_pid = -1;
	struct sockaddr_un server_address_u;
	struct sockaddr_in server_address_i;
	struct sockaddr_un client_address_u;
	struct sockaddr_in client_address_i;
	socklen_t client_address_length;


	/* TCP socket */
	if (ndo2db_socket_type == NDO_SINK_TCPSOCKET) {

		/* create a socket */
		if (!(ndo2db_sd = socket(PF_INET, SOCK_STREAM, 0))) {
			perror("Cannot create socket");
			return NDO_ERROR;
		}

		/* set the reuse address flag so we don't get errors when restarting */
		sd_flag = 1;
		if (setsockopt(ndo2db_sd, SOL_SOCKET, SO_REUSEADDR, &sd_flag, sizeof(sd_flag)) < 0) {
			close(ndo2db_sd);
			fprintf(stderr, "Could not set reuse address option on socket!\n");
			return NDO_ERROR;
		}

		/* clear the address */
		memset(&server_address_i, 0, sizeof(server_address_i));
		server_address_i.sin_family = AF_INET;
		server_address_i.sin_addr.s_addr = INADDR_ANY;
		server_address_i.sin_port = htons(ndo2db_tcp_port);

		/* bind the socket */
		if ((bind(ndo2db_sd, (struct sockaddr *)&server_address_i, sizeof(server_address_i)))) {
			close(ndo2db_sd);
			perror("Could not bind socket");
			return NDO_ERROR;
		}

		client_address_length = (socklen_t)sizeof(client_address_i);
	}

	/* UNIX domain socket */
	else {

		/* create a socket */
		if (!(ndo2db_sd = socket(AF_UNIX, SOCK_STREAM, 0))) {
			perror("Cannot create socket");
			return NDO_ERROR;
		}

		/* copy the socket path */
		strncpy(server_address_u.sun_path, ndo2db_socket_name, sizeof(server_address_u.sun_path));
		server_address_u.sun_family = AF_UNIX;

		/* bind the socket */
		if ((bind(ndo2db_sd, (struct sockaddr *)&server_address_u, SUN_LEN(&server_address_u)))) {
			close(ndo2db_sd);
			perror("Could not bind socket");
			return NDO_ERROR;
		}

		client_address_length = (socklen_t)sizeof(client_address_u);
	}

	/* listen for connections */
	if (listen(ndo2db_sd, 1)) {
		perror("Cannot listen on socket");
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	}


	/* daemonize */
#ifndef DEBUG_NDO2DB
	if (ndo2db_daemonize() != NDO_OK) {
		ndo2db_cleanup_socket();
		return NDO_ERROR;
	}
#endif

	/* accept connections... */
	while (1) {
		/* Solaris 10 gets an EINTR error when file2sock invoked on the 2nd call.
		 * An alternative could be to not fork below, but this has wider implications. */
		while (1) {
			new_sd = accept(
					ndo2db_sd,
					(ndo2db_socket_type == NDO_SINK_TCPSOCKET
							? (struct sockaddr *)&client_address_i
							: (struct sockaddr *)&client_address_u),
					&client_address_length
			);

			/* ToDo: Hendrik 08/12/2009
			 * If both ends think differently about SSL encryption, data from a ndomod
			 * will be lost (likewise on database errors/misconfiguration). This seems
			 * a good place to output some information from which client a possible
			 * misconfiguration comes from. Logging the ip address together with the
			 * ndomod instance name might be a great hint for further error hunting. */
			if (new_sd >= 0) break;/* data available */
			if (errno != EINTR) {
				perror("Accept error");
				ndo2db_cleanup_socket();
				return NDO_ERROR;
			}
		}


#ifndef DEBUG_NDO2DB
		/* Fork... */
		new_pid = fork();
		switch (new_pid) {
			case -1:
				/* Parent prints a message and keeps on going on error... */
				perror("Fork error");
				close(new_sd);
				break;

			case 0:
#endif
				/* Child processes data... */
				ndo2db_handle_client_connection(new_sd);
				/* Close our socket when done. */
				close(new_sd);
#ifndef DEBUG_NDO2DB
				return NDO_OK;
				break;

			default:
				/* Parent keeps on going... */
				close(new_sd);
				break;
		}
#endif

#ifdef DEBUG_NDO2DB_EXIT_AFTER_CONNECTION
		break;
#endif
	}

	/* cleanup after ourselves */
	ndo2db_cleanup_socket();

	return NDO_OK;
}


int ndo2db_handle_client_connection(int sd) {
	pid_t chpid;
	struct ndo2db_queue_msg msg;
	ndo2db_idi idi;
	int result = 0;
	int error = NDO_FALSE;

#ifdef HAVE_SSL
	SSL *ssl = NULL;
#endif

	/* open syslog facility */
	/*openlog("ndo2db", 0, LOG_DAEMON);*/

	/* re-open debug log */
	ndo2db_close_debug_log();
	ndo2db_open_debug_log();

	/* reset signal handling */
	signal(SIGQUIT, ndo2db_child_sighandler);
	signal(SIGTERM, ndo2db_child_sighandler);
	signal(SIGINT, ndo2db_child_sighandler);
	signal(SIGSEGV, ndo2db_child_sighandler);
	signal(SIGFPE, ndo2db_child_sighandler);

	/* Initialize our message queue for sending data to the DB writer. */
	ndo2db_queue_init(getpid());

	/* Start our DB writer child process. */
	chpid = fork();
	if (chpid == 0) ndo2db_async_client_handle();

	/* initialize input data information */
	ndo2db_idi_init(&idi);

	/* initialize database connection */
	ndo2db_db_init(&idi);
	ndo2db_db_connect(&idi);

#ifdef HAVE_SSL
	if (use_ssl && (ssl = SSL_new(ctx))) {

		SSL_set_fd(ssl, sd);

		/* keep attempting the request if needed */
		while (((result = SSL_accept(ssl)) != 1)
				&& (SSL_get_error(ssl, result) == SSL_ERROR_WANT_READ));

		if (result != 1) {
			syslog(LOG_ERR, "Error: Could not complete SSL handshake: %d", SSL_get_error(ssl, result));
			return NDO_ERROR;
		}
	}
#endif

	/* read all data from client */
	while (1) {
#ifdef HAVE_SSL
		if (!use_ssl) {
			result = read(sd, msg.text, sizeof(msg.text)-1);
		}
		else {
			result = SSL_read(ssl, msg.text, sizeof(msg.text)-1);
			if (result == -1 && (SSL_get_error(ssl, result) == SSL_ERROR_WANT_READ)) {
				syslog(LOG_ERR, "SSL read error.");
			}
		}
#else
		result = read(sd, msg.text, sizeof(msg.text)-1);
#endif

		/* bail out on hard errors */
		if (result == -1) {
			/* EAGAIN and EINTR are soft errors, so try another read(). */
			if (errno == EAGAIN || errno == EINTR) continue;
#ifdef HAVE_SSL
			if (ssl) {
				SSL_shutdown(ssl);
				SSL_free(ssl);
				syslog(LOG_INFO, "INFO: SSL Socket Shutdown.");
			}
#endif
			error = NDO_TRUE;
			break;
		}

		/* zero bytes read means we lost the connection with the client */
		if (result == 0) {
#ifdef HAVE_SSL
			if (ssl) {
				SSL_shutdown(ssl);
				SSL_free(ssl);
				syslog(LOG_INFO, "INFO: SSL Socket Shutdown.");
			}
#endif
			/* gracefully back out of current operation... */
			ndo2db_db_goodbye(&idi);
			kill(chpid, SIGTERM);
			break;
		}

		/* Make the buffer null-terminated. */
		msg.text[result] = '\0';

#ifdef DEBUG_NDO2DB2
		printf("BYTESREAD: %d, RAWBUF: %s\n", result, msg.text);
#endif

		/* Send the new data to the DB handler. We send result = strlen(msg.text)
		 * bytes, we don't send the null byte.  */
		ndo2db_queue_send(&msg, (ssize_t)result);

		/* should we disconnect the client? */
		if (idi.disconnect_client) {
			/* gracefully back out of current operation... */
			ndo2db_db_goodbye(&idi);
			kill (chpid, SIGTERM);
			break;
		}
	}

	/* disconnect from database */
	ndo2db_db_disconnect(&idi);
	ndo2db_db_deinit(&idi);

	/* free memory */
	ndo2db_free_input_memory(&idi);
	ndo2db_free_connection_memory(&idi);

	/* Destruct our message queue. */
	ndo2db_queue_free();

	/* wait for child to end work */
	waitpid(chpid, NULL, 0);

	/* close syslog facility */
	/*closelog();*/

	return (error) ? NDO_ERROR : NDO_OK;
}


/* initializes structure for tracking data */
int ndo2db_idi_init(ndo2db_idi *idi) {
	int x = 0;

	if (!idi) return NDO_ERROR;

	idi->disconnect_client = NDO_FALSE;
	idi->ignore_client_data = NDO_FALSE;
	idi->protocol_version = 0;
	idi->instance_name = NULL;
	idi->buffered_input = NULL;
	idi->agent_name = NULL;
	idi->agent_version = NULL;
	idi->disposition = NULL;
	idi->connect_source = NULL;
	idi->connect_type = NULL;
	idi->current_input_section = NDO2DB_INPUT_SECTION_NONE;
	idi->current_input_data = NDO2DB_INPUT_DATA_NONE;
	idi->bytes_processed = 0;
	idi->lines_processed = 0;
	idi->entries_processed = 0;
	idi->current_object_config_type = NDO2DB_CONFIGTYPE_ORIGINAL;
	idi->data_start_time = 0;
	idi->data_end_time = 0;

	/* initialize mbuf */
	for (x = 0; x < NDO2DB_MAX_MBUF_ITEMS; x++) {
		idi->mbuf[x].used_lines = 0;
		idi->mbuf[x].allocated_lines = 0;
		idi->mbuf[x].buffer = NULL;
	}

	return NDO_OK;
}


/* asynchronous handle clients events */
void ndo2db_async_client_handle() {
	ndo2db_idi idi;

	/* initialize input data information */
	ndo2db_idi_init(&idi);

	/* initialize database connection */
	ndo2db_db_init(&idi);
	ndo2db_db_connect(&idi);

	char *old_buf = NULL;

	for (;;) {
		char *qbuf = pop_from_queue();
		char *buf;
		char *temp_buf;
		int i;
		int start = 0;

		if (old_buf != NULL) {
			buf = calloc(strlen(qbuf)+strlen(old_buf)+2, sizeof(char));

			strcat(buf, old_buf);
			strcat(buf, qbuf);

			free(old_buf); old_buf = NULL;
			free(qbuf);
		}
		else {
			buf = qbuf;
		}

		for (i = 0; i <= (int)strlen(buf); i++) {
			if (buf[i] == '\n') {
				int size = i-start;
				temp_buf = calloc(size+1, sizeof(char));
				strncpy(temp_buf, &buf[start], size);
				temp_buf[size] = '\x0';

				ndo2db_handle_client_input(&idi, temp_buf);

				free(temp_buf);

				start = i+1;

				idi.lines_processed++;
				idi.bytes_processed += size+1;
			}
		}

		if (start <= (int)strlen(buf)) {
			old_buf = calloc(strlen(&buf[start])+1, sizeof(char));
			strcpy(old_buf, &buf[start]);
		}

		free(buf);
	}

	if (old_buf) free(old_buf);

#ifdef DEBUG_NDO2DB2
	printf("IDI processed bytes: %lu, lines: %lu\n", idi.bytes_processed, idi.lines_processed);
#endif

	/* disconnect from database */
	ndo2db_stmt_free_stmts();
	ndo2db_db_disconnect(&idi);
	ndo2db_db_deinit(&idi);

	/* free memory */
	ndo2db_free_input_memory(&idi);
	ndo2db_free_connection_memory(&idi);
}

/* handles a single line of input from a client connection */
int ndo2db_handle_client_input(ndo2db_idi *idi, char *buf) {
	char *var = NULL;
	char *val = NULL;
	unsigned long data_type_long = 0;
	int data_type = NDO_DATA_NONE;
	int input_type = NDO2DB_INPUT_DATA_NONE;

#ifdef DEBUG_NDO2DB2
	printf("HANDLING: '%s'\n", buf);
#endif

	if (!buf || !idi) return NDO_ERROR;

	/* we're ignoring client data because of wrong protocol version, etc...  */
	if (idi->ignore_client_data) return NDO_ERROR;

	/* skip empty lines */
	if (buf[0] == '\0') return NDO_OK;

	switch (idi->current_input_section) {

	case NDO2DB_INPUT_SECTION_NONE:

		var = strtok(buf, ":");
		val = strtok(NULL, "\n");

		if (strcmp(var, NDO_API_HELLO) == 0) {

			idi->current_input_section = NDO2DB_INPUT_SECTION_HEADER;
			idi->current_input_data = NDO2DB_INPUT_DATA_NONE;

			/* free old connection memory (necessary in some cases) */
			ndo2db_free_connection_memory(idi);
		}

		break;

	case NDO2DB_INPUT_SECTION_HEADER:

		var = strtok(buf, ":");
		val = strtok(NULL, "\n");

		if (strcmp(var, NDO_API_STARTDATADUMP) == 0) {

			/* client is using wrong protocol version, bail out here... */
			if (idi->protocol_version != NDO_API_PROTOVERSION) {
				syslog(LOG_USER|LOG_INFO, "Error: Client protocol version %d is incompatible with server version %d.  Disconnecting client...", idi->protocol_version, NDO_API_PROTOVERSION);
				idi->disconnect_client = NDO_TRUE;
				idi->ignore_client_data = NDO_TRUE;
				return NDO_ERROR;
			}

			idi->current_input_section = NDO2DB_INPUT_SECTION_DATA;

			/* save connection info to DB */
			ndo2db_db_hello(idi);
			ndo2db_stmt_init_stmts(idi);
		}

		else if (strcmp(var, NDO_API_PROTOCOL) == 0)
			ndo2db_strtoi((val+1), &idi->protocol_version);

		else if (strcmp(var, NDO_API_INSTANCENAME) == 0)
			idi->instance_name = strdup(val+1);

		else if (strcmp(var, NDO_API_AGENT) == 0)
			idi->agent_name = strdup(val+1);

		else if (strcmp(var, NDO_API_AGENTVERSION) == 0)
			idi->agent_version = strdup(val+1);

		else if (strcmp(var, NDO_API_DISPOSITION) == 0)
			idi->disposition = strdup(val+1);

		else if (strcmp(var, NDO_API_CONNECTION) == 0)
			idi->connect_source = strdup(val+1);

		else if (strcmp(var, NDO_API_CONNECTTYPE) == 0)
			idi->connect_type = strdup(val+1);

		else if (strcmp(var, NDO_API_STARTTIME) == 0)
			ndo2db_strtoul((val+1), &idi->data_start_time);

		break;

	case NDO2DB_INPUT_SECTION_FOOTER:

		var = strtok(buf, ":");
		val = strtok(NULL, "\n");

		/* client is saying goodbye... */
		if (strcmp(var, NDO_API_GOODBYE) == 0)
			idi->current_input_section = NDO2DB_INPUT_SECTION_NONE;

		else if (strcmp(var, NDO_API_ENDTIME) == 0)
			ndo2db_strtoul((val+1), &idi->data_end_time);

		break;

	case NDO2DB_INPUT_SECTION_DATA:

		if (idi->current_input_data == NDO2DB_INPUT_DATA_NONE) {

			var = strtok(buf, ":");
			val = strtok(NULL, "\n");

			input_type = atoi(var);

			switch (input_type) {

			/* we're reached the end of all of the data... */
			case NDO_API_ENDDATADUMP:
				idi->current_input_section = NDO2DB_INPUT_SECTION_FOOTER;
				idi->current_input_data = NDO2DB_INPUT_DATA_NONE;
				break;

			/* config dumps */
			case NDO_API_STARTCONFIGDUMP:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONFIGDUMPSTART;
				break;
			case NDO_API_ENDCONFIGDUMP:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONFIGDUMPEND;
				break;

			/* archived data */
			case NDO_API_LOGENTRY:
				idi->current_input_data = NDO2DB_INPUT_DATA_LOGENTRY;
				break;

			/* realtime data */
			case NDO_API_PROCESSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_PROCESSDATA;
				break;
			case NDO_API_TIMEDEVENTDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_TIMEDEVENTDATA;
				break;
			case NDO_API_LOGDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_LOGDATA;
				break;
			case NDO_API_SYSTEMCOMMANDDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA;
				break;
			case NDO_API_EVENTHANDLERDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_EVENTHANDLERDATA;
				break;
			case NDO_API_NOTIFICATIONDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_NOTIFICATIONDATA;
				break;
			case NDO_API_SERVICECHECKDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICECHECKDATA;
				break;
			case NDO_API_HOSTCHECKDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTCHECKDATA;
				break;
			case NDO_API_COMMENTDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_COMMENTDATA;
				break;
			case NDO_API_DOWNTIMEDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_DOWNTIMEDATA;
				break;
			case NDO_API_FLAPPINGDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_FLAPPINGDATA;
				break;
			case NDO_API_PROGRAMSTATUSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA;
				break;
			case NDO_API_HOSTSTATUSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTSTATUSDATA;
				break;
			case NDO_API_SERVICESTATUSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICESTATUSDATA;
				break;
			case NDO_API_CONTACTSTATUSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONTACTSTATUSDATA;
				break;
			case NDO_API_ADAPTIVEPROGRAMDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA;
				break;
			case NDO_API_ADAPTIVEHOSTDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA;
				break;
			case NDO_API_ADAPTIVESERVICEDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA;
				break;
			case NDO_API_ADAPTIVECONTACTDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA;
				break;
			case NDO_API_EXTERNALCOMMANDDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA;
				break;
			case NDO_API_AGGREGATEDSTATUSDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA;
				break;
			case NDO_API_RETENTIONDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_RETENTIONDATA;
				break;
			case NDO_API_CONTACTNOTIFICATIONDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA;
				break;
			case NDO_API_CONTACTNOTIFICATIONMETHODDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA;
				break;
			case NDO_API_ACKNOWLEDGEMENTDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA;
				break;
			case NDO_API_STATECHANGEDATA:
				idi->current_input_data = NDO2DB_INPUT_DATA_STATECHANGEDATA;
				break;

			/* config variables */
			case NDO_API_MAINCONFIGFILEVARIABLES:
				idi->current_input_data = NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES;
				break;
			case NDO_API_RESOURCECONFIGFILEVARIABLES:
				idi->current_input_data = NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES;
				break;
			case NDO_API_CONFIGVARIABLES:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONFIGVARIABLES;
				break;
			case NDO_API_RUNTIMEVARIABLES:
				idi->current_input_data = NDO2DB_INPUT_DATA_RUNTIMEVARIABLES;
				break;

			/* object configuration */
			case NDO_API_HOSTDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTDEFINITION;
				break;
			case NDO_API_HOSTGROUPDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION;
				break;
			case NDO_API_SERVICEDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICEDEFINITION;
				break;
			case NDO_API_SERVICEGROUPDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION;
				break;
			case NDO_API_HOSTDEPENDENCYDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION;
				break;
			case NDO_API_SERVICEDEPENDENCYDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION;
				break;
			case NDO_API_HOSTESCALATIONDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION;
				break;
			case NDO_API_SERVICEESCALATIONDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION;
				break;
			case NDO_API_COMMANDDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_COMMANDDEFINITION;
				break;
			case NDO_API_TIMEPERIODDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION;
				break;
			case NDO_API_CONTACTDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONTACTDEFINITION;
				break;
			case NDO_API_CONTACTGROUPDEFINITION:
				idi->current_input_data = NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION;
				break;
			case NDO_API_HOSTEXTINFODEFINITION:
				/* deprecated - merged with host definitions */
			case NDO_API_SERVICEEXTINFODEFINITION:
				/* deprecated - merged with service definitions */
			default:
				break;
			}

			/* initialize input data */
			ndo2db_start_input_data(idi);
		}

		/* we are processing some type of data already... */
		else {

			var = strtok(buf, "=");
			val = strtok(NULL, "\n");

			/* get the data type */
			data_type_long = strtoul(var, NULL, 0);

			/* there was an error with the data type - throw it out */
			if (data_type_long == ULONG_MAX && errno == ERANGE) break;

			data_type = (int)data_type_long;

			/* the current data section is ending... */
			if (data_type == NDO_API_ENDDATA) {

				/* finish current data processing */
				ndo2db_end_input_data(idi);

				idi->current_input_data = NDO2DB_INPUT_DATA_NONE;
			}

			/* add data for already existing data type... */
			else {

				/* the data type is out of range - throw it out */
				if (data_type > NDO_MAX_DATA_TYPES) {
#ifdef DEBUG_NDO2DB2
					printf("## DISCARD! LINE: %lu, TYPE: %d, VAL: %s\n", idi->lines_processed, data_type, val);
#endif
					break;
				}

#ifdef DEBUG_NDO2DB2
				printf("LINE: %lu, TYPE: %d, VAL:%s\n", idi->lines_processed, data_type, val);
#endif
				ndo2db_add_input_data_item(idi, data_type, val);
			}
		}

		break;

	default:
		break;
	}

	return NDO_OK;
}


int ndo2db_start_input_data(ndo2db_idi *idi) {

	if (!idi) return NDO_ERROR;

	/* sometimes ndo2db_end_input_data() isn't called, so free memory if we find it */
	ndo2db_free_input_memory(idi);

	/* allocate memory for holding buffered input */
	idi->buffered_input = calloc(NDO_MAX_DATA_TYPES, sizeof(char *));
	return idi->buffered_input ? NDO_OK : NDO_ERROR;
}


int ndo2db_add_input_data_item(ndo2db_idi *idi, int type, char *buf) {
	char *newbuf = NULL;

	if (!idi) return NDO_ERROR;

	/* escape data if necessary */
	switch (type) {

	case NDO_DATA_ACKAUTHOR:
	case NDO_DATA_ACKDATA:
	case NDO_DATA_AUTHORNAME:
	case NDO_DATA_CHECKCOMMAND:
	case NDO_DATA_COMMANDARGS:
	case NDO_DATA_COMMANDLINE:
	case NDO_DATA_COMMANDSTRING:
	case NDO_DATA_COMMENT:
	case NDO_DATA_EVENTHANDLER:
	case NDO_DATA_GLOBALHOSTEVENTHANDLER:
	case NDO_DATA_GLOBALSERVICEEVENTHANDLER:
	case NDO_DATA_HOST:
	case NDO_DATA_LOGENTRY:
	case NDO_DATA_OUTPUT:
	case NDO_DATA_LONGOUTPUT:
	case NDO_DATA_PERFDATA:
	case NDO_DATA_SERVICE:
	case NDO_DATA_PROGRAMNAME:
	case NDO_DATA_PROGRAMVERSION:
	case NDO_DATA_PROGRAMDATE:

	case NDO_DATA_COMMANDNAME:
	case NDO_DATA_CONTACTADDRESS:
	case NDO_DATA_CONTACTALIAS:
	case NDO_DATA_CONTACTGROUP:
	case NDO_DATA_CONTACTGROUPALIAS:
	case NDO_DATA_CONTACTGROUPMEMBER:
	case NDO_DATA_CONTACTGROUPNAME:
	case NDO_DATA_CONTACTNAME:
	case NDO_DATA_DEPENDENTHOSTNAME:
	case NDO_DATA_DEPENDENTSERVICEDESCRIPTION:
	case NDO_DATA_EMAILADDRESS:
	case NDO_DATA_HOSTADDRESS:
	case NDO_DATA_HOSTALIAS:
	case NDO_DATA_HOSTCHECKCOMMAND:
	case NDO_DATA_HOSTCHECKPERIOD:
	case NDO_DATA_HOSTEVENTHANDLER:
	case NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS:
	case NDO_DATA_HOSTGROUPALIAS:
	case NDO_DATA_HOSTGROUPMEMBER:
	case NDO_DATA_HOSTGROUPNAME:
	case NDO_DATA_HOSTNAME:
	case NDO_DATA_HOSTNOTIFICATIONCOMMAND:
	case NDO_DATA_HOSTNOTIFICATIONPERIOD:
	case NDO_DATA_PAGERADDRESS:
	case NDO_DATA_PARENTHOST:
	case NDO_DATA_SERVICECHECKCOMMAND:
	case NDO_DATA_SERVICECHECKPERIOD:
	case NDO_DATA_SERVICEDESCRIPTION:
	case NDO_DATA_SERVICEEVENTHANDLER:
	case NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS:
	case NDO_DATA_SERVICEGROUPALIAS:
	case NDO_DATA_SERVICEGROUPMEMBER:
	case NDO_DATA_SERVICEGROUPNAME:
	case NDO_DATA_SERVICENOTIFICATIONCOMMAND:
	case NDO_DATA_SERVICENOTIFICATIONPERIOD:
	case NDO_DATA_TIMEPERIODALIAS:
	case NDO_DATA_TIMEPERIODNAME:
	case NDO_DATA_TIMERANGE:

	case NDO_DATA_ACTIONURL:
	case NDO_DATA_ICONIMAGE:
	case NDO_DATA_ICONIMAGEALT:
	case NDO_DATA_NOTES:
	case NDO_DATA_NOTESURL:
	case NDO_DATA_CUSTOMVARIABLE:
	case NDO_DATA_CONTACT:
	case NDO_DATA_PARENTSERVICE:

		/* strings are escaped when they arrive */
		newbuf = strdup(buf ? buf : "");
		ndo_unescape_buffer(newbuf);
		break;

	default:
		/* data hasn't been escaped */
		newbuf = strdup(buf ? buf : "");
		break;
	}

	/* check for errors */
	if (!newbuf) {
#ifdef DEBUG_NDO2DB
		printf("ALLOCATION ERROR\n");
#endif
		return NDO_ERROR;
	}

	/* store the buffered data */
	switch (type) {

	/* special case for data items that may appear multiple times */
	case NDO_DATA_CONTACTGROUP:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONTACTGROUP, newbuf);
		break;
	case NDO_DATA_CONTACTGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONTACTGROUPMEMBER, newbuf);
		break;
	case NDO_DATA_SERVICEGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_SERVICEGROUPMEMBER, newbuf);
		break;
	case NDO_DATA_HOSTGROUPMEMBER:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_HOSTGROUPMEMBER, newbuf);
		break;
	case NDO_DATA_SERVICENOTIFICATIONCOMMAND:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND, newbuf);
		break;
	case NDO_DATA_HOSTNOTIFICATIONCOMMAND:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND, newbuf);
		break;
	case NDO_DATA_CONTACTADDRESS:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONTACTADDRESS, newbuf);
		break;
	case NDO_DATA_TIMERANGE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_TIMERANGE, newbuf);
		break;
	case NDO_DATA_PARENTHOST:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_PARENTHOST, newbuf);
		break;
	case NDO_DATA_CONFIGFILEVARIABLE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONFIGFILEVARIABLE, newbuf);
		break;
	case NDO_DATA_CONFIGVARIABLE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONFIGVARIABLE, newbuf);
		break;
	case NDO_DATA_RUNTIMEVARIABLE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_RUNTIMEVARIABLE, newbuf);
		break;
	case NDO_DATA_CUSTOMVARIABLE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CUSTOMVARIABLE, newbuf);
		break;
	case NDO_DATA_CONTACT:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_CONTACT, newbuf);
		break;
	case NDO_DATA_PARENTSERVICE:
		ndo2db_add_input_data_mbuf(idi, type, NDO2DB_MBUF_PARENTSERVICE, newbuf);
		break;

	/* NORMAL DATA */
	/* normal data items appear only once per data type */
	default:
		/* if there was already a matching item, discard the old one */
		my_free(idi->buffered_input[type]);
		/* save buffered item */
		idi->buffered_input[type] = newbuf;
		break;
	}

	return NDO_OK;
}



int ndo2db_add_input_data_mbuf(ndo2db_idi *idi, int type, int mbuf_slot, char *buf) {
	(void)type; /* Unused, don't warn. */

	int allocation_chunk = 80;
	char **newbuffer = NULL;

	if (!idi || !buf) return NDO_ERROR;

	if (mbuf_slot >= NDO2DB_MAX_MBUF_ITEMS) return NDO_ERROR;

	/* create buffer */
	if (!idi->mbuf[mbuf_slot].buffer) {
#ifdef NDO2DB_DEBUG_MBUF
		mbuf_bytes_allocated += allocation_chunk * sizeof(char *);
		printf("MBUF INITIAL ALLOCATION (MBUF = %lu bytes)\n", mbuf_bytes_allocated);
#endif
		idi->mbuf[mbuf_slot].buffer = malloc(sizeof(char *)*allocation_chunk);
		idi->mbuf[mbuf_slot].allocated_lines += allocation_chunk;
	}

	/* expand buffer */
	if (idi->mbuf[mbuf_slot].used_lines == idi->mbuf[mbuf_slot].allocated_lines) {
		newbuffer = realloc(idi->mbuf[mbuf_slot].buffer, sizeof(char *)*(idi->mbuf[mbuf_slot].allocated_lines+allocation_chunk));
		if (!newbuffer) return NDO_ERROR;
#ifdef NDO2DB_DEBUG_MBUF
		mbuf_bytes_allocated += allocation_chunk * sizeof(char *);
		printf("MBUF RESIZED (MBUF = %lu bytes)\n", mbuf_bytes_allocated);
#endif
		idi->mbuf[mbuf_slot].buffer = newbuffer;
		idi->mbuf[mbuf_slot].allocated_lines += allocation_chunk;
	}

	/* store the data */
	if (!idi->mbuf[mbuf_slot].buffer) return NDO_ERROR;

	idi->mbuf[mbuf_slot].buffer[idi->mbuf[mbuf_slot].used_lines] = buf;
	idi->mbuf[mbuf_slot].used_lines++;
	return NDO_OK;
}



int ndo2db_end_input_data(ndo2db_idi *idi) {
	int result = NDO_OK;

	if (!idi) return NDO_ERROR;

	/* update db stats occassionally */
	if (ndo2db_db_last_checkin_time < (time(NULL)-60)) ndo2db_db_checkin(idi);

#ifdef DEBUG_NDO2DB2
	printf("HANDLING TYPE: %d\n", idi->current_input_data);
#endif

	switch (idi->current_input_data) {

	/* archived log entries */
	case NDO2DB_INPUT_DATA_LOGENTRY:
		result = ndo2db_handle_logentry(idi);
		break;

	/* realtime Nagios data */
	case NDO2DB_INPUT_DATA_PROCESSDATA:
		result = ndo2db_handle_processdata(idi);
		break;
	case NDO2DB_INPUT_DATA_TIMEDEVENTDATA:
		result = ndo2db_handle_timedeventdata(idi);
		break;
	case NDO2DB_INPUT_DATA_LOGDATA:
		result = ndo2db_handle_logdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA:
		result = ndo2db_handle_systemcommanddata(idi);
		break;
	case NDO2DB_INPUT_DATA_EVENTHANDLERDATA:
		result = ndo2db_handle_eventhandlerdata(idi);
		break;
	case NDO2DB_INPUT_DATA_NOTIFICATIONDATA:
		result = ndo2db_handle_notificationdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICECHECKDATA:
// 		result = ndo2db_handle_servicecheckdata(idi);
		result = ndo2db_stmt_handle_servicecheckdata(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTCHECKDATA:
// 		result = ndo2db_handle_hostcheckdata(idi);
		result = ndo2db_stmt_handle_hostcheckdata(idi);
		break;
	case NDO2DB_INPUT_DATA_COMMENTDATA:
		result = ndo2db_handle_commentdata(idi);
		break;
	case NDO2DB_INPUT_DATA_DOWNTIMEDATA:
		result = ndo2db_handle_downtimedata(idi);
		break;
	case NDO2DB_INPUT_DATA_FLAPPINGDATA:
		result = ndo2db_handle_flappingdata(idi);
		break;
	case NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA:
		result = ndo2db_handle_programstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTSTATUSDATA:
// 		result = ndo2db_handle_hoststatusdata(idi);
		result = ndo2db_stmt_handle_hoststatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICESTATUSDATA:
// 		result = ndo2db_handle_servicestatusdata(idi);
		result = ndo2db_stmt_handle_servicestatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTSTATUSDATA:
		result = ndo2db_handle_contactstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA:
// 		result = ndo2db_handle_adaptiveprogramdata(idi);
		result = ndo2db_stmt_handle_adaptiveprogramdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA:
// 		result = ndo2db_handle_adaptivehostdata(idi);
		result = ndo2db_stmt_handle_adaptivehostdata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA:
// 		result = ndo2db_handle_adaptiveservicedata(idi);
		result = ndo2db_stmt_handle_adaptiveservicedata(idi);
		break;
	case NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA:
// 		result = ndo2db_handle_adaptivecontactdata(idi);
		result = ndo2db_stmt_handle_adaptivecontactdata(idi);
		break;
	case NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA:
		result = ndo2db_handle_externalcommanddata(idi);
		break;
	case NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA:
// 		result = ndo2db_handle_aggregatedstatusdata(idi);
		result = ndo2db_stmt_handle_aggregatedstatusdata(idi);
		break;
	case NDO2DB_INPUT_DATA_RETENTIONDATA:
// 		result = ndo2db_handle_retentiondata(idi);
		result = ndo2db_stmt_handle_retentiondata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA:
		result = ndo2db_handle_contactnotificationdata(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA:
		result = ndo2db_handle_contactnotificationmethoddata(idi);
		break;
	case NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA:
		result = ndo2db_handle_acknowledgementdata(idi);
		break;
	case NDO2DB_INPUT_DATA_STATECHANGEDATA:
		result = ndo2db_handle_statechangedata(idi);
		break;

	/* config file and variable dumps */
	case NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES:
// 		result = ndo2db_handle_configfilevariables(idi, 0);
		result = ndo2db_stmt_handle_configfilevariables(idi, 0);
		break;
	case NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES:
// 		result = ndo2db_handle_configfilevariables(idi, 1);
		result = ndo2db_stmt_handle_configfilevariables(idi, 1);
		break;
	case NDO2DB_INPUT_DATA_CONFIGVARIABLES:
// 		result = ndo2db_handle_configvariables(idi);
		result = ndo2db_stmt_handle_configvariables(idi);
		break;
	case NDO2DB_INPUT_DATA_RUNTIMEVARIABLES:
// 		result = ndo2db_handle_runtimevariables(idi);
		result = ndo2db_stmt_handle_runtimevariables(idi);
		break;
	case NDO2DB_INPUT_DATA_CONFIGDUMPSTART:
// 		result = ndo2db_handle_configdumpstart(idi);
		result = ndo2db_stmt_handle_configdumpstart(idi);
		break;
	case NDO2DB_INPUT_DATA_CONFIGDUMPEND:
// 		result = ndo2db_handle_configdumpend(idi);
		result = ndo2db_stmt_handle_configdumpend(idi);
		break;

	/* config definitions */
	case NDO2DB_INPUT_DATA_HOSTDEFINITION:
// 		result = ndo2db_handle_hostdefinition(idi);
		result = ndo2db_stmt_handle_hostdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION:
// 		result = ndo2db_handle_hostgroupdefinition(idi);
		result = ndo2db_stmt_handle_hostgroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEDEFINITION:
// 		result = ndo2db_handle_servicedefinition(idi);
		result = ndo2db_stmt_handle_servicedefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION:
// 		result = ndo2db_handle_servicegroupdefinition(idi);
		result = ndo2db_stmt_handle_servicegroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION:
// 		result = ndo2db_handle_hostdependencydefinition(idi);
		result = ndo2db_stmt_handle_hostdependencydefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION:
// 		result = ndo2db_handle_servicedependencydefinition(idi);
		result = ndo2db_stmt_handle_servicedependencydefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION:
// 		result = ndo2db_handle_hostescalationdefinition(idi);
		result = ndo2db_stmt_handle_hostescalationdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION:
// 		result = ndo2db_handle_serviceescalationdefinition(idi);
		result = ndo2db_stmt_handle_serviceescalationdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_COMMANDDEFINITION:
// 		result = ndo2db_handle_commanddefinition(idi);
		result = ndo2db_stmt_handle_commanddefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION:
// 		result = ndo2db_handle_timeperiodefinition(idi);
		result = ndo2db_stmt_handle_timeperiodefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTDEFINITION:
// 		result = ndo2db_handle_contactdefinition(idi);
		result = ndo2db_stmt_handle_contactdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION:
// 		result = ndo2db_handle_contactgroupdefinition(idi);
		result = ndo2db_stmt_handle_contactgroupdefinition(idi);
		break;
	case NDO2DB_INPUT_DATA_HOSTEXTINFODEFINITION:
		/* deprecated - merged with host definitions */
		break;
	case NDO2DB_INPUT_DATA_SERVICEEXTINFODEFINITION:
		/* deprecated - merged with service definitions */
		break;

	default:
		break;
	}

	/* free input memory */
	ndo2db_free_input_memory(idi);

	/* adjust items processed */
	idi->entries_processed++;

	/* perform periodic maintenance... */
	ndo2db_db_perform_maintenance(idi);

	return result;
}


/* free memory allocated to data input */
int ndo2db_free_input_memory(ndo2db_idi *idi) {
	int x = 0;
	int y = 0;

	if (!idi) return NDO_ERROR;

	/* Free memory allocated to single-instance data buffers. */
	if (idi->buffered_input) {
		for (x = 0; x < NDO_MAX_DATA_TYPES; x++) {
			if (idi->buffered_input[x]) free(idi->buffered_input[x]);
		}
		free(idi->buffered_input);
		idi->buffered_input = NULL;
	}

	/* Free memory allocated to multi-instance data buffers. */
	if (idi->mbuf) {
		for (x = 0; x < NDO2DB_MAX_MBUF_ITEMS; x++) {
			if (idi->mbuf[x].buffer) {
				for (y = 0; y < idi->mbuf[x].used_lines; y++) {
					if (idi->mbuf[x].buffer[y]) free(idi->mbuf[x].buffer[y]);
				}
				free(idi->mbuf[x].buffer);
				idi->mbuf[x].buffer = NULL;
			}
			idi->mbuf[x].used_lines = 0;
			idi->mbuf[x].allocated_lines = 0;
		}
	}

	return NDO_OK;
}


/* free memory allocated to connection */
int ndo2db_free_connection_memory(ndo2db_idi *idi) {

	my_free(idi->instance_name);
	my_free(idi->agent_name);
	my_free(idi->agent_version);
	my_free(idi->disposition);
	my_free(idi->connect_source);
	my_free(idi->connect_type);

	return NDO_OK;
}



/****************************************************************************/
/* DATA TYPE CONVERTION ROUTINES                                            */
/****************************************************************************/

int ndo2db_convert_standard_data_elements(ndo2db_idi *idi, int *type, int *flags, int *attr, struct timeval *tstamp) {
	int result1 = NDO_OK;
	int result2 = NDO_OK;
	int result3 = NDO_OK;
	int result4 = NDO_OK;

	result1 = ndo2db_strtoi(idi->buffered_input[NDO_DATA_TYPE], type);
	result2 = ndo2db_strtoi(idi->buffered_input[NDO_DATA_FLAGS], flags);
	result3 = ndo2db_strtoi(idi->buffered_input[NDO_DATA_ATTRIBUTES], attr);
	result4 = ndo2db_strtotv(idi->buffered_input[NDO_DATA_TIMESTAMP], tstamp);

	if (result1 == NDO_ERROR || result2 == NDO_ERROR || result3 == NDO_ERROR || result4 == NDO_ERROR)
		return NDO_ERROR;

	return NDO_OK;
}


int ndo2db_strtoschar(const char *buf, signed char *c) {
	long l = 0;
	int status = ndo2db_strtol(buf, &l);
	*c = (signed char)l;
	if (status != NDO_OK) {
		return status;
	}
	else {
		return (CHAR_MIN <= l && l <= CHAR_MAX) ? NDO_OK : NDO_ERROR;
	}
}


int ndo2db_strtoshort(const char *buf, signed short *s) {
	long l = 0;
	int status = ndo2db_strtol(buf, &l);
	*s = (signed short)l;
	if (status != NDO_OK) {
		return status;
	}
	else {
		return (SHRT_MIN <= l && l <= SHRT_MAX) ? NDO_OK : NDO_ERROR;
	}
}


int ndo2db_strtoint(const char *buf, signed int *i) {
	long l = 0;
	int status = ndo2db_strtol(buf, &l);
	*i = (signed int)l;
	if (status != NDO_OK) {
		return status;
	}
	else {
		return (INT_MIN <= l && l <= INT_MAX) ? NDO_OK : NDO_ERROR;
	}
}


int ndo2db_strtouint(const char *buf, unsigned int *u) {
	unsigned long l = 0;
	int status = ndo2db_strtoul(buf, &l);
	*u = (unsigned int)l;
	if (status != NDO_OK) {
		return status;
	}
	else {
		return (l <= UINT_MAX) ? NDO_OK : NDO_ERROR;
	}
}

int ndo2db_strtoi(const char *buf, int *i) {

	if (!buf) return NDO_ERROR;

	*i = atoi(buf);

	return NDO_OK;
}


int ndo2db_strtof(const char *buf, float *f) {
	char *endptr = NULL;

	if (!buf) return NDO_ERROR;

	errno = 0;
#ifdef HAVE_STRTOF
	*f = strtof(buf, &endptr);
#else
	/* Solaris 8 doesn't have strtof() */
	*f = (float)strtod(buf, &endptr);
#endif

	return ((*f == 0 && endptr == buf) || errno == ERANGE) ? NDO_ERROR : NDO_OK;
}


int ndo2db_strtod(const char *buf, double *d) {
	char *endptr = NULL;

	if (!buf) return NDO_ERROR;

	errno = 0;
	*d = strtod(buf, &endptr);

	return ((*d == 0 && endptr == buf) || errno == ERANGE) ? NDO_ERROR : NDO_OK;
}


int ndo2db_strtol(const char *buf, long *l) {
	char *endptr = NULL;

	if (!buf) return NDO_ERROR;

	errno = 0;
	*l = strtol(buf, &endptr, 0);

	if (*l == LONG_MAX && errno == ERANGE) return NDO_ERROR;
	if (*l == 0 && endptr == buf) return NDO_ERROR;
	return NDO_OK;
}


int ndo2db_strtoul(const char *buf, unsigned long *ul) {
	char *endptr = NULL;

	if (!buf) return NDO_ERROR;

	errno = 0;
	*ul = strtoul(buf, &endptr, 0);

	if (*ul == ULONG_MAX && errno == ERANGE) return NDO_ERROR;
	if (*ul == 0 && endptr == buf) return NDO_ERROR;
	return NDO_OK;
}


int ndo2db_strtotv(const char *buf, struct timeval *tv) {
	char *newbuf = NULL;
	char *ptr = NULL;
	int result = NDO_OK;

	if (!buf) return NDO_ERROR;

	*tv = (struct timeval){ 0, 0 };

	if (!(newbuf = strdup(buf))) return NDO_ERROR;

	ptr = strtok(newbuf, ".");
	result = ndo2db_strtoul(ptr, (unsigned long *)&tv->tv_sec);
	if (result == NDO_OK) {
		ptr = strtok(NULL, "\n");
		result = ndo2db_strtoul(ptr, (unsigned long *)&tv->tv_usec);
	}

	free(newbuf);

	return result;
}



/****************************************************************************/
/* LOGGING ROUTINES                                                         */
/****************************************************************************/

/* Opens the debug log for writing. */
int ndo2db_open_debug_log(void) {

	/* Don't do anything if we're not debugging. */
	if (ndo2db_debug_level == NDO2DB_DEBUGL_NONE) return NDO_OK;

	if (ndo2db_debug_file_fp) fclose(ndo2db_debug_file_fp);
	ndo2db_debug_file_fp = fopen(ndo2db_debug_file, "a+");
	if (!ndo2db_debug_file_fp) {
		syslog(LOG_ERR, "Warning: Could not open debug file '%s' - '%s'",
				ndo2db_debug_file, strerror(errno));
		return NDO_ERROR;
	}

	return NDO_OK;
}


/* Closes the debug log. */
int ndo2db_close_debug_log(void) {

	if (ndo2db_debug_file_fp) {
		fclose(ndo2db_debug_file_fp);
		ndo2db_debug_file_fp = NULL;
	}
	return NDO_OK;
}


/* Write to the debug log. */
int ndo2db_log_debug_info(int level, int verbosity, const char *fmt, ...) {
	va_list ap;
	char *temp_path = NULL;
	struct timeval current_time;

	if (!(ndo2db_debug_level == NDO2DB_DEBUGL_ALL || (level & ndo2db_debug_level)))
		return NDO_OK;

	if (verbosity > ndo2db_debug_verbosity)
		return NDO_OK;

	if (!ndo2db_debug_file_fp)
		return NDO_ERROR;

	/* Write the timestamp. */
	gettimeofday(&current_time, NULL);
	fprintf(ndo2db_debug_file_fp, "[%lu.%06lu] [%03d.%d] [pid=%lu] ",
			(unsigned long)current_time.tv_sec, (unsigned long)current_time.tv_usec,
			level, verbosity, (unsigned long)getpid());

	/* write the data */
	va_start(ap, fmt);
	vfprintf(ndo2db_debug_file_fp, fmt, ap);
	va_end(ap);

	/* Flush, so we don't have problems tailing or when fork()ing. */
	fflush(ndo2db_debug_file_fp);

	/* If file has grown beyond max, rotate it. */
	if (ndo2db_max_debug_file_size > 0
			&& (unsigned long)ftell(ndo2db_debug_file_fp) > ndo2db_max_debug_file_size) {

		/* Close the file... */
		ndo2db_close_debug_log();

		/* ...rotate the old log file... */
		if (asprintf(&temp_path, "%s.old", ndo2db_debug_file) != -1) {
			/* unlink any previous old debug file */
			unlink(temp_path);
			/* rotate the debug file */
			my_rename(ndo2db_debug_file, temp_path);
			/* free memory */
			free(temp_path);
		}

		/* open a new file */
		ndo2db_open_debug_log();
	}

	return NDO_OK;
}
