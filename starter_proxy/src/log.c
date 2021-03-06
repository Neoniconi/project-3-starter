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

void log_log(const char* fmt, ...)
{
	/*Get current time*/


	if(LOG.fp)
	{
		va_list args;
		fprintf(LOG.fp, "%u ", (unsigned)time(NULL));
		va_start(args, fmt);
		vfprintf(LOG.fp, fmt, args);
		va_end(args);
		fprintf(LOG.fp, "\n");
		fflush(LOG.fp);
	}
	return;
}