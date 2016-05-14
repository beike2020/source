#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <linux/types.h>
#include <asm/io.h> 
#ifdef __GLIBC__
#include <sys/perm.h>
#endif

#define __LIBRARY__ /* _syscall3 and friends are only available through this */
#include <linux/unistd.h>

#define PORT_FILE "/dev/port"
#define PAGE_SIZE 4096

int  gotdata = 0;
char buffer[4096];
char *prgname;

//_syscall3(int syslog, char *bufp, int len);

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf("1 : test the data size on kernel and application.\n");
	printf("2 : test the fasync signal handle method.\n");	
	printf("3 : test the input and output method by poll.\n");
	printf("4 : test the concole which recv the kernel message.\n");
	printf("5 : test the page difference by compared dev map addr.\n");
	printf("6 : test the input and ouput on all dev ports.\n");
	printf("Please input test type: ");
	scanf("%d",&choices);

	return choices;
}

/*
 * Show alignment needs and  print the size of common data items.
 */
int data_size(int argc, char **argv)
{
    struct utsname name;
	struct c   {char c;  char	   t;} c;
	struct s   {char c;  short	   t;} s;
	struct i   {char c;  int	   t;} i;
	struct l   {char c;  long	   t;} l;
	struct ll  {char c;  long long t;} ll;
	struct p   {char c;  void *    t;} p;
	struct u1b {char c;  __u8	   t;} u1b;
	struct u2b {char c;  __u16	   t;} u2b;
	struct u4b {char c;  __u32	   t;} u4b;
	struct u8b {char c;  __u64	   t;} u8b;

    uname(&name); 
    printf("arch  Align:  char  short  int  long   ptr long-long  u8 u16 u32 u64\n");
    printf("%-12s  %3i   %3i   %3i   %3i   %3i   %3i      %3i %3i %3i %3i\n", name.machine,
	   (int)((void *)(&c.t)   - (void *)&c),
	   (int)((void *)(&s.t)   - (void *)&s),
	   (int)((void *)(&i.t)   - (void *)&i),
	   (int)((void *)(&l.t)   - (void *)&l),
	   (int)((void *)(&p.t)   - (void *)&p),
	   (int)((void *)(&ll.t)  - (void *)&ll),
	   (int)((void *)(&u1b.t) - (void *)&u1b),
	   (int)((void *)(&u2b.t) - (void *)&u2b),
	   (int)((void *)(&u4b.t) - (void *)&u4b),
	   (int)((void *)(&u8b.t) - (void *)&u8b));
	
    printf("arch   Size:  char  short  int  long   ptr long-long  u8 u16 u32 u64\n");
    printf("%-12s  %3i   %3i   %3i   %3i   %3i   %3i      %3i %3i %3i %3i\n", name.machine,
	   (int)sizeof(char), (int)sizeof(short), (int)sizeof(int),
	   (int)sizeof(long),
	   (int)sizeof(void *), (int)sizeof(long long), (int)sizeof(__u8),
	   (int)sizeof(__u16), (int)sizeof(__u32), (int)sizeof(__u64));
	
    return 0;
}

void fasync_sighandler(int signo)
{
    if (signo == SIGIO)
        gotdata++;
	
    return;
}

/*
 * Use async notification to read stdin.
 */
int fasync_handle(int argc, char **argv)
{
    int count;
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = fasync_sighandler;
    action.sa_flags = 0;
    sigaction(SIGIO, &action, NULL);

    fcntl(STDIN_FILENO, F_SETOWN, getpid());
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | FASYNC);

    while(1) {
        sleep(3600); 
        if (!gotdata)
            continue;
		
        count = read(0, buffer, 4096);
        write(1,buffer,count);
        gotdata=0;
    }

	return 0;
}

/*
 * Test out reading with poll(). This should run with any Unix
 */
int io_poll(int argc, char **argv)
{
    struct pollfd pfd;
    int n;

    fcntl(0, F_SETFL, fcntl(0,F_GETFL) | O_NONBLOCK); 
    pfd.fd = 0;  
    pfd.events = POLLIN;

    while (1) {
	    n = read(0, buffer, 4096);
	    if (n >= 0)
	        write(1, buffer, n);
		
		n = poll(&pfd, 1, -1);
		if (n < 0)
	    	break;
	}
	
	perror( n < 0 ? "stdin" : "stdout");
	
	return 0;
}

/*
 * Choose a console to receive kernel messages and choose a console_log level for the kernel.
 */
int concole_ctl(int argc, char **argv)
{
	// 11 is the TIOCLINUX cmd number, bytes[1] is the tty-id which recv the kernel message.
    char bytes[2] = {11,10}; 
	int  level = 1;

    if ( argc == 2 ) {
		bytes[1] = atoi(argv[1]); 
    }
	
    if (ioctl(STDIN_FILENO, TIOCLINUX, bytes) < 0) {
        fprintf(stderr,"%s: ioctl(stdin, TIOCLINUX): %s\n",argv[0], strerror(errno));
        return -1;
    }

    if (syslog(8, NULL, level) < 0) {  
        fprintf(stderr,"%s: syslog(setlevel): %s\n", argv[0],strerror(errno));
        return -1;
    }
		
    return 0;
}

char *dev_map (const char *dev, unsigned long offset, unsigned long size)
{
	int fd;
	char *addr;

	fd = open (dev, O_RDONLY);
	if (fd < 0){
		perror (dev);
		return NULL;
	}
	
	addr = mmap (0, size, PROT_READ, MAP_PRIVATE, fd, offset);
	if (addr == MAP_FAILED){
		perror (dev);
		return NULL;
	}
	
	printf ("Mapped %s (%lu @ %lx) at %p\n", dev, size, offset, addr);
	
	return addr;
}

/*
 * Simple program to compare two mmap'd areas.
 */
int mem_cmp (int argc, char **argv)
{
	unsigned long offset, size, i;
	char *addr1, *addr2;

	offset = 0;
	size = PAGE_SIZE;	
	addr1 = dev_map ("/dev/scull0", offset, size);
	addr2 = dev_map ("/dev/scull1", offset, size);

	printf ("Comparing...");
	fflush (stdout);
	
	for (i = 0; i < size; i++){
		if (*addr1++ != *addr2++){
			printf ("areas differ at byte %ld\n", i);
			return -1;
		}
	}
	
	printf ("areas are identical.\n");
	
	return 0;
}

static int read_one(unsigned int port, int size)
{
    static int iopldone = 0;

	if (port > 1024) {
		if (!iopldone && iopl(3)) {
		    fprintf(stderr, "%s: iopl(): %s\n", prgname, strerror(errno));
		    return 1;
		}
		iopldone++;
	} else if (ioperm(port,size,1)) {
		fprintf(stderr, "%s: ioperm(%x): %s\n", prgname, port, strerror(errno));
		return 1;
	}

    if (size == 4)
		printf("%04x: %08x\n", port, inl(port));
    else if (size == 2)
		printf("%04x: %04x\n", port, inw(port));
    else if (size == 1)
		printf("%04x: %02x\n", port, inb(port));
	
    return 0;
}

static int write_one(unsigned int port, unsigned int val, int size)
{
    static int iopldone = 0;

	if (port > 1024) {
		if (!iopldone && iopl(3)) {
		    fprintf(stderr, "%s: iopl(): %s\n", prgname, strerror(errno));
		    return 1;
		}
		iopldone++;
	} else if (ioperm(port,size,1)) {
		fprintf(stderr, "%s: ioperm(%x): %s\n", prgname,port, strerror(errno));
		return 1;
    }

    if (size == 4)
		outl(val, port);
    else if (size == 2)
		outw(val&0xffff, port);
    else if (size == 1)
		outb(val&0xff, port);
	
    return 0;
}

/* 
 * Read  and write all the ports specified in hex on the command line. 
 * The program uses the faster ioperm[set port IO permissions]/iopl[change IO privilege level] calls on x86. 
 * The program acts as inb/inw/inl according  to its own name.
 */
int port_io(int argc, char **argv)
{
    unsigned int i, n, port, size, error = 0;
	unsigned int val, opt;
    
    prgname = argv[0];
    switch (prgname[strlen(prgname)-1]) {
        case 'w': 
			size = 2; 
			break;
			
        case 'l': 
			size = 4; 
			break;
			
        case 'b': 
			
		case 'p': 
			
		default:
	    	size = 1;
    }
	
    setuid(0); 

	printf("Please choose the option: 1 - read, 2 - write: ");
	scanf("%d",&opt);

	if (opt == 1) {
	    for (i = 1; i < argc; i++) {
	        if ( sscanf(argv[i], "%x%n", &port, &n) < 1 || n != strlen(argv[i]) ) {
		    	fprintf(stderr, "%s: argument \"%s\" is not a hex number\n", argv[0], argv[i]);
				error++; 
				continue;
			}
			
			if (port & (size-1)) {
			    fprintf(stderr, "%s: argument \"%s\" is not properly aligned\n", argv[0], argv[i]);
			    error++; 
				continue;
			}
			
			error += read_one(port, size);
		}
	} else if (opt == 2) {
		for (i = 1; i < argc-1; i++) {
	        if ( sscanf(argv[i], "%x%n", &port, &n) < 1 || n != strlen(argv[i]) ) {
		    	fprintf(stderr, "%s: argument \"%s\" is not a hex number\n", argv[0], argv[i]);
		    	error++; 
				continue;
			}
			
			if (port & (size-1)) {
		    	fprintf(stderr, "%s: argument \"%s\" is not properly aligned\n", argv[0], argv[i]);
		    	error++; 
				continue;
			}
			
	        if ( sscanf(argv[i+1], "%x%n", &val, &n) < 1 || n != strlen(argv[i+1]) ) {
		    	fprintf(stderr, "%s: argument \"%s\" is not a hex number\n", argv[0], argv[i+1]);
		    	error++; 
				continue;
			}
			
			if (size < 4 && val > (size == 1 ? 0xff : 0xffff)) {
			    fprintf(stderr, "%s: argument \"%s\" out of range\n", argv[0], argv[i+1]);
			    error++; 
				continue;
			}
			
			error += write_one(port, val, size);
	    }
	}
	
    return (error ? 1 : 0);
}

int main(int argc, char *argv[])
{			
	int choice = 1;

	choice = check_model();

	switch(choice) {
		//test the data size on kernel and application.
		case 1:
		{
			data_size(argc, argv);
			break;
		}

		//test the fasync signal handle method.
		case 2:
		{
			fasync_handle(argc, argv);
			break;
		}

		//test the input and output method by poll.
		case 3:
		{
			io_poll(argc, argv);
			break;
		}

		//test the concole which recv the kernel message.
		case 4:
		{
			concole_ctl(argc, argv);
			break;
		}

		//test the page difference by compared dev map addr.
		case 5:
		{
			mem_cmp(argc, argv);
			break;
		}

		//test the input and ouput on all dev ports.
		case 6:
		{
			port_io(argc, argv);
			break;
		}

		//default do nothing
		default:
		{
			break;
		}
	}
	
	return 0;
}


