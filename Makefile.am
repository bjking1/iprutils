# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.
#

lib_LTLIBRARIES = libipr.la
libipr_la_SOURCES = iprlib.c
include_HEADERS = iprlib.h

sbin_PROGRAMS = iprconfig iprdump iprupdate iprinit iprdbg

iprconfig_SOURCES = iprconfig.c iprconfig.h
iprconfig_LDADD= libipr.la

iprdump_SOURCES = iprdump.c
iprdump_LDADD= libipr.la

iprupdate_SOURCES = iprupdate.c
iprupdate_LDADD= libipr.la

iprinit_SOURCES = iprinit.c
iprinit_LDADD= libipr.la

iprdbg_SOURCES = iprdbg.c
iprdbg_LDADD= libipr.la

dist_man_MANS = iprconfig.8 iprdump.8 iprinit.8 iprupdate.8

SUBDIRS = . systemd init.d spec

