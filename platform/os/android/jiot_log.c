#include"jiot_log.h"
#include <stdarg.h> 
#include <android/log.h>


#define TAG "JIGUANG_IOT"

int g_logLevl = NO_LOG_LEVL ;

void jiot_log_out(int logLevl, const char *fmt, ...)
{    
    char buf[LOG_BUFF_LEN]={0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf,LOG_BUFF_LEN-1, fmt, args);
    
    va_end(args);

    switch(logLevl) 
    {
        case ERROR_LOG_LEVL :
            __android_log_print(ANDROID_LOG_ERROR,TAG,"%s", buf);
            break;
        case INFO_LOG_LEVL :
            __android_log_print(ANDROID_LOG_INFO,TAG,"%s", buf);
            break;
        case DEBUG_LOG_LEVL :
            __android_log_print(ANDROID_LOG_DEBUG,TAG,"%s", buf);
            break;
        default:
            return ;
    }
    return ;
}
