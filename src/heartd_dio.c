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

#include <sys/types.h>		/* include files for IP Sockets */
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

struct sendstruct to_send;
gint write_dio(gint fd, struct sendstruct, gchar *, gint);
void sighup();
void sigterm();
gint shmid;

int
main(argc, argv)
int argc;
char *argv[];
{

	gint i, fd, address;
	struct utsname tmp_name;
	gchar *shm, *FILE_KEY, *device;
	gint fd2;
	gchar **NEW_KEY;
	gchar *n;
	key_t key;

	device = g_malloc0(128);

	if (argc != 3) {
		fprintf(stderr, "Usage: heartd_dio [device] offset \n");
		exit(-1);
	}
	signal(SIGTERM, sigterm);
	signal(SIGUSR1, signal_usr1_callback_handler);
	signal(SIGUSR2, signal_usr2_callback_handler);

	daemonize("heartd_dio");
	snprintf(progname, MAX_PROGNAME_SIZE, "heartd_dio");


	address = atoi(argv[2]);
	NEW_KEY = g_strsplit(argv[1], "/", 10);
	n = g_strjoinv(".", NEW_KEY),
	    FILE_KEY =
	    g_strconcat(getenv("EZ_VAR"), "/proc/", n, ".", argv[2], ".key",
			NULL);
	g_strfreev(NEW_KEY);
	g_free(n);

	if ((fd = open(FILE_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		printf("key: %s\n", FILE_KEY);
		halog(LOG_ERR, "unable to open key file");
		exit(-1);
	}
	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock key file");
		exit(-1);
	}
	key = ftok(FILE_KEY, 0);
	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0644)) < 0) {
		halog(LOG_ERR, "shmget failed");
		exit(-1);
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		halog(LOG_ERR, "shmat failed");
		exit(-1);
	}

	setpriority(PRIO_PROCESS, 0, -15);
	to_send.hostid = gethostid();
	to_send.pid = getpid();
	uname(&tmp_name);
	strncpy(to_send.nodename, tmp_name.nodename, MAX_NODENAME_SIZE);
	//signal(SIGALRM,sighup);
	fd2 = open(argv[1], O_DIRECT | O_RDWR);
	if (fd2 < 0) {
		halog(LOG_ERR, "unable to open device");
		exit(-1);
	}
	while (1) {
		signal(SIGALRM, sighup);
		i = 0;
		to_send.elapsed = Elapsed();
		get_node_status(&to_send);
		i = write_dio(fd2, to_send, device, address);
		if (i != 0) {
			halog(LOG_ERR, "write_dio() failed");
		}
		memcpy(shm, &to_send, sizeof (to_send));
		alarm(1);
		pause();
	}
}

gint
write_dio(gint fd, struct sendstruct to_write, gchar * device, gint where)
{
	void *buff;
	int r;
	size_t s = sizeof(struct sendstruct) / BLKSIZE * BLKSIZE;

	if (s < sizeof(struct sendstruct))
		s += BLKSIZE;

	r = posix_memalign(&buff, BLKSIZE, sizeof(struct sendstruct));
	if (r != 0) {
		halog(LOG_ERR, "write_dio() posix_memalign failed");
		return 1;
	}
	memcpy(buff, &to_write, sizeof(struct sendstruct));
	lseek(fd, (where * BLKSIZE), SEEK_SET);
	r = write(fd, buff, s);
	if (r != s) {
		halog(LOG_ERR, "[write_dio] write failed: wrote %d bytes out of %d", r, s);
		free(buff);
		return 1;
	}
	halog(LOG_DEBUG, "[write_dio] successfully wrote sendstruct of %d bytes in a %d bytes chunk",
	      sizeof(struct sendstruct), s);
	free(buff);
	return 0;
}

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

