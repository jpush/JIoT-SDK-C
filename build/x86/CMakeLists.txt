
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (jiot-embedded-V1.0.0 C)

SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET (CMAKE_C_FLAGS " -g  -Wall -fPIC")
SET (CMAKE_VERBOSE_MAKEFILE on )
#set (CMAKE_BUILD_TYPE "Debug")

SET (SDK_PLATFORM_OS linux CACHE OS "os" )
SET (SDK_SSL openssl CACHE SSL  "ssl")
SET (CMAKE_INSTALL_PREFIX CACHE INSTALL_PATH "install path")
SET (JIOT_SDK_VERSION "v1.0.12" CACHE SDK_VERSION "sdk version")

SET (PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../..)
SET (CMAKE_SDK_PLATFORM_OS_PATH ${PROJECT_SOURCE_DIR}/platform/os/${SDK_PLATFORM_OS})
SET (CMAKE_SDK_SSL_PATH ${PROJECT_SOURCE_DIR}/platform/ssl/${SDK_SSL})


MESSAGE ("---------------------------------------------------------------------")
MESSAGE ("Project Name              : " ${PROJECT_NAME})
MESSAGE ("Source Dir                : " ${PROJECT_SOURCE_DIR})
MESSAGE ("Binary Dir                : " ${PROJECT_BINARY_DIR})
MESSAGE ("System Processor          : " ${CMAKE_SYSTEM_PROCESSOR})
MESSAGE ("System Platform           : " ${CMAKE_SYSTEM})
MESSAGE ("C Compiler                : " ${CMAKE_C_COMPILER})
MESSAGE ("Executable Dir            : " ${EXECUTABLE_OUTPUT_PATH})
MESSAGE ("Library Dir               : " ${LIBRARY_OUTPUT_PATH})
MESSAGE ("SSL_HOME                  : " $ENV{SSL_HOME} )
MESSAGE ("SDK Version               : " ${JIOT_SDK_VERSION})
MESSAGE ("CMAKE_SDK_PLATFORM_OS_PATH: " ${CMAKE_SDK_PLATFORM_OS_PATH})
MESSAGE ("CMAKE_SDK_SSL_PATH        : " ${CMAKE_SDK_SSL_PATH})
MESSAGE ("CMAKE_INSTALL_PREFIX      : " ${CMAKE_INSTALL_PREFIX})


ADD_DEFINITIONS (-DSDK_BUILDID="${JIOT_SDK_BUILDID}")
ADD_DEFINITIONS (-DALCS_ENABLED)
ADD_DEFINITIONS (-DJIOT_SSL)
#ADD_DEFINITIONS (-DJIOT_SSL_VERIFY_CA) #enable for test cert

INCLUDE_DIRECTORIES ($ENV{SSL_HOME}/include)

INCLUDE_DIRECTORIES (${CMAKE_SDK_PLATFORM_OS_PATH})

INCLUDE_DIRECTORIES (${CMAKE_SDK_SSL_PATH})

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/public/net/tcp)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/public/net/ssl)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/common)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/include/mqtt)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/include/jclient)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/include/sisclient)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src)


ADD_LIBRARY (c_sdk OBJECT

    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_err.c
    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_log.c
    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_std.c
    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_network.c
    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_pthread.c
    ${CMAKE_SDK_PLATFORM_OS_PATH}/jiot_timer.c

    ${CMAKE_SDK_SSL_PATH}/jiot_network_ssl.c

    ${PROJECT_SOURCE_DIR}/public/net/tcp/jiot_tcp_net.c
    ${PROJECT_SOURCE_DIR}/public/net/ssl/jiot_ssl_net.c

    ${PROJECT_SOURCE_DIR}/common/jiot_cJSON.c
    ${PROJECT_SOURCE_DIR}/common/jiot_list.c
    ${PROJECT_SOURCE_DIR}/common/jiot_httpclient.c
    ${PROJECT_SOURCE_DIR}/common/jiot_common.c

    ${PROJECT_SOURCE_DIR}/src/jclient/jiot_client.c
    ${PROJECT_SOURCE_DIR}/src/sisclient/jiot_sis_client.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTClient-C/src/jiot_mqtt_client.c
    
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTPacket.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTSerializePublish.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTSubscribeClient.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTDeserializePublish.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTUnsubscribeClient.c
    ${PROJECT_SOURCE_DIR}/src/mqtt/MQTTPacket/src/MQTTConnectClient.c
    ${EXTRA_C_SOURCES}
)

ADD_LIBRARY (jiot_c_sdk STATIC
    $<TARGET_OBJECTS:c_sdk>
)

# install
set(JIOT_SDK_INCLUDES "${PROJECT_SOURCE_DIR}/include/jclient")
set(JIOT_SDK_EXAMPLES "${PROJECT_SOURCE_DIR}/examples")
install(DIRECTORY ${LIBRARY_OUTPUT_PATH}/ DESTINATION lib)
install(DIRECTORY ${JIOT_SDK_INCLUDES}/ DESTINATION include/jclient FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${JIOT_SDK_EXAMPLES}/linux DESTINATION examples)




