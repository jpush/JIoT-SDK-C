
#SDK_SRC_DIRS    SDK源码路径。使用相对于当前目录的相对路径
#SDK_PLATFORM_OS SDK的OS 
#SDK_SSL 		 SDK的SSL库

SDK_SRC_DIRS := ../../../..
SDK_PLATFORM_OS := freertos
SDK_SSL := mbedtls

COMPONENT_ADD_INCLUDEDIRS += $(SDK_SRC_DIRS)/common \
							$(SDK_SRC_DIRS)/platform/os/$(SDK_PLATFORM_OS) \
							$(SDK_SRC_DIRS)/platform/ssl/$(SDK_SSL) \
							$(SDK_SRC_DIRS)/public/net/tcp \
							$(SDK_SRC_DIRS)/public/net/ssl  \
							$(SDK_SRC_DIRS)/include/jclient \
							$(SDK_SRC_DIRS)/include/mqtt \
							$(SDK_SRC_DIRS)/include/sisclient \
							$(SDK_SRC_DIRS)/src/mqtt/MQTTPacket/src 
							     
COMPONENT_SRCDIRS := $(SDK_SRC_DIRS)/common \
					$(SDK_SRC_DIRS)/platform/os/$(SDK_PLATFORM_OS) \
					$(SDK_SRC_DIRS)/platform/ssl/$(SDK_SSL) \
					$(SDK_SRC_DIRS)/public/net/tcp \
					$(SDK_SRC_DIRS)/public/net/ssl\
					$(SDK_SRC_DIRS)/src/jclient \
					$(SDK_SRC_DIRS)/src/sisclient \
					$(SDK_SRC_DIRS)/src/mqtt/MQTTClient-C/src \
					$(SDK_SRC_DIRS)/src/mqtt/MQTTPacket/src 


CFLAGS += -DJIOT_SSL 
CFLAGS += -DSDK_PLATFORM='"esp8266"'

