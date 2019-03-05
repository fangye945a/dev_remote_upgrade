#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "cJSON.h"
#include "mqtt.h"
#include "ftp-manager.h"
#include "param_init.h"
#include "msg_process.h"
#include "mqtt_msg_proc.h"

REMOTE_UPGRADE_CFG g_remote_upgrade_params; //远程升级配置参数
tmsg_buffer* main_process_msg = NULL;

int main(int argc, char *argv[])
{
	int ret = -1;
	tmsg_element* event = NULL;
	
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
	printf("version = %s\n",g_remote_upgrade_params.version);

	ftp_option_init(g_remote_upgrade_params.user_key); 				//FTP参数初始化

	mqtt_params_init(&g_remote_upgrade_params);				//mqtt参数初始化

	StartMqttTask(); //开启MQTT任务

	while (1)
    {
		event = main_process_msg->get_timeout(main_process_msg,500);
		if(event != NULL)
		{
			switch (event->msg)
			{
				case MQTT_MSG:
				{
					MQTT_MESSAGE *message = (MQTT_MESSAGE *)event->dt;
					ret = mqtt_msg_proc(message->topic, (char *)message->payload, message->payloadlen);
				}break;
				case FTP_RESULT:
				{
					ftp_pthread_id_clean();
					if(event->ext == FTP_BUZY)
						printf("----------- FTP_BUZY!!\n");
					else if(event->ext == FTP_UPLOAD_SUCCESS)
						printf("----------- FTP_UPLOAD_SUCCESS!!\n");
					else if(event->ext == FTP_UPLOAD_FAIL_NOT_EXIST)
						printf("----------- FTP_UPLOAD_FAIL_NOT_EXIST!!\n");
					else if(event->ext == FTP_UPLOAD_FAILED)
						printf("----------- FTP_UPLOAD_FAILED!!\n");
					else if(event->ext == FTP_DOWNLOAD_PATH_ERROR)
						printf("----------- FTP_DOWNLOAD_PATH_ERROR!!\n");
					else if(event->ext == FTP_DOWNLOAD_SUCCESS)
						printf("----------- FTP_DOWNLOAD_SUCCESS!!\n");
					else if(event->ext == FTP_DOWNLOAD_FAILED)
						printf("----------- FTP_DOWNLOAD_FAILED!!\n");
				}break;
			}
			free_tmsg_element(event);
			if(ret == -1)	//如果返回-1则退出进程
				break;
		}
		
    }
	ExitMqttTask();	//退出MQTT任务
	
	return 0;
}
