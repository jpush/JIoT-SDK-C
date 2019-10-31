#ifndef JIOT_CLIENT_H
#define JIOT_CLIENT_H
/*********************************************************************************
 * 文件名称: jiot_client.h
 * 作   者: Ice
 * 版   本:
 **********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define SDK_VERSION "v1.0.7"

#ifndef SDK_PLATFORM
#define SDK_PLATFORM "unknown"
#endif

#define NO_LOG_LEVL         0       //不输出日志
#define ERROR_LOG_LEVL      1       //输出错误日志
#define INFO_LOG_LEVL       2       //输出运行时日志和错误日志
#define DEBUG_LOG_LEVL      3       //输出调试日志、运行时日志和错误日志

typedef void*  JHandle ;

typedef enum
{
    CLIENT_INVALID = 0,                    //client无效状态
    CLIENT_INITIALIZED = 1,                //client等待调用连接的状态（初始化完成后和JClient主动断开后状态）
    CLIENT_CONNECTING =2,                  //client第一次开始连接状态
    CLIENT_CONNECTED = 3,                  //client已经连接状态
    CLIENT_DISCONNECTING = 4,              //client连接关闭中状态,主动调用sdk的disconnect
    CLIENT_DISCONNECTED = 5,               //client连接丢失状态，等待下一次重连。
    CLIENT_RECONNECTING = 6                //client正在重连状态
    
} JClientStatus;

//设备三元组
typedef struct DeviceInfo
{
    char szProductKey[32];
    char szDeviceName[32];
    char szDeviceSecret[32];
}DeviceInfo;

//调用返回结构体
typedef struct JiotResult
{
    int errCode;
    long long seqNo;
}JiotResult;

//属性
typedef struct Property
{
  char * name;
  char * value;
  long long  time  ;
}Property;

//事件
typedef struct Event
{
  char * name ;
  char * content ;
  long long   time ;
}Event;  

typedef struct PropertyReportReq
{
  long long  seq_no;
  unsigned long version ;
  int property_size;
  Property * pProperty;
}PropertyReportReq;  

typedef struct PropertyReportRsp
{
  long long  seq_no ;
  int code ;  
  unsigned long version ;
  int property_size;
  Property * pProperty;
}PropertyReportRsp; 
    
typedef struct EventReportReq
{
  long long  seq_no ;
  Event * pEvent;
}EventReportReq; 

typedef struct EventReportRsp
{
  long long  seq_no ;
  int code ;  
}EventReportRsp;   

typedef struct PropertySetReq
{
  long long  seq_no ;
  long long  time ;  
  unsigned long version ;
  int property_size;
  Property * pProperty;
}PropertySetReq;

typedef struct PropertySetRsp
{
  long long  seq_no ;
  int code ;  
  int property_size;
  Property * pProperty; 
}PropertySetRsp;

typedef struct MsgDeliverReq
{
  long long  seq_no ;
  char* message ;
  long long  time ;  
}MsgDeliverReq;

typedef struct MsgDeliverRsp
{
  long long  seq_no ;
  int code ;   
}MsgDeliverRsp;

typedef struct VersionReportReq
{
  long long  seq_no ; 
  char* app_ver ; 
}VersionReportReq;

typedef struct VersionReportRsp
{
  long long  seq_no ;  
  int code ;    
}VersionReportRsp;

// ota
typedef struct OtaUpgradeInformReq
{
  long long  seq_no;
  unsigned int size;
  char* url;
  char* md5;
  char* app_ver;
  long task_id;
}OtaUpgradeInformReq;
typedef struct OtaUpgradeInformRsp
{
  long long  seq_no ;
  int code ;
}OtaUpgradeInformRsp;

typedef struct OtaStatusReportReq
{
  long long  seq_no ;
  int  step ;
  char* desc ;
  long task_id;
}OtaStatusReportReq;
typedef struct OtaStatusReportRsp
{
  long long  seq_no ;
  int code ;
}OtaStatusReportRsp;



/**
 * 函数说明: 处理服务端返回JIOT客户端上报设备属性请求回复
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          pResp :结构体指针
 *          errcode :错误码
 */
typedef int jiotPropertyReportRsp(void* pContext,JHandle handle,const PropertyReportRsp * Rsp,int errcode);

/**
 * 函数说明: 处理服务端返回JIOT客户端上报设备事件请求回复
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          pResp :接收回复消息的结构体指针
 *          errcode :错误码
 */
typedef int jiotEventReportRsp(void* pContext,JHandle handle,const EventReportRsp * Rsp,int errcode);

/**
 * 函数说明: 处理服务端返回JIOT客户端上报设备版本请求回复
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          pResp :接收回复消息的结构体指针
 *          errcode :错误码
 */
typedef int jiotVersionReportRsp(void* pContext,JHandle handle,const VersionReportRsp * Rsp,int errcode);

/**
 * 函数说明: 服务端下发给JIOT客户端设备属性设置请求。
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          Req :接收属性设置消息的结构体指针
 *          errcode :错误码
 */
typedef int jiotPropertySetReq(void* pContext,JHandle handle,PropertySetReq *Req,int errcode);

/**
 * 函数说明: 服务端下发给JIOT客户端消息下发请求。
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          Req :接收属性设置消息的结构体指针
 *          errcode :错误码
 */
typedef int jiotMsgDeliverReq(void* pContext,JHandle handle,MsgDeliverReq *Req,int errcode);

/**
 * 函数说明: 服务端下发给JIOT客户下发OTA升级请求。
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          Req :OTA升级信息的结构体指针
 *          errcode :错误码
 */
typedef int jiotOtaUpgradeInformReq(void* pContext,JHandle handle,OtaUpgradeInformReq *Req,int errcode);

/**
 * 函数说明: JIOT客户端上报OTA升级状态回复
 * 参数:    pContext:用户注册的上下文信息
 *          handle  JIOT客户端句柄
 *          pResp :接收回复消息的结构体指针
 *          errcode :错误码
 */
typedef int jiotOtaStatusReportRsp(void* pContext,JHandle handle,const OtaStatusReportRsp * Rsp,int errcode);

//JClient处理接收消息的回调函数
typedef struct JClientMessageCallback
{
    jiotPropertyReportRsp   *_cbPropertyReportRsp;    
    jiotEventReportRsp      *_cbEventReportRsp;       
    jiotVersionReportRsp    *_cbVersionReportRsp;
    jiotPropertySetReq      *_cbPropertySetReq;      
    jiotMsgDeliverReq       *_cbMsgDeliverReq;
    jiotOtaUpgradeInformReq *_cbOtaUpgradeInformReq;
    jiotOtaStatusReportRsp  *_cbOtaStatusReportRsp;
} JClientMessageCallback;

/**
 * 函数说明: 客户端mqtt连接成功。
 * 参数:    pContext:用户注册的上下文信息
 */
typedef int jiotConnectedHandle(void* pContext);

/**
 * 函数说明: 客户端mqtt连接失败。
 * 参数:    pContext:用户注册的上下文信息
 *          errCode JIOT客户端异常错误码
 */
typedef int jiotConnectFailHandle(void* pContext,int errCode);

/**
 * 函数说明: 客户端mqtt连接中断，在此之前连接成功过。
 * 参数:    pContext:用户注册的上下文信息
 *          errCode JIOT客户端异常错误码
 */
typedef int jiotDisconnectHandle(void* pContext);

/**
 * 函数说明: 客户端subscirbe失败，ACL不通过。
 * 参数:    pContext: 用户注册的上下文信息
 *          TopicFilter:  JIOT订阅的topic
 */
typedef int jiotSubscribeFailHandle(void* pContext,char * TopicFilter);

/**
 * 函数说明: 客户端publish失败，ACL不通过
 * 参数:    pContext:用户注册的上下文信息
 *          seqNo :  消息序号
 */
typedef int jiotPublishFailHandle(void* pContext,long long seqNo);

/**
 * 函数说明: 客户端消息发送超时。
 * 参数:    pContext:用户注册的上下文信息
 *          seqNo :  消息序号
 */
typedef int jiotMessageTimeoutHandle(void* pContext,long long seqNo);

//JClient的回调函数
typedef struct JClientHandleCallback
{
    jiotConnectedHandle       *_cbConnectedHandle;        //mqtt连接成功
    jiotConnectFailHandle     *_cbConnectFailHandle;      //mqtt连接失败
    jiotDisconnectHandle      *_cbDisconnectHandle;       //mqtt连接中断
    jiotSubscribeFailHandle   *_cbSubscribeFailHandle;    //消息subscirbe失败，ACL不通过
    jiotPublishFailHandle     *_cbPublishFailHandle;      //消息publish失败，ACL不通过。由于MQTT3.1.1版本不支持，所以SDK暂时不支持,后续再开放
    jiotMessageTimeoutHandle  *_cbMessageTimeoutHandle;   //消息发送超时
} JClientHandleCallback;

/**
 * 函数说明: 设置日志级别,
 * 参数:    int logLevl 日志级别，定义如下
                NO_LOG_LEVL         0       //不输出日志
                ERROR_LOG_LEVL      1       //输出错误日志
                INFO_LOG_LEVL       2       //输出运行时日志和错误日志
                DEBUG_LOG_LEVL      3       //输出调试日志、运行时日志和错误日志
 * 返回值：  
 */
void jiotSetLogLevel(int logLevl);

/**
 * 函数说明: 创建JIOT客户端的,
 * 返回值：  创建成功返回客户端句柄，失败返回NULL
 */
JHandle jiotInit();

/**
 * 函数说明: 注册JIOT客户端的回调函数
 * 参数:    handle  JIOT客户端句柄
 *          pContext:设备的上下文信息
 *          cb: 客户端处理接收报文回调函数
 *          handleCb: 客户端处理接收状态回调函数
 * 返回值：  
 */
void jiotRegister(JHandle handle,void* pContext, JClientMessageCallback *cb,JClientHandleCallback *handleCb);

/**
 * 函数说明: 注销JIOT客户端的回调函数
 * 参数:    handle  JIOT客户端句柄
 * 返回值：  
 */
void jiotUnRegister(JHandle handle);

/**
 * 函数说明: 通过SIS获取MQTT的地址和端口，并连接MQTT服务端。
 * 参数:    handle  JIOT客户端句柄
 *          pDev :设备三元组
 * 返回值：  成功返回JIOT_SUCCESS，失败返回对应错误码
 */
int jiotConn(JHandle handle,const DeviceInfo *pDev);

/**
 * 函数说明: 断开连接MQTT服务端。
 * 参数:    pDev :设备三元组
 * 返回值：  成功返回true，失败返回false
 */
void jiotDisConn(JHandle handle);

/**
 * 函数说明: 销毁JIOT客户端
 * 参数:    handle  JIOT客户端句柄
 * 返回值：  
 */
void jiotRelease(JHandle * handle);

/**
 * 函数说明: 获取JIOT客户端状态
 * 参数:    handle  JIOT客户端句柄
 * 返回值：  JClientStatus
 */
JClientStatus jiotGetConnStatus(JHandle  handle);

/**
 * 函数说明: JIOT客户端上报设备属性请求。
 * 参数:    handle  JIOT客户端句柄
 *          pReq :上报消息的结构体指针
 * 返回值：  
 */
JiotResult jiotPropertyReportReq(JHandle handle,const PropertyReportReq *pReq);

/**
 * 函数说明: JIOT客户端上报设备事件请求。
 * 参数:    handle  JIOT客户端句柄
 *          pReq :上报消息的结构体指针
 * 返回值：  
 */
JiotResult jiotEventReportReq(JHandle handle,const EventReportReq *pReq);

/**
 * 函数说明: JIOT客户端上报设备版本请求。
 * 参数:    handle  JIOT客户端句柄
 *          pReq :上报消息的结构体指针
 * 返回值：  
 */
JiotResult jiotVersionReportReq(JHandle handle,const VersionReportReq * pReq);

/**
 * 函数说明: JIOT客户端上报设备OTA升级状态请求。
 * 参数:    handle  JIOT客户端句柄
 *          pReq :上报消息的结构体指针
 * 返回值：
 */
JiotResult jiotOtaStatusReportReq(JHandle handle,const OtaStatusReportReq * pReq);

#endif


