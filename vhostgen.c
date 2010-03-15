/*
 * Copyright (c) 2010 Svante J. Kvarnstrom <sjk@ankeborg.nu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <getopt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "mysql.h"

#include "userio.h"


static char             *mkdate(void);
static char             *mres(MYSQL, char *);
static char             *myhomedir(void);
static char             *myuser(void);
static char             *vg_asprintf(const char *, ...);
static char             *vg_strdup(char *);
static int               addvhost(MYSQL, struct config_list *);
static int               generate_vhosts_conf(MYSQL, struct config_list *);
static int               is_emptystring(char *);
static int               load_config_file(struct config_list *);
static struct entry     *get_vhost_info(struct entry *, struct config_list *); 
static struct entry     *mres_entry(MYSQL, struct entry *);
static struct optlist   *optlistinit(void);
static struct optlist   *parseargs(int *, char ***);
static void              usage(char *);
static void              free_entry(struct entry *); 
void                     printentry(struct entry *);
static int               listvhosts(MYSQL, struct config_list *, char *);


int 
main(int argc, char *argv[]) 
{
	MYSQL		         sql_conn;
    struct optlist      *cmdargs;
    struct config_list  *clist;

    cmdargs = parseargs(&argc, &argv);

    if (cmdargs->hflag) 
        usage(cmdargs->progname);

	if ((clist = malloc(sizeof(struct config_list))) == NULL) {
        perror("clist");
        exit(1);
    }

    if (load_config_file(clist)) 
        exit(1);

    /*
     * User supplied username using -u switch. Make sure to prompt them for a 
     * password; don't use any passwords from ~/.vhostgenrc.
     */
    if (!is_emptystring(cmdargs->username)) {
        free(clist->password);
        clist->password = NULL;
    }

    if (is_emptystring(clist->password)) 
        clist->password = getpass("Your MySQL password (will not echo): ");

    /*
     * We might as well go ahead and initialize the database connection 
     * as we will need it for all operations. 
     */
	mysql_init(&sql_conn);

	if (!mysql_real_connect(&sql_conn, clist->host, clist->username, 
                clist->password, clist->db, 0, NULL, 0)) {
		if (mysql_errno(&sql_conn)) {
			fprintf(stderr, "Connection error %d: %s\n", 
                    mysql_errno(&sql_conn), mysql_error(&sql_conn));
			exit(1);
		}
	}

    if (cmdargs->aflag) {
        addvhost(sql_conn, clist);
        mysql_close(&sql_conn);
        exit(1);
    }

    /*
    if (!is_emptystring(cmdargs->list)) {
        listvhosts(sql_conn, clist, cmdargs->list);
        return 0;
    }
    */

    if (generate_vhosts_conf(sql_conn, clist)) {
        mysql_close(&sql_conn);
        exit(1);
    }

    mysql_close(&sql_conn);

    return 0;
}

static void 
usage(char *progname) 
{
    fprintf(stderr, "USAGE: %s --add\n"
                    "       %s --help\n", progname, progname);
    exit(0);
}

    
static int addvhost(MYSQL sql_conn, struct config_list *clist)
{
    struct entry *newentry = { 0 };
    struct entry *escapedentry = { 0 };
    char *query = NULL;
    char *table = NULL;
    int res;

    newentry = get_vhost_info(newentry, clist);
    escapedentry = mres_entry(sql_conn, newentry);
    
    printentry(newentry);

    if (getyesno("Does the above look reasonable?", 1)) 
        printf("Adding vhost...\n");
    else {
        printf("Aborting...\n");
        exit(1);
    }

    free(newentry);

    table = mres(sql_conn, clist->vhosttable);
    
    query = vg_asprintf("INSERT INTO %s (`servername`, `serveralias`, "
            "`documentroot`, `addedby`, `user`, `group`,`port`) "
            "VALUES ('%s','%s','%s','%s','%s','%s','%s')", 
            table, escapedentry->servername, escapedentry->serveralias, 
            escapedentry->docroot, escapedentry->addedby, escapedentry->user, escapedentry->group,
            escapedentry->port);

    res = mysql_query(&sql_conn, query);
    free_entry(escapedentry);
    free(query);
    free(table);

    if (res) {
        fprintf(stderr, "Insert error %d: %s\n", 
                mysql_errno(&sql_conn), mysql_error(&sql_conn));
        return 1;
    }

    return 0;
}

/* 
 * Takes entry struct and returns another entry struct where all struct members
 * have been run through mysql_real_escape_string. Returned struct should be 
 * freed with free_entry();
 */
static struct entry *
mres_entry(MYSQL sql_conn, struct entry *newentry)
{
    struct entry *escapedentry = { 0 };
    if ((escapedentry = malloc(sizeof(struct entry))) == NULL) {
        perror("mres_entry malloc");
        exit(1);
    }

    escapedentry->servername    = mres(sql_conn, newentry->servername);
    escapedentry->serveralias   = mres(sql_conn, newentry->serveralias);
    escapedentry->docroot       = mres(sql_conn, newentry->docroot);
    escapedentry->addedby       = mres(sql_conn, newentry->addedby);
    escapedentry->user          = mres(sql_conn, newentry->user);
    escapedentry->group         = mres(sql_conn, newentry->group);
    escapedentry->port          = mres(sql_conn, newentry->port);

    return escapedentry;
}

/* 
 * Frees malloced entry struct.
 */
static void free_entry(struct entry *e) {
    free(e->servername);
    free(e->servername);
    free(e->docroot);
    free(e->addedby);
    free(e->user);
    free(e->group);
    free(e->port);
    free(e);
}

/*
 * Returns malloced string safe for database insertion. Must be freed.
 */
static char *
mres(MYSQL sql_conn, char *string)
{
    char *escaped = NULL;
    int len = 0;

    if (is_emptystring(string))
        return NULL;

    len = strlen(string);
    if ((escaped = malloc(len * 2 + 1)) == NULL) {
        perror("mres malloc");
        exit(1);
    }

    mysql_real_escape_string(&sql_conn, escaped, string, len);
    return escaped;
}

static struct entry *
get_vhost_info(struct entry *newentry, struct config_list *clist) 
{   
    char *def = NULL;

    if ((newentry = malloc(sizeof(*newentry))) == NULL) {
        perror("malloc");
        exit(1);
    }

    /* Get server name */
    newentry->servername = getinput("Server name", NULL);

    /* Get server alias */
    if (strcasestr(newentry->servername, "www") == NULL) 
        def = vg_asprintf("www.%s", newentry->servername);
    newentry->serveralias = getinput("Server alias", def);
    free(def);
    
    /* Get www data dir */
    def = vg_asprintf("%s/%s/htdocs", clist->docroot, newentry->servername);
    newentry->docroot = getinput("Document root", def);
    free(def);

    /* Get suphp user */
    newentry->user = getinput("suPHP user", NULL);

    /* Get suphp group */
    def = vg_strdup(newentry->user);
    newentry->group = getinput("suPHP group", def);
    free(def);

    /* Get port */
    newentry->port = getinput("Port", "80");

    newentry->addedby = myuser();

    return newentry;
}


static char
*myuser(void)
{
    struct passwd *passwd;
    passwd = getpwuid(getuid());
    return passwd->pw_name;
}

static char
*myhomedir(void)
{
    struct passwd *passwd;
    passwd = getpwuid(getuid());
    return passwd->pw_dir;
}

static int listvhosts(MYSQL sql_conn, struct config_list *clist, char *pattern) {
    MYSQL_ROW    sqlrow;
    MYSQL_RES   *res_ptr = NULL;
    int          res;
    char        *escaped = NULL;
    char        *query = NULL;
    
    escaped = mres(sql_conn, pattern);
    query = vg_asprintf("SELECT `id`,`servername`,`serveralias`,`documentroot`,"
            "`addedby`,`user`,`group`,`port` FROM %s WHERE `servername` LIKE '%s'",
            clist->vhosttable, escaped);
    free(escaped);
    res = mysql_query(&sql_conn, query);
    free(query);
    query = NULL;

    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&sql_conn));
        return 1;
    }

    if ((res_ptr = mysql_use_result(&sql_conn)) == NULL) {
        if (mysql_errno(&sql_conn)) {
            fprintf(stderr, "MySQL error %d: %s\n", 
                    mysql_errno(&sql_conn), mysql_error(&sql_conn));
            return 1;
        }
    }

    printf("-----[ %s ]-----\n", pattern);
    while ((sqlrow = mysql_fetch_row(res_ptr))) 
        printf("%s: %s [%s]\n", sqlrow[0], sqlrow[1], sqlrow[4]);

        
    mysql_free_result(res_ptr);
    return 0;
}
        

/*
 * Connect to database, grab information about vhosts and generate vhosts.conf
 * which can later be included into Apache configuration.
 */
static int
generate_vhosts_conf(MYSQL sql_conn, struct config_list *clist)
{
	MYSQL_ROW	         sqlrow;
	MYSQL_RES	        *res_ptr = NULL;
    int                  res;
    char                *query = NULL;
    FILE                *out;
    
    if ((asprintf(&query, "SELECT `id`,`servername`,`serveralias`,`documentroot`,"
        "`addedby`,`user`,`group`,`port` FROM %s", clist->vhosttable)) == -1) {
        perror("asprintf");
        mysql_close(&sql_conn);
        exit(1);
    }

    res = mysql_query(&sql_conn, query);
    free(query);
    query = NULL;

    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&sql_conn));
        return 1;
    }

    if ((res_ptr = mysql_use_result(&sql_conn)) == NULL) {
        if (mysql_errno(&sql_conn)) {
            fprintf(stderr, "MySQL error %d: %s\n", 
                    mysql_errno(&sql_conn), mysql_error(&sql_conn));
            return 1;
        }
    }

    if ((out = fopen(clist->outfile, "w")) == NULL) {
        perrorf("Could not fopen %s", clist->outfile);
        return 1;
    }
        
    fprintf(out, "# This file was generated by vhostgen %s.\n"
        "# DO NOT EDIT THIS FILE MANUALLY AS ANY CHANGES WILL BE"
        " LOST. \n# Make any changes to the database instead.\n \n\n", mkdate());

    while ((sqlrow = mysql_fetch_row(res_ptr))) {
        if (!is_emptystring(sqlrow[2]))
            fprintf(out,"# %d - %s (%s), added by %s\n",
                atoi(sqlrow[0]), sqlrow[1], sqlrow[2], sqlrow[4]);
        else
            fprintf(out, "\n# %d - %s, added by %s\n",
                atoi(sqlrow[0]), sqlrow[1], sqlrow[4]);
        fprintf(out, "<VirtualHost *:%s>\n\tServerName %s\n", sqlrow[7], sqlrow[1]);
        if (!is_emptystring(sqlrow[2]))
            fprintf(out, "\tServerAlias %s\n", sqlrow[2]);
        fprintf(out, "\tDocumentRoot %s\n", sqlrow[3]);
        fprintf(out, "\tCustomLog %s%s.log combined\n", clist->logpath, sqlrow[1]);
        fprintf(out, "\tErrorLog %s%s.error.log\n", clist->logpath, sqlrow[1]);
        
        if (!is_emptystring(sqlrow[5]) && !is_emptystring(sqlrow[6]))
            fprintf(out, "\tsuPHP_UserGroup %s %s\n", sqlrow[5], sqlrow[6]);
        
        fprintf(out, "</VirtualHost>\n\n");
    }

    fclose(out);
    mysql_free_result(res_ptr);
    return 0;
}

/* 
 * Returns true if string is empty, false otherwise.
 */
static int is_emptystring(char *string) {
    if ((string == NULL) || (*string == '\0'))
        return 1;
    else
        return 0;
}

/* 
 * Reads ~/.vhostgenrc and populates clist. 
 */
static int
load_config_file(struct config_list *clist)
{
	FILE *fp;
	char  line[MAXBUF];
	char  option;
	char *value;
    int len;
    int error = 0;
    char *config_file = NULL;

    config_file = vg_asprintf("%s/.vhostgenrc", myhomedir());

	if ((fp = fopen(config_file, "r")) == NULL) {
        perrorf("Could not fopen %s", config_file);
		return 1;
    }

	while (fgets(line, sizeof(line), fp)) {
		if (line[0] == '#') /* Ignore comments */
			continue;

		if (*line == '\0' || *line == '\n')
			continue;

		if (strchr(line, '\n') == NULL) { 
			fprintf(stderr, "Got huge config line. This can't be right, please "
				"fix your config.\n");
			exit(1);
		}

        len = strlen(line);

        if (line[len-1] == '\n')
            line[len-1] = '\0';

		if (line[1] == ':') {
			option = *line;
			value = line+2;
		} else
			fprintf(stderr, "Invalid line in config file:\n%s", line);

		switch(option) {
		case 'u':
		    clist->username = vg_strdup(value);
			break;
        case 'p':
            clist->password = vg_strdup(value);
            break;
		case 'o':
            clist->outfile = vg_strdup(value);
			break;
		case 'h':
            clist->host = vg_strdup(value);
			break;
		case 'd':
            clist->db = vg_strdup(value);
			break;
		case 't':
            clist->vhosttable = vg_strdup(value);
			break;
		case 'l':
            clist->logpath = vg_strdup(value);
			break;
        case 'r':
            clist->docroot = vg_strdup(value);
            break;
		default:
			fprintf(stderr, "Invalid option '%c' given.\n", option);
			exit(1);
			break;
		}
	}

    if (is_emptystring(clist->username)) {
        fprintf(stderr, "Username (u:) setting missing in config file.\n");
        error = 1;
    }

    if (is_emptystring(clist->outfile)) {
        fprintf(stderr, "Outfile (o:) missing in config file.\n");
        error = 1;
    }

    if (is_emptystring(clist->host)) {
        fprintf(stderr, "Host (h:) missing in config file.\n");
        error = 1;
    }

    if (is_emptystring(clist->db)) {
        fprintf(stderr, "Database (d:) missing in config file.\n");
        error = 1;
    }
    
    if (is_emptystring(clist->vhosttable)) {
        fprintf(stderr, "Vhost table (t:) missing in config file.\n");
        error = 1;
    }

    if (is_emptystring(clist->logpath)) {
        fprintf(stderr, "Log path (l:) missing in config file.\n");
        error = 1;
    }

    if (is_emptystring(clist->docroot)) {
        fprintf(stderr, "Document root (r:) missing in config file.\n");
        error = 1;
    }

	return error;
}

static char *
vg_strdup(char *src)
{
    char *dst;
    if ((dst = strdup(src)) == NULL) {
        perror("vg_strdup");
        exit(1);
    }
    return dst;
}

static char *
vg_asprintf(const char *fmt, ...) 
{
    va_list va;
    char *dst = NULL;
    va_start(va, fmt);
    vasprintf(&dst, fmt, va);
    if (dst == NULL) {
        perror("vasprintf");
        exit(1);
    }
    va_end(va);
    return dst;
}


static char
*mkdate(void)
{
	struct tm *tm_ptr;
	time_t the_time;
	char buf[256];
	char *our_date;

	(void) time(&the_time);
	tm_ptr = gmtime(&the_time);

	strftime(buf, sizeof(buf), "%y-%m-%d %H:%M UTC%z", tm_ptr);

	if ((our_date = malloc(strlen(buf) + 1)) == NULL) {
        perror("mkdate");
        exit(1);
    }

	strcpy(our_date, buf);

	return our_date;
}

struct optlist *
parseargs(int *argc, char ***argv)
{
    int             ch;
    struct optlist *cmdargs;

    cmdargs = optlistinit();
    cmdargs->progname = *argv[0];

    static struct option options[] = {
        { "add",    no_argument,        NULL,   'a' },
        { "help",   no_argument,        NULL,   'h' },
        { "user",   required_argument,  NULL,   'u' },
        { "list",   optional_argument,  NULL,   'l' },
    };

    while ((ch = getopt_long(*argc, *argv, "ahu:l:", options, NULL)) != -1) {
        switch (ch) {
        case 'a':
            cmdargs->aflag = 1;
            break;
        case 'h':
            cmdargs->hflag = 1;
            break;
        case 'u':
            cmdargs->username = vg_strdup(optarg);
            break;
        case 'l':
            cmdargs->list = vg_strdup(optarg);
        default:
            usage(*argv[0]);
            break; /* NOT REACHED */
        }
    }

    *argc -= optind;
    *argv += optind;

    return cmdargs;
}

static struct optlist *
optlistinit(void)
{
    struct optlist *cmdargs;
    if ((cmdargs = malloc(sizeof(struct optlist))) == NULL) {
        perror("optlistinit");
        exit(1);
    }

    cmdargs->aflag = 0;
    cmdargs->hflag = 0;
    cmdargs->progname = NULL;
    cmdargs->username = NULL;
    cmdargs->list = NULL;
    return cmdargs;
}
