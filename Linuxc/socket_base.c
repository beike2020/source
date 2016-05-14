/******************************************************************************
 * Function: 	Socket base communication.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall socket_base.c -o socket_base
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <resolv.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/if_ether.h>

#define  FILENAME  "/tmp/test_lock"
#define  BUFLINE   1024

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: get the localhost information\n");
	printf(" 2: get a peer host information\n");
	printf(" 3: get the socket options information\n");
	printf(" 4: creat a unix socket pipe\n");
	printf(" 5: creat a client by unix protocol\n");
	printf(" 6: creat a server by unix protocol\n");
	printf(" 7: chat between procs by unix protocol\n");
	printf(" 8: send pakage by raw protocol\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int get_localhost_information()
{
	int i, sock;
	struct ifreq ifreq;
	struct hostent *hostinfo;
	char **names, **addrs, mac[32];

	hostinfo = gethostbyname("localhost");
	if (hostinfo == NULL) {
		perror("localhost");
		exit(EXIT_FAILURE);
	}
	printf("results for host %s:\n", "localhost");
	printf("Name: %s\n", hostinfo->h_name);
	printf("Aliases:");
	names = hostinfo->h_aliases;
	while (*names) {
		printf(" %s", *names++);
		names++;
	}
	printf("\n");

	if (hostinfo->h_addrtype != AF_INET)
		exit(EXIT_FAILURE);
	printf("Address:");
	addrs = hostinfo->h_addr_list;
	while (*addrs) {
		printf(" %s", inet_ntoa(*(struct in_addr *)*addrs));
		addrs++;
	}
	printf("\n");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit(EXIT_FAILURE);

	strcpy(ifreq.ifr_name, "eth2");
	if (ioctl(sock, SIOCGIFADDR, &ifreq) < 0) {
		perror("SIOCGIFADDR");
		exit(EXIT_FAILURE);
	}
	printf("eth0 ip is %s\n",
	       inet_ntoa(((struct sockaddr_in *)&(ifreq.ifr_addr))->sin_addr));

	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
		exit(EXIT_FAILURE);
	for (i = 0; i < 6; i++)
		sprintf(mac + 3 * i, "%02x:",
			(unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
	mac[17] = '\0';
	printf("eth0 mac is %s\n", mac);

	if (ioctl(sock, SIOCGIFFLAGS, &ifreq) < 0)
		exit(EXIT_FAILURE);
	printf("init: eth0 debug is %s\n",
	       ifreq.ifr_flags & IFF_DEBUG ? "on" : "off");
	ifreq.ifr_flags |= IFF_DEBUG;
	if (ioctl(sock, SIOCSIFFLAGS, &ifreq) < 0)
		exit(EXIT_FAILURE);
	printf("now: eth0 debug is %s\n",
	       ifreq.ifr_flags & IFF_DEBUG ? "on" : "off");

	return 0;
}

int get_peerhost_information()
{
	int rc;
	char ipbuf[16] = "";
	char peername[32] = "";
	char peerproto[16] = "";
	struct addrinfo hints, *addr;

	strcpy(peername, "www.baidu.com");
	strcpy(peerproto, "http");
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_CANONNAME | AI_ADDRCONFIG;
	if ((rc = getaddrinfo(peername, peerproto, &hints, &addr)))
		return -1;

	do {
		printf("addr: %s\t", inet_ntop(AF_INET,
					       &(((struct sockaddr_in *)addr->
						  ai_addr)->sin_addr), ipbuf,
					       sizeof(ipbuf)));
		printf("host: %s\t", addr->ai_canonname);
		printf("lens: %d\t", addr->ai_addrlen);
		printf("port: %d\n",
		       ntohs(((struct sockaddr_in *)addr->ai_addr)->sin_port));
	} while ((addr = addr->ai_next) != NULL);

	return 0;
}

int internet_socket_information()
{
	socklen_t size;
	off_t offset = 0;
	struct stat st;
	struct tcp_info ti;
	struct timeval set_time, ret_time;
	int fd, type = 0, ttl = 0, maxseg = 0;
	int sock_fd, rcvbuf_size, sndbuf_size;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	size = sizeof(sndbuf_size);
	getsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, &size);
	printf("default: SO_SNDBUF %d, size %d\n", sndbuf_size, size);

	size = sizeof(rcvbuf_size);
	getsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, &size);
	printf("default: SO_RCVBUF %d, size %d\n", rcvbuf_size, size);

	size = sizeof(type);
	getsockopt(sock_fd, SOL_SOCKET, SO_TYPE, &type, &size);
	printf("default: SO_TYPE %d\n", type);

	size = sizeof(struct timeval);
	getsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &ret_time, &size);
	printf("default: SO_SNDTIMEO %lds %ldns\n", ret_time.tv_sec,
	       ret_time.tv_usec);

	set_time.tv_sec = 10;
	set_time.tv_usec = 100;
	setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &set_time, size);
	getsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &ret_time, &size);
	printf("modified: SO_SNDTIMEO %lds %ldns\n", ret_time.tv_sec,
	       ret_time.tv_usec);

	size = sizeof(type);
	getsockopt(sock_fd, SOL_SOCKET, TCP_NODELAY, &type, &size);
	printf("default: TCP_NODELAY option %d\n", type);

	size = sizeof(type);
	getsockopt(sock_fd, SOL_SOCKET, TCP_CORK, &type, &size);
	printf("default: TCP_CORK option %d\n", type);

	size = sizeof(type);
	getsockopt(sock_fd, SOL_SOCKET, TCP_DEFER_ACCEPT, &type, &size);
	printf("default: TCP_DEFER_ACCEPT option %d\n", type);

	size = sizeof(type);
	getsockopt(sock_fd, SOL_SOCKET, TCP_QUICKACK, &type, &size);
	printf("default: TCP_QUICKACK option %d\n", type);

	size = sizeof(ttl);
	getsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, &size);
	printf("default: IP_TTL %d\n", ttl);

	size = sizeof(maxseg);
	getsockopt(sock_fd, IPPROTO_TCP, TCP_MAXSEG, &maxseg, &size);
	printf("default: TCP_MAXSEG %d\n", maxseg);

	size = sizeof(struct tcp_info);
	getsockopt(sock_fd, IPPROTO_TCP, TCP_INFO, &ti, &size);
	printf("tcp info detail: tcpi_state \t\t%u\n", ti.tcpi_state);
	printf("tcp info detail: tcpi_ca_state \t\t%u\n", ti.tcpi_ca_state);
	printf("tcp info detail: tcpi_retransmits \t%u\n", ti.tcpi_retransmits);
	printf("tcp info detail: tcpi_probes \t\t%u\n", ti.tcpi_probes);
	printf("tcp info detail: tcpi_backoff \t\t%u\n", ti.tcpi_backoff);
	printf("tcp info detail: tcpi_options \t\t%u\n", ti.tcpi_options);
	printf("tcp info detail: tcpi_snd_wscale \t%u\n", ti.tcpi_snd_wscale);
	printf("tcp info detail: tcpi_rcv_wscale \t%u\n", ti.tcpi_rcv_wscale);
	printf("tcp info detail: tcpi_rto \t\t%d\n", ti.tcpi_rto);
	printf("tcp info detail: tcpi_ato \t\t%d\n", ti.tcpi_ato);
	printf("tcp info detail: tcpi_snd_mss \t\t%d\n", ti.tcpi_snd_mss);
	printf("tcp info detail: tcpi_rcv_mss \t\t%d\n", ti.tcpi_rcv_mss);
	printf("tcp info detail: tcpi_unacked \t\t%d\n", ti.tcpi_unacked);
	printf("tcp info detail: tcpi_sacked \t\t%d\n", ti.tcpi_sacked);
	printf("tcp info detail: tcpi_lost \t\t%d\n", ti.tcpi_lost);
	printf("tcp info detail: tcpi_retrans \t\t%d\n", ti.tcpi_retrans);
	printf("tcp info detail: tcpi_fackets \t\t%d\n", ti.tcpi_fackets);
	printf("tcp info detail: tcpi_last_data_sent \t%d\n",
	       ti.tcpi_last_data_sent);
	printf("tcp info detail: tcpi_retransmits \t%d\n", ti.tcpi_retransmits);
	printf("tcp info detail: tcpi_last_ack_sent \t%d\n", ti.tcpi_last_ack_sent);
	printf("tcp info detail: tcpi_last_data_recv \t%d\n",
	       ti.tcpi_last_data_recv);
	printf("tcp info detail: tcpi_last_ack_recv \t%d\n", ti.tcpi_last_ack_recv);
	printf("tcp info detail: tcpi_pmtu \t\t%d\n", ti.tcpi_pmtu);
	printf("tcp info detail: tcpi_rcv_ssthresh \t%d\n", ti.tcpi_rcv_ssthresh);
	printf("tcp info detail: tcpi_rtt \t\t%d\n", ti.tcpi_rtt);
	printf("tcp info detail: tcpi_rttvar \t\t%d\n", ti.tcpi_rttvar);
	printf("tcp info detail: tcpi_snd_ssthresh \t%d\n", ti.tcpi_snd_ssthresh);
	printf("tcp info detail: tcpi_snd_cwnd \t\t%d\n", ti.tcpi_snd_cwnd);
	printf("tcp info detail: tcpi_advmss \t\t%d\n", ti.tcpi_advmss);
	printf("tcp info detail: tcpi_reordering \t%d\n", ti.tcpi_reordering);

	stat(FILENAME, &st);
	fd = open(FILENAME, O_RDONLY);
	sendfile(sock_fd, fd, &offset, st.st_size);
	close(sock_fd);

	return 0;
}

int unix_socket_pipe()
{
	int nb, on, fd[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
		perror("socketpair");
		return -1;
	}

	nb = 0;
	if ((ioctl(fd[0], FIONBIO, &nb) == -1) || (ioctl(fd[1], FIONBIO, &nb) == -1)) {
		close(fd[0]);
		close(fd[1]);
		perror("ioctl FIONBIO");
		return -1;
	}

	on = 0;
	if (ioctl(fd[0], FIOASYNC, &on) == -1) {
		close(fd[0]);
		close(fd[1]);
		perror("ioctl FIOASYNC");
		return -1;
	}

	if (fcntl(fd[0], F_SETOWN, getpid()) == -1) {
		close(fd[0]);
		close(fd[1]);
		perror("fcntl F_SETOWN");
		return -1;
	}

	if ((fcntl(fd[0], F_SETFD, FD_CLOEXEC) == -1) ||
	    (fcntl(fd[1], F_SETFD, FD_CLOEXEC) == -1)) {
		close(fd[0]);
		close(fd[1]);
		perror("fcntl FD_CLOEXEC");
		return -1;
	}

	if (fork()) {
		int val = 0;
		close(fd[1]);

		while (1) {
			sleep(1);
			++val;
			printf("Parent sending data: %d\n", val);
			write(fd[0], &val, sizeof(val));
			read(fd[0], &val, sizeof(val));
			printf("Parent received data: %d\n\n", val);
		}
	} else {
		int val;
		close(fd[0]);

		while (1) {
			read(fd[1], &val, sizeof(val));
			printf("Children received data: %d\n", val);
			++val;
			printf("Children sending data: %d\n", val);
			write(fd[1], &val, sizeof(val));
		}
	}

	return 0;
}

int unix_point_client()
{
	int i, sockfd;
	char ch_recv, ch_send = 'A';
	struct sockaddr_un address;

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "server_socket");
	if (connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < 5; i++) {
		if (write(sockfd, &ch_send, 1) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		if (read(sockfd, &ch_recv, 1) == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		ch_send++;
		printf("receive from server data is %c\n", ch_recv);
	}

	printf("server close connect!\n");
	close(sockfd);

	return 0;
}

int unix_point_server()
{
	char ch_recv, ch_send = 'B';
	int i, server_len, client_len;
	int server_sockfd, client_sockfd;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;

	unlink("server_socket");

	if ((server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&server_address, 0, sizeof(struct sockaddr_un));
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, "server_socket");
	server_len = sizeof(server_address);
	if (bind(server_sockfd, (struct sockaddr *)&server_address, server_len)
	    == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server_sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("wait for client connect...\n");
	client_len = sizeof(client_address);
	if ((client_sockfd =
	     accept(server_sockfd, (struct sockaddr *)&client_address,
		    (socklen_t *) & client_len)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	printf("wait for client data...\n");
	for (i = 0; i < 5; i++) {
		if (read(client_sockfd, &ch_recv, 1) == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		printf("receive from client data is %c\n", ch_recv);
		sleep(1);

		if (write(client_sockfd, &ch_send, 1) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}

		ch_send++;
	}

	printf("server close connection!\n");
	close(client_sockfd);
	unlink("server socket");

	return 0;
}

int recv_proc_msg(int fd, void *data, size_t len, int *recv_fd)
{
	size_t n;
	struct msghdr msg;
	struct iovec iov[1];
	struct cmsghdr *cmsg = NULL;
	union {
		struct cmsghdr cm;
		char ctl[CMSG_SPACE(sizeof(int))];
	} ctl_un;

	iov[0].iov_base = data;
	iov[0].iov_len = len;
	msg.msg_control = ctl_un.ctl;
	msg.msg_controllen = sizeof(ctl_un.ctl);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ((n = recvmsg(fd, &msg, 0)) < 0) {
		perror("recvmsg");
		exit(EXIT_FAILURE);
	}

	*recv_fd = -1;
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
	    if (cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
			if (cmsg->cmsg_level == SOL_SOCKET) {
				if (cmsg->cmsg_type == SCM_RIGHTS) 
					*recv_fd = *((int *)CMSG_DATA(cmsg));
			}
		} 
	}

	return n;
}

int get_recv_fd(char *path_name, int mode)
{
	pid_t chi_pid;
	int fd, status, sockfd[2];
	char c, arg_fd[10], arg_mode[10];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) == -1) {
		perror("socketpair");
		exit(EXIT_FAILURE);
	}

	if ((chi_pid = fork()) == 0) {
		close(sockfd[0]);
		system("gcc upper.c -o upper");
		snprintf(arg_fd, sizeof(arg_fd), "%d", sockfd[1]);
		snprintf(arg_mode, sizeof(arg_mode), "%d", mode);
		execl("./upper", "upper", arg_fd, path_name, arg_mode, (char *)NULL);
		perror("Stream");
		exit(EXIT_FAILURE);
	}

	close(sockfd[1]);

	waitpid(chi_pid, &status, 0);
	if (WIFEXITED(status) != 0) {
		if ((status = WEXITSTATUS(status)) == 0) {
			recv_proc_msg(sockfd[0], &c, 1, &fd);
		} else {
			errno = status;
			fd = -1;
		}
		close(sockfd[0]);
		return fd;
	}

	return -1;
}

int unix_proc_chat()
{
	int fd, n_read;
	char buf[BUFLINE];

	if ((fd = get_recv_fd("/tmp/file.in", O_RDONLY)) < 0)
		return -1;

	while ((n_read = read(fd, buf, BUFLINE)) > 0)
		write(1, buf, n_read);

	return 0;
}

int raw_sum_check(unsigned short *addr, int len)
{
	int answer = 0;
	register int nleft = len;
	register int sum = 0;
	register unsigned short *w = addr;

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

	return (answer);
}

void raw_protocol_init(int sockfd, struct sockaddr_in *daddr)
{
	int head_len;
	char buffer[100];
	struct iphdr *ip;
	struct tcphdr *tcp;

	head_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
	bzero(buffer, 100);
	ip = (struct iphdr *)buffer;
	ip->version = IPVERSION;
	ip->ihl = sizeof(struct ip) >> 2;
	ip->tos = 0;
	ip->tot_len = htons(head_len);
	ip->id = 0;
	ip->frag_off = 0;
	ip->ttl = MAXTTL;
	ip->protocol = IPPROTO_TCP;
	ip->check = 0;
	ip->daddr = daddr->sin_addr.s_addr;

	tcp = (struct tcphdr *)(buffer + sizeof(struct ip));
	tcp->source = htons(9000);
	tcp->dest = daddr->sin_port;
	tcp->seq = random();
	tcp->ack_seq = 0;
	tcp->doff = 5;
	tcp->syn = 1;
	tcp->check = 0;

	while (1) {
		ip->saddr = random();
		tcp->check = 0;
		tcp->check =
		    raw_sum_check((unsigned short *)tcp, sizeof(struct tcphdr));
		sendto(sockfd, buffer, head_len, 0, (struct sockaddr *)daddr,
		       (socklen_t) sizeof(struct sockaddr_in));
	}
}

int raw_tcpip_send()
{
	int sockfd, on = 1;
	struct sockaddr_in daddr;

	memset(&daddr, 0, sizeof(struct sockaddr_in));
	daddr.sin_family = AF_INET;
	daddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	daddr.sin_port = htons(9001);

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	raw_protocol_init(sockfd, &daddr);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//get the localhost information.
	case 1:
		get_localhost_information();
		break;

		//get a peer host information.
	case 2:
		get_peerhost_information();
		break;

		//get the socket options information.
	case 3:
		internet_socket_information();
		break;

		//creat a unix socket pipe.
	case 4:
		unix_socket_pipe();
		break;

		//creat a client by unix protocol
	case 5:
		unix_point_client();
		break;

		//creat a server by unix protocol
	case 6:
		unix_point_server();
		break;

		//chat between procs by unix protocol
	case 7:
		unix_proc_chat();
		break;

		//send pakage by raw protocol
	case 8:
		raw_tcpip_send();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
