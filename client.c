#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define CHUNKSIZE 1024
typedef struct strarr{
  char * str;
  int size;
}NEWSTR;

void printbufdump(unsigned char *buf);
void printBuf(char buf[]);
void printBufWithSize(char buf[], int size);

void readSocket(int sockfd, char negobuf[], int size);
unsigned short checksum(const char *buf, unsigned size);
void writeChunk(int sockfd, char buffer[], int size);
NEWSTR checkSpecialChar(char str[], int size);

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
    fprintf(stderr, "usage %s -h hostname -p port -m protocol \n", argv[0]);
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
  printf("Please enter your message:\n");
  
  char * allbuf = (char *)malloc(1);
  char buf[CHUNKSIZE];
  int allbuf_size = 0;
  while(fgets(buf, 256, stdin) != NULL){
    char * savedPrePos = allbuf + allbuf_size;
    allbuf_size += strlen(buf);
    allbuf = (char*)realloc(allbuf, allbuf_size);    
    memcpy(savedPrePos, buf, strlen(buf));
    
    bzero(buf, CHUNKSIZE);
  }

  NEWSTR newstr = checkSpecialChar(allbuf, allbuf_size);
  allbuf = newstr.str;
  allbuf_size = newstr.size;

  /* make protocol */
  if(proto_num == 1) {
    allbuf = (char*)realloc(allbuf, allbuf_size + 2);
    allbuf[allbuf_size] = '\\';
    allbuf[allbuf_size + 1] = '0';
    //printBufWithSize(allbuf, allbuf_size + 2);
  }
  else if(proto_num == 2) {
    allbuf = (char *) realloc(allbuf, allbuf_size + 4);
    memcpy(allbuf+4, allbuf, allbuf_size);
    allbuf[3] = allbuf_size & 0xff;
    allbuf[2] = (allbuf_size >> 8) & 0xff;
    allbuf[1] = (allbuf_size >> 16) & 0xff;
    allbuf[0] = (allbuf_size >> 24) & 0xff;
  }
  
  /* write string to server */
  if(proto_num == 1)
    writeChunk(sockfd, allbuf, allbuf_size + 2);
  else if(proto_num == 2)
    writeChunk(sockfd, allbuf, allbuf_size + 4);

  /* string free */
  free(allbuf);

  /* decoding protocol 2 */
  i = 0;
  char * returnbuf = (char *)malloc(1);
  int returnbuf_size = 0;
  if(proto_num == 1){
    
    /* read response from server */    
    while(1){
      //receive packet
      readSocket(sockfd, buffer, CHUNKSIZE);
      int prebuf_size = returnbuf_size;
      returnbuf_size += strlen(buffer);
      printf("bufsize:%d\n", returnbuf_size);
      if(returnbuf_size == 0)
	continue;
      
      returnbuf = (char*)realloc(returnbuf, returnbuf_size);
      memcpy(returnbuf + prebuf_size, buffer, strlen(buffer));

      if(returnbuf[returnbuf_size-1] == '0' && returnbuf[returnbuf_size - 2] == '\\'){
	returnbuf[returnbuf_size-1] = 0x00;
	returnbuf[returnbuf_size-2] = 0x00;
	returnbuf = (char *)realloc(returnbuf, returnbuf_size - 2);
	break;
      }
      
    }
    printf("========================\n");
    printf("%s", returnbuf);
    //    free(returnbuf);
    printf("\n");
   
  } else if(proto_num == 2){

    /* read response from server */
    readSocket(sockfd, buffer, CHUNKSIZE);

    char tempbuf[CHUNKSIZE-4];
    memcpy(tempbuf, buffer+4, CHUNKSIZE-4);
    printf("%s",tempbuf);
    int leng = 0;
    leng = 
      ((buffer[0] & 0xFF) << 24) |
      ((buffer[1] & 0xFF) << 16) |
      ((buffer[2] & 0xFF) << 8) |
      (buffer[3] & 0xFF);

    leng = leng - (CHUNKSIZE - 4);

    while(leng > 0) {
      bzero(buffer, sizeof(buffer));
      readSocket(sockfd, buffer, CHUNKSIZE);
      printf("%s", buffer);
      leng -= CHUNKSIZE;
    }
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
    printf("%c", buf[i]);	
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
  for (i = 0; i < (size/CHUNKSIZE)+1; i++){ 
    printf("(%d) %d %d \n", i, size, CHUNKSIZE);
    //    printBufWithSize(buffer + i*CHUNKSIZE, CHUNKSIZE);
    write(sockfd, buffer + i*CHUNKSIZE, CHUNKSIZE);
  }
	printf("write complete\n");
}

NEWSTR checkSpecialChar(char* str, int size)
{
  //  printBufWithSize(str, size);
  //count '\'
  int i = 0; int numSC = 0;
  for(i = 0; i < size; i++){
    if(str[i] == '\\'){
      numSC++;
    }
  }

  //make '\' -> '\\'
  // printf("size:%d", numSC);
  char * newstr = (char*)malloc(size + numSC);
  numSC = 0;
  for(i = 0; i < size; i++){
    if(str[i] == '\\'){
      newstr[i+numSC] = str[i];  
      numSC++;
    }
    newstr[i+numSC] = str[i];
  }
  //  printBufWithSize(newstr, size + numSC);
  free(str);

  NEWSTR newStr;
  newStr.str = newstr;
  newStr.size = size + numSC;
  
  return newStr;
}
