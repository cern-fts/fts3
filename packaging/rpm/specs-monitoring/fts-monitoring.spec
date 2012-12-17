%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Summary: FTS3 Web Application for monitoring
Name: fts-monitoring
Version: 0.0.1
Release: 52%{?dist}
URL: https://svnweb.cern.ch/trac/fts3
License: ASL 2.0
Group: Applications/Internet
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch: noarch

Requires: cx_Oracle
Requires: Django
Requires: httpd
Requires: mod_wsgi
Requires: MySQL-python
Requires: python
#Requires: python-matplotlib

Source0: %{name}-%{version}.tar.gz

%description
FTS v3 web application for monitoring


%prep
%setup -qc

%build
mkdir build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/fts3web
mkdir -p %{buildroot}%{_sysconfdir}/httpd/conf.d/
cp -r --no-preserve=ownership %{_builddir}/%{name}-%{version}/* %{buildroot}%{_datadir}/fts3web/
install -m 644 %{_builddir}/%{name}-%{version}/httpd.conf.d/ftsmon.conf %{buildroot}%{_sysconfdir}/httpd/conf.d/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_datadir}/fts3web
%config(noreplace) %{_sysconfdir}/httpd/conf.d/

%changelog
 * Wed Aug 8 2012 Steve Traylen <steve.traylen@cern.ch> - 0.0.1-52%{?dist}
  - A bit like a fedora package
