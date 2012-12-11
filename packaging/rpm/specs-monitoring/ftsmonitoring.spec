%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Summary:	FTS3 Web Application for monitoring
Name:		ftsmonitoring
Version:	0.0.1
Release:	51%{?dist}
URL:		https://svnweb.cern.ch/trac/fts3
License:	Apache 2
Group:		Applications/Internet
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:	cx_Oracle
Requires:	Django
Requires:	httpd
Requires:	mod_wsgi
Requires:	MySQL-python
Requires:	python

Source0:	%{name}-%{version}.tar.gz

%description
FTS3 web Application for monitoring


%package ftsmonitoring
Summary: FTS3 Web Application for monitoring
Group: Applications/Internet

%description ftsmonitoring
FTS3 web Application for monitoring

%prep
%setup -c

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/fts3web
mkdir -p %{buildroot}%{_sysconfdir}/httpd/conf.d/
cp -dr --no-preserve=ownership %{_builddir}/%{name}-%{version}/src/fts3web %{buildroot}%{_datadir}/fts3web/
install -m 644 %{_builddir}/%{name}-%{version}/src/fts3web/httpd.conf.d/ftsmon.conf %{buildroot}%{_sysconfdir}/httpd/conf.d/

%clean
rm -rf %{buildroot}

%files ftsmonitoring
%defattr(-,root,root,-)
%{_datadir}/fts3web
%{_sysconfdir}/httpd/conf.d/

%changelog
* Mon Nov 05 2012 Alejandro Álvarez Ayllón <aalvarez@cern.ch>
 - First version of the spec file.
