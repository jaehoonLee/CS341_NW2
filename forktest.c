#include <stdio.h>
#include <stdlib.h>

int main()
{
  int counter = 0;
  pid_t pid;
  
  printf("child process create\n");
  pid = fork();

  switch(pid)
  {
  case -1:
    printf("child process create failed\n");
    break;
  case 0:
    printf("I am child process\n");
    while(1){
      sleep(1);
    }
    break;
  default:
    printf("I am parent process : %d\n", pid);
    while(1){
      sleep(1);
    }
    break;
  }
  return;
}
