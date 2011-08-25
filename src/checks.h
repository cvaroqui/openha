#include <glib.h>
#include <stdlib.h>

gboolean check_node(gchar *);
gboolean check_type(gchar *);
gboolean check_interface(gchar*,gchar*,gchar*,gchar*);
gboolean check_address(gchar*,gchar*);
gboolean check_port(gint);
gboolean check_timeout(gchar*);

// Check if the specified nodename is defined in the configuration file, and is known.
gboolean check_node(gchar *name){
	FILE *File;
	GList *list_nodes=NULL;
	gint i;
	gboolean found=FALSE;

	if ((File = fopen(EZ_NODES,"r")) != NULL) {
		list_nodes = get_liste(File,1);
		fclose(File);
	}
	else {
		return TRUE;
	}
	for (i=0;i<(g_list_length(list_nodes));i++){
		if (g_strcasecmp((g_list_nth_data(list_nodes,i)),name)==0){
#ifdef DEBUG
printf("Node %s found in $EZ_NODES\n",name);
#endif
			found=TRUE;
			i=g_list_length(list_nodes);
		}
	}
	if (!found){
		fprintf(stderr,"Node %s not found in EZ_NODES configuration file.\n",name);
		return FALSE;
	}
	else{
#ifdef DEBUG
printf("Node %s found.\n",name);
#endif
		return TRUE;
	}
}
gboolean check_type(gchar *type){

	if ( (g_strcasecmp(type,"net")==0) || (g_strcasecmp(type,"disk")==0) || (g_strcasecmp(type,"dio")==0)){
#ifdef DEBUG
printf("Type %s ok.\n",type);
#endif
		return TRUE;
	}
	else {
		fprintf(stderr,"Type %s not valid (must be disk, dio or net).\n",type);
		return FALSE;
	}
}
gboolean check_interface(gchar *type, gchar *interface,gchar *address,gchar *port){
	gint rc;
	struct in_addr mreq;
	FILE *File;
	gint content[4];
	gint port_value,addr;

	if (g_strcasecmp(type,"net")==0){
		if ((interface==NULL) || (address==NULL) || (port==NULL)){
			fprintf(stderr,"Need interface address and port.\n");
			return FALSE;
		}
	 	memset(&mreq, 0, sizeof(mreq));
 		rc = if_getaddr(interface, &mreq);
	 	if (rc == -1){
			//strcpy(message," failed. Bad interface name ? \n");
			//halog(LOG_ERR,"if_getaddr: ",message);
			perror("set_mcast_if failed ");
			fprintf(stderr,"Bad interface name ?\n");
			return FALSE;
		}
		else {
#ifdef DEBUG
printf("addr: %s\n",address);
#endif
			sscanf(address,"%d.%d.%d.%d",&content[0],&content[1],&content[2],&content[3]);
			if (	((content[0]>=224)	&& (content[0]<240)) &&
				((content[1]>=0) 	&& (content[1]<256)) &&
				((content[2]>=1) 	&& (content[2]<256)) &&
				((content[3]>=0) 	&& (content[3]<255)))	{
#ifdef DEBUG
printf("OK, valid Multicast address.\n");
#endif
				sscanf(port,"%d",&port_value);
				if (check_port(port_value)){
#ifdef DEBUG
printf("OK, port value.\n");
#endif
					return TRUE;
				}
				else {
					fprintf(stderr,"BAD port value.\n");
					return FALSE;
				}
			}
			else{
				printf("BAD Multicast address.\n");
				return FALSE;
			}
		}
	}
	if (g_strcasecmp(type,"disk")==0 || g_strcasecmp(type,"dio")==0){
		if ((interface==NULL) || (address==NULL) || (port!=NULL)){
			fprintf(stderr,"Need interface and address.\n");
			return FALSE;
		}
		if ((File = fopen(interface,"rw")) != NULL) {
#ifdef DEBUG
printf("device %s checked ...\n",interface);
printf("OK, device @ value.\n");
#endif
			fclose(File);
			sscanf(address,"%d",&addr);
			return TRUE;
			
		}
		else {
			fprintf(stderr,"Unable to check device %s.\n",interface);
			return FALSE;
		}
	}
	return FALSE;
}

gboolean check_disk_addr(gint value){
	if ((value < 0) || (value > 1024)){
		fprintf(stderr,"BAD block offset. Must be between 0 and 1023.\n");
		return FALSE;
	}
	else {
#ifdef DEBUG
printf("OK, block offset.\n");
#endif
		return TRUE;
	}
}

gboolean check_port(gint value){
	if ((value <= 1025) || (value >=  65535)){
		fprintf(stderr,"BAD port number: %d. Must be between 1025 and 65535.\n",value);
		return FALSE;
	}
	else {
#ifdef DEBUG
printf("OK, port number.\n");
#endif
		return TRUE;
	}
}

gboolean check_timeout(gchar *timeout){
guint value;
sscanf((const char*)timeout,"%d",&value);
	if ((value < 2) || (value > 120)){
		fprintf(stderr,"BAD timeout value. Must be between 2 and 120 seconds.\n");
		return FALSE;
	}
	else {
#ifdef DEBUG
printf("OK, timeout number.\n");
#endif
		return TRUE;
	}
}




