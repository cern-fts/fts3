Name: fts-mysql
Version: 3.2.26.2
Release: 1%{?dist}
Summary: File Transfer Service V3 mysql plug-in
Group: Applications/Internet
License: ASL 2.0
URL: https://svnweb.cern.ch/trac/fts3/wiki
# The source for this package was pulled from upstream's vcs.  Use the
# following commands to generate the tarball:
#  svn export https://svn.cern.ch/reps/fts3/tags/EPEL_release_1_EPEL_TESTING fts3
#  tar -czvf fts-mysql-0.0.1-60.tar.gz fts-mysql-00160
Source0: https://grid-deployment.web.cern.ch/grid-deployment/dms/fts3/tar/%{name}-%{version}.tar.gz

%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  cmake
%else
BuildRequires:  cmake28
%endif
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  soci-mysql-devel%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
Requires(pre):  shadow-utils
Requires:       soci-mysql%{?_isa}

%description
The File Transfer Service V3 mysql plug-in

%prep
%setup -qc

%build
# Make sure the version in the spec file and the version used
# for building matches
fts_cmake_ver=`sed -n 's/^set(VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9]\+\).*/\2/p' CMakeLists.txt | paste -sd '.'`
fts_spec_ver=`expr "%{version}" : '\([0-9]*\\.[0-9]*\\.[0-9]*\)'`
if [ "$fts_cmake_ver" != "$fts_spec_ver" ]; then
    echo "The version in the spec file does not match the CMakeLists.txt version!"
    echo "$fts_cmake_ver != %{version}"
    exit 1
fi

# Build
mkdir build
cd build
%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
%cmake -DMYSQLBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%else
%cmake28 -DMYSQLBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%endif
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

%post   -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/libfts_db_mysql.so.*
%{_datadir}/fts-mysql
%doc README
%doc LICENSE

%changelog
* Fri Aug 15 2014 Michal Simon <michal.simon@cern.ch> - 3.2.26.2-1
  - Update for new upstream release
* Mon May 12 2014 Michal Simon <michal.simon@cern.ch> - 3.2.26-1
  - Update for new upstream release
* Wed Aug 07 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-2
  - no longer linking explicitly to boost libraries with '-mt' sufix 
  - sql scripts have been moved to datadir
* Mon Jul 29 2013 Michal Simon <michal.simon@cern.ch> - 3.1.0-1
  - First EPEL release
  - devel package removed
* Fri Jul 02 2013 Michail Salichos <michail.salichos@cern.ch> - 3.0.3-14
  - mysql queries optimization
