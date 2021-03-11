#include "Hooks.h"

#include "Core/Core.hpp"
#include "Core/ModuleCore.h"

void CreateHooks(MODULEENTRY32 moduleEntry)
{
	// Get function addresses

	void* ptrGetBufferAddress     = moduleEntry.modBaseAddr + OffsetGetBuffer     + HookPointGetBuffer;
	void* ptrReleaseBufferAddress = moduleEntry.modBaseAddr + OffsetReleaseBuffer + HookPointReleaseBuffer;

	// Hook

#ifdef PLATFORM32
	bResult = Hook(ptrGetBufferAddress, HookGetBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!GetBuffer.");
	bResult = Hook(ptrReleaseBufferAddress, HookReleaseBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!ReleaseBuffer.");
#endif
	
	//FindModule(hSnapshot, "vlc.exe", &moduleEntry);
	//char message[500];
	//sprintf(message, "&Kill: %16llx", ((DWORD64)Kill - (DWORD64)moduleEntry.modBaseAddr));
	//MessageBoxA(NULL, message, "", MB_OK);
}

bool Hook(void* hookedAddress, void* hookFuncAddress, int length)
{
	// Hook cannot be shorter than 5 bytes
	if (length < 5)
		return false;

	DWORD oldProtection;
	DWORD jumpAddress;

	if (!VirtualProtect(hookedAddress, length, PAGE_EXECUTE_READWRITE, &oldProtection))
		return false;

	memset(hookedAddress, OP_NOP, length);

	jumpAddress = (DWORD)hookFuncAddress - (DWORD)hookedAddress;

	*(BYTE*)hookedAddress = OP_JMP;
	*(DWORD*)((BYTE*)hookedAddress + 1) = jumpAddress;

	if (!VirtualProtect(hookedAddress, length, oldProtection, &oldProtection))
		return false;

	return true;
}

bool Unhook(void* hookedAddress)
{
	return false;
}

namespace AudioSes
{
	HRESULT GetBuffer(unsigned int numFramesRequested, unsigned char** ppData)
	{
		return 0;
	}

	HRESULT ReleaseBuffer(unsigned int numFramesWritten, unsigned long dwFlags)
	{
		return 0;
	}
}


#ifdef PLATFORM32

// -----------------------
//     Hooks
// -----------------------

__declspec(naked) void HookGetBuffer()
{

}

__declspec(naked) void HookReleaseBuffer()
{

}

#endif
