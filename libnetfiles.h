#ifndef LIBNETFILES_H
#define LIBNETFILES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>//read, write
#include <string.h>
#include <sys/types.h>//sockets,open
#include <fcntl.h>//open
#include <sys/socket.h>//sockets
#include <errno.h>//error messages
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_NUM 20001
#define READ_SIZE 512
#define WRITE_SIZE 512

int netopen(const char *, int);
ssize_t netread(int, void *, size_t);
ssize_t netwrite(int, const void *, size_t);
int netclose(int);
int netserverinit(char *, int);

#endif
