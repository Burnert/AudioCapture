#pragma once

#include <Windows.h>

namespace IPC
{
	extern BYTE* BufferData;

	bool CreateByteBufferPipe(DWORD targetPid);
	bool DeleteByteBufferPipe(DWORD targetPid);
	bool ReadByteBufferPipe(DWORD targetPid);

	bool WaitForConnection(DWORD targetPid);
	bool DisconnectClient(DWORD targetPid);
}
