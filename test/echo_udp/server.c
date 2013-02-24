/* from <<UNP>> */

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#define MAXLINE 4096

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int n;
	socklen_t len;
	char mesg[MAXLINE];

	for (;;)
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		sendto(sockfd, mesg, n, 0, pcliaddr, len);
	}
}

int main()
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("sockfd");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(7);

	if ( bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(1);
	}

	dg_echo(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr));

	return 0;
}
