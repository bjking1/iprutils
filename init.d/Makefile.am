# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.
#

%: %.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' < $< > $@

if INITD
nodist_initd_SCRIPTS = iprdump iprinit iprupdate
endif

clean-local:
	-rm iprdump iprinit iprupdate iprha

sysconfdir=@sysconfdir@/ha.d/resource.d
nodist_sysconf_DATA = iprha

EXTRA_DIST = iprdump.in iprinit.in iprupdate.in iprha.in
