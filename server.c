#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
  int server_sockfd, client_sockfd;
  int state, client_len;
  int pid;
  
  struct sockaddr_in clientaddr, serveraddr;
  
  char buf[255];
  char line[255];

  if (argc != 2){
    printf("Usage : ./server [port]\n"); 
    exit(0);
  }

  printf("creating socket2");
  memset(line, '0', 255);
  state = 0;

  //get address file
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

  state = bind(server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
  if(state == -1){
    perror("bind error : ");
    exit(0);
  }
  printf("binding...\n");

  state = listen(server_sockfd, 10);
  if(state == -1){
    perror("listen error : ");
    exit(0);
  }
  printf("listening...\n");
 
  while(1){
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);
    if(client_sockfd == -1){
      perror("Accept error : ");
      exit(0);      
    }
    
    bzero(&buf, sizeof(buf));
    buf[0] = 0x01;
    write(client_sockfd, buf, strlen(buf));
    
    close(client_sockfd);
    sleep(1);
    
    /*
    while(1){      
      memset(buf, '0', 255);
      if(read(client_sockfd, buf, 255) <= 0){
	close(client_sockfd);
	break;
      }

      printf("read");      
      write(client_sockfd, "helloWorld", 255);     
    }
    */
  }    

  close(client_sockfd);
  return;
}
