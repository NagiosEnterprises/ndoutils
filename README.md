# NDO 3

[![Build Status](https://travis-ci.org/NagiosEnterprises/ndoutils.svg?branch=ndo-3)](https://travis-ci.org/NagiosEnterprises/ndoutils)

The NDOUtils addon allows you to move status and event information from Nagios to a MySQL Database for later retrieval and processing.
NDOUtils compiles to a shared-object file, ndo.so, which can be used as a broker module in Nagios Core. This broker module uses a separate
configuration file 

Upgrading From NDOUtils 2.x
---------------------------

Before upgrading to NDOUtils 3.0.0, you must upgrade to NDOUtils 2.1.3. 
The upgrade process from earlier versions is unlikely to work and unsupported.

Upgrades can use the same process as fresh installations, with the following addenda:
1. When compiling, `make install-broker-line` will comment out the default broker line from previous versions of NDOUtils.
2. When performing database initialization, skip steps 1 and 2. Perform steps 3 and 4 using the same credentials/database that were used for NDOUtils 2.
3. Configuration options are not compatible across versions. Note especially that SSL connections are performed using built-in mysql functions, rather than our own OpenSSL/GnuTLS layer. This means that the options available will depend on the version of MySQL/MariaDB installed.


Compiling
---------

Use the following commands to compile and install the NDO broker module:

    ./configure
    make all
    make install

You can also use `make` to modify your nagios configuration file and add install the broker's sample configuration file:

    make install-broker-line
    make install-config

Initializing the Database
-------------------------

Before you start using NDOUtils, you should create the database where
you will be storing all Nagios related information.

**Note: Only MySQL Databases are supported!**

1.  Create a database for storing the data (e.g. `nagios`)

2.  Create a username/password (e.g. `ndoutils`) that has at least the following privileges for
    the database:

        SELECT, INSERT, UPDATE, DELETE

3.  Run the DB installation script in the `db/` subdirectory of the NDO distribution
    to create the necessary tables in the database.

        cd db
        ./db-mgmt.sh -u <username> -p <password> -h <hostname> -P <port> -d <database>

    The username and password both default to `ndoutils`; the hostname, to `localhost`; the port, to `3306`; and the database, to `nagios`.
    You will need to manually specify only the options which differ from these defaults.

4.  Make sure the database name, prefix, and username/password you just created
    and setup match the variables specified in the NDO2DB config file (see below).

Configuring the Broker Module
-----------------------------

If you installed the configuration file from the instructions above, you can access the installed file in your `nagios/etc` folder under `ndo.cfg`.
The configuration file can be used to specify the following settings:

1. Database connection settings. The following settings should match the ones you used to set up the database in `db-mgmt.sh`.
  - db_host
  - db_name
  - db_user
  - db_pass
  - db_port
  - db_socket

2. Data processing settings. The following settings determine which tables are populated with data. Each one should be set to "1" or "0".
  - process_data
  - timed_event_data
  - log_data
  - system_command_data
  - event_handler_data
  - host_check_data
  - service_check_data
  - comment_data
  - downtime_data
  - flapping_data
  - program_status_data
  - host_status_data
  - service_status_data
  - contact_status_data
  - notification_data
  - external_command_data
  - acknowledgement_data
  - state_change_data

3. Database SSL settings. These are dependent on the version of MySQL/MariaDB that you're using. See https://dev.mysql.com/doc/refman/8.0/en/mysql-options.html for more information on each specific setting. If a setting is not available for your current database, it will be ignored.

  - mysql_opt_ssl_ca (>= 5.6.36)
  - mysql_opt_ssl_capath (>= 5.6.36)
  - mysql_opt_ssl_cert (>= 5.6.36)
  - mysql_opt_ssl_cipher (>= 5.6.36)
  - mysql_opt_ssl_crl (>= 5.6.36)
  - mysql_opt_ssl_crlpath (>= 5.6.36)
  - mysql_opt_ssl_key (>= 5.6.36)

Not available on MariaDB

   - mysql_opt_ssl_mode. This can be set to "SSL_MODE_REQUIRED" for versions >=5.6.36 or >=5.5.55. For version >= 5.07.11, it can also be one of "SSL_MODE_DISABLED", "SSL_MODE_PREFERRED", "SSL_MODE_VERIFY_CA", or "SSL_MODE_VERIFY_IDENTITY".
   - mysql_opt_tls_ciphersuites (>=8.0.15)
   - mysql_opt_tls_version (>=5.7.10)


4. Developer options. Don't touch these unless you know what you're doing.

  - db_max_reconnect_attempts
  - config_dump_options
  - max_object_insert_count
  - enable_startup_hash
  - startup_hash_script_path
  - mysql_set_charset_name
