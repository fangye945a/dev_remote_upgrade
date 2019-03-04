#include <stdio.h>
#include "mqtt.h"
#include "ftp-manager.h"
#include "param_init.h"


REMOTE_UPGRADE_CFG g_remote_upgrade_params; //远程升级配置参数

int main(int argc, char *argv[])
{
	load_remote_upgrade_param(&g_remote_upgrade_params); 	//加载远程升级参数

	mqtt_params_init(&cfg,	&g_remote_upgrade_params);		//mqtt参数初始化

	add_sub_topic(&cfg, "123456");						//订阅主题
	add_sub_topic(&cfg, "fangye");

	mosquitto_lib_init();		//mqtt库初始化

	mosq = mosquitto_new(cfg.id, cfg.clean_session, &cfg);	//创建MQTT实例
	
	mosquitto_will_set(mosq, cfg.will_topic, cfg.will_payloadlen,					//设置遗言
						cfg.will_payload, cfg.will_qos,cfg.will_retain);
	
	if(NULL != cfg.username && NULL != cfg.password )	//用户名不为空则设置用户名及密码
	{
		mosquitto_username_pw_set(mosq, cfg.username, cfg.password);			//设置用户名及密码
	}

	mosquitto_max_inflight_messages_set(mosq, cfg.max_inflight);					//设置空中最大消息条数

	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(cfg.protocol_version));  //设置MQTT协议版本

	mosquitto_connect_with_flags_callback_set(mosq, my_connect_callback);	//设置连接成功回调

	mosquitto_message_callback_set(mosq, my_message_callback);				//设置消息接收回调

	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);		//设置连接断开回调

	mosquitto_connect_bind(mosq, cfg.host, cfg.port, cfg.keepalive, cfg.bind_address);	//连接mqtt

	mosquitto_loop_forever(mosq, -1, 1); //循环处理
	
	mosquitto_destroy(mosq);
	
	mosquitto_lib_cleanup();

	FTP_OPT ftp_opt;
//	ftp_opt.url = "ftp://10.43.25.30/bigfile_test.tar.gz";
//	ftp_opt.user_key = "00006629:zengzhe.135";
//	ftp_opt.file = "./bigfile_test.tar.gz";

//	if(FTP_UPLOAD_SUCCESS == ftp_upload(ftp_opt))
//		printf("Upload success.\n");
//	else
//		printf("Upload failed.\n");
	ftp_opt.user_key = "zlgs:zlgs@00157";
	ftp_opt.url = "ftp://113.240.239.164/test.txt";
	ftp_opt.file = "./test.txt";

	if(FTP_DOWNLOAD_SUCCESS == ftp_download(ftp_opt))
		printf("Download success.\n");
	else
		printf("Download failed.\n");

	return 0;
}
