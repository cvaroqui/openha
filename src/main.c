/*
* Copyright (C) 1999,2000,2001,2002,2003,2004 Samuel Godbillot <sam028@users.sourceforge.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <interface.h>
#include <support.h>

GtkWidget *w_main, *clist1, *clist2;
gchar *progname;

gboolean init_var();
gint timeout;
guint t1,t2;

int
main (int argc, char *argv[])
{
	struct utsname name;
	gchar *title;
	//GtkStyle *style;
	extern void init_pix();
	extern gboolean fill_tab_services(gpointer);
	extern gboolean fill_tab_monitor(gpointer);

	if (init_var()!=TRUE){
		exit(-1);
	}
	timeout=5;
	uname(&name);
	title=g_strconcat("OPENHA@",name.nodename,"  0.3.5",NULL);
	
  gtk_set_locale ();
  gtk_init (&argc, &argv);	
	//gtk_rc_parse("./gtkrc");

/*
  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");
*/

  w_main = create_w_main ();
  gtk_widget_show (w_main);	
	//style=gtk_rc_get_style(w_main);
	//gtk_widget_set_style((GtkWidget *)w_main,style);
	gtk_window_set_title((GtkWindow *)w_main,(const char *)title);

	init_pix();
	clist1 = lookup_widget (GTK_WIDGET (w_main), "clist1");	
	clist2 = lookup_widget (GTK_WIDGET (w_main), "clist2");

	fill_tab_services(clist1);
	fill_tab_monitor(clist2);
  	t1=gtk_timeout_add(1000*timeout,fill_tab_services,clist1);
	t2=gtk_timeout_add(1000*timeout,fill_tab_monitor,clist2);

	gtk_main ();
  return 0;
}

