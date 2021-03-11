#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#define OffsetGetBuffer        0x20480
//#define OffsetGetBuffer32      0x20480
#define OffsetReleaseBuffer    0x20730
//#define OffsetReleaseBuffer32  0x20730

#define HookPointGetBuffer         0x0
//#define HookPointGetBuffer32       0x0
#define HookPointReleaseBuffer     0x0
//#define HookPointReleaseBuffer32   0x0

void CreateHooks(MODULEENTRY32 moduleEntry);

/* Create a hook at the specified address. */
bool Hook(void* hookedAddress, void* hookFuncAddress, int length);
/* Remove the hook at the specified address. */
bool Unhook(void* hookedAddress);

#ifdef PLATFORM64

// -------------------------
//        ASM Hooks
// -------------------------

extern "C"
{
	void asmhkGetBuffer();
	void asmhkReleaseBuffer();
}

#endif
