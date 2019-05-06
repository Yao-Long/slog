#ifndef __SLOG_H__
#define __SLOG_H__

typedef enum{
    SLOG_TYPE_EMERG,      //system is unusable
    SLOG_TYPE_ALERT,      //action must be taken immediately
    SLOG_TYPE_CRIT,       //critical conditions
    SLOG_TYPE_ERR,        //error conditions
    SLOG_TYPE_WARNING,    //warning conditions
    SLOG_TYPE_NOTICE,     //normal, but significant, condition
    SLOG_TYPE_INFO,       //informational message
    SLOG_TYPE_DEBUG,      //debug-level message
}slog_type_t;


int init_slog(void);
void slog(slog_type_t, char *fmt, ...);


#endif
