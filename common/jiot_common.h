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
#ifndef JIOT_COMMON_H_
#define JIOT_COMMON_H_

#include "jiot_timer.h"
#include "jiot_network_define.h"


typedef struct  Timer
{
    S_JIOT_TIME_TYPE time;
}Timer;

int  jiot_mqtt_network_init(Network *pNetwork, char *addr, char *port,char *ca_crt);
int  jiot_http_network_init(Network *pNetwork, char *addr, char *port,char *ca_crt);

void jiot_set_network_param(Network *pNetwork, char *addr, char *port, char *ca_crt);

char expired(Timer* timer);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);

long long left_ms(Timer* timer);
long long spend_ms(Timer* timer);

void InitTimer(Timer* timer);
void NowTimer(Timer* timer);


#endif