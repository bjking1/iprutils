Summary: Utilities for the IBM iSeries/pSeries SCSI storage adapter
Name: ibmsisutils
Version: 1.19.12
Release: 1
License: IPL
Group: Hardware/SCSI
Source0: ibmsisutils-%version-src.tgz
BuildRequires: ibmsis-devel

%description
Provides a suite of utilities to manage and configure SCSI devices
supported by the ibmsis SCSI storage device driver.

%prep
rm -rf $RPM_BUILD_DIR/sisutils
tar -xvzf $RPM_SOURCE_DIR/ibmsisutils-%version-src.tgz

%build
cd sisutils
make

%install
cd sisutils
make INSTALL_MOD_PATH=$RPM_BUILD_ROOT install
install -d $RPM_BUILD_ROOT/etc/rc.d/init.d
install init.d/sisdump $RPM_BUILD_ROOT/etc/rc.d/init.d/sisdump
install init.d/sisupdate $RPM_BUILD_ROOT/etc/rc.d/init.d/sisupdate

%post
/sbin/chkconfig --add sisdump
/sbin/chkconfig --add sisupdate

%postun
/sbin/chkconfig --del sisdump
/sbin/chkconfig --del sisupdate

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc sisutils/README
/sbin/sisconfig
/sbin/sisupdate
/sbin/sisdump
/sbin/sistrace
/sbin/sisdbg
%{_mandir}/man8/sisdump.8.gz
%{_mandir}/man8/sisconfig.8.gz
%{_mandir}/man8/sisupdate.8.gz
/etc/rc.d/init.d/sisupdate
/etc/rc.d/init.d/sisdump
