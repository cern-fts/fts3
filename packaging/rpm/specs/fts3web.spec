Summary:	FTS3 Web Application
Name:		fts3web
Version:	0.0.1
Release:	1
URL:		https://svnweb.cern.ch/trac/fts3
License:	Apache 2
Group:		Applications/Internet
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

Requires:	cx_Oracle
Requires:	Django
Requires:	httpd
Requires:	mod_wsgi
Requires:	MySQL-python
Requires:	python

Source0:	%{name}-%{version}.tar.gz
BuildArch:	noarch

%description
FTS3 Web Application. Includes simple monitoring.

%prep
%setup -c

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p ${RPM_BUILD_ROOT}/%{_datadir}/%{name}
mkdir -p ${RPM_BUILD_ROOT}/%{_sysconfdir}/httpd/conf.d/
cp -dr --no-preserve=ownership src/fts3web ${RPM_BUILD_ROOT}/%{_datadir}/%{name}/
install -m 644 src/fts3web/httpd.conf.d/ftsmon.conf ${RPM_BUILD_ROOT}/%{_sysconfdir}/httpd/conf.d/

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%{_datadir}/%{name}
%{_sysconfdir}/httpd/conf.d/

%changelog
* Mon Nov 05 2012 Alejandro Álvarez Ayllón <aalvarez@cern.ch>
- First version of the spec file.
