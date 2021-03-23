#include "ACPCH.h"

#include "CaptureCore.h"

namespace Capture
{
	HANDLE SnapshotHandle = 0;

	SModuleOffsets ModuleOffsets;

	void SModuleOffsets::LoadOffsets()
	{
		// Get function offsets from locally loaded dll
		HMODULE hACM = LoadLibraryExA(AUDIOCAPTUREDLL_NAME, NULL, DONT_RESOLVE_DLL_REFERENCES);
		EXIT_ON_NULL_M(hACM, "Cannot load " AUDIOCAPTUREDLL_NAME " library!\n");
		void* killFuncAddress = GetProcAddress(hACM, "Kill");
		EXIT_ON_NULL_M(killFuncAddress, "Cannot find Kill function offset!\n");
		Kill = (DWORD)((BYTE*)killFuncAddress - (BYTE*)hACM);
		FreeLibrary(hACM);
	}

	void InitCore()
	{
		ModuleOffsets.LoadOffsets();
	}

	bool UpdateSnapshot(DWORD flags, DWORD pid)
	{
		SnapshotHandle = CreateToolhelp32Snapshot(flags, pid);
		return SnapshotHandle != INVALID_HANDLE_VALUE;
	}

	void CloseSnapshot()
	{
		CloseHandle(SnapshotHandle);
	}

	bool _FindProcess(PROCESSENTRY32* pOutProcessEntry, unsigned long pid, const char* name)
	{
		if (pid == 0 && name == nullptr)
		{
			printf_s("Parameters are invalid!\n");
			exit(0);
		}

		bool bResult;

		if (name != nullptr)
		{
			bResult = FindProcess(SnapshotHandle, name, pOutProcessEntry);
		}
		else
		{
			bResult = FindProcess(SnapshotHandle, pid, pOutProcessEntry);
		}

		if (!bResult)
		{
			DWORD error = GetLastError();
			if (error == ERROR_NO_MORE_FILES)
			{
				printf_s("No processes exist or the snapshot does not contain process information!\n");
			}

			printf_s("Cannot find the process!\n");
			exit(0);
			return false;
		}

		printf_s("[%d] %s has been found.\n", pOutProcessEntry->th32ProcessID, pOutProcessEntry->szExeFile);
		return true;
	}

	// --------------------------------
	//  CaptureInstance:
	// --------------------------------

#define _ERROR_MESSAGE(format, ...) \
{ \
	char message[1000]; \
	sprintf_s(message, format, __VA_ARGS__); \
	CreateErrorMessage(message); \
}
#define _RETURN_ON_ERROR(x, format, ...) \
if (!(x)) \
{ \
	_ERROR_MESSAGE(format, __VA_ARGS__);  return false; \
}

	CaptureInstance::CaptureInstance(const SProcessInfo& process)
		: m_Pid(process.Pid), m_ErrorMessage(nullptr), m_hProcess(process.Handle)
	{
		m_ProcessName = process.ProcessName;
	}

	CaptureInstance::~CaptureInstance()
	{
		if (m_ErrorMessage)
			delete[] m_ErrorMessage;
	}

	bool CaptureInstance::FindAudioSessionModule()
	{
		bool bResult;

//		int processSnapshotFlags = TH32CS_SNAPMODULE;
//#ifdef PLATFORM64
//		processSnapshotFlags |= TH32CS_SNAPMODULE32;
//
//		int wow64Process;
//		IsWow64Process(m_hProcess, &wow64Process);
//		if (wow64Process)
//			processSnapshotFlags = TH32CS_SNAPMODULE32;
//#endif

		if (!UpdateSnapshot(TH32CS_SNAPMODULE, m_Pid))
			return false;

		MODULEENTRY32 moduleEntry;
		bResult = FindModule(SnapshotHandle, "AudioSes.dll", &moduleEntry);
		_RETURN_ON_ERROR(bResult, "Cannot find AudioSes.dll in module list of the process %s.\n", m_ProcessName.c_str());

		printf_s("Module AudioSes.dll has been found.\n");
		CloseSnapshot();

		return true;
	}

	bool CaptureInstance::InjectModule()
	{
		printf_s("Injecting AudioCaptureModule.dll...\n");

		bool bResult;

		char modulePath[MAX_PATH + 1];
		GetFullPathNameA(AUDIOCAPTUREDLL_NAME, MAX_PATH + 1, modulePath, NULL);

		// Alloc process memory
		m_RemoteParam = VirtualAllocEx(m_hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		_RETURN_ON_ERROR(m_RemoteParam, "Cannot allocate memory for process %s.\n", m_ProcessName.c_str());

		// Write DLL to process
		SIZE_T bytesWritten;
		bResult = WriteProcessMemory(m_hProcess, m_RemoteParam, modulePath, strlen(modulePath) + 1, &bytesWritten);
		_RETURN_ON_ERROR(bResult, "Cannot write memory for process %s.\n", m_ProcessName.c_str());

		// Load DLL
		HANDLE hThread = CreateRemoteThread(m_hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, m_RemoteParam, 0, NULL);
		_RETURN_ON_ERROR(hThread, "Cannot create thread in process %s.\n", m_ProcessName.c_str());
		CloseHandle(hThread);

		return true;
	}

	bool CaptureInstance::EjectModule()
	{
		bool bResult;

		VirtualFreeEx(m_hProcess, m_RemoteParam, 0, MEM_RELEASE);

		// Kill injected module thread
		MODULEENTRY32 moduleEntry;
		if (!UpdateSnapshot(TH32CS_SNAPMODULE, m_Pid))
			return false;
		bResult = FindModule(SnapshotHandle, AUDIOCAPTUREDLL_NAME, &moduleEntry);
		_RETURN_ON_ERROR(bResult, "Cannot find and unload module " AUDIOCAPTUREDLL_NAME "!\n");
		CloseSnapshot();

		// Execute Kill function in remote thread
		HANDLE hThread = CreateRemoteThread(m_hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(moduleEntry.modBaseAddr + ModuleOffsets.Kill), NULL, 0, NULL);
		_RETURN_ON_ERROR(hThread, "Cannot create module unloading thread in process %s.\n", m_ProcessName.c_str());
		CloseHandle(hThread);

		return true;
	}

	bool CaptureInstance::CreateBufferPipe()
	{
		bool bResult = IPC::CreateByteBufferPipe(m_Pid);
		_RETURN_ON_ERROR(bResult, "Cannot create a buffer pipe!\n");

		return true;
	}

	bool CaptureInstance::DeleteBufferPipe()
	{
		bool bResult = IPC::DeleteByteBufferPipe(m_Pid);
		_RETURN_ON_ERROR(bResult, "Cannot remove a buffer pipe!\n");

		return true;
	}

	bool CaptureInstance::WaitForClientConnection()
	{
		printf_s("Waiting for %s to connect to pipe...\n", m_ProcessName.c_str());
		bool bResult = IPC::WaitForConnection(m_Pid);
		_RETURN_ON_ERROR(bResult, "Cannot connect pipe to [%d]!", m_Pid);
		printf_s("Successfully connected to [%d]!\n", m_Pid);

		return true;
	}

	bool CaptureInstance::DisconnectClient()
	{
		bool bResult = IPC::DisconnectClient(m_Pid);
		_RETURN_ON_ERROR(bResult, "Cannot disconnect from [%d]!", m_Pid);

		return true;
	}

	void CaptureInstance::PrintLastError()
	{
		printf_s(m_ErrorMessage);
	}

	void CaptureInstance::CreateErrorMessage(const char* message)
	{
		if (m_ErrorMessage)
			delete[] m_ErrorMessage;
		
		size_t size = strlen(message) + 1;
		m_ErrorMessage = new char[size];
		strcpy_s(m_ErrorMessage, size, message);
	}
}
