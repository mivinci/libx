#ifndef X_LOG_H
#define X_LOG_H

#define LF_DEBUG (1<<0)
#define LF_INFO  (1<<1)
#define LF_WARN  (1<<2)
#define LF_ERROR (1<<3)
#define LF_FATAL (1<<4)

#define log_debug(...) log_printf(LF_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_printf(LF_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_printf(LF_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_printf(LF_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_panic(...) log_printf(LF_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_printf(int, const char*, int, const char*, ...);
void log_init(int);

#endif