Summary: Utilities for the IBM Power Linux RAID adapters
Name: iprutils
Version: 2.0.11
Release: 1
License: CPL
Group: System Environment/Base
Vendor: IBM
URL: http://www-124.ibm.com/storageio/
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
/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprdump
/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprupdate
/usr/lib/lsb/remove_initd %{_sysconfdir}/init.d/iprinit
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
