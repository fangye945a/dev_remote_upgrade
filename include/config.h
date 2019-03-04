/******************************************************************************
                  版权所有 (C), 2019-2099, 中联重科

-->文件名	: config.h
-->作  者	: fangye
-->生成日期	: 2019年3月4日
-->功能描述	: config.h 的头文件
******************************************************************************/
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define FAIL 0
#define SUCCESS 1

#define MQTT_HOST "52.82.73.175"        			//mqtt 主机地址
#define MQTT_PORT 1883                  			//mqtt 端口号
#define DEV_ID 	  "15A6002A10001001"    			//设备ID
#define QOS 	  1                     			//mqtt消息质量
#define USRNAME		" "								//mqtt连接用户名
#define PASSWORD	" "								//mqtt连接密码
#define WILL_TOPIC	"last_word"						//mqtt遗言主题
#define WILL_PAYLOAD	"{\"good\":\"bye\"}"		//mqtt遗言内容


#define FTP_USERKEY	"zlgs:zlgs@00157"				//ftp用户名及密码




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONFIG_H__ */
