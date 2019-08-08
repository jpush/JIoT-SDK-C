
#include "jiot_std.h"


#define DECSEPER    1000000000ULL           // 10^9
#define DECSEPER2   1000000000000000000ULL  // 10^18

INT32 jiot_signal_SIGPIPE(void)
{
    signal(SIGPIPE,SIG_IGN);
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
    INT32 ret = sprintf(buf, "%lld", num);
    return ret;
}

INT32 jiot_lltoa(char* buf, INT64 num) //need a buf with 21 bytes
{
    INT32 ret = sprintf(buf, "%lld", num);
    return ret;
}


