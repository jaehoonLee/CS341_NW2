#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;

  unsigned char negobuf[8];
  char buffer[256];
  char get_buffer[256];

  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);

  /* create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(portno);

  
  /* connect */
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("ERROR connecting");
    exit(1);
  }
  
  //write(sockfd, negobuf, 8);
  bzero(negobuf, 8);
  n = read(sockfd, negobuf, 8);
  if (n < 0){
    perror("ERROR reading from socket");
    exit(1);
  }
  
  int i = 0;
  for(i = 0; i < 8; i++) {
    printf("%02x", (unsigned int)(negobuf[i]));
    if(i%4 == 3) printf("\n"); 
  }
  printf("\n");

  /* enter message */
  printf("Please enter ther message: ");
  bzero(buffer, 256);
  fgets(buffer, 255, stdin);
  
  printf("PRINGING:BUFFER: %s\n", buffer);
  
  /* send message to server */
  /*
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);
  if (n < 0){
    perror("ERROR reading from socket");
    exit(1);
  }
  */
  printf("%s\n", buffer);
  return 0;
}
