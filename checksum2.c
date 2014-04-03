#include <stdio.h>
#include <memory.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
 
struct pseudohdr
{
  struct in_addr saddr;
  struct in_addr daddr;
  unsigned char zero;
  unsigned char protocol;
  unsigned short length;
  struct tcphdr tcpheader;
}__attribute__((packed));

unsigned short checksum(unsigned short *buf, int len)
{
  register unsigned long sum = 0;
 
  while(len--)
    sum += *buf++;
 
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
 
  return (unsigned short)(~sum);
}

unsigned short checksum1(const char *buf, unsigned size)
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

int main()
{
  
  char negobuf[8];

  bzero(negobuf, 8);
  negobuf[4] = 0x5a;
  negobuf[5] = 0x8f;
  negobuf[6] = 0x98;
  negobuf[7] = 0x04;

  short a = checksum1(negobuf, 8);
  short b = htons(a);
  printf("%04x", a);
  printf("%04x", htons(a));
  
  return;
}
