
include $(JIOT_C_SDK)/build/m5311/buildid.mk
CFLAGS += -DJIOT_SSL 
CFLAGS += -DSDK_PLATFORM='"m5311"'

SRC_DIRS += $(JIOT_C_SDK)/common
SRC_DIRS += $(JIOT_C_SDK)/examples/m5311
SRC_DIRS += $(JIOT_C_SDK)/include/jclient
SRC_DIRS += $(JIOT_C_SDK)/include/mqtt
SRC_DIRS += $(JIOT_C_SDK)/include/sisclient
SRC_DIRS += $(JIOT_C_SDK)/platform/os/m5311
SRC_DIRS += $(JIOT_C_SDK)/public/net/tcp
SRC_DIRS += $(JIOT_C_SDK)/public/net/ssl
SRC_DIRS += $(JIOT_C_SDK)/platform/ssl/mbedtls
SRC_DIRS += $(JIOT_C_SDK)/src/jclient
SRC_DIRS += $(JIOT_C_SDK)/src/mqtt/MQTTPacket/src
SRC_DIRS += $(JIOT_C_SDK)/src/mqtt/MQTTClient-C/src
SRC_DIRS += $(JIOT_C_SDK)/src/sisclient
SRC_DIRS += mbedtls-mbedtls-2.16.1/include

