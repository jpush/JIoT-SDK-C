#ifndef JIOT_TCP_NET_H
#define JIOT_TCP_NET_H

#include "jiot_network_define.h"


int  jiot_tcp_read(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms);
int  jiot_tcp_read_buffer(Network *pNetwork, unsigned char *buffer, int maxlen, int timeout_ms);
int  jiot_tcp_write(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms);
int  jiot_tcp_connect(Network *pNetwork);
void jiot_tcp_disconnect(Network *pNetwork);

#endif
