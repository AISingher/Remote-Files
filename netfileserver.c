#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include "libnetfiles.h"

#define PORTNUM 20001
#define READMAX 512
#define OPERATIONFAILED strcat(response,"F|"); snprintf(errnoBuff, 4, "%d", errno); snprintf(h_errnoBuff, 4, "%d", h_errno); strcat(response, errnoBuff);strcat(response, "|");strcat(response, h_errnoBuff);

typedef struct data {
	char * filename;
	int flag;
	int fd;
	int clientfd;
	char operation;
	int numBytes;
	char * bytes;
} data;

void * threadWorker(data * clientMessage) {

	int sendBytes,fd,checkClose,readBytes,readSize;
	ssize_t checkWrite;

	errno=0;
	h_errno=0;

	char response[READMAX];
	memset(response,0,READMAX);
	response[READMAX-1] = '\0';
		
	char errnoBuff[3+1];//max errno length
	memset(errnoBuff,0,3+1);
	errnoBuff[3] = '\0';

	char h_errnoBuff[3+1];
	memset(h_errnoBuff,0,3+1);
	h_errnoBuff[3] = '\0';

		switch (clientMessage->operation) {
			case '0': //open
				fd = open(clientMessage->filename,clientMessage->flag);
				free(clientMessage->filename);
				if (fd < 0) {
					OPERATIONFAILED; 
				} 
				else {
					//successfully opened
					snprintf(response, snprintf(NULL, 0, "%d", fd) + 2 + 1, "S|%d", fd); //file descriptor + 'S,' + null term
				}
				sendBytes = write(clientMessage->clientfd,response,strlen(response)+1);
				break;
			case '1': //close
				checkClose = close(clientMessage->fd);

				if (checkClose < 0) {
					OPERATIONFAILED;
				} 
				else {
					//successfully closed
					strcat(response,"S");
				}
				sendBytes = write(clientMessage->clientfd,response,strlen(response)+1);
				break;
			case '2': //read
				readBytes = snprintf(NULL, 0, "%d", clientMessage->numBytes);
				readSize = clientMessage->numBytes;

				char * readBuffer = malloc(readSize+1);//nbytes + S + 2 commas + buff + nullterm
				memset(readBuffer,0,readSize+1);
				readBuffer[readSize] = '\0';

				char * returnBuff = malloc(readSize +2+readBytes+1 +1);
				memset(returnBuff,0,readSize +2+readBytes+1 +1);
				returnBuff[readSize +2+readBytes+1] = '\0';

				ssize_t checkRead = read(clientMessage->fd,readBuffer,clientMessage->numBytes);
				printf("checkRead=%d\n",(int)checkRead);
				if (checkRead < 0) {
					OPERATIONFAILED;
					sendBytes = write(clientMessage->clientfd,response,strlen(response)+1);
				} 
				else {
					//successfully read
					strcat(returnBuff,"S|");
					char readBytesBuff[readBytes];
					snprintf(readBytesBuff, readBytes+1, "%d", (int)checkRead);
					strcat(returnBuff,readBytesBuff);
					strcat(returnBuff,"|");
					strcat(returnBuff,readBuffer);
					
					sendBytes = write(clientMessage->clientfd,returnBuff,clientMessage->numBytes+1+2+readBytes+1); //bytes + S + 2 commas + bytestoread + nullterm
				}
				free(readBuffer);
				free(returnBuff);
				break;
			case '3': //write
				checkWrite = write(clientMessage->fd,clientMessage->bytes,clientMessage->numBytes);
				printf("checkWrite=%d\n",(int)checkWrite);
				if (checkWrite < 0) {
					OPERATIONFAILED;
				} 
				else {
					//successfully wrote
					strcat(response, "S|");
					int writeBytes = snprintf(NULL, 0, "%d", (int)checkWrite);
					char writeBytesBuff[writeBytes];
					snprintf(writeBytesBuff, writeBytes+1, "%d", (int)checkWrite);
					strcat(response,writeBytesBuff);
				}

				sendBytes = write(clientMessage->clientfd,response,strlen(response)+1);
				free(clientMessage->bytes);
				break;
			default:
			//no valid instruction
			break;
		}

		if (sendBytes < 0) {
			perror("Error writing to socket");
			close(clientMessage->clientfd);
			free(clientMessage);
			exit(EXIT_FAILURE);
		}

		close(clientMessage->clientfd); //close accept file descriptor when finished with incoming connection
		free(clientMessage); //free passed in data
		return NULL;
}

int main(int argc, char * argv[]){
	
	int socketfd, acceptfd, bindReturn, listenReturn, clientAddrLen, returnBytes;
	struct sockaddr_in serverAddr, clientAddr;
	char buff[READMAX];
	char * final;

	socketfd = socket(AF_INET, SOCK_STREAM, 0); 

	if (socketfd < 0) {
		perror("Error opening socket");
		exit(EXIT_FAILURE);
	}

	memset(&serverAddr, 0, sizeof(serverAddr)); //zero out ServerAddr 

	serverAddr.sin_family = AF_INET; //IPV4 
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //get its own ip for binding
	serverAddr.sin_port = htons(PORTNUM); //listen to port 

	bindReturn = bind(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); //bind to port
	
	if (bindReturn < 0) {
		perror("Error binding");
		exit(EXIT_FAILURE);
	}

	listenReturn = listen(socketfd, 10); 

	if (listenReturn < 0) {
		perror("Error listening");
		exit(EXIT_FAILURE);
	}

	memset(&clientAddr, 0, sizeof(clientAddr)); //Zero out clientAddr
	clientAddrLen = sizeof(clientAddr);

	char server_name[100];
	server_name[99] = '\0';
	gethostname(server_name, 99);
	printf("Server running on %s:%d\n",server_name,PORTNUM);
	
	while (1) {
		acceptfd = accept(socketfd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientAddrLen);

		if (acceptfd < 0) {
			perror("Error on accept");
			pthread_exit(NULL);
		}

	//	printf("Accepting connection...\n");

		memset(buff,0,READMAX); //Zero out buff
		buff[READMAX-1] = '\0';

		returnBytes = read(acceptfd,buff,READMAX-1); //get bytes from client
		
		if (returnBytes < 0) {
			perror("Error reading from socket");
			close(acceptfd);
			pthread_exit(NULL);
		}

		printf("returnBytes=%d,buff=%s\n",returnBytes, buff);

		
		data * serverMessage = malloc(sizeof(data));
		serverMessage->operation = buff[0];
		serverMessage->clientfd = acceptfd;

		switch (serverMessage->operation) {
			case '0': //open
				serverMessage->filename = malloc(strlen(&buff[4])+1);
				strcpy(serverMessage->filename, &buff[4]);
				serverMessage->flag = atoi(&buff[2]);
				break;
			case '1': //close
				serverMessage->fd = atoi(&buff[2]);
				break;
			case '2': //read
				final = strtok(buff, "|");
	    		
	    		final = strtok(NULL, "|");
	    		serverMessage->fd = atoi(final);
	    		
	    		final = strtok(NULL, "|");
				serverMessage->numBytes = atoi(final);
				break;
			case '3': //write
				final = strtok(buff, "|");
	    		
	    		final = strtok(NULL, "|");
	    		serverMessage->fd = atoi(final);
	    		
	    		final = strtok(NULL, "|");
	    		serverMessage->bytes = malloc(strlen(final)+1);
				serverMessage->numBytes = atoi(final);

				final = strtok(NULL, "|");
				memset(serverMessage->bytes,0,serverMessage->numBytes);
				serverMessage->bytes[serverMessage->numBytes+1] = '\0';
				memcpy(serverMessage->bytes,final,serverMessage->numBytes+1);
				break;
			default:
			//no valid instruction
				break;
		}

		pthread_t threadID;
		pthread_create(&threadID, NULL, (void * (*)(void *))threadWorker, serverMessage); 

		pthread_detach(threadID);	

	}
	pthread_exit(NULL);
	return 0;
}
