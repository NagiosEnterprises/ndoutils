#!/bin/bash

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
    H | \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
  esac
done
shift $((OPTIND -1))
