#############################################################################
#  Description:
#
#  Copyright (c) 2025 Nokia
#############################################################################

# Must init the autoconf setup
# The first parameter is project name
# second is version number
# third is bug report address
AC_INIT([multidev], [1.0])

# Store the auxiliary build tools (e.g., install-sh, config.sub, config.guess)
# in this dir (build-aux)
AC_CONFIG_AUX_DIR([build-aux])

LT_INIT([dlopen shared])
#AC_SUBST([LIBTOOL_DEPS])

AC_CONFIG_MACRO_DIRS([m4])

# Safety checks in case user overwritten --srcdir
AC_CONFIG_SRCDIR([rov_config.cc])

# Init automake, and specify this program use relaxed structures.
# i.e. this program doesn't follow the gnu coding standards, and doesn't have
# ChangeLog, COPYING, AUTHORS, INSTALL, README etc. files.
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects -Wno-portability])

# Check for C and C++ compiler
AC_PROG_CC
AC_PROG_CXX

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([FMT], [fmt])

AM_SILENT_RULES([yes])

# Tells automake to create a Makefile
# See https://www.gnu.org/software/automake/manual/html_node/Requirements.html
AC_CONFIG_FILES([ Makefile ])

# AC_SUBST([FMT_CFLAGS])
# AC_SUBST([FMT_LIBS])

# Generate the output
AC_OUTPUT
