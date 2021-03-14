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
		KILL_ON_FALSE_MB(bResult, "Cannot link to " EXECUTABLE_FILE_NAME "!", "Injection error!");
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
		KILL_ON_FALSE_MB(bResult, "Cannot find module " MODULE_AUDIOSES_NAME ".", "Injection error!");
		CloseHandle(hSnapshot);

		//MessageBoxA(NULL, "Module " AUDIOCAPTUREDLL_NAME " Injected Successfully.", "Info", MB_ICONINFORMATION | MB_OK);
		printf_s("Module " AUDIOCAPTUREDLL_NAME " Injected Successfully!\n");
		printf_s("Creating hooks...\n");

		CreateHooks(SModuleInfo { moduleEntry.modBaseAddr, moduleEntry.modBaseSize });
	}

	void Cleanup()
	{
		RemoveHooks();
	}
}
