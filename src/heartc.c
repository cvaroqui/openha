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
#include <fcntl.h>   
#include <unistd.h>   
#include <glib.h>   
#include <syslog.h>   
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>   /* include files for IP Sockets */
#include <sys/stat.h>   
#include <sys/socket.h>
#include <sys/shm.h>
#include <glib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <cluster.h>
#include <sockhelp.h>

#define BUFSIZE   sizeof(struct sendstruct)*MAX_SERVICES
#define TTL_VALUE 32
#define UNKNOWN '8'

void sighup();
void sigterm();
gchar *S,*shm;	
gint flag = 0, shmid;
gchar ADDR[15];
struct sendstruct to_recV;
struct stat *f_stat;
gboolean FIRST=TRUE,DOWN=FALSE;
gchar *progname=NULL;

gint main(argc, argv)
gint argc;
gchar *argv[]; {

	struct sockaddr_in stLocal, stTo, stFrom;
	struct ip_mreq stMreq;
	gint fd, s, timeout, i, j, last, port = -1;
        socklen_t addr_size;
	key_t key;
	gchar *SHM_KEY;
	gboolean was_down=TRUE;
	gchar *message;
	gint iTmp;

	message=g_malloc0(80);
//	SHM_KEY=malloc(256);
	signal(SIGTERM,sigterm);
	daemonize("heartc");
	Setenv("PROGNAME","heartc",1);
	
	if (argc != 5) {
		fprintf(stderr,"Usage: heartc interface address port timeout \n");
		fprintf(stderr,"Where adress is the address to send\n");
		fprintf(stderr,"and port the port number or service name to\n");
		fprintf(stderr,"send to.\n");
		exit(-1);
	}
	strncpy(ADDR,argv[2],15);
	port = atoport(argv[3], "udp");
	
	if ( (timeout = atoi(argv[4])) < 2){
		message=g_strconcat("timeout must be > 1 second\n",NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);	
	}

	f_stat=malloc(sizeof(stat));

	if (getenv("EZ_LOG") == NULL) {
		message=g_strconcat("environment variable EZ_LOG not defined ...\n",NULL);
		halog(LOG_ERR, "heartc: ", message);
		exit(-1);
	}
	setpriority(PRIO_PROCESS,0,-15);
	SHM_KEY=g_strconcat(getenv("EZ_LOG"),"/proc/",ADDR,"-",argv[3],"-",argv[1],".key",NULL);
		
	if ((fd=open(SHM_KEY,O_RDWR|O_CREAT,00644)) == -1){
		message=g_strconcat("Error: unable to open SHMFILE \n",NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}	

	if (lockf(fd,F_TLOCK,0) != 0){
		message=g_strconcat("Error: unable to lock SHMFILE\n",NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}

	key=ftok(SHM_KEY,0);
	if (port == -1) {
		message=g_strconcat("Error: invalid port ",argv[3],"\n",NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}

	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0644)) < 0) {
		message=g_strconcat("shmget failed\n",NULL);
		halog(LOG_ERR, "heartc", message);
		//perror("shmget");
		exit(-1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		message=g_strconcat("shmat failed\n", NULL);
		halog(LOG_ERR, "heartc", message);
		//perror("shmat");
		exit(-1);
	}

	/* get a datagram socket */
	if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		message=g_strconcat("getting socket\n", NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}

	/* avoid EADDRINUSE error on bind() */ 
	iTmp = 1;
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&iTmp, sizeof(iTmp)) < 0){
		message=g_strconcat("Error setsockopt(SO_REUSEADDR)\n",NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}      

	/* join the multicast group. */
	stMreq.imr_multiaddr.s_addr = inet_addr(argv[2]);
	i = if_getaddr(argv[1],&stMreq.imr_interface);
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) != 0){
					fprintf(stderr,"%d\n",inet_addr(argv[2]));
					perror("add_member");
	}
	/* assign our destination address */
	stTo.sin_family =      AF_INET;
	stTo.sin_addr.s_addr = inet_addr(argv[2]);
	stTo.sin_port =        htons(port);
	stTo.sin_port =        port;
	addr_size = sizeof(struct sockaddr_in);

	/* assign interface */
	if (set_mcast_if(s, argv[1]) < 0) {
		message=g_strconcat("Error setting outbound mcast interface: ",argv[1], "\n", NULL);
		halog(LOG_ERR, "heartc", message);
		exit(-1);
	}
	/* name the socket */
	stLocal.sin_family =   AF_INET;
	stLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocal.sin_port =     htons(port);
	stLocal.sin_port =     port;
	if (bind(s, (struct sockaddr*) &stLocal, sizeof(stLocal)) != 0){
		perror("bind");
		message=g_strconcat("bind failed\n",NULL);
		halog(LOG_ERR, "heartc", message);
		//perror("bind");
		exit(1);
	}

	to_recV.up = FALSE;
	// si elapsed est tjs == 0 apres, c'est que heartc n'a pas encore pu contacter un heartd
	to_recV.elapsed = 0;
	memcpy(to_recV.port,argv[3],5);
	memcpy(to_recV.addr,ADDR,15);
	memcpy(shm, &to_recV, sizeof(to_recV));

	while(TRUE) {
		signal(SIGALRM,sighup);
		if (flag == 1) {
			alarm(0);
			flag=0;
			was_down = TRUE;
		}
		else 
		{
			alarm(timeout); 
		}
		i = recvfrom(s, (void *)&to_recV, BUFSIZE, 0,
		    (struct sockaddr*)&stFrom, &addr_size);
	 	if ((i < 0) && (flag == 0)) {
			message=g_strconcat("recvfrom failed\n",NULL);
			halog(LOG_ERR, "heartc", message);
	   	exit(-1);
	 	}
		if (i>0){
			for(j=0;j<MAX_SERVICES;j++) {
				if (strlen(to_recV.service_name[j]) == 0){
					last = j;
					j = MAX_SERVICES;}
				else{
					write_status(to_recV.service_name[j],
										 to_recV.service_state[j],
										 to_recV.nodename);
				}
			}
	 		to_recV.elapsed = Elapsed();
	 		to_recV.up = TRUE;
			memcpy(to_recV.port,argv[3],5);
	 		memcpy(to_recV.addr,ADDR,15);
	 		memcpy(shm, &to_recV, sizeof(to_recV));
			if (was_down == TRUE){
				message=g_strconcat("peer on @ ",ADDR," up !\n",NULL);
				halog(LOG_WARNING,"heartc",message);
				g_free(message);
				was_down = FALSE;
				DOWN = FALSE;
			}
		}
  }
} /* end main() */  


void sighup(){
	gchar *message;
 	to_recV.up = FALSE;
	DOWN=TRUE;
 	memcpy(shm, &to_recV, sizeof(to_recV));
	message=g_strconcat("peer on @ ",ADDR," down !\n",NULL);
	halog(LOG_WARNING,"heartc",message);
	g_free(message);
	flag=1;
}


void sigterm(){
	gchar *message;
	message=g_strconcat("SIGTERM received, exiting gracefully ...\n",NULL);
	halog(LOG_INFO,"heartc",message);
	g_free(message);
	(void)shmctl(shmid,IPC_RMID,NULL);
	exit(0);
}

