#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h> 
#include <linux/ioport.h>
#include <linux/errno.h>
#include <asm/system.h> 
#include <linux/mm.h> 
#include <asm/io.h> 

#define ISA_REGION_BEGIN 	0xA0000
#define ISA_REGION_END   	0x100000
#define STEP 				2048
#define SKULL_PORT_FLOOR 	0x280
#define SKULL_PORT_CEIL  	0x300
#define SKULL_PORT_RANGE  	0x010

static int skull_port_base = 0; 
module_param(skull_port_base, int, 0);

static int skull_detect(unsigned int port, unsigned int range)
{
    int err;

    if ((err = check_region(port, range)) < 0) 
		return err; 
		
    request_region(port, range, "skull");   
	
    return 0;
}

int skull_init_board(unsigned int port)
{
	printk("Init port %u sucess\n", port);
	
	return 0; 
}

int skull_init(void)
{
    unsigned char oldval, newval, val; 
    unsigned long flags;         
    unsigned long add, i, radd;
    void *base;
	int baseaddr; 
    int result = 0;
	int size;
    
    base = ioremap(ISA_REGION_BEGIN, ISA_REGION_END - ISA_REGION_BEGIN);
    base -= ISA_REGION_BEGIN; 
    
	for (add = ISA_REGION_BEGIN; add < ISA_REGION_END; add += STEP) {
		if (check_mem_region (add, 2048)) {
			printk(KERN_INFO "%lx: Allocated\n", add);
			continue;
		}

		save_flags(flags); 
		cli();
		oldval = readb (base + add); 
		writeb (oldval^0xff, base + add);
		mb();
		
		newval = readb (base + add);
		writeb (oldval, base + add);
		restore_flags(flags);

		if ((oldval^newval) == 0xff) {  
		    printk(KERN_INFO "%lx: RAM\n", add);
		    continue;
		}
		
		if ((oldval^newval) != 0) {  
		    printk(KERN_INFO "%lx: empty\n", add);
		    continue;
		}
		
		if ( (oldval == 0x55) && (readb (base + add + 1) == 0xaa)) {
		    size = 512 * readb (base + add + 2);
		    printk(KERN_INFO "%lx: Expansion ROM, %i bytes\n", add, size);
		    add += (size & ~2048) - 2048; 
		    continue;
		}
		
		printk(KERN_INFO "%lx: ", add);
		for (i=0; i<5; i++) {
		    radd = add + 57*(i+1);  
		    val = readb (base + radd);
		    if (val && val != 0xFF && val != ((unsigned long)radd&0xFF))
				break;
		}   
		
		printk("%s\n", i==5 ? "empty" : "ROM");
	}
	
    baseaddr = skull_port_base ? skull_port_base : SKULL_PORT_FLOOR; 
	
	do {
		if (skull_detect(baseaddr, SKULL_PORT_RANGE) == 0) 
		    skull_init_board(baseaddr);
		
		baseaddr += SKULL_PORT_RANGE; 
	}while (skull_port_base == 0 && baseaddr < SKULL_PORT_CEIL);

    return 0;
}

void skull_cleanup(void)
{
	 release_region(0,0);
}

module_init(skull_init);
module_exit(skull_cleanup);
