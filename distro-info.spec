# Copyright 2022 Wong Hoi Sing Edison <hswong3i@pantarei-design.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

%global debug_package %{nil}

Name: distro-info
Epoch: 100
Version: 1.2
Release: 1%{?dist}
Summary: Provides information about the distributions' releases
License: ISC
URL: https://salsa.debian.org/debian/distro-info/-/tags
Source0: %{name}_%{version}.orig.tar.gz
BuildRequires: distro-info-data >= 0.46
BuildRequires: fdupes
BuildRequires: make
BuildRequires: python-rpm-macros
BuildRequires: python3-devel
BuildRequires: python3-setuptools
Requires: distro-info-data >= 0.46

%description
Information about all releases of Debian and Ubuntu. The distro-info
script will give you the codename for e.g. the latest stable release of
your distribution. To get information about a specific distribution
there are the debian-distro-info and the ubuntu-distro-info scripts.

%prep
%autosetup -T -c -n %{name}_%{version}-%{release}
tar -zx -f %{S:0} --strip-components=1 -C .

%build
make debian-distro-info
make ubuntu-distro-info
cd python && %py3_build

%install
install -Dpm755 -d %{buildroot}%{_bindir}
install -Dpm755 -d %{buildroot}%{_mandir}/man1
install -Dpm755 -d %{buildroot}%{perl_vendorlib}/Debian
install -Dpm755 -t %{buildroot}%{_bindir} debian-distro-info
install -Dpm755 -t %{buildroot}%{_bindir} ubuntu-distro-info
install -Dpm644 -t %{buildroot}%{_mandir}/man1 doc/*.1
install -Dpm644 -t %{buildroot}%{perl_vendorlib}/Debian perl/Debian/*.pm
cd python && %py3_install
find %{buildroot}%{python3_sitelib} -type f -name '*.pyc' -exec rm -rf {} \;
fdupes -qnrps %{buildroot}%{python3_sitelib}

%check

%files
%{_bindir}/*
%{_mandir}/*/*

%package -n perl-distro-info
Summary: Information about distributions' releases (Perl module)
BuildArch: noarch
Requires: distro-info-data >= 0.46
Requires: perl(Time::Piece)
Requires: perl-interpreter

%description -n perl-distro-info
This package contains a Perl module for parsing the data in
distro-info-data. There is also a command line interface in the
distro-info package.

%files -n perl-distro-info
%{perl_vendorlib}/Debian

%if 0%{?suse_version} > 1500
%package -n python%{python3_version_nodots}-distro-info
Summary: Information about distributions' releases (Python 3 module)
BuildArch: noarch
Requires: distro-info-data >= 0.46
Requires: python3
Provides: python3-distro-info = %{epoch}:%{version}-%{release}
Provides: python3dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}dist(distro-info) = %{epoch}:%{version}-%{release}

%description -n python%{python3_version_nodots}-distro-info
This package contains a Python 3 module for parsing the data in
distro-info-data. There is also a command line interface in the
distro-info package.

%files -n python%{python3_version_nodots}-distro-info
%{python3_sitelib}/*
%endif

%if 0%{?sle_version} > 150000
%package -n python3-distro-info
Summary: Information about distributions' releases (Python 3 module)
BuildArch: noarch
Requires: distro-info-data >= 0.46
Requires: python3
Provides: python3-distro-info = %{epoch}:%{version}-%{release}
Provides: python3dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}dist(distro-info) = %{epoch}:%{version}-%{release}

%description -n python3-distro-info
This package contains a Python 3 module for parsing the data in
distro-info-data. There is also a command line interface in the
distro-info package.

%files -n python3-distro-info
%{python3_sitelib}/*
%endif

%if !(0%{?suse_version} > 1500) && !(0%{?sle_version} > 150000)
%package -n python3-distro-info
Summary: Information about distributions' releases (Python 3 module)
BuildArch: noarch
Requires: distro-info-data >= 0.46
Requires: python3
Provides: python3-distro-info = %{epoch}:%{version}-%{release}
Provides: python3dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version}dist(distro-info) = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}-distro-info = %{epoch}:%{version}-%{release}
Provides: python%{python3_version_nodots}dist(distro-info) = %{epoch}:%{version}-%{release}

%description -n python3-distro-info
This package contains a Python 3 module for parsing the data in
distro-info-data. There is also a command line interface in the
distro-info package.

%files -n python3-distro-info
%{python3_sitelib}/*
%endif

%changelog
