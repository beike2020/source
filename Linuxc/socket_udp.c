/******************************************************************************
 * Function: 	Socket udp communication.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall socket_udp.c -o socket_udp
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <resolv.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: creat a time application by udp protocol\n");
	printf(" 2: creat a client by udp protocol\n");
	printf(" 3: creat a server by udp protocol\n");
	printf(" 4: creat a broadcast client by udp protocol\n");
	printf(" 5: creat a broadcast server by udp protocol\n");
	printf(" 6: creat a groupcast client by udp protocol\n");
	printf(" 7: creat a groupcast server by udp protocol\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int udp_point_app()
{
	char buf[128];
	socklen_t len;
	int sockfd, ret;
	struct hostent *hostinfo;
	struct servent *servinfo;
	struct sockaddr_in saddr;

	hostinfo = gethostbyname("localhost");
	if (hostinfo == NULL) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	servinfo = getservbyname("daytime", "udp");
	if (servinfo == NULL) {
		perror("getservbyname");
		exit(EXIT_FAILURE);
	}
	printf("daytime port is %d\n", ntohs(servinfo->s_port));

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = servinfo->s_port;
	saddr.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;
	ret = sendto(sockfd, buf, 1, 0, (struct sockaddr *)&saddr, len);
	if (ret < 0) {
		perror("sendto");
		exit(EXIT_FAILURE);
	}

	memset(buf, 0, 128);
	ret = recvfrom(sockfd, buf, 128, 0, (struct sockaddr *)&saddr, &len);
	if (ret < 0) {
		perror("recvfrom");
		exit(EXIT_FAILURE);
	}
	printf("read %d bytes: %s", ret, buf);

	close(sockfd);

	return 0;
}

int udp_point_client()
{
	int sockfd;
	char buf[128];
	socklen_t len;
	struct sockaddr_in saddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(buf, 0, 128);
	strcpy(buf, "hello beike!");
	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (sendto(sockfd, buf, 128, 0, (struct sockaddr *)&saddr, len) < 0) {
		perror("sendto");
		exit(EXIT_FAILURE);
	}

	printf("sendto success.\n");

	return 0;
}

int udp_point_server()
{
	char buf[128];
	socklen_t len;
	int sockfd, ret;
	struct sockaddr_in saddr, caddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if ((bind(sockfd, (struct sockaddr *)&saddr, len)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	while (1) {
		printf("wait for new connect....\n");

		memset(buf, 0, 128);
		ret = recvfrom(sockfd, buf, 127, 0, (struct sockaddr *)&caddr, &len);
		if (ret < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		printf("recive come from %s port %d\n\t",
		       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
		printf("message: %s\n", buf);
	}

	close(sockfd);

	return 0;
}

int udp_broadcast_client()
{
	char buf[128];
	socklen_t len;
	int sockfd, on = 1;
	struct sockaddr_in saddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(on);
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, len) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	memset(buf, 0, 128);
	strcpy(buf, "hello beike!");
	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = inet_addr("192.168.94.255");
	if (sendto(sockfd, buf, 128, 0, (struct sockaddr *)&saddr, len) < 0) {
		perror("sendto");
		exit(EXIT_FAILURE);
	}

	printf("send success\n");

	return 0;
}

int udp_broadcast_server()
{
	char buf[128];
	socklen_t len;
	int sockfd, ret;
	struct sockaddr_in saddr, caddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if ((bind(sockfd, (struct sockaddr *)&saddr, len)) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	while (1) {
		printf("wait for new connect....\n");

		memset(buf, 0, 128);
		ret = recvfrom(sockfd, buf, 127, 0, (struct sockaddr *)&caddr, &len);
		if (ret < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		printf("recive come from %s port %d\n\t",
		       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
		printf("message: %s\n", buf);
	}

	close(sockfd);

	return 0;
}

int udp_groupcast_client()
{
	int sockfd;
	char buf[128];
	socklen_t len;
	struct sockaddr_in saddr, caddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(7838);
	if (inet_pton(AF_INET, "230.1.1.1", &saddr.sin_addr) <= 0) {
		perror("inet_pton");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&caddr, sizeof(caddr));
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(23456);
	caddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&caddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	while (1) {
		memset(buf, 0, 128);
		printf("input message to send: ");
		if (fgets(buf, 255, stdin) == (char *)EOF)
			exit(EXIT_FAILURE);

		if (sendto(sockfd, buf, 128, 0, (struct sockaddr *)&saddr, len) < 0) {
			perror("sendto");
			exit(EXIT_FAILURE);;
		}

		printf("\tsend message: %s\n", buf);
	}

	close(sockfd);

	return 0;
}

int udp_groupcast_server()
{
	char buf[128];
	socklen_t len;
	int sockfd, ret;
	char bcastip[128];
	struct in_addr ia;
	struct ip_mreq mreq;
	struct hostent *group;
	struct sockaddr_in saddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket");
		exit(EXIT_FAILURE);
	}

	memset(bcastip, 0, 128);
	strcpy(bcastip, "230.1.1.1");
	if ((group = gethostbyname(bcastip)) == (struct hostent *)0) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct ip_mreq);
	bzero(&mreq, sizeof(struct ip_mreq));
	bcopy((void *)group->h_addr, (void *)&ia, group->h_length);
	bcopy(&ia, &mreq.imr_multiaddr.s_addr, sizeof(struct in_addr));
	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, len) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(7838);
	if (inet_pton(AF_INET, bcastip, &saddr.sin_addr) <= 0) {
		perror("inet_pton");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	while (1) {
		memset(buf, 0, 128);
		ret = recvfrom(sockfd, buf, 127, 0, (struct sockaddr *)&saddr, &len);
		if (ret < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		} else {
			printf("recive come from %s port %d\n\t",
			       inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
			printf("message: %s\n", buf);
		}
	}

	close(sockfd);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//creat a time application by udp protocol
	case 1:
		udp_point_app();
		break;

		//creat a client by udp protocol
	case 2:
		udp_point_client();
		break;

		//creat a server by udp protocol
	case 3:
		udp_point_server();
		break;

		//creat a broadcast client by udp protocol
	case 4:
		udp_broadcast_client();
		break;

		//creat a broadcast server by udp protocol
	case 5:
		udp_broadcast_server();
		break;

		//creat a groupcast client by udp protocol
	case 6:
		udp_groupcast_client();
		break;

		//creat a groupcast server by udp protocol
	case 7:
		udp_groupcast_server();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
