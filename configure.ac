#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dispatch], [0.2], [maxime.hadjinlian@gmail.com])
AM_INIT_AUTOMAKE([dispatch], [0.2])
AC_CONFIG_SRCDIR([src/dispatch.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

TAGLIB_REQUIRED=1.7.2
PKG_CHECK_MODULES(TAGLIB,[taglib >= $TAGLIB_REQUIRED],[have_taglib=yes] , [have_taglib=no])
test "$have_taglib" = "no" && AC_MSG_ERROR('Unable to find taglib. Please make sure library and header files are installed.')
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

PKG_CHECK_MODULES(TAGLIB_C,[taglib_c >= $TAGLIB_REQUIRED],[have_taglib_c=yes] , [have_taglib_c=no])
test "$have_taglib_c" = "no" && AC_MSG_ERROR('Unable to find taglib C bindings. Please make sure library and header files are installed.')
AC_SUBST(TAGLIB_C_CFLAGS)
AC_SUBST(TAGLIB_C_LIBS)

# Check for host type
AC_CANONICAL_HOST
case $host_os in
  darwin* )
        # Do something specific for mac
        AC_SUBST(ARCH_DEFINES, "-D_APPLE")
        ;;
  linux*)
        # Do something specific for linux
        AC_SUBST(ARCH_DEFINES, "-D_LINUX")
        ;;
    *)
        #Default Case
        AC_MSG_ERROR([Your platform is not currently supported])
        ;;
esac

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset realpath strdup strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
