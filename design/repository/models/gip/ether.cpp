#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#ifdef LINUX
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "ether.h"
#endif

static int raw_s;

static unsigned char tx_packet [2048];
static int tx_packet_len;
static unsigned char mac [6];
static int if_index;

static void ether_send_packet (unsigned char * pkt, unsigned int len)
{
	memcpy (mac, pkt+6, 6); // save our mac address
#ifdef LINUX	
	struct sockaddr_ll addr;
	memset (&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = if_index;
	int n = sendto (raw_s, pkt, len, 0, (struct sockaddr *)&addr, sizeof(addr));
//	printf ("Sending packet len=%d, n=%d\n", len, n);
	if (n == -1) perror ("send");
#endif
}

extern void ether_byte (unsigned char b)
{
	tx_packet[tx_packet_len++] = b;
}

extern void ether_send (void)
{
	ether_send_packet (tx_packet, tx_packet_len);
	tx_packet_len = 0;
}

extern void ether_init (const char * netdev)
{
#ifdef LINUX
	raw_s = socket (AF_PACKET, SOCK_RAW, ntohs (ETH_P_ALL));

    if (raw_s<0)
    {
        fprintf(stderr, "Failed to open ethernet socket\n");
        return;
    }

	if_index = if_nametoindex(netdev);

	/*
	char textbuf [21024];
	struct ifconf ifc;
	ifc.ifc_len = sizeof(textbuf);
	ifc.ifc_buf = textbuf;
	if (ioctl (raw_s, SIOCGIFCONF, &ifc))
	{
		perror ("SIOCGIFCONF");
		return;
	}
	struct ifreq * ifr = ifc.ifc_req;
	int i;
	for (i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++)
	{
		printf ("%s\n", ifr[i].ifr_name);
	}
	*/

	struct packet_mreq mreq;
	memset (&mreq, 0, sizeof(mreq));
	mreq.mr_ifindex = if_index;
	mreq.mr_type = PACKET_MR_PROMISC;
	setsockopt (raw_s, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
#endif
 }


const char * pkttype [] = {"host", "broadcast", "multicast", "otherhost", "outgoing", "loopback", "fastroute"};

struct packet
{
	packet * next;
	unsigned int len;
	unsigned char data [1600];
};

static packet * rx_packets;
static packet * rx_packets_tail;
static packet * rx_current;
static unsigned int rx_current_pos;

unsigned int ether_size (void)
{
	if (!rx_packets) return 0;
	if (rx_current) free (rx_current);
	rx_current = rx_packets;
	rx_packets = rx_packets->next;
	rx_current_pos = 0;
	return rx_current->len;
}

unsigned char ether_rx (void)
{
	if (rx_current)
	{
		return rx_current->data[rx_current_pos++];
	}
	return 0;
}

unsigned char * ether_rx_packet (void)
{
	if (rx_current)
		return rx_current->data;
	return 0;
}

int ether_poll (void)
{
	int interrupt = 0;
	if (raw_s<0)
		return 0;
	for (;;)
	{
		struct timeval tv;
		tv.tv_usec = 0;
		tv.tv_sec = 0;
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(raw_s,&fd);
		int n = select (raw_s+1, &fd, 0,0, &tv);
		if (n == 0) break;
		
#ifdef LINUX
		unsigned char buf [2048];
		struct sockaddr_ll addr;
		unsigned int addr_len = sizeof(addr);
		int len = recvfrom (raw_s, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
		if (addr.sll_ifindex == if_index)
//		if ((buf[0] & 1) || !memcmp (buf, mac, 6))
		{
			packet * pack = (packet *) malloc (sizeof(packet));
			pack->len = len;
			pack->next = 0;
			memcpy (pack->data, buf, len);
			if (rx_packets == 0)
				rx_packets = pack;
			else
				rx_packets_tail->next = pack;
			rx_packets_tail = pack;
			interrupt = 1;

//			printf ("Eth got size %d\n", len);
			
//			printf ("Receive:");
//			int i;
//			for (i = 0; i < len; i++)
//				printf (" %2.2x", buf[i]);
//			printf ("\n");
		}
#endif
	}
	return interrupt;
}


