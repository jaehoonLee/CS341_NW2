#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define CHUNKSIZE 1024
#define MEMORYSIZE 4048
#define BUFSIZE 1024
#define NEGOSIZE 8

void readSocket(int sockfd, unsigned char buf[], int size);
void printbufdump(unsigned char *buf);
int negotiating(int client_sockfd, int transId);
char* removeRedundancy(char *memory, int length);
char* removeRedundancy2(char *memory, int length);
unsigned short checksum(const char *buf, unsigned size);
void printBuf(char buf[]);
void printBufWithSize(char buf[], int size);
void doprocessing(int client_sockfd, char memory[], int* protocolMap);
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

	/* non-block */
	fd_set read;
	fd_set temp;
	int fdmax;
	struct timeval timeout;
	FD_ZERO(&read);
	FD_ZERO(&temp);
	FD_SET(server_sockfd, &read);
	fdmax = server_sockfd;
	int *protocolMap = (int *)malloc((fdmax+1) * sizeof(int));

	// 연결
	while(1) {	 

		printf("running.....\n");

		//nonblock
		memcpy(&temp, &read, sizeof(read));

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int activeNum = select(fdmax+1, &temp, (fd_set *) 0, (fd_set *) 0, &timeout);

		if (activeNum < 0) {
			//error
			perror("select");
			exit(EXIT_FAILURE);
		} else if (activeNum == 0) {
			//timeout
			printf("timeout\n");
		} else {
			// 변화가 있는 fd 존재
			int fd;
			for (fd=0; fd<=fdmax && activeNum>0; fd++) {
				// 해당 fd에 변화가 있는 경우
				if (FD_ISSET(fd, &temp)) {
					activeNum -= 1;
					if(fd==server_sockfd) {
						// 서버 소켓인 경우	
						//do {
							client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);
							if(client_sockfd < 0){
								perror("Accept error : ");
								break;      
							}
					
							transId++;
							int proto = negotiating(client_sockfd, transId);
							
							FD_SET(client_sockfd, &read);
							if (client_sockfd > fdmax) {
								fdmax = client_sockfd;
								protocolMap = (int *)realloc(protocolMap, (fdmax+1) * sizeof(int));
							}

							protocolMap[client_sockfd] = proto;
							
						//} while (client_sockfd != -1);
					} else {
						// 연결된 클라이언트 소켓인 경우
						doprocessing(fd, memory, protocolMap);	
						FD_CLR(fd, &read);
						close(fd);
					}
				}
			}
		} 
	}    	
	
	free(protocolMap);
	
	int i;
	for (i=0; i <= fdmax; ++i) {
  	if (FD_ISSET(i, &read))
    	close(i);
  }

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
	readSocket(client_sockfd, negobuf, NEGOSIZE);
	
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

    //'\0'
    if(memory[i-1] == '\\' && memory[i] == '0'){
      resultbuf[j++] = memory[i-1];
      continue;
    }

    if(memory[i-1] != memory[i]){
      //'\\'
      if(memory[i-1] == '\\'){	
	if(i==1) return NULL; //wrong message
	if(memory[i-2] != '\\') return NULL; //wrong message
	
	resultbuf[j++] = '\\';
	resultbuf[j++] = '\\';
	continue;
      }
      
      resultbuf[j] = memory[i-1];
      j++;      
    }
  }
  
  //'0'
  resultbuf[j] = memory[length-1];
  j++;

  //char * finalbuf = (char *)malloc(sizeof(char)*j);
  //memcpy(finalbuf, resultbuf, j);
  //  free(resultbuf);

  return resultbuf;
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

void doprocessing(int client_sockfd, char memory[], int* protocolMap) {
  bzero(memory, strlen(memory));

  int last_str = 0;
  int buf_index = 0;
  char* allbuf;
	
  int proto = protocolMap[client_sockfd];
	printf("protocol:%d\n", proto); 

  // 연결 이후 입력 받는 중
  while(1) {
    
    printf("message reading\n");	  
    char buf[BUFSIZE];
    if(proto == 1){     

      //read from client 
      readSocket(client_sockfd, buf, BUFSIZE);			
      //      printBuf(buf);

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
			
	//	printBufWithSize(allbuf, strlen(allbuf));
	char * redunt_str = removeRedundancy(allbuf, (buf_index + 1) * BUFSIZE);
	if(redunt_str == NULL){
	  return;
	}
	//printf("size:%d", strlen(redunt_str));
	//	printBufWithSize(redunt_str, (buf_index + 1) * BUFSIZE);

 	//int c = countBuf(redunt_str);
	//printf("count:%d\n", c); 
	/*printBuf(redunt_str); */
	//	printf("sssize:%d", strlen(redunt_str));
	writeChunk(client_sockfd, redunt_str, strlen(redunt_str));
	//	write(client_sockfd, redunt_str, BUFSIZE);
	//free(redunt_str);
	//free(allbuf);
	//	free(redunt_str);
	break;
      }

      buf_index++;
    } else if(proto == 2){
      //read Socket
      bzero(buf, BUFSIZE);
      readSocket(client_sockfd, buf, BUFSIZE);			
      int leng = 0;      
      leng = ((buf[0] & 0xFF) << 24) |
      ((buf[1] & 0xFF) << 16) |
      ((buf[2] & 0xFF) << 8) |
      (buf[3] & 0xFF);

      allbuf = (char *)malloc(leng);
      memcpy(allbuf, buf+4, strlen(buf+4)); 
      buf_index = 1;

      //read whole data
      int leng_con_size = strlen(buf+4);
      while(leng_con_size  < leng){
	bzero(buf, BUFSIZE);
	readSocket(client_sockfd, buf, BUFSIZE);
	memcpy(allbuf + leng_con_size, buf, strlen(buf));
	leng_con_size += strlen(buf);
	//	printf(": leng_con_size:%d\n", leng_con_size);
	buf_index++;
      }
       
      //calculate redundancy
      //printBufWithSize(allbuf, leng);
      char * redunt_buf = (char*)malloc(leng+4); 
      char * redunt_str = removeRedundancy2(allbuf, leng);


      //make protocol 2-2
      
      int size_str = strlen(redunt_str);

      redunt_buf[3] = size_str & 0xff; 
      redunt_buf[2] = (size_str >> 8) & 0xff; 
      redunt_buf[1] = (size_str >> 16) & 0xff; 
      redunt_buf[0] = (size_str >> 24) & 0xff; 

      memcpy(redunt_buf+4, redunt_str, strlen(redunt_str));
      
      writeChunk(client_sockfd, redunt_buf, size_str+4);
      
      //free(redunt_buf);
      //free(redunt_str);
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

char* removeRedundancy2(char *memory, int length) {
  
  if(length == 0)
    return NULL;

  char * resultbuf = (char *)malloc(sizeof(length+4));
  int i = 0; int j = 0;
  
  for (i = 1; i < length; i++) {
    if(memory[i-1] != memory[i]){
      
      resultbuf[j] = memory[i-1];
      j++;      
    }
  }
  
  //'0'
  resultbuf[j] = memory[length-1];
  j++;

  //char * finalbuf = (char *)malloc(sizeof(char)*j);
  //memcpy(finalbuf, resultbuf, j);
  //  free(resultbuf);

  return resultbuf;
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





