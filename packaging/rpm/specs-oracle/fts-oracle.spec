Name: fts-oracle
Version: 3.2.27
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

%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  cmake
%else
BuildRequires:  cmake28
%endif
BuildRequires:  boost-devel%{?_isa}
BuildRequires:  glib2-devel%{?_isa}
BuildRequires:  oracle-instantclient-devel%{?_isa}
BuildRequires:  libuuid-devel%{?_isa}
Requires(pre):  shadow-utils
BuildRequires:  soci-oracle-devel

Requires:  soci-oracle%{?_isa}

AutoReq: no
AutoProv: yes

%description
The File Transfer Service V3 oracle plug-in

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
%cmake -DORACLEBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%else
%cmake28 -DORACLEBUILD=ON -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX='' ..
%endif
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

%post   -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_libdir}/libfts_db_oracle.so.*
%{_datadir}/fts-oracle
%doc README
%doc LICENSE


%changelog
* Wed Aug 07 2013 Michal Simon <michal.simon@cern.ch> - 3.1.1-2
  - no longer linking explicitly to boost libraries with '-mt' sufix 
  - sql scripts have been moved to datadir
* Tue Aug 06 2013 Michal Simon <michal.simon@cern.ch> - 3.1.0-1
 - First CERN koji release
 - devel package removed
* Fri Jul 02 2013 Michail Salichos <michail.salichos@cern.ch> - 3.0.3-14
 - oracle queries optimization
