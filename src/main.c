#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "cJSON.h"
#include "mqtt.h"
#include "ftp-manager.h"
#include "param_init.h"
#include "msg_process.h"

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

	ftp_option_init(g_remote_upgrade_params.user_key); 				//FTP参数初始化

	mqtt_params_init(&g_remote_upgrade_params);				//mqtt参数初始化

	StartMqttTask(); //开启MQTT任务

	while (1)
    {
		event = main_process_msg->get_timeout(main_process_msg,500);
		if(event != NULL)
		{
			struct mosquitto_message *message = (struct mosquitto_message *)event->dt;

			printf("Topic:%s\n",message->topic);
			printf("Recv[%d bytes]:%s\n",message->payloadlen,(char *)message->payload);
			if(FTP_DOWNLOAD_SUCCESS == ftp_download("test.txt","ftp://113.240.239.164/test.txt"))
				printf("Download success.\n");
			else
				printf("Download failed.\n");
			
			free_tmsg_element(event);
		}
		
    }
	ExitMqttTask();	//退出MQTT任务
	return 0;
}
