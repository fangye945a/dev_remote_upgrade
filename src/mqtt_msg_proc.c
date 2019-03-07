#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "mqtt.h"
#include "config.h"
#include "ftp-manager.h"
#include "param_init.h"
#include "upgrade_proc.h"

int upload_proc(char *payload)  		//上传处理
{
	cJSON *root = cJSON_Parse(payload);
	if(root == NULL)
		return FAIL;
	
	cJSON *item_path = cJSON_GetObjectItem(root, "path");
	cJSON *item_url = cJSON_GetObjectItem(root, "url");
	if(item_path == NULL || item_url == NULL)
	{
		cJSON_Delete(root);
		return FAIL;
	}
	
	ftp_upload(item_path->valuestring, item_url->valuestring);

	cJSON_Delete(root);

	return SUCCESS;
}

int download_proc(char *payload)  		//下载处理
{
	cJSON *root = cJSON_Parse(payload);
	if(root == NULL)
		return FAIL;
	
	cJSON *item_path = cJSON_GetObjectItem(root, "path");
	cJSON *item_url = cJSON_GetObjectItem(root, "url");
	if(item_path == NULL || item_url == NULL)
	{
		cJSON_Delete(root);
		return FAIL;
	}
	
	ftp_download(item_path->valuestring, item_url->valuestring);

	cJSON_Delete(root);

	return SUCCESS;
}

int setting_proc(char *payload)  		//参数设置处理
{
	return FAIL;;
}

int upgrade_proc(char *payload)  		//升级处理
{
	cJSON *root = cJSON_Parse(payload);
	if(root == NULL)
		return FAIL;
	
	cJSON *upgrade_type = cJSON_GetObjectItem(root, "upgrade_type");
	cJSON *item_url = cJSON_GetObjectItem(root, "url");
	if(upgrade_type == NULL || item_url == NULL)
	{
		cJSON_Delete(root);
		return FAIL;
	}

	if( !strcmp(upgrade_type->valuestring,"service")) //升级服务程序
	{
		ftp_download(SERVICE_PAKAGE_PATH, item_url->valuestring);
	}
	else if(!strcmp(upgrade_type->valuestring,"app")) //升级应用程序
	{
		ftp_download(APP_PAKAGE_PATH, item_url->valuestring);
	}
	else if(!strcmp(upgrade_type->valuestring,"mcu")) //升级MCU程序
	{
		ftp_download(MCU_EXE_PATH, item_url->valuestring);
	}
	else if(!strcmp(upgrade_type->valuestring,"plc")) //升级其他程序
	{
		ftp_download(PLC_EXE_PATH, item_url->valuestring);
	}

	cJSON_Delete(root);

	return SUCCESS;
}

int app_manage_proc(char *payload)  		//APP管理处理
{
	return FAIL;
}

int get_run_info_proc(char *payload)  			//获取运行信息处理
{
	return FAIL;
}

int get_dir_info_proc(char *payload)	  		//获取目录信息处理
{
	return FAIL;
}



int mqtt_msg_proc(char *topic, char *payload, int payloadlen)
{
	if(topic == NULL || payload == NULL || payloadlen == 0)
		return FAIL;

	int ret = FAIL;
	if(strstr(topic,"online_ask"))
	{
		pub_login_msg(); 		//在线查询 发布上线信息
	}
	else if(strstr(topic,"upload"))
	{
		ret = upload_proc(payload); //上传文件
	}
	else if(strstr(topic,"download"))
	{
		ret = download_proc(payload); //下载文件
	}
	else if(strstr(topic,"setting"))
	{
		ret = setting_proc(payload); //参数配置
	}
	else if(strstr(topic,"upgrade"))
	{
		ret = upgrade_proc(payload);  //升级处理
	}
	else if(strstr(topic,"app_manage"))
	{
		ret = app_manage_proc(payload); //APP管理
	}
	else if(strstr(topic,"get_run_info"))
	{
		ret = get_run_info_proc(payload); //获取运行信息
	}
	else if(strstr(topic,"get_dir_info"))
	{
		ret = get_dir_info_proc(payload); //获取目录结构
	}

	if(ret == FAIL)
	{
		printf("---------- Unknow topic or msg!!\ntopic=%s\tmsg=%s\n",topic,payload);
	}
	
	return ret;
}
