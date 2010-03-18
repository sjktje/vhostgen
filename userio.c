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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "userio.h"
#include "vhostgen.h"

/* 
 * Prints message and waits for an answer. defvalue is used if user simply
 * hits RETURN, NULL is returned if user enters a dot (``.'')
 */
char *
getinput(const char *message, const char *defvalue)
{
    char *line = NULL;
    char *p = NULL;

    printf("%s [%s]: ", message, (defvalue == NULL) ? "" : defvalue);
    fflush(stdout);

    if ((line = malloc(MAXBUF+1)) == NULL) {
        perror("getinput() could not malloc");
        exit(1);
    }


    if ((p = fgets(line, MAXBUF, stdin)) != NULL) {
        int last = strlen(line) - 1;

        if (line[0] == '\n') {   /* User just hit enter - use default value if it exists*/
            if (defvalue != NULL)
                p = strdup(defvalue);
        }

        /*
         * Inserting an unaccompanied dot means the answer should be empty
         * (just hitting RETURN would indicate using the default value.)
         */
        if ((line[0] == '.') && (line[1] == '\n'))
            return NULL;

        if (strchr(line, '\n') == NULL) {
            fprintf(stderr, "Input line would not fit in buffer. Exiting.");
            exit(1);
        }

        /* Remove trailing \n if there is one */
        if (line[last] == '\n')
            line[last] = '\0';

    }

    return p;
}

/*
 * Uses getinput() to pose a yes/no question and expects a y/n answer.
 * If def is true default answer is y, else n.
 */
int
getyesno(const char *message, int def)
{
    char *answer;
    answer = getinput(message, (def == 1) ? "y" : "n");
    if ((answer[0] == 'y' || answer[0] == 'Y') && answer[1] == '\0')
        return 1;
    else
        return 0;
}

/*
 * printf-ish perror
 */
void
perrorf(const char *fmt, ...)
{
    char buf[128];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);
    perror(buf);
}

/*
 * Print contents of an entry struct.
 */
void 
printentry(struct entry *e)
{
    int len = 0;
    int i;

    len = strlen(e->servername);

    printf("----[ %s ]----\n", e->servername);
    printf("%-15s: %s\n", "Server alias", e->serveralias);
    printf("%-15s: %s\n", "Document root", e->docroot);
    printf("%-15s: %s\n", "suPHP user", e->user);
    printf("%-15s: %s\n", "suPHP group", e->group);
    printf("%-15s: %s\n", "Port", e->port);
    printf("%-15s: %s\n", "Added by", e->addedby);
    printf("------------");
    for(i = 0; i < len; i++)
        printf("-");
    printf("\n");
}
