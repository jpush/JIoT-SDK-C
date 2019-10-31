Import('RTT_ROOT')
from building import *

# get current directory
cwd = GetCurrentDir()

# The set of source files associated with this SConscript file.
src = []
path = []

src += Glob('jiot-c-sdk/src/jclient/*.c')
src += Glob('jiot-c-sdk/src/sisclient/*.c')
src += Glob('jiot-c-sdk/src/mqtt/MQTTClient-C/src/*.c')
src += Glob('jiot-c-sdk/src/mqtt/MQTTPacket/src/*.c')
src += Glob('jiot-c-sdk/common/*.c')
src += Glob('jiot-c-sdk/public/net/tcp/*.c')
src += Glob('jiot-c-sdk/platform/os/rt-thread/*.c')

path += [cwd + '/jiot-c-sdk/common']
path += [cwd + '/jiot-c-sdk/include/jclient']
path += [cwd + '/jiot-c-sdk/include/mqtt']
path += [cwd + '/jiot-c-sdk/include/sisclient']
path += [cwd + '/jiot-c-sdk/platform/os/rt-thread']
path += [cwd + '/jiot-c-sdk/src/mqtt/MQTTPacket/src']
path += [cwd + '/jiot-c-sdk/public/net/tcp']

if GetDepend(['PKG_USING_JIOT_EXAMPLES']):
	src += Glob('samples/*.c')
	
if GetDepend(['JIOT_SSL']):

	src += Glob('jiot-c-sdk/public/net/ssl/*.c')
	src += Glob('jiot-c-sdk/platform/ssl/mbedtls/*.c')
	path += [cwd + '/jiot-c-sdk/platform/ssl/mbedtls']
	path += [cwd + '/jiot-c-sdk/public/net/ssl']
	
group = DefineGroup('jiot-c-sdk', src, depend = [''], CPPPATH = path)

Return('group')
