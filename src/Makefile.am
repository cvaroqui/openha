## Process this file with automake to produce Makefile.in

#INCLUDES =\
#	 -I.

AM_CFLAGS =\
	$(glib_CFLAGS) \
	 -Wall\
	 -O2\
	 -g

openhabindir = ${prefix}/lib/openha/bin

if SOLARIS
  openhabin_PROGRAMS = service heartc heartd heartc_unicast heartd_unicast heartc_raw heartd_raw nmond hb
else
  openhabin_PROGRAMS = service heartc heartd heartc_unicast heartd_unicast heartc_raw heartd_raw heartc_dio heartd_dio nmond hb
endif

service_SOURCES = service.c sockhelp.c cluster.c
service_LDFLAGS =
service_LDADD = $(glib_LIBS) -lpthread

heartc_SOURCES = heartc.c sockhelp.c cluster.c
heartc_LDFLAGS = 
heartc_LDADD = $(glib_LIBS)

heartc_unicast_SOURCES = heartc_unicast.c sockhelp.c cluster.c
heartc_unicast_LDFLAGS = 
heartc_unicast_LDADD = $(glib_LIBS)

heartc_raw_SOURCES = heartc_raw.c sockhelp.c cluster.c
heartc_raw_LDFLAGS = 
heartc_raw_LDADD = $(glib_LIBS)

heartc_dio_SOURCES = heartc_dio.c sockhelp.c cluster.c
heartc_dio_LDFLAGS = 
heartc_dio_LDADD = $(glib_LIBS)

heartd_SOURCES = heartd.c sockhelp.c cluster.c
heartd_LDFLAGS = 
heartd_LDADD = $(glib_LIBS)

heartd_unicast_SOURCES = heartd_unicast.c sockhelp.c cluster.c
heartd_unicast_LDFLAGS = 
heartd_unicast_LDADD = $(glib_LIBS)

heartd_raw_SOURCES = heartd_raw.c sockhelp.c cluster.c
heartd_raw_LDFLAGS = 
heartd_raw_LDADD = $(glib_LIBS)

heartd_dio_SOURCES = heartd_dio.c sockhelp.c cluster.c
heartd_dio_LDFLAGS = 
heartd_dio_LDADD = $(glib_LIBS)

nmond_SOURCES = nmond.c sockhelp.c cluster.c
nmond_LDFLAGS = 
nmond_LDADD = $(glib_LIBS) -lpthread

hb_SOURCES = hb.c sockhelp.c checks.h cluster.c
hb_LDFLAGS = 
hb_LDADD = $(glib_LIBS)
