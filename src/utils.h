#include <gtk/gtk.h>
#include <time.h>
#include <cluster.h>

 
gboolean init_var(){
	if ((EZ=getenv("EZ")) == NULL){
		printf("Error: variable EZ not defined\n");
		return FALSE;
	}	
	if ((EZ_BIN=getenv("EZ_BIN")) == NULL){
		printf("Error: variable EZ_BIN not defined\n");
		return FALSE;
	}	
	if ((EZ_MONITOR=getenv("EZ_MONITOR")) == NULL){
		printf("Error: variable EZ_MONITOR not defined\n");
		return FALSE;
	}	
	if ((EZ_SERVICES=getenv("EZ_SERVICES")) == NULL){
		printf("Error: variable EZ_SERVICES not defined\n");
		return FALSE;
	}	
	if ((EZ_LOG=getenv("EZ_LOG")) == NULL){
		printf("Error: variable EZ_LOG not defined\n");
		return FALSE;
	}	
	if ((EZ_NODES=getenv("EZ_NODES")) == NULL){
		printf("Error: variable EZ_NODES not defined\n");
		return FALSE;
	}	
	if ((PIX_PATH=getenv("EZ")) == NULL){
			printf("Error: variable EZ not defined\n");
			return FALSE;
	}
	strcat(PIX_PATH,"/pixmaps");
	return TRUE;
}

void init_pix(){

	gchar *xpm[16];
	gint i;

	for(i=0;i<16;i++){
		xpm[i]=g_malloc0(sizeof(PIX_PATH)+16);
		strcpy(xpm[i],PIX_PATH);
	}


	strcat(xpm[0],"/down.xpm");	
	strcat(xpm[1],"/up.xpm");	
	strcat(xpm[2],"/na.xpm");	
	strcat(xpm[3],"/red.xpm");	
	strcat(xpm[4],"/green.xpm");	
	strcat(xpm[5],"/blue.xpm");	
	strcat(xpm[6],"/orange.xpm");	
	strcat(xpm[7],"/black.xpm");	
	strcat(xpm[8],"/started.xpm");	
	strcat(xpm[9],"/stopped.xpm");	
	strcat(xpm[10],"/frozen_start.xpm");	
	strcat(xpm[11],"/frozen_stop.xpm");	
	strcat(xpm[12],"/starting.xpm");	
	strcat(xpm[13],"/stopping.xpm");	
	strcat(xpm[14],"/unknown.xpm");	
	strcat(xpm[15],"/yellow.xpm");	
	printf("XPM :%s\n",xpm[3]);
  down_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[0]);
  up_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[1]);
  na_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[2]);
  red_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[3]);
  green_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[4]);
  blue_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[5]);
  orange_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[6]);
  black_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[7]);
  started_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[8]);
  stopped_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[9]);
  frozen_start_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[10]);
  frozen_stop_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[11]);
  starting_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[12]);
  stopping_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[13]);
  unknown_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[14]);
  yellow_pix = gdk_pixmap_colormap_create_from_xpm(
		NULL, gdk_colormap_get_system(), &transparent,NULL, xpm[15]);
}

gboolean fill_tab_services(gpointer user_data){
	GList *list=NULL;	
	gchar *data[5]={NULL,NULL,NULL,NULL,NULL};
	gchar *service,*primary,*secondary,*pstate,*sstate;
	gint i,status_p,status_s;
	GdkPixmap *pix[MAX_STATE];

	pix[0]=stopped_pix;
	pix[1]=started_pix;
	pix[2]=stopping_pix;
	pix[3]=starting_pix;
	pix[6]=frozen_stop_pix;
	pix[7]=frozen_start_pix;

	list=get_services_list();
	gtk_clist_clear((GtkCList *)user_data);

	for (i=0;i<(g_list_length(list)/5);i++){
		service=g_malloc0(80);
		primary=g_malloc0(80);
		secondary=g_malloc0(80);
		pstate=g_malloc0(16);
		sstate=g_malloc0(16);
		service=strncpy(service,g_list_nth_data(list,(i*5)),80);
		primary=strncpy(primary,g_list_nth_data(list,(i*5)+2),80);
		secondary=strncpy(secondary,g_list_nth_data(list,(i*5)+3),80);
		status_p=get_status(list,primary,service);
		strcpy(pstate,VAL[status_p]);
		status_s=get_status(list,secondary,service);
		strcpy(sstate,VAL[status_s]);
		
		gtk_clist_append((GtkCList *)user_data,data);
		gtk_clist_set_text((GtkCList *)user_data,i,0,service);
		gtk_clist_set_text((GtkCList *)user_data,i,1,primary);
		gtk_clist_set_text((GtkCList *)user_data,i,3,secondary);
		
		switch (status_p){
			case 0:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, orange_pix,transparent);
				break;
			case 2:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, green_pix,transparent);
				break;
			case 1:
			case 3:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, blue_pix,transparent);
				break;
			case 4:
			case 5:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, red_pix,transparent);
				break;
			case 6:
			case 7:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, blue_pix,transparent);
				break;
			case 8:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,pstate,10, black_pix,transparent);
				break;
			}
		switch (status_s){
			case 0:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, orange_pix,transparent);
				break;
			case 2:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, green_pix,transparent);
				break;
			case 1:
			case 3:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, blue_pix,transparent);
				break;
			case 4:
			case 5:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, red_pix,transparent);
				break;
			case 6:
			case 7:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, blue_pix,transparent);
				break;
			case 8:
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 4,sstate,10, black_pix,transparent);
				break;
			}
		//
		//gtk_clist_set_text((GtkCList *)user_data,i,3,pstate);
		//gtk_clist_set_text((GtkCList *)user_data,i,4,sstate);

		
		g_free(service);g_free(primary);g_free(secondary);
		g_free(pstate);g_free(sstate);
	}
	//gtk_clist_set_sort_column((GtkCList *)user_data, 0);
 	//gtk_clist_set_auto_sort((GtkCList *)user_data, TRUE);
 	gtk_clist_sort((GtkCList *)user_data);

	g_list_free(list);
	return TRUE;
}

gboolean fill_tab_monitor(gpointer user_data){
	gchar *FILE_KEY, *node,*date,*interface;
	gchar *name,*status;
	gint  i, j, shmid;
	GList *list_heart;
	gchar *data[4]={NULL,NULL,NULL,NULL}, *shm;
	FILE *File;
	key_t key;
	struct sendstruct tabinfo[MAX_HEARTBEAT];

	name 			= g_malloc0(MAX_NODENAME_SIZE);
	FILE_KEY	= g_malloc0(128);
	get_my_name(name);
	if ((File = fopen(EZ_MONITOR,"r")) == NULL) {
		printf("Warning: Nothing to monitor: unable to open $EZ_MONITOR\n");
		return TRUE;
	}
	strcpy(FILE_KEY,getenv("EZ_LOG"));
	strcat(FILE_KEY,"/proc/nmond.key");
	key=ftok(FILE_KEY,0);
	g_free(FILE_KEY);


	if ((shmid = shmget(key, sizeof(struct sendstruct)*MAX_HEARTBEAT,0444)) == -1){
		perror("shmget failed");
		exit(-1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat failed");
		exit(-1);
	}
	memcpy(tabinfo,shm,sizeof(tabinfo));
	list_heart = get_liste(File,LIST_NB_ITEM);
	gtk_clist_clear((GtkCList *)user_data);
	j=0;
	for (i=0;i<(g_list_length(list_heart)/5);i++){
		node=g_malloc0(MAX_NODENAME_SIZE);
		interface=g_malloc0(80);
		status=g_malloc0(8);
		date=g_malloc0(32);
		node=strncpy(node,g_list_nth_data(list_heart,(i*5)),80);
		interface=strncpy(interface,g_list_nth_data(list_heart,(i*5)+2),80);
		strcat(interface,":");
		strcat(interface,g_list_nth_data(list_heart,(i*5)+3));
		strcat(interface,":");
		strcat(interface,g_list_nth_data(list_heart,(i*5)+4));
		if (strcmp(name,node)==0){
			strcpy(status,"N/A");
			gtk_clist_append((GtkCList *)user_data,data);
			gtk_clist_set_text((GtkCList *)user_data,i,0,node);
			gtk_clist_set_text((GtkCList *)user_data,i,1,interface);
			gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,"N/A",10, green_pix, transparent);
		}
		else{
			strcat(date,(gchar *)ctime(&tabinfo[j].elapsed));
			if (tabinfo[j].up==TRUE)
				strcpy(status,"UP");
			else
				strcpy(status,"DOWN");
			gtk_clist_append((GtkCList *)user_data,data);
			gtk_clist_set_text((GtkCList *)user_data,i,0,node);
			gtk_clist_set_text((GtkCList *)user_data,i,1,interface);
			if (strcmp(status,"DOWN")==0){
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,"DOWN",10, red_pix, transparent);
			}
			else{
				gtk_clist_set_pixtext((GtkCList *)user_data, i, 2,"UP",10, green_pix, transparent);
			}
			gtk_clist_set_text((GtkCList *)user_data,i,3,date);
			j++;
		}
		
		g_free(node);g_free(interface);g_free(status);g_free(date);
	}
		
	g_list_free(list_heart);
	fclose(File);
	g_free(name);
	shmdt(shm);
	return TRUE;
}

