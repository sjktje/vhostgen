#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "userio.h"

/* 
 * Prints message and waits for an answer. defvalue is used if user simply
 * hits RETURN, NULL is returned if user enters a dot (``.'')
 */
char
*getinput(const char *message, char *defvalue)
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
    char *answer = getinput(message, (def == 1) ? "y" : "n");
    if ((answer[0] == 'y' || answer[0] == 'Y') && answer[1] == '\0')
        return 1;
    else
        return 0;
}

