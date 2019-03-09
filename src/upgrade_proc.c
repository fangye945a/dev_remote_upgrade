#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "upgrade_proc.h"
#include "config.h"

//#include "msg_process.h"
//extern tmsg_buffer* main_process_msg;

APPS_INFO g_apps_info;

APPS_INFO *get_apps_info() //获取app信息
{
	return &g_apps_info;
}

int is_app_running(char *app_name)
{
	int i=0;
	for(i=0; i<g_apps_info.run_apps_num; i++)
	{
		if( !strcmp(g_apps_info.run_apps_name[i], app_name) )
		{
			return SUCCESS;
		}
	}
	return FAIL;
}

static int check_exist_apps() //检查存在的app
{
	DIR *dp = NULL;
	struct dirent *st = NULL;
	struct stat sta;
	int ret = -1;
	char tmp_name[1024]={0};
	dp = opendir(APPS_DIR);
	if(dp != NULL)
	{
		while(1)
		{
			st = readdir(dp);
			if(NULL == st) //读取完毕
			{
				break;
			}
			strcpy(tmp_name, APPS_DIR);
			if(tmp_name[strlen(tmp_name)-1] != '/') //判断路径名是否带/
				strcat(tmp_name,"/");
			strcat(tmp_name,st->d_name);  //新文件路径名
			ret = stat(tmp_name, &sta); //查看目录下文件属性
			if(ret >= 0)
			{
				if(S_ISDIR(sta.st_mode)) //如果为目录文件
				{
					if( 0 == strcmp("..",st->d_name) || 0 == strcmp(".",st->d_name)) //忽略当前目录和上一层目录
						continue;
					else
					{
						if(g_apps_info.exist_apps_num < MAX_APP_NUM)
						{
							strncpy(g_apps_info.exist_apps_name[g_apps_info.exist_apps_num],st->d_name,APP_NAME_MAX_LEN);
							g_apps_info.exist_apps_name[g_apps_info.exist_apps_num][APP_NAME_MAX_LEN-1]='\0';
							g_apps_info.exist_apps_num++;
						}
					}
				}
			}
		}
		closedir(dp);
	}
	return SUCCESS;
}

static int check_run_apps() //检查运行的app
{
	DIR *dp = NULL;
	struct dirent *st = NULL;
	struct stat sta;
	int ret = -1;
	char tmp_name[1024]={0};
	dp = opendir(MONIT_CFG_DIR);
	if(dp != NULL)
	{
		while(1)
		{
			st = readdir(dp);
			if(NULL == st) //读取完毕
			{
				break;
			}
			strcpy(tmp_name, MONIT_CFG_DIR);
			if(tmp_name[strlen(tmp_name)-1] != '/') //判断路径名是否带/
				strcat(tmp_name,"/");
			strcat(tmp_name,st->d_name);  //新文件路径名
			ret = stat(tmp_name, &sta); //查看目录下文件属性
			if(ret >= 0)
			{
				if(!S_ISDIR(sta.st_mode)) //如果不为目录文件
				{
					if(g_apps_info.run_apps_num < MAX_APP_NUM)
					{
						strncpy(g_apps_info.run_apps_name[g_apps_info.run_apps_num],st->d_name,APP_NAME_MAX_LEN);
						g_apps_info.run_apps_name[g_apps_info.run_apps_num][APP_NAME_MAX_LEN-1]='\0';
						g_apps_info.run_apps_num++;
					}
				}
			}
		}
		closedir(dp);
	}
	return SUCCESS;
}

int app_info_check() //检查应用信息
{	
	memset(&g_apps_info,0,sizeof(APPS_INFO));

	check_exist_apps(); //检查存在的APP

	check_run_apps();  	//检查运行的APP
}

static int get_real_name(char *path, char *name,int name_size)
{
	if(path == NULL || name == NULL || name_size ==0)
			return FAIL;
	char *ptr = strrchr(path,'/');
	bzero(name,name_size); //清空缓存区
	strncpy(name,ptr+1,name_size-1);
	return SUCCESS;
}

int creat_monit_file(char *exec_path) //生成启动文件
{
	if(exec_path == NULL)
		return FAIL;
	char real_name[APP_NAME_MAX_LEN]={0};
	char wt_path[256]={0}; //写入的路径
	char buf[1024]={0};    //写入的内容
	FILE *fp = NULL;
	if(FAIL == get_real_name(exec_path,real_name,APP_NAME_MAX_LEN) )
		return FAIL;	
	sprintf(wt_path,"%s/%s",MONIT_CFG_DIR,real_name);
	fp = fopen(wt_path,"w+");
	if(fp == NULL)
		return FAIL;
	
	fprintf(fp,MONIT_CFG,real_name,exec_path); //写入文件
	fclose(fp);
	return SUCCESS;
}

//int remove_monit_file(char *exec_path) //删除启动文件
//{
//	if(exec_path == NULL)
//		return FAIL;
//	char real_name[APP_NAME_MAX_LEN]={0};
//	char cmd[256]={0};
//	if(FAIL == get_real_name(exec_path,real_name,APP_NAME_MAX_LEN) )
//		return FAIL;
//	sprintf(cmd,"rm /usrdata/service/monit.d/%s -rf",real_name);
//	
//	system(cmd);
//	system("sync");
//	return SUCCESS;
//}

int upgrade_service_part() //升级服务程序区域
{
	char cmd[256]={0};
	if( access(SERVICE_PAKAGE_PATH,F_OK) )//判断程序是否存在
	{
		printf("file is not exist!!\n"); //文件不存在
		return FAIL;
	}
	sprintf(cmd,"tar zxf %s -C /usrdata/service",SERVICE_PAKAGE_PATH);
	system(cmd); //解压升级文件
	sleep(1);
	system("chmod 777 /usrdata/service -R");//赋予权限
	return SUCCESS;
}

int upgrade_apps_part()				//升级apps程序区域
{
	char cmd[256]={0};
	if( access(APP_PAKAGE_PATH,F_OK) )//判断程序是否存在
	{
		printf("file is not exist!!\n"); //文件不存在
		return FAIL;
	}
	sprintf(cmd, "tar zxf %s -C /usrdata/apps", APP_PAKAGE_PATH);
	system(cmd);
	sleep(1);
	system("chmod 777 /usrdata/apps -R");//赋予权限
	return SUCCESS;
}

int upgrade_mcu_exe()				//升级MCU程序区域
{
	return FAIL;
}

int upgrade_plc_exe()				//升级其他程序
{
#ifdef ARM_EC20

#endif
	printf("------------- upgrade_plc_exe!\n");
	return SUCCESS;
}
