#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#define JumpLength      5
#define Jump64Length    9

#define OffsetGetBuffer        0x20480
//#define OffsetGetBuffer32      0x20480
#define OffsetReleaseBuffer    0x20730
//#define OffsetReleaseBuffer32  0x20730

struct SHookInfo
{
	void* HookedProcAddress;
	void* HookFunctionAddress;
	void* TrampolineAddress;
	size_t Length;
};

/* Retrieves SHookInfo for the specified hook. Returns nullptr if it doesn't exist */
SHookInfo* GetHookInfo(void* hookedAddress);

void CreateHooks(MODULEENTRY32 moduleEntry);

/* Create a hook at the specified address. */
SHookInfo* Hook(void* hookedAddress, void* hookFuncAddress, size_t length, void* trampolineAddress);
/* Remove the hook at the specified address. */
bool Unhook(void* hookedAddress);

/* hookLength: length of stolen bytes, without the jump instruction */
void PlaceTrampoline(void* hookedAddress, void* targetAddress, size_t hookLength);
bool RemoveTrampoline(SHookInfo& hookInfo);

// ---------------------------
//  Data
// ---------------------------

extern BYTE** InterceptedBuffer;

// ---------------------------
//  Hooks
// ---------------------------

class IAudioRenderClient; // Fwd declare here instead of including the whole header

HRESULT __stdcall HookGetBuffer(IAudioRenderClient* pIARC, UINT32 numFramesRequested, BYTE** ppData);
HRESULT __stdcall HookReleaseBuffer(IAudioRenderClient* pIARC, UINT32 numFramesWritten, DWORD dwFlags);

// These are actually trampoline pointers, not the original function pointers.

extern HRESULT(*OriginalGetBuffer)(IAudioRenderClient*, UINT32, BYTE**);
extern HRESULT(*OriginalReleaseBuffer)(IAudioRenderClient*, UINT32, DWORD);


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
