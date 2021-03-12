#include <cstdio>
#include <string>

#include <Windows.h>
#include <TlHelp32.h>
#include <Audioclient.h>

#include "Core/Core.hpp"
#include "AudioTest.hpp"

static HANDLE SnapshotHandle = 0;

static MODULEENTRY32 AudioSesEntry;
static MODULEENTRY32 AudioCaptureModuleEntry;

static struct SModuleOffsets
{
	DWORD Kill;

	void LoadOffsets()
	{
		// Get function offsets from locally loaded dll
		HMODULE hACM = LoadLibraryExA(AUDIOCAPTUREDLL_NAME, NULL, DONT_RESOLVE_DLL_REFERENCES);
		EXIT_ON_NULL_M(hACM, "Cannot load " AUDIOCAPTUREDLL_NAME " library!\n");
		void* killFuncAddress = GetProcAddress(hACM, "Kill");
		EXIT_ON_NULL_M(killFuncAddress, "Cannot find Kill function offset!\n");
		Kill = (DWORD64)killFuncAddress - (DWORD64)hACM;
		FreeLibrary(hACM);
	}
} ModuleOffsets;

static inline void ExitOnWrongUsage()
{
	printf_s("Usage: AudioCapture.exe [-im|-pid] <process name|PID>\n");
	exit(0);
}

static bool _FindProcess(PROCESSENTRY32* pOutProcessEntry, unsigned long pid, const char* name = nullptr)
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

static inline bool FindProcessByName(const char* name, PROCESSENTRY32* pOutProcessEntry)
{
	return _FindProcess(pOutProcessEntry, 0, name);
}

static inline bool FindProcessByPID(unsigned long pid, PROCESSENTRY32* pOutProcessEntry)
{
	return _FindProcess(pOutProcessEntry, pid);
}

static PROCESSENTRY32 InterpretArgs(int& argc, char** argv)
{
	bool bResult = false;
	PROCESSENTRY32 processEntry { };

	if (_strcmpi(argv[1], "-im") == 0)
	{
		bResult = FindProcessByName(argv[2], &processEntry);
	}
	else if (_strcmpi(argv[1], "-pid") == 0)
	{
		unsigned long pid = strtoul(argv[2], nullptr, 10);
		if (pid == 0 || pid == ULONG_MAX)
		{
			ExitOnWrongUsage();
		}

		bResult = FindProcessByPID(pid, &processEntry);
	}
	else
	{
		ExitOnWrongUsage();
	}

	EXIT_ON_FALSE(bResult);

	return processEntry;
}

int main(int argc, char** argv)
{
	EXIT_ON_NO_FILE_M(AUDIOCAPTUREDLL_NAME, "Module %s not found!\n", AUDIOCAPTUREDLL_NAME);

	if (argc < 3)
	{
		ExitOnWrongUsage();
	}

	ModuleOffsets.LoadOffsets();

	BOOL bResult;

	SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	EXIT_ON_INVALID_HANDLE(SnapshotHandle);
	PROCESSENTRY32 processEntry = InterpretArgs(argc, argv);
	CloseHandle(SnapshotHandle);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID);
	EXIT_ON_INVALID_HANDLE_M(hProcess, "Cannot open process %s!\n", processEntry.szExeFile);

	int processSnapshotFlag = TH32CS_SNAPMODULE;
#ifdef PLATFORM64
	processSnapshotFlag |= TH32CS_SNAPMODULE32;

	int wow64Process;
	IsWow64Process(hProcess, &wow64Process);
	if (wow64Process)
	{
		processSnapshotFlag = TH32CS_SNAPMODULE32;
	}
#endif

	SnapshotHandle = CreateToolhelp32Snapshot(processSnapshotFlag, processEntry.th32ProcessID);
	EXIT_ON_INVALID_HANDLE(SnapshotHandle);
	MODULEENTRY32 moduleEntry;
	bResult = FindModule(SnapshotHandle, "AudioSes.dll", &moduleEntry);
	EXIT_ON_FALSE_M(bResult, "Cannot find AudioSes.dll in module list of the process %s.\n", processEntry.szExeFile);
	printf_s("Module AudioSes.dll has been found.\n");
	CloseHandle(SnapshotHandle);

	//CoInitialize(NULL);
	//MyAudioSource m;
	//PlayAudioStream(&m);

	printf_s("Injecting AudioCaptureModule.dll...\n");

	char modulePath[MAX_PATH + 1];
	GetFullPathNameA(AUDIOCAPTUREDLL_NAME, MAX_PATH + 1, modulePath, NULL);

	void* paramLocation = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	EXIT_ON_NULL_M(paramLocation, "Cannot allocate memory for process %s.\n", processEntry.szExeFile);

	size_t bytesWritten;
	bResult = WriteProcessMemory(hProcess, paramLocation, modulePath, strlen(modulePath) + 1, &bytesWritten);

	CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, paramLocation, 0, NULL);

	while (!(GetAsyncKeyState(VK_RCONTROL) & 0x8000))
	{
		Sleep(10);
	}

	VirtualFreeEx(hProcess, paramLocation, 0, MEM_RELEASE);

	// Kill injected module thread

	SnapshotHandle = CreateToolhelp32Snapshot(processSnapshotFlag, processEntry.th32ProcessID);
	bResult = FindModule(SnapshotHandle, AUDIOCAPTUREDLL_NAME, &moduleEntry);
	EXIT_ON_FALSE_M(bResult, "Cannot unload module " AUDIOCAPTUREDLL_NAME "!\n");
	CloseHandle(SnapshotHandle);

	CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(moduleEntry.modBaseAddr + ModuleOffsets.Kill), NULL, 0, NULL);

	return 0;
}
