## Process this file with automake to produce Makefile.in

#INCLUDES =\
#	 -I.

AM_CFLAGS =\
	$(glib_CFLAGS) \
	$(gtk_CFLAGS) \
	 -Wall\
	 -O2\
	 -g

if SOLARIS
bin_PROGRAMS = service heartc heartd heartc_raw heartd_raw nmond hb
else
bin_PROGRAMS = service heartc heartd heartc_raw heartd_raw heartc_dio heartd_dio nmond hb
endif

service_SOURCES = service.c sockhelp.c cluster.c
service_LDFLAGS = $(glib_LIBS)
service_LDADD = 

heartc_SOURCES = heartc.c sockhelp.c cluster.c
heartc_LDFLAGS = $(glib_LIBS)
heartc_LDADD = 

heartc_raw_SOURCES = heartc_raw.c sockhelp.c cluster.c
heartc_raw_LDFLAGS = $(glib_LIBS)
heartc_raw_LDADD = 

heartc_dio_SOURCES = heartc_dio.c sockhelp.c cluster.c
heartc_dio_LDFLAGS = $(glib_LIBS)
heartc_dio_LDADD = 

heartd_SOURCES = heartd.c sockhelp.c cluster.c
heartd_LDFLAGS = $(glib_LIBS)
heartd_LDADD = 

heartd_raw_SOURCES = heartd_raw.c sockhelp.c cluster.c
heartd_raw_LDFLAGS = $(glib_LIBS)
heartd_raw_LDADD = 

heartd_dio_SOURCES = heartd_dio.c sockhelp.c cluster.c
heartd_dio_LDFLAGS = $(glib_LIBS)
heartd_dio_LDADD = 

nmond_SOURCES = nmond.c sockhelp.c cluster.c
nmond_LDFLAGS = $(glib_LIBS)
nmond_LDADD = 

hb_SOURCES = hb.c sockhelp.c checks.h cluster.c
hb_LDFLAGS = $(glib_LIBS)
hb_LDADD = 
