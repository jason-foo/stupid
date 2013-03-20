/* in_checksum is from UNP */

#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

#include "../include/common.h"
#include "sock.h"
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
		printf("%02x%02x ", data[i], data[i+1]);
	}
	if (i != len)
		printf("%02x", data[len - 1]);
	printf("\n");
}

struct timeval tv;
int get_second()
{
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;
	int ret;

	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;

	ret = fcntl(fd, cmd, &lock);
	if (ret < 0)
	{
		perror("lock_reg: fcntl error");
		exit(1);
	}
	return ret;
}

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;

	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;

	if (fcntl(fd, F_GETLK, &lock) < 0)
	{
		perror("lock_test: fcntl error");
		exit(1);
	}

	if (lock.l_type == F_UNLCK)
		return 0;
	return lock.l_pid;
}

void utils_init()
{
	srand(get_second());
}
