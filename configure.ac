#AC_INIT(configure.in)
AC_INIT(openha, 0.3.6.osvc2)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_HOST

case $host_os in
	*freebsd* | *dragonfly*)
		freebsd=yes
		;;
	*linux*)
		linux=yes
		;;
	*netbsd*)
		case $host in
		*i386*)
			PCIACCESS_LIBS="-li386"
			;;
		*x86_64*|*amd64*)
			PCIACCESS_LIBS="-lx86_64"
			;;
		esac
		netbsd=yes
		;;
	*openbsd*)
		openbsd=yes
		;;
	*solaris*)
		solaris=yes
		PCIACCESS_LIBS="$PCIACCESS_LIBS -ldevinfo"
		;;
	gnu*)
		gnu=yes
		;;
esac

AM_CONDITIONAL(LINUX, [test "x$linux" = xyes])
AM_CONDITIONAL(FREEBSD, [test "x$freebsd" = xyes])
AM_CONDITIONAL(NETBSD, [test "x$netbsd" = xyes])
AM_CONDITIONAL(OPENBSD, [test "x$openbsd" = xyes])
AM_CONDITIONAL(SOLARIS, [test "x$solaris" = xyes])
AM_CONDITIONAL(GNU, [test "x$gnu" = xyes])


AC_ISC_POSIX
CFLAGS=""
dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_STDC
AC_PATH_PROGS(PKGCONFIG, pkg-config)
AC_HEADER_STDC
AC_PREFIX_DEFAULT(/usr/local/cluster)
LIBTOOL="$LIBTOOL --silent"
AC_PROG_INSTALL

dnl Checks for libraries.

if `pkg-config --exists glib-2.0` ; then
	_glib=glib-2.0
else
	_glib=glib-1.2
fi

AC_MSG_CHECKING(glib library flags)
glib_LIBS=`$PKGCONFIG ${_glib} --libs`
AC_MSG_RESULT($glib_LIBS)
AC_SUBST(glib_LIBS)

dnl Checks for header files.
AC_MSG_CHECKING(glib includes flags)
glib_CFLAGS=`$PKGCONFIG ${_glib} --cflags`
AC_MSG_RESULT($glib_CFLAGS)
AC_SUBST(glib_CFLAGS)

AC_MSG_CHECKING(sockio includes flags)
AC_CHECK_HEADERS(sys/sockio.h)
AC_MSG_RESULT($socket_CFLAGS)
AC_SUBST(socket_CFLAGS)


dnl Checks for typedefs, structures, and compiler characteristics.


dnl Checks for library functions.

AC_MSG_CHECKING(socket library function)
AC_CHECK_LIB(socket, recvfrom)
AC_MSG_RESULT($socket_LIBS)
AC_SUBST(socket_LIBS)

AC_MSG_CHECKING(nsl library function)
AC_CHECK_LIB(nsl, gethostbyname)
AC_MSG_RESULT($nsl_LIBS)
AC_SUBST(nsl_LIBS)
dnl Checks for Additional stuffs.

dnl Set PACKAGE SOURCE DIR in config.h.
packagesrcdir=`cd $srcdir && pwd`

dnl Set PACKAGE PREFIX
if test "x${prefix}" = "xNONE"; then
  packageprefix=${ac_default_prefix}
else
  packageprefix=${prefix}
fi

dnl Set PACKAGE DATA & DOC DIR
dnl packagedatadir=share
packagedocdir=doc
packageprocdir=proc
packagesrvdir=services
packageconfdir=conf

dnl Set PACKAGE DIRS in config.h.
packagehelpdir=${packagedatadir}/help
packagemenudir=${packagedatadir}

AC_OUTPUT([
Makefile
src/Makefile
openha.spec
])

