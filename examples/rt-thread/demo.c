#include <rtthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "jiot_std.h"
#include "jiot_code.h"
#include "jiot_err.h"
#include "jiot_log.h"
#include "jiot_pthread.h"
#include <string.h>

#ifdef PKG_USING_JIOT_PRODUCT_KEY
    #define PRODUCT_KEY             PKG_USING_JIOT_PRODUCT_KEY
#else
    #define PRODUCT_KEY             ""
#endif

#ifdef PKG_USING_JIOT_DEVICE_NAME
    #define DEVICE_NAME             PKG_USING_JIOT_DEVICE_NAME
#else
    #define DEVICE_NAME             ""
#endif

#ifdef PKG_USING_JIOT_DEVICE_SECRET
    #define DEVICE_SECRET           PKG_USING_JIOT_DEVICE_SECRET
#else
    #define DEVICE_SECRET           ""
#endif

typedef struct DEV
{
    long long seq_no;
    int desired_version ;
    int version ;
    int oldVersion;
    Property *pProperty;
    Event *pEvent;
    char app_ver[32];
    char msg[2048];
}DEV;

#define D_DECSEPER 		1000000000ULL			// 10^9
#define D_DECSEPER2 	1000000000000000000ULL	// 10^18
int lltoa(char* buf, long long num) //need a buf with 21 bytes
{
	char symble[2] = {0};
	if(num < 0)
	{
		symble[0] = '-';
		num = llabs(num);
	}

	int ret = -1;
	if(num < D_DECSEPER)
	{
		ret = sprintf(buf, "%s%d", symble, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	else if(num >= D_DECSEPER && num < D_DECSEPER2)
	{
		ret = sprintf(buf, "%s%u%09u", symble, (unsigned int)(num/D_DECSEPER) &0xFFFFFFFF, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	else if(num >= D_DECSEPER2)
	{
		ret = sprintf(buf, "%s%u%09u%09u", symble, (unsigned int)(num/D_DECSEPER2) &0xFFFFFFFF, (unsigned int)((num/D_DECSEPER)%D_DECSEPER) &0xFFFFFFFF, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	return ret;
}

#define PRINT_SEQNO(seqNo)\
    do\
    {\
    	char buf[21] = {0};\
    	lltoa(buf, seqNo);\
        printf("message seqNo:[%s]\n", buf);\
    }\
    while(0)
		
int propertyReportReq(JHandle handle, DEV* pDev,Property * property,int size)
{
    PropertyReportReq req ;
    req.seq_no =  0;
    req.version = pDev->version;
    req.property_size= size;

    Property * tempProperty =  (Property*)malloc(sizeof(Property)*size);
    memset((void*)tempProperty,0,sizeof(Property)*size);
    int i =0 ;
    for (i = 0; i < size; ++i)
    {
        tempProperty[i].name = property[i].name;
        tempProperty[i].value = property[i].value;
        tempProperty[i].time = property[i].time;
    }

    req.pProperty = tempProperty;

    JiotResult JRet = jiotPropertyReportReq(handle,&req);

	if (tempProperty != NULL)
    {
        free(tempProperty);
    }
    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}

int eventReportReq(JHandle handle,DEV* pDev,Event *event )
{
    EventReportReq req ;
    req.seq_no =  0;

    Event tempEvent ;
    tempEvent.name = event->name;
    tempEvent.content = event->content;
    tempEvent.time = event->time;

    req.pEvent = &tempEvent;

    JiotResult JRet = jiotEventReportReq(handle,&req);

    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}

int versionReportReq(JHandle handle,DEV*pDev,char* app_ver)
{
    VersionReportReq req ;
    req.seq_no =  0;
    req.app_ver= app_ver;

    JiotResult JRet  =  jiotVersionReportReq(handle,&req);

    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}


int propertyReportRsp(void* pContext,JHandle handle,const PropertyReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("propertyReportRsp err code:[%d]\n",errcode);
        return 0;
    }

    printf("propertyReportRsp:\n");
    DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        if(Rsp->version != 0)
        {
            pDev->version = Rsp->version+1;
            pDev->oldVersion = pDev->version;
        }
        printf("ERR: code [%d]\n", Rsp->code );
    }
    else
    {
        pDev->oldVersion = pDev->version;
        printf("OK\n");
    }

    return 0;
}

int eventReportRsp(void* pContext,JHandle handle,const EventReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("eventReportRsp err code:[%d]\n",errcode);
    }

    printf("eventReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    printf("OK\n");
    return 0;
}



int versionReportRsp(void* pContext,JHandle handle,const VersionReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("versionReportRsp err code:[%d]\n",errcode);
    }

    printf("versionReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    printf("OK\n");
    return 0;
}

int propertySetReq(void* pContext,JHandle handle,PropertySetReq *Req,int errcode)
{
    if (errcode != 0)
    {
        printf("propertySetReq msg err code:[%d]\n",errcode);
    }

    printf("propertySetReq:\n");
    DEV* pDev = (DEV*)pContext;
    pDev->desired_version = Req->version;

    Property * pProperty = malloc(sizeof(Property)*Req->property_size);
    int i =0;
    for (i=0; i < Req->property_size; ++i)
    {
        printf("property.name:[%s]\nproperty.value:[%s]\n",  Req->pProperty[i].name,Req->pProperty[i].value);

        struct tm *tblock;
        time_t timer = Req->pProperty[i].time/1000;
        tblock = localtime(&timer);
        printf("property.time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);

        pProperty->name = Req->pProperty[i].name;
        pProperty->value = Req->pProperty[i].value;
        pProperty->time = Req->pProperty[i].time;
    }

    if (pDev->version == pDev->oldVersion)
    {
        pDev->version ++ ;
    }

    free(pProperty);
    return 0;

}

int msgDeliverReq(void* pContext,JHandle handle,MsgDeliverReq *Req,int errcode)
{
    if (errcode != 0)
    {
        printf("msgDeliverReq msg err code:[%d]\n",errcode);
    }

    printf("msgDeliverReq:\n");
	//pContext 之前注册的上下文信息
    //DEV* pDev = (DEV*)pContext;

    printf("message:[%s]\n", Req->message);
    struct tm *tblock;
    time_t timer = Req->time;
    tblock = localtime(&timer);

    printf("time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);


    return 0;

}

int connectedHandle(void* pContext)
{
    printf("\nJclient connected \n");
    return 0;
}

int connectFailHandle(void* pContext,int errCode)
{
    printf("\nJclient connect fail . Code:[%d] \n",errCode);
    return 0;
}

int disconnectHandle(void* pContext)
{
    printf("\nJclient disconnect .  \n");
    return 0;
}

int subscribeFailHandle(void* pContext,char * TopicFilter)
{
    printf("\nJclient subscribe topic fail .Topic[%s] \n",TopicFilter);
    return 0;
}

int messageTimeoutHandle(void* pContext,long long seqNo)
{
    printf("\nJclient message time out ");
    PRINT_SEQNO(seqNo);
    return 0;
}

int check(JHandle handle)
{
    while(1)
    {
        JClientStatus status = jiotGetConnStatus(handle);
        if(status != CLIENT_CONNECTED )
        {
            printf(".");
            fflush(stdout);
            jiot_sleep(1 * 1000);
        }
        else
        {
            break ;
        }
    }
    return 0;
}

static int start_demo_test()
{
	
	int  ret = 0 ;

	jiotSetLogLevel(DEBUG_LOG_LEVL);
    DeviceInfo devinfo ;
    {
        strcpy(devinfo.szProductKey, PRODUCT_KEY);
        strcpy(devinfo.szDeviceName, DEVICE_NAME);
        strcpy(devinfo.szDeviceSecret, DEVICE_SECRET);
    }


    DEV Dev ;
	Dev.seq_no = 1 ;
	Dev.version = 1 ;
	memset((void*)Dev.app_ver,0,32);
	memset((void*)Dev.msg,0,1024);

	JClientMessageCallback cb;
	cb._cbPropertyReportRsp =   propertyReportRsp ;
	cb._cbEventReportRsp    =   eventReportRsp;
	cb._cbVersionReportRsp  =   versionReportRsp;
	cb._cbPropertySetReq    =   propertySetReq;
	cb._cbMsgDeliverReq     =   msgDeliverReq;


	JClientHandleCallback handleCb;

	handleCb._cbConnectedHandle     = connectedHandle;
	handleCb._cbConnectFailHandle   = connectFailHandle;
	handleCb._cbDisconnectHandle    = disconnectHandle;
	handleCb._cbSubscribeFailHandle = subscribeFailHandle;
	handleCb._cbPublishFailHandle   = NULL ;
	handleCb._cbMessageTimeoutHandle= messageTimeoutHandle;


	int nProperty_size = 1;
	Dev.pProperty = (Property*)malloc(sizeof(Property)*nProperty_size);
	Dev.pEvent = (Event*)malloc(sizeof(Event));

	JHandle handle = jiotInit();
	if (handle ==NULL)
	{
		printf("jClient init err \n");
		return 0;
	}

	jiotRegister(handle,&Dev,&cb,&handleCb);

	ret = jiotConn(handle,&devinfo);
	if(ret != 0)
	{
		printf("connect err,errno:%d\n",ret);
		return 0;
	}

	printf("Connection, please later \n");

	check(handle);

	for(int i = 0; i < 1; ++i)
	{
		/* 属性上报*/
		{
			printf("property report:\n");
			Dev.pProperty->name = "senduo_test_property";
			Dev.pProperty->value="1";

			Dev.pProperty->time = (long long )time(NULL)*1000; //毫秒
			ret = propertyReportReq(handle,&Dev,Dev.pProperty,1);
			if(ret == 0)
			{
				printf("send property report\n");
			}
			else
			{
				printf("send property report err,code:[%d]\n",ret);
			}
		}

		/* 事件上报*/
		{
			printf("event report:\n");
			Dev.pEvent->name = "senduo_test_event";
			Dev.pEvent->content = "This content of event!";
			Dev.pEvent->time = (long long)time(NULL);
			int ret = eventReportReq(handle,&Dev,Dev.pEvent);
			if(ret == 0)
			{
				printf("send event report \n");
			}
			else
			{
				printf("send event report err,code:[%d]\n",ret);
			}
		}

        /* 版本上报 */
		{
          printf("version report:\n");
          strncpy(Dev.app_ver,"dengdeng1.1.0",32);

          ret = versionReportReq(handle,&Dev,Dev.app_ver);
          if(ret == 0)
          {
              printf("send version report \n");
          }
          else
          {
              printf("send version report err,code:[%d]\n",ret);
          }
		}

		jiot_sleep(10 * 1000);
	}

	// waiting to accept message
	for(int i = 0; i < 5; ++i)
	{
		printf("waiting to accept message \n");
		jiot_sleep(10 * 1000);
	}


	jiotDisConn(handle);
    printf("jiotDisConn \n");

	jiotUnRegister(handle);
    printf("jiotUnRegister  \n");

	jiotRelease(&handle);
    printf("jiotRelease \n");

	free(Dev.pProperty);
	free(Dev.pEvent);


	printf("demo exit \n");
    return 0;
	
}


#ifdef RT_USING_JIOT_C_SDK
#include <finsh.h>

MSH_CMD_EXPORT_ALIAS(start_demo_test, start_demo_test,Example: start_demo_test);
#endif

