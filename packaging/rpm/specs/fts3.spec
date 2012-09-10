Name:           fts3
Version:        0.0.1 
Release:        26%{?dist}
Summary:        File Transfer Service version 3

Group:          System Environment/Daemons 
License:        Apache Software License
URL:            https://svnweb.cern.ch/trac/fts3/wiki 
Source0:        http://%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cmake
BuildRequires:  apr-devel%{?_isa}
BuildRequires:  apr-util-devel%{?_isa}
BuildRequires:  activemq-cpp-devel%{?_isa}
BuildRequires:  gsoap-devel%{?_isa}
BuildRequires:  doxygen%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  globus-gsi-credential-devel%{?_isa}
BuildRequires:  CGSI-gSOAP-devel%{?_isa}
BuildRequires:  is-interface-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  gridsite-devel%{?_isa}
BuildRequires:  gfal2-devel%{?_isa}
BuildRequires:  oracle-instantclient-devel%{?_isa}
BuildRequires:  voms-devel%{?_isa}
Requires(pre):  shadow-utils

%{?filter_setup:
%filter_provides_in /usr/lib64/oracle/11.2.0.3.0/client/lib64/.*\.so$
%filter_requires_in /usr/lib64/oracle/11.2.0.3.0/client/lib64/.*\.so$
%filter_setup
}

%description
The File Transfer Service V3

%package devel
Summary: File Transfer Service V3
Group: Applications/Internet
Requires: %{name}%{?_isa} = %{version}-%{release}
BuildRequires:  cmake
BuildRequires:  apr-devel%{?_isa}
BuildRequires:  apr-util-devel%{?_isa}
BuildRequires:  activemq-cpp-devel%{?_isa}
BuildRequires:  gsoap-devel%{?_isa}
BuildRequires:  doxygen%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  globus-gsi-credential-devel%{?_isa}
BuildRequires:  CGSI-gSOAP-devel%{?_isa}
BuildRequires:  is-interface-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  gridsite-devel%{?_isa}
BuildRequires:  gfal2-devel%{?_isa}
BuildRequires:  oracle-instantclient-devel%{?_isa}
BuildRequires:  voms-devel%{?_isa}

%description devel
Development files for File Transfer Service V3


%package server
Summary: File Transfer Service version 3 server
Group: System Environment/Daemons
Requires: fts3-libs = %{version}-%{release}
Requires: gfal2-plugin-gridftp
Requires: gfal2-plugin-srm

%package libs
Summary: File Transfer Service version 3 libs
Group: System Environment/Libraries

%package client
Summary: File Transfer Service version 3 client
Group: Applications/Internet
Requires: fts3-libs = %{version}-%{release}

%description server
FTS3 server is a service which accepts transfer jobs, quering their status, etc

%description libs
FTS3 common libraries used across the client and server

%description client
FTS3 client CLI tool for submitting transfers, check status, configure server, etc


%prep
%setup -qc


%build
mkdir build
cd build
%cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}


%install
cd build
rm -rf $RPM_BUILD_ROOT
if [ -f /dev/shm/fts3mq ]; then rm -rf /dev/shm/fts3mq; fi 
mkdir -p %{buildroot}%{_var}/lib/fts3
mkdir -p %{buildroot}%{_var}/log/fts3
make install DESTDIR=$RPM_BUILD_ROOT


%pre server
getent group fts3 >/dev/null || groupadd -r fts3
getent passwd fts3 >/dev/null || \
    useradd -r -g fts3 -d /var/log/fts3 -s /sbin/nologin \
    -c "fts3 urlcopy user" fts3
exit 0

%post server
/sbin/chkconfig --add fts3-server
/sbin/chkconfig --add fts3-msg-bulk
/sbin/chkconfig --add fts3-msg-cron
exit 0

%preun server
if [ $1 -eq 0 ] ; then
    /sbin/service fts3-server stop >/dev/null 2>&1
    /sbin/chkconfig --del fts3-server
    /sbin/service fts3-msg-bulk stop >/dev/null 2>&1
    /sbin/chkconfig --del fts3-msg-bulk
    /sbin/service fts3-msg-cron stop >/dev/null 2>&1
    /sbin/chkconfig --del fts3-msg-cron
fi
exit 0

%postun server
if [ "$1" -ge "1" ] ; then
    /sbin/service fts3-server condrestart >/dev/null 2>&1 || :
    /sbin/service fts3-msg-bulk condrestart >/dev/null 2>&1 || :
    /sbin/service fts3-msg-cron condrestart >/dev/null 2>&1 || :
fi
exit 0


%clean
if [ -f /dev/shm/fts3mq ]; then rm -rf /dev/shm/fts3mq; fi 
rm -rf $RPM_BUILD_ROOT



%files server
%defattr(-,root,root,-)
%dir %{_sysconfdir}/fts3
%dir %attr(0755,fts3,root) %{_var}/lib/fts3
%dir %attr(0755,fts3,root) %{_var}/log/fts3
%{_sbindir}/fts3_msg_cron
%{_sbindir}/fts3_msg_bulk
%{_sbindir}/fts3_server
%{_sbindir}/fts3_url_copy
%{_initddir}/fts3-msg-bulk
%{_initddir}/fts3-server
%{_initddir}/fts3-msg-cron
%{_sysconfdir}/logrotate.d/fts3-msg-cron
%{_sysconfdir}/logrotate.d/fts3-msg-bulk
%{_sysconfdir}/logrotate.d/fts3-server
%config(noreplace) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%config(noreplace) %{_sysconfdir}/fts3/fts3config
%{_mandir}/man8/fts3_server.8.gz
%doc %{_docdir}/fts3/oracle-drop.sql
%doc %{_docdir}/fts3/oracle-schema.sql
%doc %{_docdir}/fts3/fts_purge_pack.sql
%doc %{_docdir}/fts3/fts_purge_body_pack.sql
%doc %{_docdir}/fts3/fts_history_pack.sql
%doc %{_docdir}/fts3/fts_history_body_pack.sql

%files client
%defattr(-,root,root,-)
%{_bindir}/fts3-config-set
%{_bindir}/fts3-config-get
%{_bindir}/fts3-config-del
%{_bindir}/fts3-debug-set
%{_bindir}/fts3-transfer-getroles
%{_bindir}/fts3-transfer-list
%{_bindir}/fts3-transfer-listvomanagers
%{_bindir}/fts3-transfer-status
%{_bindir}/fts3-transfer-submit
%{_mandir}/man1/fts3-config-get.1*
%{_mandir}/man1/fts3-config-set.1*
%{_mandir}/man1/fts3-transfer-cancel.1*
%{_mandir}/man1/fts3-transfer-getroles.1*
%{_mandir}/man1/fts3-transfer-list.1*
%{_mandir}/man1/fts3-transfer-listvomanagers.1*
%{_mandir}/man1/fts3-transfer-status.1*
%{_mandir}/man1/fts3-transfer-submit.1*

%files libs
%defattr(-,root,root,-)
%{_libdir}/libfts3_common.so
%{_libdir}/libfts3_config.so
%{_libdir}/libfts3_db_generic.so
%{_libdir}/libfts3_db_oracle.so
%{_libdir}/libfts3_msg_ifce.so
%{_libdir}/libfts3_proxy.so
%{_libdir}/libfts3_server_gsoap_transfer.so
%{_libdir}/libfts3_server_lib.so
%{_libdir}/libfts3_cli_common.so
%{_libdir}/libfts3_ws_ifce_client.so
%{_libdir}/libfts3_ws_ifce_server.so
%{_libdir}/libfts3_delegation_api_simple.so
%{_libdir}/libfts3_delegation_api_cpp.so
%doc

%changelog
 * Wed Aug 8 2012 Steve Traylen <steve.traylen@cern.ch> - 0.0.0-11
  - A bit like a fedora package
