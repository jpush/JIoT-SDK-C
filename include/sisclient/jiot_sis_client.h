#ifndef JIOT_C_MQTT_SDK_SRC_SISCLIENT_JIOT_SIS_CLIENT_H_
#define JIOT_C_MQTT_SDK_SRC_SISCLIENT_JIOT_SIS_CLIENT_H_

#define SIS_SERVICE_URI "v1/addrget"
#define SIS_HOST_MAX_LEN 32
#define SIS_URL_MAX_LEN 256
#define SIS_RESP_MAX_LEN 256
#define SIS_RETRY_TIMES 1
#define SIS_MIN_DATA_LEN 10
#define SERVICE_ADDR_MAX 2


#define PROTOCOL_TYPE_TCP 0
#define PROTOCOL_TYPE_SSL 1



struct SisDeviceInfo
{
    char szProductKey[32];
    char szDeviceName[32];
    char szDeviceSecret[32];
};
typedef struct SisDeviceInfo SisDeviceInfo;

struct ServiceAddr
{
	char host[40];
	unsigned short port;
};
typedef struct ServiceAddr ServiceAddr;

struct SisInfo
{
	//service address
	ServiceAddr addr[SERVICE_ADDR_MAX];
};
typedef struct SisInfo SisInfo;

int getSisInfo(const SisDeviceInfo* dev, SisInfo* sis);
// int setSisHost(const char* host);


#endif /* JIOT_C_MQTT_SDK_SRC_SISCLIENT_JIOT_SIS_CLIENT_H_ */
