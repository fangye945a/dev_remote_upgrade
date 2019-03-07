#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <pthread.h>
#include "cJSON.h"
#include "ftp-manager.h"
#include "mqtt.h"
#include "upgrade_proc.h"
#include "msg_process.h"
#include "param_init.h"
#include "config.h"

static FTP_OPT ftp_arg;  	//FTP请求参数

static pthread_t ftp_thread_id = 0;	//FTP线程ID
static unsigned long long ftp_exec_time = 0; //执行时间

extern tmsg_buffer* main_process_msg;


unsigned long long get_now_ms_time()
{
	unsigned long long time = 0;
	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	time = now_time.tv_sec*1000 + now_time.tv_usec/1000;
	return time;
}


int upload_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	if(get_now_ms_time() - ftp_exec_time > TRANS_PROGRESS_T)	//每500ms上报一次进度
	{
		if(ultotal != 0)
		{
			pub_trans_progress(UPLOADING_STAT, ultotal, ulnow);		//发布上载进度信息
			ftp_exec_time = get_now_ms_time();
		}
	}	
	return 0;
}

int download_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	if(get_now_ms_time() - ftp_exec_time > TRANS_PROGRESS_T)	//每500ms上报一次进度
	{
		if(dltotal != 0)
		{
			pub_trans_progress(DOWNLOADING_STAT, dltotal, dlnow);	//发布下载进度信息
			ftp_exec_time = get_now_ms_time();
		}
	}
	return 0;
}


int ftp_option_init(char *user_key)
{
	strncpy(ftp_arg.user_key, user_key, FTP_URL_MAX_LEN);
	ftp_arg.user_key[FTP_URL_MAX_LEN-1] = '\0'; //添加结束符
	return SUCCESS;
}

int fill_ftp_option(unsigned char *filepath, unsigned char *url)
{
	if(filepath == NULL || url == NULL)
		return FAIL;
	strncpy(ftp_arg.file, filepath, FTP_PATH_MAX_LEN);
	ftp_arg.file[FTP_PATH_MAX_LEN-1] = '\0'; //添加结束符

	strncpy(ftp_arg.url, url, FTP_URL_MAX_LEN);
	ftp_arg.url[FTP_URL_MAX_LEN-1] = '\0'; //添加结束符
	
	return SUCCESS;
}

FTP_OPT *get_ftp_option(void)	//获取ftp配置项
{
	return &ftp_arg;
}

int get_file_size(FILE *file)
{
	int size = 0;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	return size;
}

CURL *curl_init()
{
	curl_global_init(CURL_GLOBAL_DEFAULT); 
	CURL *curl = curl_easy_init();
	if(NULL == curl)
	{
		fprintf(stderr, "Init curl failed.\n");
		exit(1);
	}
	return curl;
}

void curl_set_upload_opt(CURL *curl, const char *url, const char *user_key, FILE *file)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, user_key);
	curl_easy_setopt(curl, CURLOPT_READDATA, file);	
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE, get_file_size(file));
	curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
	
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); //获取进度信息
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, upload_progress_callback);//设置进度回调
}

void curl_set_download_opt(CURL *curl, const char *url, const char *user_key, FILE *file)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, user_key);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); //404页面不存在处理
	
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); //获取进度信息
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, download_progress_callback);//设置进度回调
}

void curl_exit(CURL *curl)
{
	curl_easy_cleanup(curl);
	curl_global_cleanup(); 
}

CURLcode curl_perform(CURL *curl)
{
	CURLcode ret = curl_easy_perform(curl);
	if(ret != 0)
	{
		fprintf(stderr, "Perform curl failed. ret = %d\n",ret);
	}
	return ret;
}

void ftp_pthread_id_clean()
{
	if(ftp_thread_id != 0)
	{
		pthread_join(ftp_thread_id, NULL);
		ftp_thread_id = 0;
	}
}
void *ftp_upload_task(void *arg)
{
	FTP_STATE state;
	CURL *curl;
	FILE *fp = fopen(ftp_arg.file, "r+");
	if(NULL == fp)
	{
		fprintf(stderr, "Open file failed at %s:%d\n", __FILE__, __LINE__);
		state = FTP_UPLOAD_FAIL_NOT_EXIST;
	}
	else
	{
		curl = curl_init();
		curl_set_upload_opt(curl, ftp_arg.url, ftp_arg.user_key, fp);
		if(CURLE_OK == curl_perform(curl))
			state = FTP_UPLOAD_SUCCESS;
		else
			state = FTP_UPLOAD_FAILED;
	}
	
	main_process_msg->sendmsg(main_process_msg, FTP_RESULT, state, NULL, 0);
	curl_exit(curl);

	if(fp)
		fclose(fp);
	return NULL;
}

void ftp_upload(unsigned char *filepath, unsigned char *url)
{
	FTP_STATE state;
	if(ftp_thread_id == 0) 
	{
		fill_ftp_option(filepath,url);//填充参数
		if (pthread_create(&ftp_thread_id, NULL, ftp_upload_task, NULL) != 0)
		{
			printf("Create ftp_download_task failed!\n");
			state = FTP_UPLOAD_FAILED;
		}else
			return;
	}else
		state = FTP_BUZY;

	main_process_msg->sendmsg(main_process_msg, FTP_RESULT, state, NULL, 0);
}


void *ftp_download_task(void *arg)
{
	FTP_STATE state;
	CURL *curl;
	FILE *fp = fopen(ftp_arg.file, "w+");
	if(NULL == fp)
	{
		fprintf(stderr, "Open file failed at %s:%d\n", __FILE__, __LINE__);
		state = FTP_DOWNLOAD_PATH_ERROR;
	}
	else
	{
		curl = curl_init();
		curl_set_download_opt(curl, ftp_arg.url, ftp_arg.user_key, fp);
		if(CURLE_OK == curl_perform(curl))
			state = FTP_DOWNLOAD_SUCCESS;
		else
			state = FTP_DOWNLOAD_FAILED;
	}
	
	main_process_msg->sendmsg(main_process_msg, FTP_RESULT, state, NULL, 0);
	
	if(!strcmp(ftp_arg.file,SERVICE_PAKAGE_PATH))		//文件下载完成，若是升级程序，则告知进行升级
	{
		main_process_msg->sendmsg(main_process_msg, REMOTE_UPGRADE_PROC, SERVICE_UPDATE, NULL, 0);
	}
	else if(!strcmp(ftp_arg.file,APP_PAKAGE_PATH))
	{
		main_process_msg->sendmsg(main_process_msg, REMOTE_UPGRADE_PROC, APP_UPDATE, NULL, 0);
	}
	else if(!strcmp(ftp_arg.file,MCU_EXE_PATH))
	{
		main_process_msg->sendmsg(main_process_msg, REMOTE_UPGRADE_PROC, MCU_UPDATE, NULL, 0);
	}
	else if(!strcmp(ftp_arg.file,PLC_EXE_PATH))
	{
		main_process_msg->sendmsg(main_process_msg, REMOTE_UPGRADE_PROC, PLC_UPDATE, NULL, 0);
	}

	curl_exit(curl);
	if(fp)
		fclose(fp);
	return NULL;
}

void ftp_download(unsigned char *filepath, unsigned char *url)
{
	FTP_STATE state;
	if(ftp_thread_id == 0) 
	{
		fill_ftp_option(filepath,url);//填充参数
		if (pthread_create(&ftp_thread_id, NULL, ftp_download_task, NULL) != 0)
		{
			printf("Create ftp_download_task failed!\n");
			state = FTP_DOWNLOAD_FAILED;
		}else
			return;
	}else
		state = FTP_BUZY;
		
	main_process_msg->sendmsg(main_process_msg, FTP_RESULT, state, NULL, 0);
}
