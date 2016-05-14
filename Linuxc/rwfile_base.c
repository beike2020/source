/******************************************************************************
 * Function: 	File read and write method.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01							
 * Compile:	gcc -Wall rwfile_base.c -o rwfile_base -linotifytools
******************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <inotifytools/inotify.h>
#include <inotifytools/inotifytools.h>

#define  _XOPEN_SOURCE 600
#include <fcntl.h>

#define  NRECORDS 100

typedef struct {
	int integer;
	char string[24];
} RECORD;

static char stats[128];

enum {
    ACTION_NULL_WD,
    ACTION_ADD_WD,
    ACTION_DEL_WD,
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: creat a file by open flag.\n");
	printf(" 2: creat a tmpfile by mkstemp.\n");
	printf(" 3: copy a char one time use read and write from file.\n");
	printf(" 4: copy a block one time use read and write from file.\n");
	printf(" 5: modify a block struct use mmap from file.\n");
	printf(" 6: copy a block one time use read and write from term.\n");
	printf(" 7: get the length of file by lseek.\n");
	printf(" 8: copy a char one time use fget and fput from file.\n");
	printf(" 9: copy a format string one time use fprintf from file.\n");
	printf("10: copy a block one time use read and write from file.\n");
	printf("11: get the length of file by fseek.\n");
	printf("12: get the attribute of file by ioctl api.\n");
	printf("13: get the attribute of file by stat api.\n");
	printf("14: read a dir use dir.\n");
	printf("15: select a dir use scandir filter.\n");
	printf("16: find a dir by readlink.\n");
	printf("17: notify a dir or file event.\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int creat_file_by_flag(char *file)
{
	int fd;
	struct stat st;

	if (stat(file, &st) < 0)
		return -1;

	fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return -1;

	//echo [1][2][3] > /proc/sys/vm/drop_caches
	if (posix_fadvise(fd, 0, st.st_size, POSIX_FADV_DONTNEED) != 0)
		perror("Cache FADV_DONTNEED failed\n");
	else
		printf("Cache FADV_DONTNEED done!\n");

	close(fd);

	return 0;
}

int creat_file_by_mkstemp()
{
	int fd;
	char temp_file[] = "tmp_XXXXXX";
	char content[] = "This is written by the program.\n";

	fd = mkstemp(temp_file);
	if (fd < 0)
		return -1;

	printf("Temp file id is %d!\n", fd);
	write(fd, content, sizeof(content));
	unlink(temp_file);
	close(fd);

	return 0;
}

int open_file_by_char(char *file1, char *file2)
{
	int in, out, c;

	in = open(file1, O_RDONLY);
	out = open(file2, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	while (read(in, &c, 1) == 1)
		write(out, &c, 1);
	close(in);
	close(out);

	return 0;
}

int open_file_by_block(char *file1, char *file2)
{
	int in, out, nread;
	char block[1024];

	in = open(file1, O_RDONLY);
	out = open(file2, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	while ((nread = read(in, block, sizeof(block))) > 0)
		write(out, block, nread);
	close(in);
	close(out);

	return 0;
}

int open_file_by_mmap(char *file1)
{
	FILE *fp;
	int i, fd;
	RECORD record, *mapped;

	fp = fopen("/tmp/records.dat", "w+");
	if (fp == NULL)
		return -1;

	for (i = 0; i < NRECORDS; i++) {
		record.integer = i;
		sprintf(record.string, "RECORD-%d", i);
		fwrite(&record, sizeof(record), 1, fp);
	}
	fclose(fp);

	fp = fopen("/tmp/records.dat", "r+");
	fseek(fp, 43 * sizeof(record), SEEK_SET);
	fread(&record, sizeof(record), 1, fp);
	printf("Init integer=%d, string=%s\n", record.integer, record.string);

	record.integer = 143;
	sprintf(record.string, "RECORD-%d", record.integer);
	fseek(fp, 43 * sizeof(record), SEEK_SET);
	fwrite(&record, sizeof(record), 1, fp);
	printf("Now integer=%d, string=%s\n", record.integer, record.string);
	fclose(fp);

	fd = open("/tmp/records.dat", O_RDWR);
	mapped = (RECORD *) mmap(0, NRECORDS * sizeof(record),
				 PROT_READ, MAP_SHARED, fd, 0);
	mprotect(mapped, NRECORDS * sizeof(record), PROT_WRITE);
	mapped[43].integer = 243;
	sprintf(mapped[43].string, "RECORD-%d", mapped[43].integer);
	msync((void *)mapped, NRECORDS * sizeof(record), MS_ASYNC);
	printf("Fin integer=%d, string=%s\n", mapped[43].integer,
	       mapped[43].string);
	munmap((void *)mapped, NRECORDS * sizeof(record));
	close(fd);

	return 0;
}

int open_term_by_block()
{
	int nread;
	char buffer[128];

	memset(buffer, 0, sizeof(buffer));
	nread = read(0, buffer, 128);
	if (nread == -1)
		write(2, "A read error has occurred\n", 26);

	if ((write(1, buffer, nread)) != nread)
		write(2, "A write error has occurred\n", 27);

	return 0;
}

int open_file_by_lseek(char *file)
{
	int in;
	long fsize;

	in = open(file, O_RDONLY);
	if (in < 0)
		return -1;

	fsize = lseek(in, 0L, SEEK_END);
	printf("File %s size is %ld!\n", file, fsize);
	close(in);

	execl("/bin/ls", "ls", "-l", file, NULL);

	return 0;
}

int fopen_file_by_char(char *file1, char *file2)
{
	int c;
	char buf[128];
	FILE *fin, *fout;

	fin = fopen(file1, "r");
	fout = fopen(file2, "w");
	if (fin == NULL || fout == NULL)
		return -1;

	if (setvbuf(fout, buf, _IOLBF, 128))
		return -1;

	if (fout->_flags & _IO_UNBUFFERED)
		printf("Output set NO vbuf.\n");
	else if (fout->_flags & _IO_LINE_BUF)
		printf("Output set vbuf, size is %d.\n",
		       fout->_IO_buf_end - fout->_IO_buf_base);
	else
		printf("Output set fully buffered or modified.\n");

	printf("Input file %s, read_base: %p, read_end: %p\n", file1,
	       fout->_IO_read_base, fout->_IO_read_end);
	printf("Output file %s, write_base: %p, write_end: %p\n", file2,
	       fout->_IO_write_base, fout->_IO_write_end);

	while ((c = fgetc(fin)) != EOF)
		fputc(c, fout);
	
	sync();
	fclose(fin);
	fclose(fout);

	return 0;
}

int fopen_file_by_format(char *file1, char *file2)
{
	char buf[256];
	FILE *fin, *fout;

	memset(buf, '\0', sizeof(buf));
	fin = fopen(file1, "r");
	fout = fopen(file2, "w");
	if (fin == NULL || fout == NULL)
		return -1;

	if (setvbuf(fout, buf, _IOLBF, 256))
		return -1;

	while (fgets(buf, 256, fin)) {
		fprintf(fout, "%s", buf);
		memset(buf, '\0', sizeof(buf));
	}

	if (feof(fin))
		perror("Read end");

	fclose(fin);
	fclose(fout);

	return 0;
}

int fopen_file_by_struct(char *file)
{
	int i;
	FILE *fin;
	RECORD record;

	fin = fopen(file, "w+");
	if (fin == NULL)
		return -1;

	for (i = 0; i < NRECORDS; i++) {
		record.integer = i;
		sprintf(record.string, "RECORD-%d", i);
		fwrite(&record, sizeof(record), 1, fin);
	}
	fclose(fin);

	fin = fopen(file, "r+");
	if (fin == NULL)
		return -1;

	fseek(fin, 43 * sizeof(record), SEEK_SET);
	fread(&record, sizeof(record), 1, fin);
	printf("Init integer=%d, string=%s\n", record.integer, record.string);

	record.integer = 143;
	sprintf(record.string, "RECORD-%d", record.integer);
	fseek(fin, 43 * sizeof(record), SEEK_SET);
	fwrite(&record, sizeof(record), 1, fin);
	printf("Fin integer=%d, string=%s\n", record.integer, record.string);

	fclose(fin);

	return 0;
}

int fopen_file_by_fseek(char *file)
{
	FILE *fp;
	long filelen = 0;

	fp = fopen(file, "r+");
	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	fclose(fp);
	clearerr(fp);
	printf("File %s length is %ld!\n", file, filelen);

	execl("/bin/ls", "ls", "-l", file, NULL);

	return 0;
}

int handle_file_by_ioctl(char *ifname)
{
	int sockfd;
	char *address;
	struct ifreq ifr;
	struct sockaddr_in *addr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
		perror("ioctl SIOCGIFADDR");
		return -1;
	}
	addr = (struct sockaddr_in *)&(ifr.ifr_addr);
	address = inet_ntoa(addr->sin_addr);
	printf("%s inet addr: %s\n", ifname, address);

	if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) == -1)
		return -1;
	addr = (struct sockaddr_in *)&ifr.ifr_broadaddr;
	address = inet_ntoa(addr->sin_addr);
	printf("%s broad addr: %s\n", ifname, address);

	if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) == -1)
		return -1;
	addr = (struct sockaddr_in *)&ifr.ifr_addr;
	address = inet_ntoa(addr->sin_addr);
	printf("%s inet mask: %s\n", ifname, address);

	return 0;
}

int stat_file_by_stat(char *file)
{
	char buf[128];
	struct tm *mt;
	struct stat st;
	struct group *gbuf;
	struct passwd *pbuf;

	strcpy(stats, "----------");
	memset(&st, 0, sizeof(struct stat));

	if (access(file, W_OK | R_OK) || (stat(file, &st) < 0)) {
		perror("access and stat");
		return -1;
	}

	if (S_ISDIR(st.st_mode))
		stats[0] = 'd';
	if (S_ISCHR(st.st_mode))
		stats[0] = 'c';
	if (S_ISBLK(st.st_mode))
		stats[0] = 'b';
	if (S_ISFIFO(st.st_mode))
		stats[0] = 'f';
	if (S_ISLNK(st.st_mode))
		stats[0] = 'l';
	if (S_ISSOCK(st.st_mode))
		stats[0] = 's';

	if (S_IRUSR & st.st_mode)
		stats[1] = 'r';
	if (S_IWUSR & st.st_mode)
		stats[2] = 'w';
	if ((S_IXUSR & st.st_mode) && (S_ISUID & st.st_mode))
		stats[3] = 's';
	else if (!(S_IXUSR & st.st_mode) && (S_ISUID & st.st_mode))
		stats[3] = 'S';
	else if ((S_IXUSR & st.st_mode) && !(S_ISUID & st.st_mode))
		stats[3] = 'x';

	if (S_IRGRP & st.st_mode)
		stats[4] = 'r';
	if (S_IWGRP & st.st_mode)
		stats[5] = 'w';
	if ((S_IXGRP & st.st_mode) && (S_ISGID & st.st_mode))
		stats[6] = 's';
	else if (!(S_IXGRP & st.st_mode) && (S_ISGID & st.st_mode))
		stats[6] = 'S';
	else if ((S_IXGRP & st.st_mode) && !(S_ISGID & st.st_mode))
		stats[6] = 'x';

	if (S_IROTH & st.st_mode)
		stats[7] = 'r';
	if (S_IWOTH & st.st_mode)
		stats[8] = 'w';
	if ((S_IXOTH & st.st_mode) && (S_ISVTX & st.st_mode))
		stats[9] = 't';
	else if (!(S_IXOTH & st.st_mode) && (S_ISVTX & st.st_mode))
		stats[9] = 'T';
	else if ((S_IXOTH & st.st_mode) && !(S_ISVTX & st.st_mode))
		stats[9] = 'x';

	pbuf = getpwuid(st.st_uid);
	gbuf = getgrgid(st.st_gid);
	mt = localtime(&st.st_mtime);
	strftime(buf, sizeof(buf), "%b %d %H:%M", mt);
	sprintf(stats + 10, " %d %-5s %-5s %6lu %s %s",
			st.st_nlink, pbuf->pw_name, gbuf->gr_name, st.st_size, buf, file);

	return 0;
}

int open_dir_by_readdir(char *dir, int depth)
{
	DIR *dp;
	struct stat statbuf;
	struct dirent *entry;

	if ((dp = opendir(dir)) == NULL)
		return -1;
	
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(".", entry->d_name) == 0 ||
			    strcmp("..", entry->d_name) == 0)
				continue;

			printf("%*s%s/\n", depth, " ", entry->d_name);
			open_dir_by_readdir(entry->d_name, depth + 4);
		} else {
			stat_file_by_stat(entry->d_name);
			printf("%*s%s\n", depth, " ", stats);
		}
	}
	chdir("..");
	closedir(dp);

	return 0;
}

int handle_dir_filter(const struct dirent *d)
{
	int in, out, len;
	char infile[128], buf[1024];

	if (d->d_name[0] == '.' || !strstr(d->d_name, ".c"))
		return 0;

	sprintf((char *)infile, "./%s", d->d_name);
	in = open(infile, O_RDONLY);
	out = open("/tmp/tmp.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (in < 0 || out < 0)
		return 0;

	while ((len = read(in, buf, 1024)) > 0)
		if (write(out, buf, len) != len)
			break;

	close(in);
	close(out);

	return 1;
}

int open_dir_by_filter()
{
	int n;
	char resolved_path[128];
	struct dirent **namelist;

	realpath(".", resolved_path);
	printf("current path: %s\n", resolved_path);

	n = scandir(".", &namelist, handle_dir_filter, alphasort);
	if (n < 0)
		return -1;

	while (n--) {
		printf("%s\n", namelist[n]->d_name);
		free(namelist[n]);
	}
	free(namelist);

	return 0;
}

void get_exe_path()
{
	int i, rst;
	char path[1024];

	memset(path, 0, sizeof(path));
	rst = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (rst < 0 || (rst >= sizeof(path) - 1))
		return;

	path[rst] = '\0';
	for (i = rst; i >= 0; i--) {
		if (path[i] == '/') {
			path[i + 1] = '\0';
			break;
		}
	}

	printf("%s\n", path);
}

int notify_file_events()
{
    char path[256];
    const char *monitor_path = ".";
    struct inotify_event *event = NULL;

    printf("monitor dir: %s\n", monitor_path);

    if (!inotifytools_initialize()) {
        printf("inotifytools_initialize failed: %d\n", inotifytools_error());
		return -1;
    }

    inotifytools_initialize_stats();
 
    if (!inotifytools_watch_recursively(monitor_path, IN_ALL_EVENTS)) {
        printf("inotifytools_watch_recursively failed: %d\n", inotifytools_error());
		return -1;
    }
 
    inotifytools_set_printf_timefmt("%F %T");

	event = inotifytools_next_event(-1);
    while (event) {
		if ((strlen(event->name) > 2) && !strstr(event->name, ".swp") && !strstr(event->name, ".swx"))
        	inotifytools_printf(event, "%T %w%f %e\n" );
 
        if (IN_ISDIR&event->mask) {
            int action = ACTION_NULL_WD;
 
            if ((IN_DELETE|IN_DELETE_SELF|IN_MOVED_FROM)&event->mask) {
                action = ACTION_DEL_WD;
                snprintf(path, sizeof(path), "%s%s",
                        inotifytools_filename_from_wd(event->wd), event->name);
                printf("remove path[%s] from wd\n", path);
            } else if (((IN_CREATE|IN_MOVED_TO)&event->mask) && (IN_ISDIR&event->mask)) {
                action = ACTION_ADD_WD;
                snprintf(path, sizeof(path), "%s%s",
                        inotifytools_filename_from_wd(event->wd), event->name);
                printf("add path[%s] into wd\n", path);
            }
 
            if (ACTION_ADD_WD == action) {
                if (!inotifytools_watch_recursively(path, IN_ALL_EVENTS)) {
                    printf("inotifytools_watch_recursively failed: %d\n", inotifytools_error());
					return -1;
                }
            } else if (ACTION_DEL_WD == action) {
                if (!inotifytools_remove_watch_by_wd(event->wd)) {
                    printf("inotifytools_remove_watch_by_wd failed: event->wd[%d], %d\n", 
						  event->wd, inotifytools_error());
					return -1;
                }
            }
        }
        event = inotifytools_next_event(-1);
    }
 
    return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//creat a file by open flag.
	case 1:
		creat_file_by_flag("/tmp/file.out");
		break;

		//creat a tmpfile by mkstemp.
	case 2:
		creat_file_by_mkstemp();
		break;

		//copy a char one time use read and write from file.
	case 3:
		open_file_by_char("/tmp/file.in", "/tmp/file.out");
		break;

		//copy a block string one time use read and write from file.
	case 4:
		open_file_by_block("/tmp/file.in", "/tmp/file.out");
		break;

		//modify a block struct use open mmap from file.
	case 5:
		open_file_by_mmap("/tmp/records.dat");
		break;

		//copy a block string one time use read and write from term.
	case 6:
		open_term_by_block();
		break;

		//get the length of file by lseek.
	case 7:
		open_file_by_lseek("/tmp/file.in");
		break;

		//copy a char one time use fget and fput from file.
	case 8:
		fopen_file_by_char("/tmp/file.in", "/tmp/file.out");
		break;

		//copy a format string one time use fprintf from file.
	case 9:
		fopen_file_by_format("/tmp/file.in", "/tmp/file.out");
		break;

		//modify a block struct use fread and fwrite from file.
	case 10:
		fopen_file_by_struct("/tmp/records.dat");
		break;

		//get the length of file by fseek.
	case 11:
		fopen_file_by_fseek("/tmp/file.in");
		break;

		//get the attribute of file by ioctl api.
	case 12:
		handle_file_by_ioctl("eth1");
		break;

		//get the attribute of file by stat api.
	case 13:
		stat_file_by_stat("/tmp/file.in");
		break;

		//read a dir use readdir. 
	case 14:
		open_dir_by_readdir(".", 0);
		break;

		//select a dir use scandir filter.
	case 15:
		open_dir_by_filter(".");
		break;

		//find a dir by readlink.
	case 16:
		get_exe_path();
		break;

		//notify a dir or file event.
	case 17:
		notify_file_events();

		//default do nothing
	default:
		break;
	}

	return 0;
}
