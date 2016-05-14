/******************************************************************************
 * Function: 	some methods of openssl communication use.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall -lssl -lcrypt openssl_chat.c -o openssl_chat 
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <memory.h>
#include <crypt.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/comp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/dh.h>
#include <openssl/ec.h>
#include <openssl/conf.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/objects.h>

#define  RCERTF 	"ssl/cacert.pem"
#define  DHFILE   	"ssl/dh1024.pem"
#define  CNFILE 	"ssl/openssl.cnf"
#define  SKEYF   	"ssl/serverkey.pem"
#define  CKEYF   	"ssl/clientkey.pem"
#define  CCERTF		"ssl/clientcert.pem"
#define  SCERTF 	"ssl/servercert.pem"
#define  PKCS12F 	"ssl/commoncert.p12"

#define  UPKCS12	0
#define  SERMODE	1
#define	 CLIMODE	2
#define  RESHAKE	3
#define  BUFSIZE	1024

BIO *bio_err;
int client_auth;

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: check openssl bio base client.\n");
	printf(" 2: check openssl bio base server.\n");
	printf(" 3: check openssl bio ssl client.\n");
	printf(" 4: check openssl bio ssl server.\n");
	printf(" 5: check openssl bio ssl advance client.\n");
	printf(" 6: check openssl bio ssl advance server.\n");
	printf(" 7: check openssl common client.\n");
	printf(" 8: check openssl common server.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void child_signal_func(int signo)
{
	int stat;
	pid_t pid;

	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);

	return;
}

int openssl_error_exit(const char *string, int type)
{
	char buf[BUFSIZE] = { 0 };

	if (type == 0) {
		strncpy(buf, string, sizeof(buf) - 1);
		perror(buf);
	} else if (type == 1) {
		BIO_printf(bio_err, "%s\n", string);
		ERR_print_errors(bio_err);
	} else {
		ERR_load_BIO_strings();
		ERR_clear_error();
		bio_err = BIO_new(BIO_s_file());
		BIO_set_fp(bio_err, stdout, BIO_NOCLOSE);
	}
	
	return 0;
}

int check_cacnf_func()
{
	char *p;
	long ret;
	int i, num;
	CONF *conf;
	CONF_VALUE *param;
	STACK_OF(CONF_VALUE) * sec;

	conf = NCONF_new(NULL);
	if (NCONF_load(conf, CNFILE, &ret) != 1)
		openssl_error_exit("NCONF_load", 1);

	if (NCONF_get_string(conf, NULL, "certs") == NULL)
		openssl_error_exit("NCONF_get_string", 1);
	p = NCONF_get_string(conf, "CA_default", "certs");
	printf("\ncerts: %s\n", p);
	p = NCONF_get_string(conf, "CA_default", "default_days");
	printf("default_days: %s, ", p);
	NCONF_get_number_e(conf, "CA_default", "default_days", &ret);
	printf("%ld, ", ret);
	NCONF_get_number(conf, "CA_default", "default_days", &ret);
	printf("%ld\n", ret);

	if ((sec = NCONF_get_section(conf, "CA_default")) == NULL)
		openssl_error_exit("NCONF_get_string", 1);
	num = sk_CONF_VALUE_num(sec);
	printf("\nsection CA_default :\n");
	for (i = 0; i < num; i++) {
		param = sk_CONF_VALUE_value(sec, i);
		printf("%s = %s\n", param->name, param->value);
	}
	NCONF_free(conf);

	return 0;
}

void check_cert_func(SSL * ssl)
{
	char *str;
	X509 *peer;
	char peer_CN[256];

	printf("\nSSL connection use crypt: %s\n", SSL_get_cipher(ssl));
	peer = SSL_get_peer_certificate(ssl);
	printf("Server certificate:\n");
	str = X509_NAME_oneline(X509_get_subject_name(peer), 0, 0);
	printf("\t subject: %s\n", str);
	OPENSSL_free(str);
	str = X509_NAME_oneline(X509_get_issuer_name(peer), 0, 0);
	printf("\t isser: %s\n", str);
	OPENSSL_free(str);

	X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, peer_CN, 256);
	if (strcasecmp(peer_CN, "beike"))
	  openssl_error_exit("Common name doesn't match host name", 0);

	if (SSL_get_verify_result(ssl) != X509_V_OK)
	  openssl_error_exit("SSL_get_verify_result", 1);
}

int verify_peer_func(int res, X509_STORE_CTX * xs)
{
	if (xs->error == X509_V_ERR_UNABLE_TO_GET_CRL)
		openssl_error_exit("X509_V_ERR_UNABLE_TO_GET_CRL", 1);

	return res;
}

void common_chat_func(SSL * ssl, int sockfd)
{
	fd_set readfds, writefds;
	char c2s[BUFSIZE], s2c[BUFSIZE];
	int width, r, c2sl = 0, coffset = 0, swait = 0;
	int rbow = 0, wbor = 0, rbon = 0;

	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NDELAY))
		openssl_error_exit("Couldn't make socket nonblocking", 0);

	width = sockfd + 1;
	while (1) {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(sockfd, &readfds);

		if (!wbor) {
			if (c2sl || rbow)
				FD_SET(sockfd, &writefds);
			else
				FD_SET(fileno(stdin), &readfds);
		}

		if (select(width, &readfds, &writefds, 0, 0) == 0)
			continue;

		if ((FD_ISSET(sockfd, &readfds) && !wbor) || (rbow && FD_ISSET(sockfd, &writefds))) {
			do {
				rbow = 0;
				rbon = 0;

				r = SSL_read(ssl, s2c, BUFSIZE);
				switch (SSL_get_error(ssl, r)) {
				case SSL_ERROR_NONE:
					fwrite(s2c, 1, r, stdout);
					break;

				case SSL_ERROR_ZERO_RETURN:
					if (!swait)
						SSL_shutdown(ssl);
					goto end;
					break;

				case SSL_ERROR_WANT_READ:
					rbon = 1;
					break;

				case SSL_ERROR_WANT_WRITE:
					rbow = 1;
					break;

				default:
					openssl_error_exit("SSL read problem", 1);
				}
			} while (SSL_pending(ssl) && !rbon);
		}

		if (FD_ISSET(fileno(stdin), &readfds)) {
			if (read(fileno(stdin), c2s, BUFSIZE) == 0) {
				swait = 1;
				if (SSL_shutdown(ssl))
					return;
			}
			coffset = 0;
		}

		if ((FD_ISSET(sockfd, &writefds) && c2sl) || (wbor && FD_ISSET(sockfd, &readfds))) {
			wbor = 0;

			r = SSL_write(ssl, c2s + coffset, c2sl);
			switch (SSL_get_error(ssl, r)) {
			case SSL_ERROR_NONE:
				c2sl -= r;
				coffset += r;
				break;

			case SSL_ERROR_WANT_WRITE:
				break;

			case SSL_ERROR_WANT_READ:
				wbor = 1;
				break;

			default:
				openssl_error_exit("SSL write problem", 1);
			}
		}
	}

end:
	SSL_free(ssl);
	close(sockfd);

	return;
}

int http_request_func(SSL * ssl, int sockfd)
{
	int r, len;
	char buf[BUFSIZE];
	char request[BUFSIZE];
	char *REQUEST_CMD = "GET / HTTP/1.0\r\nUser-Agent:EKRClient\r\nHost: localhost:9001\r\n\r\n";

	snprintf(request, sizeof(request), REQUEST_CMD);
	r = SSL_write(ssl, request, strlen(request));
	if (strlen(request) != r)
		openssl_error_exit("SSL_write", 1);
	printf("\nSend message: \n%s", request);

	printf("Receive message: \n");
	while (1) {
		r = SSL_read(ssl, buf, BUFSIZE);
		switch (SSL_get_error(ssl, r)) {
		case SSL_ERROR_NONE:
			len = r;
			break;

		case SSL_ERROR_WANT_READ:
			continue;

		case SSL_ERROR_ZERO_RETURN:
			goto shutdown;

		case SSL_ERROR_SYSCALL:
			openssl_error_exit("SSL_read", 1);
			goto done;

		default:
			openssl_error_exit("SSL_read", 1);
		}

		fwrite(buf, 1, len, stdout);
	}

shutdown:
	if (SSL_shutdown(ssl) != 1)
		openssl_error_exit("Shutdown failed", 1);

done:
	SSL_free(ssl);
	close(sockfd);

	return 0;
}

int http_response_func(SSL * ssl, int s)
{
	char buf[BUFSIZE];
	BIO *bbio, *sbio;
	int r, len, aid_ctx = 2;

	bbio = BIO_new(BIO_f_buffer());
	sbio = BIO_new(BIO_f_ssl());
	BIO_set_ssl(sbio, ssl, BIO_CLOSE);
	BIO_push(bbio, sbio);

	while (1) {
		r = BIO_gets(bbio, buf, BUFSIZE - 1);
		switch (SSL_get_error(ssl, r)) {
		case SSL_ERROR_NONE:
			len = r;
			break;

		case SSL_ERROR_ZERO_RETURN:
			goto shutdown;
			break;

		default:
			openssl_error_exit("BIO_gets", 1);
		}

		if (!strcmp(buf, "\r\n") || !strcmp(buf, "\n"))
			break;
	}

	if (client_auth == RESHAKE) {
		SSL_set_verify(ssl, SSL_VERIFY_PEER, 0);
		SSL_set_session_id_context(ssl, (void *)&aid_ctx, sizeof(aid_ctx));

		printf("Rehandshake start......\n");

		if (SSL_renegotiate(ssl) <= 0)
			openssl_error_exit("SSL_renegotiate", 1);

		if (SSL_do_handshake(ssl) <= 0)
			openssl_error_exit("SSL_do_handshake", 1);

		ssl->state = SSL_ST_ACCEPT;
		if (SSL_do_handshake(ssl) <= 0)
			openssl_error_exit("SSL_do_handshake", 1);
	}

	if (BIO_puts(bbio, "HTTP/1.0 200 OK\r\n") <= 0)
		openssl_error_exit("BIO_puts", 0);

	if (BIO_puts(bbio, "Server: BeikeServer\r\n\r\n") <= 0)
		openssl_error_exit("BIO_puts", 0);

	if (BIO_puts(bbio, "Server test page\r\n") <= 0)
		openssl_error_exit("BIO_puts", 0);

	if (BIO_flush(bbio) <= 0)
		openssl_error_exit("BIO_flush", 0);
	else
		BIO_free_all(bbio);

shutdown:
	r = SSL_shutdown(ssl);
	if (!r) {
		shutdown(s, 1);
		r = SSL_shutdown(ssl);
	}

	if (r != 1)
		openssl_error_exit("Shutdown failed", 1);

	SSL_free(ssl);
	close(s);

	return 0;
}

static RSA *gen_rsa_cb(SSL *s, int is_export, int keylength)
{
	static RSA *rsa_tmp = NULL;
	rsa_tmp = RSA_generate_key(keylength, RSA_F4, NULL, NULL);
	
	return rsa_tmp;
}

static void check_ssl_info(const SSL *s, int where, int ret)
{
	openlog("logmask", LOG_PID | LOG_CONS, LOG_USER);

	if (where & SSL_CB_LOOP) {
		syslog(LOG_INFO, "SSL state (%s): %s\n", where & SSL_ST_CONNECT ? 
			"connect" : where & SSL_ST_ACCEPT ? "accept" : "undefined", 
			SSL_state_string_long(s));
	} else if (where & SSL_CB_ALERT) {
		syslog(LOG_INFO, "SSL alert (%s): %s: %s\n", where & SSL_CB_READ ? 
			"read" : "write", SSL_alert_type_string_long(ret), 
			SSL_alert_desc_string_long(ret));
	}

	closelog();
}

SSL_CTX *openssl_ctx_init(int mode, char *pemfile, char *keyfile, char *password)
{
	int i;
	DH *dh;
	BIO *bio;
	FILE *fp;
	X509 *cert;
	PKCS12 *p12;
	SSL_CTX *ctx;
	EVP_PKEY *pkey;
	char pkcs12pwd[256];
	STACK_OF(X509) *ca = NULL;
	STACK_OF(X509_NAME) *cert_names;

	SSL_library_init();
	ERR_load_SSL_strings();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	if (mode == SERMODE) {
		if ((ctx = SSL_CTX_new(SSLv23_server_method())) == NULL)
			openssl_error_exit("SSL_CTX_new", 1);

		SSL_CTX_set_tmp_rsa_callback(ctx, gen_rsa_cb);

		if ((bio = BIO_new_file(DHFILE, "r")) == NULL)
			openssl_error_exit("BIO_new_file", 1);
		
		if ((dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL)) == NULL)
			openssl_error_exit("PEM_read_bio_DHparams", 1);
		else
			BIO_free(bio);
		
		if (SSL_CTX_set_tmp_dh(ctx, dh) < 0)
			openssl_error_exit("SSL_CTX_set_tmp_dh", 1);
		else
			DH_free(dh);
	} else if (mode == CLIMODE) {
		if ((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
			openssl_error_exit("SSL_CTX_new", 1);
	} else {
		openssl_error_exit("Undefine mode", 0);
		return NULL;
	}

	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
	SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);
	SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
	SSL_CTX_set_default_passwd_cb_userdata(ctx, password);

	if (UPKCS12) {
		if (!(fp = fopen(PKCS12F, "rb")))
			openssl_error_exit("fopen", 0);
		p12 = d2i_PKCS12_fp(fp, NULL);
		fclose(fp);

		if (!PKCS12_parse(p12, "", &pkey, &cert, &ca)) {
			strncpy(pkcs12pwd, password, sizeof(pkcs12pwd) - 1);
			ca = NULL;
			if (!PKCS12_parse(p12, pkcs12pwd, &pkey, &cert, &ca)) {
				PKCS12_free(p12);
				ERR_clear_error();
				if (ctx)
					SSL_CTX_free(ctx);
				return NULL;
			}
		}
		PKCS12_free(p12);

		if (!SSL_CTX_use_certificate(ctx, cert))
			openssl_error_exit("SSL_CTX_use_certificate", 1);

		if (!SSL_CTX_use_PrivateKey(ctx, pkey))
			openssl_error_exit("SSL_CTX_use_PrivateKey", 1);

		if (!SSL_CTX_check_private_key(ctx))
			openssl_error_exit("SSL_CTX_check_private_key", 1);

		if (ca && sk_num(ca)) {
			for (i = 0; i < sk_X509_num(ca); i++) {
				if (!X509_STORE_add_cert(ctx->cert_store, sk_X509_value(ca, i)))
					openssl_error_exit("X509_STORE_add_cert", 1);
				
				if (!SSL_CTX_add_client_CA(ctx, sk_X509_value(ca, i)))
					openssl_error_exit("SSL_CTX_add_client_CA", 1);
			}
		}
	} else {
		if (SSL_CTX_use_certificate_file(ctx, pemfile, SSL_FILETYPE_PEM) <= 0)
			openssl_error_exit("SSL_CTX_use_certificate_file", 1);

		if (SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) <= 0)
			openssl_error_exit("SSL_CTX_use_PrivateKey_file", 1);

		if (SSL_CTX_check_private_key(ctx) <= 0)
			openssl_error_exit("SSL_CTX_check_private_key", 1);

		if (SSL_CTX_load_verify_locations(ctx, RCERTF, NULL) == 0)
			openssl_error_exit("SSL_CTX_new", 1);

		if ((cert_names = SSL_load_client_CA_file(RCERTF)) == NULL)
			openssl_error_exit("SSL_load_client_CA_file", 1);
	}
	
	SSL_CTX_set_client_CA_list(ctx, cert_names);
	SSL_CTX_use_certificate_chain_file(ctx, pemfile);
	SSL_CTX_set_verify_depth(ctx, 2);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_peer_func);
	SSL_CTX_set_info_callback(ctx, check_ssl_info);
	SSL_CTX_set_cipher_list(ctx, "RC4-SHA:DEC-CBC3-SHA:DES-CBC-SHA");
	ERR_clear_error();

	return ctx;
}

int openssl_biobase_client()
{
	int len;
	BIO *cbio, *fbio;
	char buf[BUFSIZE];

	fbio = BIO_new_fp(stdout, BIO_NOCLOSE);
	cbio = BIO_new_connect("localhost:9001");
	if (BIO_do_connect(cbio) <= 0)
		openssl_error_exit("BIO_do_connect", 1);

	while (1) {
		memset(buf, 0, sizeof(buf));
		printf("input message to send: ");
		do {
			fgets(buf, 1023, stdin);
		} while (!strcmp(buf, "\n"));

		len = BIO_puts(cbio, buf);
		if (len > 0) {
			printf("send message: %s\n", buf);
			if (!strncasecmp(buf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(1);
				break;
			}
		}

		memset(buf, 0, sizeof(buf));
		len = BIO_gets(fbio, buf, len);
		if (len > 0) {
			printf("receive message: %s", buf);
			if (!strncasecmp(buf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(3);
				break;
			}
		}
	}

	BIO_free(cbio);
	BIO_free(fbio);

	return 0;
}

int openssl_biobase_server()
{
	BIO *sbio;
	int sockfd, len;
	char *caddr, buf[BUFSIZE];

	sockfd = BIO_get_accept_socket("9001", 0);
	sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);

	while (1) {
		printf("\nwait for new connect......\n\n");
		
		int newfd = BIO_accept(sockfd, &caddr);
		if (newfd < 0)
			continue;
		else
			BIO_set_fd(sbio, newfd, BIO_NOCLOSE);
		
		if (fork() == 0) {
			printf("\nFork PID %d accept require!\n", getppid());
			while (1) {
				memset(buf, 0, sizeof(buf));
				len = BIO_read(sbio, buf, sizeof(buf) - 1);
				if (len > 0) {
					printf("receive message: %s", buf);
					if (!strncasecmp(buf, "quit", 4)) {
						printf("close the connect!\n");
						break;
					}
				}

				len = BIO_write(sbio, buf, sizeof(buf) - 1);
				if (len > 0) {
					printf("send message: %s\n", buf);
					if (!strncasecmp(buf, "quit", 4)) {
						printf("close the connect!\n");
						close(newfd);
						break;
					}
				}
			}
			exit(0);
		}
	}
	BIO_free(sbio);

	return 0;
}

int openssl_biossl_client()
{
	SSL *ssl;
	BIO *sbio;
	SSL_CTX *ctx;
	char buf[BUFSIZE];

	ctx = openssl_ctx_init(CLIMODE, CCERTF, CKEYF, "beike2012");
	sbio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(sbio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	BIO_set_conn_hostname(sbio, "localhost:9001");
	BIO_do_connect(sbio);
	BIO_do_handshake(sbio);
	
	BIO_puts(sbio, "GET / HTTP/1.0\n\n");
	memset(buf, 0, sizeof(buf));
	while (1) {
		if (BIO_read(sbio, buf, sizeof(buf) - 1) > 0) {
			printf("receive message: %s", buf);
			break;
		}
	}

	BIO_free_all(sbio);
	SSL_CTX_free(ctx);

	return 0;
}

int openssl_biossl_server()
{
	SSL *ssl;
	SSL_CTX *ctx;
	BIO *sbio, *abio;
	char buf[BUFSIZE];

	ctx = openssl_ctx_init(SERMODE, SCERTF, SKEYF, "beike2012");
	sbio = BIO_new_ssl(ctx, 0);
	BIO_get_ssl(sbio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	
	abio = BIO_new_accept("9001");
	BIO_set_accept_bios(abio, sbio);
	BIO_do_accept(abio);
	BIO_do_accept(abio);
	sbio = BIO_pop(abio);
	BIO_do_handshake(sbio);

	memset(buf, 0, sizeof(buf));
	while (1) {		
		if (BIO_read(sbio, buf, sizeof(buf) - 1) > 0) {
			printf("receive message: %s", buf);
			break;
		}
	}
	BIO_puts(sbio, "I hear you!\n\n");

	BIO_free_all(abio);
	BIO_free_all(sbio);
	SSL_CTX_free(ctx);

	return 0;
}

int openssl_bioadv_client()
{
	SSL *ssl;
	BIO *sbio;
	int sockfd;
	SSL_CTX *ctx;
	SSL_SESSION *sess;
	struct sockaddr_in saddr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		openssl_error_exit("socket", 0);

	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(9001);
	if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
		openssl_error_exit("connect", 0);

	ctx = openssl_ctx_init(CLIMODE, CCERTF, CKEYF, "beike2012");
	ssl = SSL_new(ctx);
	sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(sbio, "localhost:9001");
	SSL_set_bio(ssl, sbio, sbio);

	if (SSL_connect(ssl) <= 0)
		openssl_error_exit("SSL_connect", 1);

	check_cert_func(ssl);

	if (client_auth == RESHAKE) {
		sess = SSL_get1_session(ssl);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(sockfd);

		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			openssl_error_exit("socket", 0);

		bzero(&saddr, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		saddr.sin_port = htons(9001);
		if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
			openssl_error_exit("connect", 0);

		ssl = SSL_new(ctx);
		sbio = BIO_new_socket(sockfd, BIO_NOCLOSE);
		SSL_set_bio(ssl, sbio, sbio);
		SSL_set_session(ssl, sess);
		if (SSL_connect(ssl) <= 0)
			openssl_error_exit("SSL_connect", 1);
		check_cert_func(ssl);
	}
	//common_chat_func(ssl, sockfd);
	http_request_func(ssl, sockfd);

	SSL_CTX_free(ctx);

	return 0;
}

int openssl_bioadv_server()
{
	BIO *sbio;
	size_t len;
	SSL_CTX *ctx;
	struct sockaddr_in caddr, saddr;
	int sockfd, val = 1, sid_ctx = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		openssl_error_exit("socket", 0);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	len = sizeof(struct sockaddr_in);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9001);
	if (bind(sockfd, (struct sockaddr *)&saddr, len) < 0)
		openssl_error_exit("bind", 1);

	listen(sockfd, 5);

	client_auth = RESHAKE;
	ctx = openssl_ctx_init(SERMODE, SCERTF, SKEYF, "beike2012");
	SSL_CTX_set_session_id_context(ctx, (void *)&sid_ctx, sizeof(sid_ctx));

	while (1) {
		printf("\nWait for new connect......\n\n");
		
		int newfd = accept(sockfd, (struct sockaddr *)&caddr, &len);
		if (newfd < 0)
			openssl_error_exit("accept", 0);

		printf("Server: got connection from %s, port %d, socket %d\n",
			   inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);

		if (fork() == 0) {
			SSL *ssl;
			sbio = BIO_new_socket(newfd, BIO_NOCLOSE);
			ssl = SSL_new(ctx);
			SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
			SSL_set_bio(ssl, sbio, sbio);
			if (SSL_accept(ssl) <= 0)
				openssl_error_exit("SSL_accept", 1);

			http_response_func(ssl, newfd);
			printf("Close the connect [%s:%d]\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
			close(newfd);
			exit(0);
		}
	}

	close(sockfd);
	SSL_CTX_free(ctx);

	return 0;
}

int openssl_common_client()
{
	SSL *ssl;
	int sd, len;
	SSL_CTX *ctx;
	struct sockaddr_in saddr;
	char buf[BUFSIZE] = { 0 };

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		openssl_error_exit("socket", 0);

	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	saddr.sin_port = htons(9001);
	if (connect(sd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
		openssl_error_exit("connect", 0);

	ctx = openssl_ctx_init(CLIMODE, CCERTF, CKEYF, "beike2012");
	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sd);
	if (SSL_connect(ssl) < 0) ;
	openssl_error_exit("SSL_connect", 1);

	check_cert_func(ssl);

	while (1) {
		memset(buf, 0, sizeof(buf));
		printf("input message to send: ");
		do {
			fgets(buf, 1023, stdin);
		} while (!strcmp(buf, "\n"));

		len = SSL_write(ssl, buf, strlen(buf) - 1);
		if (len > 0) {
			printf("send message: %s\n", buf);
			if (!strncasecmp(buf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(1);
				break;
			}
		} else if (len < 0) {
			perror("SSL_write");
		}

		memset(buf, 0, sizeof(buf));
		len = SSL_read(ssl, buf, sizeof(buf) - 1);
		if (len > 0) {
			printf("receive message: %s", buf);
			if (!strncasecmp(buf, "quit", 4)) {
				printf("wait server close the connect!\n");
				sleep(3);
				break;
			}
		} else if (len < 0) {
			perror("SSL_read");
		}
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sd);
	SSL_CTX_free(ctx);

	return 0;
}

int openssl_common_server()
{
	size_t size;
	SSL_CTX *ctx;
	int sockfd, len;
	char buf[BUFSIZE];
	struct sigaction act;
	struct sockaddr_in caddr, saddr;

	act.sa_handler = child_signal_func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, 0);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		openssl_error_exit("socket", 0);

	size = sizeof(struct sockaddr_in);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(9001);

	if (bind(sockfd, (struct sockaddr *)&saddr, size) < 0)
		openssl_error_exit("bind", 0);

	if (listen(sockfd, 5) < 0)
		openssl_error_exit("listen", 0);

	ctx = openssl_ctx_init(SERMODE, SCERTF, SKEYF, "beike2012");

	while (1) {
		printf("\nwait for new connect......\n\n");

		SSL *ssl;
		int newfd = accept(sockfd, (struct sockaddr *)&caddr, &size);
		if (newfd < 0) {
			if (errno == EINTR || errno == ECONNABORTED) {
				continue;
			} else {
				perror("accept");
				return -1;
			}
		} else {
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, newfd);
			if (SSL_accept(ssl) < 0)
				openssl_error_exit("SSL_accept", 1);

			printf("\nget onnection from %s, port %d, socket %d\n",
				   inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), newfd);
			check_cert_func(ssl);
		}

		if (fork() == 0) {
			printf("Fork PID %d accept require!\n", getppid());
			while (1) {
				memset(buf, 0, sizeof(buf));
				len = SSL_read(ssl, buf, sizeof(buf) - 1);
				if (len > 0) {
					printf("[%s:%d]: %s\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), buf);
					if (!strncasecmp(buf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
							   inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						SSL_shutdown(ssl);
						SSL_free(ssl);
						close(newfd);
						break;
					}
				} else if (len < 0) {
					perror("recv");
				}

				memset(buf, 0, sizeof(buf));
				printf("input the message to send: ");
				do {
					fgets(buf, 1023, stdin);
				} while (!strcmp(buf, "\n"));

				len = SSL_write(ssl, buf, sizeof(buf) - 1);
				if (len > 0) {
					printf("send message: %s\n", buf);
					if (!strncasecmp(buf, "quit", 4)) {
						printf("close the connect [%s:%d]\n",
							   inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
						sleep(3);
						SSL_shutdown(ssl);
						SSL_free(ssl);
						close(newfd);
						break;
					}
				} else if (len < 0) {
					perror("send");
				}
			}

			exit(0);
		}
	}

	close(sockfd);
	SSL_CTX_free(ctx);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	openssl_error_exit("init", 2);
	switch (choice) {
		//check openssl bio base client.
	case 1:
		openssl_biobase_client();
		break;

		//check openssl  bio base server.
	case 2:
		openssl_biobase_server();
		break;

		//check openssl bio ssl client.
	case 3:
		openssl_biossl_client();
		break;

		//check openssl bio ssl server.
	case 4:
		openssl_biossl_server();
		break;

		//check openssl bio advance client.
	case 5:
		check_cacnf_func();
		openssl_bioadv_client();
		break;

		//check openssl bio advance server.
	case 6:
		openssl_bioadv_server();
		break;

		//check openssl common client.
	case 7:
		openssl_common_client();
		break;

		//check openssl commont server.
	case 8:
		openssl_common_server();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
