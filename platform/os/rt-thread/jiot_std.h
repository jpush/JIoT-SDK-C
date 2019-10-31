#ifndef JIOT_CSTD_H
#define JIOT_CSTD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#if defined(__cplusplus)
 extern "C" {
#endif

typedef unsigned char         UINT8;
typedef char                  INT8;

typedef unsigned short        UINT16;
typedef short                 INT16;

typedef unsigned int          UINT32;
typedef int                   INT32;

typedef unsigned long         ULONG;
typedef long                  LONG;

typedef unsigned long long    UINT64;
typedef long long             INT64;

typedef float                 FLOAT;
typedef double                DOUBLE;

typedef unsigned int          BOOL;

#define  JIOT_TRUE    1
#define  JIOT_FALSE   0

/***********************************************************
* 函数名称: jiot_signal_SIGPIPE
* 描    述: 进程忽略SIGPIPE信号，部分平台可以不支持
* 输入参数:      
* 输出参数:      
* 返 回  值: 0
************************************************************/
INT32 jiot_signal_SIGPIPE(void);

/***********************************************************
* 函数名称: jiot_malloc
* 描    述: 申请堆上空间
* 输入参数:      INT32  size    申请空间的大小            
* 输出参数:      
* 返 回  值:     申请空间指针
************************************************************/
void* jiot_malloc(INT32 size);

/***********************************************************
* 函数名称: jiot_free
* 描    述: 释放堆上空间
* 输入参数:     void* ptr       空间指针    
* 输出参数:      
* 返 回  值: 
************************************************************/
void  jiot_free(void* ptr);

/***********************************************************
* 函数名称: jiot_realloc
* 描    述: 重新申请堆上空间
* 输入参数:      void*  ptr     空间指针    
                INT32  size    新申请空间的大小            
* 输出参数:      
* 返 回  值: 
************************************************************/
void*  jiot_realloc(void* ptr, INT32 size);

/***********************************************************
* 函数名称: jiot_ulltoa
* 描    述: 部分平台不支持 unsigned long long 型转成字符串，为代码一致性，增加此接口
* 输入参数:      char* buf                 存放转换成字符串的buf
                UINT64 num                需要转换的数字
* 输出参数:      char* buf                 转换成字符串的buf
* 返 回  值: 
************************************************************/
INT32 jiot_ulltoa(char* buf, UINT64 num); 

/***********************************************************
* 函数名称: jiot_free
* 描    述: 部分平台不支持 unsigned long long 型转成字符串，为代码一致性，增加此接口
* 输入参数:      char* buf                 存放转换成字符串的buf
                INT64 num                 需要转换的数字
* 输出参数:      char* buf                 转换成字符串的buf
* 返 回  值: 
************************************************************/
INT32 jiot_lltoa(char* buf, INT64 num); 

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif