#include "Hooks.h"

#include <cstdio>
#include <unordered_map>

#include "Core/Core.hpp"
#include "Core/ModuleCore.h"
#include "AudioCapture.h"

// ---------------------
//  HookCache
// ---------------------

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

static bool RemoveFromHookCache(void* hookedAddress)
{
	return (bool)HookCache.erase(hookedAddress);
}

// ------------------------------
//  Hooking procedures
// ------------------------------

void CreateHooks(SModuleInfo moduleInfo)
{
	// Get function addresses
	void* ptrGetBufferAddress     = moduleInfo.BaseAddress + OffsetGetBuffer;
	void* ptrReleaseBufferAddress = moduleInfo.BaseAddress + OffsetReleaseBuffer;

	// Set trampoline target address to empty bytes at the end of the module
	BYTE* trampolineAddress = moduleInfo.BaseAddress + moduleInfo.Size - 50;

	// Hook!
	SHookInfo* pHookInfo;

#ifdef PLATFORM32

	bResult = Hook(ptrGetBufferAddress, HookGetBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!GetBuffer.");
	bResult = Hook(ptrReleaseBufferAddress, HookReleaseBuffer, 5);
	_RET0_ON_FALSE_MB(bResult, "Cannot hook to function AudioSes.dll!ReleaseBuffer.");

#elif defined PLATFORM64

	trampolineAddress -= (HookLengthGetBuffer + JumpLength);
	pHookInfo = Hook(ptrGetBufferAddress, HookGetBuffer, HookLengthGetBuffer, trampolineAddress);
	KILL_ON_NULL_MB(pHookInfo, "Cannot hook GetBuffer!", "Hook error");
	OriginalGetBuffer = (HRESULT(*)(IAudioRenderClient*, UINT32, BYTE**))pHookInfo->TrampolineAddress;
	printf_s("GetBuffer function hooked.");

	trampolineAddress -= (HookLengthReleaseBuffer + JumpLength);
	pHookInfo = Hook(ptrReleaseBufferAddress, HookReleaseBuffer, HookLengthReleaseBuffer, trampolineAddress);
	KILL_ON_NULL_MB(pHookInfo, "Cannot hook ReleaseBuffer!", "Hook error");
	OriginalReleaseBuffer = (HRESULT(*)(IAudioRenderClient*, UINT32, DWORD))pHookInfo->TrampolineAddress;
	printf_s("ReleaseBuffer function hooked.");

#endif
}

void RemoveHooks()
{
	while (!HookCache.empty())
	{
		SHookInfo* currentHook = &(*HookCache.begin()).second;
		if (!Unhook(currentHook->HookedProcAddress))
		{
			char message[100];
			sprintf_s(message, "Could not remove hook at %p!", currentHook->HookedProcAddress);
			MessageBoxA(NULL, message, "Hook error", MB_ICONERROR | MB_OK);
		}
	}
}

SHookInfo* Hook(void* hookedAddress, void* hookFuncAddress, size_t length, void* trampolineAddress)
{
	// Hook cannot be shorter than 5 bytes
	if (length < JumpLength)
		return nullptr;

	DWORD oldProtection;
	DWORD jumpAddress;

	// Add to hook cache
	SHookInfo* pHookInfo = AddToHookCache(SHookInfo {
		hookedAddress,
		hookFuncAddress,
		trampolineAddress,
		length,
	});

	// Insert trampoline (at the end of the module)
	if (!PlaceTrampoline(hookedAddress, trampolineAddress, length))
		return nullptr;

	// Insert jump

	if (!VirtualProtect(hookedAddress, length, PAGE_EXECUTE_READWRITE, &oldProtection))
		return nullptr;

	// Set everything to NOP first in case the hook is longer than 5
	memset(hookedAddress, OP_NOP, length);

	// Calculate the relative jump address to the hook and insert the instruction
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

	DWORD oldProtection;

	if (!VirtualProtect(pHookInfo->HookedProcAddress, pHookInfo->Length, PAGE_EXECUTE_READWRITE, &oldProtection))
		return false;

	// Copy bytes from the trampoline back to the hooked function
	memcpy(pHookInfo->HookedProcAddress, pHookInfo->TrampolineAddress, pHookInfo->Length);

	if (!VirtualProtect(pHookInfo->HookedProcAddress, pHookInfo->Length, oldProtection, &oldProtection))
		return false;

	if (!RemoveTrampoline(pHookInfo))
		return false;

	if (!RemoveFromHookCache(hookedAddress))
		return false;

	return true;
}

bool PlaceTrampoline(void* hookedAddress, void* targetAddress, size_t hookLength)
{
	DWORD oldProtection;
	if (!VirtualProtect(targetAddress, hookLength + JumpLength, PAGE_EXECUTE_READWRITE, &oldProtection))
		return false;

	// Copy bytes from original instruction to the trampoline address
	memcpy(targetAddress, hookedAddress, hookLength);

	// Calculate the relative jump address and insert the instruction
	DWORD jumpAddress = (DWORD)(((BYTE*)hookedAddress + hookLength) - ((BYTE*)targetAddress + hookLength + JumpLength));
	*((BYTE*)targetAddress + hookLength) = OP_JMP;
	*(DWORD*)((BYTE*)targetAddress + hookLength + 1) = jumpAddress;

	if (!VirtualProtect(targetAddress, hookLength + JumpLength, PAGE_EXECUTE_READ, &oldProtection))
		return false;
}

bool RemoveTrampoline(SHookInfo* pHookInfo)
{
	DWORD oldProtection;
	if (!VirtualProtect(pHookInfo->TrampolineAddress, pHookInfo->Length + JumpLength, PAGE_EXECUTE_READWRITE, &oldProtection))
		return false;

	// Secure so it definitely gets zeroed
	SecureZeroMemory(pHookInfo->TrampolineAddress, pHookInfo->Length + JumpLength);

	if (!VirtualProtect(pHookInfo->TrampolineAddress, pHookInfo->Length + JumpLength, PAGE_READONLY, &oldProtection))
		return false;

	return true;
}

// ---------------------------
//  Data
// ---------------------------

namespace HookedData
{
	BYTE** Buffer = nullptr;
	UINT32 FramesRequested = 0;
}

// ---------------------------
//  Hooks
// ---------------------------

HRESULT(*OriginalGetBuffer)(IAudioRenderClient*, UINT32, BYTE**);
HRESULT(*OriginalReleaseBuffer)(IAudioRenderClient*, UINT32, DWORD);

HRESULT __stdcall HookGetBuffer(IAudioRenderClient* pIARC, UINT32 numFramesRequested, BYTE** ppData)
{
	// Steal the data
	HookedData::Buffer = ppData;
	HookedData::FramesRequested = numFramesRequested;

	return OriginalGetBuffer(pIARC, numFramesRequested, ppData);
}

HRESULT __stdcall HookReleaseBuffer(IAudioRenderClient* pIARC, UINT32 numFramesWritten, DWORD dwFlags)
{
	// The buffer needs not to be copied until this function is called 
	// because it can be written after GetBuffer finishes executing.

	// @TODO: Send the buffer to the main process here.

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
