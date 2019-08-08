#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "jiot_client.h"
#include<string.h>


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
        printf("message seqNo:[%lld]\n", JRet.seqNo ); 
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
        printf("message seqNo:[%lld]\n", JRet.seqNo ); 
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
        printf("message seqNo:[%lld]\n", JRet.seqNo ); 
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
    printf("\nJclient message time out .seqNo[%lld] \n",seqNo);
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
            sleep(1);
        }
        else
        {
            break ;
        }
    }
    return 0;
}

int get_char(char* buf)
{
    fd_set rfds;
    struct timeval tv;
    int ch = 0;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    if (select(1, &rfds, NULL, NULL, &tv) > 0)
    {
	    ch  = scanf("%s",buf);        
    }
    return ch;
}

int main(int argc,char *argv[] )
{
    printf("start demo!\n");

    jiotSetLogLevel(DEBUG_LOG_LEVL);

	int  ret = 0 ;

    DeviceInfo devinfo ; 
    if (argc != 4 )
    {
        printf("err input ! use [demo ProductKey DeviceName DeviceSecret ] \n");
        return 0;
    }
    else
    {
        strcpy(devinfo.szProductKey,argv[1]);
        strcpy(devinfo.szDeviceName,argv[2]);
        strcpy(devinfo.szDeviceSecret,argv[3]);
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

    printf("Enter: \"property\",\"event\",\"version\" \n");
    char buf[1024] = {0};
    while(1)
    {
        memset(buf,0,1024);
        int nRet = get_char(buf);
        if(nRet > 0 )
        {
            if (!strcmp(buf,"property"))
            {
                /* 属性上报*/
                printf("Enter Property Name:");
                char buf1[1024] = {0};
                scanf("%s",buf1);
                Dev.pProperty->name = buf1;

                printf("Enter Property Value:");
                char buf2[1024] = {0};
                scanf("%s",buf2);
                Dev.pProperty->value=buf2;

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
            else
            if (!strcmp(buf,"event"))
            {
                /* 事件上报*/
                printf("Enter Event Name:");
                char buf1[1024] = {0};
                scanf("%s",buf1);
                Dev.pEvent->name = buf1;

                printf("Enter Event Content:");
                char buf2[1024] = {0};
                scanf("%s",buf2);
                Dev.pEvent->content = buf2;
                Dev.pEvent->time = (long long)time(NULL);
                ret = eventReportReq(handle,&Dev,Dev.pEvent);
                if(ret == 0)
                {
                    printf("send event report \n");
                }
                else
                {
                    printf("send event report err,code:[%d]\n",ret);
                }
            }
            else
            if (!strcmp(buf,"version"))
            {
                /* 版本上报 */
                printf("Enter App Version:");
                memset(buf,0,1024);
                scanf("%s",buf);
                strncpy(Dev.app_ver,buf,32);

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
            else
            if (!strcmp(buf,"exit"))
            {
                break;
            }
        }
        else
        {
            check(handle);
        }
    }

    jiotDisConn(handle);

    jiotUnRegister(handle);

    jiotRelease(&handle);

    free(Dev.pProperty);
    free(Dev.pEvent);

    printf("out of demo!\n");

    return 0;
}

