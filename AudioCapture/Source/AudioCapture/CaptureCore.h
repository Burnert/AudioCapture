#pragma once

#include "Core/Core.hpp"
#include "AudioCapture/IPC.h"
#include "AudioCapture/File.h"

namespace Capture
{
	struct SModuleOffsets
	{
		DWORD Kill;

		void LoadOffsets();
	};

	extern HANDLE SnapshotHandle;

	extern SModuleOffsets ModuleOffsets;

	void InitCore();

	bool UpdateSnapshot(DWORD flags, DWORD pid = 0);
	void CloseSnapshot();

	bool _FindProcess(PROCESSENTRY32* pOutProcessEntry, unsigned long pid, const char* name = nullptr);

	inline bool FindProcessByName(const char* name, PROCESSENTRY32* pOutProcessEntry)
	{
		return _FindProcess(pOutProcessEntry, 0, name);
	}

	inline bool FindProcessByPID(unsigned long pid, PROCESSENTRY32* pOutProcessEntry)
	{
		return _FindProcess(pOutProcessEntry, pid);
	}

	// Specific:

	struct SProcessInfo
	{
		DWORD Pid;
		std::string ProcessName;
		HANDLE Handle;
	};

#define VERIFY_CI(ci_call, on_fail) \
if (!(ci_call)) \
{ \
	on_fail; \
	EXIT(); \
}

	class CaptureInstance
	{
	public:
		CaptureInstance(const SProcessInfo& process);
		~CaptureInstance();

		bool FindAudioSessionModule();
		bool InjectModule();
		bool EjectModule();

		bool CreateBufferPipe();
		bool DeleteBufferPipe();

		bool WaitForClientConnection();
		bool DisconnectClient();

		void PrintLastError();

	private:
		void CreateErrorMessage(const char* message);

	private:
		HANDLE m_hProcess;
		const DWORD m_Pid;
		std::string m_ProcessName;

		/* Param location used by CreateRemoteThread that loads the DLL */
		void* m_RemoteParam;

		char* m_ErrorMessage;
	};
}
