#pragma once

#include <Windows.h>

namespace IPC
{
	extern BYTE* BufferData;

	bool CreateByteBufferPipe(DWORD targetPid);
	bool DeleteByteBufferPipe(DWORD targetPid);
	bool ReadByteBufferPipe(DWORD targetPid, DWORD count);
	DWORD PeekByteBufferPipeSize(DWORD targetPid);

	bool AllocByteBuffer(DWORD size);
	bool FreeByteBuffer();

	bool WaitForConnection(DWORD targetPid);
	bool DisconnectClient(DWORD targetPid);
}
