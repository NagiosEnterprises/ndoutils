dnl Process this -*-m4-*- file with autoconf to produce a configure script.
dnl
dnl Copyright 2009-2014 Nagios Core Development Team and Community Contributors
dnl Copyright 2005-2009 Ethan Galstad
dnl
dnl This file is part of NDOUtils.
dnl
dnl NDOUtils is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License version 2 as
dnl published by the Free Software Foundation.
dnl
dnl NDOUtils is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with NDOUtils. If not, see <http://www.gnu.org/licenses/>.

dnl Disable caching
define([AC_CACHE_LOAD],)
define([AC_CACHE_SAVE],)

AC_INIT(src/ndo2db.c)
AC_CONFIG_HEADER(include/config.h)
AC_PREFIX_DEFAULT(/usr/local/nagios)

AC_DEFINE([DEFAULT_NAGIOS_USER], [nagios], [Default Nagios User])
AC_DEFINE([DEFAULT_NAGIOS_GROUP], [nagios], [Default Nagios Group])

PKG_NAME=ndoutils
INIT_PROG=ndo2db
PKG_VERSION="2.1"
PKG_HOME_URL="http://www.nagios.org/"
PKG_REL_DATE="07-28-2016"

dnl Figure out how to invoke "install" and what install options to use.
AC_PROG_INSTALL
AC_SUBST(INSTALL)

dnl Figure out how to invoke "perl"
AC_PATH_PROG(PERL,perl)

dnl What OS are we running?
AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

dnl Nagios O/S, Distribution, Startup, Paths, Files macros
AC_NAGIOS_GET_OS
AC_NAGIOS_GET_DISTRIB_TYPE
AC_NAGIOS_GET_INIT
AC_NAGIOS_GET_INETD
AC_NAGIOS_GET_PATHS
AC_NAGIOS_GET_FILES

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(arpa/inet.h ctype.h dirent.h dlfcn.h errno.h fcntl.h float.h getopt.h grp.h inttypes.h limits.h ltdl.h math.h netdb.h netinet/in.h pthread.h pwd.h regex.h signal.h socket.h stdarg.h stdint.h string.h strings.h sys/ipc.h sys/mman.h sys/msg.h sys/poll.h sys/resource.h sys/sendfile.h sys/socket.h sys/stat.h sys/time.h sys/timeb.h sys/types.h sys/un.h sys/wait.h syslog.h tcpd.h unistd.h values.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_TYPE_GETGROUPS

dnl Check lengths for later tests of u_int32_t and int32_t
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(long)

dnl Define u_int32_t if we don't have it already (Solaris, etc.)
AC_CHECK_TYPE(uint32_t,unsigned int)
AC_CHECK_TYPE(u_int32_t,unsigned int)
if test "$ac_cv_type_u_int32_t" = no ; then
	if test "$ac_cv_type_u_int32_t" = yes ; then
		AC_DEFINE(U_INT32_T_IS_UINT32_T)
	else
		if test "$ac_cv_sizeof_int" = 4 ; then
			AC_DEFINE(U_INT32_T_IS_UINT)
		else
			if test "$ac_cv_sizeof_long" = 4 ; then
				AC_DEFINE(U_INT32_T_IS_ULONG)
			else
				if test "$ac_cv_sizeof_short" = 4 ; then
					AC_DEFINE(U_INT32_T_IS_USHORT)
				fi
			fi
		fi
	fi
fi

dnl Define int32_t if we don't have it already
AC_CHECK_TYPE(int32_t,int)
if test "$ac_cv_type_int32_t" = no ; then
	if test "$ac_cv_sizeof_int" = 4 ; then
		AC_DEFINE(INT32_T_IS_UINT)
	else
		if test "$ac_cv_sizeof_long" = 4 ; then
			AC_DEFINE(INT32_T_IS_ULONG)
		else
			if test "$ac_cv_sizeof_short" = 4 ; then
				AC_DEFINE(INT32_T_IS_USHORT)
			fi
		fi
	fi
fi


dnl Check for getopt_long (Solaris)
# AC_CHECK_FUNCS([getopt_long],,AC_CHECK_LIB([iberty],[getopt_long],OTHERLIBS="$OTHERLIBS -liberty"))
# AC_SUBST(OTHERLIBS)

dnl Checks for library functions.
AC_CHECK_LIB(nsl,main,SOCKETLIBS="$SOCKETLIBS -lnsl")
AC_CHECK_LIB(socket,socket,SOCKETLIBS="$SOCKETLIBS -lsocket")
AC_SUBST(SOCKETLIBS)
AC_CHECK_LIB(wrap,main,[
	LIBWRAPLIBS="$LIBWRAPLIBS -lwrap"
	AC_DEFINE(HAVE_LIBWRAP)
	])
AC_SUBST(LIBWRAPLIBS)
AC_CHECK_FUNCS(getopt_long strdup strstr strtoul initgroups strtof nanosleep)

dnl Check for asprintf() and friends...
AC_CACHE_CHECK([for va_copy],ac_cv_HAVE_VA_COPY,[
AC_TRY_LINK([#include <stdarg.h>
va_list ap1,ap2;], [va_copy(ap1,ap2);],
ac_cv_HAVE_VA_COPY=yes,
ac_cv_HAVE_VA_COPY=no)])
if test x"$ac_cv_HAVE_VA_COPY" = x"yes"; then
	AC_DEFINE(HAVE_VA_COPY,1,[Whether va_copy() is available])
else
	AC_CACHE_CHECK([for __va_copy],ac_cv_HAVE___VA_COPY,[
	AC_TRY_LINK([#include <stdarg.h>
	va_list ap1,ap2;], [__va_copy(ap1,ap2);],
	ac_cv_HAVE___VA_COPY=yes,
	ac_cv_HAVE___VA_COPY=no)])
	if test x"$ac_cv_HAVE___VA_COPY" = x"yes"; then
		AC_DEFINE(HAVE___VA_COPY,1,[Whether __va_copy() is available])
	fi
fi
AC_SUBST(SNPRINTF_O)

AC_CHECK_FUNC(snprintf,,SNPRINTF_O=./snprintf.o)
AC_CHECK_FUNC(asprintf,,SNPRINTF_O=./snprintf.o)

AC_MSG_CHECKING(for type of socket size)
AC_TRY_COMPILE([#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
],
[int a = send(1, (const void *)0, (size_t *) 0, (int *) 0);],
[AC_DEFINE(SOCKET_SIZE_TYPE, size_t) AC_MSG_RESULT(size_t)],
[AC_DEFINE(SOCKET_SIZE_TYPE, int) AC_MSG_RESULT(int)])

dnl Stolen from Python code: loewis@users.sourceforge.net
dnl AC_CHECK_TYPE(socklen_t,int,
dnl	AC_DEFINE(socklen_t,int,
dnl	Define to `int' if <sys/socket.h> does not define.),[
dnl	#ifdef HAVE_SYS_TYPES_H
dnl	#include <sys/types.h>
dnl	#endif
dnl	#ifdef HAVE_SYS_SOCKET_H
dnl	#include <sys/socket.h>
dnl	#endif
dnl	])

dnl Check if SUN_LEN exists
AC_TRY_LINK([
    #include<sys/types.h>
    #include<sys/un.h>], 
    [struct sockaddr_un su; int i = SUN_LEN(&su);],
    [AC_DEFINE(HAVE_SUN_LEN)], )

dnl Check for location of init scripts
init_dir=/etc/rc.d/init.d
if test -d /etc/rc.d/init.d; then
    init_dir="/etc/rc.d/init.d"
elif test -d /usr/local/etc/rc.d; then
    init_dir="/usr/local/etc/rc.d"
elif test -d /etc/rc.d; then
    init_dir="/etc/rc.d"
elif test -d /etc/init.d; then
    init_dir="/etc/init.d"
elif test -d /sbin/init.d; then
    init_dir="/sbin/init.d"
fi

AC_ARG_WITH([ndo2db_port],
	AS_HELP_STRING([--with-ndo2db-port=<port>],
		[sets port number for NDO2DB to listen on]),
	[ndo2db_port=$withval],
	[ndo2db_port=5668])
AC_DEFINE_UNQUOTED(DEFAULT_SERVER_PORT,$ndo2db_port,[Default port for NDO2DB daemon])
AC_SUBST(ndo2db_port)

dnl - Modified version from www.erlang.org
dnl - Some 12/15/05 mods made after reading http://xaxxon.slackworks.com/phuku/dl.html
AC_MSG_CHECKING(for linker flags for loadable modules)
case $host_os in
	solaris2*|sysv4*)
		MOD_LDFLAGS="-G"
	;;
	aix4*|aix5*)
		#MOD_LDFLAGS="-G -bnoentry -bexpall"
		MOD_LDFLAGS="-G -bM:SRE -bnoentry -bexpall"
	;;
	freebsd2*)
		# Non-ELF GNU linker
		MOD_LDFLAGS="-Bshareable"
	;;
	darwin*)
		# Mach-O linker, a shared lib and a loadable
		# object file is not the same thing.
		MOD_LDFLAGS="-bundle -flat_namespace -undefined suppress"
		MOD_CFLAGS="$MOD_CFLAGS -fno-common"
	;;
	linux* | k*bsd*-gnu*)
		# assume GNU linker and ELF
		MOD_LDFLAGS="-shared"
		MOD_CFLAGS="-fPIC"
	;;
	*)
		# assume GNU linker and ELF
		MOD_LDFLAGS="-shared"
	;;
esac

if test "$dist_type" = aix; then
	MOD_LDFLAGS="$MOD_LDFLAGS -Wl,-berok"
fi

AC_MSG_RESULT([$MOD_LDFLAGS])
AC_SUBST(MOD_CFLAGS)
AC_SUBST(MOD_LDFLAGS)


dnl DB support options
USE_MYSQL=yes
AC_ARG_ENABLE(mysql,AC_HELP_STRING([--enable-mysql],[enables MySQL database support]))
AS_IF([test x$enable_mysql = xno], [USE_MYSQL=no])


dnl Are we using any MySQL routines?
if test $USE_MYSQL = yes; then
	np_mysqlclient
	if test $with_mysql = "no" ; then
		echo ""
		echo ""
		echo "*** MySQL library could not be located... **************************"
		echo ""
		echo "You chose to compile NDOutils with MySQL support, but I was unable to"
		echo "locate the MySQL library on your system.  If the library is"
		echo "installed,  use the --with-mysql-lib argument to specify the"
		echo "location of the MySQL library."
		echo "installed, use the --with-mysql=DIR argument to specify the"
		echo "location of the MySQL library, We assume mysql_config is in DIR/dir"
		echo "NOTE: After you install the necessary libraries on your system:"
		echo "      1. Make sure /etc/ld.so.conf has an entry for the directory in"
		echo "         which the MySQL libraries are installed."
		echo "      2. Run 'ldconfig' to update the run-time linker options."
		echo "      3. Run 'make devclean' in the NDBXT distribution to clean out"
		echo "         any old references to your previous compile."
		echo "      4. Rerun the configure script."
		echo ""
		echo "TIP: Try the following...."
		echo "      ./configure --with-mysql=/usr/lib/mysql"
		echo ""
		echo "********************************************************************"
		echo ""
		echo ""

		USE_MYSQL=no
	else
		echo "MySQL library and include file(s) were found!"
		AC_DEFINE_UNQUOTED(USE_MYSQL)
		DBLIBS="$DBLIBS $np_mysql_libs"
		CFLAGS="$CFLAGS $np_mysql_include"
	fi
fi

save_LDFLAGS="$LDFLAGS"
LDFLAGS="${LDFLAGS} ${DBLDFLAGS}"
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="${DBCFLAGS}"
save_CFLAGS="$CFLAGS"
CFLAGS="${DBCFLAGS}"


AC_SUBST(DBCFLAGS)
AC_SUBST(DBLDFLAGS)
AC_SUBST(DBLIBS)

LDFLAGS="$save_LDFLAGS"
CPPFLAGS="$save_CPPFLAGS"
CFLAGS="$save_CFLAGS"


dnl Moving this before the Mysql detection options breaks them....
AC_ARG_WITH(ndo2db_user,AC_HELP_STRING([--with-ndo2db-user=<user>],[sets user name to run NDO2DB]),ndo2db_user=$withval,ndo2db_user=nagios)
AC_ARG_WITH(ndo2db_group,AC_HELP_STRING([--with-ndo2db-group=<group>],[sets group name to run NDO2DB]),ndo2db_group=$withval,ndo2db_group=nagios)
AC_SUBST(ndo2db_user)
AC_SUBST(ndo2db_group)
INSTALL_OPTS="-o $ndo2db_user -g $ndo2db_group"
AC_SUBST(INSTALL_OPTS)


dnl Does the user want to check for systemd?
AC_ARG_ENABLE([systemd-sockets],
	AC_HELP_STRING([--enable-systemd-sockets],[enables systemd socket activation]),
	[],[enable_systemd_sockets=auto])

dnl Try to compile and link a program that checks for systemd sockets.
if test x$enable_systemd_sockets != xno; then
	have_systemd=no

	AC_SEARCH_LIBS([sd_listen_fds],[systemd systemd-daemon],
		AC_TRY_LINK([#include <systemd/sd-daemon.h>],[sd_listen_fds(0);],[have_systemd=yes]))

	if test x$have_systemd = xyes; then
		AC_DEFINE([HAVE_SYSTEMD])
	elif test x$enable_systemd_sockets = xyes; then
		AC_MSG_ERROR([systemd was requested, but could not be found])
	fi
fi


dnl Does user want to check for SSL?
AC_ARG_ENABLE(ssl,AC_HELP_STRING([--enable-ssl],[enables native SSL support]),[
	if test x$enableval = xyes; then
		check_for_ssl=yes
	else
		check_for_ssl=no
	fi
	],check_for_ssl=no)

dnl Optional SSL library and include paths
if test x$check_for_ssl = xyes; then
	# need_dh should only be set for NRPE and NDOUTILS
	need_dh=yes
	AC_NAGIOS_GET_SSL
fi


if test x$HAVE_NANOSLEEP = x; then
	USE_NANOSLEEP=no
else
	USE_NANOSLEEP=yes
	AC_ARG_ENABLE(nanosleep,AC_HELP_STRING([--enable-nanosleep],[enables use of nanosleep (instead of sleep) in event timing]))
	AS_IF([test x$enable_nanosleep = xno],[USE_NANOSLEEP=no],[USE_NANOSLEEP=yes])
	if test x$USE_NANOSLEEP = xyes ; then
		AC_DEFINE_UNQUOTED(USE_NANOSLEEP)
	fi
fi

AC_CONFIG_FILES([Makefile
	src/Makefile
	config/ndo2db.cfg-sample
	config/ndomod.cfg-sample
	config/nagios.cfg
	config/misccommands.cfg
	docs/docbook/en-en/Makefile
	include/io.h
	include/common.h
	startup/bsd-init
	startup/debian-init
	startup/default-init
	startup/default-inetd
	startup/default-service
	startup/default-socket
	startup/default-socket-svc
	startup/default-xinetd
	startup/mac-init.plist
	startup/mac-inetd.plist
	startup/newbsd-init
	startup/openbsd-init
	startup/openrc-conf
	startup/openrc-init
	startup/solaris-init.xml
	startup/solaris-inetd.xml
	startup/tmpfile.conf
	startup/upstart-init
	startup/rh-upstart-init
])

AC_OUTPUT()


dnl Review options
echo ""
echo ""
AC_MSG_RESULT([*** Configuration summary for $PKG_NAME $PKG_VERSION $PKG_REL_DATE ***:])

echo ""
echo " General Options:"
echo " -------------------------"

AC_MSG_RESULT([ NDO2DB user:      $ndo2db_user])
AC_MSG_RESULT([ NDO2DB group:     $ndo2db_group])
AC_MSG_RESULT([ NDO2DB tcp port:  $ndo2db_port])


echo "";\
echo "";\
echo "Review the options above for accuracy.  If they look";\
echo "okay, type 'make all' to compile the NDO utilities,";\
echo "or type 'make' to get a list of make options.";\
echo ""
