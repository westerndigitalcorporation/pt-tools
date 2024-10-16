Name:		%{pkg_name}
Version:	%{pkg_version}
Release:	1%{?dist}
Summary:	A library to execute SCSI and ATA passtrhough commands

License:	GPLv2+
URL:		https://github.com/westerndigitalcorporation/%{name}
Source0:	%{url}/releases/download/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	make
BuildRequires:	gcc

%description
A library to execute SCSI and ATA passtrhough commands.

# Development headers package
%package devel
Summary: Development header files for libptio
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package provides development header files for libptio.

# Command line tools package
%package cli-tools
Summary: Command line tools using libptio
Requires: %{name}%{?_isa} = %{version}-%{release}

%description cli-tools
This package provides command line tools using libptio.

%prep
%autosetup

%build
sh autogen.sh
%configure --libdir="%{_libdir}" --includedir="%{_includedir}"
%make_build

%install
%make_install PREFIX=%{_prefix}
chmod -x ${RPM_BUILD_ROOT}%{_mandir}/man8/*.8*

find ${RPM_BUILD_ROOT} -name '*.la' -delete

%ldconfig_scriptlets

%files
%{_libdir}/*.so.*
%exclude %{_libdir}/*.a
%exclude %{_libdir}/pkgconfig/*.pc
%license LICENSES/GPL-2.0-or-later.txt
%doc README.md

%files devel
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
%license LICENSES/GPL-2.0-or-later.txt

%files cli-tools
%{_bindir}/ptio
%{_mandir}/man8/ptio.8*
%license LICENSES/GPL-2.0-or-later.txt

%changelog
* Wed Oct 16 2024 Damien Le Moal <damien.lemoal@wdc.com> 2.0.0-1
- Version 2.0.0 package
* Fri Sep 20 2024 Damien Le Moal <damien.lemoal@wdc.com> 1.0.0-1
- Version 1.0.0 package
