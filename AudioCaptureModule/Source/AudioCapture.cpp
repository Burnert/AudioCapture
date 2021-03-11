#include "AudioCapture.h"

#include <TlHelp32.h>

#include "Core/Core.hpp"
#include "Core/ModuleCore.h"

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

		// Find the AudioSes.dll module in the target process

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		MODULEENTRY32 moduleEntry;
		bResult = FindModule(hSnapshot, MODULE_AUDIOSES_NAME, &moduleEntry);
		KILL_ON_FALSE_MB(bResult, "Cannot find module " MODULE_AUDIOSES_NAME ".", "Injection error!");
		CloseHandle(hSnapshot);

		MessageBoxA(NULL, "Module " AUDIOCAPTUREDLL_NAME " Injected Successfully.", "Info", MB_ICONINFORMATION | MB_OK);
	}
}
