#CC     =       cc -pthread
CC      =       gcc
FLAGS   =       -Wall -pthread -g
COMPILE =       $(CC) $(FLAGS)

all: server client

server: netfileserver.c
	$(COMPILE) netfileserver.c -o server

client: netfileclient.c libnetfiles.c libnetfiles.h
	$(COMPILE) libnetfiles.c netfileclient.c -o client
	
clean:
	rm -rf *.o server client