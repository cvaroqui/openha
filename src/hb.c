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
#include <config.h>
#endif

#include <stdlib.h>

#include <cluster.h>
#include <checks.h>
char *progname = NULL;

void
usage()
{
	fprintf(stderr, "Usage: hb [options] [target] ...\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"Options: -s                                            show heartbeat status\n");
	fprintf(stderr,
		"         -r [-n node -t type -i interface -d address -p port]     remove heartbeat link\n");
	fprintf(stderr,
		"         -a [-n node -t type -i interface -d address -p port -T timeout]  add heartbeat link\n");
	fprintf(stderr, "      where: \n");
	fprintf(stderr,
		"            node      = hostname of the host involved\n");
	fprintf(stderr,
		"            type      = net, dio (Linux only) or disk\n");
	fprintf(stderr,
		"            interface = network device or device fullpath\n");
	fprintf(stderr,
		"            address   = multicast address or block offset on device\n");
	fprintf(stderr,
		"            port      = udp port or nothing for a device\n");
	fprintf(stderr,
		"            timeout   = timeout before declaring the node down\n");
	exit(-1);
}

gboolean
hb_exists(gchar * Node, gchar * Type, gchar * Interface, gchar * Addr,
	  gchar * Port, gchar * Timeout)
{
	FILE *File;
	gint i = 0, j = 0;
	gchar *lu[6] = { 0, 0, 0, 0, 0, 0 };
	gchar *node[16], *type[16], *interface[16], *addr[16], *port[16],
	    *timeout[16];
	int n;

	if ((File = fopen(EZ_MONITOR, "r")) != NULL) {
		printf("hb_exists: unable to open $EZ_MONITOR.");
		return FALSE;
	}
	if (lockf(fileno(File), F_TLOCK, 0) == 0) {
		while (fscanf
		       (File, "%s %s %s %s %s", lu[0], lu[1], lu[2], lu[3],
			lu[4]) == 5) {
			node[i] = g_malloc0(MAX_NODENAME_SIZE);
			type[i] = g_malloc0(5);
			interface[i] = g_malloc0(16);
			addr[i] = g_malloc0(16);
			port[i] = g_malloc0(5);
			timeout[i] = g_malloc0(5);
			if (strcmp("net", lu[1]) == 0) {
				n = fscanf(File, "%s", lu[5]);
				if (n != 1) {
					fprintf(stderr,
						"Error: $EZ_MONITOR corruption.\n");
					return FALSE;
				}
				strcpy(node[i], lu[0]);
				strcpy(type[i], lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i], lu[3]);
				strcpy(port[i], lu[4]);
				strcpy(timeout[i], lu[5]);
			}
			if ((strcmp("disk", lu[1]) == 0)
			    || (strcmp("dio", lu[1]) == 0)) {
				strcpy(node[i], lu[0]);
				strcpy(type[i], lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i], lu[3]);
				strcpy(timeout[i], lu[4]);
			}
			i++;
		}
		fclose(File);
	}
	for (i = 0; i < j; i++) {
		if (strcmp("net", Type) == 0) {
			if ((strcmp(Node, node[i]) == 0) &&
			    (strcmp(Interface, interface[i]) == 0) &&
			    (strcmp(Addr, addr[i]) == 0) &&
			    (strcmp(Port, port[i]) == 0)) {
				fprintf(stderr,
					"Error: HeartBeat link already defined.\n");
				return FALSE;
			}
		} else {
			if ((strcmp(Node, node[i]) == 0) &&
			    (strcmp(Interface, interface[i]) == 0) &&
			    (strcmp(Addr, addr[i]) == 0)) {
				fprintf(stderr,
					"Error: HeartBeat link already defined.\n");
				return FALSE;
			}
		}
	}
	return TRUE;
}

gboolean
hb_add(gchar * Node, gchar * Type, gchar * Interface,
       gchar * Addr, gchar * Port, gchar * Timeout)
{
	FILE *File;
	gint i = 0, j = 0;
	gchar *lu[6];
	gchar *node[16], *type[16], *interface[16], *addr[16], *port[16],
	    *timeout[16];
	int n;

	if ((File = fopen(EZ_MONITOR, "a+")) == NULL) {
		printf("hb_add: unable to open $EZ_MONITOR.");
		return FALSE;
	}
	for (j = 0; j < 6; j++)
		lu[j] = g_malloc0(64);
	j = 0;
	if (lockf(fileno(File), F_TLOCK, 0) == 0) {
		while (fscanf
		       (File, "%s %s %s %s %s", lu[0], lu[1], lu[2], lu[3],
			lu[4]) == 5) {
			node[i] = g_malloc0(MAX_NODENAME_SIZE);
			type[i] = g_malloc0(5);
			interface[i] = g_malloc0(16);
			addr[i] = g_malloc0(16);
			port[i] = g_malloc0(5);
			timeout[i] = g_malloc0(5);
			if (strcmp("net", lu[1]) == 0) {
				n = fscanf(File, "%s", lu[5]);
				if (n != 1) {
					printf
					    ("hb_add: $EZ_MONITOR corruption.");
					return FALSE;
				}
				strcpy(node[i], lu[0]);
				strcpy(type[i], lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i], lu[3]);
				strcpy(port[i], lu[4]);
				strcpy(timeout[i], lu[5]);
			}
			if ((strcmp("disk", lu[1]) == 0)
			    || (strcmp("dio", lu[1]) == 0)) {
				strcpy(node[i], lu[0]);
				strcpy(type[i], lu[1]);
				strcpy(interface[i], lu[2]);
				strcpy(addr[i], lu[3]);
				//strcpy(port[i], lu[4]); 
				strcpy(timeout[i], "-");
			}
			i++;
		}
	}
	/******************/
	j = i;
	for (i = 0; i < j; i++) {
		if (strcmp("net", Type) == 0) {
			if ((strcmp(Node, node[i]) == 0) &&
			    (strcmp(Interface, interface[i]) == 0) &&
			    (strcmp(Addr, addr[i]) == 0) &&
			    (strcmp(Port, port[i]) == 0)) {
				fprintf(stderr,
					"Error: HeartBeat link already defined.\n");
				return FALSE;
			}
		} else {
			if ((strcmp(Node, node[i]) == 0) &&
			    (strcmp(Interface, interface[i]) == 0) &&
			    (strcmp(Addr, addr[i]) == 0)) {
				fprintf(stderr,
					"Error: HeartBeat link already defined.\n");
				return FALSE;
			}
		}
	}
	if (Timeout != NULL) {
		if (strcmp("net", Type) == 0) {
			fprintf(File, "%s\t%s\t%s\t%s\t%s\t%s\n", Node, Type,
				Interface, Addr, Port, Timeout);
		} else {
			fprintf(File, "%s\t%s\t%s\t%s\t%s\n", Node, Type,
				Interface, Addr, Timeout);
		}
	} else {
		if (strcmp("net", Type) == 0) {
			fprintf(File, "%s\t%s\t%s\t%s\t%s\n", Node, Type,
				Interface, Addr, Port);
		} else {
			fprintf(File, "%s\t%s\t%s\t%s\n", Node, Type, Interface,
				Addr);
		}
	}
	fclose(File);
	return TRUE;
}

gboolean
hb_remove(gchar * Node, gchar * Type, gchar * Interface, gchar * Addr,
	  gchar * Port, gchar * Timeout)
{
	FILE *File, *TMP;
	gchar node[16], type[16], interface[16], addr[16], port[16],
	    timeout[16];
	gchar *tmp;
	gboolean found = FALSE;
	int n;

	tmp = g_malloc0(300);
	tmp = g_strconcat(getenv("EZ"), "/.monitor.tmp", NULL);
	if ((TMP = fopen(tmp, "w")) == NULL) {
		fprintf(stderr, "unable to open $EZ/.monitor.tmp\n");
		return FALSE;
	}

	if ((File = fopen(EZ_MONITOR, "r")) == NULL) {
		printf("hb_rm: unable to open $EZ_MONITOR for reading");
		return FALSE;
	}
	if (!hb_exists(Node, Type, Interface, Addr, Port, Timeout)) {
		fprintf(stderr, "Error: unable to remove HeartBeat link.\n");
		return FALSE;
	}

	if ((File = freopen(EZ_MONITOR, "r+", File)) != NULL) {
		if (lockf(fileno(File), F_TLOCK, 0) == 0) {
			//printf("array: %s %s %s %s %s %s\n",array[0],array[1],array[2],array[3],array[4],array[5]);
			while (fscanf
			       (File, "%s %s %s %s %s", node, type, interface,
				addr, port) == 5) {
				if (strcmp("net", type) == 0) {
					n = fscanf(File, "%s", timeout);
					if (n != 1) {
						printf
						    ("hb_add: $EZ_MONITOR corruption.");
						return FALSE;
					}
					if ((strcmp(Node, node) == 0) &&
					    (strcmp(Type, type) == 0) &&
					    (strcmp(Interface, interface) == 0)
					    && (strcmp(Addr, addr) == 0)
					    && (strcmp(Port, port) == 0)) {
						found = TRUE;
						// don't rewrite the line
					} else {
						fprintf(TMP,
							"%s\t%s\t%s\t%s\t%s\t%s\n",
							node, type, interface,
							addr, port, timeout);
					}
				}
				if ((strcmp("disk", type) == 0)
				    || (strcmp("dio", type) == 0)) {
					if ((strcmp(Node, node) == 0)
					    && (strcmp(Type, type) == 0)
					    && (strcmp(Interface, interface) ==
						0)
					    && (strcmp(Addr, addr) == 0)) {
						found = TRUE;
						// don't rewrite the line
					} else {
						fprintf(TMP,
							"%s\t%s\t%s\t%s\t%s\n",
							node, type, interface,
							addr, port);
					}
				}
			}
		} else {
			printf("Unable to lock file !\n");
			fclose(File);
			return FALSE;
		}
	} else {
		fprintf(stderr,
			"Remove node: unable to open $EZ_MONITOR for reading\n");
	}
	fclose(File);
	fclose(TMP);
	if (!found) {
		fprintf(stderr, "Heartbeat not found.\n");
		return FALSE;
	}
	if (rename(tmp, EZ_MONITOR) != 0) {
		fprintf(stderr, "Rename failed.");
		return FALSE;
	}
	return TRUE;
}

gboolean
hb_status()
{
	gchar *FILE_KEY = NULL, *node, *date, *interface, *HB_KEY = NULL;
	gchar *status, *shm, *hb_shm;
	gint i, shmid, hb_shmid, fd;
	GList *list_heart;
	FILE *File;
	key_t key, hb_key;
	struct sendstruct tabinfo[MAX_HEARTBEAT];
	struct sendstruct to_ctrl;
	struct tm TestTime;
	long tmp_time;

	gchar *fmt = "%b %d %H:%M:%S", buff[256];

	if ((File = fopen(getenv("EZ_MONITOR"), "r")) == NULL) {
		//printf("Warning: Nothing to monitor: unable to open $EZ_MONITOR\n");
		return TRUE;
	}
	FILE_KEY = g_strconcat(getenv("EZ_LOG"), "/proc/nmond.key", NULL);
	//FILE_KEY = g_strconcat("/usr/local/cluster/log/proc/nmond.key",NULL);
	key = ftok(FILE_KEY, 0);
	g_free(FILE_KEY);

	if ((shmid =
	     shmget(key, sizeof (struct sendstruct) * MAX_HEARTBEAT,
		    0444)) == -1) {
		perror("shmget failed for nmond segment");
		//exit(-1);
		return FALSE;
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat failed for nmond segment");
		//exit(-1);
		return FALSE;
	}
	memcpy(tabinfo, shm, sizeof (tabinfo));
	list_heart = get_liste(File, LIST_NB_ITEM);
	for (i = 0; i < (g_list_length(list_heart) / 5); i++) {
		node = g_malloc0(MAX_NODENAME_SIZE);
		status = g_malloc0(8);
		date = g_malloc0(32);
		node = strncpy(node, g_list_nth_data(list_heart, (i * 5)), 80);
		if (strcmp
		    (g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 1),
		     "net") == 0) {
			interface =
			    g_strconcat(g_list_nth_data
					(list_heart, (i * 5) + 2), ":",
					g_list_nth_data(list_heart,
							(i * 5) + 3), ":",
					g_list_nth_data(list_heart,
							(i * 5) + 4), NULL);
			HB_KEY =
			    g_strconcat(getenv("EZ_LOG"), "/proc/",
					g_list_nth_data(list_heart,
							(i * 5) + 3), "-",
					g_list_nth_data(list_heart,
							(i * 5) + 4), "-",
					g_list_nth_data(list_heart,
							(i * 5) + 2), ".key",
					NULL);
		} else {
			gchar **NEW_KEY;
			gchar *m, *n;
			HB_KEY = g_malloc0(256);

			interface =
			    g_strconcat(g_list_nth_data
					(list_heart, (i * 5) + 2), ":",
					g_list_nth_data(list_heart,
							(i * 5) + 3), NULL);
			NEW_KEY =
			    g_strsplit(g_list_nth_data
				       (list_heart, (i * LIST_NB_ITEM) + 2),
				       "/", 10);
			n = g_strjoinv(".", NEW_KEY), m =
			    g_strconcat(getenv("EZ_LOG"), "/proc/", n, ".",
					g_list_nth_data(list_heart,
							(i * LIST_NB_ITEM) + 3),
					".key", NULL);
			strcpy(HB_KEY, m);
			g_free(m);
			g_free(n);
			g_strfreev(NEW_KEY);
		}

		if ((fd = open(HB_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
			fprintf(stderr, "Error: unable to open SHMFILE %s.\n",
				HB_KEY);
			g_free(interface);
			g_free(node);
			g_free(status);
			g_free(date);
			g_free(HB_KEY);
			return FALSE;
		}
		hb_key = ftok(HB_KEY, 0);
		if ((hb_shmid = shmget(hb_key, SHMSZ, IPC_CREAT | 0444)) < 0) {
			g_free(interface);
			g_free(node);
			g_free(status);
			g_free(date);
			g_free(HB_KEY);
			fprintf(stderr, "shmget failed\n");
			return FALSE;
		}
		if ((hb_shm = shmat(hb_shmid, NULL, 0)) == (char *) -1) {
			g_free(interface);
			g_free(node);
			g_free(status);
			g_free(date);
			g_free(HB_KEY);
			fprintf(stderr, "shmat failed\n");
			return FALSE;
		}
		memcpy(&to_ctrl, hb_shm, sizeof (to_ctrl));
		if (to_ctrl.up == TRUE) {
			strcpy(status, "UP");
			tmp_time = (guint32) to_ctrl.elapsed;
			memcpy(&TestTime, localtime(&tmp_time),
			       sizeof (TestTime));
			strftime(buff, sizeof (buff), fmt, &TestTime);
			printf
			    ("interface %s pid %d status %s, updated at %s \n",
			     interface, (int) to_ctrl.pid, status, buff);
		} else {
			strcpy(status, "DOWN");
			printf("interface %s status %s \n", interface, status);
		}
		g_free(interface);
		g_free(node);
		g_free(status);
		g_free(date);
		g_free(HB_KEY);
	}
	return TRUE;
}

gint
main(argc, argv)
gint argc;
gchar *argv[];
{
	guint c, sflag, rflag, aflag, nflag, tflag, iflag, dflag, pflag, Tflag =
	    0;
	gchar *nvalue, *tvalue, *ivalue, *dvalue, *pvalue =
	    NULL, *Tvalue;

	Setenv("PROGNAME", "hb", 1);
	init_var();
	sflag = rflag = aflag = nflag = tflag = iflag = dflag = pflag = Tflag =
	    0;
	nvalue = tvalue = ivalue = dvalue = pvalue = Tvalue = NULL;
	opterr = 0;

	while ((c = getopt(argc, argv, "sran:t:i:d:p:T:")) != -1)
		switch (c) {
		case 's':
			sflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'n':
			nflag = 1;
			nvalue = optarg;
			break;
		case 't':
			tflag = 1;
			tvalue = optarg;
			break;
		case 'i':
			iflag = 1;
			ivalue = optarg;
			break;
		case 'd':
			dflag = 1;
			dvalue = optarg;
			break;
		case 'p':
			pflag = 1;
			pvalue = optarg;
			break;
		case 'T':
			Tflag = 1;
			Tvalue = optarg;
			break;
		case '?':
			usage();
		default:
			usage();
			return 1;
		}

	//if ((sflag+rflag+aflag != 1) || (rflag==1 && argc!=12) || (aflag ==1 && argc != 14)){
	if ((sflag + rflag + aflag != 1)) {
		fprintf(stderr, "Incompatible options.\n");
		usage();
	}
	if (sflag == 1) {
		hb_status();
		return 0;
	}
	if (aflag == 1) {
		if (!check_node(nvalue)) {
			usage();
			return 1;
		}
		if (!check_type(tvalue)) {
			usage();
			return 2;
		}
		if (!check_interface(tvalue, ivalue, dvalue, pvalue)) {
			usage();
			return 3;
		}
		if (!check_timeout(Tvalue)) {
			usage();
			return 4;
		}
#ifdef DEBUG
		printf
		    ("node: %s type: %s interface: %s addr: %s port: %s timeout: %s\n",
		     nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue);
#endif
		if (!hb_add(nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue)) {
			fprintf(stderr, "Add heartbeat failed.\n");
			usage();
			return 1;
		} else {
			printf("Add heartbeat done.\n");
			return 0;
		}
	}
	if (rflag == 1) {
		if (!hb_remove(nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue)) {
			fprintf(stderr, "Remove heartbeat failed.\n");
			usage();
			return 1;
		} else {
			printf("Remove heartbeat done.\n");
			return 0;
		}
	}
	return 0;
}
