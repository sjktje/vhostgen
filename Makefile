CC = gcc
CFLAGS = -g -O2
LDFLAGS = -L/usr/local/lib/mysql -lmysqlclient -lz -lm -L/usr/lib -lssl -lcrypto
INCLUDE = -I/usr/local/include/mysql -pipe

all: vhostgen
vhostgen: vhostgen.o
	$(CC) $(CFLAGS) -o vhostgen vhostgen.o $(LDFLAGS)
vhostgen.o: vhostgen.c
	$(CC) $(CFLAGS) $(INCLUDE) -c vhostgen.c
clean:
	-rm vhostgen.o
distclean:
	-rm vhostgen.o vhostgen
