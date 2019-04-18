#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"

static struct 
{
	FILE* fp;
	
} LOG;

void log_set_file(FILE* fp)
{
	LOG.fp = fp;
}

void log_log(const char* file, int line, const char* fmt, ...)
{
	/*Get current time*/
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	if(LOG.fp)
	{
		va_list args;
		char buf[32];
		buf[strftime(buf, sizeof(buf),"%Y-%m-%d %H:%M:%S", lt)] = '\0';
		fprintf(LOG.fp, "%s %s:%d: ", buf,file, line);
		va_start(args, fmt);
		vfprintf(LOG.fp, fmt, args);
		va_end(args);
		fprintf(LOG.fp, "\n");
		fflush(LOG.fp);
	}
	return;
}