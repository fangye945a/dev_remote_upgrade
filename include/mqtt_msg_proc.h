/******************************************************************************
                  版权所有 (C), 2019-2099, 中联重科

-->文件名	: mqtt_msg_proc.h
-->作  者	: fangye
-->生成日期	: 2019年3月5日
-->功能描述	: mqtt_msg_proc.c 的头文件
******************************************************************************/
#ifndef __MQTT_MSG_PROC_H__
#define __MQTT_MSG_PROC_H__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct _APP_MANAGE_ADD_OPT
{
	int add_app_flag;
	char add_app_name[64];
}APP_MANAGE_ADD_OPT;


extern int mqtt_msg_proc(char *topic, char *payload, int payloadlen);
extern int judge_is_add_app();
extern 	void add_app_clear();
extern	int judge_is_add_app();	
extern APP_MANAGE_ADD_OPT *get_add_app_st();
extern int run_application(char *app_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MQTT_MSG_PROC_H__ */
