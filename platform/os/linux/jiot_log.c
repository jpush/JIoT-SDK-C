#include"jiot_log.h"
#include <stdarg.h> 

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
            printf("[E] %s\n", buf);
            break;
        case INFO_LOG_LEVL :
            printf("[I] %s\n", buf);
            break;
        case DEBUG_LOG_LEVL :
            printf("[D] %s\n", buf);
            break;
        default:
            return ;
    }
    return ;
}
