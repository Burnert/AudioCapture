#pragma once

#include <Windows.h>

namespace IPC
{
	bool CreateByteBufferPipe(DWORD targetPid);
	bool WaitForConnection(DWORD targetPid);
	bool DeleteMessagePipe(DWORD targetPid);
}
