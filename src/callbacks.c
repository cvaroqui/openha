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
#include <stdlib.h>

#include <callbacks.h>
#include <interface.h>

#include <support.h>
#include <cluster.h>
#include <gui.h>

extern gint timeout;
extern guint t1,t2;

gboolean hb_add(gchar*,gchar*,gchar*,gchar*,gchar*,gchar*);
gboolean hb_rm(gchar**);


void
on_w_main_destroy                      (GtkObject       *object,
                                        gpointer         user_data)
{
	exit(0);
}


void
on_exit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	exit(0);
}



void
on_hb_add_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy ((GtkWidget*)w_hb_list);
}
void
on_help_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


// PREFERENCES BEGIN
void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
		w_preferences=create_w_preferences();
		gtk_widget_show (w_preferences);
 		gtk_grab_add(w_preferences);
}


void
on_button1_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *W;
	extern GtkWidget *clist1, *clist2; 
		
	W  = lookup_widget (GTK_WIDGET (button), "spinbutton1");  
	timeout=gtk_spin_button_get_value_as_int((GtkSpinButton*) W);
	gtk_timeout_remove(t1);
	gtk_timeout_remove(t2);
 	t1=gtk_timeout_add(1000*timeout,fill_tab_services,clist1);
	t2=gtk_timeout_add(1000*timeout,fill_tab_monitor,clist2);
		
	gtk_widget_destroy ((GtkWidget*)w_preferences);
}

void
on_button2_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy ((GtkWidget*)w_preferences);
}

// PREFERENCES END



// NODES BEGIN
// -> LIST
//
void on_nodes_list_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *W;
		
	w_nodes_list = create_w_nodes_list();  
	W  = lookup_widget (GTK_WIDGET (w_nodes_list), "clist_nodes");  
	list_nodes((GtkCList *)W);
 	gtk_clist_sort((GtkCList *)W);
	gtk_widget_show (w_nodes_list);
 	gtk_grab_add(w_nodes_list);
}

// -> ADD
//
void
on_node_add_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	w_nodes_add = create_w_nodes_add();  
	gtk_widget_show (w_nodes_add);
 	gtk_grab_add(w_nodes_add);
}

void
on_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy ((GtkWidget*)w_nodes_list);
}

void
on_button4_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
static struct hostent *h;
gchar message[255];
GtkWidget *W;
	
	W  = lookup_widget (GTK_WIDGET (w_nodes_add), "entry1");  
	
	h=gethostbyname(gtk_entry_get_text(GTK_ENTRY(W)));
	if (h==NULL){
		//Error
		strcpy(message,"Node ");
		strcat(message,gtk_entry_get_text(GTK_ENTRY(W)));
		strcat(message," unknown.\n");
		log_txt(message);
	}
	else{
		if (exist_node(gtk_entry_get_text(GTK_ENTRY(W))) == TRUE){
			if (add_node(gtk_entry_get_text(GTK_ENTRY(W))) == TRUE){
				//Success
				strcpy(message,"Node ");
				strcat(message,gtk_entry_get_text(GTK_ENTRY(W)));
				strcat(message," added.\n");
				log_txt(message);
			}
			else{
				//Error
				strcpy(message,"Add_node() for");
				strcat(message,gtk_entry_get_text(GTK_ENTRY(W)));
				strcat(message," failed.\n");
				log_txt(message);
			}
		}
		else{
			//Error
			strcpy(message,"Node ");
			strcat(message,gtk_entry_get_text(GTK_ENTRY(W)));
			strcat(message," already created.\n");
			log_txt(message);
		}
		//g_free((struct hostent *)h);
		gtk_widget_destroy (w_nodes_add);
	}

}


void
on_button5_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy ((GtkWidget*)w_nodes_add);
}

// REMOVE
void
on_node_rm_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	GList *list_nodes=NULL;
	FILE *File;
	GtkWidget *W;
	GList *L;	
	
	if ((File = fopen(EZ_NODES,"r+")) != NULL) {
		list_nodes = get_liste(File,1);
		fclose(File);
	}
	w_nodes_rm = create_w_nodes_rm();	
	
	W  = lookup_widget (GTK_WIDGET (w_nodes_rm), "combo1");  
 	//gtk_clist_sort((GtkCList *)list_nodes);
	L=g_list_sort(list_nodes,(GCompareFunc)strcmp);
	list_nodes=L;	
	if (list_nodes != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_nodes);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	gtk_window_set_title(GTK_WINDOW(w_nodes_rm), "Node removing");
	gtk_widget_show (w_nodes_rm);
 	gtk_grab_add(w_nodes_rm);
	g_list_free(list_nodes);	
}

void
on_button6_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *entry_t, message[255];
	GtkWidget *W;
	
	W  = lookup_widget (GTK_WIDGET (w_nodes_rm), "combo1");  

	entry_t = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(W)->entry));
	if (nodes_rm(entry_t) == TRUE){
		strcpy(message,"Remove node ");
		strcat (message,entry_t);
		strcat (message," succeeded\n");
		log_txt(message);
	}
	else{		
		strcpy(message,"Remove node ");
		strcat (message,entry_t);
		strcat (message," failed\n");
		log_txt(message);
	}
	gtk_widget_destroy(GTK_WIDGET(w_nodes_rm));
}


void
on_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_widget_destroy(GTK_WIDGET(w_nodes_rm));
}

// NODES END


/****************************/
/* DIVERS FUNCTIONS					*/
/****************************/

void log_txt(gchar *to_write){
	GtkWidget* W;
	W  = lookup_widget (GTK_WIDGET (w_main), "log_zone");  
	gtk_text_insert (GTK_TEXT (W), NULL, NULL,NULL, to_write,-1);
}

gboolean exist_node(gchar* node){
	FILE *File;
	GList *list_nodes=NULL;
	gint i=0;

	File = fopen(EZ_NODES, "r");
	// File not created
 	if (File  == NULL ){
		return TRUE;
	}
	list_nodes=get_liste(File,1);
	for (i=0;i<g_list_length(list_nodes);i++){
	if (strcmp(node,g_list_nth_data(list_nodes,i))==0){
			g_list_free(list_nodes);	
			return FALSE;
		}
	}
	g_list_free(list_nodes);	
	//printf("Node can be created" );
	return TRUE;
}

gboolean add_node(gchar *node){
// Lock $EZ_NODES
// Ouvre $EZ_NODES
// Ecrit *node 
// Ferme le fichier
FILE *File;

	if ((File=fopen(EZ_NODES,"a")) == NULL){
		log_txt("unable to open $EZ_NODES");
		return FALSE;
	}	
	if (lockf(fileno(File),F_TLOCK,0) != 0){
		log_txt("Error: unable to lock $EZ_NODES");
		fclose(File);
		return FALSE;
	}
	//printf("to write: %s\n",node);
	fprintf(File,"%s\n",node);
	fclose(File);
	return TRUE;
}

gboolean nodes_rm(gchar *node)
{
	FILE *File, *F;
	GList *list_nodes=NULL;
	gint i;
	gchar message[80];

	if ((File = fopen(EZ_NODES,"r+")) != NULL) {
		list_nodes = get_liste(File,1);
		//fclose(File);
	}
	else{
		strcpy(message,"Remove node: unable to open $EZ_NODES for reading\n");
		log_txt(message);
		//g_free(message);
		return FALSE;
	}
	/*****************************************************/
	if (lockf(fileno(File),F_TLOCK,0) == 0) {
		if (((F = freopen(EZ_NODES,"w",File)) != NULL) && (list_nodes!=NULL)){
			for(i=0;i<g_list_length(list_nodes);i++){
				if(strcmp(g_list_nth_data(list_nodes,i),node)!=0){
					fprintf(F,"%s\n",(gchar *)g_list_nth_data(list_nodes,i));
				}
			}
			//g_free(message);
			g_list_free(list_nodes);
			fclose(File);
			//fclose(F);
			return TRUE;
		}
		else{
			strcpy(message,"Remove node: unable reopen $EZ_NODES\n");
			log_txt(message);
			//g_free(message);
			g_list_free(list_nodes);
			return FALSE;
		}
	}
	else{
		strcpy(message,"Remove node: unable to lock $EZ_NODES\n");
		log_txt(message);
		//g_free(message);
		g_list_free(list_nodes);
		return FALSE;
	}
}

/*           HEARTBEATS ...*/
void
on_hb_list_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GtkWidget *W;

	w_hb_list=create_w_hb_list();
	W  = lookup_widget (GTK_WIDGET(w_hb_list), "clist3");  
	fill_monitor((GtkCList *)W);
 	gtk_clist_sort((GtkCList *)W);
	gtk_widget_show (w_hb_list);
 	gtk_grab_add(w_hb_list);
	
}


void on_hb_add_net_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *W;
	FILE *File;
	GList *L;	
	GList *list_nodes=NULL;
	
	if ((File = fopen(EZ_NODES,"r")) != NULL) {
		list_nodes = get_liste(File,1);
		fclose(File);
	}
	w_hb_add=create_w_hb_add();
	W  = lookup_widget (GTK_WIDGET (w_hb_add), "combo2");  
	L=g_list_sort(list_nodes,(GCompareFunc)strcmp);
	list_nodes=L;	
	if (list_nodes != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_nodes);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	gtk_widget_show (w_hb_add);
 	gtk_grab_add(w_hb_add);

}

void on_hb_add_disk_activate (GtkMenuItem *menuitem,gpointer user_data)
{
	GtkWidget *W;
	FILE *File;
	GList *L;	
	GList *list_nodes=NULL;
	
	if ((File = fopen(EZ_NODES,"r")) != NULL) {
		list_nodes = get_liste(File,1);
		fclose(File);
	}
	w_hb_disk_add=create_w_hb_disk_add();
	W  = lookup_widget (GTK_WIDGET (w_hb_disk_add), "combo3");  
	L=g_list_sort(list_nodes,(GCompareFunc)strcmp);
	list_nodes=L;	
	if (list_nodes != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_nodes);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	gtk_widget_show (w_hb_disk_add);
 	gtk_grab_add(w_hb_disk_add);
}


void
on_button9_clicked (GtkButton *button,gpointer user_data)
{
	GtkWidget *device,*node,*add1,*add2,*add3,*add4;
	GtkWidget *port,*timeout;
	gchar *add,*val;
	gchar *Node,*Type,*Interface,*Addr,*Port,*Timeout;
	gint k;
	//
	Node=g_malloc0(MAX_NODENAME_SIZE);
	Interface=g_malloc0(5);
	Addr=g_malloc0(16);
	Port=g_malloc0(5);
	Type=g_malloc0(5);
	Timeout=g_malloc0(4);
	strcpy(Type,"net");

	node = lookup_widget(w_hb_add,"combo2");
	device = lookup_widget(w_hb_add,"entry2");
	add1 = lookup_widget(w_hb_add,"spinbutton2");
	add2 = lookup_widget(w_hb_add,"spinbutton3");
	add3 = lookup_widget(w_hb_add,"spinbutton4");
	add4 = lookup_widget(w_hb_add,"spinbutton5");
	port = lookup_widget(w_hb_add,"spinbutton6");
	timeout = lookup_widget(w_hb_add,"spinbutton7");

	if (if_testaddr(gtk_entry_get_text(GTK_ENTRY(device))) !=0){
			log_txt("Error: interface not valid.");
	return ;}			

	Node=strcpy(Node,gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(node)->entry)));
	Interface=strcat(Interface,gtk_entry_get_text(GTK_ENTRY(device)));
	
	val=malloc(4);
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add1));
	sprintf(val,"%d",k);
	Addr=strcat(Addr,val); add=strcat(Addr,".");
	
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add2));
	sprintf(val,"%d",k);
	add=strcat(Addr,val); add=strcat(Addr,".");
	
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add3));
	sprintf(val,"%d",k);
	add=strcat(Addr,val); add=strcat(Addr,".");
	
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add4));
	sprintf(val,"%d",k);
	add=strcat(Addr,val);

	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port));
	sprintf(Port,"%d",k);
	
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(timeout));
	sprintf(Timeout,"%d",k);

	hb_add(Node,Type,Interface,Addr,Port,Timeout); 
	gtk_widget_destroy ((GtkWidget*)w_hb_add);
}


void
on_button10_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_hb_add);
}

gboolean hb_add(gchar *Node,gchar *Type,gchar *Interface,
								gchar* Addr,gchar *Port,gchar *Timeout)
{
	FILE *File;
	GList *list_hb=NULL;
	gint i=0,j=0;
	gchar *lu[6];
	gchar *node[16],*type[16],*interface[16],*addr[16],*port[16],*timeout[16]	;
	
	if ((File = fopen(EZ_MONITOR,"a+")) != NULL) {
		list_hb = get_liste(File,6);
	}
	else{
		log_txt("Add HB: unable to open $EZ_MONITOR.");
		return FALSE;
	}
	for(j=0;j<6;j++)
		lu[j]=g_malloc0(64);
	j=0;
	if (lockf(fileno(File),F_TLOCK,0) == 0) {
		while(fscanf(File,"%s %s %s %s %s",lu[0],lu[1],lu[2],lu[3],lu[4]) != -1){
			node[i]=g_malloc0(MAX_NODENAME_SIZE);
			type[i]=g_malloc0(5);
			interface[i]=g_malloc0(16);
			addr[i]=g_malloc0(16);
			port[i]=g_malloc0(5);
			timeout[i]=g_malloc0(5);
			if (strcmp("net",lu[1])==0){
				fscanf(File,"%s",lu[5]);
				strcpy(node[i],lu[0]);
				strcpy(type[i],lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i],lu[3]); 
				strcpy(port[i], lu[4]); 
				strcpy(timeout[i], lu[5]); 
			}	
			if (strcmp("disk",lu[1])==0){
				strcpy(node[i],lu[0]);
				strcpy(type[i],lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i],lu[3]); 
				strcpy(port[i], lu[4]); 
				strcpy(timeout[i], "-"); 
			}
			i++;
		}
	}
	/******************/
	j=i;
	for(i=0;i<j;i++){
		//printf("%s %s %s %s %s\n",node[i],interface[i],type[i],addr[i],port[i]);	
		//printf("%s %s %s %s %s\n",Node,Interface,Type,Addr,Port);	
		if( (strcmp(Node,node[i])==0) &&
				(strcmp(Interface,interface[i])==0) &&
				(strcmp(Addr,addr[i])==0) &&
				(strcmp(Port,port[i])==0) ) {
				printf("HB link already defined\n");
				return FALSE;
		}
	}
	if (Timeout != NULL)
		fprintf(File,"%s\t%s\t%s\t%s\t%s\t%s\n",Node,Type,Interface,Addr,Port,Timeout);
	else
		fprintf(File,"%s\t%s\t%s\t%s\t%s\n",Node,Type,Interface,Addr,Port);
	fclose(File);
	return TRUE;
}


void on_button11_clicked (GtkButton *button,gpointer user_data)
{
	GtkWidget *device,*node,*add;
	GtkWidget *timeout;
	gchar *val;
	gchar *Node,*Type,*Interface,*Addr,*Port,*Timeout;
	gint k;
	struct stat *buf;
	//
	Node=g_malloc0(MAX_NODENAME_SIZE);
	Type=g_malloc0(5);
	Interface=g_malloc0(5);
	Addr=g_malloc0(16);
	Port=g_malloc0(5);
	Timeout=g_malloc0(4);
	buf=g_malloc0(sizeof(struct stat));
	strcpy(Type,"disk");

	node = lookup_widget(w_hb_disk_add,"combo3");
	device = lookup_widget(w_hb_disk_add,"entry3");
	add = lookup_widget(w_hb_disk_add,"spinbutton8");
	timeout = lookup_widget(w_hb_disk_add,"spinbutton9");

	strcpy(Node,gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(node)->entry)));
	strcpy(Interface,gtk_entry_get_text(GTK_ENTRY(device)));

	if (stat(Interface,buf) !=0){
		log_txt("Error: unable to stat raw device.");
		return ;
	}			
	if (!S_ISCHR(buf->st_mode)){
		log_txt("Error: not a raw device.");
		return ;
	}
	
	val=malloc(4);
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(add));
	sprintf(Addr,"%d",k);
	
	k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(timeout));
	sprintf(Timeout,"%d",k);

	hb_add(Node,Type,Interface,Addr,Timeout,NULL); 
	gtk_widget_destroy ((GtkWidget*)w_hb_disk_add);
}


void on_button12_clicked (GtkButton *button, gpointer user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_hb_disk_add);
}


void
on_hb_rm_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	GList *list_hb=NULL,*L=NULL;
	FILE *File;
	GtkWidget *W;
	gchar line[MAX_HEARTBEAT][256], *lu[6];
	gint i,j,max;

	if ((File = fopen(EZ_MONITOR,"r+")) == NULL) {
		log_txt("Remove heartbeat: unable to open $EZ_MONITOR for reading.");
		return;
	}
	/*
	for(i=0;i<MAX_HEARTBEAT;j++)
		line[i]=g_malloc0(256);
	*/
	for(i=0;i<6;i++)
		lu[i]=g_malloc0(128);
	i=0;	
	if (lockf(fileno(File),F_TLOCK,0) == 0) {
		while(fscanf(File,"%s %s %s %s %s",lu[0],lu[1],lu[2],lu[3],lu[4]) != -1){
			if (strcmp("net",lu[1])==0){
				max=6;
				fscanf(File,"%s",lu[5]);
			}	
			else
					max=5;
			//line[i]=g_malloc0(256);	
			strcpy(line[i],lu[0]);
			for(j=1;j<max;j++){
				strcat(line[i]," ");strcat(line[i],lu[j]);
			}
			list_hb=g_list_append(list_hb,line[i]);
			i++;
		}
	}
	else{ //unlock failed
			return;
	}
	lockf(fileno(File),F_ULOCK,0);
	fclose(File);
	L=g_list_sort(list_hb,(GCompareFunc)strcmp);
	list_hb=L;	

	w_hb_rm = create_w_hb_rm();	
	W  = lookup_widget (GTK_WIDGET (w_hb_rm), "combo4");  
	
	if (list_hb != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_hb);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}

	gtk_widget_show(w_hb_rm);
 	gtk_grab_add(w_hb_rm);
	gtk_window_set_title(GTK_WINDOW(w_hb_rm), "Heartbeat removing");
}


void
on_button13_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget *W;
	gchar *Node,*Type,*Interface,*Addr,*Port,*Timeout;
	gchar *line;
	gchar **array;
	//
	line=g_malloc0(256);

	Node=g_malloc0(MAX_NODENAME_SIZE);
	Type=g_malloc0(5);
	Interface=g_malloc0(5);
	Addr=g_malloc0(16);
	Port=g_malloc0(5);
	Timeout=g_malloc0(4);

	W = lookup_widget(w_hb_rm,"combo4");
	strcpy(line,gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(W)->entry)));

	array=g_strsplit(line," ",6);
	
	hb_rm(array); 
	gtk_widget_destroy ((GtkWidget*)w_hb_rm);
}

gboolean hb_rm(gchar **array)
{
	FILE *File;
	GList *list_hb=NULL;
	gint i=0,j=0;
	gchar message[80];
	gchar *lu[6];
	gchar *node[16],*type[16],*interface[16],*addr[16],*port[16],*timeout[16]	;
	
	if ((File = fopen(EZ_MONITOR,"r")) != NULL) {
		list_hb = get_liste(File,6);
	}
	else{
		log_txt("Remove node: unable to open $EZ_MONITOR for reading");
		return FALSE;
	}
	for(j=0;j<6;j++)
		lu[j]=g_malloc0(64);
	j=0;
//	if (lockf(fileno(File),F_TLOCK,0) == 0) {
		while(fscanf(File,"%s %s %s %s %s",lu[0],lu[1],lu[2],lu[3],lu[4]) != -1){
			node[i]=g_malloc0(MAX_NODENAME_SIZE);
			type[i]=g_malloc0(5);
			interface[i]=g_malloc0(16);
			addr[i]=g_malloc0(16);
			port[i]=g_malloc0(5);
			timeout[i]=g_malloc0(5);
			if (strcmp("net",lu[1])==0){
				fscanf(File,"%s",lu[5]);
				strcpy(node[i],lu[0]);
				strcpy(type[i],lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i],lu[3]); 
				strcpy(port[i], lu[4]); 
				strcpy(timeout[i], lu[5]); 
			}	
			if (strcmp("disk",lu[1])==0){
				strcpy(node[i],lu[0]);
				strcpy(type[i],lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i],lu[3]); 
				strcpy(port[i], lu[4]); 
				strcpy(timeout[i], " "); 
			}
			i++;
		}
	//}
	//else{
	//		perror("lock");
	//		printf("Unable to lock file !\n");
	//		fclose(File);
	//		return FALSE;
	//}
	
	/******************/
	if ((File = freopen(EZ_MONITOR,"w",File)) != NULL) {
		if (lockf(fileno(File),F_TLOCK,0) == 0) {
			j=i;
		//printf("array: %s %s %s %s %s %s\n",array[0],array[1],array[2],array[3],array[4],array[5]);
			for(i=0;i<j;i++){
				if( (strcmp(array[0],node[i])==0) &&
						(strcmp(array[1],type[i])==0) &&
						(strcmp(array[2],interface[i])==0) &&
						(strcmp(array[3],addr[i])==0) &&
						(strcmp(array[4],port[i])==0) &&
						(strcmp(array[5],timeout[i])==0) ) {
						//printf("HB link found\n");
						//fclose(File);
				}
				else {
					//printf("%s %s %s %s %s %s\n",node[i],type[i],interface[i],addr[i],port[i],timeout[i]);
					fprintf(File,"%s\t%s\t%s\t%s\t%s\t%s\n",node[i],type[i],interface[i],addr[i],port[i],timeout[i]);
				}
			}
		}
		else{
			log_txt("Unable to lock file !\n");
			fclose(File);
			return FALSE;
		}
	}
	else {
		strcpy(message,"Remove node: unable to open $EZ_MONITOR for reading\n");
	}
	fclose(File);
	return TRUE;
}

void
on_button14_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_hb_rm);
}

// SERVICES BEGIN


void on_srv_add_activate (GtkMenuItem *menuitem,gpointer user_data)
{
	GtkWidget *W;
	FILE *File;
	GList *L,*list_nodes=NULL;
	
	if ((File = fopen(EZ_NODES,"r")) != NULL) {
		list_nodes = get_liste(File,1);
		fclose(File);
	}
	w_srv_add = create_w_srv_add();  
	W  = lookup_widget (GTK_WIDGET (w_srv_add), "combo5");  
	L=g_list_sort(list_nodes,(GCompareFunc)strcmp);
	list_nodes=L;	
	if (list_nodes != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_nodes);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	W  = lookup_widget (GTK_WIDGET (w_srv_add), "combo6");  
	L=g_list_sort(list_nodes,(GCompareFunc)strcmp);
	list_nodes=L;	
	if (list_nodes != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),list_nodes);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	gtk_widget_show (w_srv_add);
 	gtk_grab_add(w_srv_add);
}

// Service->Add->OK
void on_button16_clicked (GtkButton       *button,gpointer user_data)
{
	GtkWidget *W;
	gchar	*Service, *Script, *Primary, *Secondary, *Check_script;
	gchar *cmd,*arg[8];
	gint i=0,pid,status;
	gint pout[2], perr[2];
	FILE *out,*err;
	gchar *tmp;

	tmp=g_malloc0(1024);
	pipe(pout);
	pipe(perr);

	cmd=g_malloc0(256);
	for(i=0;i<7;i++)
		arg[i]=g_malloc0(64);
	
	strcpy(cmd,EZ_BIN);
	strcat(cmd,"/service");
	//printf("cmd: %s EZ_BIN: %s\n",cmd,EZ_BIN);
	//strcpy(cmd,"/usr/local/cluster/bin/service");
	strcpy(arg[0],"service");
	strcpy(arg[1],"-a");//strcat(arg," ");

	W   = lookup_widget(w_srv_add,"entry4");
	Service=g_malloc0(MAX_SERVICES_SIZE);
	strcpy(arg[2],gtk_entry_get_text(GTK_ENTRY(W)));
	
	W   = lookup_widget(w_srv_add,"entry5");
	Script=g_malloc0(MAX_SERVICES_SIZE);
	strcpy(arg[3],gtk_entry_get_text(GTK_ENTRY(W)));

	W   = lookup_widget(w_srv_add,"combo5");
	Primary=g_malloc0(MAX_NODENAME_SIZE);
	strcpy(arg[4],gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(W)->entry)));

	W   = lookup_widget(w_srv_add,"combo6");
	Secondary=g_malloc0(MAX_NODENAME_SIZE);
	strcpy(arg[5],gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(W)->entry)));

	W   = lookup_widget(w_srv_add,"entry6");
	Check_script=g_malloc0(MAX_SERVICES_SIZE);
	strcpy(arg[6],gtk_entry_get_text(GTK_ENTRY(W)));

	arg[7]=(gchar *)0;

	switch ((pid=fork())) {
		case -1:	log_txt("Can't fork command for creating service!");
				break;
		case 0:	{	/* Child */
				close(perr[0]);
        dup2(perr[1], STDERR_FILENO);
        close(perr[1]);

        close(pout[0]);
        dup2(pout[1], STDOUT_FILENO);
        close(pout[1]);

				execv(cmd,(gchar **)arg);
				break;
			}
		default:	/* Parent */{
				waitpid(pid, &status, 0);
		    close(perr[1]);
		    close(pout[1]);
    		out = fdopen(pout[0],"r");
    		err = fdopen(perr[0],"r");
				while((fgets(tmp,1024,err) != NULL) && (strlen(tmp) > 0))
					log_txt(tmp);
				while((fgets(tmp,1024,out) != NULL) && (strlen(tmp) > 0))
					log_txt(tmp);
		}
	}
	gtk_widget_destroy ((GtkWidget*)w_srv_add);
}


// Service->Add->CANCEL
void
on_button17_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_srv_add);
}





void on_srv_rm_activate (GtkMenuItem *menuitem,gpointer user_data)
{	
	GtkWidget *W;
	FILE *File;
	GList *List_services=NULL,*L;
	gchar *tmp,*lu[5],*read[16];
	gint i=0;

	for(i=0;i<6;i++)
		lu[i]=g_malloc0(64);
	
	tmp=g_malloc0(1024);

	if ( ! (File=fopen(getenv("EZ_SERVICES"),"r"))){
		log_txt("Unable to read EZ_SERVICES !");
		gtk_widget_destroy ((GtkWidget*)w_srv_rm);
		return;
	}
	//while( (fgets(tmp,1024,File) != NULL) && (strlen(tmp)>0)){
	while(fscanf(File,"%s %s %s %s %s",lu[0],lu[1],lu[2],lu[3],lu[4]) != -1){
		//printf("read: %s\n",lu[0]);
		read[i]=g_malloc0(64);
		strcpy(read[i],lu[0]);
		List_services = g_list_append(List_services,read[i]);
		i++;
	}
	
	w_srv_rm = create_w_srv_rm();  
	W  = lookup_widget (GTK_WIDGET (w_srv_rm), "combo7");  
	L=g_list_sort(List_services,(GCompareFunc)strcmp);
	List_services=L;	
	
	if (List_services != NULL){
		gtk_combo_set_popdown_strings(GTK_COMBO(W),List_services);
		gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(W)->entry), FALSE);
	}
	gtk_widget_show (w_srv_rm);
 	gtk_grab_add(w_srv_rm);
}


void on_button18_clicked (GtkButton *button,gpointer user_data)
{
	GtkWidget *W;
	gchar *cmd,*arg[4];
	gint i=0,pid,status;
	gint pout[2], perr[2];
	FILE *out,*err;
	gchar *tmp;

	tmp=g_malloc0(1024);
	pipe(pout);
	pipe(perr);

	cmd=g_malloc0(256);
	for(i=0;i<3;i++)
		arg[i]=g_malloc0(64);
	strcpy(cmd,EZ_BIN);
	strcat(cmd,"/service");
	strcpy(arg[0],"service");
	strcpy(arg[1],"-r");
	W   = lookup_widget(w_srv_rm,"combo7");
	strcpy(arg[2],gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(W)->entry)));
	arg[3]=(gchar *)0;
		
	switch ((pid=fork())) {
		case -1:	log_txt("Can't fork command for deleting service!");
				break;
		case 0:	{	/* Child */
				close(perr[0]);
        dup2(perr[1], STDERR_FILENO);
        close(perr[1]);
        close(pout[0]);
        dup2(pout[1], STDOUT_FILENO);
        close(pout[1]);
				execv(cmd,(gchar **)arg);
				break;
			}
		default:	/* Parent */{
				waitpid(pid, &status, 0);
		    close(perr[1]);
		    close(pout[1]);
    		out = fdopen(pout[0],"r");
    		err = fdopen(perr[0],"r");
				while((fgets(tmp,1024,err) != NULL) && (strlen(tmp) > 0))
					log_txt(tmp);
				while((fgets(tmp,1024,out) != NULL) && (strlen(tmp) > 0))
					log_txt(tmp);
		}
	}
	gtk_widget_destroy ((GtkWidget*)w_srv_rm);
}


void on_button19_clicked (GtkButton *button,gpointer user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_srv_rm);
}



void
on_srv_list_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *W;
GList *list=NULL;
gchar *tab[5];
gchar *data[5]={NULL,NULL,NULL,NULL,NULL};
gint i,j;

	w_srv_list = create_w_srv_list();  
	W  = lookup_widget (GTK_WIDGET (w_srv_list), "clist4");  
	list=get_services_list();

	for (i=0;i<(g_list_length(list)/5);i++){
		gtk_clist_append((GtkCList *)W,data);
		for(j=0;j<5;j++){
			tab[j]=g_malloc0(80);
			strncpy(tab[j],g_list_nth_data(list,(i*5)+j),80);
			gtk_clist_set_text((GtkCList *)W,i,j,tab[j]);
		}
	}
	gtk_widget_show (w_srv_list);
 	gtk_grab_add(w_srv_list);
	
}


void on_button20_clicked (GtkButton *button,gpointer user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_srv_list);
}

/*
gint
signal_handler_event(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
  if (GTK_IS_LIST_ITEM(widget) && (event->type==GDK_2BUTTON_PRESS || event->type==GDK_3BUTTON_PRESS) ) {
    printf("I feel %s clicked on button %d\n",
           event->type==GDK_2BUTTON_PRESS ? "double" : "triple",
           event->button);
  }

  return FALSE;
}


void on_button21_clicked (GtkButton *button,gpointer user_data)
{
		gtk_widget_destroy ((GtkWidget*)w_srv_list);
}


void
on_clist4_select_row                   (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	//printf("Selected row %d \n",row);
}
*/
