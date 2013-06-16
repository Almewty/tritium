#include "stdafx.h"
#include "engine.hpp"

// Mutex
pthread_mutex_t mutex_msg;

// Log folder
char logfolder[500];
char allowed_name_chars[500];
int time_diff;

// Common console msg
void dbg_msg(int color, const char* sys, const char* fmt, ...)
{
	pthread_mutex_lock(&mutex_msg);

	// Logfile
	char logpath[500];
	sprintf_s(logpath, sizeof(logpath), "%s/console.log", logfolder);
	FILE* logfile;
	if(fopen_s(&logfile, logpath, "a") != 0)
	{
		printf("Couldn't open logfile '%s'!\n", logpath);
		Sleep(2000);
		exit(0);
	}

	// Clock
	SYSTEMTIME time;
	GetSystemTime(&time);

	// Colors
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// Date
	fprintf(logfile, "[%02d/%02d/%04d ",
				   	 time.wDay,
					 time.wMonth,
					 time.wYear);

	// Print the color prefix
	//SetConsoleTextAttribute(hConsole, 0x06);
	printf("[%02d:%02d:%02d] | ",
		   ((time.wHour+time_diff) % 24),
		   time.wMinute,
		   time.wSecond);
	fprintf(logfile, "%02d:%02d:%02d] | ",
					 ((time.wHour+1) % 24),
					 time.wMinute,
					 time.wSecond);

	// Buffer for str
	char str[1024*4];
	
	// Write sys into str
	sprintf_s(str, "<%s>", sys);
	sprintf_s(str, "%-13s", str);

	// Print the color prefix
	SetConsoleTextAttribute(hConsole, color);
	printf("%s", str);
	fprintf(logfile, "%s", str);
	SetConsoleTextAttribute(hConsole, 15);

	// Empty str
	sprintf_s(str, "");

	// Get length of str
	int len = strlen(str);

	// Write the str into the msg
	char *msg;
	msg = (char *)str + len;
	
	// Argument list
	va_list args;
	// Write into it
	va_start(args, fmt);
	// Print them into the msg
	vsnprintf_s(msg, sizeof(str) - len, sizeof(str) - len, fmt, args);
	// End the usage of the args
	va_end(args);

	// Print the actual string
	printf("| %s\n", str);
	fprintf(logfile, "| %s\n", str);

	fclose(logfile);

	pthread_mutex_unlock(&mutex_msg);
}

bool str_cmp(char* str1, char* str2, int count)
{
	bool matching = true;

	for(int i = 0; i < count; i++)
		if(str1[i] != str2[i])
			matching = false;

	return matching;
}

bool valid_name(char* str, unsigned int len)
{
	unsigned int _len = strlen(allowed_name_chars);

	bool proper_name;

	for(unsigned int i = 0; i < len; i++)
	{
		proper_name = false;
		for(unsigned int j = 0; j < _len; j++)
		{
			if(str[i] == allowed_name_chars[j])
				proper_name = true;
		}

		if(proper_name == false)
			return false;
	}

	return true;
}

HANDLE proc_exists(char* name)
{
	PROCESSENTRY32 entry;
    entry.dwFlags = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if(Process32First(snapshot, &entry) == TRUE)
	{
    	do
		{
    		if(str_cmp(entry.szExeFile, name, strlen(name)) == 0)
			{
				CloseHandle(snapshot);
    			return OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
			}
		}
		while(Process32Next(snapshot, &entry) == TRUE);
	}

    CloseHandle(snapshot);

	return NULL;
}
