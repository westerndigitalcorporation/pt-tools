Name:		%{pkg_name}
Version:	%{pkg_version}
Release:	1%{?dist}
Summary:	Provides a user utility to send any SCSI or ATA CDB to a device

License:	GPLv2+
URL:		https://bitbucket.wdc.com/users/damien.lemoal_wdc.com/repos/pt-tools/browse
Source0:	%{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	make
BuildRequires:	gcc

%description
Provides a user utility to send any SCSI or ATA CDB to a device.

%prep
%autosetup

%build
sh autogen.sh
%configure
%make_build

%install
%make_install
%make_install install-tests

%files
%{_bindir}/*
%{_mandir}/man8/*
%license COPYING.GPL
%doc README.md CONTRIBUTING

%changelog
* Fri Sep 20 2024 Damien Le Moal <damien.lemoal@wdc.com> 1.1.0-1
- Version 1.1.0 package
