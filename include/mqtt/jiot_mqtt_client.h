/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
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
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - documentation and platform specific header
 *    Ian Craggs - add setMessageHandler function
 *******************************************************************************/

/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#if !defined(JIOT_MQTT_CLIENT_H)
#define JIOT_MQTT_CLIENT_H

#if defined(__cplusplus)
 extern "C" {
#endif

#include "jiot_mqtt_define.h"

int jiot_mqtt_init(JMQTTClient * pClient,
                    void* pContext,
                    SubscribeHander * pSubscribe,
                    ConnectedHander * pConnected,
                    ConnectFailHander * pConnectFail,
                    DisconnectHander  * pDisConnect,
                    SubscribeFailHander * pSubscribeFail,
                    PublishFailHander   * pPublishFail,
                    MessageTimeoutHander * pMessageTimeout) ;

int jiot_mqtt_conn_init(JMQTTClient * pClient, IOT_CLIENT_INIT_PARAMS *pInitParams, char* szHostAddress,char* szPort, char* szPubkey) ;

int jiot_mqtt_release(JMQTTClient * pClient);

int jiot_mqtt_connect(JMQTTClient * pClient, char* szClientID, char* szUserName, char* szPassWord);

int jiot_mqtt_disconnect(JMQTTClient * pClient);

int jiot_mqtt_closed(JMQTTClient  *pClient);

int jiot_mqtt_subscribe(JMQTTClient * c, const char* topicFilter, int qos, messageHandler handler,bool flag);

int jiot_mqtt_unsubscribe(JMQTTClient * c, const char* topicFilter);

int jiot_mqtt_publish(JMQTTClient * c, const char* topicName, MQTTMessage* message,long long seqNo,E_JIOT_MSG_TYPE type);

JMQTTClientStatus jiot_mqtt_get_client_state(JMQTTClient  *pClient);

void jiot_mqtt_set_client_state(JMQTTClient *pClient, JMQTTClientStatus newState);

void jiot_mqtt_keepalive(JMQTTClient * pClient);

void jiot_conn_Lock(JMQTTClient *c);

void jiot_conn_Unlock(JMQTTClient *c);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
