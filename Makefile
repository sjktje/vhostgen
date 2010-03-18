PROG=   vhostgen
SRCS=   vhostgen.c userio.c logger.c
BINDIR= ${PREFIX}/bin
MAN=

#CFLAGS+=-Wall -Wextra -I/usr/local/include/mysql  -fno-strict-aliasing -pipe -g
CFLAGS+=-I/usr/local/include/mysql -std=c89 -pedantic -Wall -W
CFLAGS+=-Wno-missing-field-initializers -Wundef -Wendif-labels -Wshadow 
CFLAGS+=-Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings 
CFLAGS+=-Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -O2

LDFLAGS+=-lz -lssl -lm -lcrypto  -L/usr/local/lib/mysql -lmysqlclient -lz -lcrypt -lm

.include <bsd.prog.mk>

