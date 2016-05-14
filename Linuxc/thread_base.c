/******************************************************************************
 * Function: 	System thread called.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall thread_base.c -lpthread -o thread_base
******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define  NUM_THREADS 6
#define  WORK_SIZE   1024

pthread_key_t key;
int time_to_exit = 0;
int thread_gdata = 10;
int thread_finished = 0;
char work_area[WORK_SIZE];
char message[] = "Hello World";

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test creat a attached thread\n");
	printf(" 2: test creat a detached thread\n");
	printf(" 3: test cancel a thread\n");
	printf(" 4: test send signal to threads\n");
	printf(" 5: test running some threads at the same time\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void *attache_function(void *arg)
{
	printf("Thread_function is running. Argument was %s\n", (char *)arg);
	sleep(3);
	strcpy(message, "Bye!");
	pthread_exit("Baxia!");
}

int thread_creat_attached()
{
	int state;
	pthread_t a_thread;
	void *thread_result;
	pthread_attr_t thread_attr;

	if (pthread_attr_init(&thread_attr)) {
		perror("pthread_attr_init");
		exit(EXIT_FAILURE);
	}

	if (pthread_attr_getdetachstate(&thread_attr, &state)) {
		perror("pthread_attr_getdetachstate");
		exit(EXIT_FAILURE);
	}

	if (state == 0) 
		printf("Now, thread state is PTHREAD_CREATE_JOINABLE.\n");
	else 
		printf("Now, thread state is PTHREAD_CREATE_DETACHED.\n");

	if (pthread_create(&a_thread, NULL, attache_function, (void *)message)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	printf("Waiting for thread to finish...\n");
	if (pthread_join(a_thread, &thread_result)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	printf("Thread joined, it returned: %s\n", (char *)thread_result);
	printf("Message is now %s\n", message);

	return 0;
}

void *detache_function(void *arg)
{
	printf("thread_function is running. Argument was %s\n", (char *)arg);
	sleep(4);
	printf("Second thread setting finished flag, and exiting now\n");
	thread_finished = 1;
	pthread_exit(NULL);
}

int thread_creat_detached()
{
	pthread_t a_thread;
	pthread_attr_t thread_attr;
	struct sched_param scheduling_value;
	int state, max_priority, min_priority;

	if (pthread_attr_init(&thread_attr)) {
		perror("pthread_attr_init");
		exit(EXIT_FAILURE);
	}

	if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) {
		perror("pthread_attr_setdetachstate");
		exit(EXIT_FAILURE);
	}

	if (pthread_attr_getdetachstate(&thread_attr, &state) == -1) {
		perror("pthread_attr_getdetachstate");
		exit(EXIT_FAILURE);
	}

	if (state == 0)
		printf("Now, thread state is PTHREAD_CREATE_JOINABLE.\n");
	else 
		printf("Now, tthread state is PTHREAD_CREATE_DETACHED.\n");

	if (pthread_attr_setschedpolicy(&thread_attr, SCHED_OTHER)) {
		perror("pthread_attr_setschedpolicy");
		exit(EXIT_FAILURE);
	}

	max_priority = sched_get_priority_max(SCHED_OTHER);
	min_priority = sched_get_priority_min(SCHED_OTHER);
	scheduling_value.sched_priority = min_priority;
	if (pthread_attr_setschedparam(&thread_attr, &scheduling_value)) {
		perror("pthread_attr_setschedparam");
		exit(EXIT_FAILURE);
	}
	printf("Scheduling priority set to %d, max allowed was %d\n",
	       min_priority, max_priority);

	if (pthread_create(&a_thread, &thread_attr, detache_function, (void *)message)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	if (pthread_attr_getschedparam(&thread_attr, &scheduling_value)) {
		perror("pthread_attr_getschedparam");
		exit(EXIT_FAILURE);
	} else {
		printf("the priority is %d\n", scheduling_value.sched_priority);
	}

	pthread_attr_destroy(&thread_attr);

	while (thread_finished == 0) {
		printf("Waiting for thread...\n");
		sleep(1);
	}

	printf("Thread finished, bye!\n");

	return 0;
}

void cancel_cleanup()
{
	printf("cleanup system resource.\n");
}

void *cancel_function(void *arg)
{
	int i;

	if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)) {
		perror("pthread_setcancelstate");
		exit(EXIT_FAILURE);
	}

	if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL)) {
		perror("pthread_setcanceltype");
		exit(EXIT_FAILURE);
	}

	printf("set cleanup action here.\n");
	pthread_cleanup_push(cancel_cleanup, NULL);

	printf("thread_function is running\n");
	for (i = 0; i < 10; i++) {
		printf("Thread is still running (%d)...\n", i);
		sleep(1);
	}

	pthread_cleanup_pop(1);

	pthread_exit(0);
}

int thread_cancel_operation()
{
	pthread_t a_thread;
	void *thread_result;

	if (pthread_create(&a_thread, NULL, cancel_function, NULL)) {
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}

	sleep(3);
	printf("Canceling thread...\n");
	if (pthread_cancel(a_thread)) {
		perror("Thread cancelation failed");
		exit(EXIT_FAILURE);
	}

	printf("Waiting for thread to finish...\n");
	if (pthread_join(a_thread, &thread_result)) {
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}

	return 0;
}

void signal_report_function(int sig)
{
	printf("\nthe sig %d, the thread id %lu\n", sig, pthread_self());
}

void *signal1_function(void *arg)
{
	int i;
	__sigset_t set;

	signal(SIGUSR1, signal_report_function);
	sigfillset(&set);
	sigdelset(&set, SIGUSR2);
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	for (i = 0; i < 5; i++) {
		printf("this is set mask %lu thread\n", pthread_self());
		pause();
	}

	return NULL;
}

void *signal2_function(void *arg)
{
	int i;

	signal(SIGUSR2, signal_report_function);

	for (i = 0; i < 5; i++) {
		printf("this is no set mask %lu thread\n", pthread_self());
		pause();
	}

	return NULL;
}

int thread_signal_operation()
{
	pthread_t thread_one, thread_two;

	if (pthread_create(&thread_one, NULL, signal1_function, NULL)) {
		fprintf(stderr, "pthread_create failure\n");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&thread_two, NULL, signal2_function, NULL)) {
		fprintf(stderr, "pthread_create failure\n");
		exit(EXIT_FAILURE);
	}

	sleep(1);

	printf("parent send SIGUSR1, SIGUSR2 to thread %lu\n", thread_one);

	if (pthread_kill(thread_one, SIGUSR1)) {
		perror("pthread_kill");
		exit(EXIT_FAILURE);
	}

	if (pthread_kill(thread_one, SIGUSR2)) {
		perror("pthread_kill");
		exit(EXIT_FAILURE);
	}

	printf("parent send SIGUSR1, SIGUSR2 to thread %lu\n", thread_two);

	if (pthread_kill(thread_two, SIGUSR1)) {
		perror("pthread_kill");
		exit(EXIT_FAILURE);
	}

	if (pthread_kill(thread_two, SIGUSR2)) {
		perror("pthread_kill");
		exit(EXIT_FAILURE);
	}
	sleep(1);

	if (pthread_kill(thread_one, SIGKILL)) {
		perror("pthread_kill");
		exit(EXIT_FAILURE);
	}

	pthread_join(thread_two, NULL);
	pthread_join(thread_one, NULL);

	return 0;
}

void destructor_exec_function(void *t)
{
	printf("destructor thread %lu, data is %d\n", pthread_self(), (*(int *)t));
}

void *muti_function(void *arg)
{
	int rand_num;
	int my_number = (int)arg;

	printf("thread_function is running. Argument was %d\n", my_number);
	rand_num = 1 + (int)(9.0 * rand() / (RAND_MAX + 1.0));
	thread_gdata = rand_num * 2;
	pthread_setspecific(key, &rand_num);
	
	sleep(rand_num);
	printf("thread %lu bye from %d\n", pthread_self(), my_number);
	printf("\treturns global data %d, private data %d [address at %d].\n",
	       thread_gdata, *((int *)pthread_getspecific(key)),
	       *(int *)pthread_getspecific(key));

	pthread_exit(NULL);
}

int thread_muti_operation()
{
	int num;
	void *thread_result;
	pthread_t a_thread[NUM_THREADS];

	pthread_key_create(&key, destructor_exec_function);

	for (num = 0; num < NUM_THREADS; num++) {
		if (pthread_create(&(a_thread[num]), NULL, muti_function, (void *)num)) {
			perror("Thread creation failed");
			exit(EXIT_FAILURE);
		}
	}

	printf("Waiting for threads to finish...\n");
	for (num = NUM_THREADS - 1; num >= 0; num--) {
		if (pthread_join(a_thread[num], &thread_result)) 
			perror("pthread_join");
		else
			printf("Picked up a thread\n");
	}

	pthread_key_delete(key);
	printf("All done\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test creat a attache thread.
	case 1:
		thread_creat_attached();
		break;

		//test creat a detatche thread.
	case 2:
		thread_creat_detached();
		break;

		//test cancel a thread.
	case 3:
		thread_cancel_operation();
		break;

		//test send signal to threads. 
	case 4:
		thread_signal_operation();
		break;

		//test creat muti attache threads.
	case 5:
		thread_muti_operation();
		break;

		//default do nothing.
	default:
		break;
	}

	return 0;
}
