%global _hardened_build 1
%global selinux_policyver %(sed -e 's,.*selinux-policy-\\([^/]*\\)/.*,\\1,' /usr/share/selinux/devel/policyhelp || echo 0.0.0)
%global selinux_variants mls targeted

%if %{?fedora}%{!?fedora:0} >= 17 || %{?rhel}%{!?rhel:0} >= 7
%global systemd 1
%else
%global systemd 0
%endif

Name:       fts
Version:    3.9.2
Release:    2%{?dist}
Summary:    File Transfer Service V3
Group:      System Environment/Daemons
License:    ASL 2.0
URL:        http://fts3-service.web.cern.ch/
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  git clone https://gitlab.cern.ch/fts/fts3.git -b master --depth=1 fts-3.8.0
#  cd fts-3.8.0
#  git checkout v3.8.0
#  cd ..
#  tar --exclude-vcs -vczf fts-3.8.0.tar.gz fts-3.8.0
Source0: %{name}-%{version}.tar.gz

%if 0%{?el5}
BuildRequires:  activemq-cpp-library
%else
BuildRequires:  activemq-cpp-devel
%endif
%if %{?fedora}%{!?fedora:0} >= 21 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  boost-devel
%else
BuildRequires:  boost148-devel
%endif

BuildRequires:  cajun-jsonapi-devel
BuildRequires:  cmake
BuildRequires:  cmake3
BuildRequires:  libdirq-devel
BuildRequires:  doxygen
%if 0%{?el5}
BuildRequires:  e2fsprogs-devel
%else
BuildRequires:  libuuid-devel
%endif
BuildRequires:  gfal2-devel >= 2.14.2
BuildRequires:  glib2-devel
BuildRequires:  globus-gsi-credential-devel
BuildRequires:  gridsite-devel
BuildRequires:  openldap-devel
BuildRequires:  protobuf-devel
BuildRequires:  pugixml-devel
BuildRequires:  voms-devel
BuildRequires:  checkpolicy, selinux-policy-devel, selinux-policy-doc
%if %systemd
BuildRequires:	systemd
%endif
%if %{?fedora}%{!?fedora:0} >= 21 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  cppzmq-devel
%else
BuildRequires:  zeromq-devel
%endif

# Required for some unit tests
BuildRequires:  gfal2-plugin-mock
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
Requires: gfal2%{?_isa} >= 2.14.2
Requires: gfal2-plugin-gridftp%{?_isa} >= 2.14.2
Requires: gfal2-plugin-http%{?_isa} >= 2.14.2
Requires: gfal2-plugin-srm%{?_isa} >= 2.14.2
#Requires: gfal2-plugin-xrootd%{?_isa}
Requires: gridsite >= 1.7.25

%if %systemd
Requires(post):		systemd
Requires(preun):	systemd
Requires(postun):	systemd
%else
Requires(post):     chkconfig
Requires(preun):    chkconfig
Requires(preun):    initscripts
Requires(postun):   initscripts
%endif

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
Group:      System Environment/Libraries

Obsoletes:  fts-mysql-debuginfo < %{version}

%description libs
FTS common libraries used across the client and
server. This includes among others: configuration
parsing, logging and error-handling utilities, as
well as, common definitions and interfaces

%package infosys
Summary:    File Transfer Service version 3 infosys integration
Group:      System Environment/Daemons
Requires:   bdii
Requires:   fts-server%{?_isa} = %{version}-%{release}
Requires:   glue-schema

%if %systemd
Requires(post):		systemd
Requires(preun):	systemd
Requires(postun):	systemd
%else
Requires(post):		chkconfig
Requires(preun):	chkconfig
Requires(preun):	initscripts
Requires(postun):	initscripts
%endif

%description infosys
FTS infosys integration

%package msg
Summary:    File Transfer Service version 3 messaging integration
Group:      System Environment/Daemons
Requires:   fts-server%{?_isa} = %{version}-%{release}

%description msg
FTS messaging integration

%package client
Summary:    File Transfer Service version 3 client
Group:      Applications/Internet
Requires:   fts-libs%{?_isa} = %{version}-%{release}

%description client
A set of command line tools for submitting, querying
and canceling transfer-jobs to the FTS service. Additionally,
there is a CLI that can be used for configuration and
administering purposes.

%package server-selinux
Summary:    SELinux support for fts-server
Group:      Applications/Internet
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
Group:      Applications/Internet

BuildRequires:  soci-mysql-devel
Requires:   soci-mysql%{?_isa}
Requires:   fts-server%{?_isa}

%description mysql
The File Transfer Service V3 mysql plug-in

%prep
%setup -q

%build
#build curl
./src/cli/buildcurl.sh
# Make sure the version in the spec file and the version used
# for building matches
fts_cmake_ver=`sed -n 's/^set(VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9]\+\).*/\2/p' CMakeLists.txt | paste -sd '.'`
fts_spec_ver=`expr "%{version}" : '\([0-9]*\\.[0-9]*\\.[0-9]*\)'`
if [ "$fts_cmake_ver" != "$fts_spec_ver" ]; then
    echo "The version in the spec file does not match the CMakeLists.txt version!"
    echo "$fts_cmake_ver != %{version}"
    exit 1
fi

# Build
mkdir build
cd build
%cmake -DSERVERBUILD=ON -DMYSQLBUILD=ON -DCLIENTBUILD=ON \
    -DTESTBUILD=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX='' \
%if %systemd
    -DSYSTEMD_INSTALL_DIR=%{_unitdir} \
%endif
    ..

make %{?_smp_mflags}

cd -

# SELinux
cd selinux
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

cd -

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
%if %systemd
/bin/systemctl daemon-reload > /dev/null 2>&1 || :
%else
/sbin/chkconfig --add fts-server
/sbin/chkconfig --add fts-bringonline
/sbin/chkconfig --add fts-records-cleaner
%endif
exit 0

%preun server
if [ $1 -eq 0 ] ; then
%if %systemd
    /bin/systemctl stop fts-server.service > /dev/null 2>&1 || :
    /bin/systemctl stop fts-bringonline.service > /dev/null 2>&1 || :
    /bin/systemctl stop fts-records-cleaner.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-server.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-bringonline.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-records-cleaner.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-server stop >/dev/null 2>&1
    /sbin/service fts-bringonline stop >/dev/null 2>&1
    /sbin/service fts-records-cleaner stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-server
    /sbin/chkconfig --del fts-bringonline
    /sbin/chkconfig --del fts-records-cleaner
%endif
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
%if %systemd
    /bin/systemctl try-restart fts-server.service > /dev/null 2>&1 || :
    /bin/systemctl try-restart fts-bringonline.service > /dev/null 2>&1 || :
    /bin/systemctl try-restart fts-records-cleaner.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-server condrestart >/dev/null 2>&1 || :
    /sbin/service fts-bringonline condrestart >/dev/null 2>&1 || :
    /sbin/service fts-records-cleaner condrestart >/dev/null 2>&1 || :
%endif
fi
exit 0

# Infosys scriptlets
%post infosys
%if %systemd
    /bin/systemctl daemon-reload > /dev/null 2>&1 || :
%else
    /sbin/chkconfig --add fts-info-publisher
    /sbin/chkconfig --add fts-bdii-cache-updater
%endif
exit 0

%preun infosys
if [ $1 -eq 0 ] ; then
%if %systemd
    /bin/systemctl stop fts-info-publisher.service > /dev/null 2>&1 || :
    /bin/systemctl stop fts-bdii-cache-updater.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-info-publisher.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-bdii-cache-updater.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-info-publisher stop >/dev/null 2>&1
    /sbin/service fts-bdii-cache-updater stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-info-publisher
    /sbin/chkconfig --del fts-bdii-cache-updater
%endif
fi
exit 0

%postun infosys
if [ "$1" -ge "1" ] ; then
%if %systemd
    /bin/systemctl try-restart fts-info-publisher.service > /dev/null 2>&1 || :
    /bin/systemctl stop fts-myosg-updater.service > /dev/null 2>&1 || :
    /bin/systemctl try-restart fts-bdii-cache-updater.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-info-publisher condrestart >/dev/null 2>&1 || :
    /sbin/service fts-myosg-updater stop >/dev/null 2>&1 || :
    /sbin/service fts-bdii-cache-updater condrestart >/dev/null 2>&1 || :
%endif
fi
exit 0

# Messaging scriptlets
%post msg
%if %systemd
    /bin/systemctl daemon-reload > /dev/null 2>&1 || :
%else
    /sbin/chkconfig --add fts-msg-bulk
%endif
exit 0

%preun msg
if [ $1 -eq 0 ] ; then
%if %systemd
    /bin/systemctl stop fts-msg-bulk.service > /dev/null 2>&1 || :
    /bin/systemctl --no-reload disable fts-msg-bulk.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-msg-bulk stop >/dev/null 2>&1
    /sbin/chkconfig --del fts-msg-bulk
%endif
fi
exit 0

%postun msg
if [ "$1" -ge "1" ] ; then
%if %systemd
    /bin/systemctl try-restart fts-msg-bulk.service > /dev/null 2>&1 || :
%else
    /sbin/service fts-msg-bulk condrestart >/dev/null 2>&1 || :
%endif
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
%{_sbindir}/fts_db_rotate

%dir %attr(0755,root,root) %{_datadir}/fts/
%{_datadir}/fts/fts-database-upgrade.py*

%if %systemd
%attr(0644,root,root) %{_unitdir}/fts-server.service
%attr(0644,root,root) %{_unitdir}/fts-bringonline.service
%attr(0644,root,root) %{_unitdir}/fts-records-cleaner.service
%else
%attr(0755,root,root) %{_initddir}/fts-server
%attr(0755,root,root) %{_initddir}/fts-bringonline
%attr(0755,root,root) %{_initddir}/fts-records-cleaner
%endif

%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-records-cleaner
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts3config
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/sysconfig/fts-server
%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/sysconfig/fts-bringonline
%config(noreplace) %{_sysconfdir}/logrotate.d/fts-server
%{_mandir}/man8/fts_bringonline.8.gz
%{_mandir}/man8/fts_db_cleaner.8.gz
%{_mandir}/man8/fts_server.8.gz
%{_mandir}/man8/fts_url_copy.8.gz

%files infosys
%{_sbindir}/fts_bdii_cache_updater
%{_sbindir}/fts_info_publisher
%config(noreplace) %attr(0644,fts3,root) %{_var}/lib/fts3/bdii_cache.xml

%if %systemd
%attr(0644,root,root) %{_unitdir}/fts-info-publisher.service
%attr(0644,root,root) %{_unitdir}/fts-bdii-cache-updater.service
%else
%attr(0755,root,root) %{_initddir}/fts-info-publisher
%attr(0755,root,root) %{_initddir}/fts-bdii-cache-updater
%endif

%attr(0755,root,root) %{_sysconfdir}/cron.hourly/fts-info-publisher
%attr(0755,root,root) %{_sysconfdir}/cron.daily/fts-bdii-cache-updater
%{_mandir}/man8/fts_bdii_cache_updater.8.gz
%{_mandir}/man8/fts_info_publisher.8.gz

%files msg
%{_sbindir}/fts_msg_bulk

%if %systemd
%attr(0644,root,root) %{_unitdir}/fts-msg-bulk.service
%else
%attr(0755,root,root) %{_initddir}/fts-msg-bulk
%endif

%config(noreplace) %attr(0644,fts3,root) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%{_mandir}/man8/fts_msg_bulk.8.gz

%files client
%{_bindir}/fts-*
%{_mandir}/man1/fts*

%files libs
%{_libdir}/libfts_common.so*
%{_libdir}/libfts_config.so*
%{_libdir}/libfts_infosys.so*
%{_libdir}/libfts_db_generic.so*
%{_libdir}/libfts_msg_ifce.so*
%{_libdir}/libfts_proxy.so*
%{_libdir}/libfts_server_lib.so*
%{_libdir}/libfts_cli_common.so*
%{_libdir}/libfts_msg_bus.so*
%{_libdir}/libfts_url_copy.so*
%doc README.md
%doc LICENSE

%files server-selinux
%defattr(-,root,root,0755)
%doc selinux/*
%{_datadir}/selinux/*/fts.pp

%files mysql
%{_libdir}/libfts_db_mysql.so.*
%{_datadir}/fts-mysql

%check
export LD_LIBRARY_PATH=%{buildroot}%{_libdir}:./build/test/unit
./build/test/unit/unit --log_level=all --report_level=detailed

%changelog
* Mon Sep 02 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.2-2
- stop requiring CGSI-gsoap

* Thu Jul 25 2019 Andrea Manzi <amanzi@cern.ch> - 3.9.2-1
- New bugfix releas

* Tue Jun 18 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.1-1
- New bugfix release

* Thu May 23 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.0-2
- fix db upgrade script

* Tue May 08 2019 Edward Karavakis <edward.karavakis@cern.ch> - 3.9.0-1
- New Minor release
- Includes multiple database optimisations that improve the overall performance of FTS
- Avoid submitting multiple transfers to the same destination
- Fix optmizer running in parallel in 2 nodes
* Mon Apr 15 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.5-1
- New bugfix release

* Mon Mar 28 2019 Andrea Manzi <amanzi@cern.ch> - 3.8.4-1
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
  - documentation files listed as %doc
  - trailing white-spaces have been removed
* Wed Apr 24 2013 Michal Simon <michal.simon@cern.ch> - 3.0.0-1
  - First EPEL release

