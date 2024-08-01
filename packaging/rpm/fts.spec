%global _hardened_build 1
%global selinux_policyver %(sed -e 's,.*selinux-policy-\\([^/]*\\)/.*,\\1,' /usr/share/selinux/devel/policyhelp || echo 0.0.0)
%global selinux_variants mls targeted
# Compile Python scripts using Python3
%define __python python3

Name:       fts
Version:    3.13.1
Release:    1%{?dist}
Summary:    File Transfer Service V3
License:    ASL 2.0
URL:        https://fts.web.cern.ch/
# git clone --depth=1 --branch v3.13.1 https://gitlab.cern.ch/fts/fts3.git fts-3.13.1
# tar -vczf fts-3.13.1.tar.gz --exclude-vcs fts-3.13.1/
Source0: %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  activemq-cpp-devel
BuildRequires:  boost-devel
BuildRequires:  cmake3
BuildRequires:  libdirq-devel
BuildRequires:  doxygen
BuildRequires:  libuuid-devel
BuildRequires:  gfal2-devel >= 2.23.0
BuildRequires:  glib2-devel
BuildRequires:  globus-gsi-credential-devel
BuildRequires:  gridsite-devel
BuildRequires:  openldap-devel
BuildRequires:  protobuf-devel
BuildRequires:  voms-devel
BuildRequires:  checkpolicy, selinux-policy-devel, selinux-policy-doc
BuildRequires:  systemd
BuildRequires:  cppzmq-devel
BuildRequires:  jsoncpp-devel
BuildRequires:  davix-devel >= 0.8.4
BuildRequires:  cryptopp-devel >= 5.6.2
Requires(pre):  shadow-utils

%description
The File Transfer Service V3 is the successor of File Transfer Service V2.
It is a service and a set of command line tools for managing third party
transfers, most importantly the aim of FTS3 is to transfer the data produced
by CERN's LHC into the computing GRID.

%package devel
Summary: Development files for File Transfer Service V3
Requires: fts-libs%{?_isa} = %{version}-%{release}

%description devel
This package contains development files
(e.g. header files) for File Transfer Service V3.

%package server
Summary: File Transfer Service version 3 server

Requires: fts-libs%{?_isa} = %{version}-%{release}
Requires: gfal2%{?_isa} >= 2.23.0
Requires: gfal2-plugin-http%{?_isa} >= 2.23.0
Requires: gfal2-plugin-srm%{?_isa} >= 2.23.0
#Requires: gfal2-plugin-xrootd%{?_isa}
Requires: gridsite >= 1.7.25
Requires: jsoncpp
Requires: python3-requests
Requires: python3-PyMySQL

Requires(post):   systemd
Requires(preun):  systemd
Requires(postun): systemd

%description server
The FTS server is a service which accepts transfer jobs,
it exposes a RESTful interface. The File
Transfer Service allows also for querying and canceling
transfer-jobs. The authentication and authorization is
VOMS based. Furthermore, the service provides a mechanism that
dynamically adjust transfer parameters for optimal bandwidth
utilization and allows for configuring so called VO-shares.

%package libs
Summary:    File Transfer Service version 3 libraries

Requires:   zeromq
Obsoletes:  fts-mysql-debuginfo < %{version}

%description libs
FTS common libraries used across the client and
server. This includes among others: configuration
parsing, logging and error-handling utilities, as
well as, common definitions and interfaces

%package msg
Summary:    File Transfer Service version 3 messaging integration
Requires:   fts-server%{?_isa} = %{version}-%{release}

%description msg
FTS messaging integration

%package server-selinux
Summary:    SELinux support for fts-server
Requires:   fts-server%{?_isa} = %{version}-%{release}
%if "%{_selinux_policy_version}" != ""
Requires:   selinux-policy >= %{_selinux_policy_version}
%else
Requires:   selinux-policy >= %{selinux_policyver}
%endif
Requires(post):   /usr/sbin/semodule, /sbin/restorecon, fts-server
Requires(postun): /usr/sbin/semodule, /sbin/restorecon, fts-server

%description server-selinux
This package setup the SELinux policies for the FTS3 server.

%package mysql
Summary:    File Transfer Service V3 mysql plug-in

BuildRequires:  soci-mysql-devel
Requires:   soci-mysql%{?_isa}
Requires:   fts-server%{?_isa}

%description mysql
The File Transfer Service V3 mysql plug-in

%package tests
Summary:    FTS unit tests
Group:      Development/Tools

Requires:   fts-libs%{?_isa} = %{version}-%{release}
Requires:   gfal2-plugin-mock

%description tests
Testing binaries for the FTS Server and related components.

%prep
%setup -q

%build
# Make sure the version in the spec file and the version used
# for building matches
fts_cmake_ver=`sed -n 's/^set(VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9]\+\).*/\2/p' CMakeLists.txt | paste -sd '.'`
fts_spec_ver=`expr "%{version}" : '\([0-9]*\\.[0-9]*\\.[0-9]*\)'`
if [ "$fts_cmake_ver" != "$fts_spec_ver" ]; then
    echo "The version in the spec file does not match the CMakeLists.txt version!"
    echo "$fts_cmake_ver != %{version}"
    exit 1
fi

%cmake3 -DSERVERBUILD=ON -DMYSQLBUILD=ON \
    -DTESTBUILD=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX='' \
    -DSYSTEMD_INSTALL_DIR=%{_unitdir}

%cmake3_build

# SELinux
cd selinux
for selinuxvariant in %{selinux_variants}; do
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile
  mv fts.pp fts.pp.${selinuxvariant}
  make NAME=${selinuxvariant} -f /usr/share/selinux/devel/Makefile clean
done
cd -

%install

mkdir -p %{buildroot}%{_var}/lib/fts3
mkdir -p %{buildroot}%{_var}/lib/fts3/monitoring
mkdir -p %{buildroot}%{_var}/lib/fts3/status
mkdir -p %{buildroot}%{_var}/lib/fts3/stalled
mkdir -p %{buildroot}%{_var}/lib/fts3/logs
mkdir -p %{buildroot}%{_var}/log/fts3
mkdir -p %{buildroot}%{_sysconfdir}/fts3
%cmake3_install

# SELinux
for selinuxvariant in %{selinux_variants}; do
  install -d %{buildroot}%{_datadir}/selinux/${selinuxvariant}
  install -p -m 644 selinux/fts.pp.${selinuxvariant} %{buildroot}%{_datadir}/selinux/${selinuxvariant}/fts.pp
done

# Server scriptlets
%pre server
getent group fts3 >/dev/null || groupadd -r fts3
getent passwd fts3 >/dev/null || \
    useradd -r -g fts3 -d /var/log/fts3 -s /sbin/nologin \
    -c "fts3 urlcopy user" fts3
exit 0

%post server
/bin/systemctl daemon-reload > /dev/null 2>&1 || :
exit 0

%preun server
if [ $1 -eq 0 ] ; then
  /bin/systemctl stop fts-server.service > /dev/null 2>&1 || :
  /bin/systemctl stop fts-records-cleaner.service > /dev/null 2>&1 || :
  /bin/systemctl --no-reload disable fts-server.service > /dev/null 2>&1 || :
  /bin/systemctl --no-reload disable fts-records-cleaner.service > /dev/null 2>&1 || :
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
  /bin/systemctl try-restart fts-server.service > /dev/null 2>&1 || :
  /bin/systemctl try-restart fts-records-cleaner.service > /dev/null 2>&1 || :
fi
exit 0

# Messaging scriptlets
%post msg
/bin/systemctl daemon-reload > /dev/null 2>&1 || :
exit 0

%preun msg
if [ $1 -eq 0 ] ; then
  /bin/systemctl stop fts-msg-bulk.service > /dev/null 2>&1 || :
  /bin/systemctl --no-reload disable fts-msg-bulk.service > /dev/null 2>&1 || :
fi
exit 0

%postun msg
if [ "$1" -ge "1" ] ; then
  /bin/systemctl try-restart fts-msg-bulk.service > /dev/null 2>&1 || :
fi
exit 0

# Libs scriptlets
%post libs
/sbin/ldconfig || exit 1

%postun libs
/sbin/ldconfig || exit 1

#SELinux scriptlets
%post server-selinux
if [ $1 -eq 1 ] ; then
  for selinuxvariant in %{selinux_variants}; do
    /usr/sbin/semodule -s ${selinuxvariant} -i %{_datadir}/selinux/${selinuxvariant}/fts.pp &> /dev/null || :
  done
  /sbin/restorecon -R %{_var}/log/fts3 || :
fi
exit 0

%postun server-selinux
if [ $1 -eq 0 ] ; then
  for selinuxvariant in %{selinux_variants} ; do
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

%{_sbindir}/fts_qos
%{_sbindir}/fts_db_cleaner
%{_sbindir}/fts_server
%{_sbindir}/fts_url_copy
%{_sbindir}/fts_db_rotate
%{_sbindir}/ftstokenrefresherd
%{_sbindir}/ftstokenhousekeeperd

%dir %attr(0755,root,root) %{_datadir}/fts/
%{_datadir}/fts/fts-database-upgrade.py*

%config(noreplace) %attr(0644,root,root) %{_unitdir}/fts-server.service
%config(noreplace) %attr(0644,root,root) %{_unitdir}/fts-records-cleaner.service
%config(noreplace) %attr(0644,root,root) %{_unitdir}/fts-qos.service
%config(noreplace) %attr(0644,root,root) %{_unitdir}/ftstokenrefresherd.service
%config(noreplace) %attr(0644,root,root) %{_unitdir}/ftstokenhousekeeperd.service

%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-records-cleaner
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts3config
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/ftstokenrefresherd.ini
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/ftstokenhousekeeperd.ini
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/sysconfig/fts-server
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/sysconfig/fts-qos
%config(noreplace) %{_sysconfdir}/logrotate.d/fts-server
%{_mandir}/man8/fts_qos.8.gz
%{_mandir}/man8/fts_db_cleaner.8.gz
%{_mandir}/man8/fts_server.8.gz
%{_mandir}/man8/fts_url_copy.8.gz

%files msg
%{_sbindir}/fts_msg_bulk

%config(noreplace) %attr(0644,root,root) %{_unitdir}/fts-msg-bulk.service

%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%{_mandir}/man8/fts_msg_bulk.8.gz

%files libs
%{_libdir}/libfts_common.so*
%{_libdir}/libfts_config.so*
%{_libdir}/libfts_db_generic.so*
%{_libdir}/libfts_msg_ifce.so*
%{_libdir}/libfts_proxy.so*
%{_libdir}/libfts_server_lib.so*
%{_libdir}/libfts_msg_bus.so*
%{_libdir}/libfts_url_copy.so*
%doc README.md
%doc LICENSE

%files server-selinux
%doc selinux/*
%{_datadir}/selinux/*/fts.pp

%files mysql
%{_libdir}/libfts_db_mysql.so.*
%{_datadir}/fts-mysql

%files tests
%{_bindir}/fts-unit-tests
%{_libdir}/fts-tests

%changelog
* Thu Aug 01 2024 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.13.1
- "Overwrite-when-only-on-disk" feature
- Drop CC7 build

* Thu May 30 2024 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.13.0
- Alma9 release

* Thu Oct 19 2023 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.11
- Add Scitag label to transfers

* Mon Aug 14 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.10
- New upstream release

* Tue Aug 08 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.9
- Create TPC role property configurable per Storage Endpoint
- Turn configurable eviction flag into skip-eviction
- Send transfer clean-up result to monitoring messages
- Add GFAL2 prefix to the messages from the GFAL2 library in the UrlCopy logs

* Mon Jun 12 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.8
- Always release a file after a copy if we have a bringonline token
- Migrate database upgrade script from Python2 to Python3

* Wed Apr 12 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.7
- Make default pin-lifetime and bring-online timeout configurable
- Optimizer ignore "Staged file does not exist" errors

* Mon Apr 03 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.6
- Send archive metadata to Url-Copy process
- Remove invalid export keyword from /etc/sysconfig/fts-qos
- Log host hashSegment when restarting the QoS daemon recovering already started tasks

* Mon Feb 20 2023 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.12.5-1
- Monitoring messages support for tape archive monitoring
- Fail multiple replica archiving jobs if archive monitoring fails
- Provide more useful information on default HTTP transfer logs
- Log 3rd party pull/push client IPs

* Tue Jan 24 2023 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.4-1
- Add throughput profiling logs
- Implement Force Start Transfers mechanism
- Provide more TransferComplete Monitoring Messages fields directly at source
- Introduce new "ipver" field in TransferComplete Monitoring Message
- Pass bringonline token to copy eviction
- Upgrade compiler support to C++17

* Fri Nov 18 2022 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.3-1
- QoS task generation critical fix

* Thu Nov 10 2022 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.2-1
- HTTP Staging functionality

* Tue Aug 30 2022 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.1-1
- Fail staging transfers met with operation not supported
- Use the Gfal2 configured bringonline TURL setting

* Thu Jul 14 2022 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.12.0-1
- New minor release
- Tape REST API functionality for Eviction and ArchiveInfo
- OC11/GDPR compliance with regards to temporary proxy certifcate file names
- Eviction property configurable per SE
- Multihop overwrite-hop feature
- Improved gitlab-CI

* Tue Jun 14 2022 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.11.3-1
- Fix for S3 alternate field

* Wed Feb 16 2022 Joao Lopes <joao.pedro.batista.lopes@cern.ch> - 3.11.2-1
- Introduce profiling logs
- Introduce new sanity-checker to prevent stuck multihop jobs

* Mon Oct 25 2021 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.11.1-1
- Introduce shuffling of Activity queues

* Wed Sep 22 2021 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.11.0-1
- New minor release
- Destination file report feature
- Improved handling of timeouts during polling tasks

* Wed Jun 02 2021 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.10.2-1
- Fix memory management in archiving tasks recovery

* Fri Feb 26 2021 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.10.1-1
- Bugfix for stage-only + multihop jobs

* Mon Dec 07 2020 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.10.0-1
- New minor release
- Brings the QoS daemon, which replaces the Bringonline daemon
- OIDC Tokens Integration
- Archive Monitoring feature
- Support for QoS transitions

* Mon Jul 27 2020 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.9.5-1
- Rework the message handling mechanism

* Wed Jun 10 2020 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.9.4-2
- Remove soci < 4.0.0 requirement

* Thu May 07 2020 Mihai Patrascoiu <mihai.patrascoiu@cern.ch> - 3.9.4-1
- Release focused on HTTP TPC tests
- Fix SRM pin leak after copy
- Change minimum validity time for a proxy from 60 to 90 minutes
- Compatibility with MySQL version 8 database

* Thu Nov 21 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.3-1
- Select multihop transfers as well when reaping stalled transfers

* Mon Sep 09 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.2-3
- fix curl build when libssh2 is installed on the system

* Mon Sep 02 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.2-2
- stop requiring CGSI-gsoap

* Thu Jul 25 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.2-1
- New bugfix release

* Tue Jun 18 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.1-1
- New bugfix release

* Thu May 23 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.0-2
- fix db upgrade script

* Wed May 08 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.0-1
- New Minor release
- Includes multiple database optimisations that improve the overall performance of FTS
- Avoid submitting multiple transfers to the same destination
- Fix optmizer running in parallel in 2 nodes

* Mon Apr 15 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.5-1
- New bugfix release

* Mon Mar 18 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.4-1
- New bugfix release

* Thu Feb 21 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.3-1
- New bugfix release

* Thu Jan 10 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.2-1
- New bugfix release

* Tue Oct 16 2018 Andrea Manzi <amanzi@cern.ch> - 3.8.1-1
- New bugfix release

* Mon Sep 24 2018 Andrea Manzi <amanzi@cern.ch> - 3.8.0-1
- New Minor release
- Initial support for Macaroons/Scitokens
- Multihop Scheduler merged into Standard Scheduling system
- AutoSessionReuse support

* Mon Jun 11 2018 Andrea Manzi <amanzi@cern.ch> - 3.7.10-1
- New upstream release

* Fri May 4 2018 Andrea Manzi <amanzi@cern.ch> - 3.7.9-1
- New upstream release

* Thu Feb 15 2018 Andrea Manzi <amanzi@cern.ch> - 3.7.8-1
- New upstream release

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org> - 3.6.10-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Wed Nov 29 2017 Igor Gnatenko <ignatenko@redhat.com> - 3.6.10-3
- Rebuild for protobuf 3.5

* Mon Nov 13 2017 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 3.6.10-2
- Rebuild for protobuf 3.4

* Thu Aug 03 2017 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.6.10-1
- New upstream release

* Wed Aug 02 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.6.8-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.6.8-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Mon Jul 03 2017 Jonathan Wakely <jwakely@redhat.com> - 3.6.8-3
- Rebuilt for Boost 1.64

* Wed Jun 14 2017 Orion Poplawski <orion@cora.nwra.com> - 3.6.8-2
- Rebuild for protobuf 3.3.1

* Tue Apr 18 2017 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.6.8-1
- New upstream release
- rpmlint

* Mon Feb 20 2017 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.6.3-1
- New upstream release

* Fri Feb 10 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.5.7-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Fri Jan 27 2017 Jonathan Wakely <jwakely@redhat.com> - 3.5.7-2
- Rebuilt for Boost 1.63

* Mon Nov 14 2016 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.5.7-1
- New upstream release

* Fri Aug 26 2016 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.4.3-5
- Rebuilt for new voms

* Tue Jul 19 2016 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.4.3-4
- https://fedoraproject.org/wiki/Changes/Automatic_Provides_for_Python_RPM_Packages

* Tue May 17 2016 Jonathan Wakely <jwakely@redhat.com> - 3.4.3-3
- Rebuilt for linker errors in boost (#1331983)

* Mon Apr 18 2016 Alejandro Alvarez <aalvarez@cern.ch> - 3.4.3-2
- Patch gsoap find module
- Patch url_copy for boost scoped ptr

* Mon Apr 18 2016 Alejandro Alvarez <aalvarez@cern.ch> - 3.4.3-1
- New upstream release
- systemd scripts

* Tue Feb 02 2016 Alejandro Alvarez <aalvarez@cern.ch> - 3.3.1-5
- Rebuilt for gsoap 2.8.28

* Fri Jan 15 2016 Jonathan Wakely <jwakely@redhat.com> - 3.3.1-4
- Rebuilt for Boost 1.60

* Tue Sep 22 2015 Alejandro Alvarez <aalvarez@cern.ch> - 3.3.1-3
- Supress -devel rpm

* Tue Sep 08 2015 Alejandro Alvarez <aalvarez@cern.ch> - 3.3.1-2
- Patch to disable Google Coredumper in non x86 architectures

* Fri Sep 04 2015 Alejandro Alvarez <aalvarez@cern.ch> - 3.3.1-1
- New upstream release
- fts-mysql integrated

* Thu Aug 27 2015 Jonathan Wakely <jwakely@redhat.com> - 3.2.32-6
- Rebuilt for Boost 1.59

* Wed Jul 29 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.2.32-5
- Rebuilt for https://fedoraproject.org/wiki/Changes/F23Boost159

* Wed Jul 22 2015 David Tardon <dtardon@redhat.com> - 3.2.32-4
- rebuild for Boost 1.58

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.2.32-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Sat May 02 2015 Kalev Lember <kalevlember@gmail.com> - 3.2.32-2
- Rebuilt for GCC 5 C++11 ABI change

* Thu Mar 05 2015  Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.2.31-1
- Update for new upstream release

* Thu Jan 29 2015 Petr Machata <pmachata@redhat.com> - 3.2.30-3
- Rebuild for boost 1.57.0
- Include <boost/scoped_ptr.hpp> in src/url-copy/main.cpp
  (fts-3.2.30-boost157.patch)

* Mon Jan 26 2015 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.2.30-2
- Rebuilt for gsoap 2.8.21

* Wed Nov 26 2014 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.2.30-1
- Update for new upstream release

* Thu Sep 04 2014 Orion Poplawski <orion@cora.nwra.com> - 3.2.26.2-4
- Rebuild for pugixml 1.4

* Sat Aug 16 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.2.26.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Wed Aug 13 2014 Michal Simon <michal.simon@cern.ch> - 3.2.26.2-2
- Update for new upstream releas

* Tue Feb 04 2014 Alejandro Alvarez <aalvarez@cern.ch> - 3.2.26-1
- introduced dist back in the release

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
- documentation files listed as %%doc
- trailing white-spaces have been removed

* Wed Apr 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.0-1
- First EPEL release
