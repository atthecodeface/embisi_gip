void ether_byte (unsigned char b);
void ether_send (void);

//void ether_send_packet (unsigned char * pkt, unsigned int len);

unsigned char ether_rx (void);
unsigned char * ether_rx_packet (void);
unsigned int ether_size (void);

void ether_init (const char * netdev);
int ether_poll (void);

