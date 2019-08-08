#include "jiot_client.h"
#include "jiot_mqtt_client.h"
#include "jiot_cJSON.h"
#include "jiot_std.h"
#include "jiot_timer.h"
#include "jiot_pthread.h"
#include "jiot_log.h"
#include "jiot_sis_client.h"
#include "jiot_common.h"
#include "jiot_code.h"
#include "jiot_timer.h"


#ifndef SDK_BUILDID
#define SDK_BUILDID "0"
#endif


typedef struct CONN_INFO
{
    char szHost[32];
    char szPort[8];
    char *szPubkey;
    char szClientID[64];  
    char szUserName[32];
    char szPassword[32];  
}CONN_INFO;

typedef struct JClient
{
    char szProductKey[32];
    char szDeviceName[32];
    char szDeviceSecret[32];  
    JMQTTClient *   pJMqttCli;
    void *          pContext;
    JClientMessageCallback cb;          //消息回调函数
    JClientHandleCallback  handleCb;    //MQTT回调函数
    CONN_INFO       conn;
    JClientStatus   status;
    S_JIOT_PTHREAD  threadKeepalive;   //保活线程
    int             isRun;
    Timer           connTimer;
    int             connTimeInterval;  //重连次数 ，30 60 120 240 270
    long long       sTime ;            //时间UTC ，秒数
    int             index;                //seqNo的自序号
    S_JIOT_MUTEX    seqNoLock;         //seqNo更新锁
    Timer           pingTimer;         //发送业务心跳的时间。
}JClient ;


#define SHORT_TOPIC_NAME_LEN    128 
#define PRODUCT_KEY_MAX_LEN     24
#define DEVICE_NAME_MAX_LEN     24
#define DEVICE_SECRET_MAX_LEN   24 
#define PROPERTY_NAME_MAX_LEN   32
#define PROPERTY_VALUE_MAX_LEN  1024
#define EVENT_NAME_MAX_LEN      32
#define EVENT_CONTENT_MAX_LEN   2048
#define APP_VERSION_MAX_LEN     24

#define CODE_MAX 999
#define CODE_OK  0 

#define CONNECT_MAX_TIME_INTERVAL  270 //4分半
#define PING_TIME_INTERVAL     270 //4分半

#define SEQ_INDEX_MAX 9999

const int QOS = QOS1 ;

static int g_errcode = JIOT_SUCCESS;
#define ASSERT_JSONOBJ(X) \
g_errcode  = JIOT_SUCCESS;\
if (NULL == X)\
{\
    g_errcode = JIOT_ERR_JCLI_JSON_PARSE_ERR; \
    break;\
}\

//发布上行
#define  JMQTT_TOPIC_PROPERTY_SET_RSP            "pub/sys/%s/%s/property/set_resp"
#define  JMQTT_TOPIC_MSG_DELIVER_RSP             "pub/sys/%s/%s/msg/deliver_resp"
#define  JMQTT_TOPIC_PROPERTY_REPORT_REQ         "pub/sys/%s/%s/property/report"
#define  JMQTT_TOPIC_EVENT_REPORT_REQ            "pub/sys/%s/%s/event/report"
#define  JMQTT_TOPIC_VERSION_REPORT_REQ          "pub/sys/%s/%s/version/report"
#define  JMQTT_TOPIC_IOT_PING_REQ                "pub/sys/%s/%s/iotping/req"
#define  JMQTT_TOPIC_DEF_UP                      "pub/def/%s/%s/%s"            //上行

#define  JMQTT_SUBTOPIC_SYS_4_DEV                    "sub/sys/%s/%s/+/+"        
#define  JMQTT_SUBTOPIC_SYS_4_PRO                    "sub/sys/%s/*/msg/deliver"         
#define  JMQTT_SUBTOPIC_DEF_4_DEV                    "sub/def/%s/%s/#"          
#define  JMQTT_SUBTOPIC_DEF_4_PRO                    "sub/def/%s/*/#"           

#define  JMQTT_SHORTTOPIC_PROPERTY_SET_REQ            "property/set"
#define  JMQTT_SHORTTOPIC_MSG_DELIVER_REQ             "msg/deliver"
#define  JMQTT_SHORTTOPIC_PROPERTY_REPORT_RSP         "property/report_resp"
#define  JMQTT_SHORTTOPIC_EVENT_REPORT_RSP            "event/report_resp"
#define  JMQTT_SHORTTOPIC_IOT_PING_RSP                "iotping/resp"

#define  JMQTT_SHORTTOPIC_VERSION_REPORT_RSP          "version/report_resp"

int _jiotPingReq(JClient * pClient);
int _jiotPingRsp( void * pContext , MessageData * msg);

int _jiotSysMsg( void * pContext , MessageData * msg);

int _jiotPropertyReportRsp( void * pContext , MessageData * msg);
int _jiotEventReportRsp( void * pContext , MessageData * msg);
int _jiotPropertySetReq( void * pContext , MessageData * msg);
int _jiotMsgDeliverReq( void * pContext , MessageData * msg);
int _jiotVersionReportRsp( void * pContext , MessageData * msg);


int _jiotSubscribe(void * pContext);
int _jiotConnectedHandle( void * pContext );
int _jiotConnectFailHandle( void * pContext ,int errCode );
int _jiotDisconnectHandle( void * pContext );
int _jiotSubscribeFailHandle( void * pContext,char* topicFilter );
int _jiotPublishFailHandle( void * pContext ,long long seqNo);
int _jiotMessageTimeoutHandle( void * pContext,long long seqNo );


int _create_thread(JClient * pClient);
int _delete_thread(JClient * pClient);

int _conn(JClient *pClient,int buf_size,char * buf,int recvbuf_size,char* recvbuf);
int _disconn(JClient *pClient);


char mqtt_ca[]="-----BEGIN CERTIFICATE-----\n\
MIIDnzCCAoegAwIBAgIJALmCYYf4e6LHMA0GCSqGSIb3DQEBCwUAMGYxCzAJBgNV\n\
BAYTAkNOMQswCQYDVQQIDAJHRDELMAkGA1UEBwwCU1oxEDAOBgNVBAoMB2ppZ3Vh\n\
bmcxDDAKBgNVBAsMA2lvdDEdMBsGCSqGSIb3DQEJARYOaW90QGppZ3VhbmcuY24w\n\
HhcNMTgxMTE5MDMyODQ1WhcNMTkxMTE5MDMyODQ1WjBmMQswCQYDVQQGEwJDTjEL\n\
MAkGA1UECAwCR0QxCzAJBgNVBAcMAlNaMRAwDgYDVQQKDAdqaWd1YW5nMQwwCgYD\n\
VQQLDANpb3QxHTAbBgkqhkiG9w0BCQEWDmlvdEBqaWd1YW5nLmNuMIIBIjANBgkq\n\
hkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1Rxtn9003pFs2nLw2WqL2FYF4hgPieyr\n\
UNBdPJiRQSeAcHJaDY+2S/SGolKnNdbfh00UvIt29CEksWP/miNvOh5g1rDgzg9Y\n\
V37sJRK6Tga/xGC/VChVPs5H6XN/GzvdOpd1PyuyhGoKmdWQ1/HWfM4aSqg4moRL\n\
atgeg285PlQIXDDgUR3gou0PRFXHvY6N4yfrsJwfYqyyNDnSmPS5MueWgbF1178o\n\
c25x213kFMHDGg2aHY4S7iCn4whygCkzeF5TgPaXSPtxa+5diTrBF50XLu1GuXTy\n\
8EyA7m9h7vCbODI/tPRBNjIj+act77lv2bX2CBo+tTr9C9gL9ism8QIDAQABo1Aw\n\
TjAdBgNVHQ4EFgQUFuZxKAHqdlWCjOY0up+Mph6xyaQwHwYDVR0jBBgwFoAUFuZx\n\
KAHqdlWCjOY0up+Mph6xyaQwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOC\n\
AQEAChSDgzJ/z1U9x++zaNxZIr5R7Aimq2SxOGLWKhk3y7zq7/6kfRJzj62aS5l0\n\
jQJg1HAqEaliFWDmTqGskbjk0+hqohpV2iEc/pJs6cMA0DMnetcG9B1phx1zIhWH\n\
IDXATCpetnPBhpJAHLkhmFAFlfBGUna1wpuyVN2HHHKlMCQKsSCkCSZ8E+ycXY0r\n\
9HWBHvBLlsxI7AML6GftmM/LWOxdGZeUVHhHL12FrVYzPEFsNM8nZo//mFJQCgqh\n\
pp172G9NG4AwbKFoNfSp9kLf6+VwBJxHn4KthQGA8E5dY83XkCw7sWA6leabXWLH\n\
rfTVCiIhoRoFJzQSkonJFokT4g==\n\
-----END CERTIFICATE-----";

void jiotSetLogLevel(int logLevl)
{
    g_logLevl = logLevl;
}

JHandle jiotInit()
{
	INFO_LOG("SDK_VERSION[%s] SDK_BUILDID[%s]\n", SDK_VERSION, SDK_BUILDID);

    JClient *pClient = jiot_malloc(sizeof(JClient));

    if (NULL == pClient)
    {
        return NULL;
    }

    pClient->pJMqttCli = jiot_malloc(sizeof(JMQTTClient));
    if (pClient->pJMqttCli == NULL )
    {
        jiot_free(pClient);
        return NULL;
    }

    if(JIOT_SUCCESS != jiot_mqtt_init(pClient->pJMqttCli,
                                        pClient,
                                        _jiotSubscribe,
                                        _jiotConnectedHandle,
                                        _jiotConnectFailHandle ,
                                        _jiotDisconnectHandle,
                                        _jiotSubscribeFailHandle,
                                        _jiotPublishFailHandle,
                                        _jiotMessageTimeoutHandle))
    {
        jiot_free(pClient);
        jiot_free(pClient->pJMqttCli);
        return NULL;
    }

    jiot_signal_SIGPIPE();

    pClient->pContext = NULL;
    pClient->status = CLIENT_INVALID;
    pClient->isRun = 0;
    pClient->sTime = 0; 
    memset(&pClient->threadKeepalive,0x0,sizeof(S_JIOT_PTHREAD));
    InitTimer(&pClient->connTimer);
    InitTimer(&pClient->pingTimer);
    pClient->connTimeInterval = 0;

    jiot_mutex_init(&pClient->seqNoLock);
    
    pClient->cb._cbPropertyReportRsp    = NULL;
    pClient->cb._cbEventReportRsp       = NULL;
    pClient->cb._cbVersionReportRsp     = NULL;
    pClient->cb._cbPropertySetReq       = NULL;
    pClient->cb._cbMsgDeliverReq        = NULL;

    pClient->handleCb._cbConnectedHandle         = NULL ;
    pClient->handleCb._cbConnectFailHandle       = NULL ;
    pClient->handleCb._cbDisconnectHandle        = NULL ;
    pClient->handleCb._cbSubscribeFailHandle     = NULL ;
    pClient->handleCb._cbPublishFailHandle       = NULL ;
    pClient->handleCb._cbMessageTimeoutHandle    = NULL ;


    pClient->isRun = 1;
    pClient->status = CLIENT_INITIALIZED;

    _create_thread(pClient);

    return (void*)pClient;
}

void jiotRegister(JHandle handle,void* pContext, JClientMessageCallback *cb,JClientHandleCallback *handleCb)
{
    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        return ;
    }

    pClient->pContext = pContext;

    pClient->cb._cbPropertyReportRsp    = cb->_cbPropertyReportRsp;
    pClient->cb._cbEventReportRsp       = cb->_cbEventReportRsp;
    pClient->cb._cbVersionReportRsp     = cb->_cbVersionReportRsp;
    pClient->cb._cbPropertySetReq       = cb->_cbPropertySetReq;
    pClient->cb._cbMsgDeliverReq        = cb->_cbMsgDeliverReq;
    
    pClient->handleCb._cbConnectedHandle         = handleCb->_cbConnectedHandle;
    pClient->handleCb._cbConnectFailHandle       = handleCb->_cbConnectFailHandle;
    pClient->handleCb._cbDisconnectHandle        = handleCb->_cbDisconnectHandle;
    pClient->handleCb._cbSubscribeFailHandle     = handleCb->_cbSubscribeFailHandle;
    pClient->handleCb._cbPublishFailHandle       = handleCb->_cbPublishFailHandle;
    pClient->handleCb._cbMessageTimeoutHandle    = handleCb->_cbMessageTimeoutHandle;

    return ;
}

void jiotUnRegister(JHandle handle)
{
    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        return ;
    }

    pClient->pContext = NULL;

    pClient->cb._cbPropertyReportRsp    = NULL;
    pClient->cb._cbEventReportRsp       = NULL;
    pClient->cb._cbVersionReportRsp     = NULL;
    pClient->cb._cbPropertySetReq       = NULL;
    pClient->cb._cbMsgDeliverReq        = NULL;

    pClient->handleCb._cbConnectedHandle         = NULL;
    pClient->handleCb._cbConnectFailHandle       = NULL;
    pClient->handleCb._cbDisconnectHandle        = NULL;
    pClient->handleCb._cbSubscribeFailHandle     = NULL;
    pClient->handleCb._cbPublishFailHandle       = NULL;
    pClient->handleCb._cbMessageTimeoutHandle    = NULL;

    return ;
}

int jiotConn(JHandle handle ,const DeviceInfo *pDev)
{
    JClient *pClient  = (JClient *)handle; 

    if(pClient == NULL)
    {
        return JIOT_ERR_JCLI_ERR;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli== NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }

    if (strlen(pDev->szProductKey) > PRODUCT_KEY_MAX_LEN)
    {
    	ERROR_LOG("szProductKey length to long");
        return JIOT_ERR_PRODUCTKEY_OVERLONG;
    }

    if (strlen(pDev->szDeviceName) > DEVICE_NAME_MAX_LEN)
    {
    	ERROR_LOG("szDeviceName length to long");
        return JIOT_ERR_DEVICENAME_OVERLONG;
    }
    
    if (strlen(pDev->szDeviceSecret) > DEVICE_SECRET_MAX_LEN)
    {
    	ERROR_LOG("szDeviceSecret length to long");
        return JIOT_ERR_DEVICESECRET_OVERLONG;
    }

    if(pClient->status != CLIENT_INITIALIZED)
    {
        ERROR_LOG("JClient  status is not INITIALIZED ");
        return JIOT_ERR_MQTT_STATE_ERROR;
    }
    strcpy(pClient->szProductKey,pDev->szProductKey);
    strcpy(pClient->szDeviceName,pDev->szDeviceName);
    strcpy(pClient->szDeviceSecret,pDev->szDeviceSecret);

    pClient->status = CLIENT_CONNECTING;
    NowTimer(&pClient->connTimer);

    return JIOT_SUCCESS;
}

void jiotDisConn(JHandle handle)
{
    ENTRY;

    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        return ;
    }

    JMQTTClient * pJMqttCli = pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return ;
    }

    if((pClient->status == CLIENT_INVALID)||(pClient->status == CLIENT_INITIALIZED)||(pClient->status == CLIENT_DISCONNECTING))
    {
        DEBUG_LOG("JClient status is not CONNECTED ");
        return;
    }

    pClient->status = CLIENT_DISCONNECTING;

    int count = 0 ;
    while(true)
    {        
        if(pClient->status == CLIENT_INITIALIZED)
        {
            break ;
        }

        count ++ ;
        if (count > 2000)
        {
            //等待时间最长20秒
            break;
        }
        jiot_sleep(10);
    }
    
    return;
}

void jiotRelease(JHandle * handle)
{
    ENTRY;

    JClient *pClient  = (JClient *)*handle;
    if(pClient == NULL)
    {
        ERROR_LOG("JClient  is NULL");
        return ;
    }

    jiotDisConn(pClient);

    jiotUnRegister(pClient);

    jiot_mutex_destory(&pClient->seqNoLock);

    if(pClient->pJMqttCli != NULL)
    {
        jiot_mqtt_release(pClient->pJMqttCli);

    }

    pClient->isRun = 0;

    _delete_thread(pClient);

    if(pClient->pJMqttCli != NULL)
    {
        jiot_free(pClient->pJMqttCli);
        pClient->pJMqttCli = NULL;
    }

    if(pClient != NULL)
    {
        jiot_free(pClient);
        *handle = NULL;
    }

    return ;
}

JClientStatus jiotGetConnStatus(JHandle  handle)
{
    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        ERROR_LOG("JClient is NULL");
        return CLIENT_INVALID;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        ERROR_LOG("pJMqttCli is NULL");
        return CLIENT_INVALID;
    }

    JMQTTClientStatus mqttClientState = jiot_mqtt_get_client_state(pJMqttCli) ;
    if (CLIENT_STATE_DISCONNECTED_RECONNECTING == mqttClientState)
    {
        pClient->status = CLIENT_RECONNECTING;
    }

    return pClient->status;
}

long long jiotNextSeqNo(JHandle handle)
{
    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        return -1;
    }
    long long seqNo = 0;
    
    jiot_mutex_lock(&pClient->seqNoLock);
    
    long long time = jiot_timer_now();
    if(pClient->sTime != time)
    {
        pClient->index = 1;
        pClient->sTime = time;

        char buf[24] = {0};
        jiot_timer_s2str((UINT32)(pClient->sTime),buf);
        seqNo =  atoll(buf)*10000+pClient->index;
    }
    else
    {
        if (pClient->index > SEQ_INDEX_MAX)
        {
            pClient->index = 1;
        }
        else
        {
            pClient->index++;
        }

        char buf[24] = {0};
        jiot_timer_s2str(pClient->sTime,buf);
        seqNo =  atoll(buf)*10000+pClient->index;
    }
    
    jiot_mutex_unlock(&pClient->seqNoLock);

    return seqNo;
}

int jiotGetErrCode(JHandle  handle)
{
    JClient *pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        return JIOT_ERR_JCLI_ERR;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }

    return pClient->pJMqttCli->errCode;
}

JiotResult jiotPropertyReportReq(JHandle handle,const PropertyReportReq *pReq)
{
	char strBuf[21];
    JiotResult JRet ;
    JRet.errCode = JIOT_SUCCESS;
    JRet.seqNo = -1;

    JClient * pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        JRet.errCode =  JIOT_ERR_JCLI_ERR;
        return JRet ;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        JRet.errCode =  JIOT_ERR_MQTT_ERR;
        return JRet ;
    }

    if(pReq == NULL)
    {
        ERROR_LOG("PropertyReportReq argument is NULL");
        JRet.errCode =  JIOT_ERR_ARGU_FORMAT_ERROR;
        return JRet ;
    }

    int nRet = JIOT_SUCCESS;
    long long seqNo = pReq->seq_no ;
    if(seqNo == 0)
    {
        seqNo = jiotNextSeqNo(handle);
    }

    cJiotJSON * root =  cJiotJSON_CreateObject();
    do
    {
        if (seqNo <=0)
        {
        	jiot_lltoa(strBuf, seqNo);
            ERROR_LOG("seq_no err [%s]", strBuf);
            nRet = JIOT_ERR_SEQNO_ERROR;
            break;
        }

        if(pReq->version<=0)
        {
            ERROR_LOG("version is NULL");
            nRet = JIOT_ERR_VERSION_FORMAT_ERROR; 
            break;
        }

        cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));
        cJiotJSON_AddItemToObject(root,"time",cJiotJSON_CreateInt64(jiot_timer_now()));

        cJiotJSON_AddItemToObject(root,"version",cJiotJSON_CreateInt64(pReq->version));

        cJiotJSON * propertyList=cJiotJSON_CreateArray();
        cJiotJSON_AddItemToObject(root,"property_list",propertyList);
        
        int i = 0;
        if (pReq->pProperty == NULL)
        {
            ERROR_LOG("property is NULL");
            nRet = JIOT_ERR_PROPERTY_FORMAT_ERROR; 
            break;
        }

        for(i=0;i<pReq->property_size;i++)
        {
            cJiotJSON * obj=cJiotJSON_CreateObject();
            cJiotJSON_AddItemToArray(propertyList,obj);
            
            if ((pReq->pProperty[i].name==NULL)||((pReq->pProperty[i].value==NULL)))
            {
                ERROR_LOG("property is NULL");
                nRet = JIOT_ERR_PROPERTY_FORMAT_ERROR; 
                break;
            }
        
            int len  = strlen(pReq->pProperty[i].name);
            if ((len > PROPERTY_NAME_MAX_LEN)||(len <= 0))
            {
                ERROR_LOG("property name format err [%s]",pReq->pProperty[i].name);
                nRet = JIOT_ERR_PROPERTY_NAME_FORMAT_ERROR; 
                break;
            }

            if (strlen(pReq->pProperty[i].value) > PROPERTY_VALUE_MAX_LEN)
            {
                ERROR_LOG("property value format err [%s]",pReq->pProperty[i].value);
                nRet = JIOT_ERR_PROPERTY_VALUE_FORMAT_ERROR; 
                break;
            }
        
            cJiotJSON_AddItemToObject(obj,"name",cJiotJSON_CreateString(pReq->pProperty[i].name));
            cJiotJSON_AddItemToObject(obj,"value",cJiotJSON_CreateString(pReq->pProperty[i].value));
            cJiotJSON_AddItemToObject(obj,"time",cJiotJSON_CreateInt64(pReq->pProperty[i].time));
        }
    }while(0);
    
    if(nRet == JIOT_SUCCESS)
    {    
        char* pPayload = cJiotJSON_PrintUnformatted(root);   
        char tempTopic[128] = {0};

        //property/report
        sprintf(tempTopic,JMQTT_TOPIC_PROPERTY_REPORT_REQ,pClient->szProductKey,pClient->szDeviceName);

        MQTTMessage message = MQTTMessage_initializer ;
        message.payload = (void*)pPayload;
        message.payloadlen = strlen(pPayload);

        nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo,E_JCLIENT_MSG_NORMAL);
        if (nRet != JIOT_SUCCESS)
        {
            ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);

        }
        DEBUG_LOG("Topic:[%s] message:[%s]",tempTopic,pPayload);

        if(pPayload != NULL)
        {
            jiot_free(pPayload);
            pPayload = NULL;
        }
    }
    cJiotJSON_Delete(root);

    JRet.errCode = nRet;
    JRet.seqNo = seqNo ;

    return JRet;
}

JiotResult jiotEventReportReq(JHandle handle,const EventReportReq *pReq)
{
	char strBuf[21];
    JiotResult JRet ;
    JRet.errCode = JIOT_SUCCESS;
    JRet.seqNo = -1;

    JClient * pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        JRet.errCode =  JIOT_ERR_JCLI_ERR;
        return JRet ;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        JRet.errCode =  JIOT_ERR_MQTT_ERR;
        return JRet ;
    }

    if(pReq == NULL)
    {
        ERROR_LOG("PropertyReportReq argument is NULL");
        JRet.errCode =  JIOT_ERR_ARGU_FORMAT_ERROR;
        return JRet ;
    }

    int nRet = JIOT_SUCCESS;
    long long  seqNo = pReq->seq_no ;
    if(seqNo == 0)
    {
        seqNo = jiotNextSeqNo(handle);
    }

    if (pReq->pEvent == NULL)
    {
        ERROR_LOG("event is NULL");
        JRet.errCode  = JIOT_ERR_EVENT_FORMAT_ERROR; 
        return JRet ;
    }

    cJiotJSON * root =  cJiotJSON_CreateObject();
    do
    {
        if (seqNo <=0)
        {
        	jiot_lltoa(strBuf, seqNo);
            ERROR_LOG("seq_no err [%s]", strBuf);
            nRet = JIOT_ERR_SEQNO_ERROR;
            break;
        }

        cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));
        cJiotJSON_AddItemToObject(root,"time",cJiotJSON_CreateInt64(jiot_timer_now()));

        cJiotJSON * eventObj=cJiotJSON_CreateObject();
        cJiotJSON_AddItemToObject(root,"event",eventObj);
     
        if ((pReq->pEvent->name==NULL)||((pReq->pEvent->content==NULL)))
        {
            ERROR_LOG("event is NULL");
            nRet = JIOT_ERR_EVENT_FORMAT_ERROR; 
            break;
        }

        int len = strlen(pReq->pEvent->name);
        if (( len > EVENT_NAME_MAX_LEN)||(len <= 0))
        {
            ERROR_LOG("event name format err [%s]",pReq->pEvent->name);
            nRet = JIOT_ERR_EVENT_NAME_FORMAT_ERROR; 
            break;
        }

        len = strlen(pReq->pEvent->content);
        if ((len > EVENT_CONTENT_MAX_LEN)||(len <= 0))
        {
            ERROR_LOG("event content format err [%s]",pReq->pEvent->content);
            nRet = JIOT_ERR_EVENT_CONTENT_FORMAT_ERROR; 
            break;
        }

        cJiotJSON_AddItemToObject(eventObj,"name",cJiotJSON_CreateString(pReq->pEvent->name));
        cJiotJSON_AddItemToObject(eventObj,"content",cJiotJSON_CreateString(pReq->pEvent->content));
        cJiotJSON_AddItemToObject(eventObj,"time",cJiotJSON_CreateInt64(pReq->pEvent->time));
    
    }while(0);
    
    if(nRet == JIOT_SUCCESS)
    {
        char* pPayload = cJiotJSON_PrintUnformatted(root);   
        char tempTopic[128] = {0};

        //property/report
        sprintf(tempTopic,JMQTT_TOPIC_EVENT_REPORT_REQ,pClient->szProductKey,pClient->szDeviceName);

        MQTTMessage message = MQTTMessage_initializer ;
        message.payload = (void*)pPayload;
        message.payloadlen = strlen(pPayload);

        nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo,E_JCLIENT_MSG_NORMAL);
        if (nRet != JIOT_SUCCESS)
        {
            ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);
        }
        DEBUG_LOG("Topic:[%s] message:[%s]",tempTopic,pPayload);
        if(pPayload != NULL)
        {
            jiot_free(pPayload);
            pPayload = NULL;
        }
    }
    cJiotJSON_Delete(root);
    
    JRet.errCode = nRet;
    JRet.seqNo = seqNo ;

    return JRet;
}

JiotResult jiotVersionReportReq(JHandle handle,const VersionReportReq * pReq)
{
	char strBuf[21] = {0};
    JiotResult JRet ;
    JRet.errCode = JIOT_SUCCESS;
    JRet.seqNo = -1;

    JClient * pClient  = (JClient *)handle;
    if(pClient == NULL)
    {
        JRet.errCode =  JIOT_ERR_JCLI_ERR;
        return JRet ;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        JRet.errCode =  JIOT_ERR_MQTT_ERR;
        return JRet ;
    }

    if(pReq == NULL)
    {
        ERROR_LOG("PropertyReportReq argument is NULL");
        JRet.errCode =  JIOT_ERR_ARGU_FORMAT_ERROR;
        return JRet ;
    }

    int nRet = JIOT_SUCCESS;

    long long  seqNo = pReq->seq_no ;
    if(seqNo == 0)
    {
        seqNo = jiotNextSeqNo(handle);
    }

    cJiotJSON * root =  cJiotJSON_CreateObject();
    do
    {
        if (seqNo <=0)
        {
        	jiot_lltoa(strBuf, seqNo);
            ERROR_LOG("seq_no err [%s]", strBuf);
            nRet = JIOT_ERR_SEQNO_ERROR;
            break;
        }

        if(pReq->app_ver==NULL)
        {
            ERROR_LOG("app_ver is NULL ");
            nRet = JIOT_ERR_VERSION_APP_VAR_FORMAT_ERROR; 
            break;
        }

        int len = strlen(pReq->app_ver) ;
        if ((len > APP_VERSION_MAX_LEN)||(len <= 0))
        {
            ERROR_LOG("app_ver format err [%s]",pReq->app_ver);
            nRet = JIOT_ERR_VERSION_APP_VAR_FORMAT_ERROR; 
            break;
        }

        cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));
        cJiotJSON_AddItemToObject(root,"app_ver",cJiotJSON_CreateString(pReq->app_ver));
        cJiotJSON_AddItemToObject(root,"sdk_ver",cJiotJSON_CreateString(SDK_VERSION));
        cJiotJSON_AddItemToObject(root,"time",cJiotJSON_CreateInt64(jiot_timer_now()));
    }while(0);
    
    if(nRet == JIOT_SUCCESS)
    {
        char* pPayload = cJiotJSON_PrintUnformatted(root) ;
        char tempTopic[128] = {0};

        //property/report
        sprintf(tempTopic,JMQTT_TOPIC_VERSION_REPORT_REQ,pClient->szProductKey,pClient->szDeviceName);

        MQTTMessage message = MQTTMessage_initializer ;
        message.payload = (void*)pPayload;
        message.payloadlen = strlen(pPayload);

        nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo,E_JCLIENT_MSG_NORMAL);
        if (nRet != JIOT_SUCCESS)
        {
            ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);
        }
        DEBUG_LOG("Topic:[%s] message:[%s]",tempTopic,pPayload);
        if(pPayload != NULL)
        {
            jiot_free(pPayload);
            pPayload = NULL;
        }
    }
    cJiotJSON_Delete(root);

    JRet.errCode = nRet;
    JRet.seqNo = seqNo ;

    return JRet;
}


int _jiotPropertySetRsp(JClient * pClient ,const PropertySetRsp * pRsp)
{
	int nRet = JIOT_SUCCESS;

    if(pClient == NULL)
    {
        nRet =  JIOT_ERR_JCLI_ERR;
        return nRet ;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        nRet =  JIOT_ERR_MQTT_ERR;
        return nRet ;
    }

    if(pRsp == NULL)
    {
        ERROR_LOG("PropertyReportReq argument is NULL");
        nRet =  JIOT_ERR_ARGU_FORMAT_ERROR;
        return nRet ;
    }

    long long  seqNo = pRsp->seq_no ;

    cJiotJSON * root =  cJiotJSON_CreateObject();
    do
    {

        cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));
        
        cJiotJSON_AddItemToObject(root,"code",cJiotJSON_CreateInt64(pRsp->code));

        cJiotJSON * propertyList=cJiotJSON_CreateArray();
        cJiotJSON_AddItemToObject(root,"property_list",propertyList);
        
        if (pRsp->pProperty == NULL)
        {
            ERROR_LOG("property is NULL");
            nRet = JIOT_ERR_PROPERTY_FORMAT_ERROR; 
            break;
        }

        int i = 0;
        for(i=0;i<pRsp->property_size;i++)
        {
            cJiotJSON * obj=cJiotJSON_CreateObject();
            cJiotJSON_AddItemToArray(propertyList,obj);

            if ((pRsp->pProperty[i].name==NULL)||((pRsp->pProperty[i].value==NULL)))
            {
                ERROR_LOG("property is NULL");
                nRet = JIOT_ERR_PROPERTY_FORMAT_ERROR; 
                break;
            }

            int len = strlen(pRsp->pProperty[i].name);
            if ((len > PROPERTY_NAME_MAX_LEN)||(len <= 0))
            {
                ERROR_LOG("property name format err [%s]",pRsp->pProperty[i].name);
                nRet = JIOT_ERR_PROPERTY_NAME_FORMAT_ERROR; 
                break;
            }

            if (strlen(pRsp->pProperty[i].value) > PROPERTY_VALUE_MAX_LEN)
            {
                ERROR_LOG("property value format err [%s]",pRsp->pProperty[i].value);
                nRet = JIOT_ERR_PROPERTY_VALUE_FORMAT_ERROR; 
                break;
            }

            cJiotJSON_AddItemToObject(obj,"name",cJiotJSON_CreateString(pRsp->pProperty[i].name));
            cJiotJSON_AddItemToObject(obj,"value",cJiotJSON_CreateString(pRsp->pProperty[i].value));
            cJiotJSON_AddItemToObject(obj,"time",cJiotJSON_CreateInt64(pRsp->pProperty[i].time));
        }
    }while(0);
    
    if(nRet == JIOT_SUCCESS)
    {

        char* pPayload = cJiotJSON_PrintUnformatted(root) ;  
        char tempTopic[128] = {0};

        //property/report
        sprintf(tempTopic,JMQTT_TOPIC_PROPERTY_SET_RSP,pClient->szProductKey,pClient->szDeviceName);

        MQTTMessage message = MQTTMessage_Qos0_initializer ;
        message.payload = (void*)pPayload;
        message.payloadlen = strlen(pPayload);

        nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo,E_JCLIENT_MSG_NORMAL);
        if (nRet != JIOT_SUCCESS)
        {
            ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);
        }
        DEBUG_LOG("Topic:[%s] message:[%s]",tempTopic,pPayload);
        if(pPayload != NULL)
        {
            jiot_free(pPayload);
            pPayload = NULL;
        }
    }
    
    cJiotJSON_Delete(root);

    return nRet;
}

int _jiotMsgDeliverRsp(JClient * pClient, MsgDeliverRsp * pRsp )
{
    int nRet = JIOT_SUCCESS;

	if(pClient == NULL)
    {
        nRet =  JIOT_ERR_JCLI_ERR;
        return nRet ;
    }
    
    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        nRet =  JIOT_ERR_MQTT_ERR;
        return nRet ;
    }


    cJiotJSON * root =  cJiotJSON_CreateObject();
    long long  seqNo = pRsp->seq_no ;
        
    cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));

    cJiotJSON_AddItemToObject(root,"code",cJiotJSON_CreateInt64(pRsp->code));
        
    char* pPayload = cJiotJSON_PrintUnformatted(root) ;
    char tempTopic[128] = {0};

    //property/report
    sprintf(tempTopic,JMQTT_TOPIC_MSG_DELIVER_RSP,pClient->szProductKey,pClient->szDeviceName);

    MQTTMessage message = MQTTMessage_Qos0_initializer ;
    message.payload = (void*)pPayload;
    message.payloadlen = strlen(pPayload);

    nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo ,E_JCLIENT_MSG_NORMAL);
    if (nRet != JIOT_SUCCESS)
    {
        ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);
    }
    DEBUG_LOG("Topic:[%s] message:[%s]",tempTopic,pPayload);
    if(pPayload != NULL)
    {
        jiot_free(pPayload);
        pPayload = NULL;
    }

    cJiotJSON_Delete(root);

    return nRet;

}

int _jiotPingReq(JClient * pClient)
{
	char strBuf[21] = {0};
    if(pClient == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pClient->pJMqttCli == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }
    
    int nRet = JIOT_SUCCESS;
    cJiotJSON * root =  cJiotJSON_CreateObject();
    long long seqNo = jiotNextSeqNo((void*)pClient);
    
    cJiotJSON_AddItemToObject(root,"seq_no",cJiotJSON_CreateInt64(seqNo));

    if(nRet == JIOT_SUCCESS)
    {    
        char* pPayload = cJiotJSON_PrintUnformatted(root);   
        char tempTopic[128] = {0};

        //iotping/req
        sprintf(tempTopic,JMQTT_TOPIC_IOT_PING_REQ,pClient->szProductKey,pClient->szDeviceName);

        MQTTMessage message = MQTTMessage_initializer ;
        message.payload = (void*)pPayload;
        message.payloadlen = strlen(pPayload);

        nRet = jiot_mqtt_publish(pJMqttCli,tempTopic,&message,seqNo,E_JCLIENT_MSG_PING); // heart beat
        if (nRet != JIOT_SUCCESS)
        {
            ERROR_LOG("publish message failed, Topic:[%s] message:[%s]",tempTopic,pPayload);
        }
        else
        {
            NowTimer(&pClient->pingTimer);
        	jiot_lltoa(strBuf, seqNo);
            DEBUG_LOG("PING REQ ,seq_no[%s]",strBuf);
        }

        if(pPayload != NULL)
        {
            jiot_free(pPayload);
            pPayload = NULL;
        }
    }
    cJiotJSON_Delete(root);

    return nRet;
}

int analysisTopic(char *Topic,int len,char* shortTopicName )
{
    //订阅设备的消息sub/sys/%p/%u/+/+
    //订阅设备所属的产品群发的消息sub/sys/%p/*/msg/deliver
    
    char* pPos=strchr(Topic,'/');
    if (pPos == NULL)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }
    pPos=strchr(pPos+1,'/');
    if (pPos == NULL)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }
    pPos=strchr(pPos+1,'/');
    if (pPos == NULL)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }
    pPos=strchr(pPos+1,'/');
    if (pPos == NULL)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }
    pPos = pPos+1;
    
    //取shortTopicName
    int nPos = pPos - Topic ;
    if (nPos >= len)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }

    int shortTopicNameLen = len - nPos;
    if(shortTopicNameLen > SHORT_TOPIC_NAME_LEN)
    {
        ERROR_LOG("topicName format is error %s",Topic);
        return JIOT_FAIL;
    }

    strncpy(shortTopicName,pPos,shortTopicNameLen);

    return JIOT_SUCCESS;
}


//sys消息回调函数
int _jiotSysMsg( void * pContext , MessageData * msg)
{
    ENTRY;
    //DEBUG_LOG("msg : %s",msg->topicName->lenstring.data);
    char szShortTopicName[SHORT_TOPIC_NAME_LEN+1]={0};

    if(analysisTopic(msg->topicName->lenstring.data,msg->topicName->lenstring.len,szShortTopicName) != JIOT_SUCCESS)
    {
        ERROR_LOG("topic : %s",msg->topicName->lenstring.data);
        return JIOT_ERR_TOPIC_FOTMAT_ERROR;
    }

    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_IOT_PING_RSP))
    {
        return _jiotPingRsp(pContext,msg);
    }
    else
    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_PROPERTY_SET_REQ))
    {
        return _jiotPropertySetReq(pContext,msg);
    }
    else
    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_MSG_DELIVER_REQ))
    {
        return _jiotMsgDeliverReq(pContext,msg);
    }
    else
    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_PROPERTY_REPORT_RSP))
    {
        return _jiotPropertyReportRsp(pContext,msg);
    }
    else
    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_EVENT_REPORT_RSP))
    {
        return _jiotEventReportRsp(pContext,msg);
    }
    else
    if (0==strcmp(szShortTopicName,JMQTT_SHORTTOPIC_VERSION_REPORT_RSP))
    {
        return _jiotVersionReportRsp(pContext,msg);
    }
    else
    {
        ERROR_LOG("topicName is error %s",msg->topicName->lenstring.data);
    }
    return JIOT_FAIL;
}


int _jiotPropertyReportRsp( void * pContext , MessageData * msg)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }
    int nRet = JIOT_SUCCESS;
    PropertyReportRsp rsp;
    memset(&rsp,0,sizeof(PropertyReportRsp));

    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);

        cJiotJSON * obj  = NULL ;
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(obj);
        rsp.seq_no = obj->valueint64;
        
        obj = cJiotJSON_GetObjectItem(root,"code");
        ASSERT_JSONOBJ(obj);
        rsp.code = obj->valueint64;
        
        obj = cJiotJSON_GetObjectItem(root,"version");
        ASSERT_JSONOBJ(obj);
        rsp.version = obj->valueint64;
        
        cJiotJSON * propertyList=cJiotJSON_GetObjectItem(root,"property_list");
        ASSERT_JSONOBJ(propertyList);
        rsp.property_size = cJiotJSON_GetArraySize(propertyList);
        rsp.pProperty = NULL;
        if (rsp.property_size>0)
        {
            
            rsp.pProperty = (Property*)jiot_malloc(sizeof(Property)*rsp.property_size);

            int i = 0;
            for(i=0;i<rsp.property_size ;i++)
            {
                obj=cJiotJSON_GetArrayItem(propertyList,i);
                ASSERT_JSONOBJ(obj);
                cJiotJSON * item = NULL;

                item=cJiotJSON_GetObjectItem(obj,"name");
                ASSERT_JSONOBJ(item);
                rsp.pProperty[i].name = item->valuestring;

                item=cJiotJSON_GetObjectItem(obj,"value");
                ASSERT_JSONOBJ(item);
                rsp.pProperty[i].value = item->valuestring;
                
                item=cJiotJSON_GetObjectItem(obj,"time");
                ASSERT_JSONOBJ(item);
                rsp.pProperty[i].time = item->valueint64;
            }
        }
    }while(0);

    
    if (pClient->cb._cbPropertyReportRsp != NULL)
    {
        pClient->cb._cbPropertyReportRsp(pClient->pContext,(void*)pClient,&rsp,g_errcode);
    }
    else
    {
        ERROR_LOG("cbPropertyReportRsp not Register");
    }
    
    if (NULL != rsp.pProperty)
    {
        jiot_free(rsp.pProperty);
    }
    
    cJiotJSON_Delete(root);

    return nRet;
}

int _jiotEventReportRsp( void * pContext , MessageData * msg)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }

    int nRet = JIOT_SUCCESS;
    EventReportRsp rsp;
    memset(&rsp,0,sizeof(EventReportRsp));

    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);
        cJiotJSON * obj  = NULL ;
        
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(obj);
        rsp.seq_no = obj->valueint64;

        obj = cJiotJSON_GetObjectItem(root,"code");
        ASSERT_JSONOBJ(obj);
        rsp.code = obj->valueint64;
    }while(0);
    
    if (pClient->cb._cbEventReportRsp != NULL)
    {
        pClient->cb._cbEventReportRsp(pClient->pContext,(void*)pClient,&rsp,g_errcode);
    }
    else
    {
        ERROR_LOG("cbEventReportRsp not Register");
    }
    
    
    cJiotJSON_Delete(root);    

    return nRet;
}

int _jiotVersionReportRsp( void * pContext , MessageData * msg)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }
    
    int nRet = JIOT_SUCCESS;
    VersionReportRsp rsp;
    memset(&rsp,0,sizeof(VersionReportRsp));

    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);
        cJiotJSON * obj  = NULL ;
        
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(root);
        rsp.seq_no = obj->valueint64;

        obj = cJiotJSON_GetObjectItem(root,"code");
        ASSERT_JSONOBJ(root);
        rsp.code = obj->valueint64;
    }while(0);

    if (pClient->cb._cbVersionReportRsp != NULL)
    {
        pClient->cb._cbVersionReportRsp(pClient->pContext,(void*)pClient,&rsp,g_errcode);
    }
    else
    {
        ERROR_LOG("cbVersionReportRsp not Register");
    }  

    cJiotJSON_Delete(root);    

    return nRet;  
}

int _jiotPropertySetReq( void * pContext , MessageData * msg)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }

    int nRet = JIOT_SUCCESS;
    PropertySetReq req;
    memset(&req,0,sizeof(PropertySetReq));


    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);
        cJiotJSON * obj  = NULL ;
        
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(obj);
        req.seq_no = obj->valueint64;

        obj = cJiotJSON_GetObjectItem(root,"time");
        ASSERT_JSONOBJ(obj);
        req.time = obj->valueint64;
        
        obj = cJiotJSON_GetObjectItem(root,"version");
        ASSERT_JSONOBJ(obj);
        req.version = obj->valueint64;

        cJiotJSON * propertyList=cJiotJSON_GetObjectItem(root,"property_list");
        ASSERT_JSONOBJ(propertyList);
        
        req.property_size = cJiotJSON_GetArraySize(propertyList);
        
        req.pProperty = NULL;
        if (req.property_size >0)
        {
            req.pProperty = (Property*)jiot_malloc(sizeof(Property)*req.property_size);

            int i = 0;
            for(i=0;i<req.property_size ;i++)
            {
                obj=cJiotJSON_GetArrayItem(propertyList,i);
                ASSERT_JSONOBJ(obj);
                cJiotJSON * item = NULL;

                item=cJiotJSON_GetObjectItem(obj,"name");
                ASSERT_JSONOBJ(item);
                req.pProperty[i].name = item->valuestring;

                item=cJiotJSON_GetObjectItem(obj,"value");
                ASSERT_JSONOBJ(item);
                req.pProperty[i].value = item->valuestring;
                
                item=cJiotJSON_GetObjectItem(obj,"time");
                ASSERT_JSONOBJ(item);
                req.pProperty[i].time = item->valueint64;
            }       
        }
    }while(0);

    PropertySetRsp rsp ;
    rsp.seq_no =  req.seq_no;
    if (g_errcode != JIOT_SUCCESS)
    {
        rsp.code = g_errcode;
        rsp.property_size= 0;
        rsp.pProperty = NULL;
    }
    else
    {
        rsp.code = 0;
        rsp.property_size= req.property_size;
        rsp.pProperty = req.pProperty;
    }

    _jiotPropertySetRsp(pClient,&rsp);

    if (pClient->cb._cbPropertySetReq != NULL)
    {
        pClient->cb._cbPropertySetReq(pClient->pContext,(void*)pClient,&req,g_errcode);
    }
    else
    {
        ERROR_LOG("cbPropertySetReq not Register");
    }  
    
    if (NULL != req.pProperty)
    {
        jiot_free(req.pProperty);
    
    }
    
    cJiotJSON_Delete(root);   

    return nRet;   
}

int _jiotMsgDeliverReq( void * pContext , MessageData * msg)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }
    int nRet = JIOT_SUCCESS;
    MsgDeliverReq req;
    memset(&req,0,sizeof(MsgDeliverReq));

    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);
        cJiotJSON * obj  = NULL ;
        
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(obj);
        req.seq_no = obj->valueint64;

        obj = cJiotJSON_GetObjectItem(root,"message");
        ASSERT_JSONOBJ(obj);
        req.message = obj->valuestring;


        obj = cJiotJSON_GetObjectItem(root,"time");
        ASSERT_JSONOBJ(obj);
        req.time = obj->valuedouble;
    }while(0);

    MsgDeliverRsp rsp  ;
    rsp.seq_no = req.seq_no ;
    if (g_errcode != JIOT_SUCCESS)
    {
        rsp.code = g_errcode;
    }
    else
    {
        rsp.code = 0;
    }
    
    _jiotMsgDeliverRsp(pClient,&rsp);

    if (pClient->cb._cbMsgDeliverReq != NULL)
    {
        pClient->cb._cbMsgDeliverReq(pClient->pContext,(void*)pClient,&req,g_errcode);
    }
    else
    {
        ERROR_LOG("cbMsgDeliverReq not Register");
    }  
       
    cJiotJSON_Delete(root);    

    return nRet;   
}

int _jiotPingRsp( void * pContext , MessageData * msg)
{
	char strBuf1[21] = {0};
	char strBuf2[21] = {0};
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }

    int nRet = JIOT_SUCCESS;
    cJiotJSON * root = NULL;
    do
    {
        root = cJiotJSON_Parse(msg->message->payload);
        ASSERT_JSONOBJ(root);
        cJiotJSON * obj  = NULL ;
        
        obj = cJiotJSON_GetObjectItem(root,"seq_no");
        ASSERT_JSONOBJ(root);
        long long seq_no = obj->valueint64;

        obj = cJiotJSON_GetObjectItem(root,"code");
        ASSERT_JSONOBJ(root);
        long long code = obj->valueint64;
        jiot_lltoa(strBuf1, seq_no);
        jiot_lltoa(strBuf2, code);
        DEBUG_LOG("PING RESP, seq_no[%s],code[%s]",strBuf1,strBuf2);

    }while(0);

    cJiotJSON_Delete(root);    
    return nRet;   
}

int _jiotConnectedHandle( void * pContext )
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }
   
    if (pClient->handleCb._cbConnectedHandle!= NULL)
    {
        pClient->handleCb._cbConnectedHandle(pClient->pContext);
    }

    return JIOT_SUCCESS;    
}


int _jiotConnectFailHandle( void * pContext ,int errCode )
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }
   
    if (pClient->handleCb._cbConnectFailHandle != NULL)
    {
        pClient->handleCb._cbConnectFailHandle(pClient->pContext,errCode);
    }

    return JIOT_SUCCESS;    
}

int _jiotDisconnectHandle( void * pContext )
{
    ENTRY;
   
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }
    
    if (pClient->handleCb._cbDisconnectHandle!= NULL)
    {
        pClient->handleCb._cbDisconnectHandle(pClient->pContext);
    }
    
    return JIOT_SUCCESS;   
}

int _jiotSubscribeFailHandle( void * pContext,char* topicFilter )
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    if (pClient->handleCb._cbSubscribeFailHandle!= NULL)
    {
        pClient->handleCb._cbSubscribeFailHandle(pClient->pContext,topicFilter);
    }

    return JIOT_SUCCESS;    
}

int _jiotPublishFailHandle( void * pContext ,long long seqNo)
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }
   
    if (pClient->handleCb._cbPublishFailHandle!= NULL)
    {
        pClient->handleCb._cbPublishFailHandle(pClient->pContext,seqNo);
    }

    return JIOT_SUCCESS;   
}

int _jiotMessageTimeoutHandle( void * pContext,long long seqNo )
{
    ENTRY;
    JClient *pClient  = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }
   
    if (pClient->handleCb._cbMessageTimeoutHandle!= NULL)
    {
        pClient->handleCb._cbMessageTimeoutHandle(pClient->pContext,seqNo);
    }

    return JIOT_SUCCESS;   
}

int _jiotSubscribe(void * pContext)
{
    ENTRY;
    int nRet = JIOT_SUCCESS;
    JClient *pClient = (JClient *)pContext;
    if(pClient == NULL)
    {
        return JIOT_ERR_JCLI_ERR;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }
    
    char tempTopic[128] = {0};
    //sys
    sprintf(tempTopic,JMQTT_SUBTOPIC_SYS_4_DEV,pClient->szProductKey,pClient->szDeviceName);
    nRet = jiot_mqtt_subscribe(pJMqttCli,tempTopic,QOS,_jiotSysMsg,true) ;
    if(nRet!= JIOT_SUCCESS)
    {
        ERROR_LOG("subscribe %s failed ",tempTopic);
        return nRet;
    }

    memset(tempTopic,0,128);
    sprintf(tempTopic,JMQTT_SUBTOPIC_SYS_4_PRO,pClient->szProductKey);
    nRet = jiot_mqtt_subscribe(pJMqttCli,tempTopic,QOS,_jiotSysMsg,true);
    if(nRet!= JIOT_SUCCESS)
    {
        ERROR_LOG("subscribe %s failed ",tempTopic);
        return nRet;
    }

   return nRet;
}

int _jiotUnsubscribe(JClient *pClient)
{
    int nRet = JIOT_SUCCESS;
    if(pClient == NULL)
    {
        return JIOT_FAIL;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_FAIL;
    }
    
    char tempTopic[128] = {0};
    
    //sys
    sprintf(tempTopic,JMQTT_SUBTOPIC_SYS_4_DEV,pClient->szProductKey,pClient->szDeviceName);
    nRet = jiot_mqtt_unsubscribe(pJMqttCli,tempTopic) ;
    if (nRet != JIOT_SUCCESS )
    {
        ERROR_LOG("unsubscribe %s failed ",tempTopic);
    }

    memset(tempTopic,0,128);
    sprintf(tempTopic,JMQTT_SUBTOPIC_SYS_4_PRO,pClient->szProductKey);
    nRet = jiot_mqtt_unsubscribe(pJMqttCli,tempTopic) ;
    if (nRet != JIOT_SUCCESS )
    {
        ERROR_LOG("unsubscribe %s failed ",tempTopic);
    }

   return nRet;
}

int _disconn(JClient *pClient)
{
    ENTRY;
    int nRet = JIOT_SUCCESS;
    if(pClient == NULL)
    {
        return JIOT_ERR_JCLI_ERR;
    }

    JMQTTClient * pJMqttCli = pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }
    
    nRet = jiot_mqtt_disconnect(pJMqttCli) ;
    if (nRet != JIOT_SUCCESS )
    {
        ERROR_LOG("release mqtt client failed ");
    }
    return JIOT_SUCCESS;
}

int _conn(JClient *pClient,int buf_size,char * buf,int recvbuf_size,char* recvbuf)
{
    int nRet = JIOT_SUCCESS;
    if(pClient == NULL)
    {
        return JIOT_ERR_JCLI_ERR;
    }

    JMQTTClient * pJMqttCli =pClient->pJMqttCli;
    if(pJMqttCli == NULL)
    {
        return JIOT_ERR_MQTT_ERR;
    }
    
    //重置时间
    NowTimer(&pClient->connTimer);
    NowTimer(&pClient->pingTimer);
    
    //连接SIS ，获取MQTT连接信息
    SisDeviceInfo dev;
    strcpy(dev.szProductKey, pClient->szProductKey);
    strcpy(dev.szDeviceName, pClient->szDeviceName);
    strcpy(dev.szDeviceSecret, pClient->szDeviceSecret);
    SisInfo sis;

    nRet = getSisInfo(&dev, &sis);

    if(JIOT_SUCCESS != nRet)
    {
        return nRet;
    }

    sprintf(pClient->conn.szClientID,"%s.%s",pClient->szProductKey,pClient->szDeviceName);
    strcpy(pClient->conn.szUserName,pClient->szProductKey);
    strcpy(pClient->conn.szPassword,pClient->szDeviceSecret);
  
    IOT_CLIENT_INIT_PARAMS  iot_params;
    iot_params.buf_size =  buf_size;
    iot_params.buf = buf;                     
    iot_params.recvbuf_size = recvbuf_size;          
    iot_params.recvbuf = recvbuf;
    //connect
    int i = 0;
    do
    {
        strcpy(pClient->conn.szHost, sis.addr[i].host);
        sprintf(pClient->conn.szPort, "%d", sis.addr[i].port);
        pClient->conn.szPubkey = mqtt_ca ;
        nRet = jiot_mqtt_conn_init(pJMqttCli,&iot_params,pClient->conn.szHost,pClient->conn.szPort,pClient->conn.szPubkey);
        if(nRet!= JIOT_SUCCESS)
        {
            ERROR_LOG("jiot_mqtt_conn_init error, retry");
            return nRet;
        }
        nRet = jiot_mqtt_connect(pJMqttCli, pClient->conn.szClientID, pClient->conn.szUserName, pClient->conn.szPassword) ;
        if(nRet!= JIOT_SUCCESS)
        {
            ERROR_LOG("jiot_mqtt_connect error, retry");
        }
    }
    while(nRet != 0 && ++i < SERVICE_ADDR_MAX);
    
    if(nRet!= JIOT_SUCCESS)
    {
        ERROR_LOG("jClient connect failed! ");
        return nRet;
    }

    DEBUG_LOG("jClient connect success!");
    return JIOT_SUCCESS;
}

void * _keepalive_thread(void * param)
{
    INFO_LOG("start the  keepalive thread!");
    JClient * pClient = (JClient * )param;

    int buf_size    =  MQTT_BUF_MAX_LEN;
    char buf[MQTT_BUF_MAX_LEN] = {0};


    int recvbuf_size= MQTT_BUF_MAX_LEN;          
    char recvbuf[MQTT_BUF_MAX_LEN] = {0};

    while(pClient->isRun)
    {
        jiot_sleep(100);

        do
        {   
            JMQTTClient * pJMqttCli =pClient->pJMqttCli;
            
            JClientStatus jCliStatus = pClient->status;
            if(jCliStatus == CLIENT_INITIALIZED )
            {
                continue;
            }
            else
            if (jCliStatus == CLIENT_DISCONNECTING)
            {
                DEBUG_LOG("Jiot Client Disconnet ");
                
                jiot_mqtt_closed(pJMqttCli);

                pClient->status = CLIENT_INITIALIZED;
                pClient->connTimeInterval = 0;
                continue;
            }
            else
            if ((jCliStatus == CLIENT_CONNECTING)||(jCliStatus == CLIENT_DISCONNECTED))
            {   
                //check reconnect time
                long long nSpend_ms = spend_ms(&pClient->connTimer) ;
                if (nSpend_ms < 0)
                {
                    NowTimer(&pClient->connTimer);
                    nSpend_ms = 0 ;
                }

                if (nSpend_ms >= pClient->connTimeInterval *1000)
                {
                    //释放之前的连接
                    _disconn(pClient);
                    //创建新连接
                   
                    if (NULL != pJMqttCli)
                    {
                        int nRet =  _conn(pClient,buf_size,buf,recvbuf_size,recvbuf);    
                        //重置时间
                        NowTimer(&pClient->connTimer);
                        NowTimer(&pClient->pingTimer);


                        if(nRet != JIOT_SUCCESS)
                        {   
                            DEBUG_LOG("Jiot Client Connet Fail ,[%d]",nRet);
                            //间隔时间为10,40,100,220,270 ......
                            if( pClient->connTimeInterval  == 0)
                            {
                                pClient->connTimeInterval  = 10 ;
                            }
                            else
                            {   
                                pClient->connTimeInterval = (pClient->connTimeInterval+20)*2-20; 
                            }
                            
                            // 最大值为270秒
                            if( pClient->connTimeInterval  > CONNECT_MAX_TIME_INTERVAL)
                            {
                                pClient->connTimeInterval  = CONNECT_MAX_TIME_INTERVAL;
                            }

                            pClient->status = CLIENT_DISCONNECTED;
                            DEBUG_LOG("Jiot Client Status DISCONNECTED");

                            jiot_mqtt_set_client_state(pJMqttCli,CLIENT_STATE_DISCONNECTED);

                            _jiotConnectFailHandle((void*)pClient,nRet);

                        }
                        else
                        {
                            pClient->connTimeInterval = 0;
                            _jiotPingReq(pClient);

                            pClient->status = CLIENT_CONNECTED;
                            _jiotConnectedHandle(pClient);

                            DEBUG_LOG("Jiot Client Connect Success ");
                            DEBUG_LOG("Jiot Client Status CONNECTED");

                        }
                    }
                }
            }
            else
            if ((jCliStatus == CLIENT_CONNECTED)||(jCliStatus == CLIENT_RECONNECTING))
            {
                if (NULL != pJMqttCli)
                {
                    JMQTTClientStatus mqttClientState  = jiot_mqtt_get_client_state(pJMqttCli);
                    //DEBUG_LOG("Mqtt Client State ,[%d]",mqttClientState);

                    if (CLIENT_STATE_CONNECTED == mqttClientState||CLIENT_STATE_DISCONNECTED_RECONNECTING == mqttClientState)
                    {
                        //连接状态下才发送业务心跳
                        if (CLIENT_STATE_CONNECTED == mqttClientState)
                        {
                            long long nSpend_ms = spend_ms(&pClient->pingTimer) ;
                            if ( nSpend_ms < 0)
                            {
                                NowTimer(&pClient->pingTimer);
                                nSpend_ms = 0 ;
                            }
                            
                            if (nSpend_ms > PING_TIME_INTERVAL *1000)
                            {
                                _jiotPingReq(pClient);
                            }
                        }

                        jiot_mqtt_keepalive(pJMqttCli);

                        JMQTTClientStatus  mqttClientNewState = jiot_mqtt_get_client_state(pJMqttCli) ;
                        if (CLIENT_STATE_DISCONNECTED_RECONNECTING == mqttClientNewState)
                        {
                            //DEBUG_LOG("Jiot Client Status RECONNECTING");
                            pClient->status = CLIENT_RECONNECTING;
                        }
                        else
                        if(CLIENT_STATE_CONNECTED == mqttClientNewState)
                        {
                            //DEBUG_LOG("Jiot Client Status CONNECTED");
                            if (mqttClientState == CLIENT_STATE_DISCONNECTED_RECONNECTING)
                            {
                                //MQTT 连接从 CLIENT_STATE_DISCONNECTED_RECONNECTING 转换为 CLIENT_STATE_CONNECTED 时需要发送业务心跳
                                _jiotPingReq(pClient);                                
                                
                                pClient->status = CLIENT_CONNECTED;
                                _jiotConnectedHandle(pClient);
                            }
                            pClient->status = CLIENT_CONNECTED;
                        }
                    }
                    else
                    {
                        DEBUG_LOG("Jiot Client Status DISCONNECTED");
                        pClient->status = CLIENT_DISCONNECTED;
                    }
                }
            }

        }while(0);
    };

    INFO_LOG("exit the  keepalive thread!");
    jiot_pthread_exit(&pClient->threadKeepalive);

    return NULL;
}

int _create_thread(JClient * pClient)
{
    ENTRY;

    int result = jiot_pthread_create(&pClient->threadKeepalive,_keepalive_thread,pClient, "keepalive_thread", TASK_STACK_SIZE+2*MQTT_BUF_MAX_LEN);
    if(0 != result)
    {
        ERROR_LOG("run jiot_pthread_create error!");
        return JIOT_FAIL;
    }
    INFO_LOG("create  keepaliveThread !");

    return JIOT_SUCCESS;
}

int _delete_thread(JClient * pClient)
{
    ENTRY;
    int result = jiot_pthread_cancel(&pClient->threadKeepalive);
	if(0 != result)
	{
		ERROR_LOG("run jiot_pthread_cancel error!");
		return JIOT_FAIL;
	}
	memset(&pClient->threadKeepalive,0x0,sizeof(S_JIOT_PTHREAD));

    INFO_LOG("delete keepaliveThread !");
    return JIOT_SUCCESS;
}
