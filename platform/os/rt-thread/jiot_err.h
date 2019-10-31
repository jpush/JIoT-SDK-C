#ifndef JIOT_ERR_H
#define JIOT_ERR_H


#include "jiot_std.h"

#if defined(__cplusplus)
 extern "C" {
#endif

#define  JIOT_SUCCESS                                 0      //成功
#define  JIOT_FAIL                                   -1      //失败

// EINTR 4 /* Interrupted system call */
// EAGAIN 11 /* Try again */
// EWOULDBLOCK EAGAIN 41 /* Operation would block */
#define  JIOT_ERR_NET_EINTR                          -20001
#define  JIOT_ERR_NET_EAGAIN                         -20002
#define  JIOT_ERR_NET_EWOULDBLOCK                    -20003

#define  JIOT_ERR_TCP_SETOPT_TIMEOUT                 -21001
#define  JIOT_ERR_TCP_UNKNOWN_HOST                   -21002
#define  JIOT_ERR_TCP_LOC_NET_TYPE                   -21003
#define  JIOT_ERR_TCP_HOST_NET_TYPE                  -21004
#define  JIOT_ERR_TCP_SOCKET_CREATE                  -21005
#define  JIOT_ERR_TCP_SOCKET_CONN                    -21006
#define  JIOT_ERR_TCP_RETRY                          -21007    //网络层重试
#define  JIOT_ERR_TCP_SELECT_TIMEOUT                 -21008

#define  JIOT_ERR_SSL_ERR                            -23001
#define  JIOT_ERR_SSL_IS_NULL                        -23002
#define  JIOT_ERR_SSL_NETWORKTPYE                    -23003
#define  JIOT_ERR_SSL_CONNECT_FAIL                   -23004
#define  JIOT_ERR_SSL_SOCKET_CONNECT_FAIL            -23005
#define  JIOT_ERR_SSL_SOCKET_CREATE_FAIL             -23006
#define  JIOT_ERR_SSL_SELECT_TIMEOUT                 -23007
#define  JIOT_ERR_SSL_READ_TIMEOUT                   -23008
#define  JIOT_ERR_SSL_READ_FAIL                      -23009
#define  JIOT_ERR_SSL_WRITE_FAIL                     -23010
#define  JIOT_ERR_SSL_SELECT_FAIL                    -23011

#define  JIOT_ERR_NET_READ_TIMEOUT                   -22001
#define  JIOT_ERR_NET_RECV_FAIL                      -22002
#define  JIOT_ERR_NET_WRITE_TIMEOUT                  -22003
#define  JIOT_ERR_NET_WRITE_FAIL                     -22004
#define  JIOT_ERR_NET_IS_NULL                        -22005
#define  JIOT_ERR_NET_SELECT_FAIL                    -22006
#define  JIOT_ERR_NET_CREATE_FAIL                    -22006    

#define  JIOT_ERR_THREAD_TIME_OUT                    -24001


/***********************************************************
* 函数名称: jiot_get_errno
* 描       述: 获取系统异常时候的errno，并转换成JIOT的错误码
* 输入参数:
* 输出参数:
* 返 回  值: JIOT的错误码
************************************************************/
INT32 jiot_get_errno(void);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif