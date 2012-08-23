#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([webstatscollector], [0.2], [dvanliere@wikimedia.org])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([export.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = yes)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Need -ldb and -lpthread
# NOTE: This is probably not the correct way to set CFLAGS, 
# but Diederik and Otto had a hard time getting AM_CFLAGS 
# to work correctly.
CFLAGS="-ldb -lpthread -pedantic -Wall"

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldb':
AC_CHECK_LIB([db], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
WEB_LIBS="-ldb -lpthread"
AC_SUBST(WEB_LIBS)

# Checks for header files.

AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([alarm bzero socket strerror strstr])

#PKG_CHECK_MODULES(libdb, [libdb4.6-dev >= 4.6])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
