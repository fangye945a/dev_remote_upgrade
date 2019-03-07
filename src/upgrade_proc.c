#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "upgrade_proc.h"
#include "config.h"

//#include "msg_process.h"
//extern tmsg_buffer* main_process_msg;

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
	return FAIL;
}
