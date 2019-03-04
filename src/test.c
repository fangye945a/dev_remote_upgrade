#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mosquitto.h"

#define WILL_TOPIC	"last_word"
#define WILL_PAYLOAD	"{\"good\":\"bye\"}"



struct mosq_config {
	char *id;
	char *id_prefix;
	int protocol_version;
	int keepalive;
	char *host;
	int port;
	int qos;
	bool retain;
	int pub_mode; /* pub */
	char *file_input; /* pub */
	char *message; /* pub */
	long msglen; /* pub */
	char *topic; /* pub */
	char *bind_address;
#ifdef WITH_SRV
	bool use_srv;
#endif
	bool debug;
	bool quiet;
	unsigned int max_inflight;
	char *username;
	char *password;
	char *will_topic;
	char *will_payload;
	long will_payloadlen;
	int will_qos;
	bool will_retain;
#ifdef WITH_TLS
	char *cafile;
	char *capath;
	char *certfile;
	char *keyfile;
	char *ciphers;
	bool insecure;
	char *tls_version;
#  ifdef FINAL_WITH_TLS_PSK
	char *psk;
	char *psk_identity;
#  endif
#endif
	bool clean_session;
	char **topics; /* sub */
	int topic_count; /* sub */
	bool no_retain; /* sub */
	bool retained_only; /* sub */
	char **filter_outs; /* sub */
	int filter_out_count; /* sub */
	char **unsub_topics; /* sub */
	int unsub_topic_count; /* sub */
	bool verbose; /* sub */
	bool eol; /* sub */
	int msg_count; /* sub */
	char *format; /* sub */
	int timeout; /* sub */
#ifdef WITH_SOCKS
	char *socks5_host;
	int socks5_port;
	char *socks5_username;
	char *socks5_password;
#endif
};


struct mosquitto *mosq = NULL;
struct mosq_config cfg;


int add_sub_topic(struct mosq_config *cfg, char *topic)  //添加订阅主题
{
	cfg->topic_count++;
	cfg->topics = realloc(cfg->topics, cfg->topic_count*sizeof(char *));
	cfg->topics[cfg->topic_count-1] = strdup(topic);
	return 0;
}

int pub_msg_topic(struct mosquitto *pub_mosq, char *topic, char *msg, int msg_len)  //发布消息
{
	int mid_send = 0;
	int qos = QOS;
	mosquitto_publish(pub_mosq, &mid_send, topic, msg_len, msg, QOS, false);
	return 0;
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	printf("------------------ mqtt disconnect!! \n");
}

void my_connect_callback(struct mosquitto *mosq, void *obj, int result, int flags)
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

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct mosq_config *cfg;
	printf("Topic:%s\n",message->topic);
	printf("Recv[%d bytes]:%s\n",message->payloadlen,(char *)message->payload);
}



int mqtt_params_init(struct mosq_config *cfg)
{
	cfg->port = MQTT_PORT;
	cfg->max_inflight = 20;
	cfg->keepalive = 60;
	cfg->clean_session = true; //清除会话
	cfg->eol = true;
	cfg->protocol_version = MQTT_PROTOCOL_V311;
	cfg->host = strdup(MQTT_HOST);
	cfg->id = strdup(DEV_ID);
	cfg->qos = 1;
	
	cfg->username = strdup(USRNAME);
	cfg->password = strdup(PASSWORD);

	cfg->will_retain = false; //遗言保留
	cfg->will_topic = strdup(WILL_TOPIC);
	cfg->will_qos = 2;
	cfg->will_payload = strdup(WILL_PAYLOAD);
	cfg->will_payloadlen = strlen(WILL_PAYLOAD);

	add_sub_topic(cfg,"123456");	//订阅主题
	add_sub_topic(cfg,"fangye");
}



int main(int argc, char *argv)
{
	mqtt_params_init(&cfg);		//mqtt参数初始化

	mosquitto_lib_init();		//mqtt库初始化

	mosq = mosquitto_new(cfg.id, cfg.clean_session, &cfg);  //创建MQTT实例

	
	mosquitto_will_set(mosq, cfg.will_topic, cfg.will_payloadlen, 					//设置遗言
						cfg.will_payload, cfg.will_qos,cfg.will_retain);
	
	if(NULL != cfg.username && NULL != cfg.password )
	{
		if( strcmp(cfg.username," ") || strcmp(cfg.password," ")) //用户名不为空则设置用户名及密码
		{
			mosquitto_username_pw_set(mosq, cfg.username, cfg.password);			//设置用户名及密码
		}
	}

	mosquitto_max_inflight_messages_set(mosq, cfg.max_inflight);  					//设置空中最大消息条数

	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(cfg.protocol_version));  //设置MQTT协议版本

	mosquitto_connect_with_flags_callback_set(mosq, my_connect_callback);   //设置连接成功回调

	mosquitto_message_callback_set(mosq, my_message_callback);				//设置消息接收回调

	mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);		//设置连接断开回调

	mosquitto_connect_bind(mosq, cfg.host, cfg.port, cfg.keepalive, cfg.bind_address);  //连接mqtt

	mosquitto_loop_forever(mosq, -1, 1);
	
	mosquitto_destroy(mosq);
	
	mosquitto_lib_cleanup();

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

