#include "Hooks.h"

#include <unordered_map>

#include "Core/Core.hpp"
#include "Core/ModuleCore.h"
#include "AudioCapture.h"

std::unordered_map<void*, SHookInfo> HookCache;

SHookInfo* GetHookInfo(void* hookedAddress)
{
	auto entry = HookCache.find(hookedAddress);
	if (entry == HookCache.end())
		return nullptr;
	return &(*entry).second;
}

static SHookInfo* AddToHookCache(SHookInfo&& hookInfo)
{
	HookCache.emplace(hookInfo.HookedProcAddress, std::move(hookInfo));
	return &HookCache.at(hookInfo.HookedProcAddress);
}

void CreateHooks(MODULEENTRY32 moduleEntry)
{
	// Get function addresses
	void* ptrGetBufferAddress     = moduleEntry.modBaseAddr + OffsetGetBuffer;
	void* ptrReleaseBufferAddress = moduleEntry.modBaseAddr + OffsetReleaseBuffer;

	// Set trampoline target address to empty bytes at the end of the module
	BYTE* trampolineAddress = moduleEntry.modBaseAddr + moduleEntry.modBaseSize - 200;

	// Hook!
	SHookInfo* pHookInfo;

#ifdef PLATFORM32

	bResult = Hook(ptrGetBufferAddress, HookGetBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!GetBuffer.");
	bResult = Hook(ptrReleaseBufferAddress, HookReleaseBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!ReleaseBuffer.");

#elif defined PLATFORM64

	pHookInfo = Hook(ptrGetBufferAddress, *HookGetBuffer, 5, trampolineAddress);
	KILL_ON_NULL_MB(pHookInfo, "Cannot hook GetBuffer!", "Hook error!");
	OriginalGetBuffer = (HRESULT(*)(IAudioRenderClient*, UINT32, BYTE**))pHookInfo->TrampolineAddress;
	trampolineAddress += 5 + JumpLength;

	pHookInfo = Hook(ptrReleaseBufferAddress, *HookReleaseBuffer, 5, trampolineAddress);
	KILL_ON_NULL_MB(pHookInfo, "Cannot hook ReleaseBuffer!", "Hook error!");
	OriginalReleaseBuffer = (HRESULT(*)(IAudioRenderClient*, UINT32, DWORD))pHookInfo->TrampolineAddress;
	trampolineAddress += 5 + JumpLength;

#endif
}

SHookInfo* Hook(void* hookedAddress, void* hookFuncAddress, size_t length, void* trampolineAddress)
{
	// Hook cannot be shorter than 5 bytes
	if (length < JumpLength)
		return nullptr;

	DWORD oldProtection;
	DWORD jumpAddress;

	// Add to hook cache
	SHookInfo* pHookInfo = AddToHookCache(std::move(SHookInfo {
		hookedAddress,
		hookFuncAddress,
		trampolineAddress,
		length,
	}));

	// Place trampoline

	if (!VirtualProtect(trampolineAddress, length + JumpLength, PAGE_EXECUTE_READWRITE, &oldProtection))
		return nullptr;

	PlaceTrampoline(hookedAddress, trampolineAddress, length);

	if (!VirtualProtect(trampolineAddress, length + JumpLength, PAGE_EXECUTE_READ, &oldProtection))
		return nullptr;

	// Insert jump

	if (!VirtualProtect(hookedAddress, length, PAGE_EXECUTE_READWRITE, &oldProtection))
		return nullptr;

	// Set everything to NOP first in case the hook is longer than 5
	memset(hookedAddress, OP_NOP, length);

	jumpAddress = (DWORD)((BYTE*)hookFuncAddress - ((BYTE*)hookedAddress + JumpLength));

	*(BYTE*)hookedAddress = OP_JMP;
	*(DWORD*)((BYTE*)hookedAddress + 1) = jumpAddress;

	if (!VirtualProtect(hookedAddress, length, oldProtection, &oldProtection))
		return nullptr;

	return pHookInfo;
}

bool Unhook(void* hookedAddress)
{
	SHookInfo* pHookInfo = GetHookInfo(hookedAddress);
	if (!pHookInfo)
		return false;
	SHookInfo& hookInfo = *pHookInfo;

	RemoveTrampoline(hookInfo);

	return true;
}

void PlaceTrampoline(void* hookedAddress, void* targetAddress, size_t hookLength)
{
	// Calculate the relative jump address
	DWORD jumpAddress = (DWORD)(((BYTE*)hookedAddress + hookLength) - ((BYTE*)targetAddress + hookLength + JumpLength));

	// Copy bytes from original instruction to the trampoline address
	memcpy(targetAddress, hookedAddress, hookLength);
	*((BYTE*)targetAddress + hookLength) = OP_JMP;
	*(DWORD*)((BYTE*)targetAddress + hookLength + 1) = jumpAddress;
}

bool RemoveTrampoline(SHookInfo& hookInfo)
{
	
	VirtualFree(hookInfo.TrampolineAddress, 0, MEM_RELEASE);

	
	return true;
}

// ---------------------------
//  Data
// ---------------------------

BYTE** InterceptedBuffer = nullptr;

// ---------------------------
//  Hooks
// ---------------------------

HRESULT(*OriginalGetBuffer)(IAudioRenderClient*, UINT32, BYTE**);
HRESULT(*OriginalReleaseBuffer)(IAudioRenderClient*, UINT32, DWORD);

HRESULT __stdcall HookGetBuffer(IAudioRenderClient* pIARC, UINT32 numFramesRequested, BYTE** ppData)
{
	InterceptedBuffer = ppData;
	return OriginalGetBuffer(pIARC, numFramesRequested, ppData);
}

HRESULT __stdcall HookReleaseBuffer(IAudioRenderClient* pIARC, UINT32 numFramesWritten, DWORD dwFlags)
{
	return OriginalReleaseBuffer(pIARC, numFramesWritten, dwFlags);
}

#ifdef PLATFORM32

__declspec(naked) void nHookGetBuffer()
{

}

__declspec(naked) void nHookReleaseBuffer()
{

}

#endif
