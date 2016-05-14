/******************************************************************************
 * Function: 	System process communication.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall process_chat.c -o process_chat
******************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/ipc.h>

#define  FIFO_NAME 		"/tmp/my_fifo"
#define  BUFFER_SIZE 	PIPE_BUF
#define  TEN_MEG 		(1024 * 1024 * 10)
#define  TEXT_SZ 		2048
#define  MAX_TEXT 		512
#define  TEXT_SZ 		2048

union sem_un {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

struct shared_un {
	int written_by_you;
	char some_text[TEXT_SZ];
};

struct shared_use_st {
	int written_by_you;
	char some_text[TEXT_SZ];
};

struct msg_un {
	long int my_msg_type;
	char some_text[BUFSIZ];
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: chat between parent and child processes by pipe\n");
	printf(" 2: chat between parent and child processes by redirect.\n");
	printf(" 3: chat among processes by pipe and custom function.\n");
	printf(" 4: chat among processes by mkfifo.\n");
	printf(" 5: chat among processes by popen.\n");
	printf(" 6: chat among processes by semaphore.\n");
	printf(" 7: chat among processes by share memory read.\n");
	printf(" 8: chat among processes by share memory write.\n");
	printf(" 9: chat among processes by message send.\n");
	printf("10: chat among processes by message receive.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int pipe_test(int argc, char *argv[])
{
	int data_processed;
	int file_descriptor;
	char buffer[BUFSIZ + 1];

	memset(buffer, '\0', sizeof(buffer));
	sscanf(argv[1], "%d", &file_descriptor);
	data_processed = read(file_descriptor, buffer, BUFSIZ);
	printf("%d - read %d bytes: %s\n", getpid(), data_processed, buffer);

	return 0;
}

int process_pipe_open()
{
	int file_pipes[2];
	int data_child;
	int data_parent;
	pid_t fork_result;
	char buffer[BUFSIZ + 1] = "";
	const char some_data[] = "123";

	if (pipe(file_pipes) != 0)
		return -1;

	fork_result = fork();
	if (fork_result == (pid_t) - 1)
		return -1;

	if (fork_result == (pid_t) 0) {
		data_child = read(file_pipes[0], buffer, BUFSIZ);
		printf("[Parent-%d] read %d bytes.\n", getpid(), data_child);
		return 0;
	} else {
		data_parent = write(file_pipes[1], some_data, strlen(some_data));
		printf("[Child-%d] wrote %d bytes.\n", getpid(), data_parent);
	}

	return 0;
}

int process_pipe_redirect()
{
	int data_processed;
	int file_pipes[2];
	pid_t fork_result;
	const char some_data[] = "123";

	if (pipe(file_pipes) != 0)
		return -1;

	fork_result = fork();
	if (fork_result == (pid_t) - 1)
		return -1;

	if (fork_result == (pid_t) 0) {
		close(0);
		dup(file_pipes[0]);
		close(file_pipes[0]);
		close(file_pipes[1]);
		execlp("od", "od", "-c", (char *)0);
		return -1;
	} else {
		close(file_pipes[0]);
		data_processed = write(file_pipes[1], some_data, strlen(some_data));
		close(file_pipes[1]);
		printf("[Parent-%d] write %d bytes.\n", getpid(), data_processed);
	}

	return 0;
}

int process_pipe_custom()
{
	int data_processed;
	int file_pipes[2];
	pid_t fork_result;
	char buffer[BUFSIZ + 1];
	const char some_data[] = "123";

	memset(buffer, '\0', sizeof(buffer));

	if (pipe(file_pipes) != 0)
		return -1;

	fork_result = fork();
	if (fork_result == (pid_t) - 1)
		return -1;

	if (fork_result == (pid_t) 0) {
		system("gcc pipe_test.c -o pipe_test >/dev/null 2>&1");
		sprintf(buffer, "%d", file_pipes[0]);
		execl("pipe_test", "pipe_test", buffer, (char *)0);
		return -1;
	} else {
		data_processed = write(file_pipes[1], some_data, strlen(some_data));
		printf("%d - wrote %d bytes.\n", getpid(), data_processed);
	}

	return 0;
}

int process_fifo_open()
{
	char wbuffer[BUFFER_SIZE + 1];
	char rbuffer[BUFFER_SIZE + 1];
	pid_t fork_result, child_pid;
	int bytes_write = 0, bytes_read = 0;
	int pipe_wfd, pipe_rfd, stat_val, wes, res;

	if (access(FIFO_NAME, F_OK) == -1) {
		wes = mkfifo(FIFO_NAME, 0777);
		if (wes != 0)
			return -1;
	}

	fork_result = fork();
	if (fork_result == -1)
		return -1;

	if (fork_result == (pid_t) 0) {
		memset(wbuffer, '\0', sizeof(wbuffer));
		printf("[Child-%d] opening FIFO O_WRONLY\n", getpid());
		pipe_wfd = open(FIFO_NAME, O_WRONLY);
		printf("[Child-%d] result %d\n", getpid(), pipe_wfd);

		if (pipe_wfd != -1) {
			while (bytes_write < TEN_MEG) {
				wes = write(pipe_wfd, wbuffer, BUFFER_SIZE);
				if (wes == -1)
					return -1;
				bytes_write += wes;
			}
			close(pipe_wfd);
		}
		printf("Process %d finished\n", getpid());
	} else {
		memset(rbuffer, '\0', sizeof(rbuffer));
		printf("[Parent-%d] opening FIFO O_RDONLY\n", getpid());
		pipe_rfd = open(FIFO_NAME, O_RDONLY);
		printf("[Parent-%d] result %d\n", getpid(), pipe_rfd);

		if (pipe_rfd != -1) {
			do {
				res = read(pipe_rfd, rbuffer, BUFFER_SIZE);
				bytes_read += res;
			} while (res > 0);
			close(pipe_rfd);
		}
		printf("[Parent-%d] finished, %d bytes read\n", getpid(), bytes_read);

		child_pid = wait(&stat_val);
		printf("Child has finished: PID = %d\n", child_pid);
		if (WIFEXITED(stat_val))
			printf("Child exited with code %d\n", WEXITSTATUS(stat_val));
		else
			printf("Child terminated abnormally\n");
	}

	unlink(FIFO_NAME);

	return 0;
}

int process_popen_fopen()
{
	int chars_read;
	char buffer[BUFSIZ + 1];
	FILE *read_fp, *write_fp;

	memset(buffer, '\0', sizeof(buffer));
	printf("First read the result from popen cmd -- ps ax:\n");

	read_fp = popen("ps ax", "r");
	if (read_fp != NULL) {
		chars_read = fread(buffer, sizeof(char), BUFSIZ, read_fp);
		while (chars_read > 0) {
			buffer[chars_read - 1] = '\0';
			printf("Reading %d:-\n %s\n", BUFSIZ, buffer);
			chars_read = fread(buffer, sizeof(char), BUFSIZ, read_fp);
		}
		pclose(read_fp);
	}

	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "Once upon a time, there was...\n");
	printf("Then write mystring: [ %s ] to popen as input arg.\n", buffer);

	write_fp = popen("od -c", "w");
	if (write_fp != NULL) {
		fwrite(buffer, sizeof(char), strlen(buffer), write_fp);
		pclose(write_fp);
	}

	return 0;
}

int process_sema_vipc(int argc)
{
	key_t key;
	int i, pause_time;
	char op_char = 'O';
	union sem_un sem_union;
	struct sembuf sem_b;
	static int sem_id;

	srand((unsigned int)getpid());

	if ((key = ftok(".", 'A')) == -1)
		return -1;

	sem_id = semget(key, 1, 0666 | IPC_CREAT);

	if (argc > 1) {
		sem_union.val = 1;
		if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
			return -1;
		op_char = 'X';
		sleep(2);
	}

	for (i = 0; i < 10; i++) {
		sem_b.sem_num = 0;
		sem_b.sem_op = -1;
		sem_b.sem_flg = SEM_UNDO;
		if (semop(sem_id, &sem_b, 1) == -1)
			return -1;

		printf("%c", op_char);
		fflush(stdout);
		pause_time = rand() % 3;
		sleep(pause_time);
		printf("%c", op_char);
		fflush(stdout);

		sem_b.sem_num = 0;
		sem_b.sem_op = 1;
		sem_b.sem_flg = SEM_UNDO;
		if (semop(sem_id, &sem_b, 1) == -1)
			return -1;

		pause_time = rand() % 2;
		sleep(pause_time);
	}

	printf("\n%d - finished\n", getpid());

	if (argc < 1) {
		sleep(10);
		if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
			return -1;
	}

	return 0;
}

int process_shmc_vipc()
{
	int shmid, running = 1;
	void *shared_memory = (void *)0;
	struct shared_un *shared_stuff;

	srand((unsigned int)getpid());

	shmid = shmget((key_t) 1234, sizeof(struct shared_un), 0666 | IPC_CREAT);
	if (shmid == -1)
		return -1;

	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1)
		return -1;

	printf("Memory attached at %X.\n", (int)shared_memory);
	shared_stuff = (struct shared_un *)shared_memory;
	shared_stuff->written_by_you = 0;
	while (running) {
		if (shared_stuff->written_by_you) {
			printf("You wrote: %s", shared_stuff->some_text);
			sleep(rand() % 4);
			shared_stuff->written_by_you = 0;
			if (strncmp(shared_stuff->some_text, "end", 3) == 0) {
				running = 0;
			}
		}
	}

	if (shmdt(shared_memory) == -1)
		return -1;

	if (shmctl(shmid, IPC_RMID, 0) == -1)
		return -1;

	return 0;
}

int process_shmp_vipc()
{
	char buffer[BUFSIZ];
	int shmid, running = 1;
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;

	shmid = shmget((key_t) 1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	if (shmid == -1)
		return -1;

	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1)
		return -1;

	printf("Memory attached at %X.\n", (int)shared_memory);
	shared_stuff = (struct shared_use_st *)shared_memory;
	while (running) {
		while (shared_stuff->written_by_you == 1) {
			sleep(1);
			printf("waiting for client...\n");
		}

		printf("Enter some text: ");
		fgets(buffer, BUFSIZ, stdin);
		strncpy(shared_stuff->some_text, buffer, TEXT_SZ);
		shared_stuff->written_by_you = 1;
		if (strncmp(buffer, "end", 3) == 0)
			running = 0;
	}

	if (shmdt(shared_memory) == -1)
		return -1;

	return 0;
}

int process_msgs_vipc()
{
	char buffer[BUFSIZ];
	int msgid, running = 1;
	struct msg_un some_data;

	msgid = msgget((key_t) 1234, 0666 | IPC_CREAT);
	if (msgid == -1)
		return -1;

	while (running) {
		printf("Enter some text: ");
		fgets(buffer, BUFSIZ, stdin);

		some_data.my_msg_type = 1;
		strcpy(some_data.some_text, buffer);
		if (msgsnd(msgid, (void *)&some_data, MAX_TEXT, 0) == -1)
			return -1;

		if (strncmp(buffer, "end", 3) == 0)
			running = 0;
	}

	return 0;
}

int process_msgr_vipc()
{
	struct msqid_ds buf;
	struct msg_un some_data;
	int msgid, running = 1;
	long int msg_to_receive = 0;

	msgid = msgget((key_t) 1234, 0666 | IPC_CREAT);
	if (msgid == -1)
		return -1;

	while (running) {
		if (msgrcv(msgid, (void *)&some_data, BUFSIZ, msg_to_receive, 0) == -1)
			return -1;

		printf("You wrote: %s", some_data.some_text);
		if (strncmp(some_data.some_text, "end", 3) == 0)
			running = 0;
	}

	msgctl(msgid, MSG_INFO, &buf);
	printf("buf.msg_stime=%ld, buf.msg_rtime=%ld, buf.msg_ctime=%ld\n",
	       buf.msg_stime, buf.msg_rtime, buf.msg_ctime);
	printf("buf.__msg_cbytes=%lu, buf.msg_qnum=%u, buf.msg_qbytes=%u\n",
	       buf.__msg_cbytes, (unsigned int)buf.msg_qnum,
	       (unsigned int)buf.msg_qbytes);
	printf("buf.msg_lspid=%d, buf.msg_lrpid=%u\n", buf.msg_lspid, buf.msg_lrpid);

	if (msgctl(msgid, IPC_RMID, 0) == -1)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 3)
		choice = check_model();

	switch (choice) {
		//test communication between parent and child processes by pipe.
	case 1:
		process_pipe_open();
		break;

		//test communication between parent and child processes by pipe and redirect.
	case 2:
		process_pipe_redirect();
		break;

		//test communication among processes  by pipe and custom function.
	case 3:
		process_pipe_custom();
		break;

		//test communication among processes by mkfifo.
	case 4:
		process_fifo_open();
		break;

		//test communication among processes by popen.
	case 5:
		process_popen_fopen();
		break;

		//test communication among processes by semaphore.
	case 6:
		process_sema_vipc(argc);
		break;

		//test communication among processes by share memory read.
	case 7:
		process_shmc_vipc();
		break;

		//test communication among processes by share memory write.
	case 8:
		process_shmp_vipc();
		break;

		//test communication among processes by message send.
	case 9:
		process_msgs_vipc();
		break;

		//test communication among processes by message receive.
	case 10:
		process_msgr_vipc();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
