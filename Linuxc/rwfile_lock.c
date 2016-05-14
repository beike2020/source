/******************************************************************************
 * Function: 	File read and write lock.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall rwfile_lock.c -o rwfile_lock
******************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/wait.h>

#define  SIZE_TO_TRY	5
#define	 EACCES			13

static int check_model()
{
	int choices;

	printf("Test program as follow:\n");
	printf(" 1: lock whole file by specify open flag.\n");
	printf(" 2: lock whole file by flock method.\n");
	printf(" 3: lock whole file by lockf method.\n");
	printf(" 4: lock one file area by fcnt method.\n");
	printf(" 5: lock one file by fcnt method.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

void whole_file_open_lock()
{
	int fd, tries = 10;

	while (tries--) {
		fd = open("/tmp/test_lock", O_RDWR | O_CREAT | O_EXCL, 0666);
		if (fd < 0) {
			printf("%d - lock already present.\n", getpid());
			sleep(3);
		} else {
			printf("%d - have exclusive access.\n", getpid());
			sleep(1);
			close(fd);
			unlink("/tmp/test_lock");
			sleep(1);
		}
	}
}

void whole_file_flock_lock()
{
	int fd;

	fd = open("/tmp/test_lock", O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror("/tmp/test_lock");
		return;
	}

	if (flock(fd, LOCK_EX) < 0) {
		close(fd);
		return;
	}
	printf("%d - have exclusive access!\n", getpid());

	if (flock(fd, LOCK_UN) < 0) {
		close(fd);
		return;
	}
	printf("%d - have no access now!\n", getpid());

	close(fd);
}

void whole_file_lockf_lock()
{
	int fd;
	extern int errno;
	char buffer[1024];

	fd = open("/tmp/test_lock", O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror("/tmp/test_lock");
		return;
	}

	if (lockf(fd, F_TEST, 0) < 0 && errno == EACCES)
		return;
	else
		printf("Can access /tmp/test_lock!\n");

	if (lockf(fd, F_LOCK, 0) == -1)
		return;
	else
		printf("%d - have exclusive access!\n", getpid());

	memset(buffer, 0, sizeof(buffer));
	printf("Input string to /tmp/test_lock: ");
	scanf("%s", buffer);
	write(fd, buffer, sizeof(buffer));
	if (lockf(fd, F_ULOCK, 0) == -1)
		return;
	else
		printf("%d - have no access now!\n", getpid());

	close(fd);
	printf("Now file /tmp/test_lock content is:\n");
	execl("/bin/cat", "cat", "/tmp/test_lock", NULL);
}

void region_file_fcnt_clock1()
{
	pid_t pid;
	char *byte_to_write = "A";
	char *test_file = "/tmp/test_lock";
	int res, fd, byte_count, start_byte, stat_val;
	struct flock region_1, region_2, region_to_test;

	fd = open(test_file, O_RDWR | O_CREAT, 0666);
	if (fd == 0)
		return;

	for (byte_count = 0; byte_count < 100; byte_count++)
		write(fd, byte_to_write, 1);

	printf("Process %d locking file\n", getpid());

	//setup region 1, a shared lock, from bytes 10 to 30
	region_1.l_type = F_RDLCK;
	region_1.l_whence = SEEK_SET;
	region_1.l_start = 10;
	region_1.l_len = 20;
	res = fcntl(fd, F_SETLK, &region_1);
	if (res == -1)
		printf("Failed to lock region 1\n");

	//setup region 2, an exclusive lock, from bytes 40 to 50
	region_2.l_type = F_WRLCK;
	region_2.l_whence = SEEK_SET;
	region_2.l_start = 40;
	region_2.l_len = 10;
	res = fcntl(fd, F_SETLK, &region_2);
	if (res == -1)
		printf("Failed to lock region 2\n");

	pid = fork();
	if (pid == 0) {
		for (start_byte = 0; start_byte < 99; start_byte += SIZE_TO_TRY) {
			//test with a exclude (write/read) lock 
			region_to_test.l_type = F_WRLCK;
			region_to_test.l_whence = SEEK_SET;
			region_to_test.l_start = start_byte;
			region_to_test.l_len = SIZE_TO_TRY;
			region_to_test.l_pid = -1;

			printf("Testing F_WRLCK on region from %d to %d\n",
			       start_byte, start_byte + SIZE_TO_TRY);
			res = fcntl(fd, F_GETLK, &region_to_test);
			if (res == -1) {
				printf("F_GETLK failed\n");
				return;
			}

			if (region_to_test.l_pid != -1) {
				printf("Lock would fail. F_GETLK returned:\n");
				printf("\ttype %d, whence %d, start %d, len %d, pid %d\n",
				       region_to_test.l_type, region_to_test.l_whence,
				       (int)region_to_test.l_start,
				       (int)region_to_test.l_len, region_to_test.l_pid);
			} else {
				printf("F_WRLCK - Lock would succeed\n");
			}

			//test with a shared (read) lock 
			region_to_test.l_type = F_RDLCK;
			region_to_test.l_whence = SEEK_SET;
			region_to_test.l_start = start_byte;
			region_to_test.l_len = SIZE_TO_TRY;
			region_to_test.l_pid = -1;

			printf("Testing F_RDLCK on region from %d to %d\n",
			       start_byte, start_byte + SIZE_TO_TRY);
			res = fcntl(fd, F_GETLK, &region_to_test);
			if (res == -1) {
				printf("F_GETLK failed\n");
				return;
			}

			if (region_to_test.l_pid != -1) {
				printf("Lock would fail. F_GETLK returned:\n");
				printf("\ttype %d, whence %d, start %d, len %d, pid %d\n",
				       region_to_test.l_type, region_to_test.l_whence,
				       (int)region_to_test.l_start,
				       (int)region_to_test.l_len, region_to_test.l_pid);
			} else {
				printf("F_RDLCK - Lock would succeed\n");
			}
		}
		printf("Process %d closing file\n", getpid());
		close(fd);
	} else {
		wait(&stat_val);
	}
}

void region_file_fcnt_clock2()
{
	pid_t pid;
	int fd, res, stat_val;
	struct flock region_to_lock;
	char *test_file = "/tmp/test_lock";

	fd = open(test_file, O_RDWR | O_CREAT, 0666);
	if (fd < 0)
		return;

	region_to_lock.l_type = F_RDLCK;
	region_to_lock.l_whence = SEEK_SET;
	region_to_lock.l_start = 10;
	region_to_lock.l_len = 5;
	printf("Process %d, trying F_RDLCK, region %d to %d\n", getpid(),
	       (int)region_to_lock.l_start,
	       (int)(region_to_lock.l_start + region_to_lock.l_len));
	res = fcntl(fd, F_SETLK, &region_to_lock);
	if (res == -1)
		printf("Process %d - failed to lock region\n", getpid());
	else
		printf("Process %d - obtained lock region\n", getpid());

	region_to_lock.l_type = F_UNLCK;
	region_to_lock.l_whence = SEEK_SET;
	region_to_lock.l_start = 10;
	region_to_lock.l_len = 5;
	printf("Process %d, trying F_UNLCK, region %d to %d\n", getpid(),
	       (int)region_to_lock.l_start,
	       (int)(region_to_lock.l_start + region_to_lock.l_len));
	res = fcntl(fd, F_SETLK, &region_to_lock);
	if (res == -1)
		printf("Process %d - failed to unlock region\n", getpid());
	else
		printf("Process %d - unlocked region\n", getpid());

	region_to_lock.l_type = F_UNLCK;
	region_to_lock.l_whence = SEEK_SET;
	region_to_lock.l_start = 0;
	region_to_lock.l_len = 50;
	printf("Process %d, trying F_UNLCK, region %d to %d\n", getpid(),
	       (int)region_to_lock.l_start,
	       (int)(region_to_lock.l_start + region_to_lock.l_len));
	res = fcntl(fd, F_SETLK, &region_to_lock);
	if (res == -1)
		printf("Process %d - failed to unlock region\n", getpid());
	else
		printf("Process %d - unlocked region\n", getpid());

	region_to_lock.l_type = F_WRLCK;
	region_to_lock.l_whence = SEEK_SET;
	region_to_lock.l_start = 16;
	region_to_lock.l_len = 5;
	printf("Process %d, trying F_WRLCK, region %d to %d\n", getpid(),
	       (int)region_to_lock.l_start,
	       (int)(region_to_lock.l_start + region_to_lock.l_len));
	res = fcntl(fd, F_SETLK, &region_to_lock);
	if (res == -1)
		printf("Process %d - failed to lock region\n", getpid());
	else
		printf("Process %d - obtained lock on region\n", getpid());

	pid = fork();
	if (pid == 0) {
		region_to_lock.l_type = F_RDLCK;
		region_to_lock.l_whence = SEEK_SET;
		region_to_lock.l_start = 40;
		region_to_lock.l_len = 10;
		printf("Process %d, trying F_RDLCK, region %d to %d\n",
		       getpid(), (int)region_to_lock.l_start,
		       (int)(region_to_lock.l_start + region_to_lock.l_len));
		res = fcntl(fd, F_SETLK, &region_to_lock);
		if (res == -1)
			printf("Process %d - failed to lock region\n", getpid());
		else
			printf("Process %d - obtained lock on region\n", getpid());

		region_to_lock.l_type = F_WRLCK;
		region_to_lock.l_whence = SEEK_SET;
		region_to_lock.l_start = 16;
		region_to_lock.l_len = 5;
		printf("Process %d, trying F_WRLCK with wait, region %d to %d\n",
		       getpid(), (int)region_to_lock.l_start,
		       (int)(region_to_lock.l_start + region_to_lock.l_len));
		res = fcntl(fd, F_SETLK, &region_to_lock);
		if (res == -1)
			printf("Process %d - failed to lock region\n", getpid());
		else
			printf("Process %d - obtained lock on region\n", getpid());

		printf("Process %d ending\n", getpid());
		close(fd);
	} else {
		wait(&stat_val);
	}
}

int main(int argc, char *argv[])
{
	int choice = 1;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//lock whole file by specify open flag.
	case 1:
		whole_file_open_lock();
		break;

		//lock whole file by flock method.
	case 2:
		whole_file_flock_lock();
		break;

		//lock whole file by lockf method.
	case 3:
		whole_file_lockf_lock();
		break;

		//test the one file by fcnt clock method.
	case 4:
		region_file_fcnt_clock1();
		break;

		//test the file by fcnt clock method.
	case 5:
		region_file_fcnt_clock2();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
