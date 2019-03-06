/******************************************************************************
                  版权所有 (C), 2019-2099, 中联重科

-->文件名	: param_init.h
-->作  者	: fangye
-->生成日期	: 2019年3月4日
-->功能描述	: param_init.c 的头文件
******************************************************************************/
#ifndef __PARAM_INIT_H__
#define __PARAM_INIT_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef ARM_EC20
	#define REMOTE_UPGRADE_INI_PATH "../cfg/remote_upgrade.ini"	//配置文件路径
	#define SYSINFO_INI_PATH "../cfg/syscfg.ini"	//配置文件路径
#else
	#define REMOTE_UPGRADE_INI_PATH "/usrdata/service/etc/remote_upgrade.ini"	//配置文件路径
	#define SYSINFO_INI_PATH "/usrdata/service/etc/syscfg.ini"
#endif

#define MQTT_CFG "MQTT_CFG"
#define _MQTT_HOST "mqtt_host"						//mqtt服务器主机名
#define _MQTT_PORT "mqtt_port"						//mqtt服务器端口号
#define _MQTT_USERNAME "mqtt_username"				//mqtt连接用户名
#define _MQTT_PASSWORD "mqtt_password"				//mqtt连接密码
#define _MQTT_HEARTBEAT "mqtt_heartbeat"			//mqtt心跳周期

#define FTP_CFG "FTP_CFG"
#define _FTP_USERKEY "user_key"						//ftp服务器用户名及密码

#define OTHER_CFG "OTHER_CFG"
#define _REMOTE_UPGRADE_VER "remote_upgrade_version"	//升级程序版本

#define CFG_MAX_LEN	32
#define VER_MAX_LEN	24
#define DEVID_MAX_LEN	32


typedef struct _REMOTE_UPGRADE_CFG
{
	unsigned char host[CFG_MAX_LEN];  		//mqtt服务器主机名
	unsigned char username[CFG_MAX_LEN];		//mqtt服务器连接用户名
	unsigned char password[CFG_MAX_LEN];		//mqtt服务器连接密码
	unsigned char user_key[CFG_MAX_LEN]; 	//ftp验证用户及密码
	unsigned char version[VER_MAX_LEN];    	//远程升级程序版本号
	unsigned char devid[DEVID_MAX_LEN];    	//设备ID
	int	port;						//mqtt端口号
	int	hearbeat;					//mqtt心跳周期
}REMOTE_UPGRADE_CFG;


extern int load_remote_upgrade_param(REMOTE_UPGRADE_CFG *params);	//加载配置文件
extern int set_remote_upgrade_param(char* section, char *keyname, char *value);	//设置远程升级参数



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PARAM_INIT_H__ */
