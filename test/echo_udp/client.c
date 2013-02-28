#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096

void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t
servlen)
{
	int n;
	char sendline[MAXLINE], recvline[MAXLINE + 1];

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

		n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);

		recvline[n] = 0;
		fputs(recvline, stdout);
	}
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if (argc != 2)
	{
		puts("usage: udpcli <IPaddress>");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("sockfd");
		exit(1);
	}

	dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	return 0;
}
