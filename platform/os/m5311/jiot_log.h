#ifndef JIOT_LOG_H
#define JIOT_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "jiot_client.h"
#include "m5311_opencpu.h"

#define UART_PRINTF_MAX_LEN 600
#define UART_PRINTF_PORT HAL_UART_1

void uart_printf_init();
void uart_printf (const char *str, ...);


extern int g_logLevl ;

#define DEBUG_LOG(format, ...) \
{\
    if (g_logLevl >= DEBUG_LOG_LEVL)\
    {\
    	uart_printf("[D] %ld %s:%d %s()| "format"\n",(long int)time(NULL),__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define INFO_LOG(format, ...) \
{\
    if (g_logLevl >= INFO_LOG_LEVL)\
    {\
    	uart_printf("[I] %ld %s:%d %s()| "format"\n",(long int)time(NULL),__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define ERROR_LOG(format,...) \
{\
    if (g_logLevl >= ERROR_LOG_LEVL)\
    {\
    	uart_printf("[E] %ld %s:%d %s()| "format"\n",(long int)time(NULL),__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\
/*
#define DEBUG_LOG(format, ...) \
{\
    if (g_logLevl >= DEBUG_LOG_LEVL)\
    {\
    	uart_printf("[D] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define INFO_LOG(format, ...) \
{\
    if (g_logLevl >= INFO_LOG_LEVL)\
    {\
    	uart_printf("[D] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define NOTICE_LOG(format, ...) \
{\
    if (g_logLevl >= NOTICE_LOG_LEVL)\
    {\
    	uart_printf("[D] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define WARNING_LOG(format, ...) \
{\
    if (g_logLevl >= WARNING_LOG_LEVL)\
    {\
    	uart_printf("[D] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\

#define ERROR_LOG(format,...) \
{\
    if (g_logLevl >= ERROR_LOG_LEVL)\
    {\
    	uart_printf("[E] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}\
*/

#endif
