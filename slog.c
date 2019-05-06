#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#include "slog.h"


#define LOG_FILE_NAME_MAX_LEN           64
#define DEFAULT_LOG_FILE_NAME           "/var/log/slog.log"
#define DEFAULT_ROTATING_FILE_SIZE      5
#define DEFAULT_ROTATING_FILE_MAX_LINE  100000


static char *slog_type_str[] = {
    "SLOG_TYPE_EMERG",    
    "SLOG_TYPE_ALERT",
    "SLOG_TYPE_CRIT", 
    "SLOG_TYPE_ERR",
    "SLOG_TYPE_WARNING",
    "SLOG_TYPE_NOTICE",
    "SLOG_TYPE_INFO",
    "SLOG_TYPE_DEBUG",
};



typedef struct {
    char *log_file_name;                    //日志文件名
    FILE *fp;                               //指向log_file_name的文件指针
    unsigned int rotating_file_size;        //轮转文件大小，单位为MB
    unsigned int current_line;              //fp指向的文件当前行数
    unsigned int rotating_file_max_line;    //轮转文件的最大行数
    int is_initialized;                     //本日志模块是否已经初始化
    struct timeval startup_time;            //初始化的时间
    unsigned int rotating_file_cnt;         //轮转文件个数，不包括当前文件log_file_name
}slog_ctrl_t;

slog_ctrl_t slog_ctrl;




int init_slog(void){
    if(slog_ctrl.is_initialized == 1){
        return 0;    
    }
    slog_ctrl.log_file_name = DEFAULT_LOG_FILE_NAME;
    slog_ctrl.fp = fopen(slog_ctrl.log_file_name, "a");
    if(slog_ctrl.fp == NULL){
        perror("fopen");    
        return -1;
    }
    slog_ctrl.rotating_file_size = DEFAULT_ROTATING_FILE_SIZE;
    slog_ctrl.current_line = 0;
    slog_ctrl.rotating_file_max_line = DEFAULT_ROTATING_FILE_MAX_LINE;
    slog_ctrl.is_initialized = 1;
    int ret = gettimeofday(&slog_ctrl.startup_time, NULL);
    if(ret < 0){
        perror("gettimeofday");    
        return -1;
    }
    slog_ctrl.rotating_file_cnt = 0;
    return 0;
}


static void slog_rotate(void){
    fclose(slog_ctrl.fp);
    unsigned int current_rotating_file_cnt = slog_ctrl.rotating_file_cnt;
    //重建轮转文件，如果有缺失的话    
    for(unsigned int i = 1; i <= slog_ctrl.rotating_file_cnt; i++){
        char n1[LOG_FILE_NAME_MAX_LEN] = {0};
        char n2[LOG_FILE_NAME_MAX_LEN] = {0};
        struct stat f1;
        snprintf(n1, sizeof(n1), "%s.%u", slog_ctrl.log_file_name, i);
        snprintf(n2, sizeof(n2), "%s.%u", slog_ctrl.log_file_name, i + 1);
        int ret = stat(n1, &f1);
        if(ret < 0 && errno == ENOENT){
            current_rotating_file_cnt--;
            //最后一个文件不存在的话，这次重命名失败没关系
            rename(n2, n1);
        }
    }
    slog_ctrl.rotating_file_cnt = current_rotating_file_cnt;
    //重命名轮转文件
    for(unsigned int i = slog_ctrl.rotating_file_cnt; i > 0; i--){
        char n1[LOG_FILE_NAME_MAX_LEN] = {0};
        char n2[LOG_FILE_NAME_MAX_LEN] = {0};
        snprintf(n1, sizeof(n1), "%s.%u", slog_ctrl.log_file_name, i);
        snprintf(n2, sizeof(n2), "%s.%u", slog_ctrl.log_file_name, i + 1);
        rename(n1, n2);
    }
    char n[LOG_FILE_NAME_MAX_LEN] = {0};
    snprintf(n, sizeof(n), "%s.%u", slog_ctrl.log_file_name, 1);
    rename(slog_ctrl.log_file_name, n);
    //重建日志文件，并重置行号、增加文件个数
    slog_ctrl.fp = fopen(slog_ctrl.log_file_name, "a");
    if(slog_ctrl.fp == NULL){
        perror("fopen");    
    }
    slog_ctrl.current_line = 0;
    slog_ctrl.rotating_file_cnt++;
}

void slog(slog_type_t slog_type, char *fmt, ...){
    if(slog_ctrl.is_initialized == 0){
         init_slog();   
    }
    struct stat file_info;
    int ret = stat(slog_ctrl.log_file_name, &file_info);
    if(ret < 0 && errno == ENOENT){
        //如果文件不存在，则新建        
        slog_ctrl.fp = fopen(slog_ctrl.log_file_name, "a");
        if(slog_ctrl.fp == NULL){
            perror("fopen");    
        }
        slog_ctrl.current_line = 0;
    }else{
        //根据轮转条件，轮转日志文件
        if(slog_ctrl.current_line >= slog_ctrl.rotating_file_max_line 
            || file_info.st_size / 1024 / 1024 >= slog_ctrl.rotating_file_size){
            slog_rotate();
        }
    }
    struct timeval current_time, sub_time;
    ret = gettimeofday(&current_time, NULL);
    if(ret < 0){
        perror("gettimeofday");    
        return;
    }
    timersub(&current_time, &slog_ctrl.startup_time, &sub_time);
    //每行的开头tag是当前时间与slog初始化的时间差和日志类型字符串
    fprintf(slog_ctrl.fp, "%lu.%lu:%s:", sub_time.tv_sec, sub_time.tv_usec, slog_type_str[slog_type]);
    va_list ap;
    va_start(ap, fmt);
    //写用户子日志信息
    vfprintf(slog_ctrl.fp, fmt, ap);
    va_end(ap);
    slog_ctrl.current_line++;
    fflush(slog_ctrl.fp);
}
