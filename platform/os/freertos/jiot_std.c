
#include "jiot_std.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define DECSEPER    1000000000ULL           // 10^9
#define DECSEPER2   1000000000000000000ULL  // 10^18

INT32 jiot_signal_SIGPIPE(void)
{
    return 0;
}

void* jiot_malloc(INT32 size)
{
    return malloc(size);
}

void jiot_free(void* ptr)
{
    free(ptr);
    return;
}

void* jiot_realloc(void* ptr, INT32 size)
{
    return realloc(ptr,size);
}

INT32 jiot_ulltoa(char* buf, UINT64 num) //need a buf with 21 bytes
{
    INT32 ret = -1;
    if(num < DECSEPER)
    {
        ret = sprintf(buf, "%d", (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    else if(num >= DECSEPER && num < DECSEPER2)
    {
        ret = sprintf(buf, "%u%09u", (unsigned int)(num/DECSEPER) &0xFFFFFFFF, (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    else if(num >= DECSEPER2)
    {
        ret = sprintf(buf, "%u%09u%09u", (unsigned int)(num/DECSEPER2) &0xFFFFFFFF, (unsigned int)((num/DECSEPER)%DECSEPER) &0xFFFFFFFF, (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    return ret;
}

INT32 jiot_lltoa(char* buf, INT64 num) //need a buf with 21 bytes
{
    char symble[2] = {0};
    if(num < 0)
    {
        symble[0] = '-';
        num = 0-num ;       //取相反数
    }

    INT32 ret = -1;
    if(num < DECSEPER)
    {
        ret = sprintf(buf, "%s%d", symble, (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    else if(num >= DECSEPER && num < DECSEPER2)
    {
        ret = sprintf(buf, "%s%u%09u", symble, (unsigned int)(num/DECSEPER) &0xFFFFFFFF, (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    else if(num >= DECSEPER2)
    {
        ret = sprintf(buf, "%s%u%09u%09u", symble, (unsigned int)(num/DECSEPER2) &0xFFFFFFFF, (unsigned int)((num/DECSEPER)%DECSEPER) &0xFFFFFFFF, (unsigned int)(num%DECSEPER) &0xFFFFFFFF);
    }
    return ret;
}


