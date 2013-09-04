%global _hardened_build 1

%global __provides_exclude_from ^%{python_sitearch}/fts/.*\\.so$

Name: fts
Version: 3.1.8
Release: 1%{?dist}
Summary: File Transfer Service V3
Group: System Environment/Daemons
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export https://svn.cern.ch/reps/fts3/tags/EPEL_release_1_EPEL_TESTING fts3
#  tar -czvf fts-0.0.1-60.tar.gz fts-00160
Source0: https://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/tar/%{name}-%{version}.tar.gz

%if 0%{?el5}
BuildRequires:  activemq-cpp-library
%else
BuildRequires:  activemq-cpp-devel
%endif
BuildRequires:  apr-devel
BuildRequires:  apr-util-devel
BuildRequires:  boost-devel
BuildRequires:  CGSI-gSOAP-devel
BuildRequires:  cmake
BuildRequires:  doxygen
%if 0%{?el5}
BuildRequires:  e2fsprogs-devel
%else
BuildRequires:  libuuid-devel
%endif
BuildRequires:  gfal2-devel
BuildRequires:  glib2-devel
BuildRequires:  globus-gsi-credential-devel
BuildRequires:  gridsite-devel
BuildRequires:  gsoap-devel
BuildRequires:  is-interface-devel
BuildRequires:  libcurl-devel
BuildRequires:  openldap-devel
BuildRequires:  pugixml-devel
BuildRequires:  python2-devel
BuildRequires:  voms-devel
Requires(pre):  shadow-utils

%description
The File Transfer Service V3 is the successor of File Transfer Service V2.
It is a service and a set of command line tools for managing third party
transfers, most importantly the aim of FTS3 is to transfer the data produced
by CERN's LHC into the computing GRID.

%package devel
Summary: Development files for File Transfer Service V3
Group: Applications/Internet
Requires: fts-libs%{?_isa} = %{version}-%{release}

%description devel
This package contains development files 
(e.g. header files) for File Transfer Service V3.

%package server
Summary: File Transfer Service version 3 server
Group: System Environment/Daemons
Requires: bdii
Requires: fts-libs%{?_isa} = %{version}-%{release}
Requires: gfal2-plugin-gridftp%{?_isa} >= 2.1.0
#Requires: gfal2-plugin-http%{?_isa} >= 2.1.0
Requires: gfal2-plugin-srm%{?_isa} >= 2.1.0
Requires: glue-schema
Requires: glue-validator
Requires: gridsite >= 1.7.25
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(postun): initscripts
Requires(preun): initscripts

#Requires: emi-resource-information-service (from EMI3)
#Requires: emi-version (from EMI3)
#Requires: fetch-crl3 (metapackage)

%description server
The FTS server is a service which accepts transfer jobs,
it exposes both a SOAP and a RESTful interface. The File
Transfer Service allows also for querying and canceling
transfer-jobs. The authentication and authorization is
VOMS based. Furthermore, the service provides a mechanism that
dynamically adjust transfer parameters for optimal bandwidth
utilization and allows for configuring so called VO-shares.

%package libs
Summary: File Transfer Service version 3 libraries
Group: System Environment/Libraries

%description libs
FTS common libraries used across the client and
server. This includes among others: configuration
parsing, logging and error-handling utilities, as
well as, common definitions and interfaces

%package python
Summary: File Transfer Service version 3 python bindings
Group: System Environment/Libraries
Requires: fts-libs%{?_isa} = %{version}-%{release}
Requires: python%{?_isa}

%description python
FTS python bindings for client libraries and DB API

%package client
Summary: File Transfer Service version 3 client
Group: Applications/Internet
Requires: fts-libs%{?_isa} = %{version}-%{release}

%description client
A set of command line tools for submitting, querying
and canceling transfer-jobs to the FTS service. Additionally,
there is a CLI that can be used for configuration and
administering purposes.


%prep
%setup -qc


%build
mkdir build
cd build
%cmake -DMAINBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}


%install
cd build
mkdir -p %{buildroot}%{_var}/lib/fts3
mkdir -p %{buildroot}%{_var}/lib/fts3/monitoring
mkdir -p %{buildroot}%{_var}/lib/fts3/status
mkdir -p %{buildroot}%{_var}/lib/fts3/stalled
mkdir -p %{buildroot}%{_var}/lib/fts3/logs
mkdir -p %{buildroot}%{_var}/log/fts3
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{python_sitearch}/fts


%pre server
getent group fts3 >/dev/null || groupadd -r fts3
getent passwd fts3 >/dev/null || \
    useradd -r -g fts3 -d /var/log/fts3 -s /sbin/nologin \
    -c "fts3 urlcopy user" fts3
exit 0

%post server
/sbin/chkconfig --add fts-server
/sbin/chkconfig --add fts-bringonline
/sbin/chkconfig --add fts-msg-bulk
/sbin/chkconfig --add fts-msg-cron
/sbin/chkconfig --add fts-records-cleaner
/sbin/chkconfig --add fts-info-publisher
/sbin/chkconfig --add fts-myosg-updater
/sbin/chkconfig --add fts-bdii-cache-updater
exit 0

%preun server
if [ $1 -eq 0 ] ; then
    /sbin/service fts-server stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-server
    /sbin/service fts-bringonline stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-bringonline    
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
    /sbin/service fts-bdii-cache-updater stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-bdii-cache-updater
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
    /sbin/service fts-server condrestart >/dev/null 2>&1 || :
    /sbin/service fts-bringonline condrestart >/dev/null 2>&1 || :
    /sbin/service fts-msg-bulk condrestart >/dev/null 2>&1 || :
    /sbin/service fts-msg-cron condrestart >/dev/null 2>&1 || :
    /sbin/service fts-records-cleaner condrestart >/dev/null 2>&1 || :
    /sbin/service fts-info-publisher condrestart >/dev/null 2>&1 || :
    /sbin/service fts-myosg-updater condrestart >/dev/null 2>&1 || :
    /sbin/service fts-bdii-cache-updater condrestart >/dev/null 2>&1 || :
fi
exit 0


%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig


%files server
%dir %{_sysconfdir}/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/monitoring
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/status
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/stalled
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/logs
%dir %attr(0755,fts3,root) %{_var}/log/fts3
%attr(0644,fts3,root) %{_var}/lib/fts3/bdii_cache.xml
%attr(0644,fts3,root) %{_var}/lib/fts3/myosg.xml
%{_sbindir}/fts*
%attr(0755,root,root) %{_initddir}/fts-msg-bulk
%attr(0755,root,root) %{_initddir}/fts-server
%attr(0755,root,root) %{_initddir}/fts-bringonline
%attr(0755,root,root) %{_initddir}/fts-msg-cron
%attr(0755,root,root) %{_initddir}/fts-records-cleaner
%attr(0755,root,root) %{_initddir}/fts-info-publisher
%attr(0755,root,root) %{_initddir}/fts-myosg-updater
%attr(0755,root,root) %{_initddir}/fts-bdii-cache-updater
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-records-cleaner
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-msg-cron
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-info-publisher
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-myosg-updater
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-bdii-cache-updater
%config(noreplace) %attr(0640,root,fts3) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%config(noreplace) %attr(0640,root,fts3) %{_sysconfdir}/fts3/fts3config
%config(noreplace) %{_sysconfdir}/logrotate.d/fts-server
%{_mandir}/man8/fts*

%files client
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
%{_mandir}/man1/fts*

%files libs
%{_libdir}/libfts_common.so.*
%{_libdir}/libfts_config.so.*
%{_libdir}/libfts_infosys.so.*
%{_libdir}/libfts_db_generic.so.*
%{_libdir}/libfts_db_profiled.so
%{_libdir}/libfts_msg_ifce.so.*
%{_libdir}/libfts_proxy.so.*
%{_libdir}/libfts_server_gsoap_transfer.so.*
%{_libdir}/libfts_server_lib.so.*
%{_libdir}/libfts_cli_common.so.*
%{_libdir}/libfts_ws_ifce_client.so.*
%{_libdir}/libfts_ws_ifce_server.so.*
%{_libdir}/libfts_delegation_api_simple.so.*
%{_libdir}/libfts_delegation_api_cpp.so.*
%{_libdir}/libfts_profiler.so
%doc README
%doc LICENSE


%files python
%{python_sitearch}/fts


%files devel
%{_bindir}/fts*
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
%{_mandir}/man1/fts*


%changelog
* Wed Aug 07 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-2
  - GenericDbIfce.h includes new blacklisting API
  - no longer linking explicitly to boost libraries with '-mt' sufix 
* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1.0-3
  - Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild
* Sat Jul 27 2013 Petr Machata <pmachata@redhat.com> - 3.1.0-2
  - rebuild for boost 1.54.0
  - boost package doesn't use tagged sonames anymore, drop the -mt
    suffix from several boost libraries (fts-3.1.0-boost_mt.patch)
* Wed Jul 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.3-15
  - compatible with rawhide (f20)
* Fri Jul 02 2013 Michail Salichos <michail.salichos@cern.ch> - 3.0.3-14
  - mysql queries optimization
* Fri Jun 14 2013 Michal Simon <michal.simon@cern.ch> - 3.0.3-1
  - dependency on 'gfal2-plugin-http' has been removed
  - the calls to mktemp have been removed
  - cmake build type changed from Release to RelWithDebInfo
  - EPEL5 specifics have been removed from spec files
  - changelog has been fixed
* Fri May 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.2-1
  - speling has been fixed in package's description
  - man pages added to devel package
  - services are disabled by default
  - missing 'Requires(post): chkconfig' and 'Requires(preun): chkconfig' added
* Tue Apr 30 2013 Michal Simon <michal.simon@cern.ch> - 3.0.1-1
  - BuildRequires and Requires entries have been sorted alphabetically
  - the non standard compilation options have been removed
  - package and the subpackages descriptions have been updated
  - documentation files listed as %doc
  - trailing white-spaces have been removed
* Wed Apr 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.0-1
  - First EPEL release
