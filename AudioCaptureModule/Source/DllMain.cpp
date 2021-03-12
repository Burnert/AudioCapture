#include <Windows.h>

#include "Core/Core.hpp"
#include "Core/ModuleCore.h"
#include "AudioCapture.h"

/* Definition of handle to this module */
HMODULE g_hModule;

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) 
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hModule;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AudioCapture::InitModuleInjection, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		// @TODO: Add RemoveHooks function
		break;
	}

	return TRUE;
}
