//ftp-manager.h

#ifndef _FTP_MANAGER_H_
#define _FTP_MANAGER_H_

#define USER_KEY_MAX_LEN	256
#define FTP_URL_MAX_LEN		1024
#define FTP_PATH_MAX_LEN	1024

/*FTP OPERATION CODE*/
typedef enum FTP_STATE
{
	FTP_UPLOAD_SUCCESS,
	FTP_UPLOAD_FAIL_NOT_EXIST, //上传的文件不存在
	FTP_UPLOAD_FAILED,
	FTP_DOWNLOAD_PATH_ERROR,	//路径不存在
	FTP_DOWNLOAD_SUCCESS,
	FTP_DOWNLOAD_FAILED, 
	FTP_BUZY	//任务繁忙 
}FTP_STATE;

/*FTP OPERATIONS OPTIONS*/
typedef struct _FTP_OPT
{
	char url[FTP_URL_MAX_LEN];		/*url of ftp*/
	char user_key[USER_KEY_MAX_LEN];		/*username:password*/
	char file[FTP_PATH_MAX_LEN];		/*filepath*/
}FTP_OPT;

#ifdef __cplusplus
	extern "C" {
#endif

extern int ftp_option_init(char *user_key);

extern void ftp_upload(unsigned char *filepath, unsigned char *url);

extern void ftp_download(unsigned char *filepath, unsigned char *url);

extern	void ftp_pthread_id_clean();	//回收线程清空线程ID

extern unsigned long long get_now_ms_time();


#ifdef __cplusplus
	}
#endif
#endif
