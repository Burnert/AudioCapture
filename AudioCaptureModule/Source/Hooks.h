#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#define JumpLength      5
#define Jump64Length    9

//#define OffsetGetBuffer           0x20480
#define HookLengthGetBuffer       5
//#define OffsetReleaseBuffer       0x20730
#define HookLengthReleaseBuffer   5

struct ModuleInfo
{
	BYTE* BaseAddress;
	DWORD Size;
};

namespace Hooks
{
	struct HookInfo
	{
		void* HookedProcAddress;
		void* HookFunctionAddress;
		void* TrampolineAddress;
		size_t Length;
	};

	void ReadOffsets();

	/* Retrieves HookInfo for the specified hook. Returns nullptr if it doesn't exist */
	HookInfo* GetHookInfo(void* hookedAddress);

	void CreateHooks(ModuleInfo moduleInfo);
	void RemoveHooks();

	/* Create a hook at the specified address. */
	HookInfo* Hook(void* hookedAddress, void* hookFuncAddress, size_t length, void* trampolineAddress);
	/* Remove the hook at the specified address. */
	bool Unhook(void* hookedAddress);

	/* hookLength: length of stolen bytes, without the jump instruction */
	bool PlaceTrampoline(void* hookedAddress, void* targetAddress, size_t hookLength);
	bool RemoveTrampoline(HookInfo* pHookInfo);

	// Data:

	extern BYTE** Buffer;
	extern UINT32 FramesRequested;

	// Hooks:

	class IAudioRenderClient; // Fwd declare here instead of including the whole header

	// In these functions the first parameter is a pointer to the interface because it's a member function (__thiscall convention)

	HRESULT __stdcall HookGetBuffer(IAudioRenderClient* pIARC, UINT32 numFramesRequested, BYTE** ppData);
	HRESULT __stdcall HookReleaseBuffer(IAudioRenderClient* pIARC, UINT32 numFramesWritten, DWORD dwFlags);

	// These are actually trampoline pointers, not the original function pointers.

	using FnPtrGetBuffer     = HRESULT(__stdcall *)(IAudioRenderClient*, UINT32, BYTE**);
	using FnPtrReleaseBuffer = HRESULT(__stdcall *)(IAudioRenderClient*, UINT32, DWORD);

	extern FnPtrGetBuffer     OriginalGetBuffer;
	extern FnPtrReleaseBuffer OriginalReleaseBuffer;
}

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
