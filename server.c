#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

// TODO : fork() for multi process 

#define MEMORYSIZE 4048
#define BUFSIZE 255
#define NEGOSIZE 8

void readSocket(int sockfd, unsigned char buf[], int size);
void printbufdump(unsigned char *buf);
int negotiating(int client_sockfd, int transId);
char removeRedundancy(char memory[]);


int main(int argc, char *argv[])
{
	int server_sockfd, client_sockfd;
	int state, client_len;
  	int pid;
	int transId = 0;

	char memory[MEMORYSIZE];
  
	struct sockaddr_in clientaddr, serveraddr;
 
	if (argc != 2) {
    	printf("Usage : ./server [port]\n"); 
    	exit(0);
  	}

	printf("creating socket");

	// get address file
	client_len = sizeof(clientaddr);

	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket error: ");
		exit(0);
	}
	printf("server socket created\n");

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(atoi(argv[1]));

	// binding
	state = bind(server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(state == -1){
		perror("bind error : ");
		exit(0);
	}
	printf("binded\n");

	// listening
	state = listen(server_sockfd, 10);
	if(state == -1){
		perror("listen error : ");
		exit(0);
	}
	printf("listened...\n");

	// 연결
	while(1) {

		printf("running.....\n");
		client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);

		if(client_sockfd == -1){
			perror("Accept error : ");
			exit(0);      
		}

		// 연결하나 더 생기면 transId ++
		transId = transId + 1;
		printf("accepted\n");

		int proto = negotiating(client_sockfd, transId);

		
		// 연결 이후 입력 받는 중
		while(1) {

			printf("message reading\n");
			
			char buf[BUFSIZE];
			memset(buf, '0', BUFSIZE);

			readSocket(client_sockfd, buf, BUFSIZE);
			printbufdump(buf);
		
			strcat(memory, buf);

			if (hasTerminalSignal(buf)) {

				char *result = removeRedundancy(memory);

				write(client_sockfd, result, strlen(result));
				printf("write\n");
				printbufdump(result);

				break;
			}

			sleep(1);

		}
		
		sleep(1);

	}    

	close(client_sockfd);
	return 0;
}

void printbufdump(unsigned char *buf) {
	printf("%d\n", (int)sizeof(buf));
	int i = 0;
	for(i = 0; i < sizeof(buf); i++) {
		printf("%02x", (unsigned int)(buf[i]));
		if(i%4 == 3) printf("\n");
	}
	printf("\n");
}

void readSocket(int sockfd, unsigned char buf[], int size) {
	bzero(buf, size);
	int n = read(sockfd, buf, size);
	if (n<0) {
		perror("ERROR reading from socket");
		exit(1);
	}
}

int negotiating(int client_sockfd, int transId) {

	char negobuf[NEGOSIZE];

	// op, proto
	negobuf[0] = 0x00;
	negobuf[1] = 0x00;

	// checksum
	negobuf[2] = 0x11; 
	negobuf[3] = 0x11;
	
	char hex[4];
	bzero(hex, 4);

	//sprintf(hex, "%x", transId);
	negobuf[4] = hex[0];
	negobuf[5] = hex[1];
	negobuf[6] = hex[2];
	negobuf[7] = hex[3];

	write(client_sockfd, negobuf, NEGOSIZE);
	printf("write\n");
	printbufdump(negobuf);

	readSocket(client_sockfd, negobuf, NEGOSIZE);
	printf("read\n");
	printbufdump(negobuf);

	return negobuf[1];
}

int hasTerminalSignal(char buf[]) {

	for (int i = 0; i < strlen(buf); i++) {
		if (i!=0) {
			if (buf[i] == '0' && buf[i-1] == "\\") {
				return 1;
			}
		}
	}

	return 0;

}

char removeRedundancy(char memory[]) {

	char former;
	char current;
	char *result;

	for (int i = 0; i < strlen(memory); i++) {
		
		current = memory[i];

		if (former != current) {
			strcat(result, current);
			former = current;
		}
	}

	return result;
}




