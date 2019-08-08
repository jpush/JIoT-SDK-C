#ifndef JIOT_FILE_H
#define JIOT_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

#include "jiot_err.h"
#include "jiot_std.h"

#if defined(__cplusplus)
 extern "C" {
#endif

typedef struct JIOT_FILE_HANDLE
{
    FILE* fd;
}S_JIOT_FILE_HANDLE;

/*******************************************
 * 文件打开类型
*******************************************/
typedef enum JIOT_FILE_FLAG
{
    RD_FLAG                = 0x00000001,  //文件已存在，只读打开
    RDWR_FALG              = 0x00000002,  //文件已存在，读写打开
    CWR_FALG               = 0x00000004,  //文件可不存在，只写的方式打开
    CRDWR_FLAG             = 0x00000008,  //文件可不存在，读写的方式打开
    CA_FLAG                = 0x0000000A,  //文件可不存在，在文件末尾以附加只写的方式打开
    CARDWR_FLAG            = 0x00000010,  //文件可不存在，在文件末尾以附加读写的方式打开
}E_JIOT_FILE_FLAG;


/*******************************************
 * 文件读写指针偏离类型
*******************************************/
typedef enum JIOT_FILE_OFFSE_FLAG
{
    BEGIN_FLAG            = 0x00000001,  //从文件头部开始偏移
    END_FALG              = 0x00000002,  //从文件尾部开始偏移
}E_JIOT_FILE_OFFSE_FLAG;


/***********************************************************
* 函数名称: jiot_file_fopen
* 描       述: 打开指定路径的文件
* 输入参数:      S_JIOT_FILE_HANDLE* handle 文件句柄指针
                char*               filename 文件路径              
                E_JIOT_FILE_FLAG    flags    打开文件模式
* 输出参数:      S_JIOT_FILE_HANDLE* handle 文件句柄指针
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_fopen(S_JIOT_FILE_HANDLE* handle,char* filename,E_JIOT_FILE_FLAG flags);

/***********************************************************
* 函数名称: jiot_file_fclose
* 描       述: 关闭文件句柄
* 输入参数:      S_JIOT_FILE_HANDLE* handle 文件句柄指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_fclose(S_JIOT_FILE_HANDLE* handle);

/***********************************************************
* 函数名称: jiot_file_fwrite
* 描       述: 对文件进行写操作
* 输入参数:      S_JIOT_FILE_HANDLE* handle 文件句柄指针
                char*               buf    写入buf的指针
                INT32               size   数据类型的大小
                INT32               count  写入数据类型的个数
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_fwrite(S_JIOT_FILE_HANDLE* handle,char* buf,INT32 size,INT32 count);

/***********************************************************
* 函数名称: jiot_file_fread
* 描       述: 对文件进行读操作
* 输入参数:      S_JIOT_FILE_HANDLE* handle 文件句柄指针
                char*               buf    读取buf的指针
                INT32               size   数据类型的大小
                INT32               count  读取数据类型的最大个数
* 输出参数:      
* 返 回  值: 成功返回    读取的数据个数
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_fread(S_JIOT_FILE_HANDLE* handle,char* buf,INT32 size,INT32 count);

/***********************************************************
* 函数名称: jiot_file_fseek
* 描       述: 指向文件指定的读写位置
* 输入参数:      S_JIOT_FILE_HANDLE*     handle 文件句柄指针
                INT32                   offset 文件的偏移位置
                E_JIOT_FILE_OFFSE_FLAG  flags  偏移方式
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_fseek(S_JIOT_FILE_HANDLE* handle,INT32 offset,E_JIOT_FILE_OFFSE_FLAG flags);

/***********************************************************
* 函数名称: jiot_file_ftell
* 描       述: 获取文件的相对于文件头开始的当前读写位置
* 输入参数:      S_JIOT_FILE_HANDLE* handle 
* 输出参数:      
* 返 回  值: 成功返回    偏移位置
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_ftell(S_JIOT_FILE_HANDLE* handle);

/***********************************************************
* 函数名称: jiot_file_access
* 描       述: 判断文件是否存在
* 输入参数:      char* filename    文件名
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_access(char* filename);

/***********************************************************
* 函数名称: jiot_file_remove
* 描       述: 移除文件
* 输入参数:      char* filename    文件名
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_file_remove(char* filename);

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
