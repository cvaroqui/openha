Summary:    EASY high-availability software.
Name:       openha
Version:    0.3.6.osvc2
Release:    0
Epoch:      3
License:    GPL
Group:      Utilities
URL:        http://open-ha-cluster.sourceforge.net/
Packager:   %{packager}
Source:     %{name}-%{version}.tar.gz
BuildRoot:  %{_tmppath}/%{name}-%{version}-root

BuildRequires: gtk+, glib

%package devel
Summary: Development headers, documentation, and libraries for OpenHACluster.
Group: Utilities

%description

%description devel

%prep
%setup -q
./configure --prefix=/usr/local/cluster 

%build
make clean
make

%install
make install "DESTDIR=$RPM_BUILD_ROOT"

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)

%doc NEWS COPYING AUTHORS README 

/usr/local/cluster/*

%files devel

%changelog
