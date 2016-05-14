/******************************************************************************
 * Function: 	System process called.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall process_base.c -o process_base
******************************************************************************/
#define _GNU_SOURCE
#include <unistd.h>
#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>

static int alarm_fired = 0;
static int globl_key = 6;

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test system called\n");
	printf(" 2: test exec called\n");
	printf(" 3: test exec custom\n");
	printf(" 4: test fork called\n");
	printf(" 5: test old signal handle\n");
	printf(" 6: test new signal handle\n");
	printf(" 7: test new itime signal handle\n");
	printf(" 8: test daemon fork called\n");
	printf(" 9: test another daemon fork called\n");
	printf("10: test vfork called\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int process_system_called()
{
	uid_t uid, euid, suid;

	printf("\nGet process information: ");
	getresuid(&uid, &euid, &suid);
	printf("uid=%d, euid=%d, suid=%d\n", uid, euid, suid);

	printf("Set reuid (0,500): ");
	setreuid(0, 500);
	printf("uid=%d, euid=%d\n", getuid(), geteuid());

	printf("Set uid 0: ");
	setuid(0);
	printf("uid=%d, euid=%d\n", getuid(), geteuid());

	printf("Running ps with system:\n");
	system("ps ax");
	printf("Done.\n");

	return 0;
}

int process_exec_called()
{
	int i;
	glob_t gl;
	char *const ps_argv[] = { "ps", "ax", 0 };
	char *const ps_envp[] = { "PATH=/bin:/usr/bin", "TERM=console", 0 };

	printf("\nRunning ps with exec\n");

	execl("/bin/ps", "ps", "ax", (char *)0);
	execlp("ps", "ps", "ax", (char *)0);
	execle("/bin/ps", "ps", "ax", (char *)0, ps_envp);

	execv("/bin/ps", ps_argv);
	execvp("ps", ps_argv);
	execve("/bin/ps", ps_argv, ps_envp);

	gl.gl_offs = 2;
	glob("*.c", GLOB_DOOFFS, NULL, &gl);
	glob("../*.c", GLOB_DOOFFS | GLOB_APPEND, NULL, &gl);
	printf("Match *.c ../*.c, result as follow:\n");
	for (i = 0; i < gl.gl_pathc; ++i) {
		if (gl.gl_pathv[i])
			printf(" ,%s", gl.gl_pathv[i]);
	}
	globfree(&gl);

	printf("Done.\n");

	return 0;
}

int process_exec_custom(char *filename)
{
	if (!freopen(filename, "r", stdin))
		return -1;

	system("gcc upper.c -o upper");
	execl("./upper", "upper", (char *)0);
	perror("Could not exec ./upper!");

	return 0;
}

int process_fork_called()
{
	pid_t pid;
	char *message;
	int var = 88;
	int n, exit_code;
	uid_t uid, euid, suid;

	printf("\nFork program starting...\n");
	getresuid(&uid, &euid, &suid);
	printf("uid=%d, euid=%d, suid=%d\n", uid, euid, suid);
	printf("In beginning: glob=%d, var=%d\n", globl_key, var);

	pid = fork();
	switch (pid) {
	case -1:
		return -1;

	case 0:
		message = "This is the child...";
		n = 5;
		exit_code = 37;
		globl_key++;
		var++;
		getresuid(&uid, &euid, &suid);
		printf("uid=%d, euid=%d, suid=%d\n", uid, euid, suid);
		printf("child-%d: ppid=%d, pgid=%d ", getpid(), getppid(), getpgid(0));
		printf("sid=%d, pigd=%d\n", getsid(getpid()), getpgid(getpid()));
		printf("Child: glob=%d, var=%d\n", globl_key, var);
		break;

	default:
		message = "This is the parent...";
		n = 3;
		exit_code = 0;
		printf("Parent: glob=%d, var=%d\n", globl_key, var);
		break;
	}

	for (; n > 0; n--) {
		puts(message);
		sleep(1);
	}

	if (pid) {
		int stat_val;
		pid_t child_pid;

		child_pid = wait(&stat_val);
		printf("Child has finished: PID = %d\n", child_pid);
		if (WIFEXITED(stat_val))
			printf("Child exited with code %d\n", WEXITSTATUS(stat_val));
		else
			printf("Child terminated abnormally\n");
	}

	return exit_code;
}

void signal_oldhandle(int signal)
{
	alarm_fired = 1;
}

int process_fork_oldsignal()
{
	pid_t pid;

	printf("\nalarm application starting\n");

	pid = fork();
	switch (pid) {
	case -1:
		perror("Fork fail!");
		exit(1);

	case 0:
		sleep(5);
		kill(getppid(), SIGALRM);
		exit(0);
	}

	printf("waiting for alarm to go off\n");

	signal(SIGALRM, signal_oldhandle);
	pause();

	if (alarm_fired)
		printf("alarm now!\n");

	printf("done\n");

	return 0;
}

void signal_newhandle(int signal)
{
	printf("OUCH! - I got signal %d\n", signal);
}

int process_fork_signal()
{
	struct sigaction act;

	act.sa_handler = signal_newhandle;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, 0);

	while (1) {
		printf("Hello World!\n");
		sleep(1);
	}

	return 0;
}

int process_fork_itimer_signal()
{
	struct itimerval tick;

	signal(SIGALRM, signal_newhandle);

	memset(&tick, 0, sizeof(tick));
	tick.it_value.tv_sec = 1;
	tick.it_value.tv_usec = 0;
	tick.it_interval.tv_sec = 1;
	tick.it_interval.tv_usec = 0;

	if (setitimer(ITIMER_REAL, &tick, NULL))
		return -1;

	printf("Wait...\n");
	while (1)
		sleep(1);

	return 0;
}

void process_fork_daemon1(char *logname)
{
	time_t ticks;
	int pid, i;

	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	if ((pid = fork())) {
		exit(EXIT_SUCCESS);
	} else if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	setsid();
	if ((pid = fork())) {
		exit(EXIT_SUCCESS);
	} else if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < NOFILE; ++i)
		close(i);

	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
	chdir("/tmp");
	umask(0);
	//daemon(0,0);

	signal(SIGCHLD, SIG_IGN);
	openlog(logname, LOG_PID, LOG_KERN);

	while (1) {
		sleep(1);
		ticks = time(NULL);
		syslog(LOG_INFO, "%s", asctime(localtime(&ticks)));
	}
}

int process_fork_daemon2(char *logname)
{
	int fd;
	pid_t pid;
	time_t ticks;

	switch (fork()) {
	case -1:
		return -1;

	case 0:
		break;

	default:
		exit(0);
	}

	pid = getpid();

	if (setsid() == -1)
		return -1;

	umask(0);

	fd = open("/dev/null", O_RDWR);
	if (fd == -1)
		return -1;

	if (dup2(fd, STDIN_FILENO) == -1)
		return -1;

	if (dup2(fd, STDOUT_FILENO) == -1)
		return -1;

	if (fd > STDERR_FILENO) {
		if (close(fd) == -1)
			return -1;
	}

	signal(SIGCHLD, SIG_IGN);
	openlog(logname, LOG_PID, LOG_KERN);

	while (1) {
		sleep(1);
		ticks = time(NULL);
		syslog(LOG_INFO, "%s", asctime(localtime(&ticks)));
	}

	return 0;
}

int process_fork_vfork()
{
	pid_t pid;
	int var = 88;

	printf("\nIn beginning: glob=%d, var=%d\n", globl_key, var);
	if ((pid = vfork()) < 0) {
		return 1;
	} else if (pid == 0) {
		globl_key++;
		var++;
		printf("Child: glob=%d, var=%d\n", globl_key, var);
		exit(0);
	}

	printf("Parent: glob=%d, var=%d\n", globl_key, var);

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test system called
	case 1:
		process_system_called();
		break;

		//test exec called 
	case 2:
		process_exec_called();
		break;

		//test execl custom 
	case 3:
		process_exec_custom("/tmp/file.in");
		break;

		//test fork called
	case 4:
		process_fork_called();
		break;

		//test old signal handle  
	case 5:
		process_fork_oldsignal();
		break;

		//test new signal handle
	case 6:
		process_fork_signal();
		break;

		//test new itime signal handle
	case 7:
		process_fork_itimer_signal();
		break;

		//test daemon fork called
	case 8:
		process_fork_daemon1(argv[0]);
		break;

		//test another daemon fork called
	case 9:
		process_fork_daemon2(argv[0]);
		break;

		//test vfork called
	case 10:
		process_fork_vfork();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
