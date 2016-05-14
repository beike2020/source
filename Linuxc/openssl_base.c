/******************************************************************************
 * Function: 	some methods of openssl use.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall -lssl -lcrypt openssl_base.c -o openssl_base
*******************************************************************************/
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
#include <openssl/des.h>
#include <openssl/dh.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs7.h>
#include <openssl/pkcs12.h>
#include <openssl/objects.h>

#define  SCERTF 	"ssl/servercert.pem"
#define  SKEYF  	"ssl/serverkey.pem"
#define  CCERTF		"ssl/clientcert.pem"
#define  CKEYF   	"ssl/clientkey.pem"
#define  PCERTF		"ssl/pkcs12cert.pem"
#define  PKEYF   	"ssl/pkcs12key.pem"
#define  PKCS12F 	"ssl/commoncert.p12"
#define  RCERTF 	"ssl/cacert.pem"
#define  U1CERTF	"ssl/user1.cer"
#define  U2CERTF	"ssl/user2.cer"
#define  CRLCRL 	"ssl/crl.crl"

#define  COMM_LEN	128
#define  SHA1_LEN	160
#define  LINE_LEN 	512
#define  MAX1_LEN 	1024
#define  MAX2_LEN 	2048
#define  MAX3_LEN 	3072
#define  MAX4_LEN 	4096
#define  MAX5_LEN	5120

BIO *bio_err;

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: check openssl bio api.\n");
	printf(" 2: check openssl finger api.\n");
	printf(" 3: check openssl crypt api.\n");
	printf(" 4: check openssl evp api.\n");
	printf(" 5: check openssl x509 api.\n");
	printf(" 6: check openssl pkcs api.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void openssl_error_show(const char *string, int type)
{
	char buf[MAX1_LEN] = { 0 };

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
}

void openssl_bioBS_io()
{
	BIO *b;
	char *outs;
	int len1 = 0, len2 = 0;

	b = BIO_new(BIO_s_mem());
	BIO_write(b, "openssl ", 8);
	BIO_printf(b, "%s", "zcp");
	len1 = BIO_ctrl_pending(b);
	outs = (char *)OPENSSL_malloc(len1 + 1);
	BIO_read(b, outs, len1);
	printf("\nBIO term output: %s\n", outs);
	OPENSSL_free(outs);
	BIO_free(b);

	b = BIO_new_file("/tmp/test.txt", "w");
	BIO_write(b, "openssl ", 8);
	BIO_printf(b, "%s", "zcp");
	BIO_free(b);
	b = BIO_new_file("/tmp/test.txt", "r");
	len1 = BIO_ctrl_pending(b);
	outs = (char *)OPENSSL_malloc(len1 + 1);
	while ((len1 = BIO_read(b, outs + len2, 1)) > 0)
		len2 += len1;
	printf("\nBIO file output: %s\n", outs);
	free(outs);
	BIO_free(b);
}

void openssl_bioBS_md5()
{
	int size, len;
	BIO *fbio, *mbio;
	char buf[MAX1_LEN];

	fbio = BIO_new_file("/tmp/test.txt", "rb");
	mbio = BIO_new(BIO_f_md());
	BIO_set_md(mbio, EVP_sha1());
	fbio = BIO_push(mbio, fbio);
	mbio = BIO_new(BIO_f_md());
	BIO_set_md(mbio, EVP_md5());
	fbio = BIO_push(mbio, fbio);
	printf("\nBIO_MD5(%s) = ", "/tmp/test.txt");
	while ((len = BIO_read(fbio, buf, sizeof(buf) - 1)) > 0) {
		for (size = 0; size < len; size++)
			printf("%.02x", buf[size]);
	}
	printf("\n");
	BIO_free(mbio);
}

void openssl_bioBS_cipher()
{
	int len, i;
	BIO *bc, *bd;
	char outs[MAX1_LEN];
	const EVP_CIPHER *cd;
	unsigned char key[8], iv[8];

	for (i = 0; i < 8; i++) {
		memset(&key[i], i + 1, 1);
		memset(&iv[i], i + 1, 1);
	}

	bc = BIO_new(BIO_f_cipher());
	BIO_set_cipher(bc, EVP_des_ecb(), key, iv, 1);
	bd = BIO_new(BIO_s_null());
	bd = BIO_push(bc, bd);
	BIO_write(bd, "openssl", 7);
	len = BIO_read(bd, outs, MAX1_LEN);
	printf("\nBIO_CIPHER(%s) = ", "openssl");
	for (i = 0; i < len; i++)
		printf("%.02x", outs[i]);
	BIO_free(bd);
	BIO_free(bc);

	cd = EVP_des_ecb();
	bc = BIO_new(BIO_f_cipher());
	BIO_set_cipher(bc, cd, key, iv, 0);
	bd = BIO_new(BIO_s_null());
	bd = BIO_push(bc, bd);
	BIO_write(bc, outs, len);
	memset(outs, 0, sizeof(outs));
	BIO_read(bc, outs, MAX1_LEN);
	printf(" = %s\n", outs);
	BIO_free(bc);
	BIO_free(bd);
}

void openssl_bioBS_asnl()
{
	int len;
	BIO *bp;
	FILE *fp;
	char buf[MAX4_LEN];

	memset(buf, 0, sizeof(buf));
	bp = BIO_new(BIO_s_file());
	BIO_set_fp(bp, stdout, BIO_NOCLOSE);
	fp = fopen(RCERTF, "rb");
	len = fread(buf, 1, sizeof(buf) - 1, fp);
	fclose(fp);
	printf("\nBIO_ASNL info:\n");
	BIO_dump_indent(bp, buf, len, 5);
	BIO_free(bp);
}

void openssl_bioBS_random()
{
	int size;
	const char *p;
	unsigned char outs[SHA_DIGEST_LENGTH + 16] = { 0 };
	char buf[32], filename[COMM_LEN];

	strcpy(buf, "bioBS random");
	RAND_add(buf, 32, strlen(buf));
	strcpy(buf, "beike2012");
	RAND_seed(buf, 32);
	while (1) {
		if (RAND_status() == 1)
			break;
		else
			RAND_poll();
	}

	p = RAND_file_name(filename, COMM_LEN);
	RAND_write_file(p);
	RAND_load_file(p, MAX1_LEN);
	RAND_bytes(outs, sizeof(outs));
	printf("\nBIO_RANDOM() = ");
	for (size = 0; size < strlen((char *)&outs); size++)
		printf("%.02x", outs[size]);
	printf("\n");
	RAND_cleanup();
}

void openssl_bioBN_math()
{
	BIO *outs;
	BN_CTX *ctx;
	char num1[8], num2[8];
	BIGNUM *bg1, *bg2, *tmp, *stp;

	bg1 = BN_new();
	bg2 = BN_new();
	tmp = BN_new();
	ctx = BN_CTX_new();
	strcpy(num1, "84");
	strcpy(num2, "3");
	BN_hex2bn(&bg1, num1);
	BN_hex2bn(&bg2, num2);
	outs = BIO_new(BIO_s_file());
	BIO_set_fp(outs, stdout, BIO_NOCLOSE);

	printf("\nBIO_MATH as follow:\n");
	BN_add(tmp, bg1, bg2);
	BIO_puts(outs, "\tbn(0x84 + 0x3) = 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_sub(tmp, bg1, bg2);
	BIO_puts(outs, "\tbn(0x84 - 0x3) = 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_mul(tmp, bg1, bg2, ctx);
	BIO_puts(outs, "\tbn(0x84 * 0x3) = 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_sqr(tmp, bg1, ctx);
	BIO_puts(outs, "\tbn(sqr(0x84))  = 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_div(tmp, stp, bg1, bg2, ctx);
	BIO_puts(outs, "\tbn(0x84 / 0x3) = 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_exp(tmp, bg1, bg2, ctx);
	BIO_puts(outs, "\tbn(0x84 e 0x03)= 0x");
	BN_print(outs, tmp);
	BIO_puts(outs, "\n");

	BN_free(bg1);
	BN_free(bg2);
	BN_free(tmp);
	BIO_free(outs);
}

void openssl_md5_check()
{
	MD5_CTX ctx;
	int i, fd, len, size;
	char file_name[COMM_LEN], inputs[LINE_LEN];
	unsigned char tmps[COMM_LEN], outputs[MD5_DIGEST_LENGTH];

	memset(tmps, 0, sizeof(tmps));
	memset(inputs, 0, sizeof(inputs));
	memset(outputs, 0, sizeof(outputs));
	printf("\nPlease input the file name: ");
	scanf("%s", file_name);

	len = strlen(file_name);
	MD5((const unsigned char *)&file_name, len, tmps);
	printf("MD5(%s) = ", file_name);
	for (i = 0; i < strlen((char *)tmps); i++)
		printf("%02x", tmps[i]);
	printf("\n");

	fd = open(file_name, O_RDONLY);
	if (fd < 0)
		return;

	MD5_Init(&ctx);
	while ((size = read(fd, inputs, LINE_LEN)) > 0)
		MD5_Update(&ctx, (void *)inputs, size);
	MD5_Final(outputs, &ctx);
	close(fd);

	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
		printf("%02x", outputs[i]);
	printf("  %s\n", file_name);
}

void openssl_sha1_check()
{
	SHA_CTX s;
	int i, fd, len, size;
	char file_name[COMM_LEN], inputs[LINE_LEN];
	unsigned char tmps[COMM_LEN], outputs[SHA1_LEN / 8];

	memset(tmps, 0, sizeof(tmps));
	memset(inputs, 0, sizeof(inputs));
	memset(outputs, 0, sizeof(outputs));
	printf("\nPlease input a file name: ");
	scanf("%s", file_name);

	len = strlen(file_name);
	SHA1((const unsigned char *)&file_name, len, tmps);
	printf("SHA1(%s) = ", file_name);
	for (i = 0; i < strlen((char *)tmps); i++)
		printf("%02x", tmps[i]);
	printf("\n");

	fd = open(file_name, O_RDONLY);
	if (fd < 0)
		return;

	SHA1_Init(&s);
	while ((size = read(fd, inputs, LINE_LEN)) > 0)
		SHA1_Update(&s, inputs, size);
	SHA1_Final(outputs, &s);
	close(fd);

	for (i = 0; i < SHA1_LEN / 8; i++)
		printf("%02x", outputs[i]);
	printf("  %s\n", file_name);
}

void openssl_hmac_md5()
{
	int fd, size;
	unsigned int len;
	HMAC_CTX hmac_ctx;
	char file_name[COMM_LEN], inputs[COMM_LEN];
	unsigned char tmps[LINE_LEN], outputs[EVP_MAX_MD_SIZE];

	memset(tmps, 0, sizeof(tmps));
	memset(inputs, 0, sizeof(inputs));
	memset(outputs, 0, sizeof(outputs));
	printf("\nPlease input a file name: ");
	scanf("%s", file_name);
	fd = open(file_name, O_RDONLY);
	if (fd < 0)
		return;

	OpenSSL_add_all_digests();
	strcpy(inputs, "hmac_md5");
	HMAC_Init(&hmac_ctx, inputs, sizeof(inputs), EVP_get_digestbyname("md5"));
	while ((size = read(fd, tmps, LINE_LEN)) > 0)
		HMAC_Update(&hmac_ctx, tmps, size);
	HMAC_Final(&hmac_ctx, outputs, &len);
	HMAC_cleanup(&hmac_ctx);
	close(fd);

	printf("HMAC_MD5(%s, %s) = ", file_name, inputs);
	for (size = 0; size < len; size++)
		printf("%02x", outputs[size]);
	printf("\n");
}

void openssl_ctx_compress()
{
	COMP_CTX *ctx;
	int len, size, total;
	unsigned char inputs[COMM_LEN] = "ctx_compress";
	unsigned char tmps[COMM_LEN], outputs[COMM_LEN];

	memset(tmps, 0, sizeof(tmps));
	memset(outputs, 0, sizeof(outputs));
	ctx = COMP_CTX_new(COMP_zlib());
	total = COMP_compress_block(ctx, outputs, COMM_LEN, inputs, COMM_LEN);
	len = COMP_expand_block(ctx, tmps, sizeof(tmps), outputs, total);
	COMP_CTX_free(ctx);

	printf("\nCompress(%s) = ", inputs);
	for (size = 0; size < total; size++)
		printf("%02x", outputs[size]);
	printf("\n");
}

int openssl_random_gen(int scale)
{
	int fd;
	unsigned int i, ur, ticks;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return -1;

	ticks = (unsigned int)time(0);
	for (i = 0; i < LINE_LEN; i++) {
		read(fd, &ur, sizeof(ur));
		ticks += ur;
	}
	close(fd);
	srand(ticks);

	if (scale)
		return rand() % RAND_MAX;
	else
		return rand();
}

void openssl_salt_encrypt()
{
	char inputs[COMM_LEN] = "salt encrypt";
	char salts[COMM_LEN], outputs[COMM_LEN];

	memset(outputs, 0, sizeof(outputs));
	snprintf(salts, sizeof(salts), "%d", openssl_random_gen(0));
	strcpy(outputs, (char *)crypt(inputs, salts));
	printf("CRYPT(%s, %s) = %s\n", inputs, salts, outputs);
}

void openssl_des_crypt()
{
	int size;
	DES_cblock key;
	DES_cblock outputs;
	const_DES_cblock inputs;
	DES_key_schedule schedule;
	unsigned char tmp[16] = "des crypt";

	DES_random_key(&key);
	DES_string_to_key("beike2012", &key);
	DES_set_odd_parity(&key);
	des_check_key_parity(&key);
	DES_set_key_checked(&key, &schedule);
	DES_is_weak_key((const_DES_cblock *)tmp);
	DES_ecb_encrypt((const_DES_cblock *)tmp, &outputs, &schedule, DES_ENCRYPT);
	printf("\nDES_ecb_encrypt(%s) = ", tmp);
	for (size = 0; size < sizeof(outputs); size++)
		printf("%02x", outputs[size]);
	printf("\n");

	DES_ecb_encrypt(&outputs, &inputs, &schedule, DES_DECRYPT);
	printf("DES_ecb_decrypt(");
	for (size = 0; size < sizeof(outputs); size++)
		printf("%02x", outputs[size]);
	printf(") = %s\n", inputs);
}

void openssl_rsa_crypt()
{
	RSA *r;
	BIO *b;
	BIGNUM *bne;
	unsigned int len;
	int size, elen, dlen;
	unsigned char inputs[COMM_LEN] = "rsa crypt";
	unsigned char tmps[MAX1_LEN], outputs[MAX1_LEN];

	memset(tmps, 0, sizeof(tmps));
	memset(outputs, 0, sizeof(outputs));
	printf("\nRSA generate key:\n");
	bne = BN_new();
	BN_set_word(bne, RSA_3);
	r = RSA_new();
	RSA_generate_key_ex(r, MAX1_LEN, bne, NULL);
	RSA_print_fp(stdout, r, 11);
	b = BIO_new_file("/tmp/rsa.key", "w");
	i2d_RSAPrivateKey_bio(b, r);
	BIO_free(b);

	elen = RSA_private_encrypt(RSA_size(r) - 11,
							   inputs, outputs, r, RSA_PKCS1_PADDING);
	dlen = RSA_public_decrypt(elen, outputs, tmps, r, RSA_PKCS1_PADDING);
	if (elen <= 0 || dlen <= 0 || memcmp(inputs, tmps, RSA_size(r) - 11)) {
		printf("RSA_private_encrypt error!\n");
		RSA_free(r);
		return;
	}
	printf("RSA_private_encrypt(%s) = ", inputs);
	for (size = 0; size < elen; size++)
		printf("%02x", outputs[size]);
	printf("\n");

	memset(outputs, 0, sizeof(outputs));
	elen = RSA_public_encrypt(RSA_size(r) - 11,
							  inputs, outputs, r, RSA_PKCS1_PADDING);
	dlen = RSA_private_decrypt(elen, outputs, tmps, r, RSA_PKCS1_PADDING);
	if (elen <= 0 || dlen <= 0 || memcmp(inputs, tmps, RSA_size(r) - 11)) {
		printf("RSA_public_encrypt error!\n");
		RSA_free(r);
		return;
	}
	printf("RSA_public_encrypt(%s) = ", inputs);
	for (size = 0; size < elen; size++)
		printf("%02x", outputs[size]);
	printf("\n");

	memset(outputs, 0, sizeof(outputs));
	RSA_sign(NID_md5_sha1, inputs, 36, outputs, &len, r);
	printf("RSA_sign(%s) = ", inputs);
	for (size = 0; size < len; size++)
		printf("%02x", outputs[size]);
	printf("\n");

	memset(tmps, 0, sizeof(tmps));
	RSA_verify(NID_md5_sha1, inputs, 36, outputs, len, r);
	printf("RSA_verify(");
	for (size = 0; size < len; size++)
		printf("%02x", outputs[size]);
	printf(") = %s\n", inputs);

	RSA_free(r);
}

void openssl_dsa_crypt()
{
	DSA *d;
	unsigned int size, len;
	unsigned char inputs[COMM_LEN] = "dsa crypt";
	unsigned char outputs[MAX1_LEN] = { 0 };

	printf("\nDSA generate key:\n");
	d = DSA_new();
	DSA_generate_parameters_ex(d, LINE_LEN, NULL, 0, NULL, NULL, NULL);
	DSA_generate_key(d);
	DSA_print_fp(stdout, d, 0);
	
	DSA_sign(NID_md5_sha1, inputs, 20, outputs, &len, d);
	printf("DSA_sign(%s) = ", inputs);
	for (size = 0; size < len; size++)
		printf("%.02x", outputs[size]);
	printf("\n");
	
	DSA_verify(NID_md5_sha1, inputs, 20, outputs, len, d);
	printf("DSA_verify(");
	for (size = 0; size < len; size++)
		printf("%.02x", outputs[size]);
	printf(") = %s\n", inputs);
	
	DSA_free(d);
}

void openssl_dh_crypt()
{
	BIO *b;
	DH *d1, *d2;
	int i, len1, len2;
	unsigned char skey1[COMM_LEN], skey2[COMM_LEN];

	d1 = DH_new();
	d2 = DH_new();
	DH_generate_parameters_ex(d1, 64, DH_GENERATOR_2, NULL);
	DH_check(d1, &i);
	printf("\nDH key size: %d\n", DH_size(d1));
	DH_generate_key(d1);

	d2->p = BN_dup(d1->p);
	d2->g = BN_dup(d1->g);
	DH_generate_key(d2);
	DH_check_pub_key(d1, d1->pub_key, &i);
	len1 = DH_compute_key(skey1, d2->pub_key, d1);
	len2 = DH_compute_key(skey2, d1->pub_key, d2);
	if ((len1 != len2) || (memcmp(skey1, skey2, len1) != 0)) {
		printf("DH_compute_key err!\n");
		DH_free(d1);
		DH_free(d2);
		return;
	}

	b = BIO_new(BIO_s_file());
	BIO_set_fp(b, stdout, BIO_NOCLOSE);
	DHparams_print(b, d1);

	BIO_free(b);
	DH_free(d1);
	DH_free(d2);
}

void openssl_ec_crypt()
{
	BIO *berr;
	EC_KEY *key1, *key2;
	unsigned int sig_len;
	int clen, len1, len2;
	EC_builtin_curve *curves;
	EC_GROUP *group1, *group2;
	const EC_KEY *key3, *key4;
	const EC_GROUP *group3, *group4;
	const EC_POINT *pubkey1, *pubkey2;
	unsigned char shareKey1[COMM_LEN], shareKey2[COMM_LEN];
	unsigned char *signature, cont[COMM_LEN] = "123456";

	key1 = EC_KEY_new();
	key2 = EC_KEY_new();
	clen = EC_get_builtin_curves(NULL, 0);
	curves = (EC_builtin_curve *) malloc(sizeof(EC_builtin_curve) * clen);
	EC_get_builtin_curves(curves, clen);
	group1 = EC_GROUP_new_by_curve_name(curves[25].nid);
	group2 = EC_GROUP_new_by_curve_name(curves[25].nid);
	group3 = group1;
	group4 = group2;
	EC_KEY_set_group(key1, group3);
	EC_KEY_set_group(key2, group4);
	EC_KEY_generate_key(key1);
	EC_KEY_generate_key(key2);
	EC_KEY_check_key(key1);

	key3 = key1;
	key4 = key2;
	printf("\nECDSA_size: %d\n", ECDSA_size(key3));
	signature = (unsigned char *)malloc(ECDSA_size(key3));
	ERR_load_crypto_strings();
	berr = BIO_new(BIO_s_file());
	BIO_set_fp(berr, stdout, BIO_NOCLOSE);
	ECDSA_sign(0, cont, 8, signature, &sig_len, key1);
	ECDSA_verify(0, cont, 8, signature, sig_len, key1);

	pubkey1 = EC_KEY_get0_public_key(key1);
	pubkey2 = EC_KEY_get0_public_key(key2);
	len1 = ECDH_compute_key(shareKey1, COMM_LEN, pubkey2, key1, NULL);
	len2 = ECDH_compute_key(shareKey2, COMM_LEN, pubkey1, key1, NULL);
	if (len1 != len2 || memcmp(shareKey1, shareKey2, len1) != 0) {
		printf("ECDH_compute_key err!\n");
		return;
	}

	BIO_free(berr);
	EC_KEY_free(key1);
	EC_KEY_free(key2);
	free(signature);
	free(curves);
}

void openssl_rsa_pemkey()
{
	long len;
	BIGNUM *bne;
	BIO *ins, *outs;
	RSA *r, *read;
	char *name, *head;
	unsigned char *data;
	const EVP_CIPHER *enc;
	EVP_CIPHER_INFO cipher;

	OpenSSL_add_all_algorithms();

	bne = BN_new();
	BN_set_word(bne, RSA_3);
	r = RSA_new();
	RSA_generate_key_ex(r, LINE_LEN, bne, NULL);

	enc = EVP_des_ede3_ofb();
	outs = BIO_new_file("/tmp/pri.pem", "w");
	PEM_write_bio_RSAPrivateKey(outs, r, enc, NULL, 0, NULL, "beike2012");
	BIO_free(outs);

	outs = BIO_new_file("/tmp/pub.pem", "w");
	PEM_write_bio_RSAPublicKey(outs, r);
	BIO_free(outs);

	ins = BIO_new_file("/tmp/pri.pem", "rb");
	r = RSA_new();
	read = PEM_read_bio_RSAPrivateKey(ins, &r, NULL, "beike2012");
	if (read->d == NULL) {
		printf("PEM_read_bio_RSAPrivateKey err!\n");
		return;
	}

	printf("\nEVP_CIPHER_INFO:\n");
	while (1) {
		if (PEM_read_bio(ins, &name, &head, &data, &len) == 0)
			break;

		if (strlen(head) > 0) {
			PEM_get_EVP_CIPHER_INFO(head, &cipher);
			if (PEM_do_header(&cipher, data, &len, NULL, NULL) == 0)
				return;
			printf("name=%s, head=%s, data=%s\n", name, head, data);
		}

		OPENSSL_free(name);
		OPENSSL_free(head);
		OPENSSL_free(data);
	}

	RSA_free(read);
	BIO_free(ins);
}

void openssl_evp_encode()
{
	FILE *fin, *fout;
	int inLen, outLen;
	EVP_ENCODE_CTX ctx;
	unsigned char ins[MAX1_LEN], outs[MAX2_LEN];

	fin = fopen("/tmp/test.dat", "rb");
	fout = fopen("/tmp/test.txt", "w");
	memset(ins, 0, sizeof(ins));
	memset(outs, 0, sizeof(outs));
	printf("\nEVP_Encode(/tmp/test.dat) = ");
	EVP_EncodeInit(&ctx);
	while ((inLen = fread(ins, 1, MAX1_LEN, fin)) > 0) {
		EVP_EncodeUpdate(&ctx, outs, &outLen, ins, inLen);
		fwrite(outs, 1, outLen, fout);
		printf("%s", outs);
	}
	EVP_EncodeFinal(&ctx, outs, &outLen);
	fwrite(outs, 1, outLen, fout);
	printf("%s", outs);
	fclose(fin);
	fclose(fout);

	fin = fopen("/tmp/test.txt", "r");
	fout = fopen("/tmp/test-1.dat", "wb");
	memset(ins, 0, sizeof(ins));
	memset(outs, 0, sizeof(outs));
	printf("\nEVP_Decode(/tmp/test.txt) = ");
	EVP_DecodeInit(&ctx);
	while ((inLen = fread(ins, 1, MAX1_LEN, fin)) > 0) {
		EVP_DecodeUpdate(&ctx, outs, &outLen, ins, inLen);
		fwrite(outs, 1, outLen, fout);
		printf("%s", outs);
	}
	EVP_DecodeFinal(&ctx, outs, &outLen);
	fwrite(outs, 1, outLen, fout);
	printf("%s\n", outs);
	fclose(fin);
	fclose(fout);
}

void openssl_evp_comcrypt()
{
	EVP_CIPHER_CTX ctx;
	int i, len1 = 0, len2 = 0, len3 = 0;
	unsigned char outs[MAX1_LEN], des[MAX1_LEN];
	unsigned char msg[MAX1_LEN] = "openssl common encrypt test";
	unsigned char iv[EVP_MAX_KEY_LENGTH], key[EVP_MAX_KEY_LENGTH];

	for (i = 0; i < 24; i++)
		key[i] = i;
	for (i = 0; i < 8; i++)
		iv[i] = i;
	memset(des, 0, sizeof(des));
	memset(outs, 0, sizeof(outs));

	EVP_CIPHER_CTX_init(&ctx);
	EVP_CipherInit(&ctx, EVP_des_ede3_cbc(), key, iv, 1);
	EVP_CipherUpdate(&ctx, outs, &len1, msg, strlen((char *)msg));
	EVP_CipherFinal(&ctx, outs + len1, &len3);
	len1 += len3;
	printf("\nEVP_COMEncry (%s) = ", msg);
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", outs[i]);
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	EVP_CIPHER_CTX_init(&ctx);
	EVP_CipherInit(&ctx, EVP_des_ede3_cbc(), key, iv, 0);
	EVP_CipherUpdate(&ctx, des, &len2, outs, len1);
	EVP_CipherFinal(&ctx, des + len2, &len3);
	len2 += len3;
	printf("\nEVP_COMDecry (");
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", outs[i]);
	printf(") = %s\n", des);
	EVP_CIPHER_CTX_cleanup(&ctx);
}

void openssl_evp_symcrypt()
{
	EVP_CIPHER_CTX ctx;
	int i, len1 = 0, len2 = 0, len3 = 0;
	unsigned char outs[MAX1_LEN], des[MAX1_LEN];
	unsigned char iv[EVP_MAX_KEY_LENGTH], key[EVP_MAX_KEY_LENGTH];
	unsigned char msg[MAX1_LEN] = "openssl symmetric encrypt test";

	for (i = 0; i < 24; i++)
		key[i] = i;
	for (i = 0; i < 8; i++)
		iv[i] = i;
	memset(des, 0, sizeof(des));
	memset(outs, 0, sizeof(outs));

	OpenSSL_add_all_algorithms();
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit(&ctx, EVP_des_ede3_cbc(), key, iv);
	EVP_EncryptUpdate(&ctx, outs, &len1, msg, strlen((char *)msg));
	EVP_EncryptFinal(&ctx, outs + len1, &len3);
	len1 = len1 + len3;
	printf("\nEVP_SYNEncry (%s) = ", msg);
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", outs[i]);
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	EVP_CIPHER_CTX_init(&ctx);
	EVP_DecryptInit(&ctx, EVP_des_ede3_cbc(), key, iv);
	EVP_DecryptUpdate(&ctx, des, &len2, outs, len1);
	EVP_DecryptFinal(&ctx, des + len2, &len3);
	len2 += len3;
	printf("\nEVP_SYNDecry (");
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", outs[i]);
	printf(") = %s\n", des);
	EVP_CIPHER_CTX_cleanup(&ctx);
}

void openssl_evp_asycrypt()
{
	RSA *rkey;
	BIGNUM *bne;
	EVP_PKEY *pubkey[2];
	EVP_CIPHER_CTX ctx1, ctx2;
	int i, ekl[2], len1 = 0, len2 = 0, len3 = 0;
	unsigned char ins[] = "openssl asymmetric encrypt test";
	unsigned char iv[8], pen[MAX1_LEN], *ek[2], sde[MAX1_LEN];

	ek[0] = (unsigned char *)malloc(MAX1_LEN);
	ek[1] = (unsigned char *)malloc(MAX1_LEN);
	memset(pen, 0, MAX1_LEN);
	memset(sde, 0, MAX1_LEN);
	memset(ek[0], 0, MAX1_LEN);
	memset(ek[1], 0, MAX1_LEN);

	bne = BN_new();
	BN_set_word(bne, RSA_3);
	rkey = RSA_new();
	RSA_generate_key_ex(rkey, MAX1_LEN, bne, NULL);
	pubkey[0] = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pubkey[0], rkey);

	EVP_CIPHER_CTX_init(&ctx1);
	EVP_SealInit(&ctx1, EVP_des_ede3_cbc(), ek, ekl, iv, pubkey, 1);
	EVP_SealUpdate(&ctx1, pen, &len1, ins, strlen((char *)ins));
	EVP_SealFinal(&ctx1, pen + len1, &len3);
	len1 += len3;
	printf("\nEVP_ASYEncry(%s) = ", ins);
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", pen[i]);
	printf("\n");
	EVP_CIPHER_CTX_cleanup(&ctx1);

	len3 = 0;
	EVP_CIPHER_CTX_init(&ctx2);
	EVP_OpenInit(&ctx2, EVP_des_ede3_cbc(), ek[0], ekl[0], iv, pubkey[0]);
	EVP_OpenUpdate(&ctx2, sde, &len2, pen, len1);
	EVP_OpenFinal(&ctx2, sde + len2, &len3);
	len2 += len3;
	printf("EVP_ASYDecry(");
	for (i = 0; i < len1; i++)
		printf("0x%.02x ", pen[i]);
	printf(") = %s\n", sde);
	EVP_CIPHER_CTX_cleanup(&ctx2);

	free(ek[0]);
	free(ek[1]);
	EVP_PKEY_free(pubkey[0]);
	BN_free(bne);
}

void openssl_evp_rsacripher()
{
	RSA *rkey;
	BIGNUM *bne;
	EVP_PKEY *pubkey[2];
	const EVP_CIPHER *type;
	EVP_CIPHER_CTX ctx1, ctx2;
	int i, ekl[2], total = 0, len1 = 0, len2 = 0;
	const unsigned char ins[COMM_LEN] = "openssl evp";
	unsigned char outs[LINE_LEN], iv[8], *ek[2], de[LINE_LEN];

	bne = BN_new();
	BN_set_word(bne, RSA_3);
	rkey = RSA_new();
	RSA_generate_key_ex(rkey, MAX1_LEN, bne, NULL);
	pubkey[0] = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pubkey[0], rkey);
	type = EVP_des_cbc();

	ek[0] = malloc(LINE_LEN);
	ek[1] = malloc(LINE_LEN);
	EVP_CIPHER_CTX_init(&ctx1);
	EVP_SealInit(&ctx1, type, ek, ekl, iv, pubkey, 1);
	EVP_SealUpdate(&ctx1, outs, &total, ins, 11);
	EVP_SealFinal(&ctx1, outs + total, &len1);
	total += len1;
	printf("\nEVP_RSASEAL(%s) = ", ins);
	for (i = 0; i < total; i++)
		printf("0x%.02x ", outs[i]);
	EVP_CIPHER_CTX_cleanup(&ctx1);
	
	memset(de, 0, LINE_LEN);
	EVP_CIPHER_CTX_init(&ctx2);
	EVP_OpenInit(&ctx2, EVP_des_cbc(), ek[0], ekl[0], iv, pubkey[0]);
	EVP_OpenUpdate(&ctx2, de, &len2, outs, total);
	EVP_OpenFinal(&ctx2, de + len2, &len1);
	len2 += len1;
	printf("= %s\n", de);
	EVP_CIPHER_CTX_cleanup(&ctx2);

	free(ek[0]);
	free(ek[1]);
	EVP_PKEY_free(pubkey[0]);
	BN_free(bne);
}

void openssl_evp_digest()
{
	EVP_MD_CTX mdctx;
	unsigned int md_len, i;
	char author[] = "beike";
	char msg[] = "openssl digest";
	unsigned char md_value[EVP_MAX_MD_SIZE];

	OpenSSL_add_all_algorithms();
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, EVP_md5(), NULL);
	EVP_DigestUpdate(&mdctx, msg, strlen(msg));
	EVP_DigestUpdate(&mdctx, author, strlen(author));
	EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
	printf("\nEVP_Digest(%s, %s) = ", msg, author);
	for (i = 0; i < md_len; i++)
		printf("0x%02x ", md_value[i]);
	printf("\n");
	EVP_MD_CTX_cleanup(&mdctx);
}

void openssl_evp_comsign()
{
	RSA *rsa;
	EVP_PKEY *evpKey;
	EVP_MD_CTX mdctx;
	unsigned int i, len;
	char ins[MAX1_LEN] = "openssl signature";
	unsigned char outs[MAX1_LEN];

	OpenSSL_add_all_algorithms();

	rsa = RSA_generate_key(MAX1_LEN, RSA_F4, NULL, NULL);
	evpKey = EVP_PKEY_new();
	EVP_PKEY_set1_RSA(evpKey, rsa);
	EVP_MD_CTX_init(&mdctx);
	EVP_SignInit_ex(&mdctx, EVP_md5(), NULL);
	EVP_SignUpdate(&mdctx, ins, strlen(ins));
	EVP_SignFinal(&mdctx, outs, &len, evpKey);
	printf("\nEVP_COMSignature(%s) = ", ins);
	for (i = 0; i < len; i++)
		printf("0x%02x ", outs[i]);
	printf("\n");
	EVP_MD_CTX_cleanup(&mdctx);

	EVP_MD_CTX_init(&mdctx);
	EVP_VerifyInit_ex(&mdctx, EVP_md5(), NULL);
	EVP_VerifyUpdate(&mdctx, ins, strlen(ins));
	if (EVP_VerifyFinal(&mdctx, outs, len, evpKey) == 1)
		printf("EVP_COMVerify OK!\n");

	EVP_MD_CTX_cleanup(&mdctx);
	EVP_PKEY_free(evpKey);
	RSA_free(rsa);
}

void openssl_evp_rsasign()
{
	int inlen;
	RSA *rkey;
	BIGNUM *bne;
	EVP_PKEY *pkey;
	char data[COMM_LEN];
	EVP_MD_CTX mdctx;
	unsigned char outs[LINE_LEN];
	unsigned int i, outlen = 0;

	strcpy(data, "openssl rsasign");
	inlen = strlen(data);
	bne = BN_new();
	BN_set_word(bne, RSA_3);
	rkey = RSA_new();
	RSA_generate_key_ex(rkey, MAX1_LEN, bne, NULL);

	pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, rkey);
	EVP_MD_CTX_init(&mdctx);
	EVP_SignInit_ex(&mdctx, EVP_md5(), NULL);
	EVP_SignUpdate(&mdctx, data, inlen);
	EVP_SignFinal(&mdctx, outs, &outlen, pkey);
	printf("\nEVP_RSASignature(%s) = ", data);
	for (i = 0; i < outlen; i++)
		printf("0x%02x ", outs[i]);
	printf("\n");
	EVP_MD_CTX_cleanup(&mdctx);

	EVP_MD_CTX_init(&mdctx);
	EVP_VerifyInit_ex(&mdctx, EVP_md5(), NULL);
	EVP_VerifyUpdate(&mdctx, data, inlen);
	if (EVP_VerifyFinal(&mdctx, outs, outlen, pkey) == 1)
		printf("EVP_RSAVerify OK!\n");
	EVP_MD_CTX_cleanup(&mdctx);

	RSA_free(rkey);
	BN_free(bne);
}

void openssl_evp_keyiv()
{
	int i;
	const EVP_MD *md;
	const EVP_CIPHER *type;
	unsigned char salt[32], data[COMM_LEN], *key, *iv;

	md = EVP_md5();
	printf("\nEVP_Md info: type[%d], ", EVP_MD_type(md));
	printf("nid[%d], ", EVP_MD_nid(md));
	printf("name[%s], ", EVP_MD_name(md));
	printf("pkey type[%d], ", EVP_MD_pkey_type(md));
	printf("size[%d], ", EVP_MD_size(md));
	printf("block size[%d], ", EVP_MD_block_size(md));

	type = EVP_des_ecb();
	printf("\nEVP_ECB info: encrypto nid[%d], ", EVP_CIPHER_nid(type));
	printf("name[%s], ", EVP_CIPHER_name(type));
	printf("bock size[%d]", EVP_CIPHER_block_size(type));

	key = (unsigned char *)malloc(EVP_CIPHER_key_length(type));
	iv = (unsigned char *)malloc(EVP_CIPHER_iv_length(type));
	for (i = 0; i < COMM_LEN; i++)
		memset(&data[i], i, 1);
	for (i = 0; i < 32; i++)
		memset(&salt[i], i, 1);

	EVP_BytesToKey(type, md, salt, data, COMM_LEN, 2, key, iv);
	printf("\nEVP_key value: ");
	for (i = 0; i < EVP_CIPHER_key_length(type); i++)
		printf("%x ", key[i]);

	printf("\nEVP_iv value: ");
	for (i = 0; i < EVP_CIPHER_iv_length(type); i++)
		printf("%x ", iv[i]);
	printf("\n");
}

int openssl_x509_algorithm(BIO * bp, X509_ALGOR * sign)
{
	int nid;
	PBEPARAM *pbe;
	const unsigned char *p;

	nid = OBJ_obj2nid(sign->algorithm);
	switch (nid) {
	case NID_md5WithRSAEncryption:
		printf("md5WithRSAEncryption");
		break;
	case NID_sha1WithRSAEncryption:
		printf("sha1WithRSAEncryption");
		break;
	case NID_rsaEncryption:
		printf("rsaEncryption");
		break;
	case NID_sha1:
		printf("sha1");
		break;
	case NID_pbe_WithSHA1And3_Key_TripleDES_CBC:
		printf("NID_pbe_WithSHA1And3_Key_TripleDES_CBC");
		break;
	default:
		printf("unknown signature.");
		break;
	}

	if (sign->parameter != NULL) {
		if (nid == NID_pbe_WithSHA1And3_Key_TripleDES_CBC) {
			printf("parameter:\n");
			p = sign->parameter->value.sequence->data;
			d2i_PBEPARAM(&pbe, &p, sign->parameter->value.sequence->length);
			printf("salt: \n");
			i2a_ASN1_INTEGER(bp, pbe->salt);
			printf("\n");
			printf("iter: %ld\n", ASN1_INTEGER_get(pbe->iter));
		}
	}
	printf("\n");

	return 0;
}

int openssl_x509_cert()
{
	BIO *b;
	FILE *fp;
	RSA *rsa;
	EVP_PKEY *pkey;
	X509_NAME *name;
	const EVP_MD *md;
	X509_REQ *req, **req2;
	X509_NAME_ENTRY *entry;
	unsigned int len;
	char bytes[COMM_LEN];
	const unsigned char *pp;
	unsigned char *p, *der, *mdout, buf[MAX1_LEN];

	OpenSSL_add_all_algorithms();
	printf("\nX509_Cert info:\n");
	return -1;
	
	req = X509_REQ_new();
	X509_REQ_set_version(req, 1);

	name = X509_NAME_new();
	strcpy(bytes, "beike");
	entry = X509_NAME_ENTRY_create_by_txt(&entry, "commonName",
 		V_ASN1_UTF8STRING, (unsigned char *)bytes, strlen(bytes));
	X509_NAME_add_entry(name, entry, 0, -1);
	strcpy(bytes, "BEIJING");
	entry = X509_NAME_ENTRY_create_by_txt(&entry, "countryName",
		V_ASN1_UTF8STRING, (unsigned char *)bytes, strlen(bytes));
	X509_NAME_add_entry(name, entry, 1, -1);
	X509_REQ_set_subject_name(req, name);

	pkey = EVP_PKEY_new();
	rsa = RSA_generate_key(LINE_LEN, RSA_3, NULL, NULL);
	EVP_PKEY_assign_RSA(pkey, rsa);
	X509_REQ_set_pubkey(req, pkey);

	strcpy(bytes, "USTB");
	X509_REQ_add1_attr_by_txt(req, "organizationName",
		V_ASN1_UTF8STRING, (unsigned char *)bytes, strlen(bytes));
	strcpy(bytes, "TECH");
	X509_REQ_add1_attr_by_txt(req, "organizationalUnitName",
		V_ASN1_UTF8STRING, (unsigned char *)bytes, strlen(bytes));

	md = EVP_sha1();
	X509_REQ_digest(req, md, mdout, &len);
	X509_REQ_sign(req, pkey, md);
	b = BIO_new_file("/tmp/certreq.txt", "w");
	PEM_write_bio_X509_REQ(b, req);
	BIO_free(b);

	len = i2d_X509_REQ(req, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_X509_REQ(req, &p);
	X509_REQ_verify(req, pkey);
	fp = fopen("/tmp/certder.txt", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);

	free(der);
	X509_REQ_free(req);

	b = BIO_new_file("/tmp/certreq.txt", "r");
	PEM_read_bio_X509_REQ(b, NULL, NULL, NULL);

	fp = fopen("/tmp/certder.txt", "r");
	len = fread(buf, 1, MAX1_LEN, fp);
	fclose(fp);
	pp = buf;
	req2 = (X509_REQ **) malloc(sizeof(X509_REQ *));
	d2i_X509_REQ(req2, &pp, len);

	free(req2);
	X509_REQ_free(*req2);

	return 0;
}

void openssl_x509_purpose()
{
	BIO *b;
	int len;
	FILE *fp;
	X509 *x, *y;
	const unsigned char *p;
	unsigned char buf[MAX5_LEN];

	fp = fopen(U1CERTF, "rb");
	len = fread(buf, 1, MAX5_LEN, fp);
	fclose(fp);
	p = buf;
	x = X509_new();
	y = d2i_X509(&x, &p, len);
	b = BIO_new(BIO_s_file());
	BIO_set_fp(b, stdout, BIO_NOCLOSE);
	
	printf("\nX509_Purpose info:\n");
	X509_print(b, x);
	X509_check_purpose(x, X509_PURPOSE_OCSP_HELPER, 0);

	BIO_free(b);
	X509_free(x);
}

void openssl_x509_verify()
{
	FILE *fp;
	int uCert1Len;
	X509_CRL *Crl = NULL;
	X509_STORE_CTX *ctx = NULL;
	X509_STORE *rootCertStore = NULL;
	STACK_OF(X509) * caCertStack = NULL;
	X509 *usrCert1 = NULL, *usrCert2 = NULL, *rootCert = NULL;
	unsigned char tmp[MAX4_LEN];
	unsigned long uCert2Len, derCrlLen, derRootCertLen;
	const unsigned char *derCrl, *derRootCert, *uCert1, *uCert2;

	OpenSSL_add_all_algorithms();
	printf("\nX509_Verify info:\n");

	fp = fopen(RCERTF, "rb");
	derRootCertLen = fread(tmp, 1, 4096, fp);
	derRootCert = tmp;
	fclose(fp);
	rootCert = d2i_X509(NULL, &derRootCert, derRootCertLen);

	fp = fopen(CRLCRL, "rb");
	derCrlLen = fread(tmp, 1, 4096, fp);
	derCrl = tmp;
	fclose(fp);
	Crl = d2i_X509_CRL(NULL, &derCrl, derCrlLen);

	rootCertStore = X509_STORE_new();
	X509_STORE_add_cert(rootCertStore, rootCert);
	X509_STORE_set_flags(rootCertStore, X509_V_FLAG_CRL_CHECK);
	X509_STORE_add_crl(rootCertStore, Crl);
	ctx = X509_STORE_CTX_new();

	fp = fopen(U1CERTF, "rb");
	uCert1Len = fread(tmp, 1, 4096, fp);
	uCert1 = tmp;
	fclose(fp);
	usrCert1 = d2i_X509(NULL, &uCert1, uCert1Len);
	if (X509_STORE_CTX_init(ctx, rootCertStore, usrCert1, caCertStack) != 1) {
		perror("X509_STORE_CTX_init");
		return;
	}
	if (X509_verify_cert(ctx) != 1) {
		printf("user1.cer %s\n", X509_verify_cert_error_string(ctx->error));
		return;
	}

	fp = fopen(U2CERTF, "rb");
	uCert2Len = fread(tmp, 1, 4096, fp);
	uCert2 = tmp;
	fclose(fp);
	usrCert2 = d2i_X509(NULL, &uCert2, uCert2Len);
	if (X509_STORE_CTX_init(ctx, rootCertStore, usrCert2, caCertStack) != 1) {
		perror("X509_STORE_CTX_init");
		return;
	}
	if (X509_verify_cert(ctx) != 1) {
		printf("user2.cer %s\n", X509_verify_cert_error_string(ctx->error));
		return;
	}

	X509_free(usrCert1);
	X509_free(usrCert2);
	X509_free(rootCert);
	X509_STORE_CTX_cleanup(ctx);
	X509_STORE_CTX_free(ctx);
	X509_STORE_free(rootCertStore);

	return;
}

void openssl_x509_info()
{
	FILE *fp;
	wchar_t *pUtf8;
	X509 *x509Cert;
	long Nid, Version;
	ASN1_INTEGER *Serial;
	X509_NAME *issuer, *subject;
	X509_NAME_ENTRY *name_entry;
	ASN1_TIME *timea, *timeb;
	char msginfo[MAX1_LEN];
	const unsigned char *uCert;
	unsigned char tmp[MAX4_LEN];
	int certLen, i, eNum, msgLen, nUtf8;

	OpenSSL_add_all_algorithms();
	printf("\nX509_Issuer info:\n");

	fp = fopen(U2CERTF, "rb");
	certLen = fread(tmp, 1, 4096, fp);
	fclose(fp);
	uCert = tmp;
	x509Cert = d2i_X509(NULL, &uCert, certLen);
	Version = X509_get_version(x509Cert);
	printf("\tX509 Version: %ld\n", Version);

	Serial = X509_get_serialNumber(x509Cert);
	printf("\tserialNumber is: ");
	for (i = 0; i < Serial->length; i++)
		printf("%02x", Serial->data[i]);
	printf("\n");

	issuer = X509_get_issuer_name(x509Cert);
	eNum = sk_X509_NAME_ENTRY_num(issuer->entries);
	for (i = 0; i < eNum; i++) {
		name_entry = sk_X509_NAME_ENTRY_value(issuer->entries, i);
		if (name_entry->value->type == V_ASN1_UTF8STRING) {
			nUtf8 = 2 * name_entry->value->length;
			pUtf8 = (wchar_t *) malloc(nUtf8);
			if (pUtf8 == NULL)
				return;
			memset(pUtf8, 0, nUtf8);
			mbstowcs(pUtf8, (char *)name_entry->value->data,
					 name_entry->value->length);
			wcstombs(msginfo, pUtf8, nUtf8);
			msgLen = nUtf8;
			msginfo[msgLen] = '\0';
			free(pUtf8);
			pUtf8 = NULL;
		} else {
			msgLen = name_entry->value->length;
			memcpy(msginfo, name_entry->value->data, msgLen);
			msginfo[msgLen] = '\0';
		}

		Nid = OBJ_obj2nid(name_entry->object);
		switch (Nid) {
		case NID_countryName:
			printf("\tissuer's countryName: %s\n", msginfo);
			break;

		case NID_stateOrProvinceName:
			printf("\tissuer's provinceName: %s\n", msginfo);
			break;

		case NID_localityName:
			printf("\tissuer's localityName: %s\n", msginfo);
			break;

		case NID_organizationName:
			printf("\tissuer's organizationName: %s\n", msginfo);
			break;

		case NID_organizationalUnitName:
			printf("\tissuer's organizationalUnitName: %s\n", msginfo);
			break;

		case NID_commonName:
			printf("\tissuer's commonName: %s\n", msginfo);
			break;

		case NID_pkcs9_emailAddress:
			printf("\tissuer's emailAddress: %s\n", msginfo);
			break;

		default:
			break;
		}
	}

	subject = X509_get_subject_name(x509Cert);
	eNum = sk_X509_NAME_ENTRY_num(subject->entries);
	for (i = 0; i < eNum; i++) {
		name_entry = sk_X509_NAME_ENTRY_value(subject->entries, i);
		if (name_entry->value->type == V_ASN1_UTF8STRING) {
			nUtf8 = 2 * name_entry->value->length;
			pUtf8 = (wchar_t *) malloc(nUtf8);
			if (pUtf8 == NULL)
				return;
			memset(pUtf8, 0, nUtf8);
			mbstowcs(pUtf8, (char *)name_entry->value->data,
					 name_entry->value->length);
			wcstombs(msginfo, pUtf8, nUtf8);
			msgLen = nUtf8;
			msginfo[msgLen] = '\0';
			free(pUtf8);
			pUtf8 = NULL;
		} else {
			msgLen = name_entry->value->length;
			memcpy(msginfo, name_entry->value->data, msgLen);
			msginfo[msgLen] = '\0';
		}

		Nid = OBJ_obj2nid(name_entry->object);
		switch (Nid) {
		case NID_countryName:
			printf("\tsubject's countryName: %s\n", msginfo);
			break;

		case NID_stateOrProvinceName:
			printf("\tsubject's ProvinceName: %s\n", msginfo);
			break;

		case NID_localityName:
			printf("\tsubject's localityName: %s\n", msginfo);
			break;

		case NID_organizationName:
			printf("\tsubject's organizationName: %s\n", msginfo);
			break;

		case NID_organizationalUnitName:
			printf("\tsubject's organizationalUnitName: %s\n", msginfo);
			break;

		case NID_commonName:
			printf("\tsubject's commonName: %s\n", msginfo);
			break;

		case NID_pkcs9_emailAddress:
			printf("\tsubject's emailAddress: %s\n", msginfo);
			break;

		default:
			break;
		}
	}

	timea = X509_get_notAfter(x509Cert);
	timeb = X509_get_notBefore(x509Cert);
	printf("\tCert notAfter: %s, notBefore: %s\n", timea->data, timeb->data);
	X509_free(x509Cert);

	return;
}

void openssl_x509_crl()
{
	RSA *r;
	BIO *bp;
	int len;
	FILE *fp;
	BIGNUM *bne;
	X509_CRL *crl;
	EVP_PKEY *pkey;
	X509_NAME *issuer;
	ASN1_INTEGER *serial;
	X509_REVOKED *revoked;
	ASN1_TIME *lastUpdate, *nextUpdate, *rvTime;
	unsigned char *buf, *p, tmp[MAX1_LEN] = "crl cert";

	printf("\nX509_CRL info:\n");
	bne = BN_new();
	BN_set_word(bne, RSA_3);
	r = RSA_new();
	RSA_generate_key_ex(r, MAX1_LEN, bne, NULL);
	pkey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(pkey, r);

	crl = X509_CRL_new();
	X509_CRL_set_version(crl, 3);

	issuer = X509_NAME_new();
	X509_NAME_add_entry_by_NID(issuer, NID_commonName,
								V_ASN1_PRINTABLESTRING, tmp, 10, -1, 0);
	X509_CRL_set_issuer_name(crl, issuer);

	lastUpdate = ASN1_TIME_new();
	ASN1_TIME_set(lastUpdate, time(NULL));
	X509_CRL_set_lastUpdate(crl, lastUpdate);

	nextUpdate = ASN1_TIME_new();
	ASN1_TIME_set(nextUpdate, time(NULL) + 1280);
	X509_CRL_set_nextUpdate(crl, nextUpdate);

	revoked = X509_REVOKED_new();
	serial = ASN1_INTEGER_new();
	ASN1_INTEGER_set(serial, 1280);
	X509_REVOKED_set_serialNumber(revoked, serial);

	rvTime = ASN1_TIME_new();
	ASN1_TIME_set(rvTime, time(NULL) + 2000);
	X509_CRL_set_nextUpdate(crl, rvTime);
	X509_REVOKED_set_revocationDate(revoked, rvTime);

	X509_CRL_add0_revoked(crl, revoked);
	X509_CRL_sort(crl);
	X509_CRL_sign(crl, pkey, EVP_md5());

	bp = BIO_new(BIO_s_file());
	BIO_set_fp(bp, stdout, BIO_NOCLOSE);
	X509_CRL_print(bp, crl);
	len = i2d_X509_CRL(crl, NULL);
	buf = (unsigned char *)malloc(len + 10);
	p = buf;
	len = i2d_X509_CRL(crl, &p);
	fp = fopen("/tmp/crl.crl", "wb");
	fwrite(buf, 1, len, fp);
	fclose(fp);

	free(buf);
	BIO_free(bp);
	X509_CRL_free(crl);
}

void openssl_pkcs7_msg()
{
	int len;
	FILE *fp;
	PKCS7 *p7;
	unsigned char *der, *p, buf[SHA_DIGEST_LENGTH] = "pkcs7 msg";

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_data);
	ASN1_OCTET_STRING_set(p7->d.data, buf, SHA_DIGEST_LENGTH);

	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/test.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);
	free(der);

	PKCS7_free(p7);
}

void openssl_pkcs7_sign()
{
	int len;
	BIO *in;
	X509 *x;
	FILE *fp;
	PKCS7 *p7;
	X509_ALGOR *md;
	PKCS7_SIGNER_INFO *si;
	char name[MAX1_LEN], tmp[MAX1_LEN];
	unsigned char *der, *p, buf[SHA_DIGEST_LENGTH] = "pkcs7 sign";

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_data);
	ASN1_OCTET_STRING_set(p7->d.data, buf, SHA_DIGEST_LENGTH);
	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/test.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);
	free(der);
	PKCS7_free(p7);

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_signed);
	p7->d.sign->cert = sk_X509_new_null();
	in = BIO_new_file("/tmp/test.cer", "r");
	x = PEM_read_bio_X509(in, NULL, NULL, NULL);
	sk_X509_push(p7->d.sign->cert, x);
	BIO_free(in);

	md = X509_ALGOR_new();
	md->algorithm = OBJ_nid2obj(NID_md5);
	sk_X509_ALGOR_push(p7->d.sign->md_algs, md);

	si = PKCS7_SIGNER_INFO_new();
	ASN1_INTEGER_set(si->version, 2);
	ASN1_INTEGER_set(si->issuer_and_serial->serial, 333);
	sk_PKCS7_SIGNER_INFO_push(p7->d.sign->signer_info, si);

	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/test.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);
	free(der);

	fp = fopen("/tmp/test.cer", "rb");
	len = fread(tmp, 1, MAX1_LEN, fp);
	fclose(fp);
	p = (unsigned char *)&tmp;
	d2i_PKCS7(&p7, (const unsigned char **)&p, len);
	OBJ_obj2txt(name, MAX1_LEN, p7->type, 0);

	PKCS7_free(p7);
}

void openssl_pkcs7_enveloped()
{
	int len;
	FILE *fp;
	PKCS7 *p7;
	PKCS7_RECIP_INFO *inf;
	unsigned char *der, *p;
	const unsigned char edata[COMM_LEN] = "info....";
	const unsigned char ekeys[COMM_LEN] = "key info....";

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_enveloped);
	ASN1_INTEGER_set(p7->d.enveloped->version, 3);

	inf = PKCS7_RECIP_INFO_new();
	ASN1_INTEGER_set(inf->version, 4);
	ASN1_INTEGER_set(inf->issuer_and_serial->serial, 888888);
	inf->key_enc_algor->algorithm = OBJ_nid2obj(NID_des_ede3_cbc);
	ASN1_OCTET_STRING_set(inf->enc_key, ekeys, 12);
	sk_PKCS7_RECIP_INFO_push(p7->d.enveloped->recipientinfo, inf);

	p7->d.enveloped->enc_data->algorithm->algorithm =
		OBJ_nid2obj(NID_des_ede3_cbc);
	p7->d.enveloped->enc_data->enc_data = ASN1_OCTET_STRING_new();
	ASN1_OCTET_STRING_set(p7->d.enveloped->enc_data->enc_data, edata, 8);

	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/test.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);

	free(der);
	PKCS7_free(p7);

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_signedAndEnveloped);
	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/testEnv.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);

	PKCS7_free(p7);
	free(der);
}

void openssl_pkcs7_encrypt()
{
	BIO *b;
	int len;
	FILE *fp;
	PKCS7 *p7;
	unsigned char *der, *p;
	const unsigned char edata[COMM_LEN] = "pkcs7 encrypt";

	p7 = PKCS7_new();
	PKCS7_set_type(p7, NID_pkcs7_encrypted);
	ASN1_INTEGER_set(p7->d.encrypted->version, 3);
	p7->d.encrypted->enc_data->algorithm->algorithm =
		OBJ_nid2obj(NID_des_ede3_cbc);
	p7->d.encrypted->enc_data->enc_data = ASN1_OCTET_STRING_new();
	ASN1_OCTET_STRING_set(p7->d.encrypted->enc_data->enc_data, edata, 14);
	len = i2d_PKCS7(p7, NULL);
	der = (unsigned char *)malloc(len);
	p = der;
	len = i2d_PKCS7(p7, &p);
	fp = fopen("/tmp/test.cer", "wb");
	fwrite(der, 1, len, fp);
	fclose(fp);

	PKCS7_free(p7);
	free(der);

	b = BIO_new_file("/tmp/test.pem", "r");
	p7 = PEM_read_bio_PKCS7(b, NULL, NULL, NULL);
	BIO_free(b);
	PKCS7_free(p7);
}

void openssl_pkcs12_cert()
{
	FILE *tmpfile;
	PKCS12 *pkcs12s;
	EVP_PKEY *certprk;
	X509 *cscert, *cacert;
	STACK_OF(X509) * cacerts;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	certprk = EVP_PKEY_new();
	tmpfile = fopen(PKEYF, "r");
	certprk = PEM_read_PrivateKey(tmpfile, NULL, NULL, NULL);
	fclose(tmpfile);

	tmpfile = fopen(PCERTF, "r");
	cscert = PEM_read_X509(tmpfile, NULL, NULL, NULL);
	fclose(tmpfile);

	tmpfile = fopen(RCERTF, "r");
	cacert = PEM_read_X509(tmpfile, NULL, NULL, NULL);
	fclose(tmpfile);

	pkcs12s = PKCS12_new();
	cacerts = sk_X509_new_null();
	sk_X509_push(cacerts, cacert);
	pkcs12s = PKCS12_create("beike2012", "mypkcs12", certprk, cscert,
							cacerts, 0, 0, 0, 0, 0);
	tmpfile = fopen(PKCS12F, "w");
	if (i2d_PKCS12_fp(tmpfile, pkcs12s) <= 0)
		openssl_error_show("i2d_PKCS12_fp", 1);
	fclose(tmpfile);
	sk_X509_free(cacerts);
	PKCS12_free(pkcs12s);
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//check openssl bio api.
	case 1:
		openssl_bioBS_io();
		openssl_bioBS_md5();
		openssl_bioBS_cipher();
		openssl_bioBS_asnl();
		openssl_bioBS_random();
		openssl_bioBN_math();
		break;

		//check openssl finger api.
	case 2:
		openssl_md5_check();
		openssl_sha1_check();
		openssl_hmac_md5();
		openssl_ctx_compress();
		break;

		//check openssl crypt api.
	case 3:
		openssl_salt_encrypt();
		openssl_des_crypt();
		openssl_rsa_crypt();
		openssl_dsa_crypt();
		openssl_dh_crypt();
		openssl_ec_crypt();
		openssl_rsa_pemkey();
		break;

		//check openssl evp api.
	case 4:
		openssl_evp_encode();
		openssl_evp_comcrypt();
		openssl_evp_symcrypt();
		openssl_evp_asycrypt();
		openssl_evp_rsacripher();
		openssl_evp_digest();
		openssl_evp_comsign();
		openssl_evp_rsasign();
		openssl_evp_keyiv();
		break;

		//check openssl x509 api.
	case 5:
		openssl_x509_cert();
		openssl_x509_purpose();
		openssl_x509_verify();
		openssl_x509_info();
		openssl_x509_crl();
		break;

		//check openssl pkcs api.
	case 6:
		openssl_pkcs7_msg();
		openssl_pkcs7_sign();
		openssl_pkcs7_enveloped();
		openssl_pkcs7_encrypt();
		openssl_pkcs12_cert();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
