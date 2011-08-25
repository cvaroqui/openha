## Process this file with automake to produce Makefile.in

#INCLUDES =\
#	 -I.

AM_CFLAGS =\
	$(glib_CFLAGS) \
	$(gtk_CFLAGS) \
	 -Wall\
	 -O2\
	 -g

bin_PROGRAMS = service heartc heartd heartc_raw heartd_raw nmond gui hb

service_SOURCES = service.c sockhelp.c cluster.h sockhelp.h
service_LDFLAGS = $(glib_LIBS)
service_LDADD = 

heartc_SOURCES = heartc.c sockhelp.c
heartc_LDFLAGS = $(glib_LIBS)
heartc_LDADD = 

heartc_raw_SOURCES = heartc_raw.c sockhelp.c
heartc_raw_LDFLAGS = $(glib_LIBS)
heartc_raw_LDADD = 

heartd_SOURCES = heartd.c sockhelp.c
heartd_LDFLAGS = $(glib_LIBS)
heartd_LDADD = 

heartd_raw_SOURCES = heartd_raw.c sockhelp.c
heartd_raw_LDFLAGS = $(glib_LIBS)
heartd_raw_LDADD = 

nmond_SOURCES = nmond.c sockhelp.c
nmond_LDFLAGS = $(glib_LIBS)
nmond_LDADD = 

gui_SOURCES =  support.c main.c callbacks.c interface.c config.c callbacks.h gui.h interface.h support.h utils.h
gui_LDFLAGS = $(glib_LIBS) $(gtk_LIBS)
gui_LDADD = 

hb_SOURCES = hb.c sockhelp.c checks.h
hb_LDFLAGS = $(glib_LIBS)
hb_LDADD = 
