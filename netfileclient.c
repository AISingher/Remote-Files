#include "libnetfiles.h"

int main(int argc, char * argv[]){
	
	int res = netserverinit("factory.cs.rutgers.edu",0);
	if (res == 0) {
		//successful connection
		printf("Connection successful!\n");
	}
	char buff[10];
	buff[9] = '\0';
	
	int fd = netopen("test.txt", O_RDWR);
	netread(fd, buff, 7);
	netwrite(fd, "This is a test", 14);
	
	netclose(fd);
	

	return 0;
}
