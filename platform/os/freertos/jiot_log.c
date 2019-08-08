#include"jiot_log.h"
#include "esp_log.h"

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
            ESP_LOG_LEVEL(ESP_LOG_ERROR,"jiot",buf);
            break;
        case INFO_LOG_LEVL :
            ESP_LOG_LEVEL(ESP_LOG_INFO, "jiot",buf);
            break;
        case DEBUG_LOG_LEVL :
            ESP_LOG_LEVEL(ESP_LOG_DEBUG,"jiot",buf);
            break;
        default:
            return ;
    }
    return ;
}
