#ifndef CLUSTER_H
#define CLUSTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

/*
 * Service status
 */
#define MAX_STATE 10

#define STATE_STOPPED 0
#define STATE_STOPPING 1
#define STATE_STARTED 2
#define STATE_STARTING 3
#define STATE_START_FAILED 4
#define STATE_STOP_FAILED 5
#define STATE_FROZEN_STOP 6
#define STATE_START_READY 7
#define STATE_UNKNOWN 8
#define STATE_FORCE_STOP 9

#define MAX_PATH_SIZE 256
#define MAX_PROGNAME_SIZE 16
#define MAX_SERVICES 128
#define MAX_SERVICES_SIZE 64
#define MAX_HEARTBEAT 16
#define MAX_NODENAME_SIZE 128
#define MAX_NODES 16
#define MAX_ITEM 256
#define MAX_ACTION 7
#define SCRIPT_SIZE 128
#define LIST_NB_ITEM 5	/* nb colonne dans fichier services / nb item dans liste */
#define SHMSZ sizeof(struct sendstruct) * MAX_SERVICES
#define MAX_LOG_MSG_SIZE 1024
#define BLKSIZE 512

gchar *EZ_BIN;
gchar *EZ_VAR;
gchar *EZ_SERVICES;
gchar *EZ_MONITOR;
gchar *EZ_NODES;
gchar *EZ_DEBUG;
extern gchar progname[MAX_PROGNAME_SIZE];

gchar *VAL[MAX_STATE];

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
	gint service_state[MAX_SERVICES];
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

time_t GlobalListTimeStamp;
GList *GlobalList;
GHashTable *GLOBAL_HT_SERV;
extern gint loglevel;

gchar nodename[MAX_NODENAME_SIZE];

void get_services_list(void);
gint get_status(GList *, gchar *, gchar *);
gint launch(gchar *, gchar **);

void print_func(gpointer, gpointer, gpointer);
gboolean rm_func_serv(gpointer, gpointer, gpointer);
void service_info(GList *, GHashTable *, gchar *, gchar *);
gboolean is_service(GList *, gchar *);
void service_status(GList *, GHashTable *);
void service_status_cols(GList *, GHashTable *);
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
void signal_usr1_callback_handler();
void signal_usr2_callback_handler();
gboolean init_var();
void get_nodename();
GHashTable * get_hash(GList *);
gboolean is_primary(gchar *, gchar *);
gboolean is_secondary(gchar *, gchar *);
gint change_status_force_stop(gint, gint, gchar *, GHashTable *);
gint change_status_force_start(gint, gint, gchar *, GHashTable *);
void daemonize(gchar *);
gint if_getaddr(const char *ifname, struct in_addr *);
gint write_status(gchar *, gint, gchar *);
glong Elapsed(void);
void delete_data(gpointer, gpointer);
void drop_list(GList *);
void drop_hash(GHashTable *);
void get_liste(FILE *, guint);
GList * get_liste_generic(FILE *, guint);
int halog(int prio, const char * fmt, ...);
gint get_node_service_status(struct sendstruct *, gchar *, guint);
gint get_node_status(struct sendstruct *);
gint create_state_tree(gchar *, gchar *, gchar *);
gchar * get_shm_segment(gchar *);
gchar * get_shm_nmon_ro_segment();
#endif /* CLUSTER_H */
