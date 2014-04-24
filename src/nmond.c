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
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/utsname.h>
#include <cluster.h>
#define 	UP		1
#define 	DOWN	0

void copy_tab_send(struct sendstruct *, struct sendstruct *, guint);
void copy_tab_node(struct nodestruct *, struct nodestruct *, guint);
void *MakeDecision(void *);
void clean_tab(void);
void init(void);
gint get_seg(gint, struct shmtab_struct *);
gint fill_seg(gint, key_t, gchar *);
gboolean rm_func(gpointer, gpointer, gpointer);
void copy_func(gpointer, gpointer, gpointer);
void check_node_func(gpointer, gpointer, gpointer);
void sighup();
void sigquit();
void sigterm();
gint find_node(gchar *);

gchar *EZ_MONITOR, *SHM_KEY, *FILE_KEY;
FILE *File;
gchar STATE[2][5];
gint list_size = 0;
struct shmid_ds *buf;
static GList *list_heart = NULL;
struct sendstruct tabinfo[MAX_HEARTBEAT];
struct sendstruct tabinfo_b[MAX_HEARTBEAT];
struct nodestruct tabnode[MAX_HEARTBEAT];
struct nodestruct tabnode_b[MAX_HEARTBEAT];
gchar *name, *shm;
GHashTable *HT_SERV = NULL, *HT_NODES = NULL, *HT_NODES_OLD = NULL;
gchar SERVICE[MAX_SERVICES][MAX_SERVICES_SIZE];
gint shmid;
gchar *progname = NULL;

struct thread_info {
	pthread_t thread_id;
	gint i;
};

static void *
nmon_service_loop(void * arg)
{
	struct thread_info * ti = arg;
	gint i = ti->i;
	gpointer pointer;
	gchar service[MAX_SERVICES_SIZE];
	gchar *primary, *secondary;
	gint pstate, sstate;

	strncpy(service, SERVICE[i], 16);
	pointer = g_hash_table_lookup(HT_SERV, service);
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	pstate = get_status(GlobalList, primary, service);
	sstate = get_status(GlobalList, secondary, service);
	halog(LOG_DEBUG, "[main] Processing service [%s] - Pri[%s@%s] - Sec[%s@%s]",
		 service, VAL[pstate], primary, VAL[sstate], secondary);
	if (is_primary(nodename, service)) {
		halog(LOG_DEBUG, "[main] is_primary is true for service [%s] on node [%s]",
			 service, nodename);
		if ((sstate == STATE_STOPPED
		     || sstate == STATE_FROZEN_STOP
		     || sstate == STATE_UNKNOWN)
		    && pstate != STATE_STOPPING
		    && pstate != STATE_STARTED
		    && pstate != STATE_STARTING
		    && pstate != STATE_START_FAILED
		    && pstate != STATE_STOP_FAILED
		    && pstate != STATE_FROZEN_STOP
		    && sstate != STATE_START_READY
		    ) {
			halog(LOG_NOTICE, "Changing state of service %s", service);
			change_status_start(pstate, sstate, service, HT_SERV);
		}
	}
	else if (is_secondary(nodename, service)) {
		halog(LOG_DEBUG, "[main] is_secondary is true for service [%s] on node [%s]",
			 service, nodename);
		if ((pstate == STATE_STOPPED
		     || pstate == STATE_FROZEN_STOP
		     || pstate == STATE_UNKNOWN)
		    && pstate != STATE_STOPPING
		    && sstate != STATE_STARTED
		    && sstate != STATE_STARTING
		    && sstate != STATE_START_FAILED
		    && sstate != STATE_STOP_FAILED
		    && sstate != STATE_FROZEN_STOP
		    && sstate != STATE_START_READY
		    ) {
			halog(LOG_NOTICE, "Changing state of service %s", service);
			change_status_start(sstate, pstate, service, HT_SERV);
		}
	}
	//else
	//      printf("Nothing to do for service %s\n",service);
	return NULL;
}

gint
nmon_loop(gint nb_seg, struct shmtab_struct * tab_shm)
{
	gint i;
	struct srvstruct *ptr_service;
	gint rc;
	gpointer pointer;
	struct thread_info *tinfo;

	halog(LOG_DEBUG, "[main] looping");

	get_services_list();

	for (i = 0; i < (g_list_length(GlobalList) / LIST_NB_ITEM); i++) {
		ptr_service = g_malloc(sizeof (struct srvstruct));
		strcpy(SERVICE[i],
		       g_list_nth_data(GlobalList, i * LIST_NB_ITEM));
		strcpy(ptr_service->service_name,
		       g_list_nth_data(GlobalList, i * LIST_NB_ITEM));
		strcpy(ptr_service->script,
		       g_list_nth_data(GlobalList, (i * LIST_NB_ITEM) + 1));
		strcpy(ptr_service->primary,
		       g_list_nth_data(GlobalList, (i * LIST_NB_ITEM) + 2));
		strcpy(ptr_service->secondary,
		       g_list_nth_data(GlobalList, (i * LIST_NB_ITEM) + 3));
		strcpy(ptr_service->check_script,
		       g_list_nth_data(GlobalList, (i * LIST_NB_ITEM) + 4));
		g_hash_table_insert(HT_SERV, ptr_service->service_name,
				    ptr_service);
	}
	halog(LOG_DEBUG, "[main] HT_SERV hash table built (key=svcname, value=svcstruct)");

	/*
	 * Node Status management
	 * 
	 * On met tous les status a FALSE
	 * On sauvegarde les valeurs N-1 de tabinfo 
	 */
	if (HT_NODES != NULL) {
		memcpy(tabinfo_b, tabinfo, sizeof(tabinfo));
		for (i = 0; i < MAX_HEARTBEAT; i++)
			g_hash_table_insert(HT_NODES_OLD, tab_shm[i].nodename,
					    &tabinfo_b[i]);

		/* supprime tous les couples cle/valeur car rm_func retourne toujours true */
		g_hash_table_foreach_remove(HT_NODES, rm_func, HT_NODES);
	}
	halog(LOG_DEBUG, "[main] HT_NODES_OLD built. HT_NODES empty");
	for (i = 0; i < nb_seg; i++) {
		// On remplit tab_shm (nodename + shmid)
		if (fill_seg(i, tab_shm[i].shmid, tab_shm[i].nodename) != 0) {
			halog(LOG_ERR, "fill_seg failed");
			return -1;
		}
		pointer = g_hash_table_lookup(HT_NODES, tab_shm[i].nodename);

		// si le noeud est dela reference dans HT_NODES
		if ((pointer != NULL) && (((struct nodestruct *) pointer)->up == TRUE)) {
		} else {
			//printf("UP or DOWN : %d\n",tabinfo[i].up);
			g_hash_table_insert(HT_NODES, tab_shm[i].nodename,
					    &tabinfo[i]);
		}
	}
	halog(LOG_DEBUG, "[main] HT_NODES built");
	g_hash_table_foreach(HT_NODES, check_node_func, NULL);
	memcpy(shm, tabinfo, sizeof (tabinfo));

	// Services Status management
	//
	// Ouvre les segments SHM, pour chaque entree de EZ_MONITOR     
	// Remplit le tableau des node (tabnode): nodename, statut (UP ou DOWN), date de l'etat
	//printf("size HT: %d\n",g_hash_table_size(HT_SERV));
	//
	//
	//
	//EZ-HA: nmond[16020]: Cannot start mysql: service not in correct state (partner node is STOPPED, we are STOPPING.
	//Si on est on s'arrete, avec service -A  ..., pas la peine d'essayer de re-démarrer tout de suite ...

	tinfo = calloc(g_hash_table_size(HT_SERV), sizeof(struct thread_info));

	for (i = 0; i < g_hash_table_size(HT_SERV); i++) {
		halog(LOG_DEBUG, "[main] spawning thread [%d]", i);
		tinfo[i].i = i;
		rc = pthread_create(&tinfo[i].thread_id, NULL, &nmon_service_loop, &tinfo[i]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	for (i = 0; i < g_hash_table_size(HT_SERV); i++) {
		halog(LOG_DEBUG, "[main] joining thread [%d]", i);
		pthread_join(tinfo[i].thread_id, NULL);
	}
	halog(LOG_DEBUG, "[main] Removing each HT_SERV key/value");
	drop_hash(HT_SERV);
	halog(LOG_DEBUG, "[main] sleeping 2 seconds");
	sleep(2);
	free(tinfo);
	return 0;
}


int
main(argc, argv)
int argc;
char *argv[];
{
	key_t Key;
	gint pid;
	gint i;
	gint nb_seg, rc;
	struct shmtab_struct tab_shm[MAX_HEARTBEAT] = {};

	GlobalList = NULL;
	Setenv("PROGNAME", "nmond");
	progname = getenv("PROGNAME");

	switch (pid = fork()) {
	case 0:
		halog(LOG_INFO, "nmond starting ...");
		break;
	case (-1):
		fprintf(stderr, "Error in fork().\n");
		exit(-1);
	default:
		exit(0);
	}

	init_var();
	init();
	// chaque ligne de EZ_MONITOR
	nb_seg = 0;
	for (i = 0; i < list_size; i++) {
		// if it's NOT matching our nodename
		if (strcmp
		    (nodename,
		     g_list_nth_data(list_heart, (i * LIST_NB_ITEM))) != 0) {
			get_seg(i, &tab_shm[nb_seg]);
			nb_seg++;
		}
	}
	HT_NODES_OLD = g_hash_table_new(g_str_hash, g_str_equal);
	HT_NODES = g_hash_table_new(g_str_hash, g_str_equal);
	HT_SERV = g_hash_table_new(g_str_hash, g_str_equal);

	Key = ftok(FILE_KEY, 0);
	g_free(FILE_KEY);
	if ((shmid = shmget(Key, sizeof (tabinfo), IPC_CREAT | 0644)) == -1) {
		perror("shmget failed");
	}
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat failed");
	}

	signal(SIGTERM, sigterm);
	signal(SIGUSR1, signal_usr1_callback_handler);
	signal(SIGUSR2, signal_usr2_callback_handler);

	while (TRUE) {
		rc = nmon_loop(nb_seg, tab_shm);
		if (rc > 0)
			return rc;
	}
	return 0;
}

void
sighup()
{
}

void
check_node_func(gpointer key, gpointer value, gpointer HT)
{
	halog(LOG_DEBUG, "check_node_func", "Function start");
	gpointer new_pointer, old_pointer;
	gint i = 0;
	gchar *service;
	gchar *node_to_check;

	service = g_malloc(MAX_SERVICES_SIZE);
	node_to_check = g_malloc(MAX_NODENAME_SIZE);
	new_pointer = g_hash_table_lookup(HT_NODES, key);
	old_pointer = g_hash_table_lookup(HT_NODES_OLD, key);
	node_to_check =
	    strncpy(node_to_check, (gchar *) key, MAX_NODENAME_SIZE);
	halog(LOG_DEBUG, "[check_node_func] Will check node [%s]",
		 node_to_check);

	//Node is down  
	if ((((struct nodestruct *) new_pointer)->up) == FALSE) {
		halog(LOG_DEBUG, "[check_node_func] Node [%s] is DOWN. Need update for all services running on node.",
			 node_to_check);
		if (old_pointer != NULL) {
			for (i = 0; i < g_hash_table_size(HT_SERV); i++) {
				service = strncpy(service, (gchar *)
						  g_list_nth_data(GlobalList,
								  i *
								  LIST_NB_ITEM),
						  MAX_SERVICES_SIZE);
				if ((is_primary(node_to_check, service)
				     || is_secondary(node_to_check, service))) {
					if (get_status
					    (GlobalList, node_to_check,
					     service) != STATE_UNKNOWN) {
						write_status(service, STATE_UNKNOWN,
							     node_to_check);
						halog(LOG_WARNING, "setting service %s to UNKNOWN for node %s",
							service, node_to_check);
					}
				}
			}
		}
	} else {
		halog(LOG_DEBUG, "[check_node_func] Node [%s] is UP. Leaving check_node_func.",
			 node_to_check);
	}
	g_free(service);
	g_free(node_to_check);
}

void
copy_func(gpointer key, gpointer value, gpointer HT)
{
	g_hash_table_insert(HT, key, value);
}

gboolean
rm_func(gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}

void
init()
{
	gint fd;

	get_nodename();

	if (getenv("EZ_LOG") == NULL) {
		halog(LOG_ERR, "environment variable EZ_LOG not defined ...");
		perror("Error: environment variable EZ_LOG not defined ...\n");
		exit(-1);
	}
	FILE_KEY = g_strconcat(getenv("EZ_LOG"), "/proc/nmond.key", NULL);
	if ((fd = open(FILE_KEY, O_RDWR | O_CREAT, 00644)) == -1) {
		halog(LOG_ERR, "unable to open key file");
		perror("Error: unable to open key file");
		exit(-1);
	}
	if (lockf(fd, F_TLOCK, 0) != 0) {
		halog(LOG_ERR, "unable to lock key file");
		perror("Error: unable to lock key file");
		exit(-1);
	}
	if ((EZ_MONITOR = getenv("EZ_MONITOR")) == NULL) {
		halog(LOG_ERR, "Error: variable EZ_MONITOR not defined");
		perror("Error: variable EZ_MONITOR not defined");
		exit(-1);
	}
	if ((File = fopen(EZ_MONITOR, "r")) == NULL) {
		halog(LOG_ERR, "Nothing to monitor: unable to open $EZ_MONITOR");
		perror("Nothing to monitor: unable to open $EZ_MONITOR");
		exit(-1);
	}
//a rajouter: si EZ_MONITOR est vide, on sort 
	strcpy(STATE[0], "DOWN");
	strcpy(STATE[1], "UP  ");
	buf = malloc(sizeof (struct shmid_ds));
	SHM_KEY = g_malloc0(256);
	list_heart = get_liste_generic(File, LIST_NB_ITEM);
	list_size = g_list_length(list_heart) / LIST_NB_ITEM;
	fclose(File);
	//printf("nombre de heartbeat a surveiller:%d\n",list_size);
}

//for future use
void
show_node_diff(struct nodestruct *now, struct nodestruct *before, guint size)
{
	gint i = 0;
	for (i = 0; i < size; i++) {
	}
}

//for future use
void
show_send_diff(struct sendstruct *dest, struct sendstruct *from, guint size)
{
}

void
copy_tab_node(struct nodestruct *dest, struct nodestruct *from, guint size)
{
	memcpy(dest, from, sizeof (struct nodestruct) * size);
}

void
copy_tab_send(struct sendstruct *dest, struct sendstruct *from, guint size)
{
	memcpy(dest, from, sizeof (struct sendstruct) * size);
}

void
clean_tab()
{
	halog(LOG_DEBUG, "clean_tab", "Function start");
	gint i = 0;
	for (i = 0; i < MAX_HEARTBEAT; i++) {
		tabnode[i].nodename[0] = '\0';
		tabnode[i].up = FALSE;
		tabnode[i].tv.tv_sec = 0;
	}
}

gint
find_node(gchar * val)
{
	halog(LOG_DEBUG, "find_node", "Function start");
	gint index = 0;

	for (index = 0; index < MAX_HEARTBEAT; index++) {
		if ((strcmp(val, tabnode[index].nodename) == 0) &&
		    (strlen(tabnode[index].nodename) != 0)) {
			return (index);
		}
	}
	return (-1);
}

gint
get_node(GList * liste, gchar * port, gchar * addr)
{
	halog(LOG_DEBUG, "get_node", "Function start");
	gint i = 0;

	for (i = 0; i < g_list_length(liste); i++) {
		if ((strcmp
		     (addr,
		      g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 1)) == 0)
		    &&
		    (strcmp
		     (port,
		      g_list_nth_data(list_heart,
				      (i * LIST_NB_ITEM) + 2)) == 0))
			return i;
	}
	return -1;
}

gint
get_seg(gint i, struct shmtab_struct * S)
{
	halog(LOG_DEBUG, "get_seg", "Function start");
	gint shmid, itmp;
	key_t key;
	gchar nodename[MAX_NODENAME_SIZE], **NEW_KEY;
	gchar *m, *n;

	strcpy(nodename, g_list_nth_data(list_heart, (i * LIST_NB_ITEM)));
	strcpy(SHM_KEY, getenv("EZ_LOG"));
	strcat(SHM_KEY, "/proc/");

	if (strcmp(g_list_nth_data(list_heart, (i * LIST_NB_ITEM) + 1), "net")
	    == 0) {
		m = g_strconcat(SHM_KEY,
				g_list_nth_data(list_heart,
						(i * LIST_NB_ITEM) + 3), "-",
				g_list_nth_data(list_heart,
						(i * LIST_NB_ITEM) + 4), "-",
				g_list_nth_data(list_heart,
						(i * LIST_NB_ITEM) + 2), ".key",
				NULL);
		strcpy(SHM_KEY, m);
		g_free(m);
	}
	// raw or dio
	else {
		NEW_KEY =
		    g_strsplit(g_list_nth_data
			       (list_heart, (i * LIST_NB_ITEM) + 2), "/", 10);
		n = g_strjoinv(".", NEW_KEY), m =
		    g_strconcat(SHM_KEY, n, ".",
				g_list_nth_data(list_heart,
						(i * LIST_NB_ITEM) + 3), ".key",
				NULL);
		strcpy(SHM_KEY, m);
		g_free(m);
		g_free(n);
		g_strfreev(NEW_KEY);
	}

	if ((itmp = open(SHM_KEY, O_RDONLY)) == -1) {
		fprintf(stderr, "%s\n", SHM_KEY);
		perror("Error: unable to open SHMFILE");
		fprintf(stderr, "Heartc missing ?\n");
		exit(-2);
	}
	key = ftok(SHM_KEY, 0);
	close(itmp);
	if ((shmid = shmget(key, SHMSZ, 0444)) == -1) {
		fprintf(stderr,
			"Error: unable to get segment for address %s and port %s",
			(gchar *) g_list_nth_data(list_heart,
						  (i * LIST_NB_ITEM) + 2),
			(gchar *) g_list_nth_data(list_heart,
						  (i * LIST_NB_ITEM) + 3));
		perror(" ");
	}
	if (shmctl(shmid, IPC_STAT, buf) != 0) {
		perror("Error: unable to stat segment");
	}
	if (buf->shm_nattch < 1) {
		fprintf(stderr,
			"Error: heartc may not be running for address %s and port %s\n",
			(gchar *) g_list_nth_data(list_heart,
						  (i * LIST_NB_ITEM) + 2),
			(gchar *) g_list_nth_data(list_heart,
						  (i * LIST_NB_ITEM) + 3));
		return -1;
	} else {
		S->shmid = shmid;
		strcpy(S->nodename, nodename);
		return 0;
	}
}

gint
fill_seg(gint i, key_t key, gchar * nodename)
{
	halog(LOG_DEBUG, "fill_seg", "Function start");
	gpointer *R;
	struct sendstruct to_recv;

	R = shmat(key, NULL, 0);
	if (R != ((void *) -1)) {
		memcpy(&tabinfo[i], R, sizeof (to_recv));
		memcpy(&tabinfo[i].nodename, nodename, MAX_NODENAME_SIZE);
		shmdt((void *) R);
		return 0;
	} else {
		fprintf(stderr, "Unable to attach memory segment.\n");
		perror("");
		return -1;
	}
}

void
sigterm()
{
	halog(LOG_INFO, "SIGTERM received, exiting gracefuly ...");
	(void) shmctl(shmid, IPC_RMID, NULL);
	drop_list(GlobalList);
	drop_hash(GLOBAL_HT_SERV);
	exit(0);
}

