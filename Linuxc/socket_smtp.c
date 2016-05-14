/******************************************************************************
 * Function: 	Socket smtp communication.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall socket_smtp.c -o socket_smtp
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

static int myscocket;

struct cron_send_time {
	__u8 month;
	__u8 dayofweek;
	__u8 daily;
	__u8 hour;
	__u8 minute;
};

struct email_info {
	char addr[16];
	__u16 port;
	char auth[16];
	struct cron_send_time time;
	char username[32];
	char passwd[32];
	char subject[128];
	char attachment[128];
	char mail_addr[256];
	char cc_addr[256];
};

static int POS(char c)
{
	if (c == '=')
		return 64;

	if (isupper(c))
		return c - 'A';

	if (islower(c))
		return c - 'a' + 26;

	if (isdigit(c))
		return c - '0' + 52;

	if (c == '+')
		return 62;

	if (c == '/')
		return 63;

	return -1;
}

static char *base64encode(const void *buf, int size)
{
	int c, i = 0;
	char *str, *p;
	unsigned char *q = (unsigned char *)buf;
	char base64[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	str = (char *)malloc((size + 3) * 4 / 3 + 3);
	memset(str, 0, (size + 3) * 4 / 3 + 1);
	p = str;

	while (i < size) {
		c = q[i++];
		c *= 256;
		if (i < size)
			c += q[i];
		i++;
		c *= 256;
		if (i < size)
			c += q[i];
		i++;
		p[0] = base64[(c & 0x00fc0000) >> 18];
		p[1] = base64[(c & 0x0003f000) >> 12];
		p[2] = base64[(c & 0x00000fc0) >> 6];
		p[3] = base64[(c & 0x0000003f) >> 0];

		if (i > size)
			p[3] = '=';

		if (i > size + 1)
			p[2] = '=';
		p += 4;
	}

	*p = 0;

	return str;
}

int base64decode(char *s, void *data)
{
	int n[4];
	char *p;
	unsigned char *q;

	if (strlen(s) % 4)
		return -1;

	q = (unsigned char *)data;
	for (p = s; *p; p += 4) {
		n[0] = POS(p[0]);
		n[1] = POS(p[1]);
		n[2] = POS(p[2]);
		n[3] = POS(p[3]);

		if ((n[0] | n[1] | n[2] | n[3]) < 0)
			return -1;

		if (n[0] == 64 || n[1] == 64)
			return -1;

		if (n[2] == 64 && n[3] < 64)
			return -1;

		q[0] = (n[0] << 2) + (n[1] >> 4);

		if (n[2] < 64)
			q[1] = ((n[1] & 15) << 4) + (n[2] >> 2);

		if (n[3] < 64)
			q[2] = ((n[2] & 3) << 6) + n[3];

		q += 3;
	}

	q -= (n[2] == 64) + (n[3] == 64);

	return q - (unsigned char *)data;
}

static void connect_timeout(int sig)
{
	close(myscocket);
}

void socket_close()
{
	close(myscocket);
}

int socket_send(int fd, char *ptr, __u32 nbytes, __u32 timeout)
{
	int remaining, sent;
	struct timeval tout;
	fd_set writefds, exceptfds;

	if (fd == -1)
		return -1;

	sent = 0;
	tout.tv_sec = timeout;
	tout.tv_usec = 0;

	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(fd, &writefds);
	FD_SET(fd, &exceptfds);

	remaining = nbytes;
	while (remaining > 0) {
		int status =
		    select(fd + 1, (fd_set *) NULL, &writefds, &exceptfds,
			   &tout);
		if (status == 0)
			goto out;

		if (status < 0) {
			if (errno == EINTR)
				continue;
			goto out;
		}

		if (FD_ISSET(fd, &exceptfds))
			goto out;

		if (!FD_ISSET(fd, &writefds))
			goto out;

		sent = write(fd, ptr, remaining);
		if (sent <= 0)
			goto out;

		remaining -= sent;
		ptr += sent;
	}

	return (nbytes - remaining);

      out:
	close(fd);

	return (-1);
}

int socket_recv(int fd, char *ptr, __u32 nbytes, __u32 timeout)
{
	int nleft, nread;
	struct timeval tout;
	fd_set readfds, exceptfds;

	if (fd == -1)
		return -1;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	FD_ZERO(&exceptfds);
	FD_SET(fd, &exceptfds);
	tout.tv_sec = timeout;
	tout.tv_usec = 0;
	nleft = nbytes;

	while (nleft > 0) {
		int status =
		    select(fd + 1, &readfds, (fd_set *) NULL, &exceptfds,
			   &tout);
		if (status == 0)
			goto out;

		if (status < 0) {
			if (errno == EINTR)
				continue;
			goto out;
		}

		if (FD_ISSET(fd, &exceptfds))
			goto out;

		if (!FD_ISSET(fd, &readfds))
			goto out;

	      again:
		nread = read(fd, ptr, nleft);
		if (nread < 0) {
			if (errno == EINTR)
				goto again;
			goto out;
		} else if (nread == 0) {
			goto out;
		}

		nleft -= nread;
		if (nleft)
			ptr += nread;

		return 0;
	}

	return (nbytes - nleft);

      out:
	close(fd);

	return -1;
}

struct email_info *Init_Mail_Info()
{
	struct email_info *email;

	email = (struct email_info *)malloc(sizeof(struct email_info));
	if (email == NULL)
		return NULL;

	memset(email, 0, sizeof(struct email_info));
	email->port = htons(25);
	strcpy(email->addr, "192.168.1.169");
	strcpy(email->auth, "yes");
	strcpy(email->username, "wu_yousheng");
	strcpy(email->passwd, "ustb2007_wys");
	strcpy(email->subject, "test");
	strcpy(email->attachment, "ips.tgz");
	strcpy(email->mail_addr, "wu_yousheng@topsec.com.cn");
	strcpy(email->cc_addr, "none");

	return email;
}

int Connect_Mail_Server(struct email_info *email)
{
	struct sockaddr_in addr;
	typedef void (*sighandler_t) (int);
	sighandler_t sigfunc;

	if ((myscocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_aton(email->addr, (struct in_addr *)&addr.sin_addr.s_addr);
	addr.sin_port = email->port;
	sigfunc = signal(SIGALRM, connect_timeout);
	alarm(10);

	if (connect(myscocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	alarm(0);
	signal(SIGALRM, sigfunc);

	return 0;
}

int Login_Mail_Server(struct email_info *email)
{
	char *temp_user, *temp_pawd;
	int len_user = 0, len_pawd = 0;
	char readbuf[1024], writebuf[1024];
	char base64_user[1024], base64_pawd[1024];

	if (email->username == NULL || email->passwd == NULL)
		exit(EXIT_FAILURE);

	memset(base64_user, 0, 1024);
	memset(base64_pawd, 0, 1024);
	strcpy(base64_user, email->username);
	strcpy(base64_pawd, email->passwd);

	temp_user = base64encode(base64_user, strlen(base64_user));
	len_user = strlen(temp_user);
	temp_user[len_user] = '\r';
	temp_user[len_user + 1] = '\n';
	temp_user[len_user + 2] = 0;

	temp_pawd = base64encode(base64_pawd, strlen(base64_pawd));
	len_pawd = strlen(temp_pawd);
	temp_pawd[len_pawd] = '\r';
	temp_pawd[len_pawd + 1] = '\n';
	temp_pawd[len_pawd + 2] = 0;

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "EHLO TOS\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);
	memset(readbuf, 0, 1024);
	socket_recv(myscocket, readbuf, 1024, 5);

	if (strstr(email->auth, "yes")) {
		memset(writebuf, 0, 1024);
		strcpy(writebuf, "AUTH LOGIN\r\n");
		socket_send(myscocket, writebuf, strlen(writebuf), 5);
		memset(readbuf, 0, 1024);
		socket_recv(myscocket, readbuf, 1024, 5);

		socket_send(myscocket, temp_user, strlen(temp_user), 5);
		memset(readbuf, 0, 1024);
		socket_recv(myscocket, readbuf, 1024, 5);
		free(temp_user);

		socket_send(myscocket, temp_pawd, strlen(temp_pawd), 5);
		memset(readbuf, 0, 1024);
		socket_recv(myscocket, readbuf, 1024, 5);
		free(temp_pawd);
	}

	return 0;
}

int Send_Mail_Cotent(struct email_info *email, char *msg)
{
	char readbuf[1024], writebuf[1024];
	char date[128] = "", from_addr[1024] = "";
	char *to_addr = NULL, *cc_addr = NULL;
	time_t ti = time(0);
	struct tm t = { 0 };

	if (email == NULL) {
		close(myscocket);
		exit(EXIT_FAILURE);
	}

	localtime_r(&ti, &t);
	strftime(date, 127, "%a, %d %b %Y %H:%M:%S %z", &t);

	if (strstr(email->auth, "yes"))
		strcpy(from_addr, email->username);
	else
		strcpy(from_addr, "topsec");

	/***********************mail	***************************/
	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "MAIL FROM:<%s>\r\n", from_addr);
	socket_send(myscocket, writebuf, strlen(writebuf), 5);
	memset(readbuf, 0, 1024);
	socket_recv(myscocket, readbuf, 1024, 5);

	/***********************rcpt	***************************/
	to_addr = strtok(email->mail_addr, ";");
	while (to_addr != NULL) {
		memset(writebuf, 0, 1024);
		snprintf(writebuf, 1023, "RCPT TO:<%s>\r\n", to_addr);
		socket_send(myscocket, writebuf, strlen(writebuf), 5);
		memset(readbuf, 0, 1024);
		socket_recv(myscocket, readbuf, 1024, 5);
		to_addr = strtok(NULL, ";");
	}

	cc_addr = strtok(email->cc_addr, ";");
	while (cc_addr != NULL && !strstr(cc_addr, "none")) {
		memset(writebuf, 0, 1024);
		snprintf(writebuf, 1023, "RCPT TO:<%s>\r\n", cc_addr);
		socket_send(myscocket, writebuf, strlen(writebuf), 5);
		memset(readbuf, 0, 1024);
		socket_recv(myscocket, readbuf, 1024, 5);
		cc_addr = strtok(NULL, ";");
	}

	/***********************data	***************************/
	memset(writebuf, 0, 1024);
	strcpy(writebuf, "DATA\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);
	memset(readbuf, 0, 1024);
	socket_recv(myscocket, readbuf, 1024, 5);

	memset(writebuf, 0, 1024);
	if (cc_addr != NULL && !strstr(cc_addr, "none")) {
		snprintf(writebuf, 1023,
			 "From:%s\r\nTo:<%s>\r\nCc:<%s>\r\nDate:%s\r\nSubject:%s\r\n",
			 from_addr, email->mail_addr, email->cc_addr, date,
			 email->subject);
	} else {
		snprintf(writebuf, 1023,
			 "From:%s\r\nTo:<%s>\r\nDate:%s\r\nSubject:%s\r\n",
			 from_addr, email->mail_addr, date, email->subject);
	}
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	/**********************content**************************/
	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "MIME-Version: 1.0\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023,
		 "Content-Type: multipart/mixed; boundary=\"boundary=topsec\"\r\n\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "Content-Transfer-Encoding: 7bit\r\n\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "This is a multipart Encoded format\r\n\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "--boundary=topsec\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023,
		 "Content-Type: text/plain; charset=gb2312\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "Content-Transfer-Encoding:printable\r\n\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "%s\r\n", msg);
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	return 0;
}

int Send_Mail_Attachment(struct email_info *email)
{
	int length;
	FILE *fp = NULL;
	char writebuf[1024], filename[128];
	char *buff = NULL, *tmp = NULL;

	snprintf(filename, sizeof(filename) - 1, "%s", email->attachment);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "--boundary=topsec\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023,
		 "Content-Type:application/octet-stream;name=%s\r\n",
		 email->attachment);
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "Content-Transfer-Encoding:base64\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023,
		 "Content-Disposition:attachment;filename=%s\r\n\r\n",
		 email->attachment);
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	rewind(fp);
	buff = (char *)malloc(sizeof(char) * length);
	if (buff == NULL)
		return -1;

	fread(buff, 1, length, fp);
	fclose(fp);

	tmp = base64encode(buff, length);
	length = socket_send(myscocket, tmp, strlen(tmp), 60);
	free(buff);
	free(tmp);

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "--boundary=topsec--\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);

	return 0;
}

int Send_Mail_End()
{
	char readbuf[1024] = "", writebuf[1024] = "";

	memset(writebuf, 0, 1024);
	snprintf(writebuf, 1023, "\r\n.\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);
	memset(readbuf, 0, 1024);
	socket_recv(myscocket, readbuf, 1024, 5);

	memset(writebuf, 0, 1024);
	strcpy(writebuf, "QUIT\r\n");
	socket_send(myscocket, writebuf, strlen(writebuf), 5);
	memset(readbuf, 0, 1024);
	socket_recv(myscocket, readbuf, 1024, 5);

	close(myscocket);

	return 0;
}

int main(int argc, char *argv[])
{
	char msg[1024] = "beike";
	struct email_info *email;

	email = Init_Mail_Info();
	if (email == NULL) {
		printf("Init_Mail_Info fail\n");
		exit(EXIT_FAILURE);
	}

	if (Connect_Mail_Server(email)) {
		printf("can not connect the host\n");
		exit(EXIT_FAILURE);
	}

	if (Login_Mail_Server(email)) {
		printf("login error\n");
		exit(EXIT_FAILURE);
	}

	if (Send_Mail_Cotent(email, msg)) {
		printf("send mail error\n");
		exit(EXIT_FAILURE);
	}

	if (Send_Mail_Attachment(email)) {
		printf("send affix error\n");
		exit(EXIT_FAILURE);
	}

	if (Send_Mail_End()) {
		printf("End error\n");
		exit(EXIT_FAILURE);
	}

	socket_close();
	free(email);

	return 0;
}
