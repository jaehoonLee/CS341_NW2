#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main() 
{
  char * allbuf = (char *)malloc(1);
  char buf[256];
  int allbuf_size = 0;
  while(fgets(buf, 256, stdin) != NULL){
    char * savedPrePos = allbuf + allbuf_size;
    allbuf_size += strlen(buf);
    allbuf = (char*)realloc(allbuf, allbuf_size);
    memcpy(savedPrePos, buf, strlen(buf));
  }

  allbuf = (char*)realloc(allbuf, allbuf_size+2);
  allbuf[allbuf_size] = '\\';
  allbuf[allbuf_size + 1] = '0';
  printf("%s", allbuf);

  free(allbuf);
  return 0;
}
