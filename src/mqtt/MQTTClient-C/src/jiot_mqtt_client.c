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
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
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


#include "jiot_mqtt_client.h"
#include "jiot_pthread.h"
#include "jiot_timer.h"
#include "jiot_std.h"
#include "jiot_code.h"
#include "jiot_list.h"
#include "jiot_log.h"
#include "jiot_common.h"


#define COMMAND_TIMEOUT_MS   20*1000 //20秒，从发送到收到ACK的时间。发送超时使用该值的一半
#define REMAIN_TMP_LEN       64     //读取数据的临时空间长度  

int sendPacket(JMQTTClient * c,unsigned char *buf,int length, Timer* timer);
int readPacket(JMQTTClient * c,Timer* timer,unsigned int * packet_type);

void mqtt_yield(JMQTTClient * pClient, int timeout_ms);
int  mqtt_handle_reconnect(JMQTTClient  *pClient);

int  mqtt_keep_alive(JMQTTClient  *pClient);

int  push_pubInfo_to(JMQTTClient * c,int len,unsigned short msgId,long long seqNo,E_JIOT_MSG_TYPE type );
int  push_subInfo_to(JMQTTClient * c,int len,unsigned short msgId,enum msgTypes type,MessageHandler *handler);

int  del_pubInfo_from(JMQTTClient * c,unsigned int  msgId);
int  del_subInfo_from(JMQTTClient * c,unsigned int  msgId,MessageHandler *handler);

int  release_pubInfo(JMQTTClient * c);
int  release_subInfo(JMQTTClient * c);

int  whether_messagehandler_same(MessageHandler *messageHandlers1,MessageHandler *messageHandler2);
bool whether_mqtt_client_state_normal(JMQTTClient *c);

int common_check_rule(char *iterm,E_TOPIC_TYPE type);
int common_check_topic(const char * topicName,E_TOPIC_TYPE type);

int jiot_create_thread(JMQTTClient * pClient);
int jiot_delete_thread(JMQTTClient * pClient);


void NewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage)
{
    md->topicName = aTopicName;
    md->message = aMessage;
}

int getNextPacketId(JMQTTClient  *c)
{
    unsigned int id = 0;
    
    jiot_mutex_lock(&c->idLock);
    c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
    id = c->next_packetid;
    jiot_mutex_unlock(&c->idLock);

    return id;
}

int MQTTKeepalive(JMQTTClient * pClient)
{
    ENTRY;
    /* there is no ping outstanding - send ping packet */
    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, pClient->command_timeout_ms/2);
    int len = 0;
    int rc = 0;

    jiot_mutex_lock(&pClient->writebufLock);

    len = MQTTSerialize_pingreq(pClient->buf, pClient->buf_size);
    if(len <= 0)
    {
        jiot_mutex_unlock(&pClient->writebufLock);
        ERROR_LOG("Serialize ping request is error");
        return JIOT_ERR_MQTT_PING_PACKET_ERROR;
    }

    rc = sendPacket(pClient, pClient->buf,len, &timer);
    if(JIOT_SUCCESS != rc)
    {
        jiot_mutex_unlock(&pClient->writebufLock);
        /*ping outstanding , then close socket  unsubcribe topic and handle callback function*/
        ERROR_LOG("ping outstanding is error,result = %d",rc);
        jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        pClient->errCode = rc;
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    jiot_mutex_unlock(&pClient->writebufLock);

    return JIOT_SUCCESS;
}

/***********************************************************
* 函数名称: MQTTConnect
* 描       述: mqtt连接打包发送接口
* 输入参数: JMQTTClient *pClient
*           MQTTPacket_connectData* pConnectParams
* 输出参数: VOID
* 返 回  值: 0：成功  非0：失败
* 说       明:
************************************************************/
int MQTTConnect(JMQTTClient *pClient)
{
    ENTRY;
    MQTTPacket_connectData* pConnectParams = &pClient->mqttConnData;
    Timer connectTimer;
    int len = 0;
    int rc = 0;

    jiot_mutex_lock(&pClient->writebufLock);

    if ((len = MQTTSerialize_connect(pClient->buf, pClient->buf_size, pConnectParams)) <= 0)
    {
        jiot_mutex_unlock(&pClient->writebufLock);
        ERROR_LOG("Serialize connect packet failed,len = %d",len);
        return JIOT_ERR_MQTT_CONNECT_PACKET_ERROR;
    }

    /* send the connect packet*/
    InitTimer(&connectTimer);
    countdown_ms(&connectTimer, pClient->command_timeout_ms);
    if ((rc = sendPacket(pClient, pClient->buf,len, &connectTimer)) != JIOT_SUCCESS)
    {
        jiot_mutex_unlock(&pClient->writebufLock);
        ERROR_LOG("send connect packet failed");
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }
    jiot_mutex_unlock(&pClient->writebufLock);

    return JIOT_SUCCESS;
}

/***********************************************************
* 函数名称: MQTTPublish
* 描       述: mqtt发布消息打包发送接口
* 输入参数: JMQTTClient *pClient
*           const char* topicName
*           MQTTMessage* message
* 输出参数: VOID
* 返 回  值: 0：成功  非0：失败
* 说       明:
************************************************************/
int MQTTPublish(JMQTTClient *c,const char* topicName, MQTTMessage* message,long long seqNo,E_JIOT_MSG_TYPE type )
{
    ENTRY;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);
    
    jiot_mutex_lock(&c->writebufLock);

    len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id, topic, (unsigned char*)message->payload, message->payloadlen);
   

    if (len <= 0)
    {
        jiot_mutex_unlock(&c->writebufLock);
        
        ERROR_LOG("MQTTSerialize_publish is error,rc = %d, buf_size = %ld, payloadlen = %ld",len,(long)c->buf_size,(long)message->payloadlen);

        if (MQTTPACKET_BUFFER_TOO_SHORT == len)
        {
        	return JIOT_ERR_MQTT_PACKET_BUFFER_TOO_SHORT;
        }
        return JIOT_ERR_MQTT_PUBLISH_PACKET_ERROR;
    }

    if(message->qos > QOS0)
    {
        //将pub信息保存到pubInfoList中
        if (JIOT_SUCCESS != push_pubInfo_to(c,len,message->id,seqNo,type))
        {
            ERROR_LOG("push publish into to pubInfolist failed!");
            jiot_mutex_unlock(&c->writebufLock);
            
            return JIOT_ERR_MQTT_PUSH_TO_LIST_ERROR;
        }
    }

    if (sendPacket(c, c->buf,len, &timer) != JIOT_SUCCESS) // send the subscribe packet
    {
        if(message->qos > QOS0)
        {
            del_pubInfo_from(c, message->id);
        }

        ERROR_LOG("sendPacket failed.");
        jiot_mutex_unlock(&c->writebufLock);
       
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    jiot_mutex_unlock(&c->writebufLock);
   
    return JIOT_SUCCESS;
}

int MQTTPuback(JMQTTClient *c,unsigned int msgId,enum msgTypes type)
{
    ENTRY;
    int rc = 0;
    int len = 0;
    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);

    jiot_mutex_lock(&c->writebufLock);

    if (type == PUBACK)
    {
        len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msgId);
    }
    else if (type == PUBREC)
    {
        len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msgId);
    }
    else if(type == PUBREL)
    {
        len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREL, 0, msgId);
    }
    else
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_PUBLISH_ACK_TYPE_ERROR;
    }

    if (len <= 0)
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_PUBLISH_ACK_PACKET_ERROR;
    }

    rc = sendPacket(c, c->buf,len, &timer);
    if (rc != JIOT_SUCCESS)
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    jiot_mutex_unlock(&c->writebufLock);
    return JIOT_SUCCESS;
}

int MQTTSubscribe(JMQTTClient  *c,const char* topicFilter, int qos,unsigned int msgId,messageHandler handler)
{
    ENTRY;
    int rc = 0;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);

    jiot_mutex_lock(&c->writebufLock);

    len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, (unsigned short)msgId, 1, &topic, (int*)&qos);
    if (len <= 0)
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_SUBSCRIBE_PACKET_ERROR;
    }

    MessageHandler messagehandler ;
    memset(messagehandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);

    strcpy(messagehandler.topicFilter,topicFilter);
    messagehandler.Qos = qos;
    messagehandler.fp = handler;

    if (JIOT_SUCCESS != push_subInfo_to(c,len,msgId,SUBSCRIBE,&messagehandler))
    {
        ERROR_LOG("push publish into to pubInfolist failed!");
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_PUSH_TO_LIST_ERROR;
    }

    if ((rc = sendPacket(c, c->buf,len, &timer)) != JIOT_SUCCESS) // send the subscribe packet
    {    
        MessageHandler messagehandler;
        messagehandler.fp = NULL;
        messagehandler.Qos = 0;
        memset(messagehandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);

        del_subInfo_from(c,msgId,&messagehandler);

        jiot_mutex_unlock(&c->writebufLock);
        ERROR_LOG("run sendPacket error!");
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    jiot_mutex_unlock(&c->writebufLock);
    
    return JIOT_SUCCESS;
}

int MQTTReSubscribe(JMQTTClient  *c,const char* topicFilter, enum QoS qos,unsigned int msgId,messageHandler handler)
{
    ENTRY;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);

    jiot_mutex_lock(&c->writebufLock);

    len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, (unsigned short)msgId, 1, &topic, (int*)&qos);
    if (len <= 0)
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_SUBSCRIBE_PACKET_ERROR;
    }

    MessageHandler messagehandler ;
    memset(messagehandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);

    strcpy(messagehandler.topicFilter,topicFilter);
    messagehandler.Qos = qos;
    messagehandler.fp = handler;

    sendPacket(c, c->buf,len, &timer) ;
    
    jiot_mutex_unlock(&c->writebufLock);
    return JIOT_SUCCESS;
}

int MQTTUnsubscribe(JMQTTClient  *c,const char* topicFilter, unsigned int msgId)
{
    ENTRY;
    int rc = 0;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);

    jiot_mutex_lock(&c->writebufLock);

    if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, (unsigned short)msgId, 1, &topic)) <= 0)
    {
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_UNSUBSCRIBE_PACKET_ERROR;
    }

    MessageHandler handler ;
    memset(handler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);
    handler.Qos = 0;
    handler.fp = NULL;
    
    if (JIOT_SUCCESS != push_subInfo_to(c,len,msgId,UNSUBSCRIBE,&handler))
    {
        ERROR_LOG("push publish into to pubInfolist failed!");
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_PUSH_TO_LIST_ERROR;
    }

    if ((rc = sendPacket(c, c->buf,len, &timer)) != JIOT_SUCCESS) // send the subscribe packet
    {
        MessageHandler messagehandler;
        messagehandler.fp = NULL;
        handler.Qos = 0;
        memset(messagehandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);

        del_subInfo_from(c,msgId,&messagehandler);
        jiot_mutex_unlock(&c->writebufLock);
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    jiot_mutex_unlock(&c->writebufLock);

    return JIOT_SUCCESS;
}


int MQTTDisconnect(JMQTTClient * c)
{
    ENTRY;
    int rc = JIOT_FAIL;
    Timer timer;     // we might wait for incomplete incoming publishes to complete

    if (!whether_mqtt_client_state_normal(c))
    {
        DEBUG_LOG("mqtt client is not connected .");
        return rc;
    }

    jiot_mutex_lock(&c->writebufLock);
    int len = MQTTSerialize_disconnect(c->buf, c->buf_size);
   
    InitTimer(&timer);
    //断开超时200毫秒,不使用command_timeout_ms
    countdown_ms(&timer, 200);
   
    if (len > 0)
    {
        rc = sendPacket(c, c->buf,len, &timer);            // send the disconnect packet
    }
   
    jiot_mutex_unlock(&c->writebufLock);

    return rc;
}

int sendPacket(JMQTTClient * c,unsigned char *buf,int length, Timer* timer)
{
    int rc = JIOT_FAIL;
    int sent = 0;
    
    while (sent < length && !expired(timer))
    {
        rc = c->ipstack->_write(c->ipstack, &buf[sent], length, left_ms(timer));
        if (rc < 0)  // there was an error writing the data
        {
            break;
        }
        sent += rc;
    }

    if (sent == length)
    {
        NowTimer(&c->commandTimer);
        rc = JIOT_SUCCESS;
    }
    else
    {
        rc = JIOT_ERR_MQTT_NETWORK_ERROR;
    }
    return rc;
}

int decodePacket(JMQTTClient * c, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = JIOT_ERR_MQTT_PACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            return JIOT_ERR_MQTT_PACKET_READ_ERROR; /* bad data */
        }

        rc = c->ipstack->_read(c->ipstack, &i, 1, timeout);
        if (rc != 1)
        {
            return JIOT_ERR_MQTT_NETWORK_ERROR;
        }

        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);

    return len;
}

int readPacket(JMQTTClient * c, Timer* timer,unsigned int *packet_type)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;
    int rc = 0;

    /* 1. read the header byte.  This has the packet type in it */
    while(c->isRun)
    {
        INT32 lefttime = left_ms(timer);
        if(lefttime <= 0)
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }

        rc = c->ipstack->_read(c->ipstack, c->recvbuf, 1, left_ms(timer));

        if( JIOT_ERR_NET_READ_TIMEOUT == rc )
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }
        else
        if ( rc != 1)//1是读取的长度
        {
        DEBUG_LOG("readPacket error %d",rc);
            return JIOT_FAIL;
        }
        else
        {
            break;
        }

    }

    if(c->isRun == false)
    {
        return JIOT_FAIL;
    }

    len = 1;

    /* 2. read the remaining length.  This is variable in itself */
    if((rc = decodePacket(c, &rem_len, left_ms(timer))) < 0)
    {
        ERROR_LOG("decodePacket error,rc = %d",rc);
        return rc;
    }

    len += MQTTPacket_encode(c->recvbuf + 1, rem_len); /* put the original remaining length back into the buffer */
    
    /*Check if the received data length exceeds mqtt read buffer length*/
	if((rem_len > 0) && ((rem_len + len) > c->recvbuf_size))
	{
		ERROR_LOG("mqtt read buffer is too short, mqttReadBufLen : %zd, remainDataLen : %d", c->recvbuf_size, rem_len);
		int needReadLen = c->recvbuf_size - len;
		if(c->ipstack->_read(c->ipstack, c->recvbuf + len, needReadLen, left_ms(timer)) != needReadLen)
		{
			ERROR_LOG("mqtt read error");
			return JIOT_FAIL;
		}

		/* drop data whitch over the length of mqtt buffer*/
        //超长部分，分批读取，并丢弃掉
		int remainDataLen = rem_len - needReadLen;
        int tmpBuffLen =  0;
		unsigned char remainTmpBuf[REMAIN_TMP_LEN];

        while(remainDataLen > 0)
        {
            if(remainDataLen > REMAIN_TMP_LEN )
            {
                tmpBuffLen = REMAIN_TMP_LEN;
            }
            else
            {
                tmpBuffLen = remainDataLen ;
            }

            if(c->ipstack->_read(c->ipstack, remainTmpBuf, tmpBuffLen, left_ms(timer)) != tmpBuffLen)
            {
                ERROR_LOG("mqtt read error");
                return JIOT_FAIL;
            }    
            remainDataLen = remainDataLen - tmpBuffLen;
        }		
		
	}
    else/* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (c->ipstack->_read(c->ipstack, c->recvbuf + len, rem_len, left_ms(timer)) != rem_len))
    {
        ERROR_LOG("mqtt read error");
        return JIOT_FAIL;
    }

    header.byte = c->recvbuf[0];
    *packet_type = header.bits.type;
    return JIOT_SUCCESS;
}


char isTopicMatched(char* topicFilter, MQTTString* topicName)
{
    ENTRY;
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;
    
    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
        {
            break;
        }

        if (*curf != '+' && *curf != '#' && *curf != *curn)
        {
            break;
        }

        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
            {
                nextpos = ++curn + 1;
            }
        }
        else if (*curf == '#')
        {
            curn = curn_end - 1;    // skip until end of string
        }
        curf++;
        curn++;
    }
    
    return (curn == curn_end) && (*curf == '\0');
}

int deliverMessage(JMQTTClient * c, MQTTString* topicName, MQTTMessage* message)
{
    ENTRY;
    int i;
    int rc = JIOT_FAIL;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (strlen(c->messageHandlers[i].topicFilter) != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) || isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(c->pContext,&md);
                rc = JIOT_SUCCESS;
            }
        }
    }
    
    if (rc == JIOT_FAIL && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(c->pContext,&md);
        rc = JIOT_SUCCESS;
    }   
    
    return rc;
}


int recvConnAckProc(JMQTTClient  *c)
{
    ENTRY;
    int rc = JIOT_SUCCESS;
    unsigned char connack_rc = 255;
    char sessionPresent = 0;
    if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, c->recvbuf, c->recvbuf_size) != 1)
    {
        ERROR_LOG("connect ack is error");
        return JIOT_ERR_MQTT_CONNECT_ACK_PACKET_ERROR;
    }

    switch(connack_rc)
    {
        case CONNECTION_ACCEPTED:
            rc = JIOT_SUCCESS;
            break;
        case CONNECTION_REFUSED_UNACCEPTABLE_PROTOCOL_VERSION:
            rc = JIOT_ERR_MQTT_CONANCK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR;
            break;
        case CONNECTION_REFUSED_IDENTIFIER_REJECTED:
            rc = JIOT_ERR_MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR;
            break;
        case CONNECTION_REFUSED_SERVER_UNAVAILABLE:
            rc = JIOT_ERR_MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR;
            break;
        case CONNECTION_REFUSED_BAD_USERDATA:
            rc = JIOT_ERR_MQTT_CONNACK_BAD_USERDATA_ERROR;
            break;
        case CONNECTION_REFUSED_NOT_AUTHORIZED:
            rc = JIOT_ERR_MQTT_CONNACK_NOT_AUTHORIZED_ERROR;
            break;
        default:
            rc = JIOT_ERR_MQTT_CONNACK_UNKNOWN_ERROR;
            break;
    }

    return rc;
}

int recvPubAckProc(JMQTTClient  *c)
{
    ENTRY;
    unsigned short mypacketid;
    unsigned char dup = 0;
    unsigned char type = 0;

    if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->recvbuf, c->recvbuf_size) != 1)
    {
        return JIOT_ERR_MQTT_PUBLISH_ACK_PACKET_ERROR;
    }

    del_pubInfo_from(c,mypacketid);

    return JIOT_SUCCESS;
}


int recvSubAckProc(JMQTTClient *c)
{
    ENTRY;
    int rc = JIOT_FAIL;
    unsigned short mypacketid;
    int count = 0, grantedQoS = -1;
    if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, c->recvbuf, c->recvbuf_size) != 1)
    {
        ERROR_LOG("Sub ack packet error");
        return JIOT_ERR_MQTT_SUBSCRIBE_ACK_PACKET_ERROR;
    }

    if (grantedQoS == 0x80)
    {
        ERROR_LOG("QOS of Sub ack packet error,grantedQoS = %d",grantedQoS);
        return JIOT_ERR_MQTT_SUBSCRIBE_QOS_ERROR;
    }

    MessageHandler messagehandler;
    messagehandler.fp = NULL;
    memset(messagehandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);


    del_subInfo_from(c,mypacketid,&messagehandler);

    if(messagehandler.fp == NULL || messagehandler.topicFilter == NULL)
    {
        ERROR_LOG("Sub Info not found, mypacketid:%d",mypacketid);
        return JIOT_ERR_MQTT_SUB_INFO_NOT_FOUND_ERROR;
    }

    // 订阅返回错误
    if (grantedQoS == 0x80)
    {
        if (c->subscribeFailHander != NULL)
        {
            c->subscribeFailHander(c->pContext,(char*)messagehandler.topicFilter);
        }

        ERROR_LOG("QOS of Sub ack packet error,grantedQoS = %d",grantedQoS);
        return JIOT_ERR_MQTT_SUBSCRIBE_QOS_ERROR;
    }


    int i;
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        /*If subscribe the same topic and callback function, then ignore*/
        if((0 != strlen(c->messageHandlers[i].topicFilter)) && (0 == whether_messagehandler_same(&c->messageHandlers[i],&messagehandler)))
        {
            rc = JIOT_SUCCESS;
            break;
        }
    }
    
    if(JIOT_SUCCESS != rc)
    {
        for(i = 0 ;i < MAX_MESSAGE_HANDLERS;++i)
        {
            if (0 == strlen(c->messageHandlers[i].topicFilter))
            {
                strcpy(c->messageHandlers[i].topicFilter ,messagehandler.topicFilter);
                c->messageHandlers[i].Qos = messagehandler.Qos;
                c->messageHandlers[i].fp = messagehandler.fp;

                break;
            }
        }
    }

    return JIOT_SUCCESS;
}

int recvPublishProc(JMQTTClient *c)
{
    ENTRY;
    int result = 0;
    MQTTString topicName;
    memset(&topicName,0x0,sizeof(topicName));
    MQTTMessage msg;
    memset(&msg,0x0,sizeof(msg));

    if (MQTTDeserialize_publish((unsigned char*)&msg.dup, (int*)&msg.qos, (unsigned char*)&msg.retained, (unsigned short*)&msg.id, &topicName,
       (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->recvbuf, c->recvbuf_size) != 1)
    {
        return JIOT_ERR_MQTT_PUBLISH_PACKET_ERROR;
    }

    deliverMessage(c, &topicName, &msg);
    if (msg.qos == QOS0)
    {
        return JIOT_SUCCESS;
    }

    if (msg.qos == QOS1)
    {
        result = MQTTPuback(c,msg.id,PUBACK);
    }
    else if (msg.qos == QOS2)
    {
        result = MQTTPuback(c,msg.id,PUBREC);
    }
    else
    {
        ERROR_LOG("Qos is error,qos = %d",msg.qos);
        return JIOT_ERR_MQTT_PUBLISH_QOS_ERROR;
    }

    return result;
}

int recvPubRecProc(JMQTTClient *c)
{
    ENTRY;
    unsigned short mypacketid;
    unsigned char dup = 0;
    unsigned char type = 0;
    int rc = 0;

    if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->recvbuf, c->recvbuf_size) != 1)
    {
        return JIOT_ERR_MQTT_PUBLISH_REC_PACKET_ERROR;
    }

    del_pubInfo_from(c,mypacketid);

    rc = MQTTPuback(c,mypacketid,PUBREL);
    if (rc == JIOT_ERR_MQTT_NETWORK_ERROR)
    {
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }

    return rc;
}


int recvPubCompProc(JMQTTClient *c)
{
    ENTRY;
    unsigned short mypacketid;
    unsigned char dup = 0;
    unsigned char type = 0;

    if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->recvbuf, c->recvbuf_size) != 1)
    {
        return JIOT_ERR_MQTT_PUBLISH_COMP_PACKET_ERROR;
    }

    return JIOT_SUCCESS;
}

int recvUnsubAckProc(JMQTTClient *c)
{
    ENTRY;
    unsigned short mypacketid = 0;  // should be the same as the packetid above
    if (MQTTDeserialize_unsuback(&mypacketid,c->recvbuf, c->recvbuf_size) != 1)
    {
        return JIOT_ERR_MQTT_UNSUBSCRIBE_ACK_PACKET_ERROR;
    }

    MessageHandler messageHandler;

    messageHandler.fp = NULL;
    memset(messageHandler.topicFilter,0,JIOT_TOPIC_FILTER_LEN);
    del_subInfo_from(c,mypacketid,&messageHandler);
    /* Remove from message handler array */

    unsigned int i;
    for(i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if((strlen(c->messageHandlers[i].topicFilter)!= 0) && (0 == whether_messagehandler_same(&c->messageHandlers[i],&messageHandler)))
        {
            memset(c->messageHandlers[i].topicFilter,0,JIOT_TOPIC_FILTER_LEN);
            c->messageHandlers[i].Qos = 0;
            c->messageHandlers[i].fp = NULL;
            /* We don't want to break here, in case the same topic is registered with 2 callbacks. Unlikely scenario */
        }
    }

    return JIOT_SUCCESS;
}


int waitforConnack(JMQTTClient * c)
{
    ENTRY;
    unsigned int packetType = 0;
    int rc = 0;
    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms);

    do
    {
        // read the socket, see what work is due
        rc = readPacket(c, &timer,&packetType);
		if(rc == JIOT_ERR_NET_READ_TIMEOUT)
		{
			continue;
		}
		else
        if(rc != JIOT_SUCCESS)
        {
            ERROR_LOG("readPacket error,result = %d",rc);
            return JIOT_ERR_MQTT_NETWORK_ERROR;
        }

    }while(packetType != CONNACK);

    rc = recvConnAckProc(c);
    if(JIOT_SUCCESS != rc)
    {
        ERROR_LOG("recvConnackProc error,result = %d",rc);
    }

    return rc;
}

int waitforSuback(JMQTTClient * c)
{
    ENTRY;
    unsigned int packetType = 0;
    int rc = 0;
    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms);

    do
    {
        // read the socket, see what work is due
        rc = readPacket(c, &timer,&packetType);
		if(rc == JIOT_ERR_NET_READ_TIMEOUT)
		{
			continue;
		}
		else
        if(rc != JIOT_SUCCESS)
        {
            ERROR_LOG("readPacket error,result = %d",rc);
            return JIOT_ERR_MQTT_NETWORK_ERROR;
        }

    }while(packetType != SUBACK);

    rc = recvSubAckProc(c);
    if(JIOT_SUCCESS != rc)
    {
        ERROR_LOG("recvSubAckProc error,result = %d",rc);
    }

    return rc;
}

int cycle(JMQTTClient * c, Timer* timer)
{
    unsigned int packetType;
    int rc = JIOT_SUCCESS;

    JMQTTClientStatus state = jiot_mqtt_get_client_state(c);
    if(state != CLIENT_STATE_CONNECTED)
    {
        //DEBUG_LOG("state = %d",state);
        return JIOT_ERR_MQTT_STATE_ERROR;
    }

    // read the socket, see what work is due
    rc = readPacket(c, timer,&packetType);
    if(rc == JIOT_FAIL)
    {
        jiot_mqtt_set_client_state(c, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        DEBUG_LOG("readPacket error,result = %d",rc);
        c->errCode = rc ;
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }
	else
	if(rc == JIOT_ERR_NET_READ_TIMEOUT)
	{
		return JIOT_SUCCESS;
	}
	
    NowTimer(&c->commandRecvTimer); //收到消息
    switch (packetType)
    {
        case CONNACK:
        {
            break;
        }
        case PUBACK:
        {
            rc = recvPubAckProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvPubackProc error,result = %d",rc);
            }
            break;
        }
        case SUBACK:
        {
            rc = recvSubAckProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvSubAckProc error,result = %d",rc);
            }
            break;
        }
        case PUBLISH:
        {
            rc = recvPublishProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvPublishProc error,result = %d",rc);
            }
            break;
        }
        case PUBREC:
        {
            rc = recvPubRecProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvPubRecProc error,result = %d",rc);
            }
            break;
        }
        case PUBCOMP:
        {
            rc = recvPubCompProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvPubCompProc error,result = %d",rc);
            }
            break;
        }
        case UNSUBACK:
        {
            rc = recvUnsubAckProc(c);
            if(JIOT_SUCCESS != rc)
            {
                ERROR_LOG("recvUnsubAckProc error,result = %d",rc);
            }
            break;
        }
        case PINGRESP:
        {
            rc = JIOT_SUCCESS;
            DEBUG_LOG("receive ping response!");
            break;
        }
        default:
            ERROR_LOG("ukonw packetType ,packetType = %d",packetType);
            return JIOT_FAIL;
    }

    return rc;
}


bool whether_mqtt_client_state_normal(JMQTTClient *c)
{
    if(jiot_mqtt_get_client_state(c) == CLIENT_STATE_CONNECTED)
    {
        return true;
    }

    return false;
}


int whether_messagehandler_same(MessageHandler *messageHandlers1,MessageHandler *messageHandler2)
{
    int topicNameLen = strlen(messageHandlers1->topicFilter);

    if(topicNameLen != strlen(messageHandler2->topicFilter))
    {
        return 1;
    }

    if(strncmp(messageHandlers1->topicFilter, messageHandler2->topicFilter, topicNameLen) != 0)
    {
        return 1;
    }

    if(messageHandlers1->fp != messageHandler2->fp)
    {
        return 1;
    }

    return 0;
}

int jiot_mqtt_subscribe(JMQTTClient * c, const char* topicFilter, int qos, messageHandler handler,bool flag)
{ 
    ENTRY;
    if(NULL == c || NULL == topicFilter)
    {
        c->errCode = JIOT_ERR_MQTT_NULL_VALUE_ERROR ;
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    int rc = JIOT_FAIL;
    /*
    if (!whether_mqtt_client_state_normal(c))
    {
        ERROR_LOG("mqtt client state is error,state = %d",jiot_mqtt_get_client_state(c));
        return JIOT_ERR_MQTT_STATE_ERROR;
    }
    */
    if(0 != common_check_topic(topicFilter,TOPIC_FILTER_TYPE))
    {
        c->errCode = JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR ;
        ERROR_LOG("topic format is error,topicFilter = %s",topicFilter);
        return JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR;
    }

    unsigned int msgId = getNextPacketId(c);
    rc = MQTTSubscribe(c,topicFilter, qos,msgId,handler);
    if (rc != JIOT_SUCCESS)
    {
        if(rc == JIOT_ERR_MQTT_NETWORK_ERROR)
        {
            jiot_mqtt_set_client_state(c, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        }

        c->errCode = rc;
        ERROR_LOG("run MQTTSubscribe error");
        return rc;
    }

    if (flag)
    {
        //同步订阅，等待SUB_ACK ，在建立连接时使用
        rc = waitforSuback(c);
        if (rc != JIOT_SUCCESS)
        {
            ERROR_LOG("recv MQTTSubscribeAck error");
            return rc;
        }
    }

    INFO_LOG("mqtt subscribe success,topic = %s!",topicFilter);
    return JIOT_SUCCESS;
}

int jiot_mqtt_unsubscribe(JMQTTClient * c, const char* topicFilter)
{   
    ENTRY;
    if(NULL == c || NULL == topicFilter)
    {
        c->errCode = JIOT_ERR_MQTT_NULL_VALUE_ERROR ;
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    if(0 != common_check_topic(topicFilter,TOPIC_FILTER_TYPE))
    {
        ERROR_LOG("topic format is error,topicFilter = %s",topicFilter);
        c->errCode = JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR ;
        return JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR;
    }

    int rc = JIOT_FAIL;
    
    if (!whether_mqtt_client_state_normal(c))
    {
        ERROR_LOG("mqtt client state is error,state = %d",jiot_mqtt_get_client_state(c));
        return JIOT_ERR_MQTT_STATE_ERROR;
    }
    
    unsigned int msgId = getNextPacketId(c);

    rc = MQTTUnsubscribe(c,topicFilter, msgId);
    if(rc != JIOT_SUCCESS)
    {
        if (rc == JIOT_ERR_MQTT_NETWORK_ERROR) // send the subscribe packet
        {
            jiot_mqtt_set_client_state(c, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        }
        c->errCode = rc ;
        ERROR_LOG("run MQTTUnsubscribe error!");
        return rc;
    }
    
    INFO_LOG("mqtt unsubscribe success,topic = %s!",topicFilter);
    return JIOT_SUCCESS;
}


int jiot_mqtt_publish(JMQTTClient * c, const char* topicName, MQTTMessage* message,long long seqNo,E_JIOT_MSG_TYPE type)
{
    ENTRY;
    if(NULL == c || NULL == topicName || NULL == message)
    {
        c->errCode = JIOT_ERR_MQTT_NULL_VALUE_ERROR ;
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    if(0 != common_check_topic(topicName,TOPIC_NAME_TYPE))
    {
        ERROR_LOG("topic format is error,topicFilter = %s",topicName);
        c->errCode = JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR ;
        return JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR;
    }

    int rc = JIOT_FAIL;
    
    if (!whether_mqtt_client_state_normal(c))
    {
        ERROR_LOG("mqtt client state is error,state = %d",jiot_mqtt_get_client_state(c));
        return JIOT_ERR_MQTT_STATE_ERROR;
    }

    if (message->qos == QOS1 || message->qos == QOS2)
    {
        message->id = getNextPacketId(c);
    }
    
    rc = MQTTPublish(c,topicName, message,seqNo,type);
    if (rc != JIOT_SUCCESS) // send the subscribe packet
    {
        if(rc == JIOT_ERR_MQTT_NETWORK_ERROR)
        {
            jiot_mqtt_set_client_state(c, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        }
        c->errCode = rc ;
        ERROR_LOG("MQTTPublish is error,rc = %d",rc);
        return rc;
    }

    return JIOT_SUCCESS;
}
 
JMQTTClientStatus jiot_mqtt_get_client_state(JMQTTClient *pClient)
{
	JMQTTClientStatus state;
	jiot_mutex_lock(&pClient->statusLock);
	state = pClient->status;
	jiot_mutex_unlock(&pClient->statusLock);

	return state;
}

void jiot_mqtt_set_client_state(JMQTTClient *pClient, JMQTTClientStatus newState)
{
    JMQTTClientStatus oldState = pClient->status ;

    jiot_mutex_lock(&pClient->statusLock);

    if (newState == CLIENT_STATE_INITIALIZED)
    {
       pClient->status =newState;
    }
    else
    if (pClient->status != CLIENT_STATE_DISCONNECTED)
    {
        pClient->status = newState;    
    }

	jiot_mutex_unlock(&pClient->statusLock);

    //连接状态转变为断连状态
    if(newState == CLIENT_STATE_DISCONNECTED_RECONNECTING||newState == CLIENT_STATE_DISCONNECTED)
    {
        if(oldState == CLIENT_STATE_CONNECTED)
        {
            pClient->disconnectHander(pClient->pContext);
        }  
    }
    
}

int mqtt_set_connect_params(JMQTTClient *pClient, MQTTPacket_connectData *pConnectParams)
{
	ENTRY;
	
	if(NULL == pClient || NULL == pConnectParams) 
	{
		return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
	}

	memcpy(pClient->mqttConnData.struct_id , pConnectParams->struct_id , 4);
	pClient->mqttConnData.struct_version = pConnectParams->struct_version;
	pClient->mqttConnData.MQTTVersion = pConnectParams->MQTTVersion;
	pClient->mqttConnData.clientID = pConnectParams->clientID;
	pClient->mqttConnData.cleansession = pConnectParams->cleansession;
	pClient->mqttConnData.willFlag = pConnectParams->willFlag;
	pClient->mqttConnData.username = pConnectParams->username;
	pClient->mqttConnData.password = pConnectParams->password;
	memcpy(pClient->mqttConnData.will.struct_id , pConnectParams->will.struct_id, 4);
	pClient->mqttConnData.will.struct_version = pConnectParams->will.struct_version;
	pClient->mqttConnData.will.topicName = pConnectParams->will.topicName;
	pClient->mqttConnData.will.message = pConnectParams->will.message;
	pClient->mqttConnData.will.qos = pConnectParams->will.qos;
	pClient->mqttConnData.will.retained = pConnectParams->will.retained;

	if(pConnectParams->keepAliveInterval < KEEP_ALIVE_INTERVAL_DEFAULT_MIN)
	{
	    pClient->mqttConnData.keepAliveInterval = KEEP_ALIVE_INTERVAL_DEFAULT_MIN;
	}
	else if(pConnectParams->keepAliveInterval > KEEP_ALIVE_INTERVAL_DEFAULT_MAX)
	{
	    pClient->mqttConnData.keepAliveInterval = KEEP_ALIVE_INTERVAL_DEFAULT_MAX;
	}
	else
	{
		pClient->mqttConnData.keepAliveInterval = pConnectParams->keepAliveInterval;
	}

	return JIOT_SUCCESS;
}

int jiot_mqtt_init(JMQTTClient * pClient,
                    void* pContext,
                    SubscribeHander * pSubscribe,
                    ConnectedHander * pConnected,
                    ConnectFailHander * pConnectFail,
                    DisconnectHander  * pDisConnect,
                    SubscribeFailHander * pSubscribeFail,
                    PublishFailHander   * pPublishFail,
                    MessageTimeoutHander * pMessageTimeout) 
{
    ENTRY;
    
    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

	pClient->next_packetid = 0;
	
	//消息发送超时时间
	pClient->command_timeout_ms = COMMAND_TIMEOUT_MS;
    pClient->reConnIntervalTime = 0;
    
    jiot_mutex_init(&pClient->idLock);
    jiot_mutex_init(&pClient->statusLock);
    jiot_mutex_init(&pClient->writebufLock);
    jiot_mutex_init(&pClient->publistLock);
    jiot_mutex_init(&pClient->sublistLock);
    
    InitTimer(&pClient->connTimer);
    InitTimer(&pClient->commandTimer);
    InitTimer(&pClient->commandRecvTimer);

    NowTimer(&pClient->connTimer);
    NowTimer(&pClient->commandTimer);
    NowTimer(&pClient->commandRecvTimer);

    memset(&pClient->threadRecv,0x0,sizeof(S_JIOT_PTHREAD));
    memset(&pClient->threadRetrans,0x0,sizeof(S_JIOT_PTHREAD));

    pClient->pContext = pContext;

    pClient->subscribeHander        = pSubscribe;
    pClient->connectedHander        = pConnected;
    pClient->connectFailHander      = pConnectFail;
    pClient->disconnectHander       = pDisConnect;
    pClient->subscribeFailHander    = pSubscribeFail;
    pClient->publishFailHander      = pPublishFail;
    pClient->messageTimeoutHander   = pMessageTimeout;

    pClient->pubInfoList = initHead();
    pClient->subInfoList = initHead();
    
    pClient->ipstack = (Network*)jiot_malloc(sizeof(Network));

    memset(pClient->ipstack,0x0,sizeof(Network));
	if(NULL == pClient->ipstack)
	{
		ERROR_LOG("malloc Network failed");
        pClient->errCode = JIOT_FAIL;
		return JIOT_FAIL;
	}

    jiot_mqtt_set_client_state(pClient,CLIENT_STATE_INVALID);

    if(JIOT_SUCCESS != jiot_create_thread(pClient))
    {
        pClient->errCode = JIOT_ERR_MQTT_CREATE_THREAD_ERROR ;
        return JIOT_ERR_MQTT_CREATE_THREAD_ERROR;
    }

    return JIOT_SUCCESS ;
}

int jiot_mqtt_conn_init(JMQTTClient *pClient, IOT_CLIENT_INIT_PARAMS *pInitParams, char* szHostAddress,char * szPort, char* szPubkey)
{
	ENTRY;
    pClient->errCode = 0 ;

    if((NULL == pClient) || (NULL == pInitParams))
    {
        pClient->errCode = JIOT_ERR_MQTT_NULL_VALUE_ERROR;
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }
	int rc = JIOT_FAIL;

	int i = 0;    
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
	{
        memset(pClient->messageHandlers[i].topicFilter,0,JIOT_TOPIC_FILTER_LEN);
        pClient->messageHandlers[i].Qos = 0;
		pClient->messageHandlers[i].fp = NULL;
    }

    
    MQTTPacket_connectData connectdata = MQTTPacket_connectData_initializer;

    pClient->buf = (unsigned char *)pInitParams->buf;
    pClient->buf_size = pInitParams->buf_size;
    pClient->recvbuf = (unsigned char *)pInitParams->recvbuf;
    pClient->recvbuf_size = pInitParams->recvbuf_size;
    pClient->defaultMessageHandler = NULL;
    

	/*init mqtt connect params*/
	rc = mqtt_set_connect_params(pClient, &connectdata);
	if(JIOT_SUCCESS != rc)
	{
        pClient->errCode = rc;
	    jiot_mqtt_set_client_state(pClient,CLIENT_STATE_INVALID);
		return rc;
	}
	
	memset(pClient->ipstack,0x0,sizeof(Network));

    rc = jiot_mqtt_network_init(pClient->ipstack, szHostAddress, szPort, szPubkey);
    if(JIOT_SUCCESS != rc)
    {
        pClient->errCode = rc;
        memset(pClient->ipstack,0x0,sizeof(Network));
        jiot_mqtt_set_client_state(pClient,CLIENT_STATE_INVALID);
        return rc;
    }

 
    NowTimer(&pClient->connTimer);
    NowTimer(&pClient->commandTimer);
    NowTimer(&pClient->commandRecvTimer);
    
    jiot_mqtt_set_client_state(pClient,CLIENT_STATE_INITIALIZED);
	
	INFO_LOG("mqtt conn init success!");
	return JIOT_SUCCESS;
}


void mqtt_yield(JMQTTClient * pClient, int timeout_ms)
{
    int rc = JIOT_FAIL;
    JMQTTClientStatus cliStatus = jiot_mqtt_get_client_state(pClient);
        
    while(cliStatus == CLIENT_STATE_CONNECTED)
    {
        Timer timer;
        InitTimer(&timer);  
        countdown_ms(&timer, timeout_ms);

        /*acquire package in cycle, such as PINGRESP  PUBLISH*/

        rc = cycle(pClient, &timer);
        
        if(JIOT_SUCCESS != rc)
        {
            //DEBUG_LOG("cycle failure,rc=%d",rc);
            break;
        }

        if(!expired(&timer))
        {
            break;
        }
    } 

    if(JIOT_SUCCESS != rc)
    {
        jiot_sleep(10);
    }
}

void* jiot_receive_thread(void * param)
{
    INFO_LOG("start the receive thread!");
    JMQTTClient * pClient = (JMQTTClient * )param;

    while(pClient->isRun)
    {
        mqtt_yield(pClient,pClient->command_timeout_ms);
    }

    INFO_LOG("exit the receive thread!");

    jiot_pthread_exit(&pClient->threadRecv);
    return NULL;
}


int MQTTSubInfoProc(JMQTTClient *pClient)
{
    int rc = JIOT_SUCCESS;

    jiot_mutex_lock(&pClient->sublistLock);
    _JLNode_t* node = NULL; 
    for (;;)
    {
        node = NextNode(pClient->subInfoList,node);
        if (NULL == node)
        {
            jiot_mutex_unlock(&pClient->sublistLock);
            break;
        }

        S_SUBSCRIBE_INFO *subInfo = (S_SUBSCRIBE_INFO *)node->_val;
        if (NULL == subInfo)
        {
            ERROR_LOG("node's value is invalid!");
            continue;
        }

        //超时清除对应数据。
        if (spend_ms(&subInfo->subTime) >= (long long)(pClient->command_timeout_ms*2))
        {
            char topicFilter[JIOT_TOPIC_FILTER_LEN] = {0};
            strcpy(topicFilter,subInfo->handler.topicFilter);
            
            DelNode(pClient->subInfoList,node);
            if(NULL != subInfo )
            {
                jiot_free(subInfo);
                subInfo = NULL;
            }

            jiot_mutex_unlock(&pClient->sublistLock);

            if (pClient->subscribeFailHander != NULL)
            {
                pClient->subscribeFailHander(pClient->pContext,topicFilter);
            }

            break;

        }

        //判断节点是否超时
        if((subInfo->count == 0) && spend_ms(&subInfo->subTime) >= (long long)(pClient->command_timeout_ms/2))
        {
            unsigned int msgId = subInfo->msgId;
            MessageHandler tempMessageHandler ;

            strcpy(tempMessageHandler.topicFilter,subInfo->handler.topicFilter);
            tempMessageHandler.fp = subInfo->handler.fp;
            tempMessageHandler.Qos = subInfo->handler.Qos;
            
            jiot_mutex_unlock(&pClient->sublistLock);

            //重新订阅
            if(subInfo->type == SUBSCRIBE)
            {
                if(whether_mqtt_client_state_normal(pClient))
                {   
                    MQTTReSubscribe(pClient,tempMessageHandler.topicFilter,tempMessageHandler.Qos,msgId,tempMessageHandler.fp);
                    subInfo->count++;
                }
            }
            break;
        }
    }

    return rc;
}

int MQTTRePublish(JMQTTClient *c,unsigned char*buf,int len)
{
    int nRet = JIOT_SUCCESS;
    Timer timer;
    InitTimer(&timer);
    countdown_ms(&timer, c->command_timeout_ms/2);
    
    jiot_mutex_lock(&c->writebufLock);
   
    DEBUG_LOG("MQTT RePublish msg");
    nRet = sendPacket(c, buf,len, &timer);

	jiot_mutex_unlock(&c->writebufLock);
    if ( nRet != JIOT_SUCCESS)
    {
        return JIOT_ERR_MQTT_NETWORK_ERROR;
    }
    return JIOT_SUCCESS;
}


int MQTTPubInfoProc(JMQTTClient *pClient)
{
    int rc = 0;
    _JLNode_t* node = NULL; 
    jiot_mutex_lock(&pClient->publistLock);
	
    for (;;)
    {
        node = NextNode(pClient->pubInfoList,node);
        if (NULL == node)
        {
            jiot_mutex_unlock(&pClient->publistLock);
            break;
        }
        
        S_REPUBLISH_INFO *repubInfo = (S_REPUBLISH_INFO *) node->_val;
        if (NULL == repubInfo)
        {
            ERROR_LOG("node's value is invalid!");

            _JLNode_t* tempNode = NextNode(pClient->pubInfoList,node);
            DelNode(pClient->pubInfoList,node);
            if (tempNode == NULL)
            {
                jiot_mutex_unlock(&pClient->publistLock);
                break;
            }
            else
            {
                node = tempNode;
                continue;
            }
        }
        
        long long nSpend_ms = spend_ms(&repubInfo->pubTime) ;
        //时间异常,重置时间
        if (nSpend_ms < 0 )
        {
            NowTimer(&repubInfo->pubTime);
            nSpend_ms = 0 ;
        }
        
        if((repubInfo->count > 0)&&(nSpend_ms >= pClient->command_timeout_ms + pClient->command_timeout_ms/2))
        {    
            //检测重连时间，避免频繁重连
            if (spend_ms(&pClient->connTimer) > pClient->command_timeout_ms + pClient->command_timeout_ms/2)
            {
                ERROR_LOG("publish msg timeout ,reconnect ");
                jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
                pClient->errCode = JIOT_ERR_MQTT_NETWORK_CONNECT_ERROR;
            }
            
            E_JIOT_MSG_TYPE type = repubInfo->msgType;
            long long seqNo = repubInfo->seqNo;

            //删除对应节点
            DelNode(pClient->pubInfoList,node);
            
            //释放节点的存储的消息的空间
            if(NULL != repubInfo )
            {
                jiot_free(repubInfo);
                repubInfo = NULL;
            }
            jiot_mutex_unlock(&pClient->publistLock);
            
            //调用超时回调函数
            if(type == E_JCLIENT_MSG_NORMAL )
            {
                pClient->messageTimeoutHander(pClient->pContext,seqNo);
            }
            
            break ;
        
        }
        else
        //判断节点是否超时,超过发送超时时间的一半时，重发消息
        if((repubInfo->count == 0)&&(nSpend_ms >= pClient->command_timeout_ms/2))
        {
            //当前JClient状态异常时不发送消息。
            if(!whether_mqtt_client_state_normal(pClient))
            {
                continue ;  
            }

            int tempLen = repubInfo->len;
            unsigned char* tempBuff = (unsigned char *)jiot_malloc(tempLen);
            memcpy(tempBuff,repubInfo->buf,repubInfo->len);
            repubInfo->count = 1;
            //先解锁，后发送消息
            jiot_mutex_unlock(&pClient->publistLock);

            rc = MQTTRePublish(pClient,tempBuff,tempLen);               
            if(JIOT_ERR_MQTT_NETWORK_ERROR == rc)
            {
                jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
                pClient->errCode = rc ;
                ERROR_LOG("republish msg network err  ");
            }

            jiot_free(tempBuff);
            
            break;
        }
    }

    return JIOT_SUCCESS;
}

void* jiot_retrans_thread(void*param)
{
    INFO_LOG("start the retrans thread!");
    JMQTTClient * pClient = (JMQTTClient * )param;

    while(pClient->isRun)
    {
        //如果是pub ack则删除缓存在pub list中的信息（QOS=1时）
        MQTTPubInfoProc(pClient);

        //如果是sub ack则删除缓存在sub list中的信息
        MQTTSubInfoProc(pClient);

        jiot_sleep(50);
    }

    INFO_LOG("exit the retrans thread");
    jiot_pthread_exit(&pClient->threadRetrans);
    
    return NULL;
}

int mqtt_keep_alive(JMQTTClient *pClient)
{
    int rc = JIOT_SUCCESS;

    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }
    
    //长时间没有接收到消息,判断会话超时，需要重连
    long long nSpend_ms = spend_ms(&pClient->commandRecvTimer) ;
    //时间异常重置时间，不重连，只发送一次MQTT的心跳。
    if (nSpend_ms < 0)
    {
        NowTimer(&pClient->commandRecvTimer);
        rc = MQTTKeepalive(pClient);
        if(JIOT_SUCCESS != rc)
        {
            if(rc == JIOT_ERR_MQTT_NETWORK_ERROR)
            {
                jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
            }
            pClient->errCode = rc;
            ERROR_LOG("ping outstanding is error,result = %d",rc);
            return rc;
        }
        return JIOT_SUCCESS;
    }

    if(nSpend_ms > (pClient->mqttConnData.keepAliveInterval*1000+pClient->command_timeout_ms) )
    {

        jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        pClient->errCode = JIOT_ERR_MQTT_PING_TIMEOUT_ERR;
        ERROR_LOG("ping resp time out,reconnect ");
        return JIOT_ERR_MQTT_PING_TIMEOUT_ERR;
    }

    if(spend_ms(&pClient->commandTimer) < pClient->mqttConnData.keepAliveInterval*1000)
    {
        return JIOT_SUCCESS;
    }
    //保活超时需要记录超时时间判断是否做断网重连处理
    rc = MQTTKeepalive(pClient);
    if(JIOT_SUCCESS != rc)
    {
        if(rc == JIOT_ERR_MQTT_NETWORK_ERROR)
        {
            jiot_mqtt_set_client_state(pClient, CLIENT_STATE_DISCONNECTED_RECONNECTING);
        }
        pClient->errCode = rc;
        ERROR_LOG("ping outstanding is error,result = %d",rc);
        return rc;
    }
    return JIOT_SUCCESS;
}

void jiot_mqtt_keepalive(JMQTTClient * pClient)
{
    do
    {
        JMQTTClientStatus currentState = jiot_mqtt_get_client_state(pClient);

        if (CLIENT_STATE_CONNECTED == currentState)
        {
            mqtt_keep_alive(pClient);    
        }
        else
        if(CLIENT_STATE_DISCONNECTED_RECONNECTING == currentState)
        {
            //check reconnect time
            long long nSpend_ms = spend_ms(&pClient->connTimer) ;
            if (nSpend_ms < 0)
            {
                NowTimer(&pClient->connTimer);
                nSpend_ms = 0 ;
            }

            if (nSpend_ms >= pClient->reConnIntervalTime*1000)
            {
                //重连前先退出接收，和重发线程
                jiot_mqtt_disconnect(pClient);
                
                int rc = mqtt_handle_reconnect(pClient);
                if(JIOT_SUCCESS != rc)
                {
                    ERROR_LOG("reconnect network fail, rc = %d",rc);
                    //重连
                    jiot_mqtt_set_client_state(pClient,CLIENT_STATE_DISCONNECTED_RECONNECTING);

                    if (pClient->reConnIntervalTime ==0)
                    {
                        pClient->reConnIntervalTime = 5;
                    }
                    else
                    if (pClient->reConnIntervalTime <20) 
                    {
                        //5, 10 20  
                        pClient->reConnIntervalTime = 2*pClient->reConnIntervalTime;
                    }
                    else
                    {
                        //重连失败，MQTT断链状态
                        ERROR_LOG("MQTT Client Status is disconnected ");
                        jiot_mqtt_set_client_state(pClient,CLIENT_STATE_DISCONNECTED);
                    }   
                }
                else
                {
                    INFO_LOG("MQTT Client Status is  Connected");
                    pClient->reConnIntervalTime = 0;
                    //连接成功
                    jiot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED);
                }
            }
            else
            {
                //DEBUG_LOG("waite for connect");
            }
        }
        else
        if(CLIENT_STATE_DISCONNECTED == currentState)
        {
            jiot_sleep(100);
            ERROR_LOG("network is disconnected");
            break;
        }
    }while(0);
    return ;
}

int jiot_create_thread(JMQTTClient * pClient)
{
    ENTRY;

    pClient->isRun = true ;
    
    int result = jiot_pthread_create(&pClient->threadRecv,jiot_receive_thread,pClient, "receive_thread", TASK_STACK_SIZE);

    if(0 != result)
    {
        pClient->isRun = false ;
        ERROR_LOG("run jiot_pthread_create error!");
        return JIOT_FAIL;
    }
    INFO_LOG("create recieveThread ");
    
    result = jiot_pthread_create(&pClient->threadRetrans,jiot_retrans_thread,pClient, "retrans_thread", TASK_STACK_SIZE/2);
    if(0 != result)
    {
        pClient->isRun = false ;
        ERROR_LOG("run jiot_retrans_thread error!");
        jiot_pthread_cancel(&pClient->threadRetrans);
        return JIOT_FAIL;
    }

    INFO_LOG("create retransThread");

    return JIOT_SUCCESS;
}

int jiot_delete_thread(JMQTTClient * pClient)
{
    ENTRY;
    int result = -1;

    pClient->isRun = false ;
    jiot_sleep(100); //等待线程退出

    result = jiot_pthread_cancel(&pClient->threadRecv);
    if(0 != result)
    {
    	ERROR_LOG("run jiot_pthread_cancel error!");
    	return JIOT_FAIL;
    }
    memset(&pClient->threadRecv,0x0,sizeof(S_JIOT_PTHREAD));
    INFO_LOG("delete recieveThread ");

	result = jiot_pthread_cancel(&pClient->threadRetrans);
	if(0 != result)
	{
		ERROR_LOG("run jiot_pthread_cancel error!");
		return JIOT_FAIL;
	}

	memset(&pClient->threadRetrans,0x0,sizeof(S_JIOT_PTHREAD));

    INFO_LOG("delete retransThread!");
    return JIOT_SUCCESS;
}

int mqtt_conn(JMQTTClient * pClient)
{
    int rc = JIOT_FAIL;
    NowTimer(&pClient->commandTimer);
    NowTimer(&pClient->commandRecvTimer);

    /*Establish TCP or TLS connection*/
    if(pClient->ipstack != NULL)
    {
        rc = pClient->ipstack->_connect(pClient->ipstack);

        if(JIOT_SUCCESS != rc)
        {
            pClient->ipstack->_disconnect(pClient->ipstack);
            ERROR_LOG("TCP or TLS Connection failed");
            pClient->errCode = rc ;
            return JIOT_ERR_MQTT_NETWORK_CONNECT_ERROR;
        }
    }
    else
    {
        return JIOT_ERR_MQTT_NETWORK_CONNECT_ERROR;
    }
    
    rc = MQTTConnect(pClient);
    if (rc  != JIOT_SUCCESS)
    {
        pClient->errCode = rc ;
        pClient->ipstack->_disconnect(pClient->ipstack);
        ERROR_LOG("send connect packet failed");
        return rc;
    }

    rc = waitforConnack(pClient);
    if(rc != JIOT_SUCCESS )
    {
        pClient->errCode = rc ;
        (void)MQTTDisconnect(pClient);
        pClient->ipstack->_disconnect(pClient->ipstack);
        ERROR_LOG("mqtt waitfor connect ack timeout!");
        return JIOT_ERR_MQTT_CONNECT_ERROR;
    }

    rc = pClient->subscribeHander(pClient->pContext);
    if(rc != JIOT_SUCCESS )
    {
        pClient->errCode = rc ;
        (void)MQTTDisconnect(pClient);
        pClient->ipstack->_disconnect(pClient->ipstack);
        ERROR_LOG("mqtt waitfor connect ack timeout!");
        return JIOT_ERR_MQTT_CONNECT_ERROR;
    }

    return JIOT_SUCCESS;
}


int jiot_mqtt_connect(JMQTTClient * pClient, char* szClientID, char* szUserName, char* szPassWord)
{
	ENTRY;
    int rc = JIOT_FAIL;

    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    //根据用户信息初始化连接参数
    pClient->mqttConnData.clientID.cstring  = szClientID;
    pClient->mqttConnData.username.cstring  = szUserName;
    pClient->mqttConnData.password.cstring  = szPassWord;

    INFO_LOG("mqtt reconnect mqtt server [%s:%s]",pClient->ipstack->connectparams.pHostAddress,pClient->ipstack->connectparams.pHostPort);
    // don't send connect packet again if we are already connected
    if (whether_mqtt_client_state_normal(pClient))
    {
        INFO_LOG("already connected!");
        return JIOT_SUCCESS;
    }

    pClient->errCode = 0 ;

    rc = mqtt_conn(pClient);
    NowTimer(&pClient->connTimer);
    
    if (rc == JIOT_SUCCESS)
    {
        jiot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED);
        INFO_LOG("mqtt client connect success!");
    }
    
    return rc;
}

int mqtt_reconnect(JMQTTClient * pClient)
{
    ENTRY;
    int rc = JIOT_FAIL;

    if((NULL == pClient) )
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }
    INFO_LOG("mqtt reconnect mqtt server [%s:%s]",pClient->ipstack->connectparams.pHostAddress,pClient->ipstack->connectparams.pHostPort);
    // don't send connect packet again if we are already connected
    if (whether_mqtt_client_state_normal(pClient))
    {
        INFO_LOG("already connected!");
        return JIOT_SUCCESS;
    }
    
    pClient->errCode = 0 ;

    rc = mqtt_conn(pClient);
    NowTimer(&pClient->connTimer);

    if (rc == JIOT_SUCCESS)
    {
        jiot_mqtt_set_client_state(pClient, CLIENT_STATE_CONNECTED);
        INFO_LOG("mqtt client reconnect success!");    
    }
    
    return rc;
}

int mqtt_handle_reconnect(JMQTTClient  *pClient)
{
	int rc;
    INFO_LOG("start reconnect");

	INFO_LOG("reconnect params:Host=%s Port = %s MQTTVersion =%d clientID =%s keepAliveInterval =%d username = %s",
        pClient->ipstack->connectparams.pHostAddress, 
        pClient->ipstack->connectparams.pHostPort,
		pClient->mqttConnData.MQTTVersion,
		pClient->mqttConnData.clientID.cstring,
		pClient->mqttConnData.keepAliveInterval,
		pClient->mqttConnData.username.cstring);

	/* Ignoring return code. failures expected if network is disconnected */
	rc = mqtt_reconnect(pClient);

    NowTimer(&pClient->connTimer);

	if(JIOT_SUCCESS != rc)
	{
	    ERROR_LOG("run mqtt_reconnect error!");
        pClient->connectFailHander(pClient->pContext,rc);
	    return rc;
	}

	return JIOT_SUCCESS;
}

int jiot_mqtt_release(JMQTTClient  *pClient)
{
    ENTRY;

    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }
    
    jiot_delete_thread(pClient);

    jiot_mqtt_set_client_state(pClient, CLIENT_STATE_INVALID);

    release_subInfo(pClient);
    release_pubInfo(pClient);

    jiot_mutex_destory(&pClient->idLock);
    jiot_mutex_destory(&pClient->statusLock);
    jiot_mutex_destory(&pClient->writebufLock);
    jiot_mutex_destory(&pClient->publistLock);
    jiot_mutex_destory(&pClient->sublistLock);

    if(NULL != pClient->ipstack)
    {
        jiot_free(pClient->ipstack);
    }

    INFO_LOG("mqtt release!");
    return JIOT_SUCCESS;
}

int jiot_mqtt_closed(JMQTTClient  *pClient)
{
    ENTRY;
    
    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    (void)MQTTDisconnect(pClient);

    if (pClient->ipstack->_disconnect != NULL )
    {
        //esp8266平台在关闭连接时会触发读失败，主动关闭时先修改状态
        jiot_mqtt_set_client_state(pClient,CLIENT_STATE_DISCONNECTED);
        pClient->ipstack->_disconnect(pClient->ipstack);
    }

    
    return JIOT_SUCCESS;
}

int jiot_mqtt_disconnect(JMQTTClient  *pClient)
{
    ENTRY;
    
    if(NULL == pClient)
    {
        return JIOT_ERR_MQTT_NULL_VALUE_ERROR;
    }

    (void)MQTTDisconnect(pClient);

    /*close tcp/ip socket or free tls resources*/

    if (pClient->ipstack->_disconnect != NULL )
    {
        pClient->ipstack->_disconnect(pClient->ipstack);
    }

	INFO_LOG("mqtt disconnect!");
	return JIOT_SUCCESS;
}

int del_pubInfo_from(JMQTTClient * c,unsigned int  msgId)
{
    S_REPUBLISH_INFO *repubInfo = NULL;
    jiot_mutex_lock(&c->publistLock);
    _JLNode_t *node = NULL ;
    for (;;)
    {
        node = NextNode(c->pubInfoList,node);
        if (NULL == node)
        {
            break;
        }

        repubInfo = (S_REPUBLISH_INFO *) node->_val;
        if (NULL == repubInfo)
        {
            ERROR_LOG("node's value is invalid!");

            _JLNode_t * tempNode = NextNode(c->pubInfoList,node);

            DelNode(c->pubInfoList,node);
            node = tempNode;

            if(node == NULL)
            {
                break;
            }
            else
            {
                continue;
            }
        }

        if (repubInfo->msgId == msgId)
        {
            DelNode(c->pubInfoList,node);
            jiot_free(repubInfo);

            break;
        }
    }
    
    jiot_mutex_unlock(&c->publistLock);
    return JIOT_SUCCESS;
}

int push_pubInfo_to(JMQTTClient * c,int len,unsigned short msgId,long long seqNo,E_JIOT_MSG_TYPE type )
{
    //开辟内存空间
    S_REPUBLISH_INFO *repubInfo = (S_REPUBLISH_INFO*)jiot_malloc(sizeof(S_REPUBLISH_INFO)+len);
    if(NULL == repubInfo)
    {
        ERROR_LOG("run jiot_memory_malloc is error!");
        return JIOT_FAIL;
    }

    repubInfo->seqNo = seqNo;
    repubInfo->msgType = type;
    repubInfo->msgId = msgId;
    repubInfo->len = len;
    NowTimer(&repubInfo->pubTime);
    repubInfo->count = 0;
    repubInfo->buf = (unsigned char*)repubInfo + sizeof(S_REPUBLISH_INFO);

    //复制保存的内容
    memcpy(repubInfo->buf,c->buf,len);
    
    _JLNode_t* tempNode = PushList(c->pubInfoList,(void*)repubInfo);
     
    if(tempNode == NULL)
    {
        return JIOT_FAIL;
    }   
    return JIOT_SUCCESS;
}

int del_subInfo_from(JMQTTClient * c,unsigned int  msgId,MessageHandler *handler)
{
    S_SUBSCRIBE_INFO *subInfo = NULL;
    _JLNode_t *node = NULL ;
    jiot_mutex_lock(&c->sublistLock);
    for (;;)
    {
        node = NextNode(c->subInfoList,node);
        if (NULL == node)
        {
            break;
        }

        subInfo = (S_SUBSCRIBE_INFO *) node->_val;
        if (NULL == subInfo)
        {
            ERROR_LOG("node's value is invalid!");

            _JLNode_t * tempNode = NextNode(c->subInfoList,node);

            DelNode(c->subInfoList,node);
            node = tempNode;

            if(node == NULL)
            {
                break;
            }
            else
            {
                continue;
            }
            
        }

        if (subInfo->msgId == msgId)
        {
            strcpy(handler->topicFilter,subInfo->handler.topicFilter);
            handler->fp = subInfo->handler.fp;
            handler->Qos = subInfo->handler.Qos;
            DelNode(c->subInfoList,node);
            jiot_free(subInfo);

            break;
        }
    }
    jiot_mutex_unlock(&c->sublistLock);
    return JIOT_SUCCESS;
}

int push_subInfo_to(JMQTTClient * c,int len,unsigned short msgId,enum msgTypes type,MessageHandler *handler)
{
    S_SUBSCRIBE_INFO *subInfo = (S_SUBSCRIBE_INFO*)jiot_malloc(sizeof(S_SUBSCRIBE_INFO));
    if(NULL == subInfo)
    {
        ERROR_LOG("run jiot_memory_malloc is error!");
        return JIOT_FAIL;
    }

    subInfo->msgId = msgId;
    NowTimer(&subInfo->subTime);
    subInfo->type = type;
    subInfo->count = 0;
    memcpy(subInfo->handler.topicFilter,handler->topicFilter,JIOT_TOPIC_FILTER_LEN);
    subInfo->handler.fp = handler->fp;
    subInfo->handler.Qos = handler->Qos; ;
    
    
    _JLNode_t* tempNode = PushList(c->subInfoList,(void*)subInfo);
    
    if(tempNode == NULL)
    {
        return JIOT_FAIL;
    } 

    return JIOT_SUCCESS;
}


int  release_pubInfo(JMQTTClient * c)
{
    _JLNode_t * node = NULL;
    jiot_mutex_lock(&c->publistLock);
    for (;;)
    {
        node = NextNode(c->pubInfoList,node);
        if (NULL == node)
        {
            break;
        }

        S_REPUBLISH_INFO * repubInfo = (S_REPUBLISH_INFO *) node->_val;
        _JLNode_t * tempNode = NextNode(c->pubInfoList,node);
        
        if (NULL != repubInfo)
        {
            jiot_free(repubInfo);  
        }

        DelNode(c->pubInfoList,node);
        node = tempNode;

        if(node == NULL)
        {
            break;
        }
        else
        {
            continue;
        }

    }

    releaseHead(c->pubInfoList);
    c->pubInfoList = NULL;

    jiot_mutex_unlock(&c->publistLock);
    return 0;
}

int  release_subInfo(JMQTTClient * c)
{   
    _JLNode_t * node = NULL;
    jiot_mutex_lock(&c->sublistLock);
    for (;;)
    {
        node = NextNode(c->subInfoList,node);
        if (NULL == node)
        {
            break;
        }

        S_SUBSCRIBE_INFO * subInfo = (S_SUBSCRIBE_INFO *) node->_val;
        _JLNode_t * tempNode = NextNode(c->subInfoList,node);
        
        if (NULL != subInfo)
        {
            jiot_free(subInfo);  
        }

        DelNode(c->subInfoList,node);
        node = tempNode;

        if(node == NULL)
        {
            break;
        }
        else
        {
            continue;
        }

    }
    
    releaseHead(c->subInfoList);
    c->subInfoList = NULL;

    jiot_mutex_unlock(&c->sublistLock);
    return 0;
}

int common_check_rule(char *iterm,E_TOPIC_TYPE type)
{
    if(NULL == iterm)
    {
        ERROR_LOG("iterm is NULL");
        return JIOT_FAIL;
    }

    int i = 0;
    int len = strlen(iterm);
    for(i = 0;i<len;i++)
    {
        if(TOPIC_FILTER_TYPE == type)
        {
            if('+' == iterm[i] || '#' == iterm[i])
            {
                if(1 != len)
                {
                    ERROR_LOG("the character # and + is error");
                    return JIOT_FAIL;
                }
            }
        }
        else
        {
            if('+' == iterm[i] || '#' == iterm[i])
            {
                ERROR_LOG("has character # and + is error");
                return JIOT_FAIL;
            }
        }

        if(iterm[i] < 32 || iterm[i] >= 127 )
        {
            return JIOT_FAIL;
        }
    }
    return JIOT_SUCCESS;
}


int common_check_topic(const char * topicName,E_TOPIC_TYPE type)
{
    if(NULL == topicName )
    {
        return JIOT_FAIL;
    }

    int mask = 0;
    char topicString[JIOT_TOPIC_FILTER_LEN];
    memset(topicString,0x0,JIOT_TOPIC_FILTER_LEN);
    strncpy(topicString,topicName,strlen(topicName));

    char* delim = "/";
    char* iterm = NULL;
    iterm = strtok(topicString,delim);

    if(JIOT_SUCCESS != common_check_rule(iterm,type))
    {
        ERROR_LOG("run common_check_rule error");
        return JIOT_FAIL;
    }

    for(;;)
    {
        iterm = strtok(NULL,delim);

        if(iterm == NULL)
        {
            break;
        }

        //当路径中包含#字符，且不是最后一个路径名时报错
        if(1 == mask)
        {
            ERROR_LOG("the character # is error");
            return JIOT_FAIL;
        }

        if(JIOT_SUCCESS != common_check_rule(iterm,type))
        {
            ERROR_LOG("run common_check_rule error");
            return JIOT_FAIL;
        }

        if(iterm[0] == '#')
        {
            mask = 1;
        }
    }

    return JIOT_SUCCESS;
}
