/******************************************************************************
 * Function: 	System param handle.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile: 	gcc -Wall param_env.c -o param_env
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <syslog.h>
#include <time.h>
#include <pwd.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/select.h>

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <stddef.h>
#include <linux/sysctl.h>

int _sysctl(struct __sysctl_args *args);

#define COMMON_SIZE 		128
#define _XOPEN_SOURCE		/* glibc2 needs this for strptime */

static int count = 0;

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: test the getopt of arg.\n");
	printf(" 2: test the getopt_long of arg.\n");
	printf(" 3: handle the input string format.\n");
	printf(" 4: handle the system envior param.\n");
	printf(" 5: get the system information.\n");
	printf(" 6: handle the system call.\n");
	printf(" 7: handle the system mem resource.\n");
	printf(" 8: handle the system resource limits.\n");
	printf(" 9: handle the system log file.\n");
	printf("10: handle the system time.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int get_opt_by_getopt(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, ":if:lr")) != -1) {
		switch (opt) {
		case 'i':
		case 'l':
		case 'r':
			printf("option: %c\n", opt);
			break;

		case 'f':
			printf("filename: %s\n", optarg);
			break;

		case ':':
			printf("option needs a value\n");
			break;

		case '?':
			printf("unknown option: %c\n", optopt);
			break;
		}
	}

	for (; optind < argc; optind++) 
		printf("argument: %s\n", argv[optind]);

	return 0;
}

int get_opt_by_getoptlong(int argc, char *argv[])
{
	int opt;

	struct option longopts[] = {
		{"initialize", 0, NULL, 'i'},
		{"file", 1, NULL, 'f'},
		{"list", 0, NULL, 'l'},
		{"restart", 0, NULL, 'r'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, ":if:lr", longopts, NULL)) != -1) {
		switch (opt) {
		case 'i':
		case 'l':
		case 'r':
			printf("option: %c\n", opt);
			break;

		case 'f':
			printf("filename: %s\n", optarg);
			break;

		case ':':
			printf("option needs a value\n");
			break;

		case '?':
			printf("unknown option: %c\n", optopt);
			break;
		}
	}

	for (; optind < argc; optind++) 
		printf("argument: %s\n", argv[optind]);

	return 0;
}

int handle_string_by_format()
{
	int index;
	char *token;
	char buf[COMMON_SIZE], tmp[COMMON_SIZE], tep[COMMON_SIZE];

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "ai#dcad");
	memset(tmp, 0, COMMON_SIZE);
	sscanf(buf, "%[dlza#i]", tmp);
	printf("%s\n", tmp);	//output should be: ai#d

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "abd0123DEF##ADD");
	memset(tmp, 0, COMMON_SIZE);
	sscanf(buf, "%[0-9a-zA-Z]", tmp);
	printf("%s\n", tmp);	//output may be: adb0123DEF

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "#^#%-dc9001DGD");
	memset(tmp, 0, COMMON_SIZE);
	sscanf(buf, "%[^0-9a-zA-Z]", tmp);
	printf("%s\n", tmp);	//output may be: #^#%

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "-abce]fgdd");
	memset(tmp, 0, COMMON_SIZE);
	sscanf(buf, "%[abceg-]", tmp);
	printf("%s\n", tmp);	//output should be: -abce

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "hello 123 beike ");
	memset(tmp, 0, COMMON_SIZE);
	memset(tep, 0, COMMON_SIZE);
	sscanf(buf, "%s%*d%s", tmp, tep);
	printf("%s, %s\n", tmp, tep);	//output should be: hello, beike

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "hello world, I am beike!");
	printf("%.*s\n", 10, buf);	
	printf("%*.*s\n", 20, 10, buf);	
	printf("%-*.*s\n", 20, 10, buf);	//output should be:          hello worl 

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "984532421+-.234532Hello,beike!");
	memset(tmp, 0, COMMON_SIZE);
	strcpy(tmp, "01234567890.+-");
	index = strspn(buf, tmp);
	buf[index] = '\0';
	printf("The substring length: %d, content is: %s\n", index, buf);

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "984532421+-.234532Hello,beike!");
	memset(tmp, 0, COMMON_SIZE);
	strcpy(tmp, "+Hello");
	index = strcspn(buf, tmp);
	buf[index] = '\0';
	printf("The substring length: %d, content is: %s\n", index, buf);

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "Welcome To Beijing");
	memset(tmp, 0, COMMON_SIZE);
	strcpy(tmp, "BIT");
	token = strpbrk(buf, tmp);
	if (token)
		printf("The match string is: %s\n", token);
	else
		printf("Not found!\n");

	memset(buf, 0, COMMON_SIZE);
	strcpy(buf, "Hello,beike! Golden:Global:View");
	memset(tep, 0, COMMON_SIZE);
	strcpy(tep, ":,!");
	token = strtok(buf, tep);
	while (token) {
		printf("[%s] ", token);
		token = strtok(NULL, tep);
	}
	printf("\n");

	return 0;
}

int handle_environ_value()
{
	extern char **environ;
	char **envs = environ;
	char var[COMMON_SIZE], value[COMMON_SIZE], string[COMMON_SIZE] = "";

	printf("Input the envior var: ");
	scanf("%s", var);
	if (getenv(var)) 
		strcpy(value, getenv(var));
	else
		strcpy(value, "");

	if (strcmp(value, "")) {
		printf("Variable %s has value %s\n", var, value);
		return 0;
	} else {
		printf("Variable %s has no value\n", var);
	}

	printf("Input the value of envior var %s: ", var);
	scanf("%s", value);
	sprintf(string, "%s=%s", var, value);
	printf("Calling putenv with: %s\n", string);
	if (putenv(string) != 0) {
		perror("putenv");
		return -1;
	}

	printf("All environ as follow: \n");
	while (*envs) {
		printf("%s\n", *envs);
		envs++;
	}

	return 0;
}

int handle_system_info()
{
	uid_t uid;
	gid_t gid;
	struct passwd *pw;
	char computer[256];
	struct utsname uts;
	struct s {
		int i;
		char c;
		double d;
		char a[];
	};
	union {
		int a;
		char b;
	} c;

	uid = getuid();
	gid = getgid();

	pw = getpwuid(uid);
	printf("User is %s, uid=%d, gid=%d\n", getlogin(), uid, gid);
	printf("UID passwd entry:\n\tname=%s, uid=%d, gid=%d, home=%s, shell=%s\n",
	     pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell);

	pw = getpwnam("root");
	printf("Root passwd entry:\n");
	printf("\tname=%s, uid=%d, gid=%d, home=%s, shell=%s\n",
	       pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir, pw->pw_shell);

	if (gethostname(computer, 255) != 0 || uname(&uts) < 0) {
		perror("gethostname");
		return -1;
	}
	printf("Computer host name is %s\n", computer);
	printf("System is %s on %s hardware\n", uts.sysname, uts.machine);
	printf("Nodename is %s\n", uts.nodename);
	printf("Version is %s, %s\n", uts.release, uts.version);

	printf("Offsets: i=%ld; c=%ld; d=%ld a=%ld\n",
	       (long)offsetof(struct s, i), (long)offsetof(struct s, c),
	       (long)offsetof(struct s, d), (long)offsetof(struct s, a));
	printf("Sizeof(struct s)=%ld\n", (long)sizeof(struct s));

	c.a = 1;
	if (c.b == 1) 
		printf("System is low  byte save model\n");
	else 
		printf("System is high byte save model\n");

	if (__GNUC__ > 3 || (__GNUC__ == 3 && (__GNUC_MINOR__ > 2 || 
		(__GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ > 0))))
		printf("GNUC version is later than 3.3.2\n");
	else
		printf("GNUC version is older than 3.3.2\n");

	printf("Version : %s\n", __VERSION__);
	printf("DATE : %s %s\n", __DATE__, __TIME__);
	printf("line %d in %s %s(%s)\n", __LINE__, __FILE__, __BASE_FILE__, __FUNCTION__);

	return 0;
}

int handle_system_call(void)
{
	size_t osnamelth;
	char osname[COMMON_SIZE];
	struct __sysctl_args args;
	int name[] = { CTL_KERN, KERN_OSTYPE };

	memset(&args, 0, sizeof(struct __sysctl_args));
	args.name = name;
	args.nlen = sizeof(name) / sizeof(name[0]);
	args.oldval = osname;
	args.oldlenp = &osnamelth;
	osnamelth = sizeof(osname);
	if (syscall(SYS__sysctl, &args) == -1) {
		perror("_sysctl");
		return -1;
	}
	printf("This machine is running %*s!\n", osnamelth, osname);

	return 0;
}

void get_memoccupy()
{
	FILE *fd;
	char buf[1024] = { 0 };
	unsigned long mtotals, mfrees;
	unsigned long stotals, sfrees;
	
	fd = fopen("/proc/meminfo", "r");
	while (fgets(buf, sizeof(buf) - 1, fd)) {
		if (strstr(buf, "MemTotal"))
			sscanf(buf, "%*s %lu %*s", &mtotals);
		else if (strstr(buf, "MemFree"))
			sscanf(buf, "%*s %lu %*s", &mfrees);
		else if (strstr(buf, "SwapTotal"))
			sscanf(buf, "%*s %lu %*s", &stotals);
		else if (strstr(buf, "SwapFree"))
			sscanf(buf, "%*s %lu %*s", &sfrees);
		else
			continue;
	}
	printf("memory occupy: %luKB, swap occupy: %luKB\n",
		   mtotals - mfrees, stotals - sfrees);
	fclose(fd);
}

int handle_mem_alloc()
{
	char *ptr;
	int n, input;
	size_t i, psize;
	int *snum, *dnum;
	const int asize = 32 * 1024 * 1024;

	psize = getpagesize();
	get_memoccupy();
	ptr = (char *)malloc(asize);
	mlock(ptr, asize);
	for (i = 0; i < asize; i += psize)
	  ptr[i] = 0;
	get_memoccupy();
	munlock(ptr, asize);
	free(ptr);

	get_memoccupy();
	mlockall(MCL_CURRENT | MCL_FUTURE);
	ptr = (char *)malloc(asize);
	for (i = 0; i < asize; i += psize)
	  ptr[i] = 0;
	get_memoccupy();
	munlockall();
	free(ptr);

	dnum = (int *)malloc(5 * sizeof(int));
	for (n = 0; n < 5; n++) {
		*(dnum + n) = n;
		printf("old data: %d\n", *(dnum + n));
	}

	printf("Enter a value to remalloc (enter 0 to stop)\n");
	scanf("%d", &input);
	snum = (int *)realloc(dnum, (input + 5) * sizeof(int));
	for (n = 0; n < 5; n++) 
		printf("the snum's data copy from dnum: %d\n", *(snum + n));
	for (n = 0; n < input; n++) {
		*(snum + 5 + n) = n * 2;
		printf("new data: %d\n", *(snum + 5 + n));
	}
	printf("\n");
	free(snum);
	snum = NULL;
	
	return 0;
}

int handle_resource_limits()
{
	int priority;
	struct rusage r_usage;
	struct rlimit r_limit;

	getrusage(RUSAGE_SELF, &r_usage);
	printf("CPU usage: User = %ld.%06ld, System = %ld.%06ld\n",
	       r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec,
	       r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);

	priority = getpriority(PRIO_PROCESS, getpid());
	printf("Current priority = %d\n", priority);

	getrlimit(RLIMIT_FSIZE, &r_limit);
	printf("Current FSIZE limit: soft = %ld, hard = %ld\n",
	       r_limit.rlim_cur, r_limit.rlim_max);

	r_limit.rlim_cur = 2048;
	r_limit.rlim_max = 4096;
	printf("Setting a 2K file size limit\n");
	setrlimit(RLIMIT_FSIZE, &r_limit);
	getrlimit(RLIMIT_NOFILE, &r_limit);
	printf("Current FILE limit: soft = %ld, hard = %ld\n", 
		   r_limit.rlim_cur, r_limit.rlim_max);

	chdir("/tmp");
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

	return 0;
}

int handle_log_file()
{
	int logmask;

	openlog("logmask", LOG_PID | LOG_CONS, LOG_USER);
	syslog(LOG_INFO, "informative message, pid = %d", getpid());
	syslog(LOG_DEBUG, "debug message, should appear");

	logmask = setlogmask(LOG_UPTO(LOG_NOTICE));
	syslog(LOG_DEBUG, "debug message, should not appear");
	closelog();
	
	return 0;
}

int time_function()
{
	char buff[COMMON_SIZE];
	time_t timep, timep1, timep2;

	time(&timep);
	printf("ctime: %s", ctime(&timep));

	time(&timep1);
	printf("gmtime: %s", asctime(gmtime(&timep1)));

	time(&timep2);
	printf("localtime: %s", asctime(localtime(&timep2)));

	memset(buff, '\0', COMMON_SIZE);
	strftime(buff, COMMON_SIZE, "%Z", gmtime(&timep1));
	printf("globle TZ: %s\n", buff);

	memset(buff, '\0', COMMON_SIZE);
	strftime(buff, COMMON_SIZE, "%Z", localtime(&timep2));
	printf("local TZ: %s\n", buff);

	putenv("TZ=UTC");
	tzset();
	time(&timep1);
	printf("UTC time: %s\n", asctime(localtime(&timep1)));

	return 0;
}

void timefunc(int sig)
{
	fprintf(stderr, "Received ITIMER_PROF: %d\n", count++);
	signal(SIGPROF, timefunc);
}

int time_struct()
{
	time_t the_time;
	char *result, buf[256];
	struct itimerval value;
	struct tm *tm_ptr, timestruct;

	(void)time(&the_time);
	tm_ptr = localtime(&the_time);
	strftime(buf, 256, "%A %d %B, %I:%S %p", tm_ptr);
	printf("strftime gives: %s\n", buf);

	strcpy(buf, "Thu 26 July 2007, 17:53 will do fine");
	printf("calling strptime with: %s\n", buf);
	tm_ptr = &timestruct;
	result = strptime(buf, "%a %d %b %Y, %R", tm_ptr);
	printf("strptime consumed up to: %s\n", result);
	printf("strptime gives:\n");
	printf("date: %02d/%02d/%02d\n", tm_ptr->tm_year % COMMON_SIZE,
	       tm_ptr->tm_mon + 1, tm_ptr->tm_mday);
	printf("time: %02d:%02d\n", tm_ptr->tm_hour, tm_ptr->tm_min);

	value.it_value.tv_sec = 1;
	value.it_value.tv_usec = 500000;
	value.it_interval.tv_sec = 2;
	value.it_interval.tv_usec = 0;
	signal(SIGPROF, timefunc);
	setitimer(ITIMER_PROF, &value, NULL);
	while (1) ;

	return 0;
}

void CaculateDay()
{
	char sstart[64], send[64];
	struct tm *tm_1, *tm_2, *start, *end;
	time_t time_1, time_2, startval, endval;

	(void)time(&time_1);
	tm_1 = localtime(&time_1);
	startval = time_1 - tm_1->tm_hour * 3600 - tm_1->tm_min * 60 - 
		tm_1->tm_sec - 24 * 3600 + 1;
	start = localtime(&startval);
	snprintf(sstart, strlen(sstart) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		start->tm_year + 1900, start->tm_mon + 1, start->tm_mday,
		start->tm_hour, start->tm_min, start->tm_sec);

	(void)time(&time_2);
	tm_2 = localtime(&time_2);
	endval = time_2 - tm_2->tm_hour * 3600 - tm_2->tm_min * 60 - 
		tm_2->tm_sec - 1;
	end = localtime(&endval);
	snprintf(send, strlen(send) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
		end->tm_hour, end->tm_min, end->tm_sec);

	printf("The lastest day time from %s to %s\n", sstart, send);
}

void CaculateWeek()
{
	char week[32], sstart[64], send[64];
	struct tm *tm_1, *tm_2, *start, *end;
	time_t time_1, time_2, startval, endval;

	(void)time(&time_1);
	tm_1 = localtime(&time_1);
	strftime(week, sizeof(week), "%u", tm_1);
	startval = time_1 - (atoi(week) - 1) * 24 * 3600 - tm_1->tm_hour * 3600 -
		tm_1->tm_min * 60 - tm_1->tm_sec - 7 * 24 * 3600 + 1;
	start = localtime(&startval);
	snprintf(sstart, strlen(sstart) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		start->tm_year + 1900, start->tm_mon + 1, start->tm_mday,
		start->tm_hour, start->tm_min, start->tm_sec);

	(void)time(&time_2);
	tm_2 = localtime(&time_2);
	endval = time_2 - (atoi(week) - 1) * 24 * 3600 - tm_2->tm_hour * 3600 -
		tm_2->tm_min * 60 - tm_2->tm_sec - 1;
	end = localtime(&endval);
	snprintf(send, strlen(send) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
		end->tm_hour, end->tm_min, end->tm_sec);

	printf("The lastest week time from %s to %s\n", sstart, send);
}

void CaculateMonth()
{
	int days;
	char sstart[64], send[64];
	struct tm *tm_1, *tm_2, *start, *end;
	time_t time_1, time_2, startval, endval;

	(void)time(&time_2);
	tm_2 = localtime(&time_2);
	endval = time_2 - (tm_2->tm_mday - 1) * 24 * 3600 - tm_2->tm_hour * 3600 -
		tm_2->tm_min * 60 - tm_2->tm_sec - 1;
	end = localtime(&endval);
	snprintf(send, strlen(send) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
		end->tm_hour, end->tm_min, end->tm_sec);
	days = end->tm_mday;

	(void)time(&time_1);
	tm_1 = localtime(&time_1);
	startval = time_1 - (tm_1->tm_mday - 1) * 24 * 3600 - tm_1->tm_hour * 3600 -
		tm_1->tm_min * 60 - tm_1->tm_sec - days * 24 * 3600 + 1;
	start = localtime(&startval);
	snprintf(sstart, strlen(sstart) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
		start->tm_year + 1900, start->tm_mon + 1, start->tm_mday,
		start->tm_hour, start->tm_min, start->tm_sec);

	printf("The lastest month time from %s to %s\n", sstart, send);
}

void time_transfer()
{
	CaculateDay();
	CaculateWeek();
	CaculateMonth();
}

void handle_system_time()
{
	time_function();
	time_struct();
	time_transfer();
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//test the opt of arg.
	case 1:
		get_opt_by_getopt(argc, argv);
		break;

		//test the long opt of arg.
	case 2:
		get_opt_by_getoptlong(argc, argv);
		break;

		//handle the input string format.
	case 3:
		handle_string_by_format();
		break;

		//handle the system envior param.
	case 4:
		handle_environ_value();
		break;

		//get the system information.
	case 5:
		handle_system_info();
		break;

		//handle the system call.
	case 6:
		handle_system_call();
		break;

		//handle the system mem resource.
	case 7:
		handle_mem_alloc();
		break;

		//handle the system resource limits.
	case 8:
		handle_resource_limits();
		break;

		//handle the system log file.
	case 9:
		handle_log_file();
		break;

		//handle the system time.
	case 10:
		handle_system_time();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
