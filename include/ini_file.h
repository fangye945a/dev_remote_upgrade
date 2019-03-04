#ifndef __CFG_PARSER_H__
#define __CFG_PARSER_H__

//#define INI_FILE_PATH_PREFIX	"/data/QuecOpen/app/cfg/"
#define INI_TMP_FILE_PREFIX		"/tmp/"
#define INIT_FILE_BAK_SUFFIX	".bak"

#define MAX_FILE_PATH_LEN		(1024)

/**
 *
 * @brief	init a config file context, #ONLY need call once when system startup.
 *
 * 			It's alloc IPC shared memory.
 *
 * @param	name	[in]	the config file.
 *
 * @return	return 0 if successed, or -1.
 *
 */
int init_config_file(char* name);

/**
 *
 * @brief	term a config file context, called when need.
 *
 * 			It's free IPC shared memory, MUSE be after #close_ini_file(), if you call #parse_ini_file().
 *
 * @param	name	[in]	a config file name.
 *
 * @return  NONE.
 *
 */
void term_config_file(char* name);

/**
 *
 * @brief	Parse a config file into a data struct from IPC shared memory.
 *
 * 			For a process and thread safe, we use IPC semaph to lock the config file, so
 *
 * 			call #close_ini_file() when you not use.
 *
 * 			a full call step often use follow step:
 *
 * 			1. call parse_ini_file()
 *
 * 			2. call get_item_value() or set_item_value();
 *
 * 			3. call save_ini_file() as you need.
 *
 * 			4. call close_ini_file() at last #MUST.
 *
 * @param	name	[in]	the config file.
 *
 * @return	return the handle if successed, or NULL.
 *
 */
void* parse_ini_file(char* name);

/**
 *
 * @brief	get a key-value from data struct.
 *
 * @param	hd	[in]	the config file handle.
 *
 * @param	section	[in]	the section name.
 *
 * @param	name	[in]	the key name.
 *
 * @param	defaultval	[in]	the default value when not found the key-value.
 *
 * @return	return the value if successed, or #NULL.
 *
 */
char* get_item_value(void* hd, char* section, char* name, char* defaultval);

/**
 *
 * @brief	set a key-value from data struct.
 *
 * @param	hd	[in]	the config file handle.
 *
 * @param	section	[in]	the section name.
 *
 * @param	name	[in]	the key name.
 *
 * @param	val	[in]	the key-value.
 *
 * @return	return 0 if successed, or -1.
 *
 */
int set_item_value(void* hd, char* section, char* name, char* val);

/**
 *
 * @brief	Save data into file, it's will #IO flash, so called when you really need.
 *
 * @param	hd	[in]	the config file handle.
 *
 * @return	NONE.
 *
 */
void save_ini_file(void* hd);

/**
 *
 * @brief	close the config file. It's #MUST be call at last.
 *
 * 			It's will unlock the IPC semaph lock.
 *
 * @param	hd	[in]	the config file handle.
 *
 * @return	NONE.
 *
 */
void close_ini_file(void* hd);

#endif	/* __CFG_PARSER_H__ */
