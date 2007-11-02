CC =		gcc
CFLAGS =	-g -O2  -DHAVE_CONFIG_H
LDFLAGS =	 -lz -lssl -lm -lcrypto  -L/usr/local/lib/mysql -lmysqlclient -lz -lcrypt -lm
INCLUDE =	-I/usr/local/include/mysql -fno-strict-aliasing -pipe
PREFIX =	/usr/local
BINDIR =	$(PREFIX)/bin
OBJS = vhostgen.o 

all: vhostgen

vhostgen :		$(OBJS)
				$(CC) -o $@ $(OBJS) $(LDFLAGS) 
vhostgen.o :	vhostgen.c
				$(CC) $(CFLAGS) $(INCLUDE) -c vhostgen.c
install :
				$(INSTALL) -m 0750 vhostgen $(BINDIR)/vhostgen
				$(INSTALL) -m 0750 vhostgen.conf $(PREFIX)/etc/vhostgen.conf.example
clean :
				-rm vhostgen vhostgen.o
distclean :
				-rm vhostgen vhostgen.o Makefile config.log config.h.in configure 
