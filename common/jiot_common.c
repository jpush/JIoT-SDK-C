/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "jiot_common.h"

#ifndef JIOT_SSL
#include "jiot_tcp_net.h"
#else
#include "jiot_ssl_net.h"
#endif

char expired(Timer* timer)
{
	return (char)jiot_timer_expired(&timer->time);
}

void countdown_ms(Timer* timer, unsigned int timeout)
{
	jiot_timer_countdown(&timer->time,timeout);
}

void countdown(Timer* timer, unsigned int timeout)
{
	jiot_timer_countdown(&timer->time,(timeout*1000));
}

long long left_ms(Timer* timer)
{
	return jiot_timer_remain(&timer->time);
}

long long spend_ms(Timer* timer)
{
    return jiot_timer_spend(&timer->time);
}

void InitTimer(Timer* timer)
{
    jiot_timer_init(&timer->time);
}

void NowTimer(Timer* timer)
{
    jiot_timer_now_clock(&timer->time);
}

int  jiot_mqtt_network_init(Network *pNetwork, char *addr, char *port,char *ca_crt)
{
    jiot_set_network_param(pNetwork,addr,port,ca_crt);
#ifndef JIOT_SSL
    pNetwork->_socket = -1;
    pNetwork->_read = jiot_tcp_read;
    pNetwork->_write= jiot_tcp_write;
    pNetwork->_disconnect = jiot_tcp_disconnect;
    pNetwork->_connect = jiot_tcp_connect;

#else
    pNetwork->_socket = -1;
    pNetwork->_read = jiot_ssl_read;
    pNetwork->_write= jiot_ssl_write;
    pNetwork->_disconnect = jiot_ssl_disconnect;
    pNetwork->_connect = jiot_ssl_connect;
    pNetwork->tlsdataparams.socketId  = -1;
#endif
    return 0;
}

int  jiot_http_network_init(Network *pNetwork, char *addr, char *port,char *ca_crt)
{
    jiot_set_network_param(pNetwork,addr,port,ca_crt);
#ifndef JIOT_SSL
    pNetwork->_socket = -1;
    pNetwork->_read = jiot_tcp_read_buffer;
    pNetwork->_write= jiot_tcp_write;
    pNetwork->_disconnect = jiot_tcp_disconnect;
    pNetwork->_connect = jiot_tcp_connect;

#else
    pNetwork->_socket = -1;
    pNetwork->_read = jiot_ssl_read_buffer;
    pNetwork->_write= jiot_ssl_write;
    pNetwork->_disconnect = jiot_ssl_disconnect;
    pNetwork->_connect = jiot_ssl_connect;
    pNetwork->tlsdataparams.socketId  = -1;
#endif
    return 0;
}

void jiot_set_network_param(Network *pNetwork, char *addr, char *port, char *ca_crt)
{

#ifndef JIOT_SSL
    pNetwork->connectparams.pHostAddress = addr;
    pNetwork->connectparams.pHostPort = port;
#else
    pNetwork->connectparams.pHostAddress = addr;
    pNetwork->connectparams.pHostPort = port;
    pNetwork->connectparams.pPubKey = ca_crt;
#endif
    return ;
}


