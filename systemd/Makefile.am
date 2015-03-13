# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.
#
#

%.service: %.service.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' < $< > $@

nodist_pkgdata_DATA = iprdump.service \
	iprinit.service iprupdate.service
dist_pkgdata_DATA = iprdump.service.in \
	iprinit.service.in iprupdate.service.in

clean-local:
	rm iprdump.service iprinit.service iprupdate.service
