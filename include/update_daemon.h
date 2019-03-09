/*
 * update_daemon.h
 *
 *  Created on: 2019年3月8日
 *      Author: chenyh
 */

#ifndef _UPDATE_DAEMON_H_
#define _UPDATE_DAEMON_H_


enum
{RESULT_FAILD = 0, RESULT_SUCCESS};

typedef void (*update_callback_fn)(int result);


/**
 * 资源初始化
 *
 */
extern unsigned int InitPlatform(void);

/**
 * 资源反初始化
 *
 */
extern void UnInitPlatform(void);

/**
 * 开始PLC升级
 * 返回值： -1 错误
 *      -2  进程间通信未连接上
 *  	 0      成功
 */
extern int start_plc_update_service(char *file_path/*, update_callback_fn cb*/);


/**
 * 开始IoT升级
 * 返回值： -1 错误
 * 		 0    成功
 */
extern int start_iot_update_service(char *file_path, update_callback_fn cb);



#endif /* _UPDATE_DAEMON_H_ */
