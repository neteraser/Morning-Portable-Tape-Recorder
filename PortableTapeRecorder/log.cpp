
#include "log.h"

#include <string>
#include <stdio.h>
#include <cstdarg>

FILE* logfile;

extern const char* getAppFileName(const char* filename);

void InitLog(const char* logfilename)
{
	logfile = fopen(getAppFileName(logfilename), "w+t");
	if (!logfile)
		return;
}

void WriteToLog(const char* logstring, ...)
{
	va_list list;
	va_start(list, logstring);

	char buf[1024];
	vsprintf(buf, logstring, list);

	std::string logstrstr = buf;
	logstrstr += "\n";

	fwrite(logstrstr.c_str(), logstrstr.size(), 1, logfile);

	fflush(logfile);

	va_end(list);
}

void CloseLog()
{
	fflush(logfile);
	fclose(logfile);
}
