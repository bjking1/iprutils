# (C) Copyright 2000, 2011
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.

IPR_MAJOR_RELEASE=2
IPR_MINOR_RELEASE=4
IPR_FIX_LEVEL=3
IPR_RELEASE=1
IPR_FIX_DATE=(Sep 2, 2014)

IPR_VERSION_STR=$(IPR_MAJOR_RELEASE).$(IPR_MINOR_RELEASE).$(IPR_FIX_LEVEL) $(IPR_FIX_DATE)

IPR_DEFINES = -DIPR_MAJOR_RELEASE=$(IPR_MAJOR_RELEASE) \
		 -DIPR_MINOR_RELEASE=$(IPR_MINOR_RELEASE) \
		 -DIPR_FIX_LEVEL=$(IPR_FIX_LEVEL) \
		 -DIPR_FIX_DATE='"$(IPR_FIX_DATE)"' \
		 -DIPR_VERSION_STR='"$(IPR_VERSION_STR)"' \
		 -DIPR_RELEASE=$(IPR_RELEASE)
