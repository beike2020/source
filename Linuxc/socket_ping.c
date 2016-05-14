/******************************************************************************
 * Function: 	Socket ping information.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall socket_ping.c -lpthread -o socket_ping
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define  PACKET_SIZE 	 409600
#define  MAX_WAIT_TIME 	 10
#define  MAX_NO_PACKETS  3

pid_t pid;
struct sockaddr_in dest_addr;
struct timeval start, end, tvrecv;
pthread_t send_thread, recv_thread;
float minttl = 0, maxttl = 0, totttl = 0;
char sendpacket[PACKET_SIZE], recvpacket[PACKET_SIZE];
int sockfd, startflag = 1, datalen = 56, nsend = 0, nreceived = 0;

void check_ping_ttl(struct timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}

	out->tv_sec -= in->tv_sec;
}

unsigned short check_ping_sum(unsigned short *addr, int len)
{
	int sum = 0, nleft = len;
	unsigned short *w = addr;
	unsigned short answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return answer;
}

int encap_ping_packet(int pack_no)
{
	int packsize;
	struct icmp *icmp;
	struct timeval *tval;

	icmp = (struct icmp *)sendpacket;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = pack_no;
	icmp->icmp_id = pid;
	packsize = 8 + datalen;
	tval = (struct timeval *)icmp->icmp_data;
	gettimeofday(tval, NULL);
	icmp->icmp_cksum = check_ping_sum((unsigned short *)icmp, packsize);

	return packsize;
}

void *send_packet(void *flag)
{
	int packetsize;

	printf("PING %s %d(%d) bytes of data.\n", inet_ntoa(dest_addr.sin_addr),
	       datalen, datalen + 28);

	while (startflag) {
		packetsize = encap_ping_packet(nsend);
		if (sendto(sockfd, sendpacket, packetsize, 0,
			   (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
			perror("sendto");
			continue;
		}
		++nsend;
		sleep(1);
	}

	return NULL;
}

int uncap_ping_packet(char *buf, int len, int loop)
{
	int iphdrlen;
	double rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;
	icmp = (struct icmp *)(buf + iphdrlen);
	len -= iphdrlen;

	if (len < 8) {
		printf("ICMP packets's length is less than 8\n");
		exit(EXIT_FAILURE);
	}

	if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid)) {
		tvsend = (struct timeval *)icmp->icmp_data;
		check_ping_ttl(&tvrecv, tvsend);
		rtt = tvrecv.tv_sec * 1000 + tvrecv.tv_usec / 1000.0;

		if (loop == 1) {
			maxttl = minttl = rtt;
		} else {
			if (rtt > maxttl)
				maxttl = rtt;
			if (rtt < minttl)
				minttl = rtt;
		}
		totttl += rtt;

		printf("%d byte from %s: icmp_seq = %u ttl = %d rtt = %.3f ms\n",
		       len, inet_ntoa(dest_addr.sin_addr), icmp->icmp_seq,
		       ip->ip_ttl, rtt);
	} else {
		exit(EXIT_FAILURE);
	}

	return 0;
}

void *recv_packet(void *flag)
{
	int n;
	extern int errno;
	socklen_t destlen;

	destlen = sizeof(dest_addr);
	while (startflag) {
		if ((n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0,
				  (struct sockaddr *)&dest_addr, &destlen)) < 0) {
			perror("recvfrom");
			continue;
		}

		nreceived++;
		gettimeofday(&tvrecv, NULL);
		if (uncap_ping_packet(recvpacket, n, nreceived) == -1)
			continue;
	}

	return NULL;
}

void stop_ping_packet(int signal)
{
	startflag = 0;
}

void static_ping_result()
{
	double out;

	check_ping_ttl(&end, &start);
	out = end.tv_sec * 1000 + end.tv_usec;
	printf("\n--- %s ping statistics ---\n", inet_ntoa(dest_addr.sin_addr));
	printf("%d packets transmitted, %d received, %d packet lost, time %.0fms\n",
	       nsend, nreceived, nsend - nreceived, out);
	printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n",
	       minttl, totttl / nreceived, maxttl);
}

int main(int argc, char *argv[])
{
	int size = 50 * 1024;
	void *thread_result;
	struct sigaction act;
	struct protoent *protocol;

	pid = getpid();
	setuid(getuid());
	gettimeofday(&start, NULL);

	act.sa_handler = stop_ping_packet;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);

	if (argc < 2) {
		printf("usage:%s IP\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if ((protocol = getprotobyname("icmp")) == NULL) {
		perror("getprotobyname");
		exit(EXIT_FAILURE);
	}

	if ((sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	bzero(&dest_addr, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	if (inet_aton(argv[1], (struct in_addr *)&dest_addr.sin_addr.s_addr) == 0) {
		perror(argv[1]);
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&(send_thread), NULL, send_packet, (void *)startflag) != 0) {
		perror("pthread_create send_thread");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&(recv_thread), NULL, recv_packet, (void *)startflag) != 0) {
		perror("pthread_create recv_thread");
		exit(EXIT_FAILURE);
	}

	pthread_join(send_thread, &thread_result);
	//pthread_join(recv_thread, &thread_result);    
	gettimeofday(&end, NULL);
	static_ping_result();
	close(sockfd);

	return 0;
}
