#include "IPC.h"

#include <unordered_map>
#include "Core/Core.hpp"
#include "AudioBufferParser.h"

namespace IPC
{
	BYTE* BufferData = nullptr;

	std::unordered_map<DWORD, HANDLE> PipeMap;

	bool CreateByteBufferPipe(DWORD targetPid)
	{
		if (targetPid == 0)
			return false;

		BOOL bResult;

		// The pipe name will look like: "audiocapture-12345" where 12345 is an example PID.
		char szPipeName[50] = "";
		FillPipeName(szPipeName, targetPid);

		// @TODO: Implement a proper security shit here because the pipe is unprotected.
		// This is really complicated for some reason...
		SECURITY_DESCRIPTOR sd { };
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			return false;

		if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
			return false;

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = 0;
		sa.lpSecurityDescriptor = &sd;

		HANDLE hPipe = CreateNamedPipeA(szPipeName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE, 1, 0, 0, 0, &sa);
		if (hPipe == INVALID_HANDLE_VALUE)
			return false;

		PipeMap.insert({ targetPid, hPipe });
		return true;
	}

	bool DeleteByteBufferPipe(DWORD targetPid)
	{
		auto iter = PipeMap.find(targetPid);
		if (iter == PipeMap.end())
			return false;
		HANDLE hPipe = (*iter).second;

		CloseHandle(hPipe);
		return true;
	}

	bool ReadByteBufferPipe(DWORD targetPid, DWORD count)
	{
		BOOL bResult;
		auto iter = PipeMap.find(targetPid);
		if (iter == PipeMap.end())
			return false;
		HANDLE hPipe = (*iter).second;
		
		DWORD bytesRead;
		bResult = ReadFile(hPipe, BufferData, count, &bytesRead, NULL);

		SVolume volume = GetBufferVolume32(BufferData, count / 8, 8);
		printf_s("Buffer: (size: %d) [volume: %.2f, max: %.2f] ", count, volume.Avg, volume.Max);
		char volumeMeter[51];
		ZeroMemory(volumeMeter, 51);

		char* volumeMeterPtr = volumeMeter;
		for (int i = 0; i < 50; ++i)
		{
			float iNorm = i / 50.0f;
			*volumeMeterPtr = ((iNorm <= volume.Avg) ? '#' : '-');
			if (volume.Max > iNorm && volume.Max < (i + 1) / 50.0f || 
				i == 49 && volume.Max > 1.0f)
				*volumeMeterPtr = '|';
			volumeMeterPtr++;
		}

		printf_s("%s\n", volumeMeter);

		return bResult;
	}

	DWORD PeekByteBufferPipeSize(DWORD targetPid)
	{
		BOOL bResult;
		auto iter = PipeMap.find(targetPid);
		if (iter == PipeMap.end())
			return 0;
		HANDLE hPipe = (*iter).second;

		DWORD bytesAvailable;
		bResult = PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesAvailable, NULL);
		if (!bResult)
			return 0;

		return bytesAvailable;
	}

	bool AllocByteBuffer(DWORD size)
	{
		BufferData = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
		return BufferData;
	}

	bool FreeByteBuffer()
	{
		return HeapFree(GetProcessHeap(), 0, IPC::BufferData);
	}

	bool WaitForConnection(DWORD targetPid)
	{
		BOOL bResult;
		auto iter = PipeMap.find(targetPid);
		if (iter == PipeMap.end())
			return false;
		HANDLE hPipe = (*iter).second;

		bResult = ConnectNamedPipe(hPipe, NULL);
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_PIPE_CONNECTED)
			{
				printf_s("Pipe already connected to [%d]!\n", targetPid);
				return true;
			}
		}
		return bResult;
	}

	bool DisconnectClient(DWORD targetPid)
	{
		BOOL bResult;
		auto iter = PipeMap.find(targetPid);
		if (iter == PipeMap.end())
			return false;
		HANDLE hPipe = (*iter).second;

		bResult = FlushFileBuffers(hPipe);
		if (!bResult)
			return false;

		bResult = DisconnectNamedPipe(hPipe);
		return bResult;
	}
}
