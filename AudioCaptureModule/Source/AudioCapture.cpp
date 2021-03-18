#include "AudioCapture.h"

#include <cstdio>
#include <TlHelp32.h>

#include "Core/Core.hpp"
#include "Hooks.h"

// Exported Kill function
extern "C" void Kill()
{
	_Kill();
}

namespace AudioCapture
{
	void InitModuleInjection()
	{
		bool bResult;
		HANDLE hSnapshot;
		
		// Link the module to AudioCapture.exe

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 processEntry;
		bResult = FindProcess(hSnapshot, EXECUTABLE_FILE_NAME, &processEntry);
		KILL_ON_FALSE_MB(bResult, "Cannot link to " EXECUTABLE_FILE_NAME "!", "Injection error");
		CloseHandle(hSnapshot);
#if 0
		bResult = AttachConsole(processEntry.th32ProcessID);
		if (!bResult)
		{
			DWORD error = GetLastError();
			std::string errorMessage = FormatErrorMessage(error);
			char messageBuffer[500];
			sprintf_s(messageBuffer, 500, "Cannot attach console.\n%s", errorMessage.c_str());
			MessageBoxA(NULL, messageBuffer, "Injection error!", MB_ICONEXCLAMATION | MB_OK);
			_Kill();
		}
#endif
		printf_s("Using console output of [%d].\n", GetCurrentProcessId());

		// Find the AudioSes.dll module in the target process
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		MODULEENTRY32 moduleEntry;
		bResult = FindModule(hSnapshot, MODULE_AUDIOSES_NAME, &moduleEntry);
		KILL_ON_FALSE_MB(bResult, "Cannot find module " MODULE_AUDIOSES_NAME ".", "Injection error");
		CloseHandle(hSnapshot);

		printf_s("Module " AUDIOCAPTUREDLL_NAME " Injected Successfully!\n");

		printf_s("Creating hooks...\n");
		CreateHooks(SModuleInfo { moduleEntry.modBaseAddr, moduleEntry.modBaseSize });

		bResult = IPC::ConnectToServer();
		KILL_ON_FALSE_MB(bResult, "Cannot connect to server pipe!", "Connection error");
		MessageBoxA(NULL, "Connected to server.", "Connection", MB_ICONINFORMATION | MB_OK);
	}

	void Cleanup()
	{
		IPC::DisconnectFromServer();
		RemoveHooks();
	}
}

// --------------------------
//  IPC
// --------------------------

namespace IPC
{
	HANDLE DataPipeHandle = INVALID_HANDLE_VALUE;

	bool ConnectToServer()
	{
		BOOL bResult;

		DWORD pid = GetCurrentProcessId();
		char szPipeName[50];
		FillPipeName(szPipeName, pid);

		bResult = WaitNamedPipeA(szPipeName, 5000);
		if (!bResult)
			return false;

		// Connect to pipe
		DataPipeHandle = CreateFileA(szPipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		return DataPipeHandle != INVALID_HANDLE_VALUE;
	}

	bool DisconnectFromServer()
	{
		return CloseHandle(DataPipeHandle);
	}
}
