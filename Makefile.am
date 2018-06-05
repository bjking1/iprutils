# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.
#

noinst_LTLIBRARIES = libipr.la
libipr_la_SOURCES = iprlib.c iprlib.h

sbin_PROGRAMS = iprconfig iprdump iprupdate iprinit iprdbg
dist_sbin_SCRIPTS = iprsos

iprconfig_SOURCES = iprconfig.c iprconfig.h
iprconfig_LDADD= libipr.la $(IPRCONFIG_LIBS)

iprdump_SOURCES = iprdump.c
iprdump_LDADD= libipr.la

iprupdate_SOURCES = iprupdate.c
iprupdate_LDADD= libipr.la

iprinit_SOURCES = iprinit.c
iprinit_LDADD= libipr.la

iprdbg_SOURCES = iprdbg.c
iprdbg_LDADD= libipr.la

if STATIC_BUILD
sbin_PROGRAMS += iprconfig-static

iprconfig_static_SOURCES = iprconfig.c iprconfig.h
iprconfig_static_LDADD= libipr.la $(IPRCONFIG_LIBS)
iprconfig_static_LDFLAGS=-all-static
endif

dist_man_MANS = iprconfig.8 iprdump.8 iprinit.8 iprupdate.8 iprsos.8

dist_noinst_DATA = LICENSE

bashcompdir = ${sysconfdir}/bash_completion.d
dist_bashcomp_DATA = iprconfig-bash-completion.sh

SUBDIRS = . init.d spec

if SYSTEMD
SUBDIRS += systemd
SUBDIRS += udev
endif

if SOSREPORT
sosreportdir = @pythondir@/sos/plugins
sosreport_PYTHON = iprutils.py
endif

if IPRDUMPFMT
sbin_PROGRAMS += iprdumpfmt
iprdumpfmt_SOURCES = iprdumpfmt.c
iprdumpfmt_LDADD= libipr.la
dist_man_MANS += iprdumpfmt.8
endif
