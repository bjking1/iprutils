Summary: Utilities for the IBM Power Linux RAID adapters
Name: iprutils
Version: 1.0.2
Release: 1
License: IPL
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
install -d $RPM_BUILD_ROOT/etc/rc.d/init.d
install init.d/iprdump $RPM_BUILD_ROOT/etc/rc.d/init.d/iprdump
install init.d/iprupdate $RPM_BUILD_ROOT/etc/rc.d/init.d/iprupdate

%post
/sbin/chkconfig --add iprdump
/sbin/chkconfig --add iprupdate

%preun
/sbin/chkconfig --del iprdump
/sbin/chkconfig --del iprupdate

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc iprutils/README
/sbin/iprconfig
/sbin/iprupdate
/sbin/iprdump
/sbin/iprtrace
/sbin/iprdbg
%{_mandir}/man8/iprdump.8.gz
%{_mandir}/man8/iprconfig.8.gz
%{_mandir}/man8/iprupdate.8.gz
/etc/rc.d/init.d/iprupdate
/etc/rc.d/init.d/iprdump
