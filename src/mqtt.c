/*********************************************************
     > File name : mqtt.c
     > Create date : 2019-03-04 星期1 14:43
     > Author : 方烨
     > Email : fangye945@qq.com
 ********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "mqtt.h"
#include "msg_process.h"
#include "config.h"

static struct mosquitto *mosq = NULL;
static struct mosq_config g_mqtt_cfg;
volatile unsigned char mqtt_connect_state = 0;
static pthread_t mqtt_thread_id;

extern tmsg_buffer* main_process_msg;

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
		printf("------------------ sub %s\n",mqtt_cfg->topics[i]);
	}

	char *msg = "{\"hello\":\"world\"}";
	int len = strlen(msg);
	pub_msg_to_topic("first_word", msg, len);	//发布上线消息
	
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
	cfg->will_payload = strdup(WILL_PAYLOAD);
	cfg->will_payloadlen = strlen(WILL_PAYLOAD);

	
	add_sub_topic(cfg, "123456");				//订阅主题
	add_sub_topic(cfg, "fangye");

	return SUCCESS;
}

int pub_msg_to_topic(char *topic, char *msg, int msg_len)  //发布消息
{
	if(!mqtt_connect_state)
		return FAIL;
	int mid_send = 0;
	int qos = 1;
	if(mosq != NULL)
		mosquitto_publish(mosq, &mid_send, topic, msg_len, msg, qos, false);
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

