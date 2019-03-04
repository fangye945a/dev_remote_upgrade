#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "ini_file.h"
#include "param_init.h"

int load_remote_upgrade_param(REMOTE_UPGRADE_CFG *params)	//加载配置文件
{
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
	params->host[CFG_MAX_LEN-1] = '\0';//添加结束符

	/*获取mqtt服务器连接密码*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_PASSWORD, "null");
	strncpy(params->password, tmp, sizeof(params->password));
	params->host[CFG_MAX_LEN-1] = '\0';//添加结束符

	/*获取mqtt服务器心跳周期*/
	tmp = get_item_value(cfg_ctx, MQTT_CFG, _MQTT_HEARTBEAT, "60");
 	params->hearbeat = atoi(tmp);

	/*获取ftp用户及密码*/
	tmp = get_item_value(cfg_ctx, FTP_CFG, _FTP_USERKEY, "zlgs:zlgs@00157");
	strncpy(params->user_key, tmp, sizeof(params->user_key));
	params->host[CFG_MAX_LEN-1] = '\0';//添加结束符

	/*获取远程升级程序版本*/
	tmp = get_item_value(cfg_ctx, OTHER_CFG, _REMOTE_UPGRADE_VER, "v1.0");
	strncpy(params->version , tmp, sizeof(params->version));
	params->host[VER_MAX_LEN-1] = '\0';//添加结束符

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




