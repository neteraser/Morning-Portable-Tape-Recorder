#pragma once

void InitLog(const char* logfilename);
void WriteToLog(const char* logstring, ...);
void CloseLog();