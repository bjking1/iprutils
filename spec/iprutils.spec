Summary: Utilities for the IBM Power Linux RAID adapters
Name: iprutils
Version: 2.0.0
Release: 1
License: CPL
Group: Hardware/SCSI
Vendor: IBM
Source0: iprutils-%version-src.tgz

%description
Provides a suite of utilities to manage and configure SCSI devices
supported by the ipr SCSI storage device driver.

%prep
rm -rf $RPM_BUILD_DIR/iprutils
tar -xvzf $RPM_SOURCE_DIR/iprutils-%version-src.tgz

%build
cd iprutils
make

%install
cd iprutils
make INSTALL_MOD_PATH=$RPM_BUILD_ROOT install
install -d $RPM_BUILD_ROOT/etc/init.d
install init.d/iprdump $RPM_BUILD_ROOT/etc/init.d/iprdump
install init.d/iprinit $RPM_BUILD_ROOT/etc/init.d/iprinit
install init.d/iprupdate $RPM_BUILD_ROOT/etc/init.d/iprupdate

%post
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/etc/init.d/iprdump
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/etc/init.d/iprinit
/usr/lib/lsb/install_initd $RPM_BUILD_ROOT/etc/init.d/iprupdate

%preun
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/etc/init.d/iprdump
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/etc/init.d/iprinit
/usr/lib/lsb/remove_initd $RPM_BUILD_ROOT/etc/init.d/iprupdate

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc iprutils/README
/sbin/iprconfig
/sbin/iprupdate
/sbin/iprdump
/sbin/iprinit
/sbin/iprdbg
%{_mandir}/man8/iprdump.8.gz
%{_mandir}/man8/iprconfig.8.gz
%{_mandir}/man8/iprupdate.8.gz
/etc/init.d/iprdump
/etc/init.d/iprinit
/etc/init.d/iprupdate
/usr/share/ipr/ipr.cat
