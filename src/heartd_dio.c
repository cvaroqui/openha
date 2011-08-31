/*
* Copyright (C) 1999,2000,2001,2002,2003,2004 Samuel Godbillot <sam028@users.sourceforge.net>
* Copyright (C) 2011 Christophe Varoqui <christophe.varoqui@opensvc.com>
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
#define _GNU_SOURCE

#include <sys/types.h>   /* include files for IP Sockets */
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <glib.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cluster.h>
#include <sockhelp.h>

#define BUFSIZE   1024
#define TTL_VALUE 8 
#define BLKSIZE 512

GList *list_services=NULL;
GList *list_state=NULL;
struct sendstruct to_send;
void clean_tab();
gint get_node_status(gchar*);
gint write_dio(gint fd, struct sendstruct,gchar *, gint );
void sighup();
gchar *progname;

int main(argc, argv)
int argc;
char *argv[]; {

gint status, i, fd,address;
struct utsname tmp_name;
gchar *message, *FILE_KEY, *device;
gchar	*my_pid;
gint fd2;
gchar **NEW_KEY;
gchar *n;

	message=g_malloc0(160);
	device=g_malloc0(128);
	my_pid=g_malloc0(6);

	if (argc != 3) {
		fprintf(stderr,"Usage: heartd_dio [device] offset \n");
		exit(-1);
	}
	daemonize("heartd_dio");
	Setenv("PROGNAME","heartd_dio",1);

	address=atoi(argv[2]);
	NEW_KEY=g_strsplit(argv[1], "/", 10);
        n=g_strjoinv(".", NEW_KEY),

	FILE_KEY=g_strconcat(getenv("EZ_LOG"),"/proc/", n, ".", argv[2],".key",NULL);
        g_strfreev(NEW_KEY);
        g_free(n);

	if ((fd=open(FILE_KEY,O_RDWR|O_CREAT,00644)) == -1){
		printf("key: %s\n",FILE_KEY);
		strcpy(message,"Error: unable to open key file");
		halog(LOG_ERR, "heartd_dio", message);
		exit(-1);
	}	
	if (lockf(fd,F_TLOCK,0) != 0){
		strcpy(message,"Error: unable to lock key file");
		halog(LOG_ERR, "heartd_dio", message);
		exit(-1);
	}

	setpriority(PRIO_PROCESS,0,-15);
	to_send.hostid = gethostid();     
	uname(&tmp_name);
	strncpy(to_send.nodename,tmp_name.nodename,MAX_NODENAME_SIZE);
	//signal(SIGALRM,sighup);
	fd2 = open(argv[1], O_DIRECT|O_RDWR);
	if (fd2 < 0){
		strcpy(message,"Error: unable to open device");
		halog(LOG_ERR, "heartd_dio", message);
		exit(-1);
	}
	while(1) {
		signal(SIGALRM,sighup);
		i=0;		
		to_send.elapsed = Elapsed();     
		status=get_node_status(to_send.nodename);
		i = write_dio(fd2, to_send, device, address);
		if (i != 0) {
			strcpy(message,"write_dio() failed");
			halog(LOG_ERR, "heartd_dio",message);
		}
		alarm(1);
		pause();
   }
 } /* end main() */  

gint write_dio(gint fd, struct sendstruct to_write, gchar *device, gint where){
	void *buff;
	int r;
	char message[160];
	r = posix_memalign(&buff, BLKSIZE, BLKSIZE);
	if (r != 0) {
		strcpy(message, "write_dio() posix_memalign failed");
		halog(LOG_ERR, "heartd_dio", message);
		return 1;
	}
	memcpy(buff, &to_write, sizeof(struct sendstruct));
	lseek(fd, (where*BLKSIZE), SEEK_SET);
	r = write(fd, buff, BLKSIZE);
	if (r != BLKSIZE) {
		strcpy(message, "write_dio() write failed");
		halog(LOG_ERR, "heartd_dio", message);
		return 1;
	}
	free(buff);
	return 0;
}


gint get_node_status(gchar *nodename){
	FILE *EZ_SERVICES, *FILE_STATE;
	guint i,j,list_size,LIST_NB_ITEM=5;
	gchar *FILE_NAME, STATE, *service;

	if (getenv("EZ_SERVICES") == NULL) {
		printf("ERROR: environment variable EZ_SERVICES not defined !!!\n");
		return 0;
  	}
	if ((EZ_SERVICES = fopen(getenv("EZ_SERVICES") ,"r")) == NULL) {
		//No service(s) defined (unable to open $EZ_SERVICES file)
		return 0;
	}
	list_services=get_services_list();
	fclose(EZ_SERVICES);
	list_size = g_list_length(list_services)/LIST_NB_ITEM;
	clean_tab();
	j=0;
	for(i=0;i<list_size;i++){
		service=malloc(MAX_SERVICES_SIZE);	
		strcpy(service,(gchar*)g_list_nth_data(list_services,(i*LIST_NB_ITEM)));
		//printf("Service: %s number %d\n",service,i);
		if (is_primary(nodename,service)||is_secondary(nodename,service)){
			FILE_NAME = g_strconcat(getenv("EZ"),"/services/",service,"/STATE.",nodename,NULL);
			if ((FILE_STATE=fopen((char*)FILE_NAME,"r")) == NULL) {
				perror("No service(s) defined (unable to open SERVICE STATE file)");
				STATE = '8';
			} else {
				STATE = fgetc(FILE_STATE);
				fclose(FILE_STATE);
			}
			g_free(FILE_NAME);
			strcpy(to_send.service_name[j],service);
			to_send.service_state[j]=STATE;
			to_send.up=TRUE;
			j++;
		}
		g_free(service);
	}
	g_list_free(list_services);
	list_services=list_state=NULL;
	return list_size;
}

void clean_tab() {
	int i;
	for(i=0;i<MAX_SERVICES;i++){
		bzero(to_send.service_name[i], MAX_SERVICES_SIZE);
	}
	bzero(to_send.service_state, MAX_SERVICES);
}

void sighup() {
}

