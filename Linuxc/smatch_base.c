/******************************************************************************
* Function: 	Pcre match handle.
* Author:	       forwarding2012@yahoo.com.cn 							
* Date: 		2012.01.01							
* Compile:	gcc -Wall smatch_base.c -I/usr/local/include  -lpcre -o smatch_base
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <pcre.h>

#define  SUBSLEN 	10
#define  OVECCOUNT 	30
#define  EBUFLEN 	128
#define  BUFLEN 	1024

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf("1 : match by regmatch\n");
	printf("2 : match by prcematch\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int reg_match()
{
	int err, i;
	size_t len;
	regex_t re;
	regmatch_t subs[SUBSLEN];
	char matched[BUFLEN], errbuf[EBUFLEN];
	char src[] = "111 <title>Hello World</title> 222";
	char pattern[] = "<title>(.*)</title>";

	printf("\nString : %s\nPattern: %s\n", src, pattern);

	err = regcomp(&re, pattern, REG_EXTENDED);
	if (err) {
		len = regerror(err, &re, errbuf, sizeof(errbuf));
		printf("error, regcomp: [%d]%s\n", len, errbuf);
		return 1;
	}
	printf("Total has subexpression: %d\n", re.re_nsub);

	err = regexec(&re, src, (size_t) SUBSLEN, subs, 0);
	if (err == REG_NOMATCH) {
		printf("Sorry, no match ...\n");
		regfree(&re);
		return 0;
	} else if (err) {
		len = regerror(err, &re, errbuf, sizeof(errbuf));
		printf("error, regexec: [%d]%s\n", len, errbuf);
		return 1;
	}

	printf("\nOK, has matched ...\n");
	for (i = 0; i <= re.re_nsub; i++) {
		len = subs[i].rm_eo - subs[i].rm_so;
		printf("subexp-%d begin: %2d, len %d  ", i, subs[i].rm_so, len);
		memcpy(matched, src + subs[i].rm_so, len);
		matched[len] = '\0';
		printf("match: %s\n", matched);
	}

	regfree(&re);

	return (0);
}

int pcre_match()
{
	pcre *re;
	const char *error;
	int rc, i, ovector[OVECCOUNT];
	int erroffset, substring_length;
	char src[] = "111 <title>Hello World</title> 222";
	char pattern[] = "<title>(.*)</title>";
	char *substring_start;

	printf("\nString : %s\nPattern: %s\n", src, pattern);

	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re == NULL) {
		printf("PCRE compilation failed at offset %d: %s\n", erroffset,
		       error);
		return 1;
	}

	rc = pcre_exec(re, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
	if (rc < 0) {
		if (rc == PCRE_ERROR_NOMATCH)
			printf("Sorry, no match ...\n");
		else
			printf("Matching error %d\n", rc);
		free(re);
		return 1;
	}

	printf("\nOK, has matched ...\n");
	for (i = 0; i < rc; i++) {
		substring_start = src + ovector[2 * i];
		substring_length = ovector[2 * i + 1] - ovector[2 * i];
		printf("subexp-%d: %.*s\n", i, substring_length,
		       substring_start);
	}

	free(re);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//catch a package.
	case 1:
		reg_match();
		break;

		//parse net package.
	case 2:
		pcre_match();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
