/**********************************************************************
 * Function: 	Socket pcap information.
 * Author: 	forwarding2012@yahoo.com.cn								
 * Date: 		2012.01.01	
 * Compile:	gcc -Wall socket_pcap.c -lpcap -o socket_pcap
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pcap.h>
#include <netdb.h>
#include <resolv.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#define  CAP_NUM 	10
#define  CAP_LEN 	2048
#define  BUFSIZE 	10240
#define  STRSIZE 	1024
#define  LINE_LEN   16
#define  FILENAME 	"/tmp/test.pcap"
#define  OUTFILE    "/tmp/output.txt"

struct time_val {
	long tv_sec;
	long tv_usec;
};

typedef struct FramHeader_t {
	unsigned char DstMAC[6];
	unsigned char SrcMAC[6];
	unsigned short FrameType;
} FramHeader_t;

typedef struct IPHeader_t {
	unsigned char Ver_HLen;
	unsigned char TOS;
	unsigned short TotalLen;
	unsigned short ID;
	unsigned short Flag_Segment;
	unsigned char TTL;
	unsigned char Protocol;
	unsigned short Checksum;
	unsigned long SrcIP;
	unsigned long DstIP;
} IPHeader_t;

typedef struct TCPHeader_t {
	unsigned short SrcPort;
	unsigned short DstPort;
	unsigned long SeqNO;
	unsigned long AckNO;
	unsigned char HeaderLen;
	unsigned char Flags;
	unsigned short Window;
	unsigned short Checksum;
	unsigned short UrgentPointer;
} TCPHeader_t;

struct arp_header {
	unsigned short int arp_hardware_type;
	unsigned short int arp_protocol_type;
	unsigned char arp_hardware_length;
	unsigned char arp_protocol_length;
	unsigned short int arp_operation_code;
	unsigned char arp_source_ethernet_address[6];
	unsigned char arp_source_ip_address[4];
	unsigned char arp_destination_ethernet_address[6];
	unsigned char arp_destination_ip_address[4];
};

static int check_model()
{
	int choices;

	printf("Test program as follow: \n");
	printf(" 1: catch a package\n");
	printf(" 2: parse net package\n");
	printf(" 3: save net package\n");
	printf(" 4: parse a pcap file\n");
	printf("Please input test type: ");
	scanf("%d", &choices);

	return choices;
}

int pcap_single_handle()
{
	int i, ret;
	u_char *ptr;
	pcap_t *descr;
	char *dev, *net, *mask;
	bpf_u_int32 netp, maskp;
	char errbuf[PCAP_ERRBUF_SIZE];
	const u_char *packet;
	struct in_addr addr;
	struct pcap_pkthdr hdr;
	struct ether_header *eptr;

	if ((dev = pcap_lookupdev(errbuf)) == NULL) {
		printf("%s\n", errbuf);
		exit(EXIT_FAILURE);
	}
	printf("\nDev: %s", dev);

	if ((ret = pcap_lookupnet(dev, &netp, &maskp, errbuf)) == -1) {
		printf("%s\n", errbuf);
		exit(EXIT_FAILURE);
	}

	addr.s_addr = netp;
	if ((net = inet_ntoa(addr)) == NULL) {
		perror("inet_ntoa");
		exit(EXIT_FAILURE);
	}
	printf("[%s: ", net);

	addr.s_addr = maskp;
	if ((mask = inet_ntoa(addr)) == NULL) {
		perror("inet_ntoa");
		exit(EXIT_FAILURE);
	}
	printf("%s]\n", mask);

	if ((descr = pcap_open_live(dev, BUFSIZ, 0, -1, errbuf)) == NULL) {
		perror(errbuf);
		exit(EXIT_FAILURE);
	}

	if ((packet = pcap_next(descr, &hdr)) == NULL) {
		perror("can't grab packet\n");
		exit(EXIT_FAILURE);
	}

	printf("\tgrabbed packet of length %d\n", hdr.len);
	printf("\trecieved at ..... %s", ctime((const time_t *)&hdr.ts.tv_sec));
	printf("\tethernet address length is %d\n", hdr.caplen);

	eptr = (struct ether_header *)packet;
	if (ntohs(eptr->ether_type) == ETHERTYPE_IP) {
		printf("\tethernet type hex: %x dec: %d - an IP packet\n",
		       ntohs(eptr->ether_type), ntohs(eptr->ether_type));
	} else if (ntohs(eptr->ether_type) == ETHERTYPE_ARP) {
		printf("\tethernet type hex: %x dec: %d - an ARP packet\n",
		       ntohs(eptr->ether_type), ntohs(eptr->ether_type));
	} else {
		printf("\tethernet type %x not IP", ntohs(eptr->ether_type));
		exit(EXIT_FAILURE);
	}

	ptr = eptr->ether_dhost;
	i = ETHER_ADDR_LEN;
	printf("\testination address: ");
	do {
		printf("%s%x", (i == ETHER_ADDR_LEN) ? " " : ":", *ptr++);
	} while (--i > 0);
	printf("\n");

	ptr = eptr->ether_shost;
	i = ETHER_ADDR_LEN;
	printf("\tsource address: ");
	do {
		printf("%s%x", (i == ETHER_ADDR_LEN) ? " " : ":", *ptr++);
	} while (--i > 0);
	printf("\n");

	return 0;
}

void ip_protocol_parse(u_char * userless, const struct pcap_pkthdr *pkthdr,
		       const u_char * packet)
{
	int i;
	u_char *data;
	struct in_addr addr;
	struct iphdr *ipptr;
	struct tcphdr *tcpptr;

	if (packet == NULL) {
		perror("Can't grab packet");
		exit(EXIT_FAILURE);
	}

	printf("IP protocol(Network Layer) parse:\n");
	ipptr = (struct iphdr *)(packet + sizeof(struct ether_header));
	printf("\tVer: %u", ipptr->version);
	printf("\tIhl: %u", ipptr->ihl);
	printf("\tTos: %u", ipptr->tos);
	printf("\tTle: %d\n", ipptr->tot_len);
	printf("\tId: %d", ipptr->id);
	printf("\tFof: %d\n", ipptr->frag_off);
	printf("\tttl: %u", ipptr->ttl);
	printf("\tPro: %u", ipptr->protocol);
	printf("\tSum: %d\n", ipptr->check);
	addr.s_addr = ipptr->saddr;
	printf("\tSrc_IP: %s", inet_ntoa(addr));
	addr.s_addr = ipptr->daddr;
	printf("\tDst_IP: %s\n", inet_ntoa(addr));

	printf("TCP protocol(Transform Layer) parse:\n");
	tcpptr = (struct tcphdr *)(packet + sizeof(struct ether_header) +
				   sizeof(struct iphdr));
	printf("\tSrc_port: %d", tcpptr->source);
	printf("\tDst_port: %d\n", tcpptr->dest);
	printf("\tSeq: %u\n", tcpptr->seq);
	printf("\tAck: %d\n", tcpptr->ack);
	printf("\tOff: %u", tcpptr->doff);
	printf("\tRev: %u", tcpptr->res1);
	printf("\tFlg: %u:%u:%u:%u:%u:%u", tcpptr->urg, tcpptr->ack,
	       tcpptr->psh, tcpptr->rst, tcpptr->syn, tcpptr->fin);
	printf("\tWin: %d\n", tcpptr->window);
	printf("\tSum: %d", tcpptr->check);
	printf("\tUrp: %d\n", tcpptr->urg_ptr);

	data = (u_char *) (packet + sizeof(struct ether_header) +
			   sizeof(struct iphdr) + sizeof(struct tcphdr));
	printf("Content of packets: \n\t");
	while (*data++) {
		printf("%.2x ", *data);
		if ((i++ % LINE_LEN) == 0)
			printf("\n\t");
	}
	printf("\n\n");

}

void arp_protocol_parse(u_char * argument,
			const struct pcap_pkthdr *packet_header,
			const u_char * packet_content)
{
	struct arp_header *arp_protocol;
	struct in_addr source_ip_address;
	struct in_addr destination_ip_address;

	arp_protocol = (struct arp_header *)(packet_content + 14);
	switch (ntohs(arp_protocol->arp_operation_code)) {
	case 1:
		printf("ARP protocol(Network Layer) parse:\n");
		printf("\toperation code: %d - ARP Request Protocol\n",
		       ntohs(arp_protocol->arp_operation_code));
		break;

	case 2:
		printf("ARP protocol(Network Layer) parse:\n");
		printf("\toperation code: %d - ARP Reply Protocol\n",
		       ntohs(arp_protocol->arp_operation_code));
		break;

	case 3:
		printf("RARP protocol(Network Layer) parse:\n");
		printf("\toperation code: %d - RARP Request Protocol\n",
		       ntohs(arp_protocol->arp_operation_code));
		break;

	case 4:
		printf("RARP protocol(Network Layer) parse:\n");
		printf("\toperation code: %d - RARP Reply Protocol\n",
		       ntohs(arp_protocol->arp_operation_code));
		break;

	default:
		printf("\tInvaid operation code: %d\n",
		       ntohs(arp_protocol->arp_operation_code));
		break;
	}

	printf("\thardware type: %d\n", ntohs(arp_protocol->arp_hardware_type));
	printf("\tprotocol type: %d\n", ntohs(arp_protocol->arp_protocol_type));
	printf("\thardware length: %d\n", arp_protocol->arp_hardware_length);
	printf("\tprotocol length: %d\n ", arp_protocol->arp_protocol_length);
	printf("\tEthernet source mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       arp_protocol->arp_source_ethernet_address[0],
	       arp_protocol->arp_source_ethernet_address[1],
	       arp_protocol->arp_source_ethernet_address[2],
	       arp_protocol->arp_source_ethernet_address[3],
	       arp_protocol->arp_source_ethernet_address[4],
	       arp_protocol->arp_source_ethernet_address[5]);
	memcpy((void *)&source_ip_address,
	       (void *)&arp_protocol->arp_source_ip_address, sizeof(struct in_addr));
	printf("\tSource ip address: %s\n", inet_ntoa(source_ip_address));
	printf("\tEthernet destination mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       arp_protocol->arp_destination_ethernet_address[0],
	       arp_protocol->arp_destination_ethernet_address[1],
	       arp_protocol->arp_destination_ethernet_address[2],
	       arp_protocol->arp_destination_ethernet_address[3],
	       arp_protocol->arp_destination_ethernet_address[4],
	       arp_protocol->arp_destination_ethernet_address[5]);
	memcpy((void *)&destination_ip_address,
	       (void *)&arp_protocol->arp_destination_ip_address, sizeof(struct in_addr));
	printf("\tDestination ip address: %s\n", inet_ntoa(destination_ip_address));
}

void ethernet_protocol_parse(u_char * argument,
			     const struct pcap_pkthdr *packet_header,
			     const u_char * packet_content)
{
	u_char *mac_string;
	u_short ethernet_type;
	static int packet_number = 1;
	struct ether_header *ethernet_protocol;

	printf("\nCapture time: %s", ctime((const time_t *)&(packet_header->ts).tv_sec));
	printf("The %dth packet is captured. Packet length: %d\n",
	       packet_number, packet_header->len);

	ethernet_protocol = (struct ether_header *)packet_content;
	printf("Ethernet protocol(LINK Layer) parse:\n");
	mac_string = ethernet_protocol->ether_shost;
	printf("\tSrc_mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       *mac_string, *(mac_string + 1), *(mac_string + 2),
	       *(mac_string + 3), *(mac_string + 4), *(mac_string + 5));
	mac_string = ethernet_protocol->ether_dhost;
	printf("\tDst_mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       *mac_string, *(mac_string + 1), *(mac_string + 2),
	       *(mac_string + 3), *(mac_string + 4), *(mac_string + 5));
	ethernet_type = ntohs(ethernet_protocol->ether_type);
	printf("\tEthe_typ: %04x\n", ethernet_type);

	switch (ethernet_type) {
	case 0x0800:
		printf("\tIP packet is captured.\n");
		ip_protocol_parse(argument, packet_header, packet_content);
		break;

	case 0x0806:
		printf("\tARP packet is captured.\n");
		arp_protocol_parse(argument, packet_header, packet_content);
		break;

	case 0x8035:
		printf("\tRARP packet is captured.\n");
		arp_protocol_parse(argument, packet_header, packet_content);
		break;

	default:
		printf("\tUnknow type packet is captured.\n");
		break;
	}

	packet_number++;
}

int pcap_parse_handle()
{
	char *net_interface;
	pcap_t *pcap_handle;
	bpf_u_int32 net_mask, net_ip;
	struct bpf_program bpf_filter;
	struct in_addr net_ip_address, net_mask_address;
	char error_content[PCAP_ERRBUF_SIZE];
	char bpf_filter_string[] = "ip or arp or rarp";

	net_interface = pcap_lookupdev(error_content);
	printf("\nCapture a packet from net_interface: %s\n", net_interface);

	pcap_lookupnet(net_interface, &net_ip, &net_mask, error_content);
	net_ip_address.s_addr = net_ip;
	printf("Network ip address: %s\n", inet_ntoa(net_ip_address));
	net_mask_address.s_addr = net_mask;
	printf("Network mac address: %s\n", inet_ntoa(net_mask_address));

	pcap_handle = pcap_open_live(net_interface, BUFSIZ, 1, 0, error_content);
	pcap_compile(pcap_handle, &bpf_filter, bpf_filter_string, 0, net_ip);
	pcap_setfilter(pcap_handle, &bpf_filter);
	printf("Filter packet type: %s\n", bpf_filter_string);

	if (pcap_datalink(pcap_handle) != DLT_EN10MB) {
		perror("pcap_datalink");
		pcap_close(pcap_handle);
		exit(EXIT_FAILURE);
	}

	pcap_loop(pcap_handle, CAP_NUM, ethernet_protocol_parse, NULL);
	pcap_close(pcap_handle);

	return 0;
}

int pcap_save_handle()
{
	pcap_t *pd;
	pcap_dumper_t *p;
	struct bpf_program fcode;
	bpf_u_int32 localnet, netmask;
	char *device, ebuf[PCAP_ERRBUF_SIZE];
	int i, dev_flag = 1, cap_len = CAP_LEN;

	if ((device = pcap_lookupdev(ebuf)) == NULL) {
		perror(ebuf);
		exit(EXIT_FAILURE);
	}

	printf("device is %s, save file name is %s \n", device, FILENAME);
	pd = pcap_open_live(device, cap_len, dev_flag, 1000, ebuf);
	if (pd == NULL) {
		perror(ebuf);
		exit(EXIT_FAILURE);
	}

	if ((i = pcap_snapshot(pd)) > cap_len) {
		printf("snaplen raised from %d to %d \n", cap_len, i);
		cap_len = i;
	}

	if (pcap_lookupnet(device, &localnet, &netmask, ebuf) < 0) {
		localnet = 0;
		netmask = 0;
		perror(ebuf);
		exit(EXIT_FAILURE);
	}

	if (pcap_compile(pd, &fcode, "", 1, netmask) < 0) {
		perror("pcap_compile");
		exit(EXIT_FAILURE);
	}

	if (pcap_setfilter(pd, &fcode) < 0) {
		perror("pcap_setfilter");
		exit(EXIT_FAILURE);
	}

	if ((p = pcap_dump_open(pd, FILENAME)) == NULL) {
		perror("pcap_dump_open");
		exit(EXIT_FAILURE);
	}

	if (pcap_loop(pd, CAP_NUM, pcap_dump, (u_char *) p) < 0) {
		perror("pcap_loop");
		exit(EXIT_FAILURE);
	}

	printf("catch %d packets on %s have finished!\n", CAP_NUM, device);
	pcap_close(pd);

	return 0;
}

int pcap_file_handle()
{
	time_t timeval;
	int loop, status;
	const u_char *pktData;
	pcap_t *handle = NULL;
	char errBuff[PCAP_ERRBUF_SIZE];
	struct pcap_pkthdr *pktHeader = NULL;

	printf("pcap version %s\n", pcap_lib_version());
	printf("read file name %s\n", FILENAME);
	printf("begin time is %lu\n", time(0));

	if ((handle = pcap_open_offline(FILENAME, errBuff)) == NULL) {
		perror(errBuff);
		return (EXIT_FAILURE);
	}

	do {
		status = pcap_next_ex(handle, &pktHeader, &pktData);
		timeval = (unsigned int)pktHeader->ts.tv_sec;
		printf("+pkt length: %u, ", pktHeader->len);
		printf("cap length: %u, ", pktHeader->caplen);
		printf("cap_time: %s", ctime(&timeval));
		for (loop = 1; (loop < pktHeader->caplen + 1); loop++) {
			printf("%.2x ", pktData[loop - 1]);
			if ((loop % LINE_LEN) == 0)
				printf("\n");
		}
		printf("\n\n");
	} while (status == 1);

	pcap_close(handle);
	printf("end time is %lu\n", time(0));

	return 0;
}

int main(int argc, char *argv[])
{
	int choice;

	if (argc < 2)
		choice = check_model();

	switch (choice) {
		//catch a package.
	case 1:
		pcap_single_handle();
		break;

		//parse net package.
	case 2:
		pcap_parse_handle();
		break;

		//save net package.
	case 3:
		pcap_save_handle();
		break;

		//parse a pcap file.
	case 4:
		pcap_file_handle();
		break;

		//default do nothing
	default:
		break;
	}

	return 0;
}
