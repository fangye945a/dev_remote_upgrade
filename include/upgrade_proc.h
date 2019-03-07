/******************************************************************************
                  版权所有 (C), 2019-2099, 中联重科

-->文件名	: upgrade_proc.h
-->作  者	: fangye
-->生成日期	: 2019年3月7日
-->功能描述	: upgrade_proc.c 的头文件
******************************************************************************/
#ifndef __UPGRADE_PROC_H__
#define __UPGRADE_PROC_H__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define SERVICE_PAKAGE_PATH "/usrdata/download/service/service.tar.gz"
#define APP_PAKAGE_PATH "/usrdata/download/apps/app.tar.gz"
#define MCU_EXE_PATH "/usrdata/download/mcu/mcu.bin" 
#define PLC_EXE_PATH "/usrdata/download/other/plc.bin"

typedef enum _UPGRADE_TYPE //升级类型
{
	SERVICE_UPDATE,     //服务程序升级
	APP_UPDATE,			//应用升级
	MCU_UPDATE,         //MCU升级
	PLC_UPDATE			//PLC升级
}UPGRADE_TYPE;
	


extern int upgrade_apps_part();
extern int upgrade_mcu_exe();
extern int upgrade_plc_exe();
extern int upgrade_proc();
extern int upgrade_service_part();



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPGRADE_PROC_H__ */
