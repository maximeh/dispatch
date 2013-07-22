#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dispatch], [0.2], [maxime.hadjinlian@gmail.com])
AC_CONFIG_SRCDIR([src/dispatch.c])
AM_INIT_AUTOMAKE

msg_pkg_config_missing="'pkg-config' is missing. Make sure it is available in your PATH."
msg_taglib_missing="'taglib' is missing. Make sure the the library and headers are installed and available."
msg_taglib_c_missing="'taglib_c' is missing. Make sure the the library and headers are installed and available."

# Checks for programs.
AC_PROG_CC
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
test "$HAVE_PKG_CONFIG" != "yes" && AC_MSG_ERROR($msg_pkg_config_missing)

TAGLIB_REQUIRED=1.7.2
PKG_CHECK_MODULES(TAGLIB, [taglib >= $TAGLIB_REQUIRED], [have_taglib=yes], [have_taglib=no])
test "$have_taglib" = "no" && AC_MSG_ERROR($msg_taglib_missing)
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

PKG_CHECK_MODULES(TAGLIB_C, [taglib_c >= $TAGLIB_REQUIRED], [have_taglib_c=yes], [have_taglib_c=no])
test "$have_taglib_c" = "no" && AC_MSG_ERROR($msg_taglib_c_missing)
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
AC_CHECK_FUNCS([realpath strdup strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
