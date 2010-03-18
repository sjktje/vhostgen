#ifndef __vhostgen_h
#define __vhostgen_h

#define VERSION "1.0"

/* Struct containing data gathered from ~/.vhostgenrc */
struct config_list {
	char *username;	    /* Mysql username */
    char *password;     /* Mysql password */
	char *host;	        /* Mysql hostname */
	char *vhosttable;   /* Mysql vhost table */
	char *db;	        /* Mysql database */
	char *outfile;      /* Vhost config file to write */
	char *logpath;      /* basepath for logfiles */
    char *docroot;      /* basepath for www data */
};

/* Command line options struct */
struct optlist {
    int      aflag;     /* --add */
    int      hflag;     /* --help */
    char    *username;  /* --user */
    char    *progname;  /* Name of executable */
    char    *delete;    /* %pattern% of vhost to delete */
    char    *list;      /* %pattern% of vhosts to list */
};

/* New vhost entry struct */
struct entry {
    char *servername;   /* Name of new website */
    char *serveralias;  /* Alias (www.servername) */
    char *docroot;      /* www data directory */
    char *addedby;      /* Username of user adding it */
    char *user;         /* suPHP user */
    char *group;        /* suPHP group */
    char *port;         /* Port (80) */
};

#endif /* __vhostgen_h */
