#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

void printbufdump(unsigned char *buf);
void readSocket(int sockfd, unsigned char negobuf[], int size);

int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;

  char negobuf[8];
  char buffer[256];
  char bufin[256];

  if (argc < 7) {
    fprintf(stderr, "usage %s -h hostname -p port -m 1\n", argv[0]);
    exit(0);
  }


  portno = atoi(argv[4]);
  printf("2:%d", portno);
  
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
  printbufdump(negobuf);

  negobuf[0] = 0x01;
  negobuf[1] = 0x01;
  negobuf[2] = 0xbb;
  negobuf[3] = 0x5a;

  printbufdump(negobuf);

  /* negotiation protocal write*/
  write(sockfd, negobuf, 8);
  readSocket(sockfd, negobuf, 8);
  printbufdump(negobuf);
  
  /* enter message */
  printf("Please enter ther message: ");
  bzero(buffer, 256);
  fgets(buffer, 255, stdin);
  
  /*
  uint32_t length = 0;
  int i=0;
  while(i < 256){

    if(bufin[i] == 0x00){
      length = i - 1;
      break;
    }
    printf("%c", bufin[i]);
    length++;
    i++;
  }

  printf("num:%d\n", length);
  length = htonl(length);
  buffer[0] = (length >> 24) && 0xFF;
  buffer[1] = (length >> 16) && 0xFF;
  buffer[2] = (length >> 8) && 0xFF;
  buffer[3] = length;
  printf("%02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

   i=0;
  while((i + 4) < 256){

    if(bufin[i] == 0x00){
      if(i != 0)
	buffer[i-1+4] = '\\';
      buffer[i+4] = '0';

      printf("break:%d\n", i+4);
      break;
    }
    buffer[i+4]=bufin[i];
    printf("%c", buffer[i+4]);

    i++;
  }

  */

  int i=0;
  while(i < 256){

    if(buffer[i] == 0x00){
      if(i != 0)
	buffer[i-1] = '\\';
      buffer[i] = '0';

      printf("break:%d\n", i);
      break;
    }
    printf("%c", buffer[i]);

    i++;
  }


  //  printbufdump(buffer);
  printbufdump(buffer);
  write(sockfd, buffer, sizeof(buffer));
  readSocket(sockfd, buffer, sizeof(buffer));
  printbufdump(buffer);

  /* send message to server */
  /*
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);
  if (n < 0){
    perror("ERROR reading from socket");
    exit(1);
  }
  */

  return 0;
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

void readSocket(int sockfd, unsigned char negobuf[], int size)
{
  bzero(negobuf, size);
  int n = read(sockfd, negobuf, size);
  if (n < 0){
    perror("ERROR reading from socket");
    exit(1);
  }
}
