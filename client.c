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
unsigned short checksum(const char *buf, unsigned size);

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
  negobuf[1] = 0x01;
  negobuf[2] = 0x00;
  negobuf[3] = 0x00;

  short cs = checksum(negobuf, 8);
  negobuf[2] = cs & 0xff;
  negobuf[3] = (cs >> 8) & 0xff;

  /* negotiation protocal write*/
  write(sockfd, negobuf, 8);
  
  /* enter message */
  printf("Please enter your message: ");
  bzero(buffer, 256);
  fgets(buffer, 255, stdin);

  /* encoding protocol 2-1 */
  int i=0;
  while(i < 256){

    if(buffer[i] == 0x00){
      buffer[i-1] = '\\';
      buffer[i] = '0';
      //printf("break:%d\n", i);
      break;
    }
    //    printf("%c", buffer[i]);

    i++;
  }

  write(sockfd, buffer, sizeof(buffer));
  readSocket(sockfd, buffer, sizeof(buffer));

  /* decoding protocol 2-1 */
  while(i < 256){
    
    if(buffer[i] == '\\'){
      buffer[i] = 0x00;
      buffer[i+1] = 0x00;
      break;
    }
    i++;
  }
  
  printbufdump(buffer);
  printf("%s", buffer);

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
