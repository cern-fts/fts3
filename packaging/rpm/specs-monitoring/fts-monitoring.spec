%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Summary: FTS3 Web Application for monitoring
Name: fts-monitoring
Version: 3.1.3
Release: 1%{?dist}
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
Requires: python-matplotlib%{?_isa}

# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export https://svn.cern.ch/reps/fts3/tags/EPEL_release_1_EPEL_TESTING fts3
#  tar -czvf fts-monitoring-0.0.1.tar.gz --directory=fts3 .
Source0: https://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/tar/%{name}-%{version}.tar.gz

%description
FTS v3 web application for monitoring


%prep
%setup -qc

%build
mkdir build

%install
shopt -s extglob
mkdir -p %{buildroot}%{_datadir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/httpd/conf.d/ 
cp -r --no-preserve=ownership %{_builddir}/%{name}-%{version}/!(etc|httpd.conf.d) %{buildroot}%{_datadir}/fts3web/
cp -r --no-preserve=ownership %{_builddir}/%{name}-%{version}/etc/fts3web         %{buildroot}%{_sysconfdir}
install -m 644 %{_builddir}/%{name}-%{version}/httpd.conf.d/ftsmon.conf           %{buildroot}%{_sysconfdir}/httpd/conf.d/ 

%files
%{_datadir}/fts3web
%config(noreplace) %{_sysconfdir}/httpd/conf.d/ftsmon.conf
%config(noreplace) %dir %{_sysconfdir}/fts3web/
%config(noreplace) %attr(640, root, apache) %{_sysconfdir}/fts3web/fts3web.ini
%doc LICENSE

%changelog
 * Tue Apr 30 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-2
  - First EPEL release
