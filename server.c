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
#include <fcntl.h>


#define MAX_FILE_SIZE 100000 //100 MB (no use)

struct packet{
	unsigned int total_frag;

	unsigned int frag_no;
	unsigned int size;
	char * filename;
	char filedata[1000]; //1 KB
};

int transferToInt(char s[]);

void unpack(struct packet * pack, char * msg){
	int ptr=0,col_num=0,strl;
	char temp[2000]={0};
	strl=strlen(msg);
	for(int i=0;i<strl;i++){
		// printf("i: %d\n",i);
		if(col_num>3){
			// printf("%d\n",i-ptr);
			for(int j=0;j<pack->size;j++){
				pack->filedata[j]=msg[j+ptr];
			}
			break;
		}
		else if(msg[i]==':'){
			// printf("ptr: %d\n",ptr);
			switch (col_num){
				case 0:
					memset(temp,0,sizeof(temp));
					strncpy(temp,msg+ptr,i-ptr);
					// for(int j=0;j<i-ptr;j++){
					// 	temp[j]=msg[j+ptr];
					// }
					// temp[i-ptr]='\0';
					pack->total_frag = transferToInt(temp);
					ptr=i+1;
					
					break;
				case 1:
					memset(temp,0,sizeof(temp));
					strncpy(temp,msg+ptr,i-ptr);
					// for(int j=0;j<i-ptr;j++){
					// 	temp[j]=msg[j+ptr];
					// }
					// temp[i-ptr]='\0';
					pack->frag_no = transferToInt(temp);
					ptr=i+1;
					
					break;
				case 2:
					memset(temp,0,sizeof(temp));
					strncpy(temp,msg+ptr,i-ptr);
					// for(int j=0;j<i-ptr;j++){
					// 	temp[j]=msg[j+ptr];
					// }
					// temp[i-ptr]='\0';
					pack->size = transferToInt(temp);
					ptr=i+1;
					
					break;
				case 3:
					memset(temp,0,sizeof(temp));
					strncpy(temp,msg+ptr,i-ptr);
					// for(int j=0;j<i-ptr;j++){
					// 	temp[j]=msg[j+ptr];
					// }
					// temp[i-ptr]='\0';
					pack->filename = temp;
					ptr=i+1;
					
					break;
				default:
					break;
			}
			col_num++;
			
		}
		
	}
	return;
}

void progress(double perc){
	int x;
	x = (int)perc%4;
	switch(x){
		case 0:
			printf(" %6.2lf%% -\r",perc);
			fflush(stdout);
			break;
		case 1:
			printf(" %6.2lf%% \\\r",perc);
			fflush(stdout);
			break;
		case 2:
			printf(" %6.2lf%% |\r",perc);
			fflush(stdout);
			break;
		case 3:
			printf(" %6.2lf%% /\r",perc);
			fflush(stdout);
			break;	
		default:
			break;
	}
	return;
}

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

	//declear vailables
	int socket_fd = 0,port_num = 0,ack_pk=0,total_pk;
	int set=0, opened=0;
	struct sockaddr_in server_addr, client_addr;
	char client_msg[2000],server_msg[2000];
	socklen_t client_addr_length = sizeof(client_addr);
	struct packet pack;
	FILE * fp;
	int ret;
	
	//initializion
	memset(server_msg, '\0', sizeof(server_msg));
    memset(client_msg, '\0', sizeof(client_msg));
	//input validation
	if(argc != 2){
		usage();
	}
	else{
		port_num = transferToInt(argv[1]);
		if(port_num < 0){
			usage();
		}
	}
	//setup socket
	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_fd<0){
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind socket
	if(bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		perror("bind");
		exit(1);
	}

	//start listing
	if(recvfrom(socket_fd, client_msg, sizeof(client_msg), 0, 
		(struct sockaddr*)&client_addr, &client_addr_length) < 0){

		perror("recvfrom");
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

        printf("sendto");
        exit(1);
    }
	//file transfer sesson
	do{

		if(recvfrom(socket_fd, client_msg, sizeof(client_msg), 0, 
			(struct sockaddr*)&client_addr, &client_addr_length) < 0){

			perror("\nrecvfrom");
			exit(1);
		}
		// printf("\n%s\n",client_msg);
		unpack(&pack,client_msg);
		// printf("%d\n",pack.total_frag);
		// printf("%d\n",pack.frag_no);
		// printf("%d\n",pack.size);
		// printf("%s\n",pack.filename);
		// printf("%s\n",pack.filedata);
		if(!set){
			total_pk=pack.total_frag;
			ack_pk = pack.frag_no;
			set=1;
		}
		
		// printf("%d %d\n",pack.frag_no, ack_pk);

		if(pack.frag_no==ack_pk){
			
			// printf("receiving file: %s",pack.filename);
			// progress((double)ack_pk/(double)total_pk*100);
			// printf("%d\n",pack.frag_no);
			if(!opened){
			// printf("receiving file: %s",pack.filename);
				fp = fopen(pack.filename,"wb");
				if(fp==NULL){
					perror("\nfopen");
					exit(1);
				}
				else{
					// printf("file opened successfully\n");
				}
				opened=1;
			}
			if(opened){
				ret = fwrite(pack.filedata,1,pack.size,fp);
				if(ret < pack.size){
            		perror("\nfwrite");
					exit(1);
        		}
				ack_pk = pack.frag_no+1;
				strcpy(server_msg, "ACK");
				if (sendto(socket_fd, server_msg, sizeof(server_msg), 0, 
				(struct sockaddr*)&client_addr, client_addr_length) < 0){

        			printf("\nsendto");
        			exit(1);
    			}
			}
			else{
				strcpy(server_msg, "NACK");
			// ack_pk = pack.frag_no;
			// printf("%s\n",server_msg);
				if (sendto(socket_fd, server_msg, sizeof(server_msg), 0, 
				(struct sockaddr*)&client_addr, client_addr_length) < 0){

        			printf("\nsendto");
        			exit(1);
    			}
			}
			
		}
		else{
			strcpy(server_msg, "NACK");
			// ack_pk = pack.frag_no;
			// printf("%s\n",server_msg);
			if (sendto(socket_fd, server_msg, sizeof(server_msg), 0, 
			(struct sockaddr*)&client_addr, client_addr_length) < 0){

        		printf("\nsendto");
        		exit(1);
    		}
		}
		
	}while(ack_pk<total_pk);
	// printf("receiving file: %s",pack.filename);
	// progress((double)ack_pk/(double)total_pk*100);
	// printf("\n");
	fclose(fp);
	close(socket_fd);
	return 0;
}
