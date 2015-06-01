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

nodist_pkgdata_DATA = iprdump iprinit iprupdate
dist_pkgdata_DATA = iprdump.in iprinit.in iprupdate.in

clean-local:
	rm iprdump iprinit iprupdate

sysconfdir=@sysconfdir@/ha.d/resource.d
nodist_sysconf_DATA = iprha
dist_sysconf_DATA = iprha.in
