%global _hardened_build 1
%global __provides_exclude_from ^%{python_sitearch}/fts/.*\\.so$
%global selinux_policyver %(sed -e 's,.*selinux-policy-\\([^/]*\\)/.*,\\1,' /usr/share/selinux/devel/policyhelp || echo 0.0.0)
%global selinux_variants mls targeted

Name: fts
Version: 3.2.25
Release: 5%{?dist}
Summary: File Transfer Service V3
Group: System Environment/Daemons
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export https://svn.cern.ch/reps/fts3/trunk fts3
#  tar -czvf fts-3.2.25-2.tar.gz fts3
Source0: https://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/tar/%{name}-%{version}.tar.gz
Source1: fts.te
Source2: fts.fc

%if 0%{?el5}
BuildRequires:  activemq-cpp-library
%else
BuildRequires:  activemq-cpp-devel
%endif
BuildRequires:  boost-devel
BuildRequires:  CGSI-gSOAP-devel

%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  cmake
%else
BuildRequires:  cmake28
%endif

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
BuildRequires:  checkpolicy, selinux-policy-devel, selinux-policy-doc 

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

Requires: fts-libs%{?_isa} = %{version}-%{release}
Requires: gfal2-plugin-gridftp%{?_isa} >= 2.1.0
Requires: gfal2-plugin-http%{?_isa} >= 2.1.0
Requires: gfal2-plugin-srm%{?_isa} >= 2.1.0
#Requires: gfal2-plugin-xrootd%{?_isa} >= 0.2.2
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

%package infosys
Summary: File Transfer Service version 3 infosys integration
Group: System Environment/Daemons
Requires: bdii
Requires: fts-server%{?_isa} = %{version}-%{release}
Requires: glue-schema
Requires: glue-validator
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(postun): initscripts
Requires(preun): initscripts

%description infosys
FTS infosys integration

%package msg
Summary: File Transfer Service version 3 messaging integration
Group: System Environment/Daemons
Requires: fts-server%{?_isa} = %{version}-%{release}

%description msg
FTS messaging integration

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

%package server-selinux
Summary:    SELinux support for fts-server
Group:      Applications/Internet
Requires: fts-server%{?_isa} = %{version}-%{release}
%if "%{_selinux_policy_version}" != ""
Requires:   selinux-policy >= %{_selinux_policy_version}
%else
Requires:   selinux-policy >= %{selinux_policyver}
%endif
Requires(post):   /usr/sbin/semodule, /sbin/restorecon, fts-server
Requires(postun): /usr/sbin/semodule, /sbin/restorecon, fts-server

%description server-selinux
This package setup the SELinux policies for the FTS3 server.

%prep
%setup -qc
mkdir SELinux
cp -p %{SOURCE1} %{SOURCE2} SELinux

%build
# Make sure the version in the spec file and the version used
# for building matches
fts_cmake_ver=`sed -n 's/^set(VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9]\+\).*/\2/p' CMakeLists.txt | paste -sd '.'`
if [ "$fts_cmake_ver" != "%{version}" ]; then
    echo "The version in the spec file does not match the CMakeLists.txt version!"
    echo "$fts_cmake_ver != %{version}"
    exit 1
fi

# Build
mkdir build
cd build
%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
    %cmake -DMAINBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%else
    %cmake28 -DMAINBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%endif

make %{?_smp_mflags}

cd -

# SELinux
cd SELinux
for selinuxvariant in %{selinux_variants}; do
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile
  mv fts.pp fts.pp.${selinuxvariant}
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile clean
done
cd -

%install
cd build
mkdir -p %{buildroot}%{_var}/lib/fts3
mkdir -p %{buildroot}%{_var}/lib/fts3/monitoring
mkdir -p %{buildroot}%{_var}/lib/fts3/status
mkdir -p %{buildroot}%{_var}/lib/fts3/stalled
mkdir -p %{buildroot}%{_var}/lib/fts3/logs
mkdir -p %{buildroot}%{_var}/log/fts3
mkdir -p %{buildroot}%{_sysconfdir}/fts3
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{python_sitearch}/fts

cd -

# SELinux
for selinuxvariant in %{selinux_variants}; do
  install -d %{buildroot}%{_datadir}/selinux/${selinuxvariant}
  install -p -m 644 SELinux/fts.pp.${selinuxvariant} %{buildroot}%{_datadir}/selinux/${selinuxvariant}/fts.pp
done

# Server scriptlets
%pre server
getent group fts3 >/dev/null || groupadd -r fts3
getent passwd fts3 >/dev/null || \
    useradd -r -g fts3 -d /var/log/fts3 -s /sbin/nologin \
    -c "fts3 urlcopy user" fts3
exit 0

%post server
/sbin/chkconfig --add fts-server
/sbin/chkconfig --add fts-bringonline
/sbin/chkconfig --add fts-records-cleaner
exit 0

%preun server
if [ $1 -eq 0 ] ; then
    /sbin/service fts-server stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-server
    /sbin/service fts-bringonline stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-bringonline    
    /sbin/service fts-records-cleaner stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-records-cleaner
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
    /sbin/service fts-server condrestart >/dev/null 2>&1 || :
    /sbin/service fts-bringonline condrestart >/dev/null 2>&1 || :
    /sbin/service fts-records-cleaner condrestart >/dev/null 2>&1 || :
fi
exit 0

# Infosys scriptlets
%post infosys
/sbin/chkconfig --add fts-info-publisher
/sbin/chkconfig --add fts-myosg-updater
/sbin/chkconfig --add fts-bdii-cache-updater
exit 0

%preun infosys
if [ $1 -eq 0 ] ; then
    /sbin/service fts-info-publisher stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-info-publisher
    /sbin/service fts-myosg-updater stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-myosg-updater
    /sbin/service fts-bdii-cache-updater stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-bdii-cache-updater
fi
exit 0

%postun infosys
if [ "$1" -ge "1" ] ; then
    /sbin/service fts-info-publisher condrestart >/dev/null 2>&1 || :
    /sbin/service fts-myosg-updater condrestart >/dev/null 2>&1 || :
    /sbin/service fts-bdii-cache-updater condrestart >/dev/null 2>&1 || :
fi
exit 0

# Messaging scriptlets
%post msg
/sbin/chkconfig --add fts-msg-bulk
/sbin/chkconfig --add fts-msg-cron
exit 0

%preun msg
if [ $1 -eq 0 ] ; then
    /sbin/service fts-msg-bulk stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-msg-bulk
    /sbin/service fts-msg-cron stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-msg-cron
fi
exit 0

%postun msg
if [ $1 -eq 0 ] ; then
    /sbin/service fts-msg-bulk condrestart >/dev/null 2>&1 || :
    /sbin/service fts-msg-cron condrestart >/dev/null 2>&1 || :
fi
exit 0

# Libs scriptlets
%post libs
/sbin/ldconfig || exit 1

%postun libs
/sbin/ldconfig || exit 1

#SELinux scriptlets
%post server-selinux

# Reset the context set by fts-monitoring-selinux
if [ $1 -eq 1 ]; then
    semanage fcontext -d -t httpd_sys_content_t "/var/log/fts3(/.*)?" &> /dev/null
fi

for selinuxvariant in %{selinux_variants}; do
  /usr/sbin/semodule -s ${selinuxvariant} -i %{_datadir}/selinux/${selinuxvariant}/fts.pp &> /dev/null || :
done
/sbin/restorecon -R %{_var}/log/fts3 || :

%postun server-selinux
if [ $1 -eq 0 ] ; then
  for selinuxvariant in %{selinux_variants}
  do
    /usr/sbin/semodule -s ${selinuxvariant} -r fts &> /dev/null || :
  done
  [ -d %{_var}/log/fts3 ]  && /sbin/restorecon -R %{_var}/log/fts3 &> /dev/null || :
fi

%files server
%dir %attr(0755,fts3,root) %{_var}/lib/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/monitoring
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/status
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/stalled
%dir %attr(0755,fts3,root) %{_var}/lib/fts3/logs
%dir %attr(0755,fts3,root) %{_var}/log/fts3
%dir %attr(0755,fts3,root) %{_sysconfdir}/fts3

%{_sbindir}/fts_bringonline
%{_sbindir}/fts_db_cleaner
%{_sbindir}/fts_server
%{_sbindir}/fts_url_copy
%attr(0755,root,root) %{_initddir}/fts-server
%attr(0755,root,root) %{_initddir}/fts-bringonline
%attr(0755,root,root) %{_initddir}/fts-records-cleaner
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-records-cleaner
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts3config
%config(noreplace) %{_sysconfdir}/logrotate.d/fts-server
%{_mandir}/man8/fts_bringonline.8.gz
%{_mandir}/man8/fts_db_cleaner.8.gz
%{_mandir}/man8/fts_server.8.gz
%{_mandir}/man8/fts_url_copy.8.gz

%files infosys
%{_sbindir}/fts_bdii_cache_updater
%{_sbindir}/fts_info_publisher
%{_sbindir}/fts_myosg_updater
%config(noreplace) %attr(0644,fts3,root) %{_var}/lib/fts3/bdii_cache.xml
%config(noreplace) %attr(0644,fts3,root) %{_var}/lib/fts3/myosg.xml
%attr(0755,root,root) %{_initddir}/fts-info-publisher
%attr(0755,root,root) %{_initddir}/fts-myosg-updater
%attr(0755,root,root) %{_initddir}/fts-bdii-cache-updater
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-info-publisher
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-myosg-updater
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-bdii-cache-updater
%{_mandir}/man8/fts_bdii_cache_updater.8.gz
%{_mandir}/man8/fts_info_publisher.8.gz
%{_mandir}/man8/fts_myosg_updater.8.gz

%files msg
%{_sbindir}/fts_msg_bulk
%{_sbindir}/fts_msg_cron
%attr(0755,root,root) %{_initddir}/fts-msg-bulk
%attr(0755,root,root) %{_initddir}/fts-msg-cron
%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-msg-cron
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%{_mandir}/man8/fts_msg_bulk.8.gz
%{_mandir}/man8/fts_msg_cron.8.gz

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
%{_bindir}/fts-transfer-snapshot
%{_bindir}/fts-delegation-init

%{_mandir}/man1/fts*

%files libs
%{_libdir}/libfts_common.so.*
%{_libdir}/libfts_config.so.*
%{_libdir}/libfts_infosys.so.*
%{_libdir}/libfts_db_generic.so.*
%{_libdir}/libfts_db_profiled.so.*
%{_libdir}/libfts_msg_ifce.so.*
%{_libdir}/libfts_proxy.so.*
%{_libdir}/libfts_server_gsoap_transfer.so.*
%{_libdir}/libfts_server_lib.so.*
%{_libdir}/libfts_cli_common.so.*
%{_libdir}/libfts_ws_ifce_client.so.*
%{_libdir}/libfts_ws_ifce_server.so.*
%{_libdir}/libfts_delegation_api_simple.so.*
%{_libdir}/libfts_delegation_api_cpp.so.*
%{_libdir}/libfts_profiler.so.*
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

%files server-selinux
%defattr(-,root,root,0755)
%doc SELinux/*
%{_datadir}/selinux/*/fts.pp

%changelog
* Tue Feb 04 2014 Alejandro Alvarez <aalvarez@cern.ch> - 3.2.25-4
  - introduced dist back in the release
* Tue Jan 14 2014 Alejandro Alvarez <aalvarez@cern.ch> - 3.2.25-3
  - using cmake28
* Mon Jan 13 2014 Alejandro Alvarez <aalvarez@cern.ch> - 3.2.25-2
  - separated rpms for messaging and infosys subsystems
* Mon Nov 18 2013 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.1.33-2
  - Added missing changelog entry
  - Fixed bogus date
* Tue Oct 29 2013 Michal Simon <michal.simon@cern.ch> - 3.1.33-1
  - Update for new upstream release
* Wed Aug 07 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-2
  - no longer linking explicitly to boost libraries with '-mt' sufix 
* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1.0-3
  - Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild
* Sat Jul 27 2013 Petr Machata <pmachata@redhat.com> - 3.1.0-2
  - rebuild for boost 1.54.0
  - boost package doesn't use tagged sonames anymore, drop the -mt
    suffix from several boost libraries (fts-3.1.0-boost_mt.patch)
* Wed Jul 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.3-15
  - compatible with rawhide (f20)
* Tue Jul 02 2013 Michail Salichos <michail.salichos@cern.ch> - 3.0.3-14
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
