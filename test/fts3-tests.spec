Name:    fts3-tests
Version: 0.0.1
Release: 1
Summary: Testing package for FTS3
Group:   Application/Internet
License: Apache 2

Source0:   %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: cmake

Requires: fts-client
Requires: voms-clients

%description
FTS3 tests

%prep
%setup -q -n %{name}-%{version}

%build
cmake . -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_ROOT/usr -DALLBUILD=ON -DCMAKE_BUILD_TYPE=Debug
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT

make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/share/fts3/*

