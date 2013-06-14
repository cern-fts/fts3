%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Summary: FTS3 Web Application for monitoring
Name: fts-monitoring
Version: 3.0.2
Release: 3%{?dist}
URL: https://svnweb.cern.ch/trac/fts3
License: ASL 2.0
Group: Applications/Internet
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
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
#  svn export http://svnweb.cern.ch/guest/fts3/trunk fts3
#  tar -czvf fts-monitoring-0.0.1.tar.gz --directory=fts3 .
Source0: %{name}-%{version}.tar.gz

%description
FTS v3 web application for monitoring


%prep
%setup -qc

%build
mkdir build

%install
shopt -s extglob
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/fts3web/
mkdir -p %{buildroot}%{_sysconfdir}/httpd/conf.d/
cp -r --no-preserve=ownership %{_builddir}/%{name}-%{version}/!(etc|httpd.conf.d) %{buildroot}%{_datadir}/fts3web/
cp -r --no-preserve=ownership %{_builddir}/%{name}-%{version}/etc/fts3web         %{buildroot}%{_sysconfdir}
install -m 644 %{_builddir}/%{name}-%{version}/httpd.conf.d/ftsmon.conf           %{buildroot}%{_sysconfdir}/httpd/conf.d/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_datadir}/fts3web
%config(noreplace) %{_sysconfdir}/httpd/conf.d/ftsmon.conf
%config(noreplace) %{_sysconfdir}/fts3web/

%changelog
 * Tue Apr 30 2013 Michal Simon <michal.simon@cern.ch> - 3.0.2-2%{?dist}
  - First EPEL release
