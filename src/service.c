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
#include <pthread.h>

void exit_usage(gchar *);

struct thread_info {
	pthread_t thread_id;
	gchar * service;
	gchar * action;
};

struct thread_info *tinfo;

GHashTable *HT_SERV = NULL;
gchar *ACTION[MAX_ACTION] = {
	"STOP",
	"START",
	"FREEZE-STOP",
	"FREEZE-START",
	"UNFREEZE",
	"FORCE-STOP",
	"FORCE-START"
};

int
service_action(gchar * service, gchar * action)
{
	gint action_id, state, ostate, pstate, sstate;
	gchar *primary, *secondary;
	gpointer pointer;
	gboolean PRIMARY, SECONDARY;

	pointer = g_hash_table_lookup(HT_SERV, (gchar *) service);
	if (pointer == NULL) {
		halog(LOG_ERR, "service %s not found", service);
		exit(-1);
	}
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	pstate = get_status(GlobalList, primary, service);
	sstate = get_status(GlobalList, secondary, service);

	gchar *action_up;
	action_up = g_ascii_strup(action, -1);
	action_id = find_action(ACTION, action_up);
	g_free(action_up);

	if (action_id == -1) {
		halog(LOG_ERR, "action %s not found", action);
		exit(-1);
	}

	PRIMARY = is_primary(nodename, service);
	SECONDARY = is_secondary(nodename, service);
	if (PRIMARY) {
		state = pstate;
		ostate = sstate;
	} else if (SECONDARY) {
		state = sstate;
		ostate = pstate;
	} else {
		halog(LOG_ERR, "we are not primary nor secondary for service %s", service);
		return -1;
	}

	switch (action_id) {
	//STOP
	case 0:
		//si on est STARTED|UNKNOWN 
		return (change_status_stop(state, ostate, service, HT_SERV));
		break;

	//START
	case 1:
		//si on est STOP|UNKNOWN et que l'autre est STOP|FROZEN_STOP|UNKOWN
		return (change_status_start(state, ostate, service, HT_SERV));
		break;

	//FREEZE_STOP
	case 2:
		return (change_status_freeze_stop(state, ostate, service, HT_SERV));
		break;

	//FREEZE_START
	case 3:
		return (change_status_freeze_start(state, ostate, service, HT_SERV));
		break;

	//UNFREEZE
	case 4:
		return (change_status_unfreeze(state, service, HT_SERV));
		break;
	//FORCE_STOP
	case 5:
		return (change_status_force_stop(state, ostate, service, HT_SERV));
		break;
	//FORCE_START
	case 6:
		return (change_status_force_start(state, ostate, service, HT_SERV));
		break;
	}
	return -1;
}

static void *
service_action_thread(void * arg)
{
	struct thread_info * ti = arg;
	service_action(ti->service, ti->action);
	return NULL;
}

int
services_action(gchar * action)
{
	int i, rc;
	struct thread_info * ti;

	ti = calloc(g_list_length(GlobalList) / LIST_NB_ITEM, sizeof(struct thread_info));
	for (i = 0; i < (g_list_length(GlobalList) / LIST_NB_ITEM); i++) {
                halog(LOG_DEBUG, "[service] spawning thread [%d]", i);
                ti[i].action = action;
                ti[i].service = g_list_nth_data(GlobalList, i * LIST_NB_ITEM);
                rc = pthread_create(&ti[i].thread_id, NULL, &service_action_thread, &ti[i]);
                if (rc) {
                        halog(LOG_ERR, "return code from pthread_create() is %d", rc);
                        ti[i].thread_id = -1;
                }
        }
	for (i = 0; i < (g_list_length(GlobalList) / LIST_NB_ITEM); i++) {
                if (ti[i].thread_id < 0)
                        continue;
                halog(LOG_DEBUG, "[main] joining thread [%d]", i);
                pthread_join(ti[i].thread_id, NULL);
                ti[i].thread_id = -1;
        }
	free(ti);
	return -1;
}

int
main(argc, argv)
int argc;
char *argv[];
{
	gint ret;
	gchar *action, *service;
	gchar * shm;

	//FILE *EZ_SERVICES;
	gint list_size, svccount;

	if ((argc < 2) || (argc > 7)) {
		exit_usage(argv[0]);
	}

	snprintf(progname, MAX_PROGNAME_SIZE, "service");
	if (!init_var()) {
		return -1;
	}

	if (strncmp(argv[1], "-s", 2) == 0 || strncmp(argv[2], "-s", 2) == 0) {
		shm = get_shm_nmon_ro_segment();
		if (shm == NULL)
			return 1;
	}

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
	//SERVICE ADD
	if ((strcmp(argv[1], "-a") == 0) && (argc == 7)) {
		if (svccount >= MAX_SERVICES) {
			fprintf(stderr, "Error : Your already have created max services count (%d)\n", svccount);
			return -1;
		}
		if (strlen(argv[2]) >= MAX_SERVICES_SIZE) {
			fprintf(stderr, "Error : service name is too long. max %d\n", MAX_SERVICES_SIZE);
			return -1;
		}
		printf("Creating service %s :\n", argv[2]);
		if (service_add(argv[2], argv[3], argv[4], argv[5], argv[6], HT_SERV) == 0) {
			printf("Done.\n");
			return 0;
		}
		printf("Failed.\n");
		return -1;
	}

	//SERVICE RM
	if ((strcmp(argv[1], "-r") == 0) && (argc == 3)) {
		//change_status_deleted("unknown",state, ostate, service,HT_SERV);
		ret = service_rm(argv[2], HT_SERV);
		if (ret == 0) {
			printf("Done.\n");
			return 0;
		}
		printf("Failed.\n");
		return -1;
		
	}
	//ACTION
	if ((strcmp(argv[1], "-A") == 0) && (argc == 4)) {
		service = argv[2];
		action = argv[3];
		return (service_action(service, action));
	}

	// ACTION on all services
	if ((strcmp(argv[1], "-A") == 0) && (argc == 3)) {
		action = argv[2];
		return (services_action(action));
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
