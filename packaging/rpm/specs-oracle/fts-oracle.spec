%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name: fts-oracle
Version: 0.0.1 
Release: 51%{?dist}
Summary: File Transfer Service V3 oracle plug-in
Group: Applications/Internet
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki 
Source0: https://svnweb.cern.ch/trac/fts3/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cmake
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  python-devel%{?_isa}
BuildRequires:  oracle-instantclient-devel%{?_isa}
Requires(pre):  shadow-utils
Requires: fts-libs = %{version}-%{release}
Requires:  oracle-instantclient-basic%{?_isa}

%description
The File Transfer Service V3 oracle plug-in

%description
FTS V3 oracle plug-ins

%prep
%setup -qc

%build
mkdir build
cd build
%cmake -DORACLEBUILD=ON -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}


%install
cd build
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{python_sitearch}/fts

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/libfts_db_oracle.so*
%doc %{_docdir}/fts3/oracle-schema.sql
%doc %{_docdir}/fts3/oracle-drop.sql

%changelog
 * Wed Aug 8 2012 Steve Traylen <steve.traylen@cern.ch> - 0.0.0-51%{?dist}
  - A bit like a fedora package
