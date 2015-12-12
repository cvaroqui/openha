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
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>		/* include files for IP Sockets */
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

#define TTL_VALUE 32
#define UNKNOWN '8'

void sighup();
void sigterm();

gchar *S, *shm;
gint flag = 0, shmid;
gchar ADDR[15];
struct sendstruct to_recV;
struct stat *f_stat;
gboolean FIRST = TRUE, DOWN = FALSE;

gint
main(argc, argv)
gint argc;
gchar *argv[];
{

	struct sockaddr_in stLocal, stFrom;
	struct ifreq ifr;
	//struct sockaddr_in stTo;
	gint fd, s, timeout, i, j, port = -1;
	socklen_t addr_size;
	key_t key;
	gchar *SHM_KEY;
	gboolean was_down = TRUE;
	gint iTmp;

	signal(SIGTERM, sigterm);
	signal(SIGUSR1, signal_usr1_callback_handler);
	signal(SIGUSR2, signal_usr2_callback_handler);

	daemonize("heartc_unicast");
	snprintf(progname, MAX_PROGNAME_SIZE, "heartc_unicast");

	if (argc != 5) {
		fprintf(stderr,
			"Usage: heartc_unicast interface address port timeout \n");
		fprintf(stderr, "Where adress is the address to send to\n");
		fprintf(stderr,
			"and port the port number or service name to");
		fprintf(stderr, "send to.\n");
		exit(-1);
	}
	strncpy(ADDR, argv[2], 15);
	port = atoport(argv[3], "udp");

	if ((timeout = atoi(argv[4])) < 2) {
		halog(LOG_ERR, "timeout must be > 1 second");
		exit(-1);
	}

	f_stat = malloc(sizeof (stat));

	if (getenv("EZ_LOG") == NULL) {
		halog(LOG_ERR, "environment variable EZ_LOG not defined ...");
		exit(-1);
	}
	setpriority(PRIO_PROCESS, 0, -15);
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

	/* get a datagram socket */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		halog(LOG_ERR, "getting socket");
		exit(-1);
	}

	/* avoid EADDRINUSE error on bind() */
	iTmp = 1;
	if (setsockopt
	    (s, SOL_SOCKET, SO_REUSEADDR, (void *) &iTmp, sizeof (iTmp)) < 0) {
		halog(LOG_ERR, "Error setsockopt(SO_REUSEADDR)");
		exit(-1);
	}


	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", argv[1]);

	#ifdef __linux__
	if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		halog(LOG_ERR, "Error setsockopt(SO_BINDTODEVICE)");
		exit(-1);
	}
	#endif

	/* assign our destination address */
	addr_size = sizeof (struct sockaddr_in);

	/* name the socket */
	stLocal.sin_family = AF_INET;
	stLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocal.sin_port = port;
	if (bind(s, (struct sockaddr *) &stLocal, sizeof (stLocal)) != 0) {
		perror("bind");
		halog(LOG_ERR, "bind failed");
		//perror("bind");
		exit(1);
	}

	to_recV.up = FALSE;
	// si elapsed est tjs == 0 apres, c'est que heartc n'a pas encore pu contacter un heartd
	to_recV.elapsed = 0;
	memcpy(to_recV.port, argv[3], 5);
	memcpy(to_recV.addr, ADDR, 15);
	memcpy(shm, &to_recV, sizeof (to_recV));

	while (TRUE) {
		signal(SIGTERM, sigterm);
		signal(SIGUSR1, signal_usr1_callback_handler);
		signal(SIGUSR2, signal_usr2_callback_handler);
		signal(SIGALRM, sighup);

		if (flag == 1) {
			alarm(0);
			flag = 0;
			was_down = TRUE;
		} else {
			alarm(timeout);
		}
		i = recvfrom(s, (void *) &to_recV, sizeof(struct sendstruct),
			     0, (struct sockaddr *) &stFrom, &addr_size);
		if ((i < 0) && (flag == 0)) {
			halog(LOG_ERR, "recvfrom failed: %s", strerror(errno));
			continue;
		}
		if (i > 0) {
			halog(LOG_DEBUG, "[main] recvfrom() OK : %zu bytes From %s:%d",
				 sizeof (struct sendstruct),
				 inet_ntoa(stFrom.sin_addr),
				 ntohs(stFrom.sin_port));

			for (j = 0; j < MAX_SERVICES; j++) {
				if (strlen(to_recV.service_name[j]) == 0) {
					j = MAX_SERVICES;
				} else {
					halog(LOG_DEBUG, "[main] svc #%d name=[%s] state=[%d] node=[%s] ela=[%u]",
						 j,
						 to_recV.service_name[j],
						 to_recV.service_state[j],
						 to_recV.nodename,
						 to_recV.elapsed);
					write_status(to_recV.service_name[j],
						     to_recV.service_state[j],
						     to_recV.nodename);
				}
			}
			to_recV.elapsed = Elapsed();
			to_recV.up = TRUE;
			memcpy(to_recV.port, argv[3], 5);
			memcpy(to_recV.addr, ADDR, 15);
			memcpy(shm, &to_recV, sizeof (to_recV));
			if (was_down == TRUE) {
				halog(LOG_WARNING, "peer on @ %s up !", ADDR);
				was_down = FALSE;
				DOWN = FALSE;
			}
		}
	}
}				/* end main() */

void
sighup()
{
	to_recV.up = FALSE;
	DOWN = TRUE;
	memcpy(shm, &to_recV, sizeof (to_recV));
	halog(LOG_WARNING, "peer on @ %s down !", ADDR);
	flag = 1;
}

void
sigterm()
{
	halog(LOG_INFO, "SIGTERM received, exiting gracefully ...");
	(void) shmctl(shmid, IPC_RMID, NULL);
	exit(0);
}
