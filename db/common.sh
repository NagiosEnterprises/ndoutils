#!/bin/bash

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

default_args
process_args $@
debug_args