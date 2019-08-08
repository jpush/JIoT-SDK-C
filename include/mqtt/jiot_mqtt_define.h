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
 *    Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentationcJSON_strdup
 *    Ian Craggs - documentation and platform specific header
 *    Ian Craggs - add setMessageHandler function
 *******************************************************************************/

#if !defined(JIOT_MQTT_STRUCT_H)
#define JIOT_MQTT_STRUCT_H

#if defined(__cplusplus)
 extern "C" {
#endif

#include <stdbool.h>

#include "jiot_common.h"
#include "jiot_timer.h"
#include "jiot_pthread.h"
#include "jiot_list.h"
#include "MQTTPacket.h"

#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */

#define MAX_MESSAGE_HANDLERS 5 /* redefinable - how many subscriptions do you want? */

#define JIOT_TOPIC_FILTER_LEN 128

#ifndef MQTT_BUF_MAX_LEN
#define MQTT_BUF_MAX_LEN        2048+512  //mqtt读和写的缓冲区长度
#endif

#ifndef TASK_STACK_SIZE    
#define TASK_STACK_SIZE         8192      //freeRTOS任务的堆栈大小 
#endif

#define ENTRY DEBUG_LOG("enter fuction .")


enum QoS { QOS0, QOS1, QOS2, SUBFAIL=0x80 };

typedef enum TOPIC_TYPE
{
    TOPIC_NAME_TYPE = 0,
    TOPIC_FILTER_TYPE
}E_TOPIC_TYPE;

typedef enum
{
    CLIENT_STATE_INVALID = 0,                    //client无效状态
    CLIENT_STATE_INITIALIZED = 1,                //client初始化状态
    CLIENT_STATE_CONNECTED = 2,                  //client已经连接状态
    CLIENT_STATE_DISCONNECTED = 3,               //client连接丢失状态
    CLIENT_STATE_DISCONNECTED_RECONNECTING = 4   //client正在重连状态
} JMQTTClientStatus;

typedef enum
{
    E_JCLIENT_MSG_NORMAL = 0,                     //0:业务消息、
    E_JCLIENT_MSG_PING   = 1                      //1:心跳

} E_JIOT_MSG_TYPE;

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

#define MQTTMessage_initializer { QOS1, 0, 0, 0, NULL, 0 }

#define MQTTMessage_Qos0_initializer { QOS0, 0, 0, 0, NULL, 0 }

typedef struct MessageData
{
    MQTTMessage* message;
    MQTTString* topicName;
} MessageData;

typedef int (*messageHandler) (void * pContext, MessageData*); 
typedef struct MessageHandler
{
    char topicFilter[JIOT_TOPIC_FILTER_LEN];
    int Qos ;      
    messageHandler fp ;
}MessageHandler;

typedef struct SUBSCRIBE_INFO
{
    int             type;                               //sub消息类型（1:sub or 0:unsub）
    unsigned int    msgId;                              //sub消息的报文标识符
    Timer           subTime;                            //sub消息时间
	int             count;          					//sub消息重订阅次数  
    MessageHandler  handler;                            //订阅消息处理函数
	
}S_SUBSCRIBE_INFO;

typedef struct REPUBLISH_INFO
{
    Timer             pubTime;      //pub消息的时间
    long long         seqNo;        //业务层消息ID
    E_JIOT_MSG_TYPE   msgType;     //业务层消息类型，0:业务消息、1:心跳
    int               count;        //pub消息重发次数     
    unsigned int      msgId;        //pub消息的报文标识符
    int               len;          //pub消息长度
    unsigned char*    buf;          //pub消息体
}S_REPUBLISH_INFO;


typedef int SubscribeHander (void * pContext) ;                           //客户端重连成功后,注册订阅信息的函数。
typedef int ConnectedHander (void * pContext) ;                           //客户端连接成功后,注册回调函数。
typedef int ConnectFailHander (void * pContext,int errCode ) ;            //客户端连接失败后,注册回调函数。
typedef int DisconnectHander (void * pContext ) ;                         //客户端连接断开后,注册回调函数。
typedef int SubscribeFailHander (void * pContext, char *topicFilter) ;    //客户端注册订阅信息失败，注册回调函数。
typedef int PublishFailHander (void * pContext,long long seqNo) ;         //客户端发布消息失败，注册的函数。
typedef int MessageTimeoutHander (void * pContext,long long seqNo) ;      //客户端消息超时，注册的函数。


typedef struct JMQTTClient 
{    
    void *pContext;                 //MQTT客户端的上下文信息，在SDK中对应的是JClient的指针
    unsigned int next_packetid;
    
    size_t buf_size;                //发送消息的buffer大小
    size_t recvbuf_size;            //接收消息的buffer大小
    unsigned char *buf;             //发送消息的buffer
    unsigned char *recvbuf;         //接收消息的buffer
    
    Timer commandTimer ;                 //发送消息且收到ack的时间，
    Timer commandRecvTimer ;             //默认使用Qos1接收到PUBACK和心跳的PINGRESP                
    unsigned int command_timeout_ms;     //消息发送超时时间                
    int cleansession;

    MessageHandler messageHandlers[MAX_MESSAGE_HANDLERS]; 
    void (*defaultMessageHandler) (void * pContext, MessageData*);

    SubscribeHander      * subscribeHander ;        //设备重连成功后,注册订阅信息的函数。
    ConnectedHander      * connectedHander;         //设备连接成功后,注册回调函数。     
    ConnectFailHander    * connectFailHander;       //设备连接失败后,注册回调函数。  
    DisconnectHander     * disconnectHander;        //设备连接断开后,注册回调函数。   
    SubscribeFailHander  * subscribeFailHander;     //设备注册订阅信息失败，注册回调函数。
    PublishFailHander    * publishFailHander;       //设备发布消息失败，注册的函数。 
    MessageTimeoutHander * messageTimeoutHander;    //设备消息超时，注册的函数。
    
    Network* ipstack;
    JMQTTClientStatus status;           //客户端状态    

    S_JIOT_MUTEX    statusLock;         //status客户端状态锁
    S_JIOT_MUTEX    idLock;             //packetid更新锁
    S_JIOT_MUTEX    writebufLock;       //sendbuffer写锁
    S_JIOT_MUTEX    publistLock;        //publist的更新锁
    S_JIOT_MUTEX    sublistLock;        //sublist的更新锁

    S_JIOT_PTHREAD  threadRecv;         //接收消息处理线程
    S_JIOT_PTHREAD  threadRetrans;      //数据重发线程Qos>0

    bool        isRun;                      //工作线程是否运行 

    Timer       connTimer;                  //客户端连接的时间。
    int         reConnIntervalTime;         //重连时间间隔，5、10、20秒，重连3次，连接成功后时间重置。

    JList_t*     subInfoList;                //subscribe订阅消息列表，接收到对应的SUBACK或者UNSUBACK时从列表删除
    JList_t*     pubInfoList;                //publish消息列表，接收到对应的PUBACK时从列表删除

    MQTTPacket_connectData  mqttConnData;   //MQTT连接数据
    
    int         errCode ;                   //错误码
    
} JMQTTClient ;

typedef enum
{
    /*
    0x00 Connection Accepted ,Connection accepted
    0x01 Connection Refused, unacceptable protocol version, The Server does not support the level of the MQTT protocol requested by the Client
    0x02 Connection Refused, identifier rejected, The Client identifier is correct UTF-8 but not allowed by the Server
    0x03 Connection Refused, Server unavailable, The Network Connection has been made but the MQTT service is unavailable
    0x04 Connection Refused, bad user name or password, The data in the user name or password is malformed
    0x05 Connection Refused, not authorized, The Client is not authorized to connect
    */
    CONNECTION_ACCEPTED = 0,                                
    CONNECTION_REFUSED_UNACCEPTABLE_PROTOCOL_VERSION = 1,
    CONNECTION_REFUSED_IDENTIFIER_REJECTED = 2,
    CONNECTION_REFUSED_SERVER_UNAVAILABLE = 3,
    CONNECTION_REFUSED_BAD_USERDATA = 4,
    CONNECTION_REFUSED_NOT_AUTHORIZED = 5
} CONACK_CODE;


typedef struct
{
    char                   *buf;                    //MQTT发送buffer
    unsigned int           buf_size;                //MQTT发送buffer的长度,
    char                   *recvbuf;                //MQTT接收buffer
    unsigned int           recvbuf_size;            //MQTT接收buffer的长度
   
} IOT_CLIENT_INIT_PARAMS;


#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif