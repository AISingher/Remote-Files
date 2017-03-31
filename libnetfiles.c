#include "libnetfiles.h"
#define SERVERERRORS

int can_connect = 0;
struct sockaddr * server_addr;
int addr_len;

/**
    Used to open a file on the server from the client.

    file: 		The file to open on the server
   	flags: 		O_RDONLY   O_WRONLY   O_RDWR
   
    returns: 	The file descriptor, or -1 if an error occurred
*/
int netopen(const char * file, int flag){
	int buff_size = strlen(file)+1+2+2;//filename + nullterm + 'operation,' + 'flag,'
	char buff[buff_size];
	int receive_data_size = 32;
	char received_data[receive_data_size];
	int file_desc = -1, sendBytes, returnBytes, connect_resp, socket_fd;
	char status = 'F'; //fail by default

	if (can_connect == 0) {
		fprintf(stderr,"Error: netserverinit() must run successfully before netopen() can run.\n");
		return -1;
	}

	//Build message with format: "OPERATION|FLAG|FILENAME"
	memset(&buff,0,buff_size);
	strcat(buff,"0|"); //0 = open
	char flag_buff[3];
	snprintf(flag_buff, 3, "%d|", flag);
	strcat(buff,flag_buff);
	strcat(buff,file);
	buff[buff_size-1] = '\0';

	//Get ready to error check
	errno = 0;
	h_errno = 0;

	//Create socket and connect
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {
		perror("Error opening socket with netopen()");
		exit(EXIT_FAILURE);
	}

	connect_resp = connect(socket_fd,server_addr,addr_len);

	if (connect_resp < 0) {
		perror("netopen() failed to connect");
		exit(EXIT_FAILURE);
	}

	//Attempt to send message to server
	sendBytes = write(socket_fd,buff,strlen(buff)+1);

	//Data successfully sent
	if (sendBytes >= 0) {

		memset(received_data,0,receive_data_size);
		received_data[receive_data_size-1] = '\0';

		returnBytes = read(socket_fd,received_data,receive_data_size-1);
		printf("response= %s \n",received_data);
		status = received_data[0];

		//Data successfully received
	    if (returnBytes >= 0) {

	    	//Receive message with format: "S,FILE_DESCRIPTOR" or "F,ERRNO,H_ERRNO"
	    	//STATUS: F = fail, S = success

	    	if (status == 'S') {

	    		file_desc = atoi(&received_data[2]);

	    	} 
	    	else if (status == 'F') {

	    		char * final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		errno = atoi(final);
	    		
	    		final = strtok(NULL, "|");
				h_errno = atoi(final);
	    		
	    		//printf("errno=%d,h_errno=%d\n",errno,h_errno);
	    	} 
	    	else {

	    		fprintf(stderr, "Error: netopen() invalid response from server.\n");
	    	
	    	}

	    //Data not received
	    } 
	    else {

	    	fprintf(stderr, "Error: netopen() failed to receive data from server.\n");
	    
	    }
	//Data failed to send
	} 
	else {

		fprintf(stderr, "Error: netopen() failed to send data to the server.\n");
	
	}       

	//Close socket when done
	close(socket_fd);

	if (file_desc > -1){
		
		return file_desc;

	} 
	else {
		//fails, return -1
		perror("netopen() failed");
		return -1;

	}
}

/**
    Used to read from a file on the server.

    file_desc: 	The server file descriptor of the file to read
   	buff: 		Buffer to store read data
   	nbyte: 		Number of bytes to read

    returns: 	A non-negative integer indicating the number of bytes actually read, or -1 if an error occurred
*/
ssize_t netread(int file_desc, void * output, size_t nbyte){
	
	int file_desc_length = snprintf(NULL, 0, "%d", file_desc);
	int nbyte_length = snprintf(NULL, 0, "%d", (int)nbyte);
	int buff_size = file_desc_length + 1 + 2 + 1 + nbyte_length;// file_desc + operation + commas + nullterm + numbytes
	char buff[buff_size];
	int receive_data_size = nbyte + nbyte_length + 1 + 1 + 2; //bytes + bytesread + null term + s or f + 2 commas
	char received_data[receive_data_size];
	int sendBytes, returnBytes, connect_resp, socket_fd,bytes_read_from_server;
	char status = 'F'; //fail by default
	char * final;

	if (can_connect == 0) {
		fprintf(stderr,"Error: netserverinit() must run successfully before netread() can run.\n");
		return -1;
	}

	//zero output
	memset(output,0,nbyte);


	memset(&buff,0,buff_size);
	strcat(buff,"2|"); //2 = read
	char file_desc_buff[file_desc_length+1];
	snprintf(file_desc_buff, file_desc_length+1, "%d", file_desc);
	strcat(buff,file_desc_buff);
	strcat(buff,"|");
	char nbyte_buff[nbyte_length+1];
	snprintf(nbyte_buff, nbyte_length+1, "%d", (int)nbyte);
	strcat(buff,nbyte_buff);
	buff[buff_size-1] = '\0';

	//Get ready to error check
	errno = 0;
	h_errno = 0;

	//Create socket and connect
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {
		perror("Error opening socket with netread()");
		exit(EXIT_FAILURE);
	}

	connect_resp = connect(socket_fd,server_addr,addr_len);

	if (connect_resp < 0) {
		perror("netread() failed to connect");
		exit(EXIT_FAILURE);
	}

	//Attempt to send message to server
	sendBytes = write(socket_fd,buff,strlen(buff)+1);

	//Data successfully sent
	if (sendBytes >= 0) {

		memset(received_data,0,receive_data_size);
		received_data[receive_data_size-1] = '\0';

		returnBytes = read(socket_fd,received_data,receive_data_size-1);
		printf("response= %s \n",received_data);
		status = received_data[0];

		//Data successfully received
	    if (returnBytes >= 0) {
	    	//Receive message with format: "S,NUM_BYTES,CONTENT" or "F,ERRNO,H_ERRNO"
	    	//STATUS: F = fail, S = success

	    	if (status == 'S') {

	    		final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		bytes_read_from_server = atoi(final);
	    		
	    		final = strtok(NULL, "|");

	    		if (bytes_read_from_server > nbyte) {

	    			fprintf(stderr,"Error: netread() more bytes read than requested.\n");

	    		} else {
	    			
	    			memcpy(output, final, bytes_read_from_server);

	    		}

	    	} else if (status == 'F') {

	    		final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		errno = atoi(final);
	    		
	    		final = strtok(NULL, "|");
				h_errno = atoi(final);
	    		
	    		//printf("errno=%d,h_errno=%d\n",errno,h_errno);
	    	} else {

	    		fprintf(stderr, "Error: netread() invalid response from server.\n");
	    	
	    	}

	    //Data not received
	    } else {

	    	fprintf(stderr, "Error: netread() failed to receive data from server.\n");
	    
	    }
	//Data failed to send
	} else {

		fprintf(stderr, "Error: netread() failed to send data to the server.\n");
	
	}       

	//Close socket when done
	close(socket_fd);

	if (status == 'S'){
		
		return bytes_read_from_server;

	} else {
		//fails, set errno and return -1
		perror("netread() failed");
		return -1;	

	}
}

/**
    Used to write to a file on the server.

    file_desc: 	The server file descriptor of the file to write to
   	buff: 		Buffer to write from
   	nbyte: 		Number of bytes to write

    returns: 	A non-negative integer indicating the number of bytes actually written, or -1 if an error occurred
*/
ssize_t netwrite(int file_desc, const void * input, size_t nbyte){
	
	int file_desc_length = snprintf(NULL, 0, "%d", file_desc);
	int nbyte_length = snprintf(NULL, 0, "%d", (int)nbyte);
	int buff_size = file_desc_length + 1 + 3 + 1 + nbyte_length + nbyte;// file_desc + operation + 3 commas + nullterm + numbytes + numbyte_len
	char buff[buff_size];
	int receive_data_size = nbyte_length + 1 + 1 + 2; //bytes_length + null term + s or f + 2 commas
	char received_data[receive_data_size];
	int sendBytes, returnBytes, connect_resp, socket_fd,bytes_written_to_server;
	char status = 'F'; //fail by default
	char * final;

	if (can_connect == 0) {
		fprintf(stderr,"Error: netserverinit() must run successfully before netwrite() can run.\n");
		return -1;
	}

	//Build message with format: "OPERATION,FILE_DESC,NUMBER_OF_BYTES,CONTENT"
	memset(&buff,0,buff_size);
	strcat(buff,"3|"); //3 = write
	char file_desc_buff[file_desc_length+1];
	snprintf(file_desc_buff, file_desc_length+1, "%d", file_desc);
	strcat(buff,file_desc_buff);
	strcat(buff,"|");
	char nbyte_buff[nbyte_length+1];
	snprintf(nbyte_buff, nbyte_length+1, "%d", (int)nbyte);
	strcat(buff,nbyte_buff);
	strcat(buff,"|");
	char temp[nbyte+1];
	temp[nbyte] = '\0';
	memcpy(temp,input,nbyte);
	strcat(buff,temp);
	buff[buff_size-1] = '\0';

	//Get ready to error check
	errno = 0;
	h_errno = 0;

	//Create socket and connect
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {
		perror("Error opening socket with netwrite()");
		exit(EXIT_FAILURE);
	}

	connect_resp = connect(socket_fd,server_addr,addr_len);

	if (connect_resp < 0) {
		perror("netwrite() failed to connect");
		exit(EXIT_FAILURE);
	}

	//Attempt to send message to server
	sendBytes = write(socket_fd,buff,strlen(buff)+1);

	//Data successfully sent
	if (sendBytes >= 0) {

		memset(received_data,0,receive_data_size);
		received_data[receive_data_size-1] = '\0';

		returnBytes = read(socket_fd,received_data,receive_data_size-1);
		printf("response= %s \n",received_data);
		status = received_data[0];

		//Data successfully received
	    if (returnBytes >= 0) {
	    	//Receive message with format: "S,NUM_BYTES_WRITTEN" or "F,ERRNO,H_ERRNO"
	    	//STATUS: F = fail, S = success

	    	if (status == 'S') {

	    		final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		bytes_written_to_server = atoi(final);
	    		

	    		if (bytes_written_to_server > nbyte) {

	    			fprintf(stderr,"Error: netwrite() more bytes read than requested.\n");

	    		}

	    	} else if (status == 'F') {

	    		final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		errno = atoi(final);
	    		
	    		final = strtok(NULL, "|");
				h_errno = atoi(final);
	    		
	    		//printf("errno=%d,h_errno=%d\n",errno,h_errno);
	    	} else {

	    		fprintf(stderr, "Error: netwrite() invalid response from server.\n");
	    	
	    	}

	    //Data not received
	    } else {

	    	fprintf(stderr, "Error: netwrite() failed to receive data from server.\n");
	    
	    }
	//Data failed to send
	} else {

		fprintf(stderr, "Error: netwrite() failed to send data to the server.\n");
	
	}       

	//Close socket when done
	close(socket_fd);

	if (status == 'S'){
		
		return bytes_written_to_server;

	} else {
		//fails, set errno and return -1
		perror("netwrite() failed");
		return -1;	

	}
}

/**
    Used to close a file on the server.

    file_desc: 	The server file descriptor of the file to close

    returns: 	0 if successful, or -1 if an error occurred
*/
int netclose(int file_desc) {
	int file_desc_length = snprintf(NULL, 0, "%d", file_desc);
	int buff_size = 1 + 2 + file_desc_length;//nullterm + 'operation,' + file_descriptor
	char buff[buff_size];
	int receive_data_size = 32;
	char received_data[receive_data_size];
	int sendBytes, returnBytes, connect_resp, socket_fd;
	char status = 'F'; //fail by default

	if (can_connect == 0) {
		fprintf(stderr,"Error: netserverinit() must run successfully before netclose() can run.\n");
		return -1;
	}

	//Build message with format: "OPERATION,FLAG,FILENAME"
	memset(&buff,0,buff_size);
	strcat(buff,"1|"); //1 = close
	char file_desc_buff[file_desc_length+1];
	snprintf(file_desc_buff, file_desc_length+1, "%d,", file_desc);
	strcat(buff,file_desc_buff);
	buff[buff_size-1] = '\0';

	//Get ready to error check
	errno = 0;
	h_errno = 0;
	
	//Create socket and connect
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {
		perror("Error opening socket with netclose()");
		exit(EXIT_FAILURE);
	}

	connect_resp = connect(socket_fd,server_addr,addr_len);

	if (connect_resp < 0) {
		perror("netopen() failed to connect");
		exit(EXIT_FAILURE);
	}

	//Attempt to send message to server
	sendBytes = write(socket_fd,buff,strlen(buff)+1);

	//Data successfully sent
	if (sendBytes >= 0) {

		memset(received_data,0,receive_data_size);
		received_data[receive_data_size-1] = '\0';

		returnBytes = read(socket_fd,received_data,receive_data_size-1);
		printf("response= %s \n",received_data);
		status = received_data[0];

		//Data successfully received
	    if (returnBytes >= 0) {

	    	//Receive message with format: "S" or "F,ERRNO,H_ERRNO"
	    	//STATUS: F = fail, S = success

	    	if (status == 'S') {

	    		file_desc = atoi(&received_data[2]);

	    	} else if (status == 'F') {

	    		char * final = strtok(received_data, "|");
	    		
	    		final = strtok(NULL, "|");
	    		errno = atoi(final);
	    		
	    		final = strtok(NULL, "|");
				h_errno = atoi(final);
	    		
	    		//printf("errno=%d,h_errno=%d\n",errno,h_errno);
	    	} else {

	    		fprintf(stderr, "Error: netclose() invalid response from server.\n");
	
	    	}

	    //Data not received
	    } else {

	    	fprintf(stderr, "Error: netclose() failed to receive data from server.\n");
	    
	    }
	//Data failed to send
	} else {

		fprintf(stderr, "Error: netclose() failed to send data to the server.\n");
	
	}

	//Close socket when done
	close(socket_fd);

	if (status == 'F'){
		//fails, set errno and return -1
		perror("netclose() failed");
		return -1;

	} else {
		
		return 0;

	}
}

/**
    Used to initialize connection to server and open socket.

    hostname: 	The name or IP of the server to connect to
    mode:		"unrestricted"   "exclusive"   "transaction"

    returns: 	0 if successful, or -1 if an error occurred
*/
int netserverinit(char * hostname, int mode){
	int connect_resp, socket_fd;
	struct addrinfo hints, * infoptr, * curr;

	memset(&hints, 0, sizeof(hints));//Zero out hints
	hints.ai_family = AF_INET; // IPv4 Address only
	hints.ai_socktype = SOCK_STREAM;

	//Prepare for error check
	h_errno = 0;
	errno = 0;

	int status = getaddrinfo(hostname, NULL, &hints, &infoptr); //DNS Lookup
	

	//Success
	if (status == 0) {

		//Create socket
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);

		if (socket_fd < 0) {
			perror("Error opening socket");
			exit(EXIT_FAILURE);
		}

		//Loop through attempt to create connection
		for (curr = infoptr;curr!=NULL;curr=curr->ai_next) {

			((struct sockaddr_in *)curr->ai_addr)->sin_port = htons(PORT_NUM);
			((struct sockaddr_in *)curr->ai_addr)->sin_family = AF_INET;

			connect_resp = connect(socket_fd,(struct sockaddr *) curr->ai_addr,curr->ai_addrlen);
			
			if (connect_resp == 0) {
				break;
			}
		}
	
		if (connect_resp < 0) {
			perror("Error connecting");
			exit(EXIT_FAILURE);
		}
		
		//At this point ready to run other functions.
		can_connect = 1;
		server_addr = (struct sockaddr *) curr->ai_addr;
		addr_len = curr->ai_addrlen;
		close(socket_fd);

		freeaddrinfo(infoptr); //free memory from getaddrinfo
		return 0;

	//Failure
	} else {
		h_errno = HOST_NOT_FOUND;
		freeaddrinfo(infoptr); //free memory from getaddrinfo
		return -1;

	}
}
