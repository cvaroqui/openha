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
#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
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

#define BUFSIZE   sizeof(struct sendstruct)*MAX_SERVICES
#define TTL_VALUE 32
#define BLKSIZE 512

void sighup();
void sigterm();
gint read_dio(gint, gint);
gchar *S, *shm;
gint address, flag = 0, shmid;
struct sendstruct to_recV;
struct stat *f_stat;
gboolean FIRST = TRUE;
gchar ADDR[64];

gint
main(argc, argv)
gint argc;
gchar *argv[];
{

	gint fd, timeout, j;
	key_t key;
	gchar *SHM_KEY;
	gboolean was_down = TRUE;
	gchar *FILE_KEY, *device;
	gint address;
	long old_elapsed = 0;
	gchar **NEW_KEY;
	gchar *n;
	int r;

	device = g_malloc0(128);

	SHM_KEY = malloc(256);
	signal(SIGTERM, sigterm);

	if (argc != 4) {
		fprintf(stderr, "Usage: heartc_dio [device] offset timeout \n");
		exit(-1);
	}
	daemonize("heartc_dio");
	snprintf(progname, MAX_PROGNAME_SIZE, "heartc_dio");

	strcpy(device, argv[1]);
	address = atoi(argv[2]);
	strncpy(ADDR, argv[1], 64);

	if ((timeout = atoi(argv[3])) < 2) {
		halog(LOG_ERR, "timeout must be > 1 second");
		exit(-1);
	}

	f_stat = malloc(sizeof (stat));

	if (getenv("EZ_LOG") == NULL) {
		halog(LOG_ERR, "environment variable EZ_LOG not defined ...");
		exit(-1);
	}
	setpriority(PRIO_PROCESS, 0, -15);
	NEW_KEY = g_strsplit(argv[1], "/", 10);
	n = g_strjoinv(".", NEW_KEY),
	    SHM_KEY =
	    g_strconcat(getenv("EZ_LOG"), "/proc/", n, ".", argv[2], ".key",
			NULL);
	g_strfreev(NEW_KEY);
	g_free(n);

	FILE_KEY = SHM_KEY;

	if ((fd = open(SHM_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		halog(LOG_ERR, "unable to open SHMFILE");
		exit(-1);
	}

	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock SHMFILE");
		exit(-1);
	}
	key = ftok(SHM_KEY, 0);
	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0644)) < 0) {
		halog(LOG_ERR, "shmget failed");
		exit(-1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		halog(LOG_ERR, "shmat failed");
		exit(-1);
	}
	if ((fd = open(FILE_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		printf("key: %s\n", FILE_KEY);
		halog(LOG_ERR, "unable to open key file");
		exit(-1);
	}
	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock key file");
		exit(-1);
	}
	fd = open(argv[1], O_DIRECT | O_RDONLY);
	if (fd < 0) {
		halog(LOG_ERR, "unable to open dio device");
		exit(-1);
	}
	to_recV.up = FALSE;
	// si elapsed est tjs == 0 apres, c'est que heartc n'a pas encore pu contacter un heartd
	to_recV.elapsed = 0;
	memcpy(to_recV.port, argv[3], 5);
	memcpy(to_recV.addr, ADDR, 15);
	memcpy(shm, &to_recV, sizeof (to_recV));

	while (TRUE) {
		//signal(SIGALRM,sighup);
		if (flag == 1) {
			flag = 0;
			was_down = TRUE;
		}
		r = read_dio(fd, address);
		if (r != 0) {
			halog(LOG_ERR, "unable to read dio device");
			sleep(timeout);
			continue;
		}
		//printf("to recv: %d elapsed: %d\n",to_recV.elapsed,old_elapsed);
		if ((to_recV.elapsed == old_elapsed) && (old_elapsed != 0)) {
			if (was_down == FALSE)
				sighup();
			else
				to_recV.up = FALSE;
			memcpy(shm, &to_recV, sizeof (to_recV));
		} else {
			old_elapsed = to_recV.elapsed;
			to_recV.elapsed = Elapsed();
			to_recV.up = TRUE;
			if (was_down == TRUE) {
				halog(LOG_WARNING, "peer on %s  up !", ADDR);
				was_down = FALSE;
			}
			for (j = 0; j < MAX_SERVICES; j++) {
				if (strlen(to_recV.service_name[j]) == 0) {
					j = MAX_SERVICES;
				} else {
					write_status(to_recV.service_name[j],
						     to_recV.service_state[j],
						     to_recV.nodename);
				}
				memcpy(shm, &to_recV, sizeof (to_recV));
			}
		}
		sleep(timeout);
	}
}				/* end main() */

void
sighup()
{
	to_recV.up = FALSE;
	to_recV.elapsed = Elapsed();
	memcpy(shm, &to_recV, sizeof (to_recV));
	halog(LOG_WARNING, "peer on %s  down !", ADDR);
	flag = 1;
}

gint
read_dio(gint fd, gint where)
{
	void *buff;
	int r;
	r = posix_memalign(&buff, BLKSIZE, BLKSIZE);
	if (r != 0) {
		halog(LOG_WARNING, "heartc_dio() posix_memalign error");
		return 1;
	}
	memset(buff, 0, BLKSIZE);
	lseek(fd, (BLKSIZE * where), SEEK_SET);
	r = read(fd, buff, BLKSIZE);
	if (r != BLKSIZE) {
		halog(LOG_WARNING, "heartc_dio() read error");
		return 1;
	}
	memcpy(&to_recV, buff, sizeof (struct sendstruct));
	free(buff);
	return 0;
}

void
sigterm()
{
	halog(LOG_WARNING, "SIGTERM received, exiting gracefuly ...");
	(void) shmctl(shmid, IPC_RMID, NULL);
	exit(0);
}
