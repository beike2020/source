/******************************************************************************
 * Function: 	Socket tcp communication.
 * Author: 	forwarding2012@yahoo.com.cn							
 * Date: 		2012.01.01
 * Compile:	gcc -Wall socket_tcp.c -lpthread -o socket_tcp
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <resolv.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

struct sock_info {
	int connfd;
	struct sockaddr_in client;
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: creat a client connect\n");
	printf(" 2: creat a server accept single client\n");
	printf(" 3: creat a server accept muti clients by fork\n");
	printf(" 4: creat a server accept muti clients by muti_threads\n");
	printf(" 5: creat a server accept muti clients by select\n");
	printf(" 6: creat a server accept muti clients by poll\n");
	printf(" 7: creat a server accept muti clients by epoll\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int tcp_comm_client()
{
	char cbuf[1024];
	int sockfd, len;
	struct sockaddr_in caddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero(&caddr, sizeof(caddr));
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(9001);
	if (!inet_aton("127.0.0.1", (struct in_addr *)&caddr.sin_addr.s_addr)) {
		perror("127.0.0.1");
		exit(EXIT_FAILURE);
	}

	if (connect(sockfd, (struct sockaddr *)&caddr, sizeof(caddr)) == -1) {
		perror("connect ");
		exit(EXIT_FAILURE);
	}

	printf("\nserver connected\n\n");

	while (1) {
		memset(cbuf, 0, 1024);
		printf("input message to send: ");
		do {
			fgets(cbuf, 1023, stdin);
		} while (!strcmp(cbuf, "\n"));

		len = send(sockfd, cbuf, strlen(cbuf) - 1, 0);
		if (len > 0) {
			printf("send message: %s\n", cbuf);
			if (!strncasecmp(cbuf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(1);
				exit(EXIT_FAILURE);
			}
		} else if (len < 0) {
			perror("send");
		}

		memset(cbuf, 0, 1024);
		len = recv(sockfd, cbuf, 1023, 0);
		if (len > 0) {
			printf("receive message: %s\n\n", cbuf);
			if (!strncasecmp(cbuf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(3);
				exit(EXIT_FAILURE);
			}
		} else if (len < 0) {
			perror("recv");
		}
	}

	close(sockfd);

	return 0;
}

int tcp_point_server()
{
	socklen_t len;
	char sbuf[1024];
	int sockfd, newfd;
	struct sockaddr_in saddr, caddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("wait for new connect....\n");

	if ((newfd = accept(sockfd, (struct sockaddr *)&caddr, &len)) == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	printf("server: got connection from %s, port %d, socket %d\n",
	       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

	while (1) {
		memset(sbuf, 0, 1024);
		len = recv(newfd, sbuf, 1023, 0);
		if (len > 0) {
			printf("[%s:%d]: %s\n", inet_ntoa(caddr.sin_addr), 
				   ntohs(caddr.sin_port), sbuf);
			if (!strncasecmp(sbuf, "quit", 4)) {
				printf("close the connect [%s:%d]\n",
				       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
				sleep(1);
				close(newfd);
				exit(EXIT_FAILURE);
			}
		} else if (len < 0) {
			perror("recv");
		}

		memset(sbuf, 0, 1024);
		printf("input the message to send: ");
		do {
			fgets(sbuf, 1023, stdin);
		} while (!strcmp(sbuf, "\n"));

		len = send(newfd, sbuf, strlen(sbuf) - 1, 0);
		if (len > 0) {
			printf("send message: %s\n", sbuf);
			if (!strncasecmp(sbuf, "quit", 4)) {
				printf("close the connect [%s:%d]\n",
				       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
				sleep(3);
				close(newfd);
				exit(EXIT_FAILURE);
			}
		} else if (len < 0) {
			perror("send");
		}
	}

	close(newfd);
	close(sockfd);

	return 0;
}

int tcp_proc_server()
{
	socklen_t len;
	int sockfd, newfd, ret;
	struct sockaddr_in saddr, caddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(9001);
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN);

	while (1) {
		char rbuf[128], sbuf[128];

		printf("wait for new connect....\n");

		if ((newfd = accept(sockfd, (struct sockaddr *)&caddr, &len)) == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("server: got connection from %s, port %d, socket %d\n",
		       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

		if (fork() == 0) {
			printf("Fork PID %d accept require!\n", getppid());
			while (1) {
				memset(rbuf, 0, 128);
				ret = read(newfd, rbuf, 128);
				if (ret > 0) {
					printf("[%s:%d]: %s\n", inet_ntoa(caddr.sin_addr),
					       ntohs(caddr.sin_port), rbuf);
					if (!strncasecmp(rbuf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
						     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(1);
						close(newfd);
						exit(EXIT_FAILURE);
					}
				} else if (ret < 0) {
					perror("recv");
				}

				memset(sbuf, 0, 128);
				printf("input message to send: ");
				do {
					fgets(sbuf, 128, stdin);
				} while (!strcmp(sbuf, "\n"));

				ret = write(newfd, sbuf, sizeof(sbuf));
				if (ret > 0) {
					printf("send message: %s\n", sbuf);
					if (!strncasecmp(sbuf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
						     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(3);
						close(newfd);
						exit(EXIT_FAILURE);
					}
				} else if (ret < 0) {
					perror("send");
				}
			}
		} else {
			int stat_val;
			pid_t child_pid;
			child_pid = wait(&stat_val);
			printf("Fork PID %d finished cleanup!\n", child_pid);
			close(newfd);
		}
	}
}

void *sock_thread(void *args)
{
	int ret, i;
	char rbuf[1024], sbuf[1024];
	struct sock_info *info = (struct sock_info *)args;

	printf("\nserver: got connection from %s, port %d\n",
	       inet_ntoa(info->client.sin_addr), ntohs(info->client.sin_port));

	memset(rbuf, 0, 1024);
	memset(sbuf, 0, 1024);
	while ((ret = recv(info->connfd, rbuf, 1024, 0))) {
		rbuf[ret] = '\0';
		printf("[%s:%d]: %s\n", inet_ntoa(info->client.sin_addr),
		       ntohs(info->client.sin_port), rbuf);
		if (!strncasecmp(rbuf, "quit", 4)) {
			printf("close the connect [%s:%d]\n",
			       inet_ntoa(info->client.sin_addr),
			       ntohs(info->client.sin_port));
			sleep(1);
			close(info->connfd);
			free(info);
			pthread_exit(NULL);
		}

		for (i = 0; i < ret; i++)
			sbuf[i] = rbuf[ret - i - 1];

		sbuf[ret + 1] = '\0';
		send(info->connfd, sbuf, strlen(sbuf), 0);
	}

	close(info->connfd);
	free(info);
	pthread_exit(NULL);
}

int tcp_thread_server()
{
	socklen_t len;
	pthread_t thread;
	struct sockaddr_in saddr, caddr;
	int ret, sockfd, newfd, opt = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("sockfd");
		exit(EXIT_FAILURE);
	}

	len = sizeof(opt);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, len) == -1) {
		perror("setsockop");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	while (1) {
		struct sock_info *info;
		
		printf("wait for new connect....\n");

		if ((newfd = accept(sockfd, (struct sockaddr *)&caddr, &len)) == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		info = (struct sock_info *)malloc(sizeof(struct sock_info));
		if (info == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		info->connfd = newfd;
		memcpy((void *)&info->client, &caddr, sizeof(caddr));

		if ((ret = pthread_create(&thread, NULL, sock_thread, (void *)info))) {
			free(info);
			perror("pthread");
			exit(EXIT_FAILURE);
		}
	}

	close(sockfd);

	return 0;
}

int tcp_select_server()
{
	fd_set fds;
	socklen_t len;
	char sbuf[1024];
	struct timeval tv;
	struct sockaddr_in saddr, caddr;
	int sockfd, newfd, ret, on = 1, maxfd = -1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	len = sizeof(on);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len) == -1) {
		perror("setsockop");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	while (1) {
		printf("wait for new connect....\n");
		
		if ((newfd = accept(sockfd, (struct sockaddr *)&caddr, &len)) == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("accept connection from %s, port %d, socket %d\n",
		       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

		while (1) {
			FD_ZERO(&fds);
			FD_SET(0, &fds);
			FD_SET(newfd, &fds);
			maxfd = newfd;
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
			if (ret == -1) {
				perror("select");
				exit(EXIT_FAILURE);
			} else if (ret == 0) {
				continue;
			}

			if (FD_ISSET(0, &fds)) {
				memset(sbuf, 0, 1024);
				fgets(sbuf, 1023, stdin);
				ret = send(newfd, sbuf, strlen(sbuf) - 1, 0);
				if (ret > 0) {
					printf("send message: %s\n", sbuf);
					if (!strncasecmp(sbuf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
						     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(3);
						close(newfd);
						break;
					}
				} else if (ret < 0) {
					perror("send");
				}
			}

			if (FD_ISSET(newfd, &fds)) {
				memset(sbuf, 0, 1024);
				ret = recv(newfd, sbuf, 1023, 0);
				if (ret > 0) {
					printf("[%s:%d]: %s\n", inet_ntoa(caddr.sin_addr), 
						ntohs(caddr.sin_port), sbuf);
					if (!strncasecmp(sbuf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
						     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(1);
						close(newfd);
						break;
					}
				} else if (ret < 0) {
					perror("recv");
				}
			}
		}
		close(newfd);
	}

	close(sockfd);

	return 0;
}

int tcp_poll_server()
{
	socklen_t len;
	char sbuf[1024];
	struct pollfd fds[2];
	int sockfd, newfd, on = 1, ret;
	struct sockaddr_in saddr, caddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

 	len = sizeof(on);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len) == -1) {
		perror("setsockop");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	while (1) {
		printf("wait for new connect...\n\n");
		if ((newfd = accept(sockfd, (struct sockaddr *)&caddr, &len)) == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		printf("accept connection from %s, port %d, socket %d\n",
		       inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

		while (1) {
			fds[0].fd = 0;
			fds[1].fd = newfd;
			fds[0].events = POLLIN | POLLOUT | POLLERR;
			fds[1].events = POLLIN | POLLOUT | POLLERR;

			while (fds[0].events || fds[1].events) {
				if (poll(fds, 2, 600) <= 0) {
					perror("poll error\n");
					exit(EXIT_FAILURE);
				}

				if (fds[0].revents) {
					memset(sbuf, 0, 1024);
					fgets(sbuf, 1023, stdin);
					ret = send(newfd, sbuf, strlen(sbuf) - 1, 0);
					if (ret > 0) {
						printf("send message: %s\n", sbuf);
						if (!strncasecmp(sbuf, "quit", 4)) {
							printf("close the connect [%s:%d]\n",
							     inet_ntoa(caddr.sin_addr),
							     ntohs(caddr.sin_port));
							sleep(3);
							close(newfd);
							break;
						}
					} else if (ret < 0) {
						perror("send");
					}
				}

				if (fds[1].revents) {
					memset(sbuf, 0, 1024);
					ret = recv(newfd, sbuf, 1023, 0);
					if (ret > 0) {
						printf("[%s:%d]: %s\n\n", inet_ntoa(caddr.sin_addr),
						       ntohs(caddr.sin_port), sbuf);
						if (!strncasecmp(sbuf, "quit", 4)) {
							printf("close the connect [%s:%d]\n",
							     inet_ntoa(caddr.sin_addr),
							     ntohs(caddr.sin_port));
							sleep(1);
							close(newfd);
							break;
						}
					} else if (ret < 0) {
						perror("recv");
					}
				}
			}
		}
		close(newfd);
	}

	close(sockfd);

	return 0;
}

int tcp_epoll_server()
{
	long flag;
	socklen_t len;
	char sbuf[1024];
	struct rlimit rt;
	struct sockaddr_in saddr, caddr;
	struct epoll_event ev, events[10000];
	int sockfd, newfd, kfd, cfd, nfd, n, ret;

	rt.rlim_max = rt.rlim_cur = 10000;
	if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
		perror("setrlimit");
		exit(EXIT_FAILURE);
	}

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	flag = fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, flag) == -1) {
		perror("fcntl");
		exit(EXIT_FAILURE);
	}

	len = sizeof(struct sockaddr);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = PF_INET;
	saddr.sin_port = htons(9001);
	saddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&saddr, len) == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	kfd = epoll_create(10000);
	len = sizeof(struct sockaddr_in);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sockfd;
	if (epoll_ctl(kfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
		perror("sockfd");
		exit(EXIT_FAILURE);
	}

	cfd = 1;
	while (1) {
		printf("wait for new connect....\n");
		nfd = epoll_wait(kfd, events, cfd, -1);
		if (nfd == -1) {
			perror("epoll_wait");
			break;
		}

		for (n = 0; n < nfd; ++n) {
			if (events[n].data.fd == sockfd) {
				newfd = accept(sockfd, (struct sockaddr *)&caddr, &len);
				if (newfd < 0) {
					perror("accept");
					continue;
				}

				printf("accept connect from %s, port %d, socket: %d\n",
				     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

				flag = fcntl(newfd, F_GETFD, 0) | O_NONBLOCK;
				if (fcntl(newfd, F_SETFL, flag) == -1) {
					perror("fcntl");
					exit(EXIT_FAILURE);
				}

				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = newfd;
				if (epoll_ctl(kfd, EPOLL_CTL_ADD, newfd, &ev) < 0) {
					perror("epoll");
					exit(EXIT_FAILURE);
				}
				cfd++;
			} else {
				memset(sbuf, 0, 1024);
				ret = recv(events[n].data.fd, sbuf, 1023, 0);
				if (ret > 0) {
					printf("[%s:%d]: %s\n", inet_ntoa(caddr.sin_addr),
					       ntohs(caddr.sin_port), sbuf);
					if (!strncasecmp(sbuf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
						     inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(1);
						close(newfd);
						break;
					} else if (ret < 0) {
						perror("recv");
						epoll_ctl(kfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
						cfd--;
					}
				}
			}
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
		//creat a client connect.
	case 1:
		tcp_comm_client();
		break;

		//creat a server accept single client.
	case 2:
		tcp_point_server();
		break;

		//creat a server accept muti clients by fork.
	case 3:
		tcp_proc_server();
		break;

		//creat a server accept muti clients by muti_threads.
	case 4:
		tcp_thread_server();
		break;

		//creat a server accept muti clients by select.
	case 5:
		tcp_select_server();
		break;

		//creat a server accept muti clients by poll.
	case 6:
		tcp_poll_server();
		break;

		//creat a server accept muti clients by epoll.
	case 7:
		tcp_epoll_server();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
