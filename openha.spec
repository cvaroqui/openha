Summary:    EASY high-availability software.
Name:       openha
Version:    0.4.3.osvc1
Release:    0
Epoch:      3
License:    GPL
Group:      Utilities
URL:        http://open-ha-cluster.sourceforge.net/
Packager:   %{packager}
Source:     %{name}-%{version}.tar.gz
BuildRoot:  %{_tmppath}/%{name}-%{version}-root

%package devel
Summary: Development headers, documentation, and libraries for OpenHACluster.
Group: Utilities

%description

%description devel

%prep
%setup -q
aclocal
autoconf
automake --add-missing
./configure --prefix=/usr/local/cluster 

%build
make clean
make

%install
make install "DESTDIR=$RPM_BUILD_ROOT"
mkdir $RPM_BUILD_ROOT/usr/local/cluster/log $RPM_BUILD_ROOT/usr/local/cluster/log/proc $RPM_BUILD_ROOT/usr/local/cluster/conf 
cp -f postinstall $RPM_BUILD_ROOT/usr/local/cluster/.postinstall
cp -f preremove $RPM_BUILD_ROOT/usr/local/cluster/.preremove
cp -f systemd.opensvc-openha.service $RPM_BUILD_ROOT/usr/local/cluster/.systemd.opensvc-openha.service

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
/usr/local/cluster/.postinstall
/usr/local/cluster/.preremove
/usr/local/cluster/.systemd.opensvc-openha.service

%doc COPYING AUTHORS README 

/usr/local/cluster/*

%files devel

%changelog

%preun
/usr/local/cluster/.preremove
%post
/usr/local/cluster/.postinstall
