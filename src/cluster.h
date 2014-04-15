#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <glib.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <ctype.h>
#include <wait.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <config.h>

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#define MAX_SERVICES 128
#define MAX_SERVICES_SIZE 16
#define MAX_HEARTBEAT 16
#define MAX_NODENAME_SIZE 128
#define MAX_NODES 16
#define MAX_ITEM 256
#define MAX_STATE 10
#define MAX_ACTION 7
#define SCRIPT_SIZE 128

extern unsigned sleep(unsigned);
gchar *EZ;
gchar *EZ_BIN;
gchar *EZ_SERVICES;
gchar *EZ_MONITOR;
gchar *EZ_LOG;
gchar *EZ_NODES;
extern gchar *progname;

gchar *VAL[MAX_STATE] = {
	"STOPPED", "STOPPING", "STARTED",
	"STARTING", "START_FAILED", "STOP_FAILED",
	"FROZEN_STOP", "START_READY", "UNKNOWN",
	"FORCE-STOP"
};

const gint LIST_NB_ITEM = 5;  /* nb colonne dans fichier services */
gint DEBUGGING=0;
gchar IDENT[128];
gchar debugmsg[512];

GList *get_services_list(void);
gint get_status(GList *, gchar *, gchar *);
gint launch(gchar *, gchar **);

void print_func(gpointer, gpointer, gpointer);
gboolean rm_func_serv(gpointer, gpointer, gpointer);
void service_info(GList *, GHashTable *, gchar *, gchar *);
void service_status(GList *, GHashTable *);
gint change_status_stop(gint, gint, gchar *, GHashTable *);
gint change_status_start(gint, gint, gchar *, GHashTable *);
gint change_status_freeze_stop(gint, gint, gchar *, GHashTable *);
gint change_status_freeze_start(gint, gint, gchar *, GHashTable *);
gint change_status_unfreeze(gint, gchar *, GHashTable *);
gint change_status_force_stop(gint, gint, gchar *, GHashTable *);
gint find_action(gchar **, gchar *);
gint service_add(gchar *, gchar *, gchar *, gchar *, gchar *, GHashTable *);
gint create_file(gchar *, gchar *);
gint create_dir(gchar *);
gint service_rm(gchar *, GHashTable *);
gint service_mod(gchar *);
gint rm_file(gchar *);
gint set_mcast_if(gint, gchar *);
void halog(gint, gchar *, gchar *);
void debuglog(gchar *, gchar *, gchar *);
void signal_usr1_callback_handler();
void signal_usr2_callback_handler();


void
delete_data(gpointer data ,gpointer user_data) {
    g_free(data);
    data = NULL;
}

void debuglog(gchar *prg_src, gchar *prg_func, gchar *message) {
	if (DEBUGGING != 1) {
		return;
        }

	gchar *LOGFILE;
	FILE *fd;
	time_t clock = time(NULL);
	char buf[1024];

	LOGFILE=g_strconcat(getenv("EZ_LOG"),"/cluster.log",NULL);
	if ((fd = fopen(LOGFILE, "a+")) ==NULL){
		printf("Error opening logging file <%s>\n",LOGFILE);
		exit(-1);
	}
	strftime(buf, sizeof(buf), "%c", localtime(&clock));
	fprintf(fd,"[%s][%s][%s] %s\n",buf,prg_src,prg_func,message);

	fclose(fd);
}

void
debug_list(GList * liste)
{
	gint i = 0;

	for (i = 0; i <= (g_list_length(liste) / LIST_NB_ITEM)-1; i++) {
		snprintf(debugmsg,sizeof(debugmsg),"i=%d SVC[%s] SCRIPT[%s] PRI[%s] SEC[%s] CHK[%s]",
			i,
			(char*)g_list_nth_data(liste, i * LIST_NB_ITEM),
			(char*)g_list_nth_data(liste, (i * LIST_NB_ITEM) + 1),
			(char*)g_list_nth_data(liste, (i * LIST_NB_ITEM) + 2),
			(char*)g_list_nth_data(liste, (i * LIST_NB_ITEM) + 3),
			(char*)g_list_nth_data(liste, (i * LIST_NB_ITEM) + 4));
		debuglog(IDENT,"debug_list",debugmsg);
	}
}

void
halog(gint type, gchar * prg_src, gchar * message)
{
	gchar *msg;

	debuglog(prg_src, "halog", message);
	msg = g_strconcat("EZ-HA: ", prg_src, "\n", NULL);
	openlog(msg, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(type, "%s", message);
#ifdef VERBOSE
	printf("%s", message);
#endif
	g_free(msg);
}

void signal_usr1_callback_handler()
{
	DEBUGGING=1;
}

void signal_usr2_callback_handler()
{
	DEBUGGING=0;
}

glong
Elapsed(void)
{
	gint ts;
	struct timeval buf;
	if ((ts = gettimeofday(&buf, NULL)) == 0)
		return buf.tv_sec;
	else
		return -1;
/* printf("Time: %ld  %ld\n",buf.tv_sec,buf.tv_usec); */
}

struct service {
	gchar name[MAX_SERVICES_SIZE];
	guint state;
};

struct srvstruct {
	gchar service_name[MAX_SERVICES_SIZE];
	gchar script[SCRIPT_SIZE];
	gchar primary[MAX_NODENAME_SIZE];
	gchar secondary[MAX_NODENAME_SIZE];
	gchar check_script[SCRIPT_SIZE];
	gboolean status;
};

struct sendstruct {
	gchar nodename[MAX_NODENAME_SIZE];
	gboolean up;
	gboolean srv_lock;
	gchar port[5];
	gchar addr[15];
	gchar service_name[MAX_SERVICES][MAX_SERVICES_SIZE];
	gchar service_state[MAX_SERVICES];
	gboolean service_lock[MAX_SERVICES];
	guint32 hostid;
	guint32 elapsed;
	pid_t pid;
};

struct nodestruct {
	gchar nodename[MAX_NODENAME_SIZE];
	gboolean up;
	struct timeval tv;
};

struct shmtab_struct {
	gchar nodename[MAX_NODENAME_SIZE];
	key_t shmid;
};

struct flock *
file_lock(short type, short whence)
{
	static struct flock ret;
	ret.l_type = type;
	ret.l_start = 0;
	ret.l_whence = whence;
	ret.l_len = 0;
	ret.l_pid = getpid();
	return &ret;
}

void
get_my_name(gpointer name)
{
	struct utsname tmp_name;
	uname(&tmp_name);
	strncpy(name, tmp_name.nodename, MAX_NODENAME_SIZE);
}

gboolean
is_primary(gchar * node, gchar * service)
{
	GList *List_services = NULL;
	gint i = 0, j, list_size;
	gchar *name, *service_to_cmp, *node_to_cmp;

	name = g_malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	List_services = get_services_list();
	list_size = g_list_length(List_services);
	j = list_size / LIST_NB_ITEM;
	for (i = 0; i < j; i++) {
		node_to_cmp =
		    g_list_nth_data(List_services, (i * LIST_NB_ITEM) + 2);
		service_to_cmp =
		    g_list_nth_data(List_services, (i * LIST_NB_ITEM));
		if ((strncmp(service_to_cmp, service, MAX_SERVICES_SIZE) == 0)
		    && (strncmp(node_to_cmp, node, MAX_NODENAME_SIZE) == 0)) {
			g_free(name);
			g_list_foreach(List_services, delete_data, NULL);
			g_list_free(List_services);
			return TRUE;
		}
	}
	g_free(name);
	g_list_foreach(List_services, delete_data, NULL);
	g_list_free(List_services);
	return FALSE;
}

gboolean
is_secondary(gchar * node, gchar * service)
{
	GList *List_services = NULL;
	gint i = 0, j, list_size;
	gchar *name, *service_to_cmp, *node_to_cmp;

	name = g_malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	List_services = get_services_list();
	list_size = g_list_length(List_services);
	j = list_size / LIST_NB_ITEM;
	for (i = 0; i < j; i++) {
		node_to_cmp =
		    g_list_nth_data(List_services, (i * LIST_NB_ITEM) + 3);
		service_to_cmp =
		    g_list_nth_data(List_services, (i * LIST_NB_ITEM));
		if ((strcmp(service_to_cmp, service) == 0)
		    && (strcmp(node_to_cmp, node) == 0)) {
			g_free(name);
			g_list_foreach(List_services, delete_data, NULL);
			g_list_free(List_services);
			return TRUE;
		}
	}
	g_free(name);
	g_list_foreach(List_services, delete_data, NULL);
	g_list_free(List_services);
	return FALSE;
}

gchar *
get_our_secondary(gchar * node, gchar * service, GList * liste)
{
	gint i = 0, j, list_size;
	gchar *name, *service_to_cmp, *node_to_cmp;
	gint LIST_NB_ITEM = 5;

	name = g_malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	list_size = g_list_length(liste);
	j = list_size / LIST_NB_ITEM;
	for (i = 0; i < j; i++) {
		node_to_cmp = g_list_nth_data(liste, (i * LIST_NB_ITEM) + 2);
		service_to_cmp = g_list_nth_data(liste, (i * LIST_NB_ITEM));
		if ((strcmp(service_to_cmp, service) == 0) &&
		    (strcmp(node_to_cmp, node) == 0)) {
			return g_list_nth_data(liste, (i * LIST_NB_ITEM) + 3);
		}
	}
	return NULL;
}

gint
read_lock(gint fd)
{				/* a shared lock on an entire file */
	return (fcntl(fd, F_SETLKW, file_lock(F_RDLCK, SEEK_SET)));
}

gint
write_lock(gint fd)
{				/* an exclusive lock on an entire file */
	return (fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_SET)));
}

gint
append_lock(gint fd)
{				/* a lock on the _end_ of a file -- other
				   processes may access existing records */
	return (fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_END)));
}

void
write_status(gchar service[MAX_SERVICES_SIZE], gchar state, gchar * node)
{
	gchar *FILE_NAME;
	FILE *FILE_STATE;
	gchar to_copy[2];

	FILE_NAME =
	    g_strconcat(getenv("EZ"), "/services/", service, "/STATE.", node,
			NULL);

	if ((FILE_STATE = fopen((char *) FILE_NAME, "r")) != NULL) {
//                      if (read_lock(fileno(FILE_STATE)) != -1){
		if (TRUE) {
//                      Lock success
			if ((FILE_STATE =
			     freopen((char *) FILE_NAME, "r+",
				     FILE_STATE)) == NULL) {
				perror("Unable to reopen SERVICE STATE file");
				fclose(FILE_STATE);
				return;
			} else {
				to_copy[0] = state;
				to_copy[1] = '\n';
				fwrite(to_copy, 2, 1, FILE_STATE);
				fflush(FILE_STATE);
				g_free(FILE_NAME);
				fclose(FILE_STATE);
			}
		} else {
			printf("Lock Unsuccessful\n");
			perror("lockf:");
			fclose(FILE_STATE);
			return;
		}
	} else {
		//perror("No service(s) defined (unable to open SERVICE STATE file)");
		g_free(FILE_NAME);
		return;
	}
}

#define SHMSZ sizeof(struct sendstruct) * MAX_SERVICES


GList *
get_liste(FILE * File, guint elem)
{
	GList *L = NULL;
	gint i = 0, j = 0, k = 0, l = 0, LAST;
	gchar *s = NULL;
	gchar item[MAX_ITEM];
	gchar tmp_tab[MAX_ITEM][MAX_ITEM];

    for(i=0; i<MAX_ITEM; i++)
        for(j=0; j<MAX_ITEM; j++)
            tmp_tab[i][j] = '\0';	

    i = 0; j = 0;
	// Count the number of lines
	while (fgets(tmp_tab[i], MAX_ITEM-1, File) != NULL) {
		size_t ln = strlen(tmp_tab[i]) - 1;
        if (tmp_tab[i][ln] == '\n')
            tmp_tab[i][ln] = '\0';
		snprintf(debugmsg,sizeof(debugmsg),"loading service line tmp_tab[%d] => [%s]",i,tmp_tab[i]);
		debuglog(IDENT,"get_liste",debugmsg);
		i++;
	}
	LAST = i;
	// For each line
	for (i = 0; i < LAST; i++) {
		j = 0;
		// for each element
		for (l = 0; l < elem; l++) {
			while ((tmp_tab[i][j] != ' ') && (tmp_tab[i][j] != '\t')
			       && (tmp_tab[i][j] != '\n')
			       && (tmp_tab[i][j] != '\0')) {
				item[k] = tmp_tab[i][j];
				k++;
				j++;
			}
			item[k] = '\0';
			while ((tmp_tab[i][j] == ' ')
			       || (tmp_tab[i][j] == '\t')) {
				j++;
			}
                        snprintf(debugmsg,sizeof(debugmsg),"item [%s] - item len [%d]",item,strlen(item));
                        debuglog(IDENT,"get_liste",debugmsg);
			k = 0;
			s = g_strdup(item);
			L = g_list_append(L, s);
			item[0] = '\0';
		}
	}
	fseek(File, 0L, SEEK_SET);
	return L;
}

GHashTable *
get_hash(GList * liste)
{
	GHashTable *HT;
	gint i = 0;
	struct srvstruct *service;

	HT = g_hash_table_new(g_str_hash, g_str_equal);
	for (i = 0; i < (g_list_length(liste) / LIST_NB_ITEM); i++) {
		service = g_malloc(sizeof (struct srvstruct));
		strcpy(service->service_name,
		       g_list_nth_data(liste, i * LIST_NB_ITEM));
		strcpy(service->script,
		       g_list_nth_data(liste, (i * LIST_NB_ITEM) + 1));
		strcpy(service->primary,
		       g_list_nth_data(liste, (i * LIST_NB_ITEM) + 2));
		strcpy(service->secondary,
		       g_list_nth_data(liste, (i * LIST_NB_ITEM) + 3));
		strcpy(service->check_script,
		       g_list_nth_data(liste, (i * LIST_NB_ITEM) + 4));
		g_hash_table_insert(HT, service->service_name, service);
	}
	return HT;
}

GList *
get_services_list()
{
	FILE *EZ_SERVICES;
	GList *L = NULL;

	if (getenv("EZ_SERVICES") == NULL) {
		//printf("ERROR: environment variable EZ_SERVICES not defined !!!\n");
		return NULL;
	}
	if ((EZ_SERVICES = fopen(getenv("EZ_SERVICES"), "r")) == NULL) {
		//printf("No service(s) defined (unable to open $EZ_SERVICES file)\n");
		return NULL;
	}
	L = get_liste(EZ_SERVICES, LIST_NB_ITEM);
        snprintf(debugmsg,sizeof(debugmsg),"Service List Content");
        debuglog(IDENT,"get_services_list",debugmsg);
        debug_list(L);
	fclose(EZ_SERVICES);
	return (L);
}

gint
Cmd(char *prg, gchar * argsin[2])
{
	gint pid, etat;
	gchar *message;
	//gint  del, fs;

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	switch (pid = fork()) {
	case 0:
		//      close(0);
		//      close(1);
		//      close(2);
		//signal(SIGINT, del);
		//signal(SIGQUIT, fs);
		//Mettre un fstat sur le prg a executer
		execv(prg, argsin);
		message =
		    g_strconcat("Error: exec failed: ", strerror(errno), " \n",
				NULL);
		halog(LOG_WARNING, progname, message);
		g_free(message);
		exit(-1);
	case (-1):
		//fprintf(stderr,"Error in fork().\n");
		message =
		    g_strconcat("Error: in Cmd(): ", strerror(errno), " \n",
				NULL);
		halog(LOG_WARNING, progname, message);
		g_free(message);
		etat = -1;
		break;
	default:
		while (wait(&etat) != pid) ;
	}
	//printf("result: %d\n",execv(prg,arg));
	return etat;
}

gint
is_num(gchar * val)
{
	gint i = 0, RET = 0;
	if (val == NULL)
		return -1;
	while (val[i] != '\0') {
		//printf("read: %d\n",val[i]);
		if ((isdigit((int) val[i])) == 0) {
			RET = -1;
			break;
		}
		i++;
	}
	//chaine de taille 0
	if (i == 0)
		RET = -1;
	return RET;
}

gint
change_status_start(gint state, gint ostate, gchar * service, GHashTable * HT)
{
	gpointer pointer;
	gchar *arg[3];
	gchar *arg0[3];
	gboolean PRIMARY, SECONDARY;
	GList *List_services = NULL;
	gint old_state, pstate, sstate;
	gchar *primary, *secondary, *name;
	gchar *m;

	name = g_malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	if (((state == 0) || (state == 8)) &&
	    ((ostate == 0) || (ostate == 6) || (ostate == 8))) {

		//printf("Ready to start, partner node is %s , we are %s\n",VAL[ostate],VAL[state]);
		m = g_strconcat("Ready to start, partner node is ", VAL[ostate],
				", we are ", VAL[state], ".\n", NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
	} else {
		m = g_strconcat("Cannot start ", service,
				": service not in correct state (partner node is ",
				VAL[ostate], ", we are ", VAL[state], ".\n",
				NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
		g_free(name);
		return -1;
	}

	old_state = ostate;
	pointer = g_hash_table_lookup(HT, service);
	arg[0] = ((struct srvstruct *) (pointer))->script;
	arg[1] = "start";
	arg[2] = (gchar *) 0;
	arg0[0] = ((struct srvstruct *) (pointer))->check_script;
	arg0[1] = "start";
	arg0[2] = (gchar *) 0;
	// Put state service in START_READY
	write_status(service, '7', name);
	sleep(5);
	// Si l'autre a decider de START en meme temps:
	List_services = get_services_list();
	pointer = g_hash_table_lookup(HT, (gchar *) service);
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	pstate = get_status(List_services, primary, service);
	sstate = get_status(List_services, secondary, service);
	PRIMARY = is_primary(name, service);
	SECONDARY = is_secondary(name, service);
	if (PRIMARY) {
		state = pstate;
		ostate = sstate;
	} else {
		state = ostate;
		ostate = pstate;
	}
	if (old_state != ostate) {
		m = g_strconcat("Service ", service,
				" state has changed on Partner node\n", NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
		if (SECONDARY) {
			write_status(service, '0', name);
			g_list_foreach(List_services, delete_data, NULL);
			g_list_free(List_services);
			g_free(name);
			return 0;
		}
	}
	// OK, we are really ready to start. Put state service in STARTING
	write_status(service, '3', name);
	if (launch(arg0[0], arg0) != 0) {
		m = g_strconcat("Error: failed to start ", service,
				". Check script failed. State is now start-failed.\n",
				NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
		write_status(service, '4', name);
		g_list_foreach(List_services, delete_data, NULL);
		g_list_free(List_services);
		g_free(name);
		return -1;
	} else {
		m = g_strconcat("Check script for service ", service, " OK.\n",
				NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
	}
	if (launch(arg[0], arg) == 0) {
		m = g_strconcat("Service ", service, " successfully started.\n",
				NULL);
		halog(LOG_NOTICE, progname, m);
		g_free(m);
		write_status(service, '2', name);
		g_list_foreach(List_services, delete_data, NULL);
		g_list_free(List_services);
		g_free(name);
		return 0;
	} else {
		m = g_strconcat("Error: failed to start ", service,
				". State is now start-failed.\n", NULL);
		halog(LOG_WARNING, progname, m);
		g_free(m);
		write_status(service, '4', name);
		g_list_foreach(List_services, delete_data, NULL);
		g_list_free(List_services);
		g_free(name);
		return -1;
	}
}

gint
launch(gchar * command, gchar * arg[])
{
	gint i = 0;
	i = Cmd(command, arg);
	//printf("command: %s %s i: %d \n",argsIn[0],argsIn[1],i);
	if ((i > 255) || (i < 0))
		return (-1);
	else
		return (0);
}

gint
get_status(GList * liste, gchar * node, gchar * service)
{
	gint i, j, k, size;
	gchar *FILE_NAME, STATE, *m;
	FILE *FILE_STATE;

	size = g_list_length(liste) / LIST_NB_ITEM;
	for (i = 0; i < size; i++) {
		j = i * LIST_NB_ITEM;
		if (strncmp
		    (g_list_nth_data(liste, j), service,
		     MAX_SERVICES_SIZE) == 0) {
			if ((strncmp
			     (g_list_nth_data(liste, j + 2), node,
			      MAX_NODENAME_SIZE) == 0)
			    ||
			    (strncmp
			     (g_list_nth_data(liste, j + 3), node,
			      MAX_NODENAME_SIZE) == 0)) {
				FILE_NAME =
				    g_strconcat(EZ, "/services/", service,
						"/STATE.", node, NULL);

				if ((FILE_STATE =
				     fopen((char *) FILE_NAME, "r")) == NULL) {
					m = g_strconcat
					    ("Error: unable to open SERVICE STATE file ",
					     FILE_NAME, ".\n", NULL);
					halog(LOG_ERR, progname, m);
					/*
					   openlog("EZ-HA: nmond", LOG_PID|LOG_CONS, LOG_DAEMON);
					   syslog(LOG_ERR,"Error: unable to open SERVICE STATE file %s\n",FILE_NAME);
					 */
					fprintf(stderr,
						"Error: unable to open SERVICE STATE file %s\n",
						FILE_NAME);
					g_free(FILE_NAME);
					return (-1);
				}
				g_free(FILE_NAME);
				STATE = fgetc(FILE_STATE);
				fclose(FILE_STATE);
				sscanf(&STATE, "%1d", &k);
				return (k);
			}
		}
	}
	// If not found ...
	return -1;
}

//SERVICES FUNCTIONS

gint
service_rm(gchar * name, GHashTable * HT)
{
	gpointer pointer;
	gchar *env, *env1, *dir, *primary, *secondary;

	env = g_malloc0(300);
	env1 = g_malloc0(300);
	dir = g_malloc0(256);

	dir = getenv("EZ");

	if (HT != NULL)
		pointer = g_hash_table_lookup(HT, (gchar *) name);
	else
		pointer = NULL;
	if (pointer == NULL) {
		printf("Service %s not found.\n", name);
		return -1;
	}
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	if (dir == NULL) {
		fprintf(stderr, "Variable $EZ not defined.\n");
		return -1;
	}
	strcat(dir, "/services/");
	strcat(dir, name);
	strcpy(env, dir);
	strcat(env, "/STATE.");
	strcpy(env1, env);
	strcat(env, primary);
	strcat(env1, secondary);
	if (rm_file(env) != 0)
		fprintf(stderr, "Unable to remove file %s \n", env);
	if (rm_file(env1) != 0)
		fprintf(stderr, "Unable to remove file %s \n", env1);
	if (rmdir(dir) != 0) {
		perror("Error");
		return -1;
	}
	if (service_mod(name) != 0) {
		fprintf(stderr, "Failed\n");
		exit(-1);
	}
	printf("Remove of service %s done\n", name);
	return 0;
}

gint
service_mod(gchar * name)
{
	FILE *EZ_SERVICES, *TMP;
	gchar lu[5][300];
	gchar *tmp, *services;

	tmp = g_malloc0(300);
	services = g_malloc0(300);
	tmp = getenv("EZ");
	services = getenv("EZ_SERVICES");

	if (getenv("EZ_SERVICES") == 0) {
		printf
		    ("ERROR: environment variable EZ_SERVICES not defined !!!\n");
		exit(-1);
	}
	if ((EZ_SERVICES = fopen((char *) getenv("EZ_SERVICES"), "r+")) == NULL) {
		perror("Error: unable to open $EZ_SERVICES");
		exit(-1);
	}

	strcat(tmp, ".services.tmp");

	if ((TMP = fopen(tmp, "w")) == NULL) {
		perror("Error: unable to open $EZ/.services.tmp");
		fclose(EZ_SERVICES);
		exit(-1);
	}

	while (fscanf
	       (EZ_SERVICES, "%s %s %s %s %s", lu[0], lu[1], lu[2], lu[3],
		lu[4]) != -1) {
		if (strcmp(name, lu[0]) != 0) {
			fprintf(TMP, "%s %s %s %s %s\n", lu[0], lu[1], lu[2],
				lu[3], lu[4]);
		}
	}

	fclose(EZ_SERVICES);
	fclose(TMP);
	if (rename(tmp, services) != 0) {
		perror("Rename failed:");
		return -1;
	}
	return 0;

}

gint
service_add(gchar * name, gchar * startup_script,
	    gchar * primary, gchar * secondary,
	    gchar * check_script, GHashTable * HT)
{
	gpointer pointer;
	FILE *FILE_STATE, *EZ_SERVICES, *SCRIPT;
	struct hostent *host;
	struct srvstruct *srv;
	gint len;
	struct stat buf;

	if (HT != NULL)
		pointer = g_hash_table_lookup(HT, (gchar *) name);
	else
		pointer = NULL;

	if (pointer != NULL) {
		printf("Service %s already created.\n", name);
		return -1;
	}
	if ((FILE_STATE = fopen((char *) startup_script, "r")) == NULL) {
		fprintf(stderr,
			"Error: unable to open startup script file %s\n",
			startup_script);
		return -1;
	}
	if ((FILE_STATE = fopen((char *) check_script, "r")) == NULL) {
		fprintf(stderr, "Error: unable to open check script file %s\n",
			check_script);
		return -1;
	}

	host = gethostbyname(primary);
	if (host == NULL) {
		fprintf(stderr, "Error: unknown host %s\n", primary);
		return -1;
	}
	host = gethostbyname(secondary);
	if (host == NULL) {
		fprintf(stderr, "Error: unknown host %s\n", secondary);
		return -1;
	}

	if (strcmp(primary, secondary) == 0) {
		fprintf(stderr,
			"ERROR: primary and secondary node must differ.\n");
		return -1;
	}
	if (getenv("EZ_SERVICES") == NULL) {
		fprintf(stderr,
			"ERROR: environment variable EZ_SERVICES not defined !!!\n");
		return -1;
	}
	if ((EZ_SERVICES = fopen(getenv("EZ_SERVICES"), "a+")) == NULL) {
		fprintf(stderr, "Unable to open $EZ_SERVICES file)\n");
		return -1;
	}
	if ((SCRIPT = fopen(startup_script, "r")) == NULL) {
		fprintf(stderr, "Unable to open startup_script file)\n");
		return -1;
	}

	if (fstat(fileno(SCRIPT), &buf) < 0) {
		fprintf(stderr, "Unable to stat %s\n", startup_script);
		return -1;
	}
	if (!(buf.st_mode & (S_IXUSR | S_IRUSR))) {
		printf("Startup script %s mode not valid\n", startup_script);
		return -1;
	}

	fseek(EZ_SERVICES, 0L, SEEK_END);
	srv = g_malloc0(sizeof (struct srvstruct));

	len = strlen(name);
	strncpy(srv->service_name, name, len);
	fwrite(srv->service_name, len, 1, EZ_SERVICES);
	fwrite("\t", 1, 1, EZ_SERVICES);

	len = strlen(startup_script);
	strncpy(srv->script, startup_script, len);
	fwrite(srv->script, len, 1, EZ_SERVICES);
	fwrite("\t", 1, 1, EZ_SERVICES);

	len = strlen(primary);
	strncpy(srv->primary, primary, len);
	fwrite(srv->primary, len, 1, EZ_SERVICES);
	fwrite("\t", 1, 1, EZ_SERVICES);

	len = strlen(secondary);
	strncpy(srv->secondary, secondary, len);
	fwrite(srv->secondary, len, 1, EZ_SERVICES);
	fwrite("\t", 1, 1, EZ_SERVICES);

	len = strlen(check_script);
	strncpy(srv->check_script, check_script, len);
	fwrite(srv->check_script, len, 1, EZ_SERVICES);
	fwrite("\n", 1, 1, EZ_SERVICES);
	fclose(EZ_SERVICES);

	create_dir(name);
	create_file(name, primary);
	create_file(name, secondary);

	return 0;
}

gint
rm_file(gchar * name)
{
	if (unlink(name) != 0) {
		perror("Error:");
		return -1;
	}
	return 0;
}

gint
create_file(gchar * name, gchar * node)
{
	gchar *O, *env;
	struct stat buf;
	FILE *F;
	gchar to_copy[2];

	O = g_malloc0(256);
	env = g_malloc0(256);

	/* Bona's patch: when a service is created, its initial state is now FROZEN-STOP, instead of UNKNOWN */
	to_copy[0] = '6';
	to_copy[1] = '\n';

	env = getenv("EZ");
	strcat(O, env);

	if (O == NULL) {
		fprintf(stderr, "Variable $EZ not defined.\n");
		return -1;
	}
	strcat(O, "/services/");
	strcat(O, name);
	strcat(O, "/");

	if (stat(O, &buf) == 0) {
		strcat(O, "STATE.");
		strcat(O, node);
		if ((F = fopen(O, "w")) == NULL) {
			perror("Unable to create file:");
			return -1;
		}
		fwrite(to_copy, 2, 1, F);
		fclose(F);
		return 0;
	} else
		return -1;
}

gint
create_dir(gchar * name)
{
	gchar *File, *env;
	struct stat buf;

	env = g_malloc0(256);
	File = g_malloc0(256);
	env = getenv("EZ");
	strcat(File, env);
	if (File == NULL) {
		fprintf(stderr, "Variable $EZ not defined.\n");
		return -1;
	}
	strcat(File, "/services/");
	strcat(File, name);
	if (stat(File, &buf) != 0) {
		if (mkdir(File, 0744) != 0) {
			perror("Make of services directory failed:");
			g_free(File);
			return -1;
		}
	}

	printf("Make of services directory done\n");
	return 0;
}

gint
find_action(gchar ** tab, gchar * action)
{
	int i = 0;
	for (i = 0; i < MAX_ACTION; i++) {
		if (strcmp(action, tab[i]) == 0)
			return i;
	}
	return -1;
}

void
print_func(gpointer key, gpointer value, gpointer HT)
{
	gchar check_script[SCRIPT_SIZE];

	strcat(check_script, ((struct srvstruct *) (value))->check_script);
	printf("key: %s value %s\n", (gchar *) key, check_script);
}

void
service_info(GList * liste, GHashTable * HT, gchar * name, gchar * service)
{
	gint pstate, sstate;
	gpointer pointer;
	gchar *primary, *secondary;

	if (is_primary(name, service)) {
		printf("we are Primary for service %s\n", service);
	}
	if (is_secondary(name, service)) {
		printf("we are Secondary for service %s\n", service);
	}
	pointer = g_hash_table_lookup(HT, service);
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	sstate = get_status(liste, secondary, service);
	pstate = get_status(liste, primary, service);
	printf("Service: %s\t state: %s , %s\n", service, VAL[pstate],
	       VAL[sstate]);
	return;
}

void
service_status(GList * liste, GHashTable * HT)
{
	gint i, list_size, pstate, sstate;
	gpointer pointer;
	gchar *service, *primary, *secondary;

	list_size = g_list_length(liste) / LIST_NB_ITEM;
	printf("%d service(s) defined:  \n", list_size);
	for (i = 0; i < list_size; i++) {
		service = g_list_nth_data(liste, i * LIST_NB_ITEM);
		pointer = g_hash_table_lookup(HT, service);
		primary = ((struct srvstruct *) (pointer))->primary;
		secondary = ((struct srvstruct *) (pointer))->secondary;
		pstate = get_status(liste, primary, service);
		sstate = get_status(liste, secondary, service);
		printf
		    ("Service: %s\n\tPrimary  : %s, %s\n\tSecondary: %s, %s\n",
		     service, primary, VAL[pstate], secondary, VAL[sstate]);
	}
}

gint
change_status_stop(gint state, gint ostate, gchar * service, GHashTable * HT)
{
	gpointer pointer;
	gchar *arg[3];
	gchar *name, *m = NULL;

	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);

	if ((state == 2) || (state == 8)) {
		m = g_strconcat("Ready to stop, partner node is ", VAL[ostate],
				" we are ", VAL[state], ".\n", NULL);
		halog(LOG_NOTICE, progname, m);
#ifdef VERBOSE
		printf("%s", m);
#endif
		g_free(m);
	} else {
		m = g_strconcat("Cannot stop, ", service,
				": service not in correct state (partner node is ",
				VAL[ostate], ", we are ", VAL[state], ".\n",
				NULL);
		halog(LOG_NOTICE, progname, m);
#ifdef VERBOSE
		printf
		    ("Cannot stop %s: service not in correct state (partner node is %s , we are %s),\n",
		     service, VAL[ostate], VAL[state]);
#endif
		g_free(m);
		return -1;
	}

	if (ostate != 0) {
		m = g_strconcat
		    ("Warning: PARTNER is not in STOPPED state: service will not migrate.\n",
		     NULL);
		halog(LOG_NOTICE, progname, m);
#ifdef VERBOSE
		printf
		    ("Warning: PARTNER is not in STOPPED state: service will not migrate.\n");
#endif
		g_free(m);
	}
	pointer = g_hash_table_lookup(HT, service);
	arg[0] = ((struct srvstruct *) (pointer))->script;
	arg[1] = "stop";
	arg[2] = (gchar *) 0;
	write_status(service, '1', name);
	if (launch(arg[0], arg) == 0) {
		m = g_strconcat("Service ", service, " successfully stopped\n",
				NULL);
#ifdef VERBOSE
		printf("Service %s successfully stopped\n", service);
#endif
		write_status(service, '0', name);
		g_free(m);
		return 0;
	} else {
		m = g_strconcat("Error: failed to stop ", service, ".\n", NULL);
#ifdef VERBOSE
		printf("Error: failed to stop %s.\n", service);
#endif
		write_status(service, '5', name);
		return -1;
	}
}

gint
change_status_force_stop(gint state, gint ostate, gchar * service,
			 GHashTable * HT)
{
	gchar *name, *message = NULL;

	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	progname = getenv("PROGNAME");

#ifdef VERBOSE
	printf("Ready to force stop, partner node is %s , we are %s\n",
	       VAL[ostate], VAL[state]);
#endif
	message =
	    g_strconcat("Ready to force stop, partner node is ", VAL[ostate],
			", we are ", VAL[state], "\n", NULL);
	halog(LOG_INFO, progname, message);
	g_free(message);

	write_status(service, '0', name);
#ifdef VERBOSE
	printf("Service %s successfully stopped\n", service);
#endif
	message =
	    g_strconcat("Service ", service, " successfully stopped\n", NULL);
	halog(LOG_INFO, progname, message);
	g_free(message);
	return 0;
}

gint
change_status_force_start(gint state, gint ostate, gchar * service,
			  GHashTable * HT)
{
	gchar *name, *message = NULL;

	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);

#ifdef VERBOSE
	printf("Ready to force start, partner node is %s , we are %s\n",
	       VAL[ostate], VAL[state]);
#endif
	message =
	    g_strconcat("Ready to force start, partner node is ", VAL[ostate],
			", we are ", VAL[state], ".\n", NULL);
	halog(LOG_INFO, progname, message);

#ifdef VERBOSE
	printf("Service %s forced to started\n", service);
#endif
	message =
	    g_strconcat("Service ", service, " forced to started\n", NULL);
	halog(LOG_INFO, progname, message);

	write_status(service, '2', name);
	return 0;
}

gint
change_status_freeze_stop(gint state, gint ostate, gchar * service,
			  GHashTable * HT)
{
	gchar *name, *message = NULL;

	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	if ((state == 0) || (state == 2) || (state == 4) || (state == 5)) {
		message =
		    g_strconcat("Ready to FREEZE, we are ", VAL[state], ".\n",
				NULL);
		halog(LOG_INFO, progname, message);
		g_free(message);

#ifdef VERBOSE
		printf("Ready to FREEZE, we are %s\n", VAL[state]);
#endif
		if ((state == 0) || (state == 4) || (state == 5)) {
			write_status(service, '6', name);
			message =
			    g_strconcat("Service ", service, " FROZEN-STOP.\n",
					NULL);
			halog(LOG_INFO, progname, message);
			g_free(message);
#ifdef VERBOSE
			printf("Service %s FROZEN-STOP\n", service);
#endif
			return 0;
		}
		if (state == 2) {
			if (change_status_stop(state, ostate, service, HT) == 0) {
				write_status(service, '6', name);
#ifdef VERBOSE
				printf("Service %s FROZEN-STOP\n", service);
#endif
				message =
				    g_strconcat("Service ", service,
						" FROZEN-STOP.\n", NULL);
				halog(LOG_INFO, progname, message);
				g_free(message);
				return 0;
			} else {
#ifdef VERBOSE
				printf
				    ("Warning: error in stopping service %s\n",
				     service);
#endif
				message =
				    g_strconcat("error in stopping service ",
						service, ".\n", NULL);
				halog(LOG_INFO, progname, message);
				g_free(message);
				return -1;
			}
		}
	}
#ifdef VERBOSE
	printf
	    ("Cannot FREEZE-STOP %s: service not in STARTED/STOPPED state (we are %s),\n",
	     service, VAL[state]);
#endif
	message =
	    g_strconcat("Cannot FREEZE-STOP ", service,
			": service not in STARTED/STOPPED state (we are ",
			VAL[state], ".\n", NULL);
	halog(LOG_INFO, progname, message);
	g_free(message);
	return -1;
}

gint
change_status_freeze_start(gint state, gint ostate, gchar * service,
			   GHashTable * HT)
{
	gchar *name;

	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);

	if ((state == 0) || (state == 2)) {
		printf("Ready to FREEZE, we are %s\n", VAL[state]);
		if (state == 2) {
			write_status(service, '7', name);
			printf("Service %s FROZEN\n", service);
			return 0;
		}
		if (state == 0) {
			if (change_status_start(state, ostate, service, HT) ==
			    0) {
				write_status(service, '7', name);
				printf("Service %s FROZEN-START\n", service);
				return 0;
			} else {
				printf
				    ("Warning: error in starting service %s\n",
				     service);
				return -1;
			}
		}
	}
	printf
	    ("Cannot FREEZE-START %s: service not in STARTED/STOPPED state (we are %s),\n",
	     service, VAL[state]);
	return -1;
}

gint
change_status_unfreeze(gint state, gchar * service, GHashTable * HT)
{
	gchar *name, *message = NULL;
	name = malloc(MAX_NODENAME_SIZE);
	get_my_name(name);
	if ((state == 6) || (state == 7)) {
#ifdef VERBOSE
		printf("Ready to UNFREEZE, we are %s\n", VAL[state]);
#endif
		message =
		    g_strconcat("Ready to UNFREEZE, we are (", VAL[state],
				").\n", NULL);
		halog(LOG_INFO, progname, message);
		g_free(message);
		if (state == 6)
			write_status(service, '0', name);
		if (state == 7)
			write_status(service, '2', name);
#ifdef VERBOSE
		printf("Service %s UNFROZEN\n", service);
#endif
		message =
		    g_strconcat("Service ", service, " UNFROZEN.\n", NULL);
		halog(LOG_INFO, progname, message);
		g_free(message);
		return 0;
	} else {
		message =
		    g_strconcat("Cannot UNFREEZE ", service,
				": not in FREEZE state (we are ", VAL[state],
				").\n", NULL);
		halog(LOG_INFO, progname, message);
		g_free(message);
#ifdef VERBOSE
		printf
		    ("Cannot UNFREEZE %s: service not in FREEZE state (we are %s),\n",
		     service, VAL[state]);
#endif
		return -1;
	}
}

gint
if_getaddr(const char *ifname, struct in_addr * addr)
{
	gint fd;
	struct ifreq if_info;

	if (!addr)
		return -1;
	addr->s_addr = INADDR_ANY;
	memset(&if_info, 0, sizeof (if_info));
	if (ifname) {
		strncpy(if_info.ifr_name, ifname, 16);
	} else {
		return 0;
	}

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		halog(LOG_ERR, "heartc", "Error getting socket\n");
		return -1;
	}
	if (ioctl(fd, SIOCGIFADDR, &if_info) == -1) {
		halog(LOG_ERR, "heartc", "Error ioctl(SIOCGIFADDR)\n");
		close(fd);
		return -1;
	}

/*
 * This #define w/void cast is to quiet alignment errors on some
 * platforms (notably Solaris)
 */
#define SOCKADDR_IN(a)        ((struct sockaddr_in *)((void*)(a)))

	memcpy(addr, &(SOCKADDR_IN(&if_info.ifr_addr)->sin_addr)
	       , sizeof (struct in_addr));

	close(fd);
	return 0;
}

/*
*/
gint
set_mcast_if(gint sockfd, gchar * ifname)
{
	gint rc, i = 0;
	struct in_addr mreq;

	memset(&mreq, 0, sizeof (mreq));
	rc = if_getaddr(ifname, &mreq);
	if (rc == -1) {
		//strcpy(message," failed. Bad interface name ? \n");
		//halog(LOG_ERR,"if_getaddr: ",message);
		perror("set_mcast_if");
		return (-1);
	}
	i = setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (void *) &mreq,
		       sizeof (mreq));
	if (i < 0) {
		halog(LOG_ERR, "heartc", "setsockopt\n");
		return (-1);
	}
	return i;
}

gboolean
rm_func_serv(gpointer key, gpointer value, gpointer user_data)
{
	g_free((struct srvstruct *) value);
	return TRUE;
}

gboolean
init_var()
{
	EZ = g_malloc0(256);
	EZ_BIN = g_malloc0(256);
	EZ_MONITOR = g_malloc0(256);
	EZ_SERVICES = g_malloc0(256);
	EZ_LOG = g_malloc0(256);
	EZ_NODES = g_malloc0(256);

	if ((EZ_BIN = getenv("EZ_BIN")) == NULL) {
		printf("Error: variable EZ_BIN not defined\n");
		return FALSE;
	}
	if ((EZ_MONITOR = getenv("EZ_MONITOR")) == NULL) {
		printf("Error: variable EZ_MONITOR not defined\n");
		return FALSE;
	}
	if ((EZ_SERVICES = getenv("EZ_SERVICES")) == NULL) {
		printf("Error: variable EZ_SERVICES not defined\n");
		return FALSE;
	}
	if ((EZ_LOG = getenv("EZ_LOG")) == NULL) {
		printf("Error: variable EZ_LOG not defined\n");
		return FALSE;
	}
	if ((EZ_NODES = getenv("EZ_NODES")) == NULL) {
		printf("Error: variable EZ_NODES not defined\n");
		return FALSE;
	}
	if ((EZ = getenv("EZ")) == NULL) {
		printf("Error: variable EZ not defined\n");
		return FALSE;
	}
	return TRUE;
}

void
daemonize(gchar * message)
{
	gint pid;

	switch (pid = fork()) {
	case 0:
		halog(LOG_INFO, message, "starting ...");
		break;
	case (-1):
		fprintf(stderr, "Error in fork().\n");
		halog(LOG_ERR, message, "Error in fork()");
		exit(-1);
	default:
		exit(0);
	}
}

void
Setenv(gchar * name, gchar * value, gint why)
{
	gchar *env;
	env = g_strconcat(name, "=", value, NULL);
	putenv(env);
}
