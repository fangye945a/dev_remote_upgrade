#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "msg_process.h"
 
static unsigned long timeout_ns = 0;

#define LOG_ERR(fmt, ...) do{\
	printf("[ERROR]  "fmt"  [line:%d] [%s]\n", ##__VA_ARGS__, __LINE__, __FUNCTION__);\
}while(0);
 
#define LOG_WARN(fmt, ...) do{\
	printf("[WARNING]  "fmt"  [line:%d] [%s]\n", ##__VA_ARGS__, __LINE__, __FUNCTION__);\
}while(0); 
 
/**
 * 消息处理模块内部接口
 */
static void put_msg_to_buffer(tmsg_buffer* buf, tmsg_element* elm){
	if (NULL == buf || NULL == elm) {
		LOG_ERR("buf or elm is NULL");
		return;
	}
 
	if (NULL != elm->next) {
		elm->next = NULL;
	}
 
	pthread_mutex_lock(&buf->mutex);
	//缓冲区尚无消息节点
	if (buf->first == buf->last
			&& 0 == buf->num) {
		buf->first = elm;
		buf->last = elm;
		buf->num ++;
		//TODO:通知等待消息而阻塞的线程
		pthread_cond_signal(&buf->not_empty);
	} else {
		//将新的消息节点信息添加到缓冲区的尾部
		buf->last->next = elm;
		buf->last = elm;
		buf->num ++;
	}
 
	pthread_mutex_unlock(&buf->mutex);
}
 
 
static tmsg_element* get_msg_from_buffer(tmsg_buffer* buf, int block){
	tmsg_element *elm = NULL;
 
	if (NULL == buf) {
		LOG_ERR("buf is NULL");
		return NULL;
	}
 
	pthread_mutex_lock(&buf->mutex);
	//缓冲区中无消息节点
	while (0 == buf->num) {
		//阻塞线程等待消息节点
		pthread_cond_wait(&buf->not_empty, &buf->mutex);
	}
	//从缓冲区首部取出消息节点
	elm = buf->first;
	if (1 == buf->num) {
		buf->first = buf->last = NULL;
		buf->num = 0;
	} else {
		buf->first = buf->first->next;
		buf->num --;
	}
 
	pthread_mutex_unlock(&buf->mutex);
 
	return elm;
}
 
 
 
static tmsg_element* get_msg_from_buffer_timeout(tmsg_buffer* buf, int block/*ms*/){
	tmsg_element *elm = NULL;
	struct timeval timenow;
	struct timespec timeout;
 
	if (NULL == buf) {
		LOG_ERR("buf is NULL");
		return NULL;
	}
 
	pthread_mutex_lock(&buf->mutex);
	//缓冲区中无消息节点
	if (0 == buf->num) {
		gettimeofday(&timenow,NULL);
		timeout.tv_sec = timenow.tv_sec + block/1000; //加上秒数
		block %= 1000;	//得到毫秒数

		timeout_ns = timenow.tv_usec*1000 + block*1000*1000;
		if( timeout_ns >= 1000*1000*1000 ) //若超过1s
		{
			timeout.tv_sec ++;
			timeout.tv_nsec = timeout_ns - 1000*1000*1000;
		}
		else
			timeout.tv_nsec = timeout_ns;
		//带超时时间阻塞线程等待消息节点
		pthread_cond_timedwait(&buf->not_empty, &buf->mutex, &timeout);
	}
 
	if (buf->num > 0) {
		//从缓冲区首部取出消息节点
		elm = buf->first;
		if (1 == buf->num) {
			buf->first = buf->last = NULL;
			buf->num = 0;
		} else {
			buf->first = buf->first->next;
			buf->num --;
		}
	}
 
	pthread_mutex_unlock(&buf->mutex);
 
	return elm;
}
 
 
static tmsg_element* clear_msg_buffer(tmsg_buffer* buf){
	tmsg_element* elm = NULL;
	tmsg_element* elm_tmp = NULL;
 
	if (NULL == buf){
		LOG_ERR("buf is NULL");
		return NULL;
	}
 
	//清空buffer中当前消息节点之前的所有消息节点
	pthread_mutex_lock(&buf->mutex);
	if (buf->num > 0) {
		elm = buf->first;
		while(elm != NULL) {
			//首尾指针指向同一消息节点
			if (elm == buf->last) {
				buf->first = buf->last;
				if (buf->num != 1) {
					buf->num = 1;
				}
				break;
			}
 
			elm_tmp = elm->next;
			free_tmsg_element(elm);
			buf->num --;
			elm = elm_tmp;
			buf->first = elm;
		}
	}
 
	pthread_mutex_unlock(&buf->mutex);
 
	return elm;
}
 
 
static void send_msg_to_buffer(tmsg_buffer* buf, int msg, int ext, char* str, int len)
{
	tmsg_element *elm = NULL;
 
	elm = (tmsg_element *)malloc(sizeof(tmsg_element));
	if (NULL == elm) {
		LOG_ERR("new msg element failed!!");
		return;
	}
 
	if(len > TMSG_MAX_LEN) //限制最大申请长度
	{
		len = TMSG_MAX_LEN;
		LOG_WARN("Data is truncated,which must less than %d!",TMSG_MAX_LEN);
	}
	//填充消息节点数据
	memset(elm, 0, sizeof(tmsg_element));
	elm->msg = msg;
	elm->ext = ext;
	elm->dt = NULL;
	elm->sub0 = 0;
	elm->sub1 = 0;
	elm->dt_len = len;
	if (str) 
	{
		elm->dt = (char *)malloc(len);  //根据发送的大小申请内存
		if(elm->dt == NULL)
		{
			LOG_ERR("new element->dt failed!!");
			free_tmsg_element(elm);
			return;
		}	
		else
			memmove(elm->dt, str, len);
	}
	
	elm->next = NULL;
	//将消息节点添加到缓冲区中
	put_msg_to_buffer(buf, elm);
}
 
 
static void send_msg_to_buffer_ex(tmsg_buffer* buf, int msg, int ext, int sub0, int sub1, char* str, int len){
	tmsg_element *elm = NULL;
 
	elm = (tmsg_element *)malloc(sizeof(tmsg_element));
	if (NULL == elm) {
		LOG_ERR("new msg element failed!!");
		return;
	}
	
	if(len > TMSG_MAX_LEN) //限制最大申请长度
	{
		len = TMSG_MAX_LEN;
		LOG_WARN("Data is truncated,which must less than %d!",TMSG_MAX_LEN);
	}
	
	//填充消息节点数据
	memset(elm, 0, sizeof(tmsg_element));
	elm->msg = msg;
	elm->ext = ext;
	elm->sub0 = sub0;
	elm->sub1 = sub1;
	elm->dt = NULL;
	elm->dt_len = len;
	if (str) 
	{
		elm->dt = (char *)malloc(len);  //根据发送的大小申请内存
		if(elm->dt == NULL)
		{
			LOG_ERR("new element->dt failed!!");
			free_tmsg_element(elm);
			return;
		}	
		else
			memmove(elm->dt, str, len);
	}
	elm->next = NULL;
	//将消息节点添加到缓冲区中
	put_msg_to_buffer(buf, elm);
}
 
 
static void dispose_msg_buffer(tmsg_buffer* buf){
	tmsg_element* elm = NULL;
 
	if (NULL == buf) {
		return;
	}
 
	if (buf->first != buf->last
			&& buf->num > 0) {
		elm = clear_msg_buffer(buf);
	} else {
		elm = buf->last;
	}
 
	if (NULL != elm) {
		free_tmsg_element(elm);
		buf->first = buf->last = NULL;
		buf->num = 0;
	}
 
	pthread_mutex_destroy(&buf->mutex);
	pthread_cond_destroy(&buf->not_empty);
	free(buf);
 
	buf = NULL;
}
 
 
static int get_msg_num(tmsg_buffer* buf){
	if (NULL == buf) {
		return 0;
	}
 
	return buf->num;
}
 
 
/**
 * 以下为消息处理模块对外接口
 */
 
/*消息缓冲区初始化*/
tmsg_buffer* msg_buffer_init(void){
	tmsg_buffer* msg_buffer = NULL;
 
	msg_buffer = (tmsg_buffer *)malloc(sizeof(tmsg_buffer));
	if (NULL == msg_buffer){
		LOG_ERR("init msg buffer failed!!");
		return NULL;
	}
 
	//初始化成员变量和函数
	memset(msg_buffer, 0, sizeof(tmsg_buffer));
	msg_buffer->first = NULL;
	msg_buffer->last = NULL;
	msg_buffer->num = 0;
 
	pthread_mutex_init(&(msg_buffer->mutex), NULL);
	pthread_cond_init(&(msg_buffer->not_empty), NULL);
 
	//继续绑定接口
	msg_buffer->put = put_msg_to_buffer;
	msg_buffer->get = get_msg_from_buffer;
	msg_buffer->get_timeout = get_msg_from_buffer_timeout;
	msg_buffer->clear = clear_msg_buffer;
	msg_buffer->sendmsg = send_msg_to_buffer;
	msg_buffer->sendmsgex = send_msg_to_buffer_ex;
	msg_buffer->dispose = dispose_msg_buffer;
	msg_buffer->getnum = get_msg_num;
 
	return msg_buffer;
}
 
 
/*复制消息节点*/
tmsg_element* dup_msg_element(tmsg_element* elm){
	tmsg_element* msg_element = NULL;
	if (NULL == elm) {
		LOG_ERR("msg element is NULL!!");
		return NULL;
	}
 
	msg_element = (tmsg_element *)malloc(sizeof(tmsg_element));
	if (NULL == msg_element) {
		LOG_ERR("create msg element is failed!!");
		return NULL;
	}
 
	memcpy(msg_element, elm, sizeof(tmsg_element));
 
	return msg_element;
}
 
void free_tmsg_element(tmsg_element *msg_element)
{
	if(msg_element->dt != NULL)
	{
		free(msg_element->dt);
		msg_element->dt = NULL;
	}
	
	if(msg_element != NULL)
	{
		free(msg_element);
		msg_element = NULL;
	}
}
 
