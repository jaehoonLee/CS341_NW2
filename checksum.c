#include <stdio.h>
#include <memory.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>


uint16_t tcp_checksum(const void *buff, size_t len, in_addr_t src_addr, in_addr_t dest_addr)
{
  const uint16_t * buf = buff;
  uint16_t *ip_src = (void *)&src_addr, *ip_dst = (void *)&dest_addr;
  uint32_t sum;
  size_t length = len; 

  // calculate sum 
  sum = 0;
  while(len > 1){
    sum += *buf++;
    if(sum & 0x80000000){
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
    len -= 2;
  }

  if(len & 1){
    sum += *((uint8_t *)buf);
  }

  // Add the pseudo-header
  sum += *(ip_src++);
  sum += *ip_src;
  sum += *(ip_dst++);
  sum += *ip_dst;
  sum += htons(IPPROTO_TCP);
  sum += htons(length);

	printf("%d", sum);

  // Add the carries                                              
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  // Return the one's complement of sum                           
  return ( (uint16_t)(~sum) );
}

int main()
{
  unsigned char negobuf[8];
  negobuf[4] = 0x5a;
  negobuf[5] = 0x8f;
  negobuf[6] = 0x98;
  negobuf[7] = 0x04;

  int i = 0;
  for(i = 0; i < 8; i++)
    printf("%02x ", (unsigned char)negobuf[i]);
  printf("\n");
  
  in_addr_t dest_addr  = inet_addr("143.248.229.32");
  in_addr_t srd_addr = inet_addr("143.248.48.110"); 

  uint16_t val = tcp_checksum(negobuf, sizeof(negobuf), srd_addr, dest_addr);

  printf("hello:%04x\n", val);
  printf("hello:%04x\n", htonl(val));

  return 0;
}
