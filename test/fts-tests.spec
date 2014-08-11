Name:    fts-tests
Version: 3.2.27
Release: 1%{?dist}
Summary: Testing package for FTS3
Group:   Application/Internet
License: Apache 2

Source0:   %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: cmake

Requires: fts-client
Requires: fts-rest-cli
Requires: gfal2-python
Requires: gfal2-plugin-srm
Requires: gfal2-plugin-gridftp
Requires: gfal2-plugin-http
Requires: python
Requires: voms-clients

%description
FTS3 tests

%prep
%setup -q -n %{name}-%{version}

%build
cmake . -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_ROOT -DALLBUILD=ON -DCMAKE_BUILD_TYPE=Debug
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT

make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/share/fts3/tests/*

