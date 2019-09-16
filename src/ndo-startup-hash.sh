#!/bin/bash

# this script is used on ndo startup to determine if we need to write
# the objects or not. currently on startup, all object definitions are truncated
# and recreated (except for the nagios_objects table)

cfgdir=/usr/local/nagios/etc/
hashfile=/usr/local/nagios/var/ndo.hash

# check if hashfile exists and attempt to create a new one if not
if [ ! -f $hashfile ]; then
    if ! touch $hashfile >/dev/null 2>&1; then
        echo "Invalid permissions, can't create $hashfile"
        exit 2
    fi
fi

# calculate the newest hash
# https://stackoverflow.com/questions/545387/linux-compute-a-single-hash-for-a-given-folder-contents
newhash=$((find $cfgdir -type f -print0  | sort -z | xargs -0 sha1sum; find $cfgdir \( -type f -o -type d \) -print0 | sort -z | xargs -0 stat -c '%n %a') | sha1sum | awk '{print $1}')

# get the contents of the old hash
oldhash=$(cat $hashfile)

# update the hashfile
echo $newhash > $hashfile

# check if the hashes match
if [ "x$newhash" = "x$oldhash" ]; then
    exit 0
fi

# if we got this far, it means the hashes dont match
exit 1
