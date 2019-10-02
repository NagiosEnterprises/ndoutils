define([AC_CACHE_LOAD],)
define([AC_CACHE_SAVE],)


AC_PREREQ([2.69])



PKG_NAME="NDOUtils"
PKG_VERSION="3.0.0"
PKG_HOME_URL="http://www.nagios.org/"
PKG_REL_DATE="2019"
AC_INIT([NDOUtils], [3.0.0], [devteam@nagios.com])


AC_CONFIG_SRCDIR([src/ndo.c])
AC_CONFIG_HEADERS([include/config.h])


AC_PROG_CC


AC_PROG_INSTALL
AC_SUBST(INSTALL)


AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([string.h])

AC_CHECK_HEADERS([mysql/mysql.h], [],  [AC_MSG_ERROR([Can't locate mysql headers])])
AC_CHECK_HEADERS([mysql/errmsg.h], [], [AC_MSG_ERROR([Can't locate mysql headers])])

AC_FUNC_MALLOC


# Nagios user/group
AC_ARG_WITH(
    
    user,
    AC_HELP_STRING(
        [--with-user=<user>],
        [Sets user for shared object and config permissions]
    ),
    user=$withval,
    user=nagios
)
AC_ARG_WITH(
    
    group,
    AC_HELP_STRING(
        [--with-group=<group>],
        [Sets group for shared object and config permissions]
    ),
    group=$withval,
    group=nagios
)

INSTALL_OPTS="-o $user -g $group"
AC_SUBST(INSTALL_OPTS)


# Directory to place the compiled binary
AC_ARG_WITH(
    
    bin_dir,
    AC_HELP_STRING(
        [--with-bin-dir=</path/to/bin>],
        [Directory to place the compiled binary]
    ),
    bin_dir=$withval,
    bin_dir=/usr/local/nagios/bin/
)

BINDIR="$bin_dir"
AC_SUBST(BINDIR)


# Directory to place the configuration file
AC_ARG_WITH(
    
    cfg_dir,
    AC_HELP_STRING(
        [--with-cfg-dir=</path/to/cfg>],
        [Directory to place the configuration file]
    ),
    cfg_dir=$withval,
    cfg_dir=/usr/local/nagios/etc/
)

CFGDIR="$cfg_dir"
AC_SUBST(CFGDIR)


# Nagios configuration file
AC_ARG_WITH(

    nagios_config,
    AC_HELP_STRING(
        [--with-nagios-config=<nagios.cfg>],
        [Identify the Nagios main configuration file]
    ),
    nagios_config=$withval,
    nagios_config=/usr/local/nagios/etc/nagios.cfg
)

if ! test -f $nagios_config; then
    nagios_config="$cfg_dir/nagios.cfg"
    if ! test -f $nagios_config; then
        nagios_config="$bin_dir/nagios.cfg"
        if ! test -f $nagios_config; then
            nagios_config=""
            AC_MSG_ERROR([Unable to find Nagios config file. Use --with-nagios-config])
        fi
    fi
fi

NAGIOSCFG="$nagios_config"
AC_SUBST(NAGIOSCFG)


# Enable testing
AC_ARG_ENABLE(

    testing,
    AC_HELP_STRING(
        [--enable-testing],
        [Create the 'make test' target (requires subunit and libcheck)]
    ),
    [CFLAGS="${CFLAGS} -O0 -g3 -ggdb3 -fprofile-arcs -ftest-coverage"]
)


# Check dependencies for enable-testing
if test "x$enable_testing" != "x" ; then

    AC_CHECK_HEADERS([check.h], [], [AC_MSG_ERROR(Can't locate libcheck headers)])
    AC_CHECK_LIB(subunit, subunit_test_start)
    AC_SUBST(tests_enabled, [enabled])

    AC_DEFINE_UNQUOTED(DEBUG)
else
    AC_SUBST(tests_enabled, [disabled])
fi


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


echo ""
echo ""

AC_MSG_RESULT([*** Configuration summary for $PKG_NAME $PKG_VERSION $PKG_REL_DATE ***:])

echo ""
echo "----------------"

AC_MSG_RESULT([ User:              $user])
AC_MSG_RESULT([ Group:             $group])
AC_MSG_RESULT([ Bin directory:     $bin_dir])
AC_MSG_RESULT([ Config directory:  $cfg_dir])
AC_MSG_RESULT([ Nagios config:     $nagios_config])

echo "----------------"
echo ""

echo "Review the options above for accuracy.  If they look okay,"
echo "type 'make all' to compile NDOUtils, or type 'make' to get"
echo "a list of make options."
echo ""