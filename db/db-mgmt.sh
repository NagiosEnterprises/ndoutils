#!/bin/bash

# current version
version="3.0.0"

# previous versions we perform upgrades for
versions=("2.0.1" "2.1.2" "2.1.3" "3.0.0")

# these get updated during some checks later on
install=TRUE
upgrade=FALSE

basedir=$(dirname $(readlink -f $0))

# first argument is string containing required params for usage line
# second argument is string containing optional params
# third argument is additional help message [string] to print
function print_help ()
{
    echo "NDOUtils Database Management"
    echo "$0 -u <db user> -p <db pass> -h <db host> -P <db port> -d <db name> $1[ -H $2]"
    echo ""
    echo "  -u   Database user"
    echo "       default: 'ndo'"
    echo ""
    echo "  -p   Database password"
    echo "       default: 'ndo'"
    echo ""
    echo "  -h   Database host"
    echo "       default: 'localhost'"
    echo ""
    echo "  -P   Database port"
    echo "       default: 3306"
    echo ""
    echo "  -d   Database name"
    echo "       default: 'ndo'"
    echo ""
    echo "  -H   Print help and exit"
    echo ""
    echo "$3"
}

function default_args ()
{
    dbuser=ndo
    dbpass=ndo
    dbhost=localhost
    dbport=3306
    dbname=ndo
}

function process_args ()
{
    while getopts "u:p:h:P:d:H" opt; do
        case ${opt} in
        u )
            dbuser=$OPTARG
            ;;
        p )
            dbpass=$OPTARG
            ;;
        h )
            dbhost=$OPTARG
            ;;
        P )
            dbport=$OPTARG
            ;;
        d )
            dbname=$OPTARG
            ;;
        H )
            print_help
            exit 0
            ;;
        esac
    done
    shift $(( OPTIND - 1 ))
}

function debug_args ()
{
    echo "dbuser: $dbuser"
    echo "dbpass: $dbpass"
    echo "dbhost: $dbhost"
    echo "dbport: $dbport"
    echo "dbname: $dbname"
}

function mysql_exec ()
{
    use_host=""
    if [ "x$dbhost" != "xlocalhost" ]; then
        use_host="-h $dbhost"
    fi

    use_port=""
    if [ $(( $dbport )) -ne 3306 ]; then
        use_port="-P $dbport"
    fi

    if [ "x$use_flags" = "x" ]; then
        use_flags=""
    fi

    mysql -u$dbuser -p$dbpass $dbname $use_host $use_port $use_flags -e "$@"
}

function check_mysql_creds ()
{
    mysql_exec "SELECT 1" >/dev/null 2>&1
    echo $?
}

function get_database_version ()
{
    use_flags="-sN"
    mysql_exec "SELECT version FROM nagios_dbversion LIMIT 1" 2>/dev/null
    use_flags=""
}

function set_database_version ()
{
    mysql_exec "TRUNCATE TABLE nagios_dbversion"
    mysql_exec "INSERT INTO nagios_dbversion (name, version) VALUES ('ndoutils', '$version')"
}

function check_database_installed ()
{
    mysql_exec "USE $dbname" >/dev/null 2>&1
    echo $?
}

function check_database_populated ()
{
    mysql_exec "SELECT host_id FROM nagios_hosts LIMIT 1" >/dev/null 2>&1
    echo $?
}

function check_database_version_installed ()
{
    get_database_version >/dev/null 2>&1
    echo $?
}

function install_database_version_table ()
{
    mysql_exec "CREATE TABLE nagios_dbversion (name VARCHAR(10) NOT NULL, version VARCHAR(10) NOT NULL)"
}

# thanks stackoverflow #16989598/bash-comparing-version-numbers
function version_gt ()
{
    test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"
}

# some notes:
#############

#############
# check if database exists
# if database exists and has nagios_hosts table, then its an upgrade
# otherwise, it's a fresh install

#############
# if install:
# get root password
# create username,password,db
# populate schema
# set dbversion

#############
# if upgrade:
# if version is undetected, then output error and exit
# if version is older than last supported, output error and exit
# if version is one of the supported versions...
# loop over from $from_version to $version
# and execute each schema upgrade file

##############################################################################
##############################################################################
# Handle arguments and do quick sanity
##############################################################################
##############################################################################
default_args
process_args $@
#debug_args

# check if db is already set up
if [ $(check_database_installed) -eq 0 ]; then
    # ..and if it's been populated already
    if [ $(check_database_populated) -eq 0 ]; then
        upgrade=TRUE
        install=FALSE
    fi
fi

##############################################################################
##############################################################################
# Installation
##############################################################################
##############################################################################
if [ "x$install" = "xTRUE" ]; then

    echo "Performing install..."

    # has mysql root already created user/pass/db?
    if [ $(check_mysql_creds) -ne 0 ]; then
        
        echo " > Creating database with user"
        read -p "   > Enter MySQL root password: " mysqlpass

        echo " > Using command line supplied credentials for account/db creation..."
        echo "   > Username: $dbuser"
        echo "   > Password: *****"
        echo "   > Database: $dbname"

        read -p " > Press <ENTER> to continue..." enter

        mysql -u root -p$mysqlpass -e "CREATE DATABASE $dbname"
        if [ $? -ne 0 ]; then
            echo "Something went wrong creating database '$dbname'"
            exit 1
        fi

        mysql -u root -p$mysqlpass -e "CREATE USER $dbuser@'localhost' IDENTIFIED BY '$dbpass'"
        if [ $? -ne 0 ]; then
            echo "Something went wrong creating user $dbuser@'localhost'"
            exit 1
        fi

        mysql -u root -p$mysqlpass -e "GRANT ALL PRIVILEGES ON $dbname.* TO '$dbuser'@'localhost' WITH GRANT OPTION"
        if [ $? -ne 0 ]; then
            echo "Something went wrong granting privileges"
            exit 1
        fi
    fi

    # use_host and use_port would have already been set by this point
    # because of extensive use of mysql_exec() func call
    mysql -u$dbuser -p$dbpass $dbname $use_host $use_port < $basedir/db.sql
    if [ $? -ne 0 ]; then
        echo "Something went wrong importing database"
        exit 1
    fi

    echo " > Installing dbversion table"
    install_database_version_table
    set_database_version

    echo "Database installation is complete!"
fi

##############################################################################
##############################################################################
# Upgrade
##############################################################################
##############################################################################
if [ "x$upgrade" = "xTRUE" ]; then

# if version is undetected, then output error and exit
# if version is older than last supported, output error and exit
# if version is one of the supported versions...
# loop over from $from_version to $version
# and execute each schema upgrade file

    echo "Performing upgrade..."

    # make sure our mysql credentials are sane
    if [ $(check_mysql_creds) -ne 0 ]; then
        echo "Supplied MySQL credentials appear to be invalid"
        exit 1
    fi

    if [ $(check_database_version_installed) -ne 0 ]; then
        echo "No database version detected. Unable to continue!"
        echo "Upgrade your version of NDOUtils to ${versions[0]} before continuing..."
        exit 1
    fi

    # the lowest version we support is larger than the current dbversion
    # AND the current dbversion is not the same as the lowest version
    from_version=$(get_database_version)
    if version_gt $versions[0] $(get_database_version) && [ "x$(get_database_version)" != "x${versions[0]}" ]; then
        echo "Your dbversion is $from_version, and our minimum supported upgrade version is "
        echo "Upgrade your version of NDOUtils to ${versions[0]} before attempting this upgrade"
        exit 1
    fi

    for ver in "${versions[@]}"; do
        file=$basedir/upgrade-from-$ver.sql
        if [ -f $file ]; then
            echo " > Upgrading from version $ver ($file)"

            # use_host and use_port would have already been set by this point
            # because of extensive use of mysql_exec() func call
            mysql -u$dbuser -p$dbpass $dbname $use_host $use_port < $file
        fi
    done

    echo " > Updating dbversion table"
    set_database_version

    echo "Database upgrade is complete!"
fi
