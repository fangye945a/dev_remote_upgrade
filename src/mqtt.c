/*********************************************************
     > File name : mqtt.c
     > Create date : 2019-03-04 星期1 14:43
     > Author : 方烨
     > Email : fangye945@qq.com
 ********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mosquitto.h"
#include "param_init.h"

static struct mosquitto *mosq = NULL;
static struct mosq_config cfg;


static int add_sub_topic(struct mosq_config *cfg, char *topic)  //添加订阅主题
{
	cfg->topic_count++;
	cfg->topics = realloc(cfg->topics, cfg->topic_count*sizeof(char *));
	cfg->topics[cfg->topic_count-1] = strdup(topic);
	return 0;
}

int pub_msg_to_topic(char *topic, char *msg, int msg_len)  //发布消息
{
	int mid_send = 0;
	int qos = QOS;
	if(mosq != NULL)
		mosquitto_publish(mosq, &mid_send, topic, msg_len, msg, QOS, false);
	return 0;
}

static void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	printf("------------------ mqtt disconnect!! \n");
}

static void my_connect_callback(struct mosquitto *mosq, void *obj, int result, int flags)
{
	int i;
	switch(result)
	{
		case 0:
			printf("connect success\n");
			break;
		case 1:
			printf("connection refused (unacceptable protocol version)\n");
			break;
		case 2:
			printf("connection refused (identifier rejected)\n");
			break;
		case 3:
			printf("connection refused (broker unavailable)\n");
			break;
		default:
			printf("Unknow connection results(reserved for future use)\n");
			break;
	}	
	struct mosq_config *mqtt_cfg;
	printf("--- connect flags = %d\n",flags);
	if(NULL == obj)
	{
		printf("------------------ mosquitto lib error!!\n");
		return;
	}
	
	mqtt_cfg = (struct mosq_config *)obj;
	
	for(i=0; i<mqtt_cfg->topic_count; i++)	//订阅主题
	{
		mosquitto_subscribe(mosq, NULL, mqtt_cfg->topics[i], mqtt_cfg->qos);
		printf("------------------ sub %s\n",mqtt_cfg->topics[i]);
	}

	char *msg = "{\"hello\":\"world\"}";
	int len = strlen(msg);
	pub_msg_topic(mosq,"first_word",msg,len);	//发布上线消息

	
}

static void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct mosq_config *cfg;
	printf("Topic:%s\n",message->topic);
	printf("Recv[%d bytes]:%s\n",message->payloadlen,(char *)message->payload);
}



int mqtt_params_init(struct mosq_config *cfg, REMOTE_UPGRADE_CFG *param)
{
	memset(cfg, 0, sizeof(struct mosq_config));	//清空
	
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

	return 0;
}



static int mqtt_proc(void)
{
	mqtt_params_init(&cfg);		//mqtt参数初始化

	mosquitto_lib_init();		//mqtt库初始化

	

//	mosquitto_threaded_set(struct mosquitto *mosq, bool threaded);//使用线程
//	mosquitto_username_pw_set(struct mosquitto *mosq, const char *username, const char *password);
//	mosquitto_tls_set(mosq, cfg->cafile, cfg->capath, cfg->certfile, cfg->keyfile, NULL); //设置TLS加密
//	mosquitto_tls_insecure_set(mosq, true))
//	mosquitto_tls_psk_set(mosq, cfg->psk, cfg->psk_identity, NULL)){
//	mosquitto_tls_opts_set(mosq, 1, cfg->tls_version, cfg->ciphers)){
//	//mosquitto_max_inflight_messages_set	//空中消息数量  0表示无上限
//	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V31);
//	
//	mosquitto_connect_with_flags_callback_set(mosq, my_connect_callback);
//	mosquitto_message_callback_set(mosq, my_message_callback);

//	mosquitto_connect_callback_set(mosq, my_connect_callback);
//	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
//	mosquitto_publish_callback_set(mosq, my_publish_callback);
//	
//	mosquitto_connect_bind(mosq, cfg->host, port, cfg->keepalive, cfg->bind_address);
//	mosquitto_publish(mosq, &mid_sent, topic, buf_len_actual-1, buf, qos, retain);
//	mosquitto_loop_forever(mosq, -1, 1);

//	mosquitto_destroy(mosq);
//	mosquitto_lib_cleanup();
	return 0;
}



