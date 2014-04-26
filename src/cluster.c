#include <cluster.h>

gint loglevel = LOG_INFO;
gchar progname[MAX_PROGNAME_SIZE] = {};

gchar *VAL[MAX_STATE] = {
        "STOPPED", "STOPPING", "STARTED",
        "STARTING", "START_FAILED", "STOP_FAILED",
        "FROZEN_STOP", "START_READY", "UNKNOWN",
        "FORCE-STOP"
};

void
delete_data(gpointer data, gpointer user_data)
{
	g_free(data);
	data = NULL;
}

void
debug_list(GList * liste)
{
	halog(LOG_DEBUG, "[debug_list] enter");
	gint i = 0;

	for (i = 0; i <= (g_list_length(liste) / LIST_NB_ITEM) - 1; i++) {
		halog(LOG_DEBUG, "[debug_list] i=%d SVC[%s] SCRIPT[%s] PRI[%s] SEC[%s] CHK[%s]",
			 i,
			 (char *) g_list_nth_data(liste, i * LIST_NB_ITEM),
			 (char *) g_list_nth_data(liste,
						  (i * LIST_NB_ITEM) + 1),
			 (char *) g_list_nth_data(liste,
						  (i * LIST_NB_ITEM) + 2),
			 (char *) g_list_nth_data(liste,
						  (i * LIST_NB_ITEM) + 3),
			 (char *) g_list_nth_data(liste,
						  (i * LIST_NB_ITEM) + 4));
	}
}

void
signal_usr1_callback_handler()
{
	if (loglevel == LOG_DEBUG)
		return;
	gint prev = loglevel;
	loglevel += 1;
	halog(LOG_INFO, "loglevel changed from %d to %d", prev, loglevel);
}

void
signal_usr2_callback_handler()
{
	if (loglevel == LOG_CRIT)
		return;
	gint prev = loglevel;
	loglevel -= 1;
	halog(LOG_INFO, "loglevel changed from %d to %d", prev, loglevel);
}

glong
Elapsed(void)
{
	halog(LOG_DEBUG, "[Elapsed] enter");
	gint ts;
	struct timeval buf;
	if ((ts = gettimeofday(&buf, NULL)) == 0)
		return buf.tv_sec;
	else
		return -1;
/* printf("Time: %ld  %ld\n",buf.tv_sec,buf.tv_usec); */
}

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
get_nodename()
{
	halog(LOG_DEBUG, "[get_nodename] enter");
	struct utsname tmp_name;
	uname(&tmp_name);
	strncpy(nodename, tmp_name.nodename, MAX_NODENAME_SIZE);
}

gboolean
is_primary(gchar * node, gchar * service)
{
	/* halog(LOG_DEBUG, "[is_primary] enter"); */
	gchar *primary;
	gpointer ptr;

	ptr = g_hash_table_lookup(GLOBAL_HT_SERV, service);
	primary = ((struct srvstruct *) (ptr))->primary;
    
	if (strncmp(primary, node, MAX_NODENAME_SIZE) == 0)
		return TRUE;
	return FALSE;
}

gboolean
is_secondary(gchar * node, gchar * service)
{
	/* halog(LOG_DEBUG, "[is_secondary] enter"); */
	gchar *secondary;
	gpointer ptr;

	ptr = g_hash_table_lookup(GLOBAL_HT_SERV, service);
	secondary = ((struct srvstruct *) (ptr))->secondary;
    
	if (strncmp(secondary, node, MAX_NODENAME_SIZE) == 0)
		return TRUE;
	return FALSE;
}

gchar *
get_our_secondary(gchar * node, gchar * service, GList * liste)
{
	halog(LOG_DEBUG, "[get_our_secondary] enter");
	gint i = 0, j, list_size;
	gchar *service_to_cmp, *node_to_cmp;

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
	halog(LOG_DEBUG, "{read_lock] enter");
	return (fcntl(fd, F_SETLKW, file_lock(F_RDLCK, SEEK_SET)));
}

gint
write_lock(gint fd)
{				/* an exclusive lock on an entire file */
	halog(LOG_DEBUG, "[write_lock] enter");
	return (fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_SET)));
}

gint
append_lock(gint fd)
{				/* a lock on the _end_ of a file -- other
				   processes may access existing records */
	halog(LOG_DEBUG, "[append_lock] enter");
	return (fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_END)));
}

void
write_status(gchar service[MAX_SERVICES_SIZE], gint state, gchar * node)
{
	halog(LOG_DEBUG, "[write_status] enter");
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
				g_free(FILE_NAME);
				return;
			} else {
				snprintf(to_copy, 2, "%i\n", state);
				fwrite(to_copy, 2, 1, FILE_STATE);
				fflush(FILE_STATE);
				g_free(FILE_NAME);
				fclose(FILE_STATE);
			}
		} else {
			printf("Lock Unsuccessful\n");
			perror("lockf:");
			fclose(FILE_STATE);
			g_free(FILE_NAME);
			return;
		}
	} else {
		//perror("No service(s) defined (unable to open SERVICE STATE file)");
		g_free(FILE_NAME);
		return;
	}
}

GList *
list_copy_deep(GList * src)
{
	GList *dst;

	dst = NULL;

	for (; src != NULL; src = g_list_next(src)) {
		dst = g_list_append(dst, g_strdup(src->data));
	}

	return dst;
}

void
drop_list(GList *liste)
{
	if (liste == NULL)
		return;
	g_list_foreach(liste, delete_data, liste);
	g_list_free(liste);
}

void
drop_hash(GHashTable *HT)
{
	if (HT == NULL)
		return;
	g_hash_table_foreach_remove(HT, rm_func_serv, HT);
}

GHashTable *
get_hash(GList * liste)
{
	halog(LOG_DEBUG, "[get_hash] enter");
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
get_liste_generic(FILE * File, guint elem)
{
	/* appel direct depuis nmond.c func init() */
	halog(LOG_DEBUG, "[get_liste_generic] enter");
	GList *L = NULL;
	gint i = 0, j = 0, k = 0, l = 0, LAST;
	gchar *s = NULL;
	gchar item[MAX_ITEM];
	gchar tmp_tab[MAX_ITEM][MAX_ITEM];

	for (i = 0; i < MAX_ITEM; i++)
		for (j = 0; j < MAX_ITEM; j++)
			tmp_tab[i][j] = '\0';

	i = 0;
	j = 0;
	// Count the number of lines
	while (fgets(tmp_tab[i], MAX_ITEM - 1, File) != NULL) {
		size_t ln = strlen(tmp_tab[i]) - 1;
		if (tmp_tab[i][ln] == '\n')
			tmp_tab[i][ln] = '\0';
		/*
		   halog(LOG_DEBUG, "[get_liste_generic] loading line tmp_tab[%d] => [%s]", i, tmp_tab[i]);
		 */
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
			/*
			   halog(LOG_DEBUG, "[get_liste_generic] item [%s] - item len [%d]", item, strlen(item));
			 */
			k = 0;
			s = g_strdup(item);
			L = g_list_append(L, s);
			item[0] = '\0';
		}
	}
	fseek(File, 0L, SEEK_SET);
	return L;
}

void
get_liste(FILE * File, guint elem)
{
	halog(LOG_DEBUG, "[get_liste] enter");
	gint i = 0, j = 0, k = 0, l = 0, LAST;
	gchar *s = NULL;
	gchar item[MAX_ITEM];
	gchar tmp_tab[MAX_ITEM][MAX_ITEM];

	drop_list(GlobalList);
	GlobalList = NULL;

	for (i = 0; i < MAX_ITEM; i++)
		for (j = 0; j < MAX_ITEM; j++)
			tmp_tab[i][j] = '\0';

	i = 0;
	j = 0;
	// Count the number of lines
	while (fgets(tmp_tab[i], MAX_ITEM - 1, File) != NULL) {
		size_t ln = strlen(tmp_tab[i]) - 1;
		if (tmp_tab[i][ln] == '\n')
			tmp_tab[i][ln] = '\0';
/*
		halog(LOG_DEBUG, "[get_liste] loading service line tmp_tab[%d] => [%s]", i, tmp_tab[i]);
 */
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
/*
			halog(LOG_DEBUG, "[get_liste] item [%s] - item len [%d]", item, strlen(item));
 */
			k = 0;
			s = g_strdup(item);
			GlobalList = g_list_append(GlobalList, s);
			item[0] = '\0';
		}
	}
	fseek(File, 0L, SEEK_SET);
	time(&GlobalListTimeStamp);
	/* on rafraichi aussi la hash table globale */
	/* attention les états de service ne sont pas tenus à jour */
	/* utile pour identifier quel noeud est primaire/secondaire pour quel service */
	drop_hash(GLOBAL_HT_SERV);
	GLOBAL_HT_SERV = get_hash(GlobalList);
	return;
}

gboolean
need_refresh(gchar * path2file, time_t refstamp)
{
	halog(LOG_DEBUG, "[need_refresh] enter");

	struct stat *buf;
	time_t filestamp;
	double diff_t;

	buf = malloc(sizeof (struct stat));

	stat(path2file, buf);
	filestamp = buf->st_mtime;

	diff_t = difftime(filestamp, refstamp);
	free(buf);

	if (diff_t > 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void
get_services_list()
{
	halog(LOG_DEBUG, "[get_services_list] enter");

	FILE *EZFD_SERVICES;

	if ((EZ_SERVICES = getenv("EZ_SERVICES")) == NULL) {
		//printf("ERROR: environment variable EZ_SERVICES not defined !!!\n");
		drop_list(GlobalList);
		GlobalList = NULL;
		return;
	}

	if (GlobalList == NULL || need_refresh(EZ_SERVICES, GlobalListTimeStamp)) {
		if ((EZFD_SERVICES = fopen(EZ_SERVICES, "r")) == NULL) {
			//printf("No service(s) defined (unable to open $EZ_SERVICES file)\n");
			drop_list(GlobalList);
			GlobalList = NULL;
			return;
		}
		get_liste(EZFD_SERVICES, LIST_NB_ITEM);
/*
		halog(LOG_DEBUG, "[get_services_list] REFRESHED Service List Content"); 
 */
		fclose(EZFD_SERVICES);
	} else {
/*
		 halog(LOG_DEBUG, "[get_services_list] GLOBAL Service List Content");
 */
	}
	/* debug_list(L); */
	return;
}

gint
Cmd(char *prg, gchar * argsin[2])
{
	halog(LOG_DEBUG, "[Cmd] enter");
	gint pid, status, retval;
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
		retval = execv(prg, argsin);
		halog(LOG_WARNING, "Error: exec failed: %s", strerror(errno));
		exit(-1);
	case (-1):
		halog(LOG_WARNING, "Error: fork error in Cmd(): %s", strerror(errno));
		return -1;
	default:
		wait(&status);
                if (WIFEXITED(status)) {
                        status = WEXITSTATUS(status);
                        if (status == 0)
                                retval = 0;
                        else
                                halog(0, "%s exitted with %d", prg, status);
                }
                else if (WIFSIGNALED(status))
                        halog(0, "%s was terminated by signal %d", prg, WTERMSIG(status));
                else
                        halog(0, "%s terminated abnormally", prg);

	}
	return retval;
}

gint
is_num(gchar * val)
{
	halog(LOG_DEBUG, "[is_num] enter");
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
	halog(LOG_DEBUG, "[change_status_start] enter");
	gpointer pointer;
	gchar *arg[3];
	gchar *arg0[3];
	gboolean PRIMARY, SECONDARY;
	GList *List_services = NULL;
	gint old_state, pstate, sstate;
	gchar *primary, *secondary;
	gint r = 0;

	if ((state == STATE_STOPPED || state == STATE_UNKNOWN) &&
	    (ostate == STATE_STOPPED || ostate == STATE_FROZEN_STOP || ostate == STATE_UNKNOWN)) {
		halog(LOG_NOTICE, "Ready to start, partner node is %s , we are %s",
			VAL[ostate], VAL[state]);
	} else {
		halog(LOG_NOTICE, "Cannot start %s: service not in correct state (partner node is %s, we are %s)",
			service, VAL[ostate], VAL[state]);
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
	write_status(service, STATE_START_READY, nodename);
	sleep(5);
	// Si l'autre a decider de START en meme temps:
	get_services_list();
	pointer = g_hash_table_lookup(HT, (gchar *) service);
	primary = ((struct srvstruct *) (pointer))->primary;
	secondary = ((struct srvstruct *) (pointer))->secondary;
	pstate = get_status(List_services, primary, service);
	sstate = get_status(List_services, secondary, service);
	PRIMARY = is_primary(nodename, service);
	SECONDARY = is_secondary(nodename, service);
	if (PRIMARY) {
		state = pstate;
		ostate = sstate;
	} else {
		state = ostate;
		ostate = pstate;
	}
	if (old_state != ostate) {
		halog(LOG_NOTICE, "service %s state has changed on partner node", service);
		if (SECONDARY) {
			write_status(service, STATE_STOPPED, nodename);
			r = 0;
			goto out;
		}
	}
	// OK, we are really ready to start. Put state service in STARTING
	write_status(service, STATE_STARTING, nodename);
	if (launch(arg0[0], arg0) != 0) {
		halog(LOG_ERR, "failed to start %s. Check script failed. State is now start-failed", service);
		write_status(service, STATE_START_FAILED, nodename);
		r = -1;
		goto out;
	} else {
		halog(LOG_NOTICE, "Check script for service %s OK", service);
	}
	if (launch(arg[0], arg) == 0) {
		halog(LOG_NOTICE, "Service %s successfully started", service);
		write_status(service, STATE_STARTED, nodename);
		r = 0;
		goto out;
	} else {
		halog(LOG_ERR, "failed to start %s. State is now start-failed", service);
		write_status(service, STATE_START_FAILED, nodename);
		r = -1;
		goto out;
	}
out:
	g_list_foreach(List_services, delete_data, NULL);
	g_list_free(List_services);
	return r;
}

gint
launch(gchar * command, gchar * arg[])
{
	halog(LOG_DEBUG, "[launch] enter");
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
	/* halog(LOG_DEBUG, "[get_status] enter"); */
	gint i, j, k = 0, size;
	gchar *FILE_NAME, STATE;
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
					halog(LOG_ERR, "unable to open SERVICE STATE file %s", FILE_NAME);
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
	halog(LOG_DEBUG, "[service_rm] enter");
	gpointer pointer;
	gchar *envdir, *file1, *file2, *dir, *primary, *secondary;

	dir = g_malloc0(256);
	envdir = g_malloc0(300);
	file1 = g_malloc0(300);
	file2 = g_malloc0(300);

	dir = getenv("EZ");
	strcpy(envdir, dir);
	strcat(envdir, "/services/");
	strcat(envdir, name);

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
	/*
    retire le service du fichier de config en premier
    permet aux autres process de comprendre que le service est supprime
    evite les controles d etat sur le service en cours de suppression
	*/
	if (service_mod(name) != 0) {
		fprintf(stderr, "Failed\n");
		exit(-1);
	}
	sleep(3);
	
	strcpy(file1, envdir);
	strcpy(file2, envdir);

	strcat(file1, "/STATE.");
	strcat(file2, "/STATE.");

	strcat(file1, primary);
	strcat(file2, secondary);
	
	if (rm_file(file1) != 0)
		fprintf(stderr, "Unable to remove file %s \n", file1);
	if (rm_file(file2) != 0)
		fprintf(stderr, "Unable to remove file %s \n", file2);
	if (rmdir(envdir) != 0) {
		perror("Error");
		return -1;
	}

	printf("Remove of service %s done\n", name);
	return 0;
}

gint
service_mod(gchar * name)
{
	halog(LOG_DEBUG, "[service_mod] enter");
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
	halog(LOG_DEBUG, "[service_add] enter");
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
	halog(LOG_DEBUG, "[rm_file] enter");
	if (unlink(name) != 0) {
		perror("Error:");
		return -1;
	}
	return 0;
}

gint
create_file(gchar * name, gchar * node)
{
	halog(LOG_DEBUG, "[create_file] enter");
	gchar *O, *env;
	struct stat buf;
	FILE *F;
	gchar to_copy[2];

	O = g_malloc0(256);
	env = g_malloc0(256);

	// When a service is created, its initial state is FROZEN_STOP
	snprintf(to_copy, 2, "%i\n", STATE_FROZEN_STOP);

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
	halog(LOG_DEBUG, "[create_dir] enter");
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
	halog(LOG_DEBUG, "[find_action] enter");
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
	halog(LOG_DEBUG, "[print_func] enter");
	gchar check_script[SCRIPT_SIZE];

	strcat(check_script, ((struct srvstruct *) (value))->check_script);
	printf("key: %s value %s\n", (gchar *) key, check_script);
}

void
service_info(GList * liste, GHashTable * HT, gchar * name, gchar * service)
{
	halog(LOG_DEBUG, "[service_info] enter");
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
	halog(LOG_DEBUG, "[service_status] enter");
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

void
service_status_cols(GList * liste, GHashTable * HT)
{
       halog(LOG_DEBUG, "[service_status_cols] enter");
       gint i, list_size, pstate, sstate;
       gpointer pointer;
       gchar *service, *primary, *secondary;

       list_size = g_list_length(liste) / LIST_NB_ITEM;
       printf("%16s %16s %14s %16s %14s\n","service","prinode","pristate","secnode","secstate");
       for (i = 0; i < list_size; i++) {
               service = g_list_nth_data(liste, i * LIST_NB_ITEM);
               pointer = g_hash_table_lookup(HT, service);
               primary = ((struct srvstruct *) (pointer))->primary;
               secondary = ((struct srvstruct *) (pointer))->secondary;
               pstate = get_status(liste, primary, service);
               sstate = get_status(liste, secondary, service);
               printf("%16s %16s %14s %16s %14s\n",service,primary,VAL[pstate],secondary,VAL[sstate]);
       }
}

gint
change_status_stop(gint state, gint ostate, gchar * service, GHashTable * HT)
{
	halog(LOG_DEBUG, "[change_status_stop] enter");
	gpointer pointer;
	gchar *arg[3];

	if (state == STATE_STARTED || state == STATE_UNKNOWN) {
		halog(LOG_NOTICE, "Ready to stop, partner node is %s we are %s",
			VAL[ostate], VAL[state]);
	} else {
		halog(LOG_NOTICE, "Cannot stop %s: service not in correct state (partner node is %s, we are %s",
			service, VAL[ostate],VAL[state]);
		return -1;
	}

	if (ostate != STATE_STOPPED) {
		halog(LOG_NOTICE, "PARTNER is not in STOPPED state: service will not migrate");
	}
	pointer = g_hash_table_lookup(HT, service);
	arg[0] = ((struct srvstruct *) (pointer))->script;
	arg[1] = "stop";
	arg[2] = (gchar *) 0;
	write_status(service, STATE_STOPPING, nodename);
	if (launch(arg[0], arg) == 0) {
		halog(LOG_INFO, "Service %s successfully stopped", service);
		write_status(service, STATE_STOPPED, nodename);
		return 0;
	} else {
		halog(LOG_ERR, "failed to stop %s", service);
		write_status(service, STATE_STOP_FAILED, nodename);
		return -1;
	}
}

gint
change_status_force_stop(gint state, gint ostate, gchar * service,
			 GHashTable * HT)
{
	halog(LOG_DEBUG, "[change_status_force_stop] enter");

	halog(LOG_INFO, "Ready to force stop, partner node is %s, we are %s",
		VAL[ostate], VAL[state]);

	write_status(service, STATE_STOPPED, nodename);
	halog(LOG_INFO, "Service %s successfully stopped", service);
	return 0;
}

gint
change_status_force_start(gint state, gint ostate, gchar * service,
			  GHashTable * HT)
{
	halog(LOG_DEBUG, "[change_status_force_start] enter");

	halog(LOG_INFO, "Ready to force start, partner node is  %s, we are %s",
		VAL[ostate], VAL[state]);

	halog(LOG_INFO, "Service %s forced to started", service);

	write_status(service, STATE_STARTED, nodename);
	return 0;
}

gint
change_status_freeze_stop(gint state, gint ostate, gchar * service,
			  GHashTable * HT)
{
	halog(LOG_DEBUG, "[change_status_freeze_stop] enter");

	if (state == STATE_STOPPED ||
	    state == STATE_STARTED ||
	    state == STATE_START_FAILED ||
	    state == STATE_STOP_FAILED) {
		halog(LOG_INFO, "Ready to FREEZE, we are %s", VAL[state]);
		if (state == STATE_STOPPED ||
		    state == STATE_START_FAILED ||
		    state == STATE_STOP_FAILED) {
			write_status(service, STATE_FROZEN_STOP, nodename);
			halog(LOG_INFO, "Service %s FROZEN-STOP", service);
			return 0;
		}
		if (state == STATE_STARTED) {
			if (change_status_stop(state, ostate, service, HT) == 0) {
				write_status(service, STATE_FROZEN_STOP, nodename);
				halog(LOG_INFO, "Service %s FROZEN-STOP", service);
				return 0;
			} else {
				halog(LOG_INFO, "error in stopping service %s", service);
				return -1;
			}
		}
	}
	halog(LOG_INFO, "Cannot FREEZE-STOP %s: service not in STARTED/STOPPED state (we are %s)",
		service, VAL[state]);
	return -1;
}

gint
change_status_freeze_start(gint state, gint ostate, gchar * service,
			   GHashTable * HT)
{
	halog(LOG_DEBUG, "[change_status_freeze_start] enter");

	if (state == STATE_STOPPED || state == STATE_STARTED) {
		printf("Ready to FREEZE, we are %s\n", VAL[state]);
		if (state == STATE_STARTED) {
			write_status(service, STATE_START_READY, nodename);
			printf("Service %s FROZEN\n", service);
			return 0;
		}
		if (state == STATE_STOPPED) {
			if (change_status_start(state, ostate, service, HT) ==
			    0) {
				write_status(service, STATE_START_READY, nodename);
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
	halog(LOG_DEBUG, "[change_status_unfreeze] enter");

	if (state == STATE_FROZEN_STOP || state == STATE_START_READY) {
		halog(LOG_INFO, "Ready to UNFREEZE, we are (%s)", VAL[state]);
		if (state == STATE_FROZEN_STOP)
			write_status(service, STATE_STOPPED, nodename);
		if (state == STATE_START_READY)
			write_status(service, STATE_STARTED, nodename);
		halog(LOG_INFO, "Service %s UNFROZEN", service);
		return 0;
	} else {
		halog(LOG_INFO, "Cannot UNFREEZE %s: not in FREEZE state (we are %s)",
			service, VAL[state]);
		return -1;
	}
}

gint
if_getaddr(const char *ifname, struct in_addr * addr)
{
	halog(LOG_DEBUG, "[if_getaddr] enter");
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
	halog(LOG_DEBUG, "[set_mcast_if] enter");
	gint rc, i = 0;
	struct in_addr mreq;

	memset(&mreq, 0, sizeof (mreq));
	rc = if_getaddr(ifname, &mreq);
	if (rc == -1) {
		//halog(LOG_ERR, "if_getaddr: failed. Bad interface name ?");
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
	halog(LOG_DEBUG, "[init_var] enter");

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
	halog(LOG_DEBUG, "[daemonize] enter");
	gint pid;

	switch (pid = fork()) {
	case 0:
		halog(LOG_INFO, "%s starting ...", message);
		break;
	case (-1):
		fprintf(stderr, "Error in fork().\n");
		halog(LOG_ERR, "%s Error in fork()", message);
		exit(-1);
	default:
		exit(0);
	}
}

int
halog(int prio, const char * fmt, ...)
{
	va_list ap;
        char buff[MAX_LOG_MSG_SIZE];

	if (prio > loglevel)
		return 0;
	va_start(ap, fmt);
        vsnprintf(buff, MAX_LOG_MSG_SIZE, fmt, ap);
	openlog(progname, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(prio, "%s", buff);
	if (isatty(0)
	    && strlen(progname) >= 7
	    && strncmp(progname, "service", 7) == 0
	    && prio < LOG_DEBUG)
		printf("%s\n", buff);
	return 0;
}

