/******************************************************************************
 * Function: 	Tread communication.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall thread_chat.c -lpthread -o thread_chat
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <bits/pthreadtypes.h>

#define  BUFFER_SIZE 	2
#define  WORK_SIZE 		1024
#define	 READER_MAX     3
#define  WRITER_MAX     2

int time_to_exit;
static sem_t bin_sem;
char work_area[WORK_SIZE];
static pthread_rwlock_t rwlock;
static pthread_mutex_t work_mutex;

struct prodcons {
	int buffer[BUFFER_SIZE];
	pthread_mutex_t lock;
	int readpos, writepos;
	pthread_cond_t notempty;
	pthread_cond_t notfull;
};

struct prodcons buffer;


static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test communication between threads by mutex_lock.\n");
	printf(" 2: test communication between threads by semaphore.\n");
	printf(" 3: test communication between threads by cond_lock.\n");
	printf(" 4: test communication between threads by rw_lock.\n");
	printf(" 5: test communication between threads by rw_trylock.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void *mutex_lock_function()
{
	sleep(1);

	pthread_mutex_lock(&work_mutex);

	while (strncmp("end", work_area, 3)) {
		printf("You input %d characters\n", strlen(work_area) - 1);
		work_area[0] = '\0';
		pthread_mutex_unlock(&work_mutex);
		sleep(1);

		pthread_mutex_lock(&work_mutex);
		while (work_area[0] == '\0') {
			pthread_mutex_unlock(&work_mutex);
			sleep(1);
			pthread_mutex_lock(&work_mutex);
		}
	}

	time_to_exit = 1;
	work_area[0] = '\0';

	pthread_mutex_unlock(&work_mutex);
	pthread_exit(0);
}

int thread_mutex_lock()
{
	pthread_t a_thread;
	void *thread_result;

	if (pthread_mutex_init(&work_mutex, NULL)) {
		perror("pthread_mutex_init");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&a_thread, NULL, mutex_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	pthread_mutex_lock(&work_mutex);
	printf("Input some text. Enter 'end' to finish\n");
	while (time_to_exit == 0) {
		fgets(work_area, WORK_SIZE, stdin);
		pthread_mutex_unlock(&work_mutex);

		while (1) {
			pthread_mutex_lock(&work_mutex);
			if (work_area[0] != '\0') {
				pthread_mutex_unlock(&work_mutex);
				sleep(1);
			} else {
				break;
			}
		}
	}
	pthread_mutex_unlock(&work_mutex);

	printf("\nWaiting for thread to finish...\n");
	if (pthread_join(a_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	printf("Thread joined\n");
	pthread_mutex_destroy(&work_mutex);

	return 0;
}

void *sema_lock_function(void *arg)
{
	sem_wait(&bin_sem);
	while (strncmp("end", work_area, 3)) {
		printf("You input %d characters\n", strlen(work_area) - 1);
		sem_wait(&bin_sem);
	}

	pthread_exit(NULL);
}

int thread_sema_lock()
{
	pthread_t a_thread;
	void *thread_result;

	if (sem_init(&bin_sem, 0, 0)) {
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&a_thread, NULL, sema_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	printf("Input some text. Enter 'end' to finish\n");
	while (strncmp("end", work_area, 3)) {
		fgets(work_area, WORK_SIZE, stdin);
		sem_post(&bin_sem);
	}

	printf("\nWaiting for thread to finish...\n");
	if (pthread_join(a_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	printf("Thread joined\n");
	sem_destroy(&bin_sem);

	return 0;
}

void *cond_lock_producer()
{
	int n;

	for (n = 0; n < 5; n++) {
		printf("cond_lock_producer sleep 1 second......\n");
		sleep(1);
		printf("put the %d product\n", n);

		pthread_mutex_lock(&buffer.lock);

		while ((buffer.writepos + 1) % BUFFER_SIZE == buffer.readpos) {
			printf("cond_lock_producer wait for not full\n");
			pthread_cond_wait(&buffer.notfull, &buffer.lock);
		}

		buffer.buffer[buffer.writepos] = n;
		buffer.writepos++;
		if (buffer.writepos >= BUFFER_SIZE)
			buffer.writepos = 0;

		pthread_cond_signal(&buffer.notempty);
		pthread_mutex_unlock(&buffer.lock);
	}

	for (n = 5; n < 10; n++) {
		printf("cond_lock_producer sleep 3 second......\n");
		sleep(3);
		printf("put the %d product\n", n);

		pthread_mutex_lock(&buffer.lock);

		while ((buffer.writepos + 1) % BUFFER_SIZE == buffer.readpos) {
			printf("cond_lock_producer wait for not full\n");
			pthread_cond_wait(&buffer.notfull, &buffer.lock);
		}

		buffer.buffer[buffer.writepos] = n;
		buffer.writepos++;
		if (buffer.writepos >= BUFFER_SIZE)
			buffer.writepos = 0;

		pthread_cond_signal(&buffer.notempty);
		pthread_mutex_unlock(&buffer.lock);
	}

	pthread_mutex_lock(&buffer.lock);

	while ((buffer.writepos + 1) % BUFFER_SIZE == buffer.readpos) {
		printf("cond_lock_producer wait for not full\n");
		pthread_cond_wait(&buffer.notfull, &buffer.lock);
	}

	buffer.buffer[buffer.writepos] = -1;
	buffer.writepos++;
	if (buffer.writepos >= BUFFER_SIZE)
		buffer.writepos = 0;

	pthread_cond_signal(&buffer.notempty);
	pthread_mutex_unlock(&buffer.lock);

	printf("cond_lock_producer stopped!\n");

	return NULL;
}

void *cond_lock_consumer()
{
	int data = 0;

	while (1) {
		printf("cond_lock_consumer sleep 2 second......\n");
		sleep(2);

		pthread_mutex_lock(&buffer.lock);

		while (buffer.writepos == buffer.readpos) {
			printf("cond_lock_consumer wait for not empty\n");
			pthread_cond_wait(&buffer.notempty, &buffer.lock);
		}

		data = buffer.buffer[buffer.readpos];
		buffer.readpos++;
		if (buffer.readpos >= BUFFER_SIZE) 
			buffer.readpos = 0;

		pthread_cond_signal(&buffer.notfull);
		pthread_mutex_unlock(&buffer.lock);

		printf("get the %d product\n", data);
		if (data == -1) 
			break;
	}

	printf("cond_lock_consumer stopped!\n");

	return NULL;
}

int thread_cond_lock()
{
	void *retval;
	pthread_t thread_p, thread_c;

	pthread_mutex_init(&buffer.lock, NULL);
	pthread_cond_init(&buffer.notempty, NULL);
	pthread_cond_init(&buffer.notfull, NULL);
	buffer.readpos = 0;
	buffer.writepos = 0;

	pthread_create(&thread_p, NULL, cond_lock_producer, 0);
	pthread_create(&thread_c, NULL, cond_lock_consumer, 0);

	pthread_join(thread_p, &retval);
	pthread_join(thread_c, &retval);

	return 0;
}

void *rwa_lock_function()
{
	printf("thread read one try to get lock\n");

	pthread_rwlock_rdlock(&rwlock);

	while (strncmp("end", work_area, 3)) {
		printf("this is thread read one.");
		printf("the characters is %s", work_area);
		pthread_rwlock_unlock(&rwlock);
		sleep(2);
		pthread_rwlock_rdlock(&rwlock);

		while (work_area[0] == '\0') {
			pthread_rwlock_unlock(&rwlock);
			sleep(2);
			pthread_rwlock_rdlock(&rwlock);
		}
	}

	pthread_rwlock_unlock(&rwlock);
	time_to_exit = 1;
	pthread_exit(0);
}

void *rwb_lock_function()
{
	printf("thread read one try to get lock\n");
	pthread_rwlock_rdlock(&rwlock);

	while (strncmp("end", work_area, 3)) {
		printf("this is thread read two.");
		printf("the characters is %s", work_area);
		pthread_rwlock_unlock(&rwlock);
		sleep(5);
		pthread_rwlock_rdlock(&rwlock);

		while (work_area[0] == '\0') {
			pthread_rwlock_unlock(&rwlock);
			sleep(5);
			pthread_rwlock_rdlock(&rwlock);
		}
	}

	pthread_rwlock_unlock(&rwlock);
	time_to_exit = 1;
	pthread_exit(0);
}

void *rwc_lock_function()
{
	printf("this is write thread one try to get lock\n");

	while (time_to_exit == 0) {
		pthread_rwlock_wrlock(&rwlock);
		printf("this is write thread one.\nInput 'end' to finish\n");
		fgets(work_area, WORK_SIZE, stdin);
		pthread_rwlock_unlock(&rwlock);
		sleep(15);
	}

	pthread_rwlock_unlock(&rwlock);
	pthread_exit(0);
}

void *rwd_lock_function()
{
	sleep(10);

	while (time_to_exit == 0) {
		pthread_rwlock_wrlock(&rwlock);
		printf("this is write thread two.\nInput 'end' to finish\n");
		fgets(work_area, WORK_SIZE, stdin);
		pthread_rwlock_unlock(&rwlock);
		sleep(20);
	}

	pthread_rwlock_unlock(&rwlock);
	pthread_exit(0);
}

int thread_rw_lock()
{
	void *thread_result;
	pthread_t a_thread, b_thread, c_thread, d_thread;

	if (pthread_rwlock_init(&rwlock, NULL)) {
		perror("pthread_rwlock_init");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&a_thread, NULL, rwa_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&b_thread, NULL, rwb_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&c_thread, NULL, rwc_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
	
	if (pthread_create(&d_thread, NULL, rwd_lock_function, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(a_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(b_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(c_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(d_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	pthread_rwlock_destroy(&rwlock);

	return 0;
}

void *rw_tryrdlock_function()
{
	while (time_to_exit == 0) {
		if (pthread_rwlock_tryrdlock(&rwlock)) {
			printf("Read_thread %u can't read data now.\n",
			       (unsigned int)pthread_self());
			sleep(1);
		} else {
			printf("Read_thread %u read data now.\n",
			       (unsigned int)pthread_self());
			sleep(1);
			printf("Read_thread %u read all data now.\n",
			       (unsigned int)pthread_self());
			pthread_rwlock_unlock(&rwlock);
			sleep(2);
		}
	}

	pthread_exit(0);
}

void *rw_trywrlock_function()
{
	while (time_to_exit == 0) {
		if (pthread_rwlock_trywrlock(&rwlock)) {
			printf("Write_thread %u can't write data now.\n",
			       (unsigned int)pthread_self());
			sleep(2);
		} else {
			printf("Write_thread %u write data now.\n",
			       (unsigned int)pthread_self());
			sleep(2);
			printf("Write_thread %u write all data now.\n",
			       (unsigned int)pthread_self());
			pthread_rwlock_unlock(&rwlock);
			sleep(3);
		}
	}

	pthread_exit(0);
}

int thread_rw_trylock()
{
	int i;
	void *thread_result;
	pthread_t reader, writer;

	pthread_rwlock_init(&rwlock, NULL);

	for (i = 0; i < READER_MAX; i++) 
		pthread_create(&reader, NULL, (void *)rw_tryrdlock_function, NULL);

	for (i = 0; i < WRITER_MAX; i++) 
		pthread_create(&writer, NULL, (void *)rw_trywrlock_function, NULL);

	sleep(10);
	time_to_exit = 1;

	if (pthread_join(reader, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(writer, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	pthread_rwlock_destroy(&rwlock);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test communication between threads by mutex_lock.
	case 1:
		thread_mutex_lock();
		break;

		//test communication between threads by semaphore.
	case 2:
		thread_sema_lock();
		break;

		//test communication between threads by cond_lock.
	case 3:
		thread_cond_lock();
		break;

		//test communication between threads by rw_lock. 
	case 4:
		thread_rw_lock();
		break;

		//test communication between threads by rw_trylock. 
	case 5:
		thread_rw_trylock();
		break;

		//default do nothing.
	default:
		break;
	}

	return 0;
}
