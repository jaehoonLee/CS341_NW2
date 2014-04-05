#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define CHUNKSIZE 16

void printbufdump(unsigned char *buf);
void readSocket(int sockfd, char negobuf[], int size);
unsigned short checksum(const char *buf, unsigned size);
void writeChunk(int sockfd, char buffer[], int size);
int checkLastInput(char buf[], int size);
void printBuf(char buf[]);
void printBufWithSize(char buf[], int size);

int main(int argc, char *argv[])
{
  int sockfd, portno, n, proto_num, i;
  struct sockaddr_in serv_addr;

  char negobuf[8];
  char buffer[CHUNKSIZE];
  char strbuffer[CHUNKSIZE];

  if (argc < 7) {
    fprintf(stderr, "usage %s -h hostname -p port -m protocol \n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[4]);
  proto_num = atoi(argv[6]);
  if((proto_num != 1) && (proto_num != 2)) {
    perror("wrong protocol number, only 1 and 2 are available");
    exit(0);
  }

 	if ((strncmp(argv[1], "-h", strlen(argv[1])) != 0) ||
			(strncmp(argv[3], "-p", strlen(argv[3])) != 0) ||
			(strncmp(argv[5], "-m", strlen(argv[5])) != 0)) {
    perror("error! usage : ./client -h 127.0.0.1 -p 9999 -m 2");
    exit(0);
	}

  /* create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[2]);
  serv_addr.sin_port = htons(portno);

  /* connect */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("ERROR connecting");
    exit(1);
  }

  /* protocol negotiaton */
  readSocket(sockfd, negobuf, 8);

  /* checksum calculation */
  negobuf[0] = 0x01;
  negobuf[1] = proto_num & 0xff;
  negobuf[2] = 0x00;
  negobuf[3] = 0x00;

  short cs = checksum(negobuf, 8);
  negobuf[2] = cs & 0xff;
  negobuf[3] = (cs >> 8) & 0xff;

  /* negotiation protocal write*/
  write(sockfd, negobuf, 8);
  
  /* enter message */
  printf("Please enter your message: ");
  
  char* allbuf = (char * ) malloc(CHUNKSIZE);
  int numberOfChunk = 1;
  int extra = 0;
  while(1) {
    bzero(buffer, CHUNKSIZE);
    fgets(buffer, CHUNKSIZE + 1, stdin);
    
    memcpy(allbuf+(numberOfChunk-1)*CHUNKSIZE, buffer, CHUNKSIZE);
    
    if (checkLastInput(buffer, CHUNKSIZE)) {
      /* encoding protocol */
      if(proto_num == 1) {    
	/* encoding protocol 2-1 */
	for(i=(numberOfChunk-1)*CHUNKSIZE; i < numberOfChunk * CHUNKSIZE ;i++){
	  if(allbuf[i] == 0x00){
	    allbuf[i-1] = '\\';
	    allbuf[i] = '0';
	    //printf("break:%d\n", i);
	    break;
	  }

	  if(i==(numberOfChunk * CHUNKSIZE - 1)){
	    if(allbuf[i] == '\n' || allbuf[i] == EOF){
	      allbuf =  (char *) realloc(allbuf, numberOfChunk*CHUNKSIZE + 1);
	      allbuf[i] = '\\';
	      allbuf[i+1] = '0';
	      extra = 1;
	    }	     
	    else{
	      allbuf =  (char *) realloc(allbuf, numberOfChunk*CHUNKSIZE + 2);
	      allbuf[i+1] = '\\';
	      allbuf[i+2] = '0';
	      extra = 2;
	    }
	  }
	}
      }else if(proto_num == 2) {
	/* encoding protocol 2-2 */
	allbuf = (char *) realloc(allbuf, numberOfChunk*CHUNKSIZE + 4);
	memcpy(allbuf+4, allbuf, numberOfChunk*CHUNKSIZE);
	for(i=(numberOfChunk-1)*CHUNKSIZE; i < numberOfChunk * CHUNKSIZE ;i++){
	  if(allbuf[i] == '\n' || allbuf[i] == EOF){
	    break;
	  }
	}
	strbuffer[3] = i & 0xff;
	strbuffer[2] = (i >> 8) & 0xff;
	strbuffer[1] = (i >> 16) & 0xff;
	strbuffer[0] = (i >> 24) & 0xff;
	memcpy(allbuf, strbuffer, 4);	
      }

      break;
    }
    
    numberOfChunk++;
    allbuf = (char *) realloc(allbuf, numberOfChunk*CHUNKSIZE);
  }
  
  //  printBuf(allbuf);

  /* write string to server */
  if(proto_num == 1)
    writeChunk(sockfd, allbuf, numberOfChunk*CHUNKSIZE+extra);
  else if(proto_num == 2)
    writeChunk(sockfd, allbuf, numberOfChunk*CHUNKSIZE + 4);

  /* decoding protocol 2 */
  i = 0;
  if(proto_num == 1){
    
    /* read response from server */
    readSocket(sockfd, buffer, CHUNKSIZE);
    int lastline = 0;
    while(1)
    {
      printBuf(buffer);
      if(buffer[i] == '\\'){
	buffer[i] = 0x00;
	buffer[i+1] = 0x00;
	lastline = 1;
	break;
      }
      i++;
      if(i == CHUNKSIZE){
	printf("%s", buffer);
	i = 0;
      }
    }
    printf("\n");
  } else if(proto_num == 2){

    /* read response from server */
    readSocket(sockfd, buffer, CHUNKSIZE);

    char tempbuf[CHUNKSIZE-4];
    memcpy(tempbuf, buffer+4, CHUNKSIZE-4);
    printf("%s",tempbuf);
 
    int leng = 0;
    leng = leng + buffer[0];
    leng = (leng << 8) + buffer[1];
    leng = (leng << 8) + buffer[2];
    leng = (leng << 8) + buffer[3];

    leng = leng - (CHUNKSIZE - 4);
    while(leng > 0) {
      bzero(buffer, sizeof(buffer));
      readSocket(sockfd, buffer, CHUNKSIZE);
      printf("%s", buffer);
      leng -= CHUNKSIZE;
    }
    printf("\n");
  }

  
  /* print output */
  //  printf("%s\n", buffer);

  return 0;
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

void printBuf(char buf[]) {
  printf("size:%d\n", (int)strlen(buf));
  int i=0;
  for (i=0; i<strlen(buf); i++) {
    printf("%c", buf[i]);
  }
  printf("\n");
}

void printBufWithSize(char buf[], int size) {	
  printf("size:%d\n", size);
  int i = 0;
  for(i = 0; i < size; i++) {
    printf("%02x", buf[i]);	
  }
  printf("\n");
}

void printbufdump(unsigned char *buf)
{
  printf("%d\n", (int)sizeof(buf));
  int i = 0;
  for(i = 0; i < sizeof(buf); i++) {
    printf("%02x", (unsigned int)(buf[i]));
    if(i%4 == 3) printf("\n"); 
  }
  printf("\n");
}

void printallbuffer(char buffer[], int length)
{
  int i = 0;
  for(i = 0; i < length; i++){
    printf("%02x ", buffer[i]);
  }
}

void readSocket(int sockfd, char negobuf[], int size)
{
  bzero(negobuf, size);
  int n = read(sockfd, negobuf, size);
  if (n < 0){
    perror("ERROR reading from socket");
    exit(1);
  }
}


void writeChunk(int sockfd, char buffer[], int size){
  int i;
  printf("writeCHUNK\n");
  for (i = 0; i < (size/CHUNKSIZE)+1; i++){ 
    printf("(%d) %d %d \n", i, size, CHUNKSIZE);
    write(sockfd, buffer + i*CHUNKSIZE, CHUNKSIZE);
  }
}

int checkLastInput(char buf[], int size)
{
  int i = 0;
  for(i = (size - 1); i >= 0; i--){
    if(buf[i] == 0x00)
      return 1;
    else if(buf[i] == EOF)
      return 1;
  }
  
  return 0;
}
