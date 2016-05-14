/******************************************************************************
 * Function: 	Socket proxy communication.
 * Author: 	forwarding2012@yahoo.com.cn							
 * Date: 		2012.01.01
 * Compile:	gcc -Wall -lssl socket_proxy.c -o socket_proxy
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
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
#include <openssl/des.h>
#include <openssl/md4.h>

#define HTTP_AUTH_NONE  0
#define HTTP_AUTH_BASIC 1
#define HTTP_AUTH_NTLM  2
#define HTTP_AUTH_N     3
#define COMM_LINE_LEN 	128
#define MAX1_LINE_LEN 	256
#define MAX2_LINE_LEN 	512

struct user_pass {
	int defined;
	int nocache;
	char username[COMM_LINE_LEN];
	char password[COMM_LINE_LEN];
};

struct proxy_opt {
	char *server;
	int   port;
	int   retry;
	int   timeout;
	const char *auth_method;
	const char *auth_file;
	const char *http_version;
	const char *user_agent;
};

struct http_proxy_info {
	int   defined;
	int    auth_method;
	struct user_pass ups;
	struct proxy_opt opt;
};

struct socks_proxy_info {
	int defined;
	int retry;
	char server[COMM_LINE_LEN];
	int  port;
};

static struct user_pass static_upwd;

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test com http proxy.\n");
	printf(" 2: test tcp sock proxy.\n");
	printf(" 3: test udp sock proxy.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void format_line_enter(char *str)
{
	int modified;
	
	do {
		const int len = strlen(str);
		modified = 0;
		if (len > 0) {
			char *cp = str + (len - 1);
			if (*cp == '\n' || *cp == '\r') {
				*cp = '\0';
				modified = 1;
			}
		}
	} while (modified);
}

static int recv_inter_line(int sd, char *buf, int len, const int timeout)
{
	int lastc = 0;

	while (1) {
		uint8_t c;
		int status;
		ssize_t size;
		fd_set reads;
		struct timeval tv;

		FD_ZERO(&reads);
		FD_SET(sd, &reads);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		status = select(sd + 1, &reads, NULL, NULL, &tv);
		if (status == 0) {
			printf("recv_inter_line: TCP port read timeout expired");
			return 0;
		} else if (status < 0) {
			printf("recv_inter_line: TCP port read failed on select()");
			return 0;
		}

		size = recv(sd, &c, 1, 0);
		if (size != 1) {
			printf("recv_inter_line: TCP port read failed on recv()");
			return 0;
		}

		if (len > 1) {
			*buf++ = c;
			--len;
		}

		if (lastc == '\r' && c == '\n')
			break;

		lastc = c;
	}

	if (len > 0)
		*buf++ = '\0';

	return 1;
}

static int send_inter_line(int sd, const char *src)
{
	char *buf;
	ssize_t size;
	
	buf = (char *)malloc(strlen(src) + 3);
	snprintf(buf, strlen(src) + 3, "%s\r\n", src);
	size = send(sd, buf, strlen(buf), 0);
	if (size != (ssize_t)strlen(buf)) {
		printf("send_line: TCP port write failed on send()");
		return 0;
	}
	free(buf);

	return 1;
}

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

static char *handle_base64_encode(const void *buf, int size)
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
		c *= MAX1_LINE_LEN;
		if (i < size)
			c += q[i];
		i++;
		c *= MAX1_LINE_LEN;
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

int handle_base64_decode(char *s, void *data)
{
	char *p;
	int n[4];
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


static void gen_des_keys(const unsigned char *hash, unsigned char *key)
{
	key[0] = hash[0];
	key[1] = ((hash[0] & 1) << 7) | (hash[1] >> 1);
	key[2] = ((hash[1] & 3) << 6) | (hash[2] >> 2);
	key[3] = ((hash[2] & 7) << 5) | (hash[3] >> 3);
	key[4] = ((hash[3] & 15) << 4) | (hash[4] >> 4);
	key[5] = ((hash[4] & 31) << 3) | (hash[5] >> 5);
	key[6] = ((hash[5] & 63) << 2) | (hash[6] >> 6);
	key[7] = ((hash[6] & 127) << 1);
	des_set_odd_parity((des_cblock *) key);
}

static void gen_md4_hash(const char *data, int data_len, char *result)
{
	MD4_CTX c;
	char md[16];

	MD4_Init(&c);
	MD4_Update(&c, data, data_len);
	MD4_Final((unsigned char *)md, &c);
	memcpy(result, md, 16);
}

static int gen_unicode_outs(char *dst, const char *src)
{
	int i = 0;
	
	do {
		dst[i++] = *src;
		dst[i++] = 0;
	}
	while (*src++);

	return i;
}

const char *ntlm_phase_3(const struct http_proxy_info *p, char *phase_2)
{
	char pwbuf[sizeof(p->ups.password) * 2];
	char buf2[COMM_LINE_LEN], phase3[146], md4_hash[21];
	char challenge[8], response[24];
	int i, buflen;
	des_cblock key1, key2, key3;
	des_key_schedule sched1, sched2, sched3;

	gen_md4_hash(pwbuf, gen_unicode_outs(pwbuf, p->ups.password) - 2, md4_hash);
	memset(md4_hash + 16, 0, 5);

	handle_base64_decode(phase_2, (void *)buf2);
	for (i = 0; i < 8; i++) 
		challenge[i] = buf2[i + 24];

	gen_des_keys((unsigned char *)md4_hash, key1);
	des_set_key_unchecked((des_cblock *) key1, sched1);
	des_ecb_encrypt((des_cblock *) challenge, 
					(des_cblock *) response, sched1, DES_ENCRYPT);

	gen_des_keys((unsigned char *)&(md4_hash[7]), key2);
	des_set_key_unchecked((des_cblock *) key2, sched2);
	des_ecb_encrypt((des_cblock *) challenge,
					(des_cblock *) & (response[8]), sched2, DES_ENCRYPT);

	gen_des_keys((unsigned char *)&(md4_hash[14]), key3);
	des_set_key_unchecked((des_cblock *) key3, sched3);
	des_ecb_encrypt((des_cblock *) challenge,
					(des_cblock *) & (response[16]), sched3, DES_ENCRYPT);

	memset(phase3, 0, sizeof(phase3));
	strcpy(phase3, "NTLMSSP\0");
	phase3[8] = 3;

	buflen = 0x58 + strlen(p->ups.username);
	if (buflen > (int)sizeof(phase3))
		buflen = sizeof(phase3);

	phase3[0x10] = buflen;		/* lm not used */
	phase3[0x20] = buflen;		/* default domain (i.e. proxy's domain) */
	phase3[0x30] = buflen;		/* no workstation name supplied */
	phase3[0x38] = buflen;		/* no session key */
	phase3[0x14] = 24;			/* ntlm response is 24 bytes long */
	phase3[0x16] = phase3[0x14];
	phase3[0x18] = 0x40;		/* ntlm offset */
	memcpy(&(phase3[0x40]), response, 24);
	phase3[0x24] = strlen(p->ups.username);	/* username in ascii */
	phase3[0x26] = phase3[0x24];
	phase3[0x28] = 0x58;
	strncpy(&(phase3[0x58]), p->ups.username, sizeof(phase3) - 0x58);
	phase3[0x3c] = 0x02;		/* negotiate oem */
	phase3[0x3d] = 0x02;		/* negotiate ntlm */

	return handle_base64_encode((unsigned char *)phase3, sizeof(phase3));
}

struct http_proxy_info *new_http_proxy(const struct proxy_opt *o)
{
	struct http_proxy_info *p;

	if (o->server == NULL)
		printf("HTTP_PROXY: server not specified");

	p->opt = *o;
	p->auth_method = HTTP_AUTH_NONE;
	if (o->auth_method) {
		if (!strcmp(o->auth_method, "none"))
			p->auth_method = HTTP_AUTH_NONE;
		else if (!strcmp(o->auth_method, "basic"))
			p->auth_method = HTTP_AUTH_BASIC;
		else if (!strcmp(o->auth_method, "ntlm"))
			p->auth_method = HTTP_AUTH_NTLM;
		else
			printf("ERROR: only 'none', 'basic', 'ntlm' methods are supported");
	}

	if (p->auth_method == HTTP_AUTH_BASIC || p->auth_method == HTTP_AUTH_NTLM) 
		p->ups = static_upwd;
	
	p->defined = 1;

	return p;
}

void socket_http_proxy(struct http_proxy_info *p, int sd, char *host, int port)
{
	char *tmp = NULL;
	int status, nparms;
	char buf1[MAX1_LINE_LEN], buf2[COMM_LINE_LEN], get[80], out[MAX2_LINE_LEN];

	snprintf(buf1, sizeof(buf1), "CONNECT %s:%d HTTP/%s",
					 host, port, p->opt.http_version);
	printf("Send to HTTP proxy: '%s'", buf1);
	send_inter_line(sd, buf1);

	if (p->opt.user_agent) {
		snprintf(buf1, sizeof(buf1), "User-Agent: %s", p->opt.user_agent);
		send_inter_line(sd, buf1);
	}

	switch (p->auth_method) {
	case HTTP_AUTH_NONE:
		break;

	case HTTP_AUTH_BASIC:
		printf("Attempting Basic Proxy-Authorization");
		snprintf(out, sizeof(out), "%s:%s", p->ups.username, p->ups.password);
		tmp = handle_base64_encode(out, strlen(out));
		snprintf(buf1, sizeof(buf1), "Proxy-Authorization: Basic %s", tmp);
		free(tmp);
		sleep(1);
		send_inter_line(sd, buf1);
		break;

	case HTTP_AUTH_NTLM:
		snprintf(buf1, sizeof(buf1), "Proxy-Authorization: NTLM %s", 
			"TlRMTVNTUAABAAAAAgIAAA==");
		sleep(1);
		send_inter_line(sd, buf1);
		break;

	default:
		return;
	}

	sleep(1);
	send_inter_line(sd, "");

	if (!recv_inter_line(sd, buf1, sizeof(buf1), p->opt.timeout))
		return;

	format_line_enter(buf1);
	printf("HTTP proxy returned: '%s'", buf1);
	nparms = sscanf(buf1, "%*s %d", &status);
	if (nparms >= 1 && status == 407) {
		if (p->auth_method != HTTP_AUTH_NTLM) 
			return;
		
		printf("Proxy requires authentication");
		while (1) {
			if (!recv_inter_line(sd, buf1, sizeof(buf1), p->opt.timeout))
				return;
			
			format_line_enter(buf1);
			printf("HTTP proxy returned: '%s'", buf1);
			snprintf(get, sizeof(get), "%%*s NTLM %%%ds", (int)sizeof(buf2) - 1);
			nparms = sscanf(buf1, get, buf2);
			buf2[COMM_LINE_LEN - 1] = 0;	
			if (nparms == 1) {
				printf("auth string: '%s'", buf2);
				break;
			}
		}

		printf("Received NTLM Proxy-Authorization phase 2 response");
		while (recv_inter_line(sd, NULL, 0, p->opt.timeout)) ;

		snprintf(buf1, sizeof(buf1), "CONNECT %s:%d HTTP/%s", 
			host, port, p->opt.http_version);
		printf("Send to HTTP proxy: '%s'", buf1);
		send_inter_line(sd, buf1);

		sleep(1);
		snprintf(buf1, sizeof(buf1), "Host: %s", host);
		printf("Send to HTTP proxy: '%s'", buf1);
		send_inter_line(sd, buf1);

		snprintf(buf1, sizeof(buf1), "Proxy-Authorization: NTLM %s", 
			ntlm_phase_3(p, buf2));
		printf("Attempting NTLM Proxy-Authorization phase 3");
		printf("Send to HTTP proxy: '%s'", buf1);
		sleep(1);
		send_inter_line(sd, buf1);
		sleep(1);
		send_inter_line(sd, "");

		if (!recv_inter_line(sd, buf1, sizeof(buf1), p->opt.timeout))
			return;

		format_line_enter(buf1);
		printf("HTTP proxy returned: '%s'", buf1);
		nparms = sscanf(buf1, "%*s %d", &status);
		if (nparms < 1 || status != 200) {
			printf("HTTP proxy returned bad status");
			return;
		}
	}
	
	return;
}

struct socks_proxy_info *new_socks_proxy(char *server, int port, int retry)
{
	struct socks_proxy_info *p;
	
	strncpy(p->server, server, sizeof(p->server));
	p->port = port;
	p->retry = retry;
	p->defined = 1;

	return p;
}

static int send_socks_handshake(int sd)
{
	int len = 0;
	char buf[2];
	const int timeout = 5;

	if (send(sd, "\x05\x01\x00", 3, 0) != 3) {
		printf("send_socks_handshake: TCP port write failed on send()");
		return 0;
	}

	while (len < 2) {
		char c;
		fd_set reads;
		struct timeval tv;

		FD_ZERO(&reads);
		FD_SET(sd, &reads);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		if (select(sd + 1, &reads, NULL, NULL, &tv) <= 0) {
			printf("send_socks_handshake: TCP port read failed on select()");
			return 0;
		} 
		
		if (recv(sd, &c, 1, 0) != 1) {
			printf("send_socks_handshake: TCP port read failed on recv()");
			return 0;
		}
		buf[len++] = c;
	}

	if (buf[0] != '\x05' || buf[1] != '\x00') {
		printf("send_socks_handshake: Socks proxy returned bad status");
		return 0;
	}

	return 1;
}

static int recv_socks_reply(int sd, struct sockaddr_in *addr)
{
	int len = 0;
	int alen = 0;
	char buf[22];
	char atyp = '\0';
	const int timeout = 5;

	if (addr != NULL) {
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
		addr->sin_port = htons(0);
	}

	while (len < 4 + alen + 2) {
		char c;
		fd_set reads;
		struct timeval tv;

		FD_ZERO(&reads);
		FD_SET(sd, &reads);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		if (select(sd + 1, &reads, NULL, NULL, &tv) <= 0) {
			printf("recv_socks_reply: TCP port read timeout expired");
			return 0;
		}

		if (recv(sd, &c, 1, 0) != 1) {
			printf("recv_socks_reply: TCP port read failed on recv()");
			return 0;
		}

		if (len == 3)
			atyp = c;

		if (len == 4) {
			switch (atyp) {
			case '\x01':
				alen = 4;
				break;

			case '\x03':
				alen = (unsigned char)c;
				break;

			case '\x04':
				alen = 16;
				break;

			default:
				printf("recv_socks_reply: Socks proxy returned bad type");
				return 0;
			}
		}

		if (len < (int)sizeof(buf))
			buf[len] = c;
		++len;
	}

	if (buf[0] != '\x05' || buf[1] != '\x00') {
		printf("recv_socks_reply: Socks proxy returned bad reply");
		return 0;
	}

	if (atyp == '\x01' && addr != NULL) {
		memcpy(&addr->sin_addr, buf + 4, sizeof(addr->sin_addr));
		memcpy(&addr->sin_port, buf + 8, sizeof(addr->sin_port));
	}

	return 1;
}

void socket_tsock_proxy(struct socks_proxy_info *p, char *host, int port)
{
	int sd;
	size_t len;
	char buf[COMM_LINE_LEN];

	new_socks_proxy(host, port, 1);

	if (!send_socks_handshake(sd))
		return;

	buf[0] = '\x05';	
	buf[1] = '\x01';
	buf[2] = '\x00';
	buf[3] = '\x03';
	len = strlen(host);
	len = (5 + len + 2 > sizeof(buf)) ? (sizeof(buf) - 5 - 2) : len;
	buf[4] = (char)len;
	memcpy(buf + 5, host, len);
	buf[5 + len] = (char)(port >> 8);
	buf[5 + len + 1] = (char)(port & 0xff);

	if ((int)send(sd, buf, 5 + len + 2, 0) != 5 + (int)len + 2) {
		printf("socket_tsock_proxy: TCP port write failed on send()");
		return;
	}

	if (!recv_socks_reply(sd, NULL))
		return;

	return;
}

void socket_usock_proxy(struct socks_proxy_info *p, struct sockaddr_in *relay_addr)
{
	int sd;

	new_socks_proxy(p->server, p->port, 1);

	if (!send_socks_handshake(sd))
		return;

	if (send(sd, "\x05\x03\x00\x01\x00\x00\x00\x00\x00\x00", 10, 0) != 10) {
		printf("socket_usock_proxy: TCP port write failed on send()");
		return;
	}

	bzero(relay_addr, 0);
	if (!recv_socks_reply(sd, relay_addr))
		return;

	return;
}


int main(int argc, char *argv[])
{
	int sd, choice;
	struct http_proxy_info *hp;
	struct socks_proxy_info *sp;
	struct sockaddr_in *relay_addr;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test http proxy.
	case 1:
		socket_http_proxy(hp, sd, hp->opt.server, hp->opt.port);
		break;
	
		//test tcp sock proxy.
	case 2:
		socket_tsock_proxy(sp, sp->server, sp->port);
		break;

		//test udp sock proxy.
	case 3:
		socket_usock_proxy(sp, relay_addr);
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}


