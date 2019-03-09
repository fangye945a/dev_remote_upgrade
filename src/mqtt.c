/*********************************************************
     > File name : mqtt.c
     > Create date : 2019-03-04 星期1 14:43
     > Author : 方烨
     > Email : fangye945@qq.com
 ********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/vfs.h>

#include "cJSON.h"
#include "mqtt.h"
#include "msg_process.h"
#include "param_init.h"
#include "config.h"
#include "ftp-manager.h"
#include "upgrade_proc.h"

static struct mosquitto *mosq = NULL;
static struct mosq_config g_mqtt_cfg;
volatile unsigned char mqtt_connect_state = 0;
static pthread_t mqtt_thread_id;

extern tmsg_buffer* main_process_msg;

double get_sd_used_percent()
{
    double percent = 0;
    struct statfs diskInfo;
	
#ifdef ARM_EC20
    statfs("/usrdata",&diskInfo);
#else
	statfs("/",&diskInfo);
#endif

    unsigned long int f_blocks = diskInfo.f_blocks;
    unsigned long int f_bavail = diskInfo.f_bavail;
    if(f_bavail > f_blocks)
    {
        printf("Error: diskInfo.f_bavail > diskInfo.f_blocks!!\n");
    }
    else
    {
        double free_percent = f_bavail*100.0/f_blocks;
        percent = 100.0 - free_percent;
        printf("-----------Disk used percent:%lf%%\n",percent);
    }
    return percent;
}

static int add_sub_topic(struct mosq_config *cfg, char *topic)  //添加订阅主题
{
	cfg->topic_count++;
	cfg->topics = realloc(cfg->topics, cfg->topic_count*sizeof(char *));
	cfg->topics[cfg->topic_count-1] = strdup(topic);
	return SUCCESS;
}

static void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	printf("------------------ mqtt disconnect!! \n");
	mqtt_connect_state = 0; //连接断开
}

static void my_connect_callback(struct mosquitto *mosq, void *obj, int result, int flags)
{
	int i;
	switch(result)
	{
		case 0:
			printf("---- connect success!!\n");
			mqtt_connect_state = 1;
			break;
		case 1:
			printf("connection refused!! (unacceptable protocol version)\n");
			break;
		case 2:
			printf("connection refused!! (identifier rejected)\n");
			break;
		case 3:
			printf("connection refused!! (broker unavailable)\n");
			break;
		default:
			printf("Unknow connection results!! (reserved for future use)\n");
			break;
	}	
	
	if(NULL == obj)
	{
		printf("------------------ mosquitto lib error!!\n");
		return;
	}
	
	struct mosq_config *mqtt_cfg;
	mqtt_cfg = (struct mosq_config *)obj;
	
	for(i=0; i<mqtt_cfg->topic_count; i++)	//订阅主题
	{
		mosquitto_subscribe(mosq, NULL, mqtt_cfg->topics[i], mqtt_cfg->qos);
		printf("sub[success]: %s\n",mqtt_cfg->topics[i]);
	}
	pub_login_msg(); //发布上线消息
}

static void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	MQTT_MESSAGE msg;
	strncpy(msg.topic, message->topic, sizeof(msg.topic));
	msg.topic[TOPIC_MAX_LEN-1] = '\0';	//超出最大长度64将被截断
	
	strncpy(msg.payload, message->payload, sizeof(msg.payload));
	msg.payload[PAYLOAD_MAX_LEN -1] = '\0';	//超出最大长度1024将被截断
	
	msg.payloadlen = message->payloadlen;  //消息长度

	msg.retain = message->retain;  //被设置为保留的数据

	//将收到的消息发送至主线程处理
	main_process_msg->sendmsg(main_process_msg, MQTT_MSG, 0, (char *)&msg, sizeof(MQTT_MESSAGE));
}


static void client_config_cleanup(struct mosq_config *cfg)
{
	int i;
	
	if(cfg->id)
		free(cfg->id);
	if(cfg->id_prefix)
		free(cfg->id_prefix);
	if(cfg->host)
		free(cfg->host);
	if(cfg->file_input)
		free(cfg->file_input);
	if(cfg->message)
		free(cfg->message);
	if(cfg->topic)
		free(cfg->topic);
	if(cfg->bind_address)
		free(cfg->bind_address);
	if(cfg->username)
		free(cfg->username);
	if(cfg->password)
		free(cfg->password);
	if(cfg->will_topic)
		free(cfg->will_topic);
	if(cfg->will_payload)
		free(cfg->will_payload);
	if(cfg->format)
		free(cfg->format);

#ifdef WITH_TLS
	if(cfg->cafile)
		free(cfg->cafile);
	if(cfg->capath)
		free(cfg->capath);
	if(cfg->certfile)
		free(cfg->certfile);
	if(cfg->keyfile)
		free(cfg->keyfile);
	if(cfg->ciphers)
		free(cfg->ciphers);
	if(cfg->tls_version)
		free(cfg->tls_version);
	
#ifdef FINAL_WITH_TLS_PSK
	if(cfg->psk)
		free(cfg->psk);
	if(cfg->psk_identity)
		free(cfg->psk_identity);
#endif

#endif
	if(cfg->topics){
		for(i=0; i<cfg->topic_count; i++){
			free(cfg->topics[i]);
		}
		free(cfg->topics);
	}
	if(cfg->filter_outs){
		for(i=0; i<cfg->filter_out_count; i++){
			free(cfg->filter_outs[i]);
		}
		free(cfg->filter_outs);
	}
	if(cfg->unsub_topics){
		for(i=0; i<cfg->unsub_topic_count; i++){
			free(cfg->unsub_topics[i]);
		}
		free(cfg->unsub_topics);
	}
#ifdef WITH_SOCKS
	if(cfg->socks5_host)
		free(cfg->socks5_host);
	if(cfg->socks5_username)
		free(cfg->socks5_username);
	if(cfg->socks5_password)
		free(cfg->socks5_password);
#endif

}

int mqtt_params_init(REMOTE_UPGRADE_CFG *param)
{
	char will_msg[PAYLOAD_MAX_LEN] = {0};

	memset(&g_mqtt_cfg, 0, sizeof(struct mosq_config));	//清空
	struct mosq_config *cfg = &g_mqtt_cfg;
	cfg->port = param->port;
	cfg->max_inflight = 20;
	cfg->keepalive = param->hearbeat;
	cfg->clean_session = true; //清除会话
	cfg->eol = true;
	cfg->protocol_version = MQTT_PROTOCOL_V31;	//MQTT协议版本
	cfg->host = strdup(param->host);
	cfg->id = strdup(param->devid);
	cfg->qos = 1;	//消息质量
	
	if( strcmp(param->username,"null") && strcmp(param->password,"null") )
	{
		cfg->username = strdup(param->username);
		cfg->password = strdup(param->password);
	}
	else
	{
		cfg->username = NULL;
		cfg->password = NULL;
	}
	
	cfg->will_retain = false; //遗言保留
	cfg->will_topic = strdup(WILL_TOPIC);
	cfg->will_qos = 2;	//遗言消息质量
	sprintf(will_msg, "{\"devid\":\"%s\"}", cfg->id);
	cfg->will_payload = strdup(will_msg);
	cfg->will_payloadlen = strlen(will_msg);

	char tmp_topic[TOPIC_MAX_LEN]={0};
	
	sprintf(tmp_topic, "ZL/online_ask/%s", cfg->id);	//文件上传主题
	add_sub_topic(cfg, tmp_topic);
	
	sprintf(tmp_topic, "ZL/upload/%s", cfg->id);	//文件上传主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/download/%s", cfg->id);	//文件下载主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/setting/%s", cfg->id);	//参数设置主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/upgrade/%s", cfg->id);	//升级主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/app_manage/%s", cfg->id);	//上传主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/get_dir_info/%s", cfg->id);	//获取目录主题
	add_sub_topic(cfg, tmp_topic);

	sprintf(tmp_topic, "ZL/get_run_info/%s", cfg->id);	//获取运行信息主题
	add_sub_topic(cfg, tmp_topic);
	
	return SUCCESS;
}

int pub_msg_to_topic(char *topic, char *msg, int msg_len)  //发布消息
{
	if(!mqtt_connect_state)
		return FAIL;
	int mid_send = 0;
	int qos = 1;
	if(mosq != NULL)
	{
		if(MOSQ_ERR_SUCCESS == mosquitto_publish(mosq, &mid_send, topic, msg_len, msg, qos, false))
		{
			printf("pub[success]: %s ---> %s\n",msg, topic);
		}else
			printf("pub[fail]: %s ---> %s\n",msg, topic);
			
	}
	return SUCCESS;
}

int StartMqttTask(void)	//开启MQTT任务
{
	mosquitto_lib_init();		//mqtt库初始化

	mosq = mosquitto_new(g_mqtt_cfg.id, g_mqtt_cfg.clean_session, &g_mqtt_cfg);	//创建MQTT实例
	
	mosquitto_will_set(mosq, g_mqtt_cfg.will_topic, g_mqtt_cfg.will_payloadlen,					//设置遗言
						g_mqtt_cfg.will_payload, g_mqtt_cfg.will_qos,g_mqtt_cfg.will_retain);
	
	if(NULL != g_mqtt_cfg.username && NULL != g_mqtt_cfg.password )	//用户名不为空则设置用户名及密码
	{
		mosquitto_username_pw_set(mosq, g_mqtt_cfg.username, g_mqtt_cfg.password);			//设置用户名及密码
	}

	mosquitto_max_inflight_messages_set(mosq, g_mqtt_cfg.max_inflight);					//设置空中最大消息条数

	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(g_mqtt_cfg.protocol_version));  //设置MQTT协议版本

	mosquitto_connect_with_flags_callback_set(mosq, my_connect_callback);	//设置连接成功回调

	mosquitto_message_callback_set(mosq, my_message_callback);				//设置消息接收回调

	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);		//设置连接断开回调

	mosquitto_connect_bind(mosq, g_mqtt_cfg.host, g_mqtt_cfg.port, g_mqtt_cfg.keepalive, g_mqtt_cfg.bind_address);	//连接mqtt

	mosquitto_loop_start(mosq); //创建线程处理mqtt消息

	return SUCCESS;
}

int ExitMqttTask(void)	//退出MQTT任务
{
	mosquitto_loop_stop(mosq, true); //创建线程处理mqtt消息
	
	mosquitto_destroy(mosq);

	client_config_cleanup(&g_mqtt_cfg); //释放mqtt参数信息
	
	mosquitto_lib_cleanup();

	return SUCCESS;
}


void pub_login_msg() //发布上线消息
{
	REMOTE_UPGRADE_CFG *ptr = get_remote_upgrade_cfg();
	cJSON *root = cJSON_CreateObject();  //创建json
	cJSON_AddStringToObject(root, "devid", ptr->devid);
	cJSON_AddStringToObject(root, "upgrade_version", ptr->upgrade_version);
	cJSON_AddStringToObject(root, "servcie_version", ptr->service_version);
	cJSON_AddStringToObject(root, "mcu_version", ptr->mcu_version);

	cJSON *root1 = cJSON_CreateObject();  
	cJSON_AddStringToObject(root1, "dev_type", ptr->dev_type);
	if( !strcmp(ptr->dev_type,"tower_crane") ) 		//塔机
	{
		cJSON_AddStringToObject(root1, "plc_version", ptr->other_version);
	}
	else if(!strcmp(ptr->dev_type,"pump")) 		//泵车
	{
	}
	else if(!strcmp(ptr->dev_type,"excavator"))	//挖掘机
	{
	}
	else if(!strcmp(ptr->dev_type,"car_carne")) 	//汽车起重机
	{
	}
	else if(!strcmp(ptr->dev_type,"rotary_drill")) //旋挖钻
	{
	}
	else if(!strcmp(ptr->dev_type,"env_sanitation")) //环卫
	{
	}
	cJSON_AddItemToObject(root, "type", root1);

	char *msg = cJSON_PrintUnformatted(root);
	pub_msg_to_topic(LOGIN_TOPIC, msg, strlen(msg));	//发布上线消息

	cJSON_Delete(root);
	msg = NULL;
	
}

void pub_trans_progress(char *trans_state, unsigned int total_size, unsigned int finish_size) //发布传输进度
{
	FTP_OPT *ftp_opt = get_ftp_option();
	REMOTE_UPGRADE_CFG *ptr = get_remote_upgrade_cfg();

	char topic[TOPIC_MAX_LEN]={0};
	sprintf(topic, "ZL/trans_progress/%s", ptr->devid);	//传输进度主题
	
	cJSON *root = cJSON_CreateObject();  //创建json
	cJSON_AddStringToObject(root, "state", trans_state); //传输状态
	if( !strcmp(UPLOADING_STAT,trans_state) || !strcmp(DOWNLOADING_STAT,trans_state))
	{
		cJSON_AddNumberToObject(root, "total_size", total_size);
		cJSON_AddNumberToObject(root, "finish_size", finish_size);
	}
	cJSON_AddStringToObject(root, "url", ftp_opt->url);
	cJSON_AddStringToObject(root, "filepath", ftp_opt->file);
	char *msg = cJSON_PrintUnformatted(root);
	pub_msg_to_topic(topic, msg, strlen(msg));	//发布传输进度
	cJSON_Delete(root);
	msg = NULL;
}

void pub_setting_reply(int flag)   //发布参数设置结果
{
	REMOTE_UPGRADE_CFG *ptr = get_remote_upgrade_cfg();
	char topic[TOPIC_MAX_LEN]={0};
	sprintf(topic, "ZL/set_reply/%s", ptr->devid); //传输进度主题

	cJSON *root = cJSON_CreateObject();  //创建json
	if(flag)
		cJSON_AddStringToObject(root, "result", "success"); //设置结果
	else
		cJSON_AddStringToObject(root, "result", "fail"); //设置结果
	char *msg = cJSON_PrintUnformatted(root);
	pub_msg_to_topic(topic, msg, strlen(msg));	//发布传输进度
	cJSON_Delete(root);
	msg = NULL;	
}

void pub_run_info()   //发布运行状态
{
	app_info_check();
	APPS_INFO *info = get_apps_info();
	int i = 0;

	REMOTE_UPGRADE_CFG *ptr = get_remote_upgrade_cfg();
	char topic[TOPIC_MAX_LEN]={0};
	sprintf(topic, "ZL/run_info/%s", ptr->devid); //传输进度主题

	cJSON *root = cJSON_CreateObject();  //创建json对象
	cJSON *service = cJSON_CreateArray();  //创建服务信息
	cJSON *apps = cJSON_CreateArray();   	//创建app信息

	cJSON_AddNumberToObject(root, "disk_use_percent", get_sd_used_percent()); //磁盘使用百分比
		
	cJSON *tmp = cJSON_CreateObject();
	cJSON_AddStringToObject(tmp, "name","ZBox_IOT_Service");
	cJSON_AddStringToObject(tmp, "state","run");
	cJSON_AddItemToArray(service, tmp);
	cJSON_AddItemToObject(root, "service", service);

	for(i=0; i<info->exist_apps_num; i++)
	{
		tmp = cJSON_CreateObject();
		cJSON_AddStringToObject(tmp, "name", info->exist_apps_name[i]);
		
		if( is_app_running(info->exist_apps_name[i]) )
			cJSON_AddStringToObject(tmp, "state","run");
		else
			cJSON_AddStringToObject(tmp, "state","stop");
		
		cJSON_AddItemToArray(apps, tmp);
	}
	cJSON_AddItemToObject(root, "apps", apps);
	
	char *msg = cJSON_PrintUnformatted(root);
	pub_msg_to_topic(topic, msg, strlen(msg));	//发布传输进度
	cJSON_Delete(root);
	msg = NULL; 
}

void pub_dir_info(char *dirpath)       //发布目录结构信息
{
	cJSON *dirs = NULL;
	cJSON *files = NULL;
	cJSON *root = cJSON_CreateObject();  //创建json

	REMOTE_UPGRADE_CFG *ptr = get_remote_upgrade_cfg();
	char topic[TOPIC_MAX_LEN]={0};
	sprintf(topic, "ZL/dir_info/%s", ptr->devid); //传输进度主题
	int ret = FAIL;
	
	if(dirpath != NULL)
	{
		DIR *dp = NULL;
		struct dirent *st = NULL;
		struct stat sta;
		char tmp_name[1024]={0};
		dp = opendir(dirpath);
		if(dp != NULL)
		{
			while(1)
			{
				st = readdir(dp);
				if(NULL == st) //读取完毕
				{
					break;
				}
				strcpy(tmp_name, dirpath);
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
							if(NULL == dirs)
							{
								dirs = cJSON_CreateArray(); //目录数组
							}
							cJSON *dir_name = cJSON_CreateString(st->d_name);
							cJSON_AddItemToArray(dirs, dir_name);
						}
					}
					else	//不为目录则打印文件路径名
					{
						if(NULL == files)
						{
							files = cJSON_CreateArray(); //文件数组
						}
						cJSON *file_name = cJSON_CreateString(st->d_name);
						cJSON_AddItemToArray(files, file_name);
					}
				}
				else
				{
					printf("--- read dir[%s] stat fail!!\n",st->d_name);
				}
			}
			closedir(dp);
		}
	}
	
	if(root != NULL && files != NULL )
	{
		cJSON_AddItemToObject(root, "files", files);
	}
	if(root != NULL && dirs != NULL)
	{
		cJSON_AddItemToObject(root, "dirs", dirs);
	}

	if(root != NULL)
	{
		if( files != NULL || dirs != NULL )
			cJSON_AddStringToObject(root, "result", "success"); //设置查询结果
		else
			cJSON_AddStringToObject(root, "result", "fail"); //设置查询结果
		
		char *msg = cJSON_Print(root);
		printf("msg = %s\n",msg);
		pub_msg_to_topic(topic, msg, strlen(msg));	//发布目录结构
		cJSON_Delete(root);
		msg = NULL;
	}
	return;
}

