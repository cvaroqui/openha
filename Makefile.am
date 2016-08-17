## Process this file with automake to produce Makefile.in

SUBDIRS = src

openhadir = ${prefix}
openhainitdir = ${datarootdir}/openha/init
openhadocdir = ${datarootdir}/doc/openha
openhaconfdir = ${sysconfdir}/openha
openhasrvdir = ${localstatedir}/lib/openha/services
openhaprocdir = ${localstatedir}/lib/openha/proc
openhalogdir = ${localstatedir}/log/openha

openhadoc_DATA = \
	README \
	COPYING \
	AUTHORS

bin_SCRIPTS = openha
openhainit_SCRIPTS = openha.init

EXTRA_DIST = $(openhadoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

install-data-local:
	$(MKDIR_P) $(DESTDIR)/$(openhainitdir)
	$(MKDIR_P) $(DESTDIR)/$(openhaprocdir)
	$(MKDIR_P) $(DESTDIR)/$(openhaconfdir)
	$(MKDIR_P) $(DESTDIR)/$(openhasrvdir)
	$(MKDIR_P) $(DESTDIR)/$(openhalogdir)

