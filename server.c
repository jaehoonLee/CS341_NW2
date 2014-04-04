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
#define BUFSIZE 256
#define NEGOSIZE 8

void readSocket(int sockfd, unsigned char buf[], int size);
void printbufdump(unsigned char *buf);
int negotiating(int client_sockfd, int transId);
char* removeRedundancy(char memory[]);
unsigned short checksum(const char *buf, unsigned size);
void printBuf(char buf[]);
void doprocessing(int client_sockfd, int transId, char memory[]);

int main(int argc, char *argv[])
{
	int server_sockfd, client_sockfd;
	int state, client_len;
  	int pid;
	int transId = 0;

	char memory[MEMORYSIZE];
  
	struct sockaddr_in clientaddr, serveraddr;

	
	char test[4];
	test[0] = 'a';
	test[1] = 'a';
	test[2] = 'b';
	test[3] = 'c';

	
	char * test2 = removeRedundancy(test);
	printf("%s:%d", test2, sizeof(test2) / sizeof(char));
 
	if (argc != 2) {
    	printf("Usage : ./server [port]\n"); 
    	exit(0);
  	}

	printf("creating socket\n");

	// get address file
	client_len = sizeof(clientaddr);

	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket error: ");
		exit(0);
	}

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

		printf("accepted\n");

		pid = fork();
     
    if (pid < 0) {
        perror("ERROR on fork");
	   		exit(1);
     }
        
     // child proccess
     if (pid == 0) {

     	printf("forked\n");

     	transId = transId + 1;
     	doprocessing(client_sockfd, transId, memory);
            
		 	exit(0);
  	} 
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

void printBuf(char buf[]) {
	
  printf("size:%d\n", (int)strlen(buf));
  int i = 0;
  for(i = 0; i < strlen(buf); i++) {
    printf("%c", buf[i]);	
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

	negobuf[0] = 0x00;
	negobuf[1] = 0x00;
	negobuf[2] = 0x00; 
	negobuf[3] = 0x00;
	
	negobuf[4] = (transId >> 24) & 0xff;
	negobuf[5] = (transId >> 16) & 0xff;
	negobuf[6] = (transId >> 8) & 0xff;
	negobuf[7] = transId & 0xff;
	
	short cs = checksum(negobuf, 8);
	negobuf[2] = cs & 0xff;
	negobuf[3] = (cs >> 8) & 0xff;
	
	write(client_sockfd, negobuf, NEGOSIZE);
	printf("write\n");
	printbufdump(negobuf);

	readSocket(client_sockfd, negobuf, NEGOSIZE);
	printf("read\n");
	printbufdump(negobuf);
	
	//TODO: checksum check

	return negobuf[1];
}

int hasTerminalSignal(char buf[]) {
  int i = 0;
  for (i = 0; i < strlen(buf); i++) {
    if (i!=0) {
      //				printf("%d)%c,%c ", i, buf[i-1], buf[i]); 
      //			printf("(%d%d%d)", buf[i-1] == '\\', buf[i]==0, buf[i]=='0');
      if (buf[i] == '0'){ // && buf[i-1] == '\\') {
	printf("******");
	return 1;
      }
    }
  }
  return 0;
}

char* removeRedundancy(char memory[]) {
  
  char * resultbuf = (char *)malloc(sizeof(memory));
  int i = 0; int j = 0;
  for (i = 1; i < sizeof(memory) / sizeof(char); i++) {
    if(memory[i-1] != memory[i]){
      resultbuf[j] = memory[i-1];
      j++;      
    }
  }
  char * finalbuf = (char *)malloc(j);
  memcpy(finalbuf, resultbuf, j);
  free(resultbuf);

  return finalbuf;
}

unsigned short checksum(const char *buf, unsigned size)
{
  unsigned sum = 0;
  int i;

  /* Accumulate checksum */
  for (i = 0; i < size - 1; i += 2)
    {
      unsigned short word16 = *(unsigned short *) &buf[i];
      sum += word16;
    }

  /* Handle odd-sized case */
  if (size & 1)
    {
      unsigned short word16 = (unsigned char) buf[i];
      sum += word16;
    }

  /* Fold to get the ones-complement result */
  while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

  /* Invert to get the negative in ones-complement arithmetic */
  return ~sum;
}

void doprocessing(int client_sockfd, int transId, char memory[]) {

	// protocol negotiation
	int proto = negotiating(client_sockfd, transId);

	bzero(memory, strlen(memory));

	// 연결 이후 입력 받는 중
	while(1) {

		printf("message reading\n");
		
		char buf[BUFSIZE];

		readSocket(client_sockfd, buf, BUFSIZE);			
		printBuf(buf);
		
		/*
		strcat(memory, &buf);
		printBuf(memory);

		printf("1\n");
		if (hasTerminalSignal(buf)) {
			printf("2\n");
			char * result = removeRedundancy(memory);
			printf("3\n");
			write(client_sockfd, result, strlen(result));
			printf("write\n");
			printBuf(result);

			break;
		}
		*/
	}
}
