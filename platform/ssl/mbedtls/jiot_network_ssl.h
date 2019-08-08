#ifndef JIOT_NETWORK_SSL_H
#define JIOT_NETWORK_SSL_H

#include "jiot_err.h"
#include "jiot_network.h"



#ifdef JIOT_SSL

#include <string.h>
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/debug.h"

typedef struct _TLSDataParams {
    mbedtls_ssl_context ssl;          /**< mbed TLS control context. */
    mbedtls_net_context fd;           /**< mbed TLS network context. */
    mbedtls_ssl_config conf;          /**< mbed TLS configuration context. */
    mbedtls_x509_crt cacertl;         /**< mbed TLS CA certification. */
    mbedtls_x509_crt clicert;         /**< mbed TLS Client certification. */
    mbedtls_pk_context pkey;          /**< mbed TLS Client key. */
    int socketId;
}TLSDataParams;

/***********************************************************
* 函数名称: jiot_network_ssl_read
* 描       述: 接收报文，指定报文长度
* 输入参数:      TLSDataParams *pTlsData  TLS结构体
                unsigned char *buffer    读取缓冲区
                int len                  需要读取数据长度
                int timeout_ms           超时时间
* 输出参数:      
* 返 回  值: 成功返回              接收的字节数
            SELECT超时返回        JIOT_ERR_SSL_SELECT_TIMEOUT
            SELECT失败返回        JIOT_ERR_SSL_SELECT_FAIL
            SSL读取超时返回        JIOT_ERR_SSL_READ_TIMEOUT
            SSL读取失败返回        JIOT_ERR_SSL_READ_FAIL
************************************************************/
int jiot_network_ssl_read(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms);

/***********************************************************
* 函数名称: jiot_network_ssl_read_buffer
* 描       述: 接收报文，指定缓冲区的长度
* 输入参数:      TLSDataParams *pTlsData  TLS结构体
                unsigned char *buffer    读取缓冲区
                int maxlen               缓冲区长度
                int timeout_ms           超时时间
* 输出参数:      
* 返 回  值: 成功返回              接收的字节数
            SELECT超时返回        JIOT_ERR_SSL_SELECT_TIMEOUT
            SELECT失败返回        JIOT_ERR_SSL_SELECT_FAIL
            SSL读取超时返回        JIOT_ERR_SSL_READ_TIMEOUT
            SSL读取失败返回        JIOT_ERR_SSL_READ_FAIL
************************************************************/
int jiot_network_ssl_read_buffer(TLSDataParams *pTlsData, unsigned char *buffer, int maxlen, int timeout_ms);

/***********************************************************
* 函数名称: jiot_network_ssl_write
* 描       述: 发送报文
* 输入参数:      TLSDataParams *pTlsData  TLS结构体
                unsigned char *buffer    发送缓冲区
                int len                  需要发生数据长度
                int timeout_ms           超时时间
* 输出参数:      
* 返 回  值: 成功返回              发送的字节数
            SELECT超时返回        JIOT_ERR_SSL_SELECT_TIMEOUT
            SELECT失败返回        JIOT_ERR_SSL_SELECT_FAIL
            SSL发送失败返回        JIOT_ERR_SSL_WRITE_FAIL
************************************************************/
int jiot_network_ssl_write(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms);

/***********************************************************
* 函数名称: jiot_network_ssl_disconnect
* 描       述: 关闭socket
* 输入参数:      TLSDataParams *pTlsData  TLS结构体
* 输出参数:      
* 返 回  值: 
************************************************************/
void jiot_network_ssl_disconnect(TLSDataParams *pTlsData);

/***********************************************************
* 函数名称: jiot_network_ssl_connect
* 描       述:  创建socket会话
* 输入参数:      TLSDataParams *pTlsData    TLS结构体
                const char *addr           地址
                const char *port           端口
                const char *ca_crt         CA证书
                int ca_crt_len             CA证书长度
                E_NET_TYPE nType           网络类型
* 输出参数:      
* 返 回  值: 成功返回            JIOT_SUCCESS
            TLS内容为空返回      JIOT_ERR_SSL_IS_NULL  
            TLS连接创建失败返回   JIOT_ERR_SSL_SOCKET_CREATE_FAIL 
            TLS连接失败返回      JIOT_ERR_SSL_CONNECT_FAIL         
************************************************************/
int jiot_network_ssl_connect(TLSDataParams *pTlsData, const char *addr, const char *port, const char *ca_crt, int ca_crt_len,E_NET_TYPE nType);

#endif

#endif
