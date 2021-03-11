#pragma once

#include <Windows.h>

/* Handle to this module */
extern HMODULE g_hModule;

/* Unloads the module from the process. */
static inline void _Kill()
{
	FreeLibraryAndExitThread(g_hModule, 0);
}

/* Used to kill the module from a different process.
   Don't call this anywhere else than the module unload function. */
extern "C" __declspec(dllexport) void Kill();

namespace AudioCapture
{
	void InitModuleInjection();
}
