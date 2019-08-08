
#SDK_OBJ_DIRS    SDK源码路径。使用相对于当前目录的相对路径
#SDK_PLATFORM_OS SDK的OS 
#SDK_SSL 		 SDK的SSL库

SDK_OBJ_DIRS := ../../../..
SDK_PLATFORM_OS := freertos
SDK_SSL := mbedtls

COMPONENT_ADD_INCLUDEDIRS += $(SDK_OBJ_DIRS)/common \
							$(SDK_OBJ_DIRS)/platform/os/$(SDK_PLATFORM_OS) \
							$(SDK_OBJ_DIRS)/platform/ssl/$(SDK_SSL) \
							$(SDK_OBJ_DIRS)/public/net/tcp \
							$(SDK_OBJ_DIRS)/public/net/ssl  \
							$(SDK_OBJ_DIRS)/include/jclient \
							$(SDK_OBJ_DIRS)/include/mqtt \
							$(SDK_OBJ_DIRS)/include/sisclient \
							$(SDK_OBJ_DIRS)/src/mqtt/MQTTPacket/src 
							     
COMPONENT_SRCDIRS := $(SDK_OBJ_DIRS)/common \
					$(SDK_OBJ_DIRS)/platform/os/$(SDK_PLATFORM_OS) \
					$(SDK_OBJ_DIRS)/platform/ssl/$(SDK_SSL) \
					$(SDK_OBJ_DIRS)/public/net/tcp \
					$(SDK_OBJ_DIRS)/public/net/ssl\
					$(SDK_OBJ_DIRS)/src/jclient \
					$(SDK_OBJ_DIRS)/src/sisclient \
					$(SDK_OBJ_DIRS)/src/mqtt/MQTTClient-C/src \
					$(SDK_OBJ_DIRS)/src/mqtt/MQTTPacket/src 


CFLAGS += -DJIOT_SSL 