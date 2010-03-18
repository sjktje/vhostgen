#ifndef __userio_h
#define __userio_h

#define MAXBUF 128

#include "vhostgen.h"

char *getinput(const char *, const char *);
int   getyesno(const char *, int);
void  perrorf(const char *, ...);
void  printentry(struct entry *);

#endif
