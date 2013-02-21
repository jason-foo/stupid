#include <stdio.h>
#include <stdlib.h>
#include <linux/if_ether.h>

#include "skbuff.h"

struct sk_buff *buf;

int main()
{
	buf = (struct sk_buff *)malloc(sizeof(struct sk_buff));
	buf->data = (unsigned char *)malloc(1600 * sizeof(char));
	printf("%s\n", "please input the packet data");
	scanf("%s", buf->data);
	printf("%s\n", buf->data);
	return 0;
}
