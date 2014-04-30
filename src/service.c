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
#include <stdio.h>
#include <glib.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <cluster.h>
#include <config.h>

void exit_usage(gchar *);

gchar *ACTION[MAX_ACTION] = { "STOP",
	"START",
	"FREEZE-STOP",
	"FREEZE-START",
	"UNFREEZE",
	"FORCE-STOP",
	"FORCE-START"
};

int
main(argc, argv)
int argc;
char *argv[];
{

	gint state, ostate, pstate, sstate, ret;
	gchar *service, *primary, *secondary, *action;
	gpointer pointer;
	gchar * shm;

	//FILE *EZ_SERVICES;
	gint i, list_size, svccount;
	gboolean PRIMARY, SECONDARY;
	GHashTable *HT_SERV = NULL;

	if ((argc < 2) || (argc > 7)) {
		exit_usage(argv[0]);
	}

	snprintf(progname, MAX_PROGNAME_SIZE, "service");
	init_var();

	shm = get_shm_nmon_ro_segment();
	if (shm == NULL)
		return 1;

	get_nodename();
	get_services_list();
	list_size = g_list_length(GlobalList);
	svccount = list_size / LIST_NB_ITEM;

	if (GlobalList == NULL) {
		//fprintf(stderr,"%s: no service defined, no action to take.\n",argv[0]);
		HT_SERV = NULL;
	} else {
		HT_SERV = get_hash(GlobalList);
	}

	// STATUS
	if ((strcmp(argv[1], "-s") == 0) && (argc == 2)) {
		service_status(GlobalList, HT_SERV);
		return 0;
	}

	// STATUS ONE LINE
	if ((strcmp(argv[1], "-s") == 0) && (strcmp(argv[2], "-c") == 0) && (argc == 3)) {
		service_status_cols(GlobalList, HT_SERV);
		return 0;
	}

	//INFO
	if ((strcmp(argv[1], "-i") == 0) && (argc == 3)) {
		service = argv[2];
		service_info(GlobalList, HT_SERV, nodename, service);
		return 0;
	}
	//SERVICE ADD RM
	if ((strcmp(argv[1], "-a") == 0) && (argc == 7)) {
		if (svccount < MAX_SERVICES) {
			printf("Creating service %s :\n", argv[2]);
			if (service_add
			    (argv[2], argv[3], argv[4], argv[5], argv[6],
			     HT_SERV) == 0)
				printf("Done.\n");
			else
				printf("Failed.\n");
			return 0;
		} else {
			printf
			    ("Error : Your already have created max services count (%d)\n",
			     svccount);
			exit(-1);
		}
	}

	if ((strcmp(argv[1], "-r") == 0) && (argc == 3)) {
		//change_status_deleted("unknown",state, ostate, service,HT_SERV);
		ret = service_rm(argv[2], HT_SERV);
		if (ret == 0) {
			printf("ok\n");
			return 0;
		} else {
			printf("Failed to remove service\n");
			return -1;
		}
		
	}
	//ACTION
	if ((strcmp(argv[1], "-A") == 0) && (argc == 4)) {
		service = argv[2];
		action = argv[3];
		pointer = g_hash_table_lookup(HT_SERV, (gchar *) service);
		primary = ((struct srvstruct *) (pointer))->primary;
		secondary = ((struct srvstruct *) (pointer))->secondary;
		pstate = get_status(GlobalList, primary, service);
		sstate = get_status(GlobalList, secondary, service);

		if (pointer == NULL) {
			printf("Error: service %s not found !\n", service);
			exit(-1);
		}
		gchar *action_up;
		action_up = g_ascii_strup(action, -1);

		if ((i = find_action(ACTION, action_up)) != -1) {
			PRIMARY = is_primary(nodename, service);
			SECONDARY = is_secondary(nodename, service);
			if (PRIMARY) {
				state = pstate;
				ostate = sstate;
			} else if (SECONDARY) {
				state = sstate;
				ostate = pstate;
			} else {
				printf("We are not Primary or secondary for service %s !\n", service);
				g_free(action_up);
				return -1;
			}
			g_free(action_up);

			switch (i) {
				//STOP
			case 0:
				//si on est STARTED|UNKNOWN 
				return (change_status_stop
					(state, ostate, service, HT_SERV));
				break;

				//START
			case 1:
				//si on est STOP|UNKNOWN et que l'autre est STOP|FROZEN_STOP|UNKOWN
				return (change_status_start
					(state, ostate, service, HT_SERV));
				break;

				//FREEZE_STOP
			case 2:
				return (change_status_freeze_stop
					(state, ostate, service, HT_SERV));
				break;

				//FREEZE_START
			case 3:
				return (change_status_freeze_start
					(state, ostate, service, HT_SERV));
				break;

				//UNFREEZE
			case 4:
				return (change_status_unfreeze
					(state, service, HT_SERV));
				break;
				//FORCE_STOP
			case 5:
				return (change_status_force_stop
					(state, ostate, service, HT_SERV));
				break;
				//FORCE_START
			case 6:
				return (change_status_force_start
					(state, ostate, service, HT_SERV));
				break;
			}
		} else {
			printf("Error: action %s not found !\n", action_up);
			g_free(action_up);
			exit(-1);
		}
	}
	exit_usage(argv[0]);
	return -1;
}

void
exit_usage(gchar * arg)
{
	fprintf(stderr,
		"Usage: %s [-s] [-i Service] [-A Service Action] [-r Service] [-a service script primary secondary check_up_script]\n",
		arg);
	fprintf(stderr,
		"Action in start|stop|freeze-stop|freeze-start|unfreeze|force-stop|force-start\n");
	fprintf(stderr, "  try to change state of service on this node\n");
	fprintf(stderr, "    start: start service\n");
	fprintf(stderr,
		"    stop: stop service...if other nodes are in stopped state, they will try a takeover\n");
	fprintf(stderr,
		"                      ...if other nodes are in frozen-stopped state, no takeover occur\n");
	fprintf(stderr,
		"    freeze-stop: set service to frozen-stop state(no takeover can occur...)\n");
	fprintf(stderr, "    freeze-start: not implemented...\n");
	fprintf(stderr,
		"    unfreeze: change from [start|stop]-freeze to started|stopped...\n");
	fprintf(stderr,
		"    force-start|force-stop: force state change to [start|stop]\n");
	exit(-1);
}
