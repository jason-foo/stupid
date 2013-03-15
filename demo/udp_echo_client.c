#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "stupid.h"

char dst[25] = "10.82.25.138";
#define ECHO_PORT 7

char content[][80] = {"hello kitty", "what the"};

void error_sys(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void test_app()
{
	int sfd = 0;
	char recvbuff[4096];
	int len;
	int i;
	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(ECHO_PORT);
	inet_pton(AF_INET, dst, &servaddr.sin_addr);

	if ( (sfd = stupid_socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		error_sys("error socket");

	for (i = 0; i < 2; i++)
	{
		stupid_sendto(sfd, content[i], strlen(content[i]), &servaddr,
			      sizeof(servaddr));
		len = stupid_recvfrom(sfd, recvbuff, 4096, &servaddr, sizeof(servaddr));
		recvbuff[len] = 0;
		printf("%s\n", recvbuff);
	}

	sfd_release(sfd);
}

int main()
{
	test_app();
	return 0;
}
