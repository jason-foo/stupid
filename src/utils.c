/* in_checksum is from UNP */

#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

void error_msg_and_die(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

__u16 in_checksum(__u16 *addr, int len)
{
	int nleft = len;
	__u32 sum = 0;
	__u16 *w = addr;
	__u32 answer = 0;

	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
	{
		*(__u8 *) (&answer) = *(__u8 *) w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}

char *c_ntoa(__u32 addr)
{
	struct in_addr in_addr;
	in_addr.s_addr = addr;
	return inet_ntoa(in_addr);
}

void data_dump(char *des, unsigned char *data, int len)
{
	int i;
	if (len > 64)
		len = 64;

	printf("%s: %d bytes", des, len);
	for (i = 0; i < (len / 2 * 2);i += 2)
	{
		if (i % 16 == 0)
			printf("\n         ");
		printf("%02x %02x  ", data[i], data[i+1]);
	}
	if (i != len)
		printf("%02x", data[len - 1]);
	printf("\n");
}
