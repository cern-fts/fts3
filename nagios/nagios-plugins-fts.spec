# Package needs to stay arch specific (due to nagios plugins location), but
# there's nothing to extract debuginfo from
%global debug_package %{nil}

%define nagios_plugins_dir %{_libdir}/nagios/plugins

Name:       nagios-plugins-fts
Version:    3.2.0
Release:    1%{?dist}
Summary:    Nagios probes to be run remotely against FTS3 machines
License:    ASL 2.0
Group:      Applications/Internet
URL:        https://svnweb.cern.ch/trac/fts3
# The source of this package was pulled from upstream's vcs. Use the
# following commands to generate the tarball:
# svn export http://svn.cern.ch/guest/fts3/trunk/nagios/ nagios-plugins-fts-3.2.0
# tar -czvf nagios-plugins-fts-3.2.0.tar.gz nagios
Source0:   %{name}-%{version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch: noarch

%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  cmake
%else
BuildRequires:  cmake28
%endif

Requires:   nagios%{?_isa}
Requires:   python%{?_isa}
Requires:   python-pycurl%{?_isa}

%if %{?rhel}%{!?rhel:0} <= 5
Requires:   python-simplejson%{?_isa}
%endif

%description
This package provides the nagios probes for FTS3. Usually they are installed
in the nagios host, and they will contact the remote services running in the
FTS3 machines.

%prep
%setup -q -n %{name}-%{version}

%build
%if %{?fedora}%{!?fedora:0} >= 18 || %{?rhel}%{!?rhel:0} >= 7
%cmake . -DCMAKE_INSTALL_PREFIX=/
%else
%cmake28 . -DCMAKE_INSTALL_PREFIX=/
%endif

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}

make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%config(noreplace) %{_sysconfdir}/nagios/objects/fts3-template.cfg
%{nagios_plugins_dir}/fts
%doc LICENSE README

%changelog
* Tue Nov 12 2013 Alejandro Alvarez Ayllon <aalvarez@cern.ch> - 3.2.0-1
- Initial build

