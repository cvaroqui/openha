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
			type[i] = g_malloc0(8);
			interface[i] = g_malloc0(16);
			addr[i] = g_malloc0(16);
			port[i] = g_malloc0(5);
			timeout[i] = g_malloc0(5);
			if ((strcmp("net", lu[1]) == 0)
			    || (strcmp("unicast", lu[1]) == 0)) {
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
		if ((strcmp("net", Type) == 0)
		    || (strcmp("unicast", Type) == 0)) {
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
			type[i] = g_malloc0(8);
			interface[i] = g_malloc0(16);
			addr[i] = g_malloc0(16);
			port[i] = g_malloc0(5);
			timeout[i] = g_malloc0(5);
			if ((strcmp("net", lu[1]) == 0)
			    || (strcmp("unicast", lu[1]) == 0)) {
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
		if ((strcmp("net", Type) == 0)
		    || (strcmp("unicast", Type) == 0)) {
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
		if ((strcmp("net", Type) == 0)
		    || (strcmp("unicast", Type) == 0)) {
			fprintf(File, "%s\t%s\t%s\t%s\t%s\t%s\n", Node, Type,
				Interface, Addr, Port, Timeout);
		} else {
			fprintf(File, "%s\t%s\t%s\t%s\t%s\n", Node, Type,
				Interface, Addr, Timeout);
		}
	} else {
		if ((strcmp("net", Type) == 0)
		    || (strcmp("unicast", Type) == 0)) {
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
				if ((strcmp("net", type) == 0)
				    || (strcmp("unicast", type) == 0)) {
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
_hb_status(GList * list_heart, gint i)
{
	gchar * node;
	gchar * hb_role;
	gchar * hb_type;
	gchar * dev;
	gchar * addr_or_offset;
	gchar * port;
	gchar interface[MAX_PATH_SIZE];
	gchar key_path[MAX_PATH_SIZE];
	gchar *hb_shm;
	struct sendstruct to_ctrl;
	struct tm TestTime;
	long tmp_time;
	gchar *fmt = "%b %d %H:%M:%S", buff[256];

	node = (gchar *) g_list_nth_data(list_heart, (i * LIST_NB_ITEM));
	hb_type = (gchar *) g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 1);
	dev = (gchar *) g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 2);
	addr_or_offset = (gchar *) g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 3);
	port = (gchar *) g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 4);

	if (strcmp(nodename, node) == 0) {
		hb_role = "sender";
	} else {
		hb_role = "listener";
	}
	snprintf(interface, MAX_PATH_SIZE, "%s %s %s:%s",
		 hb_type, hb_role, dev, addr_or_offset);

	if ((strcmp(hb_type, "net") == 0)
	    || (strcmp(hb_type, "unicast") == 0)) {
		snprintf(key_path, MAX_PATH_SIZE, "%s/proc/%s-%s-%s.key",
			 getenv("EZ_LOG"), addr_or_offset, port, dev);
	} else {
		gchar **v;
		gchar *n;
		v = g_strsplit(dev, "/", 10);
		n = g_strjoinv(".", v);
		snprintf(key_path, MAX_PATH_SIZE, "%s/proc/%s.%s.key",
			 getenv("EZ_LOG"), n, addr_or_offset);
		g_free(n);
		g_strfreev(v);
	}

	hb_shm = get_shm_segment(key_path);
	if (hb_shm == NULL)
		return FALSE;

	memcpy(&to_ctrl, hb_shm, sizeof(to_ctrl));
	if (to_ctrl.up == TRUE) {
		tmp_time = (guint32) to_ctrl.elapsed;
		memcpy(&TestTime, localtime(&tmp_time), sizeof(TestTime));
		strftime(buff, sizeof(buff), fmt, &TestTime);
		printf("%s pid %d status UP, updated at %s\n",
			interface, (int) to_ctrl.pid, buff);
	} else {
		printf("%s status DOWN\n", interface);
	}
	return TRUE;
}

gboolean
hb_status()
{
	gchar *shm;
	FILE * fds;
	gint i;
	GList *list_heart;
	struct sendstruct tabinfo[MAX_HEARTBEAT];

	get_nodename();

	fds = fopen(getenv("EZ_MONITOR"), "r");
	if (fds == NULL) {
		halog(LOG_ERR, "unable to open %s", getenv("EZ_MONITOR"));
		return TRUE;
	}

	shm = get_shm_nmon_ro_segment();
	if (shm == NULL)
		return FALSE;

	memcpy(tabinfo, shm, sizeof(tabinfo));
	list_heart = get_liste_generic(fds, LIST_NB_ITEM);
	fclose(fds);

	for (i = 0; i < (g_list_length(list_heart) / 5); i++) {
		if (! _hb_status(list_heart, i))
			return FALSE;
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
	gchar *nvalue, *tvalue, *ivalue, *dvalue, *pvalue = NULL, *Tvalue;

	init_var();
	snprintf(progname, MAX_PROGNAME_SIZE, "hb");

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
		        fprintf(stderr, "node validation error.\n");
			usage();
			return 1;
		}
		if (!check_type(tvalue)) {
		        fprintf(stderr, "type validation error.\n");
			usage();
			return 2;
		}
		if (!check_interface(tvalue, ivalue, dvalue, pvalue)) {
		        fprintf(stderr, "interface validation error.\n");
			usage();
			return 3;
		}
		if (!check_timeout(Tvalue)) {
		        fprintf(stderr, "timeout validation error.\n");
			usage();
			return 4;
		}
#ifdef DEBUG
		printf
		    ("node: %s type: %s interface: %s addr: %s port: %s timeout: %s\n",
		     nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue);
#endif
		if (!hb_add(nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue)) {
			fprintf(stderr, "heartbeat add failed.\n");
			usage();
			return 1;
		} else {
			printf("heartbeat added.\n");
			return 0;
		}
	}
	if (rflag == 1) {
		if (!hb_remove(nvalue, tvalue, ivalue, dvalue, pvalue, Tvalue)) {
			fprintf(stderr, "heartbeat remove failed.\n");
			usage();
			return 1;
		} else {
			printf("heartbeat removed.\n");
			return 0;
		}
	}
	return 0;
}
