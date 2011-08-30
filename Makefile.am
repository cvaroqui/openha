## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = src

openhadir = ${prefix}
openhadocdir = ${prefix}/doc
openhabindir = ${prefix}/bin
openhalogdir = ${prefix}/log
openhaprocdir = ${prefix}/log/proc
openhaconfdir = ${prefix}/conf
openhasrvdir = ${prefix}/services

openhadoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

openha_DATA =  env.sh ezha
openhaproc_DATA =  
openhaconf_DATA =  
openhasrv_DATA =  

EXTRA_DIST = $(openhadoc_DATA) $(openha_DATA)\
	     $(openhaproc_DATA) \
	     $(openhaconf_DATA) \
	     $(openhasrv_DATA)
#bin_SCRIPTS =  ezha

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

