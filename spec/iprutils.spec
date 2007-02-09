Summary: Utilities for the IBM Power Linux RAID adapters
Name: iprutils
Version: 2.2.4.cvs
Release: 1
License: CPL
Group: System Environment/Base
Vendor: IBM
URL: http://sourceforge.net/projects/iprdd/
Source0: iprutils-%{version}-src.tgz
BuildRoot: %{_tmppath}/%{name}-root

%description
Provides a suite of utilities to manage and configure SCSI devices
supported by the ipr SCSI storage device driver.

%prep
%setup -q -n %{name}

%build
make

%install
make INSTALL_MOD_PATH=$RPM_BUILD_ROOT install
install -d $RPM_BUILD_ROOT/%{_sysconfdir}/init.d
install -m 755 init.d/iprinit $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprinit
install -m 755 init.d/iprdump $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprdump
install -m 755 init.d/iprupdate $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprupdate

%ifarch ppc ppc64
%post
/usr/lib/lsb/install_initd %{_sysconfdir}/init.d/iprinit
/usr/lib/lsb/install_initd %{_sysconfdir}/init.d/iprdump
/usr/lib/lsb/install_initd %{_sysconfdir}/init.d/iprupdate
%endif

%ifarch ppc ppc64
%preun
if [ $1 = 0 ]; then
	%{_sysconfdir}/init.d/iprdump stop  > /dev/null 2>&1
	%{_sysconfdir}/init.d/iprupdate stop  > /dev/null 2>&1
	%{_sysconfdir}/init.d/iprinit stop  > /dev/null 2>&1
	/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprdump
	/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprupdate
	/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprinit
fi
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc README LICENSE
/sbin/*
%{_mandir}/man*/*
%{_sysconfdir}/init.d/*

%changelog
* Fri Feb 9 2007 Brian King <brking@us.ibm.com>
- Add filename date to microcode download screen. (Ryan Hong)
- Fix to prevent unnecessarily writing sysfs attributes. (Ryan Hong)
* Mon Feb 5 2007 Brian King <brking@us.ibm.com>
- Fix incorrect memory free in analyze log menus.
* Wed Jan 10 2007 Brian King <brking@us.ibm.com>
- Fix send diagnostics buffer transfer length to be only what was
  received in the receive diagnostics. Fixes disk hotplug on
  some SAS disk enclosures.
* Thu Jan 4 2007 Brian King <brking@us.ibm.com>
- Sourceforge patch 1627673: iprutils fix to buffer overflow
- Add checking to iprconfig command "set-qdepth" input queue
  depth value. If the given value is larger than 255, fail the command.
- Fix a bug in which the iprconfig command "raid-create"
  may create an illegal queue depth value to the ipr
  config file.
* Thu Jan 4 2007 Brian King <brking@us.ibm.com>
- Sourceforge patch 1627672: iprutils fix to buffer overflow
- Fix a bug in which the iprconfig command "raid-create"
  may create an illegal queue depth value to the ipr
  config file.
* Wed Nov 29 2006 Brian King <brking@us.ibm.com>
- Fixes for SES microcode download on SAS.
* Mon Nov 20 2006 Brian King <brking@us.ibm.com>
- Change to handle UA responses in the JBOD iprinit sequence.
* Thu Nov 16 2006 Brian King <brking@us.ibm.com>
- Fix segfault in iprconfig if /var/log does not exist.
- Allow IOA microcode download to a secondary IOA.
- Fix to prevent errors during SAS SES microcode download.
* Tue Oct 10 2006 Brian King <brking@us.ibm.com>
- Add a couple utility functions for SAS
	ipr_query_sas_expander_info
	ipr_query_res_redundancy_info
* Wed Sep 27 2006 Brian King <brking@us.ibm.com> 2.2.3
- Fix SAS disk hotplug dual path bug.
* Tue Sep 12 2006 Brian King <brking@us.ibm.com>
- Change default QERR setting for SAS to 0.
* Tue Sep 12 2006 Brian King <brking@us.ibm.com> 2.2.2
- Fix iprconfig set-format-timeout.
* Mon Sep 11 2006 Brian King <brking@us.ibm.com> 2.2.1
* Fri Sep 8 2006 Brian King <brking@us.ibm.com>
- Reduce default JBOD queue depth to 3.
- Fix iprconfig -c set-bus-speed.
* Wed Aug 23 2006 Brian King <brking@us.ibm.com>
- Fix a race condition with hotplug events which could
  cause the ipr daemons to run before newly added devices
  are completed added to the system.
* Wed Aug 9 2006 Brian King <brking@us.ibm.com>
- Fix a segfault in iprdbg when using the macro function
* Tue Jul 25 2006 Brian King <brking@us.ibm.com> 2.2.0
- Fix for command line SES microcode update.
* Mon May 8 2006 Brian King <brking@us.ibm.com>
- Use IOA's default format timeout for AF DASD instead
  of using a hard coded default.
- Remove RAID support for some older drives that should never
  have been supported.
* Mon May 1 2006 Brian King <brking@us.ibm.com>
- Add support to iprinit for it to handle disks going
  from JBOD <-> AF format across an adapter reset. When this
  is detected, iprinit will now attempt to delete the disk
  and then rescan that slot.
- Fixed an ncurses screen drawing bug which resulted in the
  screen getting paged down if the cursor was on the last item
  on the screen and 't' was pressed to toggle the display.
- Added disk concurrent maintenance support for handling
  dual pathed SAS disks.
* Fri Mar 17 2006 Brian King <brking@us.ibm.com>
- Improve iprdbg's logging
* Thu Mar 16 2006 Brian King <brking@us.ibm.com>
- Print better status for devices when IOA is offline/dead.
* Tue Mar 14 2006 Brian King <brking@us.ibm.com>
- Fix to allow for compiling with libsysfs 2.0.0
* Tue Mar 14 2006 Brian King <brking@us.ibm.com> 2.1.4
- Concurrent maintenance fix for certain iSeries
  enclosures which would result in non existent
  drive slots being displayed in iprconfig.
* Wed Mar 8 2006 Brian King <brking@us.ibm.com>
- Remove some redundant code in disk hotplug path
* Thu Mar 2 2006 Brian King <brking@us.ibm.com>
- Fixup status of RAID 10 arrays to print a better status
  under multiple failure scenarios.
* Thu Feb 24 2006 Brian King <brking@us.ibm.com> 2.1.3
- Prevent duplicate mode sense commands from being issued.
- More uevent handling improvements.
- Automatically create hotplug directory if it doesn't
  already exist so adapter microcode update works.
* Thu Feb 9 2006 Brian King <brking@us.ibm.com>
- Improve robustness of uevents failure handling. Fall
  back to polling method if needed. 
* Fri Feb 3 2006 Brian King <brking@us.ibm.com>
- Auxiliary cache adapter fixes.
* Fri Jan 27 2006 Brian King <brking@us.ibm.com>
- Fix iprconfig -c update-ucode to properly report an
  error if the wrong microcode level is specified.
* Mon Jan 23 2006 Brian King <brking@us.ibm.com>
- Fixed a compiler issue.
* Fri Jan 20 2006 Brian King <brking@us.ibm.com>
- Fixed a bug in iprconfig query-raid-create that prevented
  JBOD candidates from being displayed if there were no
  AF candidates as well.
* Thu Jan 5 2006 Brian King <brking@us.ibm.com> 2.1.2
- Make iprupdate return success/failure indication
  when invoked with --force.
* Tue Jan 3 2006 Brian King <brking@us.ibm.com>
- Concurrent maintenance fix for 7031-D24/T24.
* Tue Dec 20 2005 Brian King <brking@us.ibm.com> 2.1.1
- Fix compile error in iprconfig
* Thu Dec 18 2005 Brian King <brking@us.ibm.com> 2.1.0
- Updates for aux cache IOAs
- Updates for SAS adapters
- Misc fixes for new iprconfig command line options
* Wed Dec 7 2005 Brian King <brking@us.ibm.com>
- Add command line options to iprconfig to perform virtually
  every iprconfig function available in the ncurses interface.
* Tue Nov 15 2005 Brian King <brking@us.ibm.com> 2.0.15.6
- Fix concurrent maintenance with disk drawers reporting
  multiple SES devices on the same SCSI bus.
* Fri Oct 7 2005 Anton Blanchard <anton@samba.org>
- Fix string length calculation in ipr_get_hotplug_dir
* Wed Aug 17 2005 Brian King <brking@us.ibm.com> 2.0.15.4
- Fix a couple of uninitialized variable compile errors
* Wed Jul 27 2005 Brian King <brking@us.ibm.com> 2.0.15.3
- Fix: iprconfig: IOA microcode update would leave AF DASD
  (disks that are in disk arrays) in a state where they were
  no longer tagged queueing. Fix iprconfig to run iprinit on the
  adapter after a microcode download to ensure all attached devices
  are properly setup after a microcode download.
- Fix iprinit: If an IOA was reset for some reason at runtime,
  this would cause AF DASD devices to get tagged queueing turned
  off and it would never get turned back on. Change iprinit to
  detect this and turn tagged queueing back on if this happens.
- Changing the queue depth for a disk array was broken. Fix iprinit
  to properly restore the queue depth from the ipr configuration file.
- Fix iprconfig to handle disk format failures better
- Fix potential iprutils segfaults when iterating over disk arrays
* Wed Jun 1 2005 Brian King <brking@us.ibm.com> 2.0.15.1
- Fix iprconfig Analyze Log options
* Wed May 18 2005 Brian King <brking@us.ibm.com> 2.0.15
- Clarify format options
- Setup mode page 0 for IBM drives to ensure command aging is
  enabled. This ensures commands are not starved on some drives.
- Fix so that iprdump properly names dump files once 100 dumps
  have been made.
- Make iprconfig handle failures of scsi disk formats better
- Fix iprconfig Set root kernel message log directory menu
- Properly display RAID level on all iprconfig screens
- Don't disable init.d daemons on an rpm -U
* Tue Apr 12 2005 Brian King <brking@us.ibm.com> 2.0.14.2
- Fixed bug preventing disk microcode update from working.
* Mon Apr 4 2005 Brian King <brking@us.ibm.com>
- Add ability to force RAID consistency check
* Mon Mar 25 2005 Brian King <brking@us.ibm.com> 2.0.14.1
- Removed mention of primary/secondary adapters in some error
  screens since multi-initiator RAID is not supported and the
  messages will just cause confusion.
* Mon Mar 24 2005 Brian King <brking@us.ibm.com>
- iprconfig: During disk hotplug, wait for sd devices to show
  up. Fixes errors getting logged by iprconfig during hotplug.
* Mon Mar 23 2005 Brian King <brking@us.ibm.com>
- iprconfig: Fix cancel path on concurrent add/remove of disks
- Don't display current bus width and speed for SAS disks
* Mon Mar 21 2005 Brian King <brking@us.ibm.com>
- Fix scoping bug caught by gcc 4.0.
* Fri Mar 18 2005 Brian King <brking@us.ibm.com>
- Stop iprupdate from continually logging errors for adapters with
  backlevel adapter firmware.
* Mon Mar 7 2005 Brian King <brking@us.ibm.com> 2.0.14
- Add support for non-interactive array creation and deletion through
  iprconfig.
- Use kobject_uevent notifications instead of polling if the kernel
  supports it.
- Fix iprconfig to set the actual queue depth for advanced function disks
- Allow user to force tagged queuing on to drives that do not support
  QERR=1.
- Fix handling of medium format corrupt drives for drives
- iprconfig: Download microcode. Fix blank screen when displaying
  lots of microcode images.
- Fix iprinit to wait for scsi generic devices to show up in case we are
  racing with hotplug. Fixes the following error:
      0:255:0:0: Mode Sense failed. rc=1, SK: 5 ASC: 24 ASCQ: 0
- Add "known to be zeroed" tracking to iprconfig to drastically reduce the
  time required to create a RAID array when starting with 512 formatted disks
- Add ability to query multi-adapter status for dual initiator RAID configs
- Add ability to set "preferred primary" adapter when running dual initiator RAID configs
- Add iprconfig screen to display dual adapter status for dual initiator RAID configs
- Prevent RAID configuration from occurring on "secondary" adapter in dual initiator RAID configs
- Use /dev/sd for SG_IO instead of /dev/sg when possible
- Set QERR=3 rather than 1 for multi-initiator configurations
- Set TST=1 for multi-initiator configurations
- Allow Format device for JBOD function to work for JBOD adapters
- Fix handling of dead adapters in all of iprutils.
- Fix iprconfig RAID start bug for systems with multiple RAID adapters.
- Fix iprconfig RAID include bug for systems with multiple RAID adapters.
- Fix failing array add device due to race condition with iprinit.
* Tue Oct 5 2004 Brian King <brking@us.ibm.com> 2.0.13
- Improve iprupdate error logs to indicate where to download microcode from.
- Set default tcq queue depth for AS400 disks to 16.
- Don't log errors in iprdump if CONFIG_IPR_DUMP not enabled in the kernel
- Fix sysfs parsing to handle new sdev target kernel change
- Rescan JBOD devices following recovery format to make the device usable if
  it was originally in an unsupported sector size.
- Display correct adapter serial number in iprconfig.
- Support for microcode download to new adapters.
- Support for iSeries disk microcode update using microcode images from
  the pSeries microcode website.
* Fri Jun 11 2004 Brian King <brking@us.ibm.com> 2.0.12
- Fix bug preventing ucode download to iSeries disks from working
* Thu Jun 10 2004 Brian King <brking@us.ibm.com> 2.0.11
- Fix segmentation fault in _sg_ioctl that was causing a silent
  failure of microcode update to disks. The microcode update would
  fail, but no error would be logged. The seg fault was in a child
  process, so the parent process kept running.
* Thu May 23 2004 Brian King <brking@us.ibm.com> 2.0.10
- Don't let iprdbg sg ioctls be retried.
- Add --force flag to iprconfig to allow user to workaround buggy
  drive firmware.
- Don't initialize read/write protected disks
- Fix some reclaim cache bugs
- Don't setup Mode Page 0x0A if test unit ready fails
* Thu May 2 2004 Brian King <brking@us.ibm.com> 2.0.9
- Add --debug option to all utilities
- Make utilities behave better when ipr is not loaded
- Fix dependencies in init.d scripts
- Only enable init.d scripts on ppc
- Don't log an error if ipr is not loaded
* Mon Apr 28 2004 Brian King <brking@us.ibm.com> 2.0.8
- Fix to properly enable init.d scripts when the rpm is installed
- Fix memory leak in code download path
- Increase size of page 0 inquiry buffer so that extended vpd is displayed
- Decrease write buffer timeout to 2 minutes
* Wed Apr 16 2004 Brian King <brking@us.ibm.com> 2.0.7
- Load sg module in init.d scripts if not loaded
- Load sg module in iprconfig if not loaded
* Wed Apr 14 2004 Brian King <brking@us.ibm.com> 2.0.6
- Battery maintenance fixes.
- Fix to properly display failed status for pulled physical disks.
* Tue Apr 6 2004 Brian King <brking@us.ibm.com> 2.0.5
- Battery maintenance fixes.
- Fix init.d scripts to work properly with yast runlevel editor.
- Fix device details screen in iprconfig for Failed array members
- Allow formatting devices even if qerr cannot be disabled 
* Tue Mar 29 2004 Brian King <brking@us.ibm.com> 2.0.4
- Fixed some sysfs calls that changed their calling interfaces
* Tue Mar 17 2004 Brian King <brking@us.ibm.com> 2.0.2-3
- Fixed 
