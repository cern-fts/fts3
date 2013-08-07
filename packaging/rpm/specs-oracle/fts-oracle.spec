Name: fts-oracle
Version: 3.1.1
Release: 1%{?dist}
Summary: File Transfer Service V3 oracle plug-in
Group: Applications/Internet
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export http://svnweb.cern.ch/guest/fts3/trunk
#  tar -czvf fts-oracle-0.0.1-60.tar.gz fts-oracle-00160
Source0: %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  oracle-instantclient-devel%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
Requires(pre):  shadow-utils
Requires:  oracle-instantclient-basic%{?_isa}

%description
The File Transfer Service V3 oracle plug-in

%prep
%setup -qc

%build
mkdir build
cd build
%cmake -DORACLEBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

%post   -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/libfts_db_oracle.so.*
%doc %{_docdir}/fts3/oracle-schema.sql
%doc %{_docdir}/fts3/oracle-drop.sql
%doc README
%doc LICENSE


%changelog
* Wed Aug 07 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-1
  - GenericDbIfce.h includes new blacklisting API
  - no longer linking explicitly to boost libraries with '-mt' sufix 
* Tue Aug 06 2013 Michal Simon <michal.simon@cern.ch> - 3.1.0-1
 - First CERN koji release
 - devel package removed
* Fri Jul 02 2013 Michail Salichos <michail.salichos@cern.ch> - 3.0.3-14
 - oracle queries optimization
