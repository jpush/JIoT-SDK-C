/*
 * user_ota.h
 *
 *  Created on: Jun 28, 2019
 *      Author: guowenhe
 */

#ifndef BUILD_ESP8266_MAIN_USER_OTA_H_
#define BUILD_ESP8266_MAIN_USER_OTA_H_

#include "esp_err.h"

enum OTA_STEP
{
	OTA_STEP_INIT = 0,
	OTA_STEP_HTTP_DOWNLOAD_START = 1,
	OTA_STEP_HTTP_DOWNLOADING,
	OTA_STEP_HTTP_DOWNLOAD_FINASHED,
	OTA_STEP_FIRMWARE_CHECK,
	OTA_STEP_SYSTEM_REBOOT,
	OTA_STEP_HTTP_DOWNLOAD_FAILED,
	OTA_STEP_FIRMWARE_CHECK_FAILED,
	OTA_STEP_FAILED,
};

struct httpOTACallbackParam
{
	int step;
	int downloadPercent; //[1-100]
	char* desc;
	long taskId;
};
typedef struct httpOTACallbackParam httpOTACallbackParam;
typedef esp_err_t (*httpOTACallback)(httpOTACallbackParam* param);

struct httpOTAParam
{
	unsigned firmwareSize;
	char* url;
	char* checksum;
	char* version;
	long taskId;
	httpOTACallback callback;
};
typedef struct httpOTAParam httpOTAParam;


esp_err_t httpOTA(httpOTAParam* param);




#endif /* BUILD_ESP8266_MAIN_USER_OTA_H_ */
