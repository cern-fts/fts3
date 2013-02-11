%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name: fts
Version: 0.0.1 
Release: 61%{?dist}
Summary: File Transfer Service V3
Group: System Environment/Daemons 
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export http://svnweb.cern.ch/guest/fts3/trunk
#  tar -czvf fts-0.0.1-60.tar.gz fts-00160
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cmake
BuildRequires:  apr-devel%{?_isa}
BuildRequires:  apr-util-devel%{?_isa}
%if 0%{?el5}
BuildRequires:  activemq-cpp-library%{?_isa}
%else
BuildRequires:  activemq-cpp-devel%{?_isa}
%endif
BuildRequires:  openldap-devel%{?_isa}
BuildRequires:  gsoap-devel%{?_isa}
BuildRequires:  doxygen%{?_isa}
%if 0%{?el5}
BuildRequires:  e2fsprogs-devel%{?_isa}
%else
BuildRequires:  libuuid-devel%{?_isa}
%endif
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  globus-gsi-credential-devel%{?_isa}
BuildRequires:  CGSI-gSOAP-devel%{?_isa}
BuildRequires:  is-interface-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  gridsite-devel%{?_isa}
BuildRequires:  gfal2-devel%{?_isa}
BuildRequires:  voms-devel%{?_isa}
BuildRequires:  python-devel%{?_isa}
BuildRequires:  pugixml-devel%{?_isa}
BuildRequires:  libcurl-devel%{?_isa}
Requires(pre):  shadow-utils


%description
The File Transfer Service V3

%package devel
Summary: Development files for File Transfer Service V3
Group: Applications/Internet
Requires: fts-libs = %{version}-%{release}

%description devel
Development files for File Transfer Service V3

%package server
Summary: File Transfer Service version 3 server
Group: System Environment/Daemons
Requires: fts-libs = %{version}-%{release}
Requires: gfal2-plugin-gridftp >= 2.1.0
Requires: gfal2-plugin-srm >= 2.1.0
Requires: bdii
Requires: glue-schema
Requires: glue-validator

#Requires: emi-resource-information-service (from EMI3)
#Requires: emi-version (from EMI3)
#Requires: fetch-crl3 (metapackage)
#Requires: gfal2-plugin-http (when ready)

%package libs
Summary: File Transfer Service version 3 libs
Group: System Environment/Libraries
Requires: python

%package client
Summary: File Transfer Service version 3 client
Group: Applications/Internet
Requires: fts-libs = %{version}-%{release}

%description server
FTS server is a service which accepts transfer jobs, querying their status, etc

%description libs
FTS common libraries used across the client and server

%description client
FTS client CLI tool for submitting transfers, etc

%prep
%setup -qc


%build
mkdir build
cd build
%cmake -DMAINBUILD=ON -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}


%install
cd build
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_var}/lib/fts3
mkdir -p %{buildroot}%{_var}/lib/fts3/monitoring
mkdir -p %{buildroot}%{_var}/lib/fts3/status
mkdir -p %{buildroot}%{_var}/lib/fts3/stalled
mkdir -p %{buildroot}%{_var}/log/fts3
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{python_sitearch}/fts

%pre server
getent group fts3 >/dev/null || groupadd -r fts3
getent passwd fts3 >/dev/null || \
    useradd -r -g fts3 -d /var/log/fts3 -s /sbin/nologin \
    -c "fts3 urlcopy user" fts3
exit 0

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post devel -p /sbin/ldconfig

%postun devel -p /sbin/ldconfig

%post server
/sbin/chkconfig --add fts-server
/sbin/chkconfig --add fts-msg-bulk
/sbin/chkconfig --add fts-msg-cron
/sbin/chkconfig --add fts-records-cleaner
/sbin/chkconfig --add fts-info-publisher
/sbin/chkconfig --add fts-myosg-updater
exit 0

%preun server
if [ $1 -eq 0 ] ; then
    /sbin/service fts-server stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-server
    /sbin/service fts-msg-bulk stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-msg-bulk
    /sbin/service fts-msg-cron stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-msg-cron
    /sbin/service fts-records-cleaner stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-records-cleaner
    /sbin/service fts-info-publisher stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-info-publisher
    /sbin/service fts-myosg-updater stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-myosg-updater
    if [ -f /dev/shm/fts3mqupdater ]; then rm -rf /dev/shm/fts3mqupdater; fi
    if [ -f /dev/shm/fts3mqmon ]; then rm -rf /dev/shm/fts3mqmon; fi
    if [ -f /dev/shm/fts3mq ]; then rm -rf /dev/shm/fts3mq; fi
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
    /sbin/service fts-server condrestart >/dev/null 2>&1 || :
    /sbin/service fts-msg-bulk condrestart >/dev/null 2>&1 || :
    /sbin/service fts-msg-cron condrestart >/dev/null 2>&1 || :
    /sbin/service fts-records-cleaner condrestart >/dev/null 2>&1 || :    
    /sbin/service fts-info-publisher condrestart >/dev/null 2>&1 || :
    /sbin/service fts-myosg-updater condrestart >/dev/null 2>&1 || :        
fi
exit 0


%clean
rm -rf %{buildroot}



%files server
%defattr(-,root,root,-)
%dir %{_sysconfdir}/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/monitoring
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/status
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/stalled
%dir %attr(0755,fts3,root) %{_var}/log/fts3
%{_sbindir}/fts_msg_cron
%{_sbindir}/fts_msg_bulk
%{_sbindir}/fts_server
%{_sbindir}/fts_url_copy
%{_sbindir}/fts_db_cleaner
%{_sbindir}/fts_info_publisher
%{_sbindir}/fts_myosg_updater
%attr(0755,root,root) %{_initddir}/fts-msg-bulk
%attr(0755,root,root) %{_initddir}/fts-server
%attr(0755,root,root) %{_initddir}/fts-msg-cron
%attr(0755,root,root) %{_initddir}/fts-records-cleaner
%attr(0755,root,root) %{_initddir}/fts-info-publisher
%attr(0755,root,root) %{_initddir}/fts-myosg-updater
%config %{_sysconfdir}/logrotate.d/fts-server
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-records-cleaner
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-msg-cron
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-info-publisher
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-myosg-updater
%config(noreplace) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%config(noreplace) %{_sysconfdir}/fts3/fts3config
%{_mandir}/man8/fts_server.8.gz

%files client
%defattr(-,root,root,-)
%{_bindir}/fts-config-set
%{_bindir}/fts-config-get
%{_bindir}/fts-config-del
%{_bindir}/fts-set-debug
%{_bindir}/fts-set-blacklist
%{_bindir}/fts-set-priority
%{_bindir}/fts-transfer-list
%{_bindir}/fts-transfer-status
%{_bindir}/fts-transfer-submit
%{_bindir}/fts-transfer-cancel
%{_mandir}/man1/fts-config-get.1*
%{_mandir}/man1/fts-config-set.1*
%{_mandir}/man1/fts-config-del.1*
%{_mandir}/man1/fts-transfer-cancel.1*
%{_mandir}/man1/fts-transfer-list.1*
%{_mandir}/man1/fts-transfer-status.1*
%{_mandir}/man1/fts-transfer-submit.1*
%{_mandir}/man1/fts-set-priority.1*
%{_mandir}/man1/fts-set-debug.1*
%{_mandir}/man1/fts-set-blacklist.1*

%files libs
%defattr(-,root,root,-)
%{_bindir}/fts*
%{python_sitearch}/fts/*.py*
%{python_sitearch}/fts/ftsdb.so*
%{python_sitearch}/fts/libftspython.so*
%{_libdir}/libfts_common.so.*
%{_libdir}/libfts_config.so.*
%{_libdir}/libfts_infosys.so.*
%{_libdir}/libfts_db_generic.so.*
%{_libdir}/libfts_msg_ifce.so.*
%{_libdir}/libfts_proxy.so.*
%{_libdir}/libfts_server_gsoap_transfer.so.*
%{_libdir}/libfts_server_lib.so.*
%{_libdir}/libfts_cli_common.so.*
%{_libdir}/libfts_ws_ifce_client.so.*
%{_libdir}/libfts_ws_ifce_server.so.*
%{_libdir}/libfts_delegation_api_simple.so.*
%{_libdir}/libfts_delegation_api_cpp.so.*
%{_docdir}/fts3/README

%files devel
%defattr(-,root,root,-)
%{_bindir}/fts*
%{python_sitearch}/fts/ftsdb.so
%{python_sitearch}/fts/libftspython.so
%{_libdir}/libfts_common.so
%{_libdir}/libfts_config.so
%{_libdir}/libfts_infosys.so
%{_libdir}/libfts_db_generic.so
%{_libdir}/libfts_msg_ifce.so
%{_libdir}/libfts_proxy.so
%{_libdir}/libfts_server_gsoap_transfer.so
%{_libdir}/libfts_server_lib.so
%{_libdir}/libfts_cli_common.so
%{_libdir}/libfts_ws_ifce_client.so
%{_libdir}/libfts_ws_ifce_server.so
%{_libdir}/libfts_delegation_api_simple.so
%{_libdir}/libfts_delegation_api_cpp.so
%{_docdir}/fts3/README


%changelog
 * Wed Aug 8 2012 Steve Traylen <steve.traylen@cern.ch> - 0.0.0-61%{?dist}
  - A bit like a fedora package
