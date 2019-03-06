#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "ini_file.h"
#include "param_init.h"

extern REMOTE_UPGRADE_CFG g_remote_upgrade_params; //远程升级配置参数


REMOTE_UPGRADE_CFG *get_remote_upgrade_cfg()
{
	return &g_remote_upgrade_params;
}


static int get_devid(char *devid) //获取设备ID
{
	if(devid == NULL)
		return FAIL;
	
	void* cfg_ctx = NULL;
	char* tmp = NULL;
	if(init_config_file(SYSINFO_INI_PATH) != 0)
	{
		printf("---- init config file fail!!\n");
		term_config_file(SYSINFO_INI_PATH);
		return FAIL;
	}
	cfg_ctx = parse_ini_file(SYSINFO_INI_PATH);
	if(cfg_ctx == NULL)
	{
		printf("---- parse ini file error,path:%s\n", SYSINFO_INI_PATH);
		term_config_file(SYSINFO_INI_PATH);
		return FAIL;
	}

	/*获取盒子ID*/
	tmp = get_item_value(cfg_ctx, "INIT_PARAM_THEME", "dev_id", "157AD18120001101");
	strncpy(devid, tmp, DEVID_MAX_LEN);
	devid[DEVID_MAX_LEN-1] = '\0';//添加结束符

	/*关闭配置文件*/
	close_ini_file(cfg_ctx);	
	term_config_file(SYSINFO_INI_PATH);
	
	return SUCCESS;
}

int load_remote_upgrade_param(REMOTE_UPGRADE_CFG *params)	//加载配置文件
{
	memset(params,0,sizeof(REMOTE_UPGRADE_CFG));
	
	void* cfg_ctx = NULL;
	char* tmp = NULL;
	if(init_config_file(REMOTE_UPGRADE_INI_PATH) != 0)
	{
		printf("---- init config file fail!!\n");
		term_config_file(REMOTE_UPGRADE_INI_PATH);
		return FAIL;
	}
	
	cfg_ctx = parse_ini_file(REMOTE_UPGRADE_INI_PATH);
	if(cfg_ctx == NULL)
	{
		printf("---- parse ini file error,path:%s\n", REMOTE_UPGRADE_INI_PATH);
		term_config_file(REMOTE_UPGRADE_INI_PATH);
		return FAIL;
	}
	
	/*获取mqtt服务器主机地址*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_HOST, "52.82.73.175");
	strncpy(params->host, tmp, sizeof(params->host));
	params->host[CFG_MAX_LEN-1] = '\0';//添加结束符

	/*获取mqtt服务器端口号*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_PORT, "1883");
	params->port = atoi(tmp);

	/*获取mqtt服务器连接用户名*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_USERNAME, "null");
	strncpy(params->username, tmp, sizeof(params->username));
	params->username[CFG_MAX_LEN-1] = '\0';//添加结束符

	
	/*获取mqtt服务器连接密码*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_PASSWORD, "null");
	strncpy(params->password, tmp, sizeof(params->password));
	params->password[CFG_MAX_LEN-1] = '\0';//添加结束符

	
	/*获取mqtt服务器心跳周期*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_HEARTBEAT, "60");
 	params->hearbeat = atoi(tmp);


	/*获取ftp用户及密码*/
	tmp = get_item_value(cfg_ctx, FTP_CFG, _FTP_USERKEY, "zlgs:zlgs@00157");
	strncpy(params->user_key, tmp, sizeof(params->user_key));
	params->user_key[CFG_MAX_LEN-1] = '\0';//添加结束符


	/*获取远程升级程序版本*/
	tmp = get_item_value(cfg_ctx, VERSION_CFG, _REMOTE_UPGRADE_VER, "2.255.1.0");
	strncpy(params->upgrade_version , tmp, sizeof(params->upgrade_version));
	params->upgrade_version[VER_MAX_LEN-1] = '\0';//添加结束符

	/*获取服务程序版本*/
	tmp = get_item_value(cfg_ctx, VERSION_CFG, _SERVICE_VER, "2.0.1.0");
	strncpy(params->service_version , tmp, sizeof(params->service_version));
	params->service_version[VER_MAX_LEN-1] = '\0';//添加结束符

	/*获取MCU程序版本*/
	tmp = get_item_value(cfg_ctx, VERSION_CFG, _MCU_VER, "2.254.1.0");
	strncpy(params->mcu_version , tmp, sizeof(params->mcu_version));
	params->mcu_version[VER_MAX_LEN-1] = '\0';//添加结束符

	/*获取其他程序版本*/
	tmp = get_item_value(cfg_ctx, VERSION_CFG, _OTHER_VER, "2.20.1.0");
	strncpy(params->other_version , tmp, sizeof(params->other_version));
	params->other_version[VER_MAX_LEN-1] = '\0';//添加结束符

	/*获取设备类型*/
	tmp = get_item_value(cfg_ctx, OTHER_CFG, _DEV_TYPE, "unknow");
	strncpy(params->dev_type , tmp, sizeof(params->dev_type));
	params->dev_type[DEV_TYPE_MAX_LEN-1] = '\0';//添加结束符

	/*获取盒子ID*/
	get_devid(params->devid);

	/*关闭配置文件*/
	close_ini_file(cfg_ctx);	
	term_config_file(REMOTE_UPGRADE_INI_PATH);
	
	return SUCCESS;
}

int set_remote_upgrade_param(char* section, char *keyname, char *value)	//设置远程升级参数
{
	void* cfg_ctx = NULL;
	int tmp = 0;
	if(init_config_file(REMOTE_UPGRADE_INI_PATH) != 0)
	{
		printf("---- init config file fail!!\n");
		term_config_file(REMOTE_UPGRADE_INI_PATH);
		return FAIL;
	}
	
	cfg_ctx = parse_ini_file(REMOTE_UPGRADE_INI_PATH);
	if(cfg_ctx == NULL)
	{
		printf("---- parse ini file error,path:%s\n", REMOTE_UPGRADE_INI_PATH);
		term_config_file(REMOTE_UPGRADE_INI_PATH);
		return FAIL;
	}

	tmp = set_item_value(cfg_ctx, section, keyname, value);
	if(tmp != 0)
	{
		printf("---- set %s fail,Error code:%d",keyname, tmp);
		close_ini_file(cfg_ctx);
		term_config_file(REMOTE_UPGRADE_INI_PATH);
		return FAIL;
	}
	
	/*关闭配置文件*/
	close_ini_file(cfg_ctx);
	term_config_file(REMOTE_UPGRADE_INI_PATH);
	
	return SUCCESS;
}




