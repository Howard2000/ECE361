#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

void usage(){
	printf("Usage: deliver <server address> <server port number>\n");
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
	printf("Starting client...\n");

	int socket_fd, server_port, fd;
	struct sockaddr_in server_addr;
	char server_msg[1000], client_msg[1000], file_path[1000];
	in_addr_t nbo_addr;
	socklen_t server_addr_length = sizeof(server_addr);

	memset(server_msg, '\0', sizeof(server_msg));
    memset(client_msg, '\0', sizeof(client_msg));
	memset(file_path, '\0', sizeof(file_path));

	if(argc != 3){
		usage();
	}
	else{
		nbo_addr = inet_addr(argv[1]);
		// printf("%x\n", nbo_addr);
		if(nbo_addr < 0){
			perror("Invalid address\n");
			exit(1);
		}

		server_port = transferToInt(argv[2]);
		if(server_port < 0){
			usage();
		}
	}

	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_fd<0){
		perror("error");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = nbo_addr;
	printf("ftp command: ftp <file name>\n");
	printf(">>");
	scanf("%s %s", client_msg, file_path);

	if(strcmp("ftp",client_msg) == 0){
		if((fd = open(file_path, O_RDONLY)) < 0){
			perror("error");
			exit(1);
		}
	}
	else{
		printf("command not found\n");
		exit(1);
	}
	
	if(sendto(socket_fd, client_msg, sizeof(client_msg), 0, 
		(struct sockaddr*)&server_addr, server_addr_length) < 0){
		perror("error: sending message failed\n");
		exit(1);
	}

	if(recvfrom(socket_fd, server_msg, sizeof(server_msg), 0, 
		(struct sockaddr*)&server_addr, &server_addr_length) < 0){
		perror("error: receiving message failed\n");
		exit(1);
	}

	printf("%s\n", server_msg);
	if(strcmp("yes",server_msg) == 0){
		printf("A file transfer can start.\n");
	}
	else{
		exit(1);
	}
	close(fd);
	close(socket_fd);
	return 0;
}
