/******************************************************************************
                  版权所有 (C), 2019-2099, 中联重科

-->文件名	: mqtt.h
-->作  者	: fangye
-->生成日期	: 2019年3月4日
-->功能描述	: mqtt.c 的头文件
******************************************************************************/
#ifndef __MQTT_H__
#define __MQTT_H__

#include "mosquitto.h"
#include "param_init.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define TOPIC_MAX_LEN 64
#define PAYLOAD_MAX_LEN 2048

#define LOGIN_TOPIC "ZL/first_word"
#define WILL_TOPIC "ZL/last_word"


#define UPLOADING_STAT "uploading"
#define DOWNLOADING_STAT "downloading"
#define UPLOAD_FAIL_STAT "up_fail"
#define DOWNLOAD_FAIL_STAT "down_fail"
#define UPLOAD_SUCC_STAT "up_success"
#define DOWNLOAD_SUCC_STAT "down_success"


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

typedef struct _MQTT_MESSAGE{
	char topic[TOPIC_MAX_LEN];
	char payload[PAYLOAD_MAX_LEN];
	int payloadlen;
	bool retain;
}MQTT_MESSAGE;

extern void pub_login_msg(); //发布上线消息
extern int mqtt_params_init(REMOTE_UPGRADE_CFG *param);	//初始化mqtt参数
extern int pub_msg_to_topic(char *topic, char *msg, int msg_len); //发布消息
extern int StartMqttTask(void);	//开启MQTT任务
extern int ExitMqttTask(void);	//退出MQTT任务
extern void pub_trans_progress(char *trans_state, unsigned int total_size, unsigned int finish_size); //发布传输进度
extern void pub_setting_reply(int flag);			//发布设置结果

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MQTT_H__ */
