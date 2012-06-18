Name:           fts3
Version:        0.0.1 
Release:        1%{?dist}
Summary:        FTS3

Group:          System Environment/Daemons 
License:        ASL 2.0
URL:            https://svnweb.cern.ch/trac/fts3/wiki 
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
AutoReqProv: yes

BuildRequires:  cmake
BuildRequires:  apr-devel
BuildRequires:  apr-util-devel
BuildRequires:  activemq-cpp-devel
BuildRequires:  gsoap-devel
BuildRequires:  doxygen
BuildRequires:  libuuid-devel
BuildRequires:  boost-devel
BuildRequires:  globus-gsi-credential-devel
BuildRequires:  CGSI-gSOAP-devel
BuildRequires:  is-interface-devel
BuildRequires:  glib2-devel
BuildRequires:  gridsite-devel
BuildRequires:  gfal2-devel
BuildRequires:  oracle-instantclient-devel

%description
The File Transfer Service V3

%package -n fts3-server
Summary:        FTS3 server
Group:          System Environment/Daemons
Requires:       fts3-common

%package -n fts3-common
Summary:        FTS3 common
Group:          System Environment/Libraries

%package -n fts3-client
Summary:        FTS3 server
Group:          System Environment/Daemons
Requires:       fts3-common

%description -n fts3-server
FTS3 server

%description -n fts3-common
FTS3 common

%description -n fts3-client
FTS3 client


%prep
%setup -qc


%build
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX='' ..
make VERBOSE=1


%install
cd build
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files -n fts3-server
%defattr(-,root,root,-)
%dir %{_sysconfdir}/fts3
%{_sbindir}/fts3_msg_cron
%{_sbindir}/fts3_msg_bulk
%{_sbindir}/fts3_server
%{_sbindir}/fts3_url_copy
%{_initddir}/fts3-msg-bulk.initd
%{_initddir}/fts3-server.initd
%{_initddir}/fts3-msg-cron.initd
%{_sysconfdir}/logrotate.d/fts3-msg-cron.logrotate
%{_sysconfdir}/logrotate.d/fts3-server.logrotate
%config(noreplace) %{_sysconfdir}/fts3/fts-msg-monitoring.conf
%config(noreplace) %{_sysconfdir}/fts3/fts3config

%files -n fts3-client
%defattr(-,root,root,-)
%{_bindir}/fts3-transfer-getroles
%{_bindir}/fts3-transfer-list
%{_bindir}/fts3-transfer-listvomanagers
%{_bindir}/fts3-transfer-status
%{_bindir}/fts3-transfer-submit

%files -n fts3-common
%defattr(-,root,root,-)
%{_libdir}/libfts3_common.so
%{_libdir}/libfts3_config.so
%{_libdir}/libfts3_db_generic.so
%{_libdir}/libfts3_db_oracle.so
%{_libdir}/libfts3_msg_ifce.so
%{_libdir}/libfts3_proxy.so
%{_libdir}/libfts3_server_gsoap_transfer.so
%{_libdir}/libfts3_server_lib.so
%doc



%changelog
