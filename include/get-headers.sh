#!/bin/bash

if [ x$1 = x ]; then
    echo "First parameter must be directory of nagioscore includes (e.g.: /path/to/nagioscore/include)"
    exit 1
fi

includedir=$1

if [ ! -f $includedir/nagios.h ]; then
    echo "Looks like the path you've specified isn't a valid include directory"
    exit 1
fi

if [ x$2 = x ]; then
    echo "Second parameter must be destination directory (e.g.: /path/to/ndoutils/include/nagios)"
    exit 1
fi

destdir=$2

mkdir -p $destdir/lib

cp $includedir/*.h $destdir/
cp $includedir/../lib/*.h $destdir/lib/*.h

rm $destdir/config.h
rm $destdir/locations.h

rm $destdir/lib/iobroker.h
rm $destdir/lib/snprintf.h
