#pragma once

#include <Windows.h>
#include "Core/ModuleCore.h"

/* Handle to this module */
extern HMODULE g_hModule;

/* Unloads the module from the process. */
static inline void _Kill()
{
	FreeLibraryAndExitThread(g_hModule, 0);
}

/* Used to kill the module from a different process.
   Don't call this anywhere else than the module unload function. */
extern "C" ACM_API void Kill();

// Non-blocking MsgBox message types:

#define MSG_CONNECTED_TO_SERVER   1

void MsgBoxNonBlocking(unsigned int type);

namespace AudioCapture
{
	void InitModuleInjection();
	void Cleanup();
}

namespace IPC
{
	extern HANDLE DataPipeHandle;

	bool ConnectToServer();
	bool DisconnectFromServer();

	bool SendData(BYTE* data, unsigned long count);
}
