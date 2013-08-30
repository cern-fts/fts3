Summary: FTS3 Web Application for monitoring
Name: fts-monitoring
Version: 3.1.5
Release: 2%{?dist}
URL: https://svnweb.cern.ch/trac/fts3
License: ASL 2.0
Group: Applications/Internet
BuildArch: noarch

#Requires: cx_Oracle%{?_isa}
Requires: MySQL-python%{?_isa}
Requires: Django >= 1.3.7
Requires: httpd%{?_isa}
Requires: mod_wsgi%{?_isa}
Requires: python%{?_isa}
Requires: python-decorator
Requires: python-matplotlib%{?_isa}

Source0: https://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/tar/%{name}-%{version}.tar.gz

%description
FTS v3 web application for monitoring,
it gives a detailed view of the current state of FTS
including the queue with submitted transfer-jobs,
the active, failed and finished transfers, as well
as some statistics (e.g. success rate)


%prep
%setup -qc

%build
mkdir build

%install
shopt -s extglob
mkdir -p %{buildroot}%{_datadir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/httpd/conf.d/ 
cp -r -p !(etc|httpd.conf.d) %{buildroot}%{_datadir}/fts3web/
cp -r -p etc/fts3web         %{buildroot}%{_sysconfdir}
install -m 644 httpd.conf.d/ftsmon.conf           %{buildroot}%{_sysconfdir}/httpd/conf.d/ 

%files
%{_datadir}/fts3web
%config(noreplace) %{_sysconfdir}/httpd/conf.d/ftsmon.conf
%config(noreplace) %dir %{_sysconfdir}/fts3web/
%config(noreplace) %attr(640, root, apache) %{_sysconfdir}/fts3web/fts3web.ini
%doc LICENSE

%changelog
 * Wed Aug 28 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-1
  - replacing '--no-preserve=ownership'
  - python macros have been removed
  - comments regarding svn have been removed
  - '%{_builddir}/%{name}-%{version}/' prefix is not used anymore
  - more detailed description
 * Tue Apr 30 2013 Michal Simon <michal.simon@cern.ch> - 3.1.0-1
  - First EPEL release
