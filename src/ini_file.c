#define _GNU_SOURCE //strchrnul函数需要开启对所有标准化(ANSI、POXIS等)的支持
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <libgen.h>
#include <errno.h>

#include "ini_file.h"

#define SEM_IDENTIFY	'p'
#define SHM_IDENTIFY	's'

union semun
{
    int val;                    /* value for SETVAL */
    struct semid_ds* buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int* array;  /* array for GETALL, SETALL */
    struct seminfo* __buf;      /* buffer for IPC_INFO */
};

#define MAX_LINE_LEN	(1024)
typedef struct _value_item
{
    char* name;	/* the key value name. */
    char* value;

    struct _value_item* next;
} value_item;
typedef struct _section_item
{
    char* name;	/* the section name. */
    int count;	/* the number of key in this section. */

    value_item* head;
    value_item* tail;

    struct _section_item* next;
} section_item;

typedef struct _cfg_hd
{
    char* name;	/* config file name. */
    int count;	/* the number of section in this config file. */
    int modify;	/* identify config file if changed. */
    key_t sem_key;

    section_item* head;
    section_item* tail;
} cfg_hd;
static char* up_default = "undefined";


char* basename_ta(char* path, char* buf, int buflen)
{

#define DEFAULT_RESULT_DOT     "."
#define DEFAULT_RESULT_SLASH   "/"

    /* 如果输入的路径长度小于 PATH_MAX，
     * 则使用自动变量 i_fixed_bufer 作为内部缓冲区 ,
     * 否则申请堆内存做为字符串存放缓冲区。
     */

    char i_fixed_buf[MAX_LINE_LEN + 1];
    const int i_fixed_buf_len = sizeof(i_fixed_buf) / sizeof(char);

    char* result = buf;
    char* i_buf = NULL;
    int i_buf_len = 0;

    int adjusted_path_len = 0;
    int path_len = 0;

    int i, j;

    if (path == NULL)
    {
        /* 如果输入为空指针，则直接返回当前目录 */
        path = DEFAULT_RESULT_DOT;
    }

    /* 分配内部缓冲区用来存放输入字符串 */
    path_len = strlen(path);
    if ((path_len + 1) > i_fixed_buf_len)
    {
        i_buf_len = (path_len + 1);
        i_buf = (char*) malloc(i_buf_len * sizeof(char));
    }
    else
    {
        i_buf_len = i_fixed_buf_len;
        i_buf = i_fixed_buf;
    }

    /* 拷贝字符串到缓冲区，以便接下来对字符串做预处理 */
    strcpy(i_buf, path);
    adjusted_path_len = path_len;

    /* 预处理：删除路径未的路径符号 '/'; */
    if (adjusted_path_len > 1)
    {
        while (i_buf[adjusted_path_len - 1] == '/')
        {
            if (adjusted_path_len != 1)
            {
                adjusted_path_len--;
            }
            else
            {
                break;
            }
        }
        i_buf[adjusted_path_len] = '\0';
    }

    /* 预处理：折叠最后出现的连续 '/'; */
    if (adjusted_path_len > 1)
    {

        for (i = (adjusted_path_len - 1), j = 0; i >= 0; i--)
        {
            if (j == 0)
            {
                if (i_buf[i] == '/')
                {
                    j = i;
                }
            }
            else
            {
                if (i_buf[i] != '/')
                {
                    i++;
                    break;
                }
            }
        }

        if (j != 0 && i < j)
        {
            /* 折叠多余的路径符号 '/';
             */
            strcpy(i_buf + i, i_buf + j);
        }

        adjusted_path_len -= (j - i);
    }

    /* 预处理：寻找最后一个路径符号 '/' */
    for (i = 0, j = -1; i < adjusted_path_len; i++)
    {
        if (i_buf[i] == '/')
        {
            j = i;
        }
    }

    /* 查找 basename */
    if (j >= 0)
    {

        /* found one '/' */
        if (adjusted_path_len == 1)    /* 输入的是跟路径 ("/")，则返回根路径 */
        {
            if (2 > buflen)
            {
                return NULL;
            }
            else
            {
                strcpy(result, DEFAULT_RESULT_SLASH);
            }
        }
        else
        {
            if ((adjusted_path_len - j) > buflen)    /* 缓冲区不够，返回空指针 */
            {
                result = NULL;
            }
            else
            {
                strcpy(result, (i_buf + j + 1));
            }
        }

    }
    else
    {

        /* no '/' found  */
        if (adjusted_path_len == 0)
        {
            if (2 > buflen)                /* 如果传入的参数为空字符串 ("") */
            {
                return NULL;                /* 直接返回当前目录 (".")  */
            }
            else
            {
                strcpy(result, DEFAULT_RESULT_DOT);
            }
        }
        else
        {
            if ((adjusted_path_len + 1) > buflen)
            {
                result = NULL;             /* 缓冲区不够，返回空指针    */
            }
            else
            {
                strcpy(result, i_buf);   /* 拷贝整个字符串做为返回值   */
            }
        }
    }

    if (i_buf_len != i_fixed_buf_len)     /* 释放缓冲区         */
    {
        free(i_buf);
        i_buf = NULL;
    }

    return result;
}


int record(char* msg)
{
#if 0
    char buff[1024];
    int len = 0;
    static FILE* ini_parser = NULL;

    ini_parser = fopen("/tmp/decoder.cfg", "ab");
    if (ini_parser == NULL)
    {
        return -1;
    }
    fseek(ini_parser, 0, SEEK_END);
    memset(buff, 0x00, 1024);
    strcat(buff, msg);
    len = strlen(buff);
    fwrite(buff, len, 1, ini_parser);
    fclose(ini_parser);
#endif
    return 0;
}
static char* skip_whitespace(const char* s)
{
    while (isspace(*s))
    {
        ++s;
    }

    return (char*) s;
}
static char* get_trimmed_slice(char* s, char* e)
{
    /* First, consider the value at e to be nul and back up until we
     * reach a non-space char.  Set the char after that (possibly at
     * the original e) to nul. */
    while (e-- > s)
    {
        if (!isspace(*e))
        {
            break;
        }
    }
    e[1] = 0;

    /* Next, advance past all leading space and return a ptr to the
     * first non-space char; possibly the terminating nul. */
    return skip_whitespace(s);
}

/* IPC semp control. */
/**
 *
 * @brief	check the semaph of the key if exists.
 *
 * @return	1 if exists, or 0.
 *
 */
static int semaph_check(key_t key)
{
    int semid = -1;

    semid = semget(key, 0, 0666);
    if (semid != -1)
    {
        return 1;
    }

    return 0;
}

static int semaph_delete(key_t key)
{
    int ret = -1;
    int semid = -1;
    union semun options;
    memset(&options, 0, sizeof(union semun));

    semid = semget(key, 0, 0666);
    if (semid != -1)
    {
        ret = semctl(semid, 0, IPC_RMID, options);
    }

    return ret;
}

static int semaphcreate(key_t key, int nums, int val)
{
    int semid = 0;
    int lp = 0;
    union semun options;
    int ret = 0;

    if (nums <= 0)
    {
        printf("sema array size err, size:%d\n", nums);
        ret = -1;
        goto semacreate_err;
    }

    if (val < 0)
    {
        printf("sema init value err, value:%d\n", val);
        ret = -1;
        goto semacreate_err;
    }

    semaph_delete(key);

    semid = semget(key, nums, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1)
    {
        //can't create
        printf("sema create \n");
        ret = -1;
        goto semacreate_err;
    }

    //set origin value
    options.val = val;
    for (lp = 0; lp < nums; lp++)
    {
        if (semctl(semid, lp, SETVAL, options) == -1)
        {
            printf("sema set origin\n");
            ret = -1;
            goto semacreate_err;
        }
    }

    ret = 0;

semacreate_err:
    return ret;
}
/* semaphore lock
 *  id:  ret val for semaphore open
 *  index: semaphore index of set
 *  flag:  0, wait,   IPC_NOWAIT, no wait
*/
static int semaoplock(int id, int index_c, int flag)
{
    struct sembuf lock_it;
    int ret = -1;

    lock_it.sem_num = index_c; // semaphore index
    lock_it.sem_op  = -1; // operation
    lock_it.sem_flg = flag; // operation flags:  0, wait  , IPC_NOWAIT, no wait

    ret = semop(id, &lock_it, 1);
    if (ret == -1)
    {
        printf("[INI]Call %s() error. \n", __FUNCTION__);
    }

    return ret;
}

/* semaphore unlock
 *  id:  ret val for semaphore open
 *  index: semaphore index of set
*/
static int semaopunlock(int id, int index_c)
{
    struct sembuf lock_it;
    int ret = -1;

    lock_it.sem_num = index_c; // semaphore index
    lock_it.sem_op  = 1; // operation
    lock_it.sem_flg = IPC_NOWAIT; // operation flags

    ret = semop(id, &lock_it, 1);
    if (ret == -1)
    {
        printf("[INI]Call %s() error. \n", __FUNCTION__);
    }

    return ret;
}

static int semaphlock(key_t key, int index_c)
{
    int semid = -1;

    semid = semget(key, 0, 0666);
    if (semid == -1)
    {
        printf("lock: sema get , key= %d, index = %d\n", key, index_c);
        return -1;
    }

    return semaoplock(semid, index_c, 0);
}

static int semaphunlock(key_t key, int index_c)
{
    int semid = -1;

    semid = semget(key, 0, 0666);
    if (semid == -1)
    {
        printf("unlock: sema get , key= %d, index = %d\n", key, index_c);

        return -1;
    }

    return semaopunlock(semid, index_c);
}


static void add_section(cfg_hd* head, char* sec)
{
    section_item* section = NULL;

    /* malloc a section node. */
    section = (section_item*)malloc(sizeof(section_item));
    if (!section)
    {
        printf("Malloc error.\n");
        return;
    }

    section->name = strdup(sec);
    section->count = 0;
    section->head = NULL;
    section->tail = NULL;
    section->next = NULL;

    /* add section node in cfg_head. */
    if (head->tail)
    {
        head->tail->next = section;
    }
    else
    {
        head->head = section;
    }
    head->tail = section;
    head->count++;
}
static void add_item(cfg_hd* head, char* key, char* value)
{
    value_item* node = NULL;

    /* check if has a section. */
    if (!head->tail)
    {
        printf("Not has a section, so drop the key-value. \n");
        return;
    }

    /* malloc a value node. */
    node = (value_item*)malloc(sizeof(value_item));
    if (!node)
    {
        printf("Malloc error. \n");
        return;
    }

    node->name = strdup(key);
    node->value = strdup(value);
    node->next = NULL;

    /* add a value node into section list. */
    if (head->tail->tail)
    {
        head->tail->tail->next = node;
    }
    else
    {
        head->tail->head = node;
    }
    head->tail->tail = node;
    head->tail->count++;
}
static void add_item_in_sec(section_item* sec, char* key, char* value)
{
    value_item* node = NULL;

    /* check if has a section. */
    if (!sec)
    {
        printf("Not has a section, so drop the key-value. \n");
        return;
    }

    /* malloc a value node. */
    node = (value_item*)malloc(sizeof(value_item));
    if (!node)
    {
        printf("Malloc error. \n");
        return;
    }

    node->name = strdup(key);
    node->value = strdup(value);
    node->next = NULL;

    /* add a value node into section list. */
    if (sec->tail)
    {
        sec->tail->next = node;
    }
    else
    {
        sec->head = node;
    }
    sec->tail = node;
    sec->count++;
}

static void parse_one_line_buf(char* buf, int len, cfg_hd* head)
{
    char* e = NULL;
    char* s = buf;

    /* Trim leading and trailing whitespace, ignoring comments, and
     * check if the resulting string is empty. */
    if (!*(s = get_trimmed_slice(s, (char*)strchrnul(s, '#'))))
    {
        return;
    }

    /* Check for a section header. */
    if (*s == '[')
    {
        /* Unlike the old code, we ignore leading and trailing
         * whitespace for the section name.  We also require that
         * there are no stray characters after the closing bracket. */
        if (!(e = strchr(s, ']'))	/* Missing right bracket? */
                || e[1]					/* Trailing characters? */
                || !*(s = get_trimmed_slice(s + 1, e)) /* Missing name? */
           )
        {
            printf("section header, filename:\n");
        }

        //printf ("section : %s \n", s);
        add_section(head, s);
        return;
    }

    /* Process sections. */

    /* Since we trimmed leading and trailing space above, we're
     * now looking for strings of the form
     *    <key>[::space::]*=[::space::]*<value>
     * where both key and value could contain inner whitespace. */

    /* First get the key (an applet name in our case). */
    if (!!(e = strchr(s, '=')))
    {
        s = get_trimmed_slice(s, e);
    }
    if (!e || !*s)
    {
        /* Missing '=' or empty key. */
        printf("Missing \'=\' or empty key.\n");
        return;
    }

    /* Ok, we have an applet name.  Process the rhs if this
     * applet is currently built in and ignore it otherwise.
     * Note: This can hide config file bugs which only pop
     * up when the busybox configuration is changed. */
    /* Note: We currently don't check for duplicates!
     * The last config line for each applet will be the
     * one used since we insert at the head of the list.
     * I suppose this could be considered a feature. */

    /* Get the specified mode. */

    e = skip_whitespace(e + 1);
    if (!e)
    {
        printf("key no value\n");
        return;
    }

    //printf ("s:%s e:%s \n", s, e);
    add_item(head, s, e);
}

static void* parse_file_from_data(char* file, char* buf, int len)
{
    char line[MAX_LINE_LEN];
    cfg_hd* head = NULL;
    char* line_head = buf;

    head = (cfg_hd*)calloc(1, sizeof(cfg_hd));
    head->name = strdup(file);
    head->count = 0;
    head->modify = 0;
    head->sem_key = 0;
    head->head = NULL;
    head->tail = NULL;

    do
    {
        memset(line, 0x00, sizeof(line));
        if (!memccpy(line, line_head, '\n', MAX_LINE_LEN - 1))
        {
            //printf ("End of file. \n");
            break;
        }

        /* parse one line buffer. */
        parse_one_line_buf(line, MAX_LINE_LEN - 1, head);

        /* get next line head. */
        line_head = strstr(line_head, "\n");
        if (!line_head || *(line_head + 1) == '\0')
        {
            break;
        }

        line_head++;
    }
    while (1);

    return (void*)head;
}

#if 0
char* read_file_in_buf(char* name, int* len)
{
    struct stat st;
    FILE* fd = NULL;
    char* buf = NULL;

    if (!name)
    {
        printf("config file error. \n");
        return NULL;
    }

    if ((stat(name, &st) != 0) || !(fd = fopen(name, "rb")))
    {
        printf("read file error. \n");
        return NULL;
    }

    printf("file size: %d \n", (int)st.st_size);

    buf = (char*)malloc(st.st_size + 2);
    fread(buf, st.st_size, 1, fd);

    /* to Fix last line lost bug. */
    buf[st.st_size] = '\n';
    buf[st.st_size + 1] = '\0';

    *len = st.st_size + 2;

    fclose(fd);

    return buf;
}
#endif

static void free_key_value_list(section_item* h)
{
    value_item* node = NULL;
    while (h->head)
    {
        node = h->head;
        h->head = h->head->next;

        //printf ("%s=%s\r\n", node->name, node->value);
        if (node->name)
        {
            free(node->name);
        }
        if (node->value)
        {
            free(node->value);
        }
        free(node);
    }
}
static void free_section_list(cfg_hd* head)
{
    cfg_hd* h = head;
    section_item* sec = NULL;

    while (h->head)
    {
        sec = h->head;
        h->head = h->head->next;

        //printf ("[%s]\r\n", sec->name);
        free_key_value_list(sec);

        if (sec->name)
        {
            free(sec->name);
        }
        free(sec);
    }
}

static int dump_config_list_to_file(cfg_hd* h, char* path)
{
    FILE* back = NULL;
    section_item* sec = NULL;
    value_item* node = NULL;

    back = fopen(path, "w");
    if (!back)
    {
        perror("Open backup file error.\n");
        goto FAIL;
    }

    /* write config list to file. */
    sec = h->head;
    while (sec)
    {
        if (sec->name)
        {
            fprintf(back, "[%s]\r\n", sec->name);

            node = sec->head;
            while (node)
            {
                if (node->name)
                {
                    fprintf(back, "%s=%s\r\n", node->name, node->value);
                }

                node = node->next;
            }
        }

        sec = sec->next;
    }

    /* sync file to flash. */
    fsync(fileno(back));
    fclose(back);

    return 0;

FAIL:
    return -1;
}

void save_ini_file(void* hd)
{
    cfg_hd* h = NULL;
    char path[MAX_FILE_PATH_LEN];
    int ret = -1;

    if (!hd)
    {
        printf("Invalide config handle.\n");
        goto EXIT;
    }

    h = (cfg_hd*)hd;
    if (!h->modify)
    {
        printf("The config file not changed. \n");
        goto EXIT;
    }

    memset(path, 0x00, sizeof(path));
    /* write to the backup file. */
    snprintf(path, MAX_FILE_PATH_LEN - 1, "%s"INIT_FILE_BAK_SUFFIX, h->name);
    ret = dump_config_list_to_file(h, path);
    if (ret != 0)
    {
        printf("[INI] Write file(%s) error.\n", path);
    }

    /* write current config file. */
    ret = dump_config_list_to_file(h, h->name);
    if (ret != 0)
    {
        printf("[INI] Write file(%s) error.\n", h->name);
    }

    /* set modify flag. */
    h->modify = 0;

    /* reload IPC shared buffer. */
    ret = init_config_file(h->name);
    if (ret != 0)
    {
        printf("[INI] Reload file(%s) error.\n", h->name);
    }
EXIT:
    return;
}

void close_ini_file(void* hd)
{
    cfg_hd* h = NULL;

    if (!hd)
    {
        printf("Invalide config handle.\n");
        return;
    }
    h = (cfg_hd*)hd;

    free_section_list(h);

    if (h->name)
    {
        free(h->name);
    }

    if (h->modify)
    {
        printf("[INI] Not save the config file after you change. Modify will be lost.\n");
    }

    /* free the sem lock. */
    if (h->sem_key)
    {
        semaphunlock(h->sem_key, 0);
    }

    free(h);
}

char* get_item_value(void* hd, char* section, char* name, char* defaultval)
{
    cfg_hd* h = NULL;
    section_item* sec = NULL;
    value_item* node = NULL;

    if (!hd)
    {
        printf("Invalide handle.\n");
        goto GET_FAIL;
    }

    if (!section || !name)
    {
        goto GET_FAIL;
    }

    h = (cfg_hd*)hd;

    /* get the section node. */
    sec = h->head;
    while (sec)
    {
        if (sec->name && strcmp(sec->name, section) == 0)
        {
            break;
        }
        sec = sec->next;
    }

    /* not found the section in list. */
    if (!sec)
    {
        goto GET_FAIL;
    }

    node = sec->head;
    while (node)
    {
        if (node->name && strcmp(node->name, name) == 0)
        {
            /* we get the key-value. */
            return node->value;
        }
        node = node->next;
    }

GET_FAIL:
    return (defaultval ? : up_default);
}

int set_item_value(void* hd, char* section, char* name, char* val)
{
    cfg_hd* h = NULL;
    section_item* sec = NULL;
    value_item* node = NULL;

    if (!hd)
    {
        printf("Invalide handle.\n");
        goto SET_FAIL;
    }

    if (!section || !name || !val)
    {
        goto SET_FAIL;
    }

    h = (cfg_hd*)hd;

    /* get the section node. */
    sec = h->head;
    while (sec)
    {
        if (sec->name && strcmp(sec->name, section) == 0)
        {
            break;
        }
        sec = sec->next;
    }

    /* not found the section in list. */
    if (!sec)
    {
        goto ADD_ITEM;
    }

    node = sec->head;
    while (node)
    {
        if (node->name && strcmp(node->name, name) == 0)
        {
            /* we get the key-value. */

            /* check the value if diff. */
            if (node->value)
            {
                if (strcmp(node->value, val) != 0)
                {
                    /* release the node value. */
                    free(node->value);
                    node->value = NULL;

                    node->value = strdup(val);
                    h->modify = 1;

                    /* return the successed value. */
                    return 0;
                }
                else
                {
                    printf("[INI] Value not changed.{%s}{%s} \n", node->value, val);
                    return 0;
                }
            }
        }
        node = node->next;
    }

ADD_ITEM:
    /* not found the node in list, so we add a new section and key-value into list. */
    if (sec)
    {
        printf("[INI]Add key:%s \n", name);
        add_item_in_sec(sec, name, val);
    }
    else
    {
        printf("[INI]Add section:%s key:%s \n", section, name);
        add_section(h, section);
        add_item(h, name, val);
    }

    /* FIXME: add new item maybe faild. */
    h->modify = 1;

    return 0;
SET_FAIL:
    return -1;
}

static inline void generate_tmp_file_path(char* file, char* path, int len)
{
    memset(path, 0x00, len);
    snprintf(path, len, "%s%s", INI_TMP_FILE_PREFIX, file);
}

static inline void generate_ini_file_path(char* file, char* path, int len)
{
    memset(path, 0x00, len);
//    snprintf(path, len, "%s%s", INI_FILE_PATH_PREFIX, file);
    snprintf(path, len, "%s", file);
}

static int create_temp_empty_file(char* file, char* path, int len)
{
    FILE* fd = NULL;

    generate_tmp_file_path(file, path, len);

    /* remove the tmp file, if have or not. */
    remove(path);

    /* here, we never use system() call. */
    fd = fopen(path, "w");
    if (!fd)
    {
        goto FAIL;
    }

    fclose(fd);

    return 0;
FAIL:
    return -1;
}

static int create_ipc_shm(key_t key, char* file)
{
    int shmid = -1;
    FILE* fd = NULL;
    char* buf = NULL;
    struct stat st;
    int ret = 0;

    /* here we not to stat() file. */
    if ((stat(file, &st) != 0) || !(fd = fopen(file, "rb")))
    {
        goto FAIL;
    }

    shmid = shmget(key, 0, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    if (shmid != -1)  //force rm
    {
        //printf ("Delte exists shm id. \n");
        shmctl(shmid, IPC_RMID, 0);
    }

    shmid = shmget(key, st.st_size + 2, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | IPC_CREAT | IPC_EXCL);
    if (shmid == -1)
    {
        perror("call shmget failed.\n");
        goto FAIL;
    }

    buf = (char*)shmat(shmid, 0, 0);
    if (buf == (void*) - 1)
    {
        perror("call shmat failed.\n");
        goto FAIL;
    }

    /* read all config content into buf. */
    ret = fread(buf, st.st_size, 1, fd);
    if (ret < 0)
    {
        goto FAIL;
    }

    /* Add to Fix last line lost bug by xiongfei. */
    buf[st.st_size] = '\n';
    buf[st.st_size + 1] = '\0';

    shmdt(buf);
    fclose(fd);
    return 0;

FAIL:
    if (shmid != -1)
    {
        //printf ("Delte exists shm id. \n");
        shmctl(shmid, IPC_RMID, 0);
    }
    if (fd != NULL)
    {
        fclose(fd);
    }
    return -1;
}

int init_config_file(char* name)
{
    char file_path[MAX_FILE_PATH_LEN] = "\0";
    struct stat st;
    int ret = -1;
    key_t semaph = -1;
    key_t key = -1;
    char* file = NULL;
    char tmp_buf[1024] = {0};

    if (!name)
    {
        printf("Invalide file name. \n");
        goto FAIL;
    }

    /* check the orig file. */
    //    file = basename(name);
    file = basename_ta(name, tmp_buf, 1024);

//    generate_ini_file_path(file, file_path, MAX_FILE_PATH_LEN - 1);
    generate_ini_file_path(name, file_path, MAX_FILE_PATH_LEN - 1);

    if (stat(file_path, &st) != 0)
    {
        printf("file(%s) can not be stat.\n", file_path);
        goto FAIL;
    }

    /* create temp file to generate uniqu key_t for each shm. */
    ret = create_temp_empty_file(file, file_path, MAX_FILE_PATH_LEN - 1);
    if (ret != 0)
    {
        printf("create temp file fail. %s \n", file_path);
        goto FAIL;
    }
    //    printf("temp file path: %s \n", file_path);

    /* create IPC semp id. */
    semaph = ftok(file_path, SEM_IDENTIFY);
    if (semaph == -1)
    {
        perror("ftok:");
        printf("[INI] ftok error %d\n", errno);
        goto FAIL;
    }

    if (semaph_check(semaph))
    {
        printf("[INI] Semaph has exists try to del. key:0x%x\n", semaph);
        semaph_delete(semaph);
        if (semaph_check(semaph))
        {
            printf("[INI] Semaph has exists. key:0x%x\n", semaph);
            goto FAIL;
        }

    }

    if (semaphcreate(semaph, 1, 1) == -1)
    {
        printf("[INI] semaphcreate error %d\n", errno);
        goto SEM_SHM_FAIL;
    }
    //    printf("sem:%s: %x \n", name, semaph);

    /* create IPC shm id. */
    key = ftok(file_path, SHM_IDENTIFY);
    if (key == -1)
    {
        perror("ftok:");
        goto SEM_SHM_FAIL;
    }

    /* read config file into IPC share memory. */
//    generate_ini_file_path(file, file_path, MAX_FILE_PATH_LEN - 1);
    generate_ini_file_path(name, file_path, MAX_FILE_PATH_LEN - 1);

    if (create_ipc_shm(key, file_path) != 0)
    {
        printf("create file(%s) shm fail. \n", file_path);

        /* read backup file. */
        strcat(file_path, INIT_FILE_BAK_SUFFIX);
        if (create_ipc_shm(key, file_path) != 0)
        {
            printf("create file(%s) shm fail. \n", file_path);

            /* read orig file. */
            /* FIXME: if crate shm fail, we need to delete IPC semp. */

            goto SEM_SHM_FAIL;
        }
    }
    //    printf("shm:%s: %x \n", name, key);

    return 0;

SEM_SHM_FAIL:
    /* delete semaph. */
    if (semaph != -1)
    {
        semaph_delete(semaph);
    }

FAIL:
    printf("fail \n");
    return -1;
}

void term_config_file(char* name)
{
    struct stat st;
    char file_path[MAX_FILE_PATH_LEN] = "\0";
    key_t key = -1;
    key_t sem_key = -1;
    int shmid = 0;
    char tmp_buf[1024] = {0};

    if (!name)
    {
        printf("Invalide file name. \n");
        goto FAIL;
    }

    /* construct temp file path. */
    generate_tmp_file_path(basename_ta(name, tmp_buf, 1024), file_path, MAX_FILE_PATH_LEN - 1);
    //    printf("temp file : %s \n", file_path);

    /* check the tmp file. In out STB, the tmp fs alwayls in memory, so not bad to flash. */
    if (stat(file_path, &st) != 0)
    {
        printf("file(%s) can not be stat.\n", file_path);

        goto FAIL;
    }

    /* get the sem key. */
    sem_key = ftok(file_path, SEM_IDENTIFY);
    if (-1 == sem_key)
    {
        perror("ftok");
        goto FAIL;
    }

    /* lock the config file. */
    semaphlock(sem_key, 0);

    /* get the shm buffer. */
    key = ftok(file_path, SHM_IDENTIFY);
    if (key == -1)
    {
        perror("ftok:");
        goto FREE_LOCK;
    }

    shmid = shmget(key, 0, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    if (shmid == -1)
    {
        perror("call shmget failed.\n");
        goto FREE_LOCK;
    }

    /* delete IPC shared memory. */
    shmctl(shmid, IPC_RMID, 0);
FREE_LOCK:
    if (sem_key != -1)
    {
        semaph_delete(sem_key);
    }
FAIL:
    return;
}

void* parse_ini_file(char* name)
{
    key_t key = -1;
    key_t sem_key = -1;
    int shmid = 0;
    char* buf = NULL;
    cfg_hd* head = NULL;
    struct stat st;
    char file_path[MAX_FILE_PATH_LEN] = "\0";
    int try_count = 0;
    char tmp_buf[1024] = {0};
    //    printf("parse_ini_file  ---1. \n");
    if (!name)
    {
        record("Invalide file name. \n");
        goto FAIL;
    }

RETRY_GET:
    /* construct temp file path. */
    generate_tmp_file_path(basename_ta(name, tmp_buf, 1024), file_path, MAX_FILE_PATH_LEN - 1);
    //    printf("temp file : %s %s\n", name, file_path);
    record("temp file : %s \n");
    record(file_path);

    /* check the tmp file. In out STB, the tmp fs alwayls in memory, so not bad to flash. */
    if (stat(file_path, &st) != 0)
    {
        record("file(%s) can not be stat.\n");
        record(file_path);
        /* FIXME: if not have tmp file, we think it not read into shared memory. */
        /* we check if have the orig file, if have we init it. */
//        generate_ini_file_path(basename_ta(name, tmp_buf, 1024), file_path, MAX_FILE_PATH_LEN - 1);
        generate_ini_file_path(name, file_path, MAX_FILE_PATH_LEN - 1);
        if (stat(file_path, &st) == 0)
        {
            try_count++;
            if (try_count == 1 && (init_config_file(basename_ta(name, tmp_buf, 1024)) == 0))
            {
                goto RETRY_GET;
            }
        }

        goto FAIL;
    }

    /* get the sem key. */
    sem_key = ftok(file_path, SEM_IDENTIFY);
    if (-1 == sem_key)
    {
        record("ftok");
        goto FAIL;
    }

    //    printf("start lock sem\n");
    /* lock the config file. */
    semaphlock(sem_key, 0);
    //    printf("ok lock sem\n");

    /* get the shm buffer. */
    key = ftok(file_path, SHM_IDENTIFY);
    if (key == -1)
    {
        record("ftok:");
        goto FREE_LOCK;
    }
    errno = 0;
    //    printf("key:%X\n", key);
    shmid = shmget(key, 0, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    if (shmid == -1)
    {
        record("call shmget failed.\n");
        printf("\ncall shmget failed.%s\n", strerror(errno));
        goto FREE_LOCK;
    }

    buf = (char*)shmat(shmid, 0, SHM_RDONLY);
    if (!buf)
    {
        record("[INI] Get shared memory buf error.\n");
        goto FREE_LOCK;
    }
    //	printf("temp file : %s key %X get buf:%p\n",name,key,buf);

    /* parse the ini buffer. */
//    generate_ini_file_path(basename_ta(name, tmp_buf, 1024), file_path, MAX_FILE_PATH_LEN - 1);
    generate_ini_file_path(name, file_path, MAX_FILE_PATH_LEN - 1);

    head = parse_file_from_data(file_path, buf, 0);

    /* detach the shared memory. */
    shmdt((void*)buf);

    if (!head)
    {
        record("[INI] create config file handle error.\n");
        goto FREE_LOCK;
    }

    /* save the semaph key to close. */
    head->sem_key = sem_key;

    return (void*)head;

FREE_LOCK:
    if (sem_key != -1)
    {
        semaphunlock(sem_key, 0);
    }
FAIL:
    return NULL;
}


