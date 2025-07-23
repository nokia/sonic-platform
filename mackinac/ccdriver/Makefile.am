#############################################################################
# Description:
#
# Copyright (c) 2025 Nokia
#############################################################################

EXTRA_DIST =
BUILT_SOURCES =
lib_LTLIBRARIES =
noinst_LTLIBRARIES = libyanked.la
sbin_PROGRAMS = asic_rov_config sets_setup pcon_cmds
bin_SCRIPTS =
dist_bin_SCRIPTS =
sysconf_DATA =

asic_rov_config_SOURCES = \
$(srcdir)/rov_config.cc
asic_rov_config_LDADD = libyanked.la

sets_setup_SOURCES = \
$(srcdir)/sets_setup.cc
sets_setup_LDADD = libyanked.la

pcon_cmds_SOURCES = \
$(srcdir)/pcon.cc
pcon_cmds_LDADD = libyanked.la

libyanked_la_SOURCES = \
$(srcdir)/conf_file.cc \
$(srcdir)/replacements.cc \
$(srcdir)/hwPcon.cc \
$(srcdir)/fpgaImageUtils.cc \
$(srcdir)/idt8a3xxxx.cc \
$(srcdir)/idtfw_4_9_7.cc
libyanked_la_LIBADD = $(FMT_LIBS)


ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
-I/usr/include $(blowFpga_DEFINES) \
-g $(FMT_CFLAGS)

AM_CXXFLAGS = -std=c++20
AM_LDFLAGS = -static

#libtool: $(LIBTOOL_DEPS)
# $(SHELL) ./config.status libtool