#ifndef X_LOG_H
#define X_LOG_H

#define LF_DEBUG 0x1
#define LF_INFO  0x2
#define LF_WARN  0x4
#define LF_ERROR 0x8
#define LF_FATAL 0x16

#define debugf(...) log_printf(LF_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define infof(...) log_printf(LF_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define warnf(...) log_printf(LF_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define errorf(...) log_printf(LF_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define fatalf(...) log_printf(LF_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_printf(int, const char*, int, const char*, ...);
void log_init(int);

#endif