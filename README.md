# JIoT-SDK-C 

### 简介

极光 IoT 是极光面向物联网开发者推出的 SaaS 服务平台，依托于极光在开发者服务领域的技术积累能力。专门为 IoT 设备优化协议，提供高并发，高覆盖，高可用的设备接入及消息通信能力。同时针对物联网使用场景提供安全连接，实时统计，设备管理 ，影子设备等一些列解决方案。

### 接入方式

- 通过极光的官方网站注册开发者帐号；
- 登录进入管理控制台，创建应产品，得到 ProductKey（ProductKey 与服务器端通过 Appkey 互相识别）；
- 在产品设置中为产品完善属性设置，上报事件设置。
- 为产品添加设备：定义设备名，并获得分配的设备密钥。
- 通过SDK中的demo进行调试。



### 平台支持

- x86(linux)
- armv7(linux)
- esp8266(freeRTOS, wifi)
- stm32(rt-thread)
- m5311(freeRTOS, nbiot)


### 相关文档



- [JIoT C SDK 接口](https://docs.jiguang.cn/jiot/client/c_sdk_api)
- [JIoT C SDK OTA说明](https://docs.jiguang.cn/jiot/client/c_sdk_ota_readme)
- [JIoT C SDK armv7集成指南](https://docs.jiguang.cn/jiot/client/c_sdk_armv7_guide)
- [JIoT C SDK x86集成指南](https://docs.jiguang.cn/jiot/client/c_sdk_x86_guide)
- [JIoT C SDK esp8266集成指南](https://docs.jiguang.cn/jiot/client/c_sdk_8266_guide)
- [JIoT C SDK rt-thread集成指南](https://docs.jiguang.cn/jiot/client/c_sdk_rt_thread_guide)
- [JIoT C SDK m5311集成指南](https://docs.jiguang.cn/jiot/client/c_sdk_m5311_guide)