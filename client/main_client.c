#include "common.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"main_client:"


#undef MAXLINE
#define MAXLINE   1024


void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int		n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];

	connect(sockfd, (struct sockaddr *) pservaddr, servlen);  /*!!!!!!!!!!!!*/

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		write(sockfd, sendline, strlen(sendline));

		n = read(sockfd, recvline, MAXLINE);

		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}
}




int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		dbg_printf("usage: udpcli <IPaddress>\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(45620);
	udp_sock_pton(argv[1],(struct  sockaddr *)&servaddr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	/*客户端也要对端口进行绑定，不然系统会分配一个随机的端口*/
	struct sockaddr_in	clisent_addr;
	bzero(&clisent_addr, sizeof(clisent_addr));
	clisent_addr.sin_family = AF_INET;
	clisent_addr.sin_port = htons(45625);
	bind(sockfd, (struct sockaddr *) &clisent_addr, sizeof(clisent_addr));

	dg_cli(stdin, sockfd, (struct  sockaddr *) &servaddr, sizeof(servaddr));

	exit(0);
}