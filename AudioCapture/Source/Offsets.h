#pragma once

#include <Windows.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#define _VERIFY(hr) if (FAILED((hr))) { Cleanup(); return false; }

namespace Offsets
{
	using FnPtrGetBuffer     = HRESULT(__stdcall*)(IAudioRenderClient*, UINT32, BYTE**);
	using FnPtrReleaseBuffer = HRESULT(__stdcall*)(IAudioRenderClient*, UINT32, DWORD);

#define FAIL_CLOSE(x) if (!(x)) { CloseHandle(hFile); return false; }
	bool WriteOffsetsFile(DWORD getBufferOffset, DWORD releaseBufferOffset)
	{
		GET_OFFSET_FILE_PATH(tempPath);

		HANDLE hFile = CreateFile(tempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		DWORD dwBytesWritten;
		FAIL_CLOSE(WriteFile(hFile, (void*)&getBufferOffset, sizeof(DWORD), &dwBytesWritten, NULL));
		FAIL_CLOSE(WriteFile(hFile, (void*)&releaseBufferOffset, sizeof(DWORD), &dwBytesWritten, NULL));

		CloseHandle(hFile);
		return true;
	}

	void DeleteOffsetsFile()
	{
		GET_OFFSET_FILE_PATH(tempPath);

		if (!DeleteFile(tempPath))
		{
			printf_s("Could not delete the offsets file!");
		}
	}

	bool LoadAudioSesOffsets(DWORD* outGetBuffer, DWORD* outReleaseBuffer)
	{
		HRESULT hr;
		REFERENCE_TIME hnsRequestedDuration = 10000000;
		IMMDeviceEnumerator* pEnumerator = NULL;
		IMMDevice* pDevice = NULL;
		IAudioClient* pAudioClient = NULL;
		IAudioRenderClient* pRenderClient = NULL;
		WAVEFORMATEX* pwfx = NULL;
		UINT32 bufferFrameCount;
		BYTE* pData;
		DWORD flags = 0;

		auto Cleanup = [&]
		{
			CoTaskMemFree(pwfx);
			COM_RELEASE(pEnumerator);
			COM_RELEASE(pDevice);
			COM_RELEASE(pAudioClient);
			COM_RELEASE(pRenderClient);
			CoUninitialize();
		};

		// Initialize everything first, to hack Windows

		if (FAILED(CoInitialize(nullptr)))
			return false;

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pEnumerator));
		_VERIFY(hr);

		hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
		_VERIFY(hr);

		hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
		_VERIFY(hr);

		hr = pAudioClient->GetMixFormat(&pwfx);
		_VERIFY(hr);

		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, pwfx, NULL);
		_VERIFY(hr);

		// Get the actual size of the allocated buffer.
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
		_VERIFY(hr);

		hr = pAudioClient->GetService(IID_PPV_ARGS(&pRenderClient));
		_VERIFY(hr);

		// Get the CAudioRenderClient vtable
		size_t* vtable = (size_t*)((size_t*)pRenderClient)[0];
		// Get the function pointers from the vtable
		FnPtrGetBuffer     fnAudioSesGetBuffer     = (FnPtrGetBuffer)    vtable[3];
		FnPtrReleaseBuffer fnAudioSesReleaseBuffer = (FnPtrReleaseBuffer)vtable[4];

		// Find Audio Session module
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			Cleanup();
			return false;
		}
		MODULEENTRY32 moduleEntry;
		if (!FindModule(snapshot, "AudioSes.dll", &moduleEntry))
		{
			Cleanup();
			return false;
		}
		CloseHandle(snapshot);

		Cleanup();

		*outGetBuffer     = (char*)fnAudioSesGetBuffer     - (char*)moduleEntry.modBaseAddr;
		*outReleaseBuffer = (char*)fnAudioSesReleaseBuffer - (char*)moduleEntry.modBaseAddr;

		return true;
	}
}
