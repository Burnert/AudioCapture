#pragma once

#include <string>
#include <Windows.h>
#include <TlHelp32.h>

#define EXECUTABLE_FILE_NAME   "AudioCapture.exe"
#define AUDIOCAPTUREDLL_NAME   "AudioCaptureModule.dll"
#define MODULE_AUDIOSES_NAME   "AudioSes.dll"

#define KILL_FUNC_OFFSET   0x110f5

// Op codes

#define OP_NOP  0x90
#define OP_JMP  0xE9

// Verify macros

#define EXIT_ON(x) \
if ((x)) { \
	exit(0); \
}
#define EXIT_ON_M(x, format, ...) \
if ((x)) { \
	printf_s(format, __VA_ARGS__); \
	exit(0); \
}

#define EXIT_ON_FALSE(x)                          EXIT_ON(!(x))
#define EXIT_ON_FALSE_M(x, format, ...)           EXIT_ON_M(!(x), format, __VA_ARGS__)
										          
#define EXIT_ON_NULL(x)                           EXIT_ON_FALSE(x)
#define EXIT_ON_NULL_M(x, format, ...)            EXIT_ON_FALSE_M(x, format, __VA_ARGS__)

#define EXIT_ON_INVALID_HANDLE(x)                 EXIT_ON((x) == INVALID_HANDLE_VALUE)
#define EXIT_ON_INVALID_HANDLE_M(x, format, ...)  EXIT_ON_M((x) == INVALID_HANDLE_VALUE, format, __VA_ARGS__)

#define EXIT_ON_NO_FILE(x) \
{ \
	DWORD attr = GetFileAttributes(x); \
	EXIT_ON(!(attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))); \
}
#define EXIT_ON_NO_FILE_M(x, format, ...) \
{ \
	DWORD attr = GetFileAttributes(x); \
	EXIT_ON_M(!(attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)), format, __VA_ARGS__); \
}

// Utilities

static std::string FormatErrorMessage(DWORD code)
{
	LPSTR messageBuffer;
	DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
	return std::string(messageBuffer, size);
}

static bool FindModule(HANDLE hSnapshot, const char* name, MODULEENTRY32* pOutModuleEntry)
{
	MODULEENTRY32 moduleEntry { };
	moduleEntry.dwSize = sizeof(moduleEntry);

	if (Module32First(hSnapshot, &moduleEntry))
	{
		do
		{
			if (_strcmpi(moduleEntry.szModule, name) == 0)
			{
				*pOutModuleEntry = moduleEntry;
				return true;
			}
		}
		while (Module32Next(hSnapshot, &moduleEntry));
	}

	return false;
}

static bool FindProcess(HANDLE hSnapshot, const char* name, PROCESSENTRY32* pOutProcessEntry)
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(processEntry);

	if (Process32First(hSnapshot, &processEntry))
	{
		while (Process32Next(hSnapshot, &processEntry))
		{
			if (_strcmpi(processEntry.szExeFile, name) == 0)
			{
				*pOutProcessEntry = processEntry;
				return true;
			}
		}
	}

	return false;
}

static bool FindProcess(HANDLE hSnapshot, unsigned int pid, PROCESSENTRY32* pOutProcessEntry)
{
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(processEntry);

	if (Process32First(hSnapshot, &processEntry))
	{
		while (Process32Next(hSnapshot, &processEntry))
		{
			if (processEntry.th32ProcessID == pid)
			{
				*pOutProcessEntry = processEntry;
				return true;
			}
		}
	}

	return false;
}
