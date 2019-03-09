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

enum
{
	MQTT_MSG,	//MQTT消息
	FTP_RESULT,	//FTP执行结果
	REMOTE_UPGRADE_PROC, //升级处理
	SYS_REBOOT			//系统重启
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONFIG_H__ */
