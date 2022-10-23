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
#include <time.h>
#include <sys/time.h>

#define MAX_FILE_SIZE 1000*5 //5 MB (no use)

struct packet{
	unsigned int total_frag;

	unsigned int frag_no;
	unsigned int size;
	char * filename;
	char filedata[1000]; //1 KB
};

void intToString(int n, char * s_num);

void progress_bar(double percentage){
	int x;
	printf("[");
	for(x=0;x<=percentage*30;x++){
		printf(">");
	}
	for(x=percentage*30;x<30;x++){
		printf("-");
	}
	printf("] %6.2lf%% completed\r",(double)(percentage*100));
	fflush(stdout);
	return;
}

void packUpFile(FILE * _fp, char * file_name, struct packet * pack, int pack_num, int total_pack){

	size_t rd;
	rd = fread(pack->filedata,1,1000,_fp);
	if(rd < 0 ){
		perror("fread");
		exit(1);
	}
	// printf("data: %s",pack->filedata);
	pack->total_frag = total_pack;
	pack->frag_no = pack_num;
	pack->size = rd;
	pack->filename = file_name;

	return;
}

void packToMsg(struct packet * pack, char * msg){
	char temp[2000]={0};
	memset(msg, 0, 2000);
	int ptr_shift=0;
	intToString(pack->total_frag,temp);
	strcpy(msg,temp);
	strcat(msg,":");
	memset(temp,0,sizeof(temp));
	intToString(pack->frag_no,temp);
	strcat(msg,temp);
	strcat(msg,":");
	memset(temp,0,sizeof(temp));
	intToString(pack->size,temp);
	strcat(msg,temp);
	strcat(msg,":");
	strcat(msg,pack->filename);
	strcat(msg,":");
	ptr_shift = strlen(msg);
	for(int i = 0;i<pack->size;i++){
		msg[i+ptr_shift]=pack->filedata[i];
	}
	return;
}


void usage(){
	printf("Usage: deliver <server address> <server port number>\n");
	exit(1);
}

void intToString(int n, char * s_num){
	sprintf(s_num,"%d",n);
	return;
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

	//declear variables
	int socket_fd, server_port,  pack_ptr=0, total=0;
	struct sockaddr_in server_addr;
	char server_msg[2000], client_msg[2000], file_path[2000];
	in_addr_t nbo_addr;
	socklen_t server_addr_length = sizeof(server_addr);
	struct timeval startT,endT;
	struct packet pack;
	double timeuse;
	int next = 1;
	FILE * fp;
	
	//initailization 
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
			perror("inet_addr");
			exit(1);
		}

		server_port = transferToInt(argv[2]);
		if(server_port < 0){
			usage();
		}
	}

	//crate socket
	socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(socket_fd<0){
		perror("socket");
		exit(1);
	}
	//set socket parameters
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = nbo_addr;
	printf("ftp command: ftp <file name>\n");
	printf(">>");

	//input
	scanf("%s %s", client_msg, file_path);
	//input vaildating
	if(strcmp("ftp",client_msg) == 0){
		//opoen file
		fp = fopen(file_path,"rb");
		if(fp==NULL){
			perror("fopen");
			exit(1);
		}
	}
	else{
		printf("command not found\n");
		exit(1);
	}

	//send msg
	//get start time
	gettimeofday(&startT,NULL);
	//get expected packets number
	fseek(fp,0L,SEEK_END);
	pack_ptr=0;
	total = ftell(fp)/1000 + 1; 
	//reset file stream pointer
	fseek(fp,0L,SEEK_SET);
	// printf("%d\n",total);
	if(sendto(socket_fd, client_msg, sizeof(client_msg), 0, 
		(struct sockaddr*)&server_addr, server_addr_length) < 0){
		perror("sendto");
		exit(1);
	}

	//receive
	if(recvfrom(socket_fd, server_msg, sizeof(server_msg), 0, 
		(struct sockaddr*)&server_addr, &server_addr_length) < 0){
		perror("recvfrom");
		exit(1);
	}

	//output result
	// printf("%s\n", server_msg);
	// printf("%d %d\n",pack_ptr, total);
	if(strcmp("yes",server_msg) == 0){
		printf("A file transfer can start.\n");
		while(pack_ptr < total){
			// printf("%d %d\n",pack_ptr, total);
			progress_bar((double)(pack_ptr/(double)total));
			if(next){
				packUpFile(fp,file_path,&pack,pack_ptr,total);
			}
			// printf("%d\n",pack.total_frag);
			// printf("%d\n",pack.frag_no);
			// printf("%d\n",pack.size);
			// printf("%s\n",pack.filename);
			packToMsg(&pack,client_msg);
			// printf("\n%s\n",client_msg);
			if(sendto(socket_fd, client_msg, sizeof(client_msg), 0, 
				(struct sockaddr*)&server_addr, server_addr_length) < 0){
				perror("\nsendto");
				exit(1);
			}
			if(recvfrom(socket_fd, server_msg, sizeof(server_msg), 0, 
				(struct sockaddr*)&server_addr, &server_addr_length) < 0){
				perror("\nrecvfrom");
				exit(1);
			}
			// printf("%s\n",server_msg);
			if(strcmp("ACK",server_msg) == 0){
				pack_ptr++;
				next = 1;
			}
			else{
				next = 0;
			}

		}
		progress_bar((double)(pack_ptr/(double)total));
		
	}
	else{
		exit(1);
	}
	gettimeofday(&endT,NULL);
	//calcuate time lapses
	timeuse = (endT.tv_sec-startT.tv_sec) + (double)(endT.tv_usec-startT.tv_usec)/CLOCKS_PER_SEC;
	printf("\n");
	printf("Successfully transfered, time lapses: %lf seconds\n",timeuse);
	//close file and socket
	fclose(fp);
	close(socket_fd);
	return 0;
}
