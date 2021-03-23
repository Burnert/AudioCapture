#include "ACPCH.h"

#include "Core/Core.hpp"
#include "AudioCapture/IPC.h"
#include "AudioCapture/File.h"
#include "AudioCapture/AudioTest.hpp"

static HANDLE SnapshotHandle = 0;

static MODULEENTRY32 AudioSesEntry;
static MODULEENTRY32 AudioCaptureModuleEntry;

struct SModuleOffsets
{
	DWORD Kill;

	void LoadOffsets()
	{
		// Get function offsets from locally loaded dll
		HMODULE hACM = LoadLibraryExA(AUDIOCAPTUREDLL_NAME, NULL, DONT_RESOLVE_DLL_REFERENCES);
		EXIT_ON_NULL_M(hACM, "Cannot load " AUDIOCAPTUREDLL_NAME " library!\n");
		void* killFuncAddress = GetProcAddress(hACM, "Kill");
		EXIT_ON_NULL_M(killFuncAddress, "Cannot find Kill function offset!\n");
		Kill = (DWORD)((BYTE*)killFuncAddress - (BYTE*)hACM);
		FreeLibrary(hACM);
	}
};

static SModuleOffsets ModuleOffsets;

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
	EXIT_ON_NO_FILE_M(AUDIOCAPTUREDLL_NAME, "File %s not found!\n", AUDIOCAPTUREDLL_NAME);

	if (argc < 3)
	{
		ExitOnWrongUsage();
	}

	ModuleOffsets.LoadOffsets();

	BOOL bResult;

	// Find wanted process
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

	// Find Audio Session module
	SnapshotHandle = CreateToolhelp32Snapshot(processSnapshotFlag, processEntry.th32ProcessID);
	EXIT_ON_INVALID_HANDLE(SnapshotHandle);
	MODULEENTRY32 moduleEntry;
	bResult = FindModule(SnapshotHandle, "AudioSes.dll", &moduleEntry);
	EXIT_ON_FALSE_M(bResult, "Cannot find AudioSes.dll in module list of the process %s.\n", processEntry.szExeFile);
	printf_s("Module AudioSes.dll has been found.\n");
	CloseHandle(SnapshotHandle);
#if 0
	CoInitialize(NULL);
	MyAudioSource m;
	PlayAudioStream(&m);
#endif
	// Create an appropriate pipe
	bResult = IPC::CreateByteBufferPipe(processEntry.th32ProcessID);
	EXIT_ON_FALSE_M(bResult, "Cannot create a message pipe!\n");

	// Create a file for writing
	AudioFile file;
	bResult = file.Open("AudioOutput.wav", IO::FA_ReadWrite);
	EXIT_ON_FALSE_M(bResult, "Cannot create AudioOutput.wav!\n");

	Audio::SAudioFormat audioFormatInfo { };
	audioFormatInfo.Channels = 2;
	audioFormatInfo.BitDepth = 32;
	audioFormatInfo.SampleRate = 48000;

	Audio::SWaveInfo waveInfo { };
	waveInfo.Format = &audioFormatInfo;
	waveInfo.DataSize = 0;
	waveInfo.WaveFormat = WAVE_FORMAT_IEEE_FLOAT;

	file.WriteWaveOpening(&waveInfo);
	EXIT_ON_FALSE_M(bResult, "Cannot write to AudioOutput.wav!\n");

	DWORD fileSize = 0;

	// Inject AudioCaptureModule to the process
	printf_s("Injecting AudioCaptureModule.dll...\n");

	char modulePath[MAX_PATH + 1];
	GetFullPathNameA(AUDIOCAPTUREDLL_NAME, MAX_PATH + 1, modulePath, NULL);

	// Alloc process memory
	void* paramLocation = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	EXIT_ON_NULL_M(paramLocation, "Cannot allocate memory for process %s.\n", processEntry.szExeFile);

	// Write DLL to process
	size_t bytesWritten;
	bResult = WriteProcessMemory(hProcess, paramLocation, modulePath, strlen(modulePath) + 1, &bytesWritten);
	EXIT_ON_FALSE_M(bResult, "Cannot write memory for process %s.\n", processEntry.szExeFile);

	// Load DLL
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, paramLocation, 0, NULL);
	EXIT_ON_NULL_M(hThread, "Cannot create thread in process %s.\n", processEntry.szExeFile);
	CloseHandle(hThread);

	// Wait for the process to connect to the pipe
	printf_s("Waiting for %s to connect to pipe...\n", processEntry.szExeFile);
	bResult = IPC::WaitForConnection(processEntry.th32ProcessID);
	EXIT_ON_FALSE_M(bResult, "Cannot connect pipe to [%d]!", processEntry.th32ProcessID);
	printf_s("Successfully connected to [%d]!\n", processEntry.th32ProcessID);

	// Wait for the key press
 	while (!(GetAsyncKeyState(VK_RCONTROL) & 0x8000))
	{
		// Read from the buffer pipe

		DWORD bufferSize = IPC::PeekByteBufferPipeSize(processEntry.th32ProcessID);
		if (bufferSize == 0)
			continue;

		bResult = IPC::AllocByteBuffer(bufferSize);
		EXIT_ON_FALSE(bResult);

		bResult = IPC::ReadByteBufferPipe(processEntry.th32ProcessID, bufferSize);
		EXIT_ON_FALSE(bResult);

		fileSize += bufferSize;
		// Write the read buffer to file
		bResult = file.Write(IPC::BufferData, bufferSize);
		EXIT_ON_FALSE(bResult);

		bResult = IPC::FreeByteBuffer();
		EXIT_ON_FALSE(bResult);
	}
	// Finalize the file by writing the size
	file.WriteWaveSize(fileSize);
	file.Close();

	IPC::DisconnectClient(processEntry.th32ProcessID);

	VirtualFreeEx(hProcess, paramLocation, 0, MEM_RELEASE);

	// Kill injected module thread
	SnapshotHandle = CreateToolhelp32Snapshot(processSnapshotFlag, processEntry.th32ProcessID);
	bResult = FindModule(SnapshotHandle, AUDIOCAPTUREDLL_NAME, &moduleEntry);
	EXIT_ON_FALSE_M(bResult, "Cannot find and unload module " AUDIOCAPTUREDLL_NAME "!\n");
	CloseHandle(SnapshotHandle);

	// Execute Kill function in remote thread
	hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(moduleEntry.modBaseAddr + ModuleOffsets.Kill), NULL, 0, NULL);
	EXIT_ON_NULL_M(hThread, "Cannot create module unloading thread in process %s.\n", processEntry.szExeFile);
	CloseHandle(hThread);

	IPC::DeleteByteBufferPipe(processEntry.th32ProcessID);

	return 0;
}
