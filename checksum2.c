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

  struct iphdr *ipheader = (struct iphdr *)buffer;
  struct tcphdr *tcpheader = (struct tcphdr *)(buffer + sizeof(struct iphdr));
  struct pseudohdr pheader;

  // Initialize set zero
  memset(buffer, 0x00, sizeof(struct iphdr) + sizeof(struct tcphdr));
  
  // set IP header
  ipheader->version    = 4;
  ipheader->ihl        = sizeof(struct iphdr) >> 2;
  ipheader->tot_len    = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
  ipheader->ttl        = 255;
  ipheader->protocol   = IPPROTO_TCP;
  ipheader->saddr      = inet_addr("143.248.48.110");
  ipheader->daddr      = inet_addr("143.248.229.147");

  // set TCP header
  tcpheader->source = htons(10341);
  tcpheader->dest = htons(51918);
  //tcpheader->seq       = htonl(0xEFC);
  //tcpheader->syn       = 1;
  //tcpheader->doff      = sizeof(struct tcphdr) >> 2;
  //  tcpheader->window    = htons(2048);

  // set pseudo header
  pheader.saddr.s_addr = inet_addr("143.248.48.110");
  pheader.daddr.s_addr = inet_addr("143.248.229.147");
  pheader.protocol     = IPPROTO_TCP;
  pheader.length       = htons(sizeof(struct tcphdr));
  
  memcpy(&pheader.tcpheader, tcpheader, sizeof(struct tcphdr));
  
  short a = checksum((unsigned short *)&pheader, sizeof(struct pseudohdr) / sizeof(unsigned short));

  printf("%02x\n", a);
  printf("%02x\n", htons(a));

  short negobuf[4];
  negobuf[2] = 0x5a8f;
  negobuf[3] = 0x9804;

  short b = checksum(negobuf, 4);

  printf("%02x\n", b);
  printf("%02x\n", htons(b));
  
  return;
}
