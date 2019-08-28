define([AC_CACHE_LOAD],)
define([AC_CACHE_SAVE],)


AC_PREREQ([2.69])


AC_INIT([NDOUtils], [3.0.0], [devteam@nagios.com])


AC_CONFIG_SRCDIR([src/ndo.c])
AC_CONFIG_HEADERS([include/config.h])


AC_PROG_CC


AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([string.h])

AC_CHECK_HEADERS([mysql/mysql.h], [],  [AC_MSG_ERROR([Can't locate mysql headers])])
AC_CHECK_HEADERS([mysql/errmsg.h], [], [AC_MSG_ERROR([Can't locate mysql headers])])

AC_CHECK_HEADERS([check.h], [], [AC_MSG_ERROR(Can't locate libcheck headers)])


AC_FUNC_MALLOC


# Enable debugging
# ./configure --enable-debugging
AC_ARG_ENABLE(

    debugging,
    AC_HELP_STRING(
        [--enable-debugging],
        [Pass debugging flags to the compiler]
    ),
    [CFLAGS="${CFLAGS} -O0 -g3 -ggdb3"]
)


# Enable coverage reporting
# ./configure --enable-coverage
AC_ARG_ENABLE(

    coverage,
    AC_HELP_STRING(
        [--enable-coverage],
        [Pass coverage flags to the compiler]
    ),
    [CFLAGS="${CFLAGS} -O0 -g3 -ggdb3 -fprofile-arcs -ftest-coverage"]
)


# Determine MySQL C flags and libraries
# This also kind of determines if the mysql library is installed
if which mysql_config; then
    DBLIBS=`mysql_config --libs`
    DBFLAGS=`mysql_config --cflags`
else
    AC_MSG_ERROR([Unable to find mysql_config binary])
fi


AC_SUBST(DBLIBS)
AC_SUBST(DBFLAGS)


AC_OUTPUT([

    Makefile
    src/Makefile
    test/Makefile

])
