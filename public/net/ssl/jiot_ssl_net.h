#ifndef JIOT_MQTT_NETTYPE_H
#define JIOT_MQTT_NETTYPE_H

#include "jiot_network_define.h"

#ifdef JIOT_SSL

int  jiot_ssl_read(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms);
int  jiot_ssl_read_buffer(Network *pNetwork, unsigned char *buffer, int maxlen, int timeout_ms);
int  jiot_ssl_write(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms);
int  jiot_ssl_connect(Network *pNetwork);
void jiot_ssl_disconnect(Network *pNetwork);

#endif

#endif
