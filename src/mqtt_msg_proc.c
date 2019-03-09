#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "mqtt.h"
#include "config.h"
#include "ftp-manager.h"
#include "param_init.h"
#include "upgrade_proc.h"
#include "msg_process.h"
#include "mqtt_msg_proc.h"

extern tmsg_buffer* main_process_msg;


APP_MANAGE_ADD_OPT add_app;

void add_app_clear()
{
	memset(&add_app,0,sizeof(APP_MANAGE_ADD_OPT));
}

int judge_is_add_app()
{
	return add_app.add_app_flag;
}

APP_MANAGE_ADD_OPT *get_add_app_st()
{
	return &add_app;
}

int add_application(char *app_name, char *url) //添加应用
{
	app_info_check();	//检查应用
	APPS_INFO *info = get_apps_info();
	int i = 0;
	int exist_flag = 0;  //应用存在标志
	int run_flag = 0;	 //应用运行标志
	for(i = 0; i< info->run_apps_num; i++)
	{
		if(!strcmp(info->run_apps_name[i],app_name))
		{
			run_flag = 1;
		}
	}
	
	for(i = 0; i< info->exist_apps_num; i++)
	{
		if(!strcmp(info->exist_apps_name[i],app_name))
		{
			exist_flag = 1;
		}
	}

	if(exist_flag == 0) //应用不存在则下载
	{
		add_app.add_app_flag = 1; 				//添加应用
		strncpy(add_app.add_app_name ,app_name, 64);
		add_app.add_app_name[63]= '\0';
		ftp_download(APP_PAKAGE_PATH, url);
	}
	return SUCCESS;
}

int del_application(char *app_name) //删除应用
{
	
	printf("---------start del application!\n");
	app_info_check();	//检查应用
	APPS_INFO *info = get_apps_info();
	int i = 0;
	int exist_flag = 0;  //应用存在标志
	int run_flag = 0;	 //应用运行标志
	for(i = 0; i< info->run_apps_num; i++)
	{
		if(!strcmp(info->run_apps_name[i],app_name))
		{
			run_flag = 1;
		}
	}
	
	for(i = 0; i< info->exist_apps_num; i++)
	{
		if(!strcmp(info->exist_apps_name[i],app_name))
		{
			exist_flag = 1;
		}
	}

	if(exist_flag == 1) //如果存在
	{
		char cmd[256]={0}; //shell命令
		sprintf(cmd, "rm /usrdata/apps/%s -rf", app_name);
		system(cmd); 						//删除应用程序
		if(run_flag == 1) //如果运行
		{
			char stop_cmd[256]={0}; //启动脚本路径
			sprintf(stop_cmd, "rm /usrdata/service/monit.d/%s -rf", app_name);  //删除启动脚本
			system(stop_cmd);
			main_process_msg->sendmsg(main_process_msg,SYS_REBOOT,0,NULL,0); //重启系统
		}
		printf("--------- del application success!\n");
	}
	
	return SUCCESS;
}

int run_application(char *app_name)       //启动应用
{
	app_info_check();	//检查应用
	APPS_INFO *info = get_apps_info();
	int i = 0;
	int exist_flag = 0;  //应用存在标志
	int run_flag = 0; 	 //应用运行标志
	for(i = 0; i< info->run_apps_num; i++)
	{
		if(!strcmp(info->run_apps_name[i],app_name))
		{
			run_flag = 1;
		}
	}

	for(i = 0; i< info->exist_apps_num; i++)
	{
		if(!strcmp(info->exist_apps_name[i],app_name))
		{
			exist_flag = 1;
		}
	}

	if(run_flag == 0 && exist_flag == 1) //应用存在但是未运行
	{
		char exec_path[256]={0}; //应用可执行文件路径
		sprintf(exec_path,"/usrdata/apps/%s/%s",app_name,app_name);
		creat_monit_file(exec_path); //生成启动文件
		main_process_msg->sendmsg(main_process_msg,SYS_REBOOT,0,NULL,0); //重启系统
	}

	return SUCCESS;
}

int stop_application(char *app_name)	//停止应用
{
	app_info_check();	//检查应用
	APPS_INFO *info = get_apps_info();
	int i = 0;
	int run_flag = 0;	 //应用运行标志
	
	for(i = 0; i< info->run_apps_num; i++)
	{
		if(!strcmp(info->run_apps_name[i],app_name))
		{
			run_flag = 1;
		}
	}
	
	if(run_flag)	//如果应用在运行
	{
		char stop_cmd[256]={0}; //启动脚本路径
		sprintf(stop_cmd, "rm /usrdata/service/monit.d/%s", app_name);
		system(stop_cmd);
		main_process_msg->sendmsg(main_process_msg,SYS_REBOOT,0,NULL,0); //重启系统
	}
	return SUCCESS;
}

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
	cJSON *root = cJSON_Parse(payload);
	int ret = FAIL;
	if(root == NULL)
		return FAIL;
	cJSON *opt = cJSON_GetObjectItem(root, "opt");
	cJSON *name = cJSON_GetObjectItem(root, "name");
	if(opt != NULL && name != NULL)
	{
		if(!strcmp(opt->valuestring,"add"))        //add 添加应用
		{
			cJSON *url = cJSON_GetObjectItem(root, "url");
			if(url != NULL)
				add_application(name->valuestring, url->valuestring); //添加并启动应用
		}
		else if(!strcmp(opt->valuestring, "del"))   //del  删除应用
		{
			ret = del_application(name->valuestring);
		}
		else if(!strcmp(opt->valuestring, "run")) //start 启动应用
		{
			ret = run_application(name->valuestring);
		}
		else if(!strcmp(opt->valuestring, "stop"))  //stop 停止应用
		{
			ret = stop_application(name->valuestring);
		}
	}
	cJSON_Delete(root);
	root = NULL;
	
	return ret;
}

int get_run_info_proc(char *payload)  			//获取运行信息处理
{
	if(strcmp("{\"ask\":\"run_info\"}", payload))
	{
		return FAIL;
	}
	pub_run_info();   //发布运行状态
	return SUCCESS;
}

int get_dir_info_proc(char *payload)	  		//获取目录信息处理
{
	cJSON *root = cJSON_Parse(payload);
	if(root == NULL)
		return FAIL;
	
	cJSON *dirpath = cJSON_GetObjectItem(root, "dirname");
	if(dirpath == NULL)
	{
		cJSON_Delete(root);
		return FAIL;
	}
	pub_dir_info(dirpath->valuestring);  //发布目录结构
	cJSON_Delete(root);
	return SUCCESS;
}



int mqtt_msg_proc(char *topic, char *payload, int payloadlen)
{
	if(topic == NULL || payload == NULL || payloadlen == 0)
		return FAIL;

	int ret = FAIL;
	if(strstr(topic,"online_ask"))	//上线查询
	{
		if( !strcmp("{\"ask\":\"login_info\"}", payload) )
		{
			pub_login_msg(); 		//在线查询 发布上线信息
			ret = SUCCESS;
		}
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
