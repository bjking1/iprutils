Summary: Utilities for the IBM Power Linux RAID adapters
Name: iprutils
Version: 2.0.6
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
install -m 755 init.d/iprdump $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprdump
install -m 755 init.d/iprinit $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprinit
install -m 755 init.d/iprupdate $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprupdate

%post
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprdump
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprinit
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprupdate

%preun
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprdump
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprinit
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/iprupdate

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc README LICENSE
/sbin/*
%{_mandir}/man*/*
%{_sysconfdir}/init.d/*

%changelog
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
