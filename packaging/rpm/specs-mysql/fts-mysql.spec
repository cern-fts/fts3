%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name: fts-mysql
Version: 3.0.0
Release: 14%{?dist}
Summary: File Transfer Service V3 mysql plug-in
Group: Applications/Internet 
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export http://svnweb.cern.ch/guest/fts3/trunk
#  tar -czvf fts-mysql-0.0.1-60.tar.gz fts-mysql-00160
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  cmake
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  python-devel%{?_isa}
BuildRequires:  soci-mysql-devel%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
Requires(pre):  shadow-utils
Requires: fts-libs = %{version}-%{release}
Requires:  soci-mysql%{?_isa}

%description
The File Transfer Service V3 mysql plug-in

%package devel
Summary: Development files for File Transfer Service V3 mysql plug-in
Group: Applications/Internet
Requires: fts-mysql = %{version}-%{release}

%description devel
Development files for File Transfer Service V3 mysql plug-in

%prep
%setup -qc

%build
mkdir build
cd build
%cmake -DMYSQLBUILD=ON -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}


%install
cd build
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{python_sitearch}/fts

%post  -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/libfts_db_mysql.so.*
%doc %{_docdir}/fts3/mysql-schema.sql
%doc %{_docdir}/fts3/mysql-drop.sql
%doc %{_docdir}/fts3/mysql-truncate.sql
%doc README
%doc LICENSE

%files devel
%defattr(-,root,root,-)
%{_libdir}/libfts_db_mysql.so
%doc README
%doc LICENSE


%changelog
 * Tue Apr 30 2013 Michal Simon <michal.simon@cern.ch> - 3.0.0-14%{?dist}
  - First EPEL release
