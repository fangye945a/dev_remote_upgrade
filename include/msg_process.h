#ifndef _MSG_PROCESS_H_
#define _MSG_PROCESS_H_
#include <pthread.h>


#define TMSG_MAX_LEN 4096 //最大限制为4K

typedef struct msg_element tmsg_element;
 
struct msg_element
{
    tmsg_element* next;
    int msg;
    int ext;
    int sub0;
    int sub1;
	int dt_len;
    char *dt;
};
  
typedef struct msg_buffer tmsg_buffer;

struct msg_buffer
{
    tmsg_element* first;
    tmsg_element* last;
    int             num;
 
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
 
    void (*put)(tmsg_buffer* buf, tmsg_element* elm);
    tmsg_element* (*get)(tmsg_buffer* buf, int block);
    tmsg_element* (*get_timeout)(tmsg_buffer* buf, int block);
 
    tmsg_element* (*clear)(tmsg_buffer* buf);
    void (*sendmsg)(tmsg_buffer* buf, int msg, int ext, char* str, int len);
    void (*sendmsgex)(tmsg_buffer* buf, int msg, int ext, int sub0, int sub1, char* str, int len);
    void (*dispose)(tmsg_buffer* buf);
 
    int (*getnum)(tmsg_buffer* buf) ;
};
 
 
 
/*消息缓冲区初始化*/
tmsg_buffer* msg_buffer_init(void);
 
 
/*复制消息节点*/
tmsg_element* dup_msg_element(tmsg_element* elm);
 
/*释放消息节点*/
void free_tmsg_element(tmsg_element *msg_element);
 
 
#endif /* MESSAGE_MSG_CENTER_H_ */
