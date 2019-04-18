#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#define log_info(...) log_log(__VA_ARGS__)

void log_set_file(FILE *fp);
void log_log(const char *fmt, ...);

#endif

