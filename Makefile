PROG=   vhostgen
SRCS=   vhostgen.c userio.c logger.c
BINDIR= ${PREFIX}/bin
MAN=

CFLAGS+=-Wall -Wextra -I/usr/local/include/mysql  -fno-strict-aliasing -pipe -g
LDFLAGS+=-lz -lssl -lm -lcrypto  -L/usr/local/lib/mysql -lmysqlclient -lz -lcrypt -lm

.include <bsd.prog.mk>

