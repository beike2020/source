/******************************************************************************
 * Function: 	some methods of libcurl use.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall curl_base.c -lcurl -o curl_base  
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

#define  MAXP 	10

pthread_mutex_t mutex;

struct InfoStruct {
	char *memory;
	size_t size;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *) stream);
}

size_t get_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct InfoStruct *mem = (struct InfoStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL)
		return -1;

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void *get_info(void *url)
{
	FILE *fp;
	CURL *curl;
	CURLcode res;
	char *cacert;
	long http_code;
	char sockfile[1024];

	curl = curl_easy_init();
	if (curl == NULL)
		return NULL;

	memset(sockfile, 0, sizeof(sockfile));
	if (strstr(url, "http://"))
		memcpy(sockfile, (char *)(url + 7), strlen(url) - 7);
	else if (strstr(url, "https://"))
		memcpy(sockfile, (char *)(url + 8), strlen(url) - 8);
	else
		memcpy(sockfile, (char *)(url), strlen(url));

	fp = fopen(sockfile, "w");
	if (fp == NULL) {
		curl_easy_cleanup(curl);
		return NULL;
	}

	cacert = "test.crt";
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	//curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);
	//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (res != CURLE_OK || http_code != 200)
		printf("Received error status code: %ld\n", http_code);

	fclose(fp);
	curl_easy_cleanup(curl);

	return NULL;
}

void post_info(void *url, char *send_msg)
{
	CURL *curl;
	CURLcode res;
	char *cacert;
	long http_code;
	struct InfoStruct chunk;

	curl = curl_easy_init();
	if (curl == NULL)
		return;

	chunk.memory = malloc(1);
	chunk.size = 0;

	cacert = "test.crt";
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	//curl_easy_setopt(curl, CURLOPT_CAINFO, cacert);
	//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, CURL_SSLVERSION_TLSv1);
	//curl_easy_setopt(curl, CURLOPT_SSLVERSION, 2L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, send_msg);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Beike UA");
	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
	if (res != CURLE_OK || http_code != 200 || !strstr(chunk.memory, "OK"))
		printf("Error post info to: %s, %d bytes retrieved %s\n",
		       (char *)url, chunk.size, chunk.memory);

	if (chunk.memory)
		free(chunk.memory);
	curl_easy_cleanup(curl);
}

void *handle_task(void *arg)
{
	char name[1024];
	char send_msg[1024];

	memset(send_msg, 0, sizeof(send_msg));
	strncpy(send_msg, "Hello world!", sizeof(send_msg));

	while (1) {
		memset(name, 0, sizeof(name));
		pthread_mutex_lock(&mutex);
		if (scanf("%s\n", name) == EOF) {
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		pthread_mutex_unlock(&mutex);
		get_info((void *)name);
		post_info((void *)name, send_msg);
	}
}

int main(int argc, char **argv)
{
	int i;
	pthread_t p[MAXP];

	curl_global_init(CURL_GLOBAL_ALL);
	pthread_mutex_init(&mutex, NULL);

	freopen("weblist.txt", "r", stdin);
	for (i = 0; i < MAXP; ++i)
		pthread_create(&p[i], NULL, handle_task, NULL);

	for (i = 0; i < MAXP; ++i)
		pthread_join(p[i], NULL);

	curl_global_cleanup();

	return 0;
}
