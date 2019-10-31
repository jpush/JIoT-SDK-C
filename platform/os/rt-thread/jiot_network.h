#ifndef JIOT_PLATFORM_NETWORK_H
#define JIOT_PLATFORM_NETWORK_H

#include "jiot_std.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "jiot_err.h"
#include <netdb.h>


#if defined(__cplusplus)
 extern "C" {
#endif

typedef enum NET_TYPE
{
    NET_UNSPEC  = 0,
    NET_IP4     = 1,
    NET_IP6     = 2,
}E_NET_TYPE;

typedef enum TRANS_TYPE
{
    TRANS_RECV = 0,
    TRANS_SEND = 1,
}E_TRANS_TYPE;

typedef enum TRANS_FLAGS
{
    TRANS_FLAGS_DEFAULT = 0, //默认
    TRANS_FLAGS_NOBLOCK = 1, //非阻塞调用
}E_TRANS_FLAGS;


/***********************************************************
* 函数名称: jiot_network_create
* 描       述:  创建socket会话
* 输入参数:      INT32 *pfd         socket句柄指针      
                const char* host   目的地址或者是域名 
                const char* port   端口 
                E_NET_TYPE type    网络类型 
* 输出参数:      INT32 *pfd         socket句柄指针  
* 返 回  值: 成功返回            JIOT_SUCCESS
            失败返回            JIOT_FAIL
            域名解析错误返回     JIOT_ERR_NET_UNKNOWN_HOST  
            网络类型错误返回     JIOT_ERR_NET_HOST_NET_TYPE
            socket创建失败返回   JIOT_ERR_NET_SOCKET_CREATE
            服务端连接失败返回    JIOT_ERR_NET_SOCKET_CONN            
************************************************************/
INT32 jiot_network_create(INT32 *pfd, const char* host,const char* port, E_NET_TYPE type);

/***********************************************************
* 函数名称: jiot_network_send
* 描       述: 发送报文
* 输入参数:      INT32 fd       socket句柄
                void *buf      发送buffer
                INT32 nbytes   字节数
* 输出参数:      
* 返 回  值: 成功返回              发送的字节数
            失败返回              JIOT_FAIL
            网络出现需要重试返回    JIOT_ERR_NET_RETRY
************************************************************/
INT32 jiot_network_send(INT32 fd, void *buf, INT32 nbytes);

/***********************************************************
* 函数名称: jiot_network_recv
* 描       述: 接收报文
* 输入参数:      INT32 fd               socket句柄
                void *buf              接收buffer
                INT32 nbytes           期望接收字节数
                E_TRANS_FLAGS flags    接收模式
* 输出参数:      
* 返 回  值: 成功返回              接收的字节数
            失败返回              JIOT_FAIL
            网络出现需要重试返回    JIOT_ERR_NET_RETRY
************************************************************/
INT32 jiot_network_recv(INT32 fd, void *buf, INT32 nbytes, E_TRANS_FLAGS flags);

/***********************************************************
* 函数名称: jiot_network_select
* 描       述: 监听句柄的读写时间
* 输入参数:      INT32 fd           socket句柄
                E_TRANS_TYPE type  监听事件类型
                INT32 timeoutMs    监听超时的时间，单位毫秒
* 输出参数:      
* 返 回  值: 成功返回            JIOT_SUCCESS
            失败返回            JIOT_FAIL
            事件超时返回         JIOT_ERR_NET_SELECT_TIMEOUT
            句柄出现需要重试返回  JIOT_ERR_NET_RETRY
************************************************************/
INT32 jiot_network_select(INT32 fd,E_TRANS_TYPE type,INT32 timeoutMs);

/***********************************************************
* 函数名称: jiot_network_settimeout
* 描       述: 设置socket超时参数
* 输入参数:      INT32 fd            socket句柄
                E_TRANS_TYPE type   事件类型
                INT32 timeoutMs     超时的时间，单位毫秒                
* 输出参数:      
* 返 回  值: 成功返回         JIOT_SUCCESS
            失败返回         JIOT_FAIL
            设置参数超市返回  JIOT_ERR_NET_SETOPT_TIMEOUT
************************************************************/
INT32 jiot_network_settimeout(INT32 fd,E_TRANS_TYPE type,INT32 timeoutMs);

/***********************************************************
* 函数名称: jiot_network_set_nonblock
* 描       述: 设置当前socket为非阻塞
* 输入参数:      INT32 fd socket句柄
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_network_set_nonblock(INT32 fd);

/***********************************************************
* 函数名称: jiot_network_set_block
* 描       述: 设置当前socket为阻塞
* 输入参数:      INT32 fd socket句柄
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_network_set_block(INT32 fd);

/***********************************************************
* 函数名称: jiot_network_close
* 描       述: 关闭socket
* 输入参数:      INT32 fd socket句柄
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_network_close(INT32 fd);

/***********************************************************
* 函数名称: jiot_network_shutdown
* 描       述: 关闭socket
* 输入参数:      INT32 fd   socket句柄
                INT32 how  关闭模式
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_network_shutdown(INT32 fd,INT32 how);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif


