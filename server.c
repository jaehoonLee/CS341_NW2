#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define CHUNKSIZE 16
#define MEMORYSIZE 4048
#define BUFSIZE 16
#define NEGOSIZE 8

void readSocket(int sockfd, unsigned char buf[], int size);
void printbufdump(unsigned char *buf);
int negotiating(int client_sockfd, int transId);
char* removeRedundancy(char *memory, int length);
unsigned short checksum(const char *buf, unsigned size);
void printBuf(char buf[]);
void printBufWithSize(char buf[], int size);
void doprocessing(int client_sockfd, int transId, char memory[]);
void writeChunk(int sockfd, char buffer[], int size);

int main(int argc, char *argv[])
{
	int server_sockfd, client_sockfd;
	int state, client_len;
  	int pid;
	int transId = 0;

	char memory[MEMORYSIZE];
  
	struct sockaddr_in clientaddr, serveraddr;       
 
	if (argc != 3) {
	  printf("Usage : ./server -p [port]\n"); 
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
	serveraddr.sin_port = htons(atoi(argv[2]));

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

void printBufWithSize(char buf[], int size) {	
  printf("size:%d\n", size);
  int i = 0;
  for(i = 0; i < size; i++) {
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
	
	//checksum check
	char negobuf_check[8];
	memcpy(negobuf_check, negobuf, 8);
	negobuf_check[2] = 0x00;
	negobuf_check[3] = 0x00;
	cs = checksum(negobuf_check, 8);
	negobuf_check[2] = cs & 0xff;
	negobuf_check[3] = (cs >> 8) & 0xff;

	if((negobuf[2] != negobuf_check[2]) || (negobuf[3] != negobuf_check[3])){
	  printf("wrong checksum:%d", transId);
	  return -1;
	}

	return negobuf[1];
}

char* removeRedundancy(char *memory, int length) {
  
  if(length == 0)
    return NULL;

  char * resultbuf = (char *)malloc(sizeof(length));
  int i = 0; int j = 0;
  
  for (i = 1; i < length; i++) {
    if(memory[i-1] != memory[i]){
      resultbuf[j] = memory[i-1];
      j++;      
    }
  }
  
  resultbuf[j] = memory[length-1];
  j++;

  char * finalbuf = (char *)malloc(sizeof(char)*j);
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

  int last_str = 0;
  int buf_index = 0;
  char* allbuf;
	
  // 연결 이후 입력 받는 중
  while(1) {
    
    printf("message reading\n");	  
    char buf[BUFSIZE];
    if(proto == 1){     

      //read from client 
      readSocket(client_sockfd, buf, BUFSIZE);			
      printBuf(buf);

      //concat buf to allbut
      if(buf_index == 0){
	allbuf = (char *)malloc((buf_index + 1) * BUFSIZE);
      }
      else{
	allbuf = (char *)realloc(allbuf, (buf_index + 1) * BUFSIZE);
      }
      
      //copy read buf to allbuf
      int i = 0;
      for(i = 0; i < BUFSIZE ; i++){
	allbuf[i + (buf_index * BUFSIZE)] = buf[i]; 
	if((i + (buf_index * BUFSIZE)) != 0){
	  if((buf[i-1] == '\\') && (buf[i] == '0')){
	    last_str = 1;
	    break;
	  }
	}
      }
      
      //redundancy calculation 
      if(last_str){
			
	printBufWithSize(allbuf, (buf_index + 1) * BUFSIZE);
	char * redunt_str = removeRedundancy(allbuf, (buf_index + 1) * BUFSIZE);
	int c = countBuf(redunt_str);
	printf("count:%d\n", c); 
	/* printBuf(redunt_str); */
	writeChunk(client_sockfd, redunt_str, countBuf(redunt_str));
	//	write(client_sockfd, redunt_str, BUFSIZE);
	free(redunt_str);
	free(allbuf);
	
	break;
      }

      buf_index++;
    } else if(proto == 2){
      //read Socket
      readSocket(client_sockfd, buf, BUFSIZE);			
      int leng = 0;      
      leng = leng + buf[0];
      leng = (leng << 8) + buf[1];
      leng = (leng << 8) + buf[2];
      leng = (leng << 8) + buf[3];
      
      allbuf = (char *)malloc(leng + BUFSIZE);
      memcpy(allbuf, buf+4, BUFSIZE - 4); 
      buf_index = 1;

      //read whole data
      while((buf_index * BUFSIZE - 4)  < leng){
	readSocket(client_sockfd, buf, BUFSIZE);
	memcpy(allbuf + (BUFSIZE-4) + (buf_index - 1) * BUFSIZE, buf, BUFSIZE);
	buf_index++;
      }
       
      //calculate redundancy
      printBuf(allbuf);
      char * redunt_str = removeRedundancy(allbuf, leng + BUFSIZE);
      printBuf(redunt_str);

      //make protocol 2-2
      int size_str = countBuf(redunt_str)-1;
      char redunt_buf[BUFSIZE]; 
      redunt_buf[3] = size_str & 0xff; 
      redunt_buf[2] = (size_str >> 8) & 0xff; 
      redunt_buf[1] = (size_str >> 16) & 0xff; 
      redunt_buf[0] = (size_str >> 24) & 0xff; 
      memcpy(redunt_buf+4, redunt_str, BUFSIZE-4);
      write(client_sockfd, redunt_buf, BUFSIZE);
      free(redunt_str);
      free(allbuf);
      break;
    } else if(proto == -1){
      readSocket(client_sockfd, buf, BUFSIZE);
      char dummy[1] ={0x00};
      write(client_sockfd, dummy, 1);
      break;
    }
  }
}

int countBuf(char buf[]) {
  return strlen(buf);
}

void writeChunk (int sockfd, char buffer[], int size) 
{
  int i;
  printf("total:%d", (size/CHUNKSIZE));
  for (i = 0; i < (size/CHUNKSIZE) + 1; i++) {
    write(sockfd, buffer + i * CHUNKSIZE, CHUNKSIZE);
  }
}
