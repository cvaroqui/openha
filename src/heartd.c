/*
* Copyright (C) 1999-2004 Samuel Godbillot <sam028@users.sourceforge.net>
* Copyright (C) 2014 Christophe Varoqui <christophe.varoqui@opensvc.com>
* Copyright (C) 2014 Arnaud Veron <arnaud.veron@opensvc.com>
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
#include <sys/types.h>
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

gint shmid;
struct sendstruct to_send;
void sighup();
void sigterm();

int
main(argc, argv)
int argc;
char *argv[];
{

	struct sockaddr_in stLocal, stTo;
	gint s;
	struct ip_mreq stMreq;
	gint addr_size, i, port = -1, fd;
	unsigned char ttl = 6;
	struct utsname tmp_name;
	gchar *FILE_KEY;
	gchar ADDR[15];
	pid_t pid;
	key_t key;
	gchar *shm, *SHM_KEY;


	if (argc != 4) {
		fprintf(stderr, "Usage: heartd interface address port\n");
		exit(-1);
	}
	signal(SIGTERM, sigterm);
	signal(SIGUSR1, signal_usr1_callback_handler);
	signal(SIGUSR2, signal_usr2_callback_handler);
	daemonize("heartd");
	snprintf(progname, MAX_PROGNAME_SIZE, "heartd");

	strncpy(ADDR, argv[2], 15);
	FILE_KEY =
	    g_strconcat(getenv("EZ_LOG"), "/proc/", ADDR, "-", argv[3], "-",
			argv[1], ".key", NULL);
	pid = getpid();

	if ((fd = open(FILE_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		halog(LOG_ERR, "unable to open key file");
		g_free(FILE_KEY);
		exit(-1);
	}
	g_free(FILE_KEY);
	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock key file");
		exit(-1);
	}

	port = atoport(argv[3], "udp");
	if (port == -1) {
		halog(LOG_ERR, "unable to find service: %s", argv[3]);
		exit(EXIT_FAILURE);
	}

	/* get a datagram socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);

	/* avoid EADDRINUSE error on bind() */
	SHM_KEY =
	    g_strconcat(getenv("EZ_LOG"), "/proc/", ADDR, "-", argv[3], "-",
			argv[1], ".key", NULL);

	if ((fd = open(SHM_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		halog(LOG_ERR, "unable to open SHMFILE");
		g_free(SHM_KEY);
		exit(-1);
	}

	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock SHMFILE");
		g_free(SHM_KEY);
		exit(-1);
	}

	key = ftok(SHM_KEY, 0);
	g_free(SHM_KEY);
	if (port == -1) {
		halog(LOG_ERR, "invalid port %s", argv[3]);
		exit(-1);
	}

	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0644)) < 0) {
		halog(LOG_ERR, "shmget failed");
		//perror("shmget");
		exit(-1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		halog(LOG_ERR, "shmat failed");
		//perror("shmat");
		exit(-1);
	}

	/* name the socket */
	stLocal.sin_family = AF_INET;
	stLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocal.sin_port = htons(port);
	stLocal.sin_port = port;
	bind(s, (struct sockaddr *) &stLocal, sizeof (stLocal));

	if (set_mcast_if(s, argv[1]) < 0) {
		halog(LOG_ERR, "Error setting outbound mcast interface: %s", argv[1]);
		exit(-1);
	}

	/* join the multicast group. */
	stMreq.imr_multiaddr.s_addr = inet_addr(argv[2]);
	stMreq.imr_interface.s_addr = INADDR_ANY;
	setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &stMreq,
		   sizeof (stMreq));

	/* assign our destination address */
	stTo.sin_family = AF_INET;
	stTo.sin_addr.s_addr = inet_addr(argv[2]);
	stTo.sin_port = htons(port);
	stTo.sin_port = port;

	/* set TTL to traverse up to multiple routers */
	setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof (ttl));
	addr_size = sizeof (struct sockaddr_in);
	setpriority(PRIO_PROCESS, 0, -15);
	to_send.hostid = gethostid();
	uname(&tmp_name);
	strncpy(to_send.nodename, tmp_name.nodename, MAX_NODENAME_SIZE);
	pid = getpid();
	to_send.pid = pid;

	while (1) {
		signal(SIGALRM, sighup);
		signal(SIGTERM, sigterm);
		signal(SIGUSR1, signal_usr1_callback_handler);
		signal(SIGUSR2, signal_usr2_callback_handler);

		i = 0;
		get_node_status(&to_send);
		to_send.elapsed = Elapsed();
		to_send.up = TRUE;
		to_send.pid = pid;
		memcpy(to_send.port, argv[3], 5);
		memcpy(to_send.addr, ADDR, 15);
		memcpy(shm, &to_send, sizeof (to_send));

		i = sendto(s, (const void *) &to_send, sizeof (to_send),
			   0, (struct sockaddr *) &stTo, addr_size);
		if (i < 0) {
			halog(LOG_ERR, "sento() failed");
		} else {
			halog(LOG_DEBUG, "[main] sendto() OK : %zu bytes From %s:%d",
				 sizeof (to_send),
				 inet_ntoa(stLocal.sin_addr),
				 ntohs(stLocal.sin_port));
			halog(LOG_DEBUG, "[main] sendto() OK : Dest %s:%d",
				 inet_ntoa(stTo.sin_addr),
				 ntohs(stTo.sin_port));
		}

		alarm(1);
		pause();
	}
}				/* end main() */


void
sighup()
{
}

void
sigterm()
{
	halog(LOG_INFO, "SIGTERM received, exiting gracefully ...");
	(void) shmctl(shmid, IPC_RMID, NULL);
	exit(0);
}
