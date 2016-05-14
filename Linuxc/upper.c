/******************************************************************************
 * Function: 	Upper called by other program.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall upper.c -o upper
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

int send_back(int send_to, void *data, size_t len, int file_fd)
{
	struct iovec iov[1];
	struct msghdr msg;
	struct cmsghdr *cmsg = NULL;
	union {
		struct cmsghdr cm;
		char ctl[CMSG_SPACE(sizeof(int))];
	} ctl_un;

	iov[0].iov_base = data;
	iov[0].iov_len = len;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ctl_un.ctl;
	msg.msg_controllen = sizeof(ctl_un.ctl);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsg)) = file_fd;

	return (sendmsg(send_to, &msg, 0));
}

int main(int argc, char **argv)
{
	int fd;
	char ch;
	ssize_t ret_n;

	if (argc != 4) {
		while ((ch = getchar()) != EOF) 
			putchar(toupper(ch));
		return 0;
	}

	if ((fd = open(argv[2], atoi(argv[3]))) < 0) {
		perror("open file");
		exit(EXIT_FAILURE);
	}

	if ((ret_n = send_back(atoi(argv[1]), "", 1, fd)) < 0) {
		perror("send_back");
		exit(EXIT_FAILURE);
	}

	return 0;
}
