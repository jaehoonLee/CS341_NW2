#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char *argv[])
{
  int server_sockfd, client_sockfd;
  int state, client_len;
  int pid;
  
  FILE *fp;
  struct sockaddr_in clientaddr, serveraddr;
  
  char buf[255];
  char line[255];
  
  if (argc != 2){
    printf("Usage : ./server [port]\n"); 
    exit(0);
  }

  memset(line, '0', 255);
  



  return;
}
