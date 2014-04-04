#include<stdio.H>
#include<string.h>
int main()
{
  char buffer[3];
  while(1)
  {
    bzero(buffer, 3);
    fgets(buffer, 4, stdin);
    printf("%s", buffer);
    printf(" ::: %02x %02x %02x\n", buffer[0], buffer[1], buffer[2]);
    if(checkLastInput(buffer, 3)){
      break;
    }     
  }
  return;
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
