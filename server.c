#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

void usage(){
	printf("Usage: server <UDP listen port>\n");
	exit(1);
}

int transferToInt(char s[]){
	int n,i,c,ans;
	ans=0;
	n = strlen(s);
	for(i=0;i<n;i++){
		c=s[i];
		if(c>=48 && c<=57){
			ans += (c-48)*pow(10,n-i-1);
		}
		else return(-1);
	}
	return ans;
}

int main(int argc, char * argv[]){
	printf("Starting server...\n");
	int socket_fd = 0,port_num = 0;
	struct sockaddr_in server_addr, client_addr;
	char client_msg[1000],server_msg[1000];
	socklen_t client_addr_length = sizeof(client_addr);

	memset(server_msg, '\0', sizeof(server_msg));
    memset(client_msg, '\0', sizeof(client_msg));

	if(argc != 2){
		usage();
	}
	else{
		port_num = transferToInt(argv[1]);
		if(port_num < 0){
			usage();
		}
	}

	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_fd<0){
		perror("error: socket started failed\n");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	

	if(bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		perror("error: binding failed\n");
		exit(1);
	}

	if(recvfrom(socket_fd, client_msg, sizeof(client_msg), 0, 
		(struct sockaddr*)&client_addr, &client_addr_length) < 0){

		perror("error: receiving failed\n");
		exit(1);
	}
	printf("Received message from %s: %s\n",
           inet_ntoa(client_addr.sin_addr),client_msg);

	if(strcmp("ftp",client_msg) == 0){
		strcpy(server_msg, "yes");
	}
	else{
		strcpy(server_msg, "no");
	}

	if (sendto(socket_fd, server_msg, sizeof(server_msg), 0, 
		(struct sockaddr*)&client_addr, client_addr_length) < 0){

        printf("error: sending failed\n");
        exit(1);
    }
	close(socket_fd);
	return 0;
}
