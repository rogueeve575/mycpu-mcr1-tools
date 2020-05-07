
#ifndef _TCPSTUFF_H
#define _TCPSTUFF_H

#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

#define LOCALHOST		0x7f000001

int senddata(uint32_t sock, const uint8_t *packet, int len);
int sendstr(uint32_t sock, const char *str);
int sendnow(uint32_t sock, const uint8_t *packet, int len);
void net_flush(uint32_t sock);

int chkread(uint32_t sock);
int chkwrite(uint32_t sock);

bool net_setnonblock(uint32_t sock, bool enable);
bool net_nodelay(uint32_t sock, int flag);
bool net_cork(uint32_t sock, int flag);

char *decimalip(uint32_t ip);
uint32_t net_dnslookup(const char *host);

int net_connect(uint32_t ip, uint16_t port, int timeout_ms);
int net_connect_interruptible(uint32_t ip, uint16_t port, int timeout_ms, \
					int interrupt_fd=-1, bool *interrupt_var=NULL, \
					bool interrupt_var_condition=true);

uint32_t net_open_and_listen(uint16_t port, bool reuse_addr = true);
uint32_t net_open_and_listen(uint32_t ip, uint16_t port, bool reuse_addr = true);
char net_listen_socket(uint32_t sock, uint32_t listen_ip, uint16_t listen_port);
uint32_t net_accept(uint32_t s, uint32_t *connected_ip);

#endif
