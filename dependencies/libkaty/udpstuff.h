
#ifndef _UDPSTUFF_H
#define _UDPSTUFF_H

int udp_createsocket(int port = 0);
int udp_createsocket_anyport(uint16_t *port_out = NULL);

void udp_send(int sock, const uint8_t *data, int len, uint32_t ip, uint16_t port);
int udp_recv(int sock, uint8_t *buffer, int buffer_size, \
			uint32_t *ip_out, uint16_t *port_out);

#endif
