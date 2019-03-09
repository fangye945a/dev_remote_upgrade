#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "cJSON.h"
#include "mqtt.h"
#include "ftp-manager.h"
#include "param_init.h"
#include "msg_process.h"
#include "mqtt_msg_proc.h"
#include "upgrade_proc.h"

REMOTE_UPGRADE_CFG g_remote_upgrade_params; //远程升级配置参数
tmsg_buffer* main_process_msg = NULL;

volatile unsigned char main_running_flag = 1;


static void main_exit_proc(int signal)
{
	switch(signal)
	{
		case SIGINT:
			if(main_process_msg != NULL)
			{
				main_running_flag = 0;
			}
			break;
		default:
			break;
	}
}

int main(int argc, char *argv[])
{
	int ret = -1;
	tmsg_element* event = NULL;
	
	/*接收用户中断信息号,退出程序*/
	signal(SIGINT, main_exit_proc);
	
	main_process_msg = msg_buffer_init();	//消息队列初始化
	if(main_process_msg == NULL)
	{
		printf("main msg queue init fail!\n");
		return FAIL;
	}

	load_remote_upgrade_param(&g_remote_upgrade_params); 	//加载远程升级参数
	printf("devid = %s\n",g_remote_upgrade_params.devid);
	printf("hearbeat = %d\n",g_remote_upgrade_params.hearbeat);
	printf("host = %s\n",g_remote_upgrade_params.host);
	printf("port = %d\n",g_remote_upgrade_params.port);
	printf("username = %s\n",g_remote_upgrade_params.username);
	printf("password = %s\n",g_remote_upgrade_params.password);
	printf("user_key = %s\n",g_remote_upgrade_params.user_key);
	printf("upgrade_version = %s\n",g_remote_upgrade_params.upgrade_version);
	printf("service_version = %s\n",g_remote_upgrade_params.service_version);
	printf("mcu_version = %s\n",g_remote_upgrade_params.mcu_version);
	printf("other_version = %s\n",g_remote_upgrade_params.other_version);
	printf("dev_type = %s\n",g_remote_upgrade_params.dev_type);

	ftp_option_init(g_remote_upgrade_params.user_key); 				//FTP参数初始化

	mqtt_params_init(&g_remote_upgrade_params);				//mqtt参数初始化

	StartMqttTask(); //开启MQTT任务

	while (main_running_flag)
    {
		event = main_process_msg->get_timeout(main_process_msg,500);
		if(event != NULL)
		{
			switch (event->msg)
			{
				case MQTT_MSG:
				{
					MQTT_MESSAGE *message = (MQTT_MESSAGE *)event->dt;
					mqtt_msg_proc(message->topic, (char *)message->payload, message->payloadlen);
				}break;
				case FTP_RESULT:
				{
					ftp_pthread_id_clean();
					switch(event->ext)
					{							
						case FTP_UPLOAD_FAIL_NOT_EXIST:
						case FTP_UPLOAD_FAILED:
							printf("--------- upload fail!\n");
							pub_trans_progress(UPLOAD_FAIL_STAT, 0, 0); //上报结果
							break;
						case FTP_UPLOAD_SUCCESS:
							pub_trans_progress(UPLOAD_SUCC_STAT, 0, 0);
							printf("--------- upload success!\n");
							break;
						case FTP_DOWNLOAD_PATH_ERROR:
						case FTP_DOWNLOAD_FAILED:
							pub_trans_progress(DOWNLOAD_FAIL_STAT, 0, 0);
							printf("--------- download fail!\n");
							break;
						case FTP_DOWNLOAD_SUCCESS:
							pub_trans_progress(DOWNLOAD_SUCC_STAT, 0, 0);
							printf("--------- download success!\n");
							break;
						case FTP_BUZY:
							printf("--------- ftp pthread busy!!\n");
						default:break;
					}
				}break;
				case REMOTE_UPGRADE_PROC:
				{
					switch(event->ext)
					{
						case SERVICE_UPDATE:
						{
							ret = upgrade_service_part(); //升级服务程序区域
						}break;
						case APP_UPDATE:
						{
							ret = upgrade_apps_part();   //升级应用程序区域
							if(judge_is_add_app()) //如果是添加应用
							{
								APP_MANAGE_ADD_OPT *opt = get_add_app_st();
								run_application(opt->add_app_name);  //添加启动脚本
								add_app_clear();	//清楚结构体
							}
						}break;
						case MCU_UPDATE:
						{
							upgrade_mcu_exe();     //升级单片机
						}break;
						case PLC_UPDATE:
						{
							upgrade_plc_exe();     //升级PLC
						}break;
						default:break;
					}
				}break;
				case SYS_REBOOT:
				{
					ret = SUCCESS;
				}break;					
			}
			free_tmsg_element(event);
			
			if(ret == SUCCESS)		//如果升级成功则退出线程
			{
				system("sync"); //将系统缓存数据同步到磁盘
				sleep(1);
				break;
			}
		}
		
    }
	ExitMqttTask();	//退出MQTT任务
	
	if(main_running_flag)  //如果不是用户退出则重启
		system("reboot");
	return 0;
}
