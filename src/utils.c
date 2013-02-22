#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>

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
