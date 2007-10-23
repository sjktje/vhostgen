/*
 * Copyright (c) 2007 Svante Kvarnström <sjk@ankeborg.nu>
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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "mysql.h"

#define CONFIG_FILE "/etc/vhostgen.conf"
#define MAX_BUFF 512

struct config_list {
	char *username;		/* Mysql username */
	char *password;		/* Mysql password */
	char *host;			/* Mysql hostname */
	char *vhosttable;	/* Mysql vhost table */
	char *db;			/* Mysql database */
	char *outfile;		/* Vhost config file to write */
};

int load_config_file(const char *, struct config_list *);
char *mkdate(void);

int
main(int argc, char *argv[])
{
	MYSQL		 sql_conn;
	MYSQL_ROW	 sqlrow;
	MYSQL_RES	*res_ptr;

	res_ptr = NULL;

	int   res;
	char *query;	/* Points at a mysql (INSERT/DELETE/SELECT/WHATEVER) query */
	int   qlen;		/* Length of query string */
	FILE *out;		/* File handler for vhost config file (/var/www/conf/vhosts.conf or so) */

	struct config_list *clist;
	clist = malloc(sizeof(struct config_list));

	/* Populate the clist struct with information taken from CONFIG_FILE */
	/* If we can't open the config file, exit. */
	if (load_config_file(CONFIG_FILE, clist)) {
		exit(1);
	}

	/* Initialize mysql database connection, select database, etc */
	mysql_init(&sql_conn);

	if (!mysql_real_connect(&sql_conn, clist->host, clist->username, clist->password, clist->db, 0, NULL, 0)) {
		if (mysql_errno(&sql_conn)) {
			fprintf(stderr, "Connection error %d: %s\n", mysql_errno(&sql_conn), mysql_error(&sql_conn));
			exit(1);
		}
	}

	/*
	 * Since mysql_query doesn't support printf's %s/%d/whatever format, we'll
	 * have to craft the exact SELECT message with snprintf first, and then send
	 * that to mysql_query. snprintf wants the length of the string, so we start
	 * of by getting that.
	 */
	qlen = strlen("SELECT * FROM ") + strlen(clist->vhosttable) + 1;

	/* Next we allocate enough memory for our sql query. */
	query = malloc(sizeof(char) * qlen);

	/*
	 * And now we put the query into "query" (or well, we put it into memory and
	 * have query direct us to it :)
	 */
	snprintf(query, qlen, "SELECT * FROM %s", clist->vhosttable);

	res = mysql_query(&sql_conn, query);

	if (res)
		fprintf(stderr, "SELECT error: %s\n", mysql_error(&sql_conn));
	else {
		res_ptr = mysql_use_result(&sql_conn);
		if (res_ptr) {
			/*
			 * Alright, we've got stuff from the database. Now we need to write it
			 * to the vhost config file. Let's see if we can open the config file for
			 * writing. If not we're screwed.
			 */
			if ((out = fopen(clist->outfile, "w")) == NULL) {
				fprintf(stderr, "ERROR: %s could not be opened for writing", clist->outfile);
				exit(1);
			}

			fprintf(out, "# This file was generated by vhostgen %s.\n"
				"# DO NOT EDIT THIS FILE MANUALLY AS ANY CHANGES WILL BE"
				" LOST. \n# Make any changes to the database instead.\n \n\n", mkdate());

			while ((sqlrow = mysql_fetch_row(res_ptr))) {
				if (sqlrow[2] && *sqlrow[2] != '\0')
					fprintf(out,"# %d - %s (%s), maintained by %s\n",
						atoi(sqlrow[0]), sqlrow[1], sqlrow[2], sqlrow[4]);
				else
					fprintf(out, "\n# %d - %s, maintained by %s\n",
						atoi(sqlrow[0]), sqlrow[1], sqlrow[4]);
				fprintf(out, "<VirtualHost *>\n\tServerName %s\n", sqlrow[1]);
				if (sqlrow[2] && *sqlrow[2] != '\0')
					fprintf(out, "\tServerAlias %s\n", sqlrow[2]);
				fprintf(out, "\tDocumentRoot %s\n", sqlrow[3]);
				fprintf(out, "</VirtualHost>\n\n");
			}
			if (mysql_errno(&sql_conn))
				fprintf(stderr, "Retrieve error: %s\n", mysql_error(&sql_conn));
		}
	}

	mysql_free_result(res_ptr);
	return(0);
}

void
chk_alloc_mem(char **dest, char *from)
{
	/* strip trailing \n */
	int length = strlen(from) - 1;
	*dest = malloc(length);
	if (*dest == NULL) {
		printf("ERROR: Couldn't allocate memory\n");
		exit(1);
	}
	strncpy(*dest, from, length);
	(*dest)[length] = '\0';
}

int
load_config_file(const char *file_name, struct config_list *clist)
{
	FILE *fp;
	char  line[MAX_BUFF];
	char  option;
	char *value;

	if ((fp = fopen(file_name, "r")) == NULL) {
		fprintf(stderr, "Couldn't open configuration file %s\n", file_name);
		return 1;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (line[0] == '#') /* Ignore comments */
			continue;

		if (strchr(line, '\n') == NULL) { /* Got huge line, ignore it. */
			fprintf(stderr, "Got huge config line. This can't be right, please "
				"fix your config.\n");
			exit(1);
		}

		if (*line == '\0' || *line == '\n')
			continue;

		if (line[1] == ':') {
			option = *line;
			value = line+2;
		} else
			fprintf(stderr, "Invalid line in config file:\n%s", line);

		switch(option) {
		case 'u':
			chk_alloc_mem(&clist->username, value);
			break;
		case 'p':
			chk_alloc_mem(&clist->password, value);
			break;
		case 'o':
			chk_alloc_mem(&clist->outfile, value);
			break;
		case 'h':
			chk_alloc_mem(&clist->host, value);
			break;
		case 'd':
			chk_alloc_mem(&clist->db, value);
			break;
		case 't':
			chk_alloc_mem(&clist->vhosttable, value);
			break;
		default:
			fprintf(stderr, "Invalid option '%c' given.", option);
			exit(1);
			break;
		}
	}

	return 0;
}

/*
 * This function returns a pointer to a malloced string containing the current
 * date, month, date, hour, minute and second in format
 * YYYY-MM-DD HH:SS UTC(+/-)HH.
 */
char
*mkdate(void)
{
	struct tm *tm_ptr;
	time_t the_time;
	char buf[256];
	char *our_date;

	(void) time(&the_time);
	tm_ptr = gmtime(&the_time);

	strftime(buf, sizeof(buf), "%y-%m-%d %H:%M UTC%z", tm_ptr);

	our_date = malloc(strlen(buf) + 1);

	if (our_date == NULL) {
		printf("ERROR: Couldn't allocate memory\n");
		exit(1);
	}

	strcpy(our_date, buf);

	return our_date;
}
