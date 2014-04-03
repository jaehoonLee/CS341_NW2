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

int main()
{
  char buffer[1024];

  struct tcphdr tcpheader;
  struct pseudohdr pheader;

  pheader.saddr.s_addr = inet_addr("143.248.48.110");
  pheader.daddr.s_addr = inet_addr("143.248.229.147");
  pheader.protocol     = IPPROTO_TCP;
  pheader.length       = htons(sizeof(struct tcphdr)+8);
  
  tcpheader.source = htons(23000);
  tcpheader.dest = htons(80);
  

  //  memcpy(&pheader.tcpheader, tcpheader, sizeof(struct tcphdr));
  
  short a = checksum((unsigned short *)&pheader, sizeof(struct pseudohdr) / sizeof(unsigned short));

  printf("%02x\n", a);
  printf("%02x", htons(a));
  return;
}
