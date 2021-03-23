#include "ACPCH.h"

#include "AudioCapture/CaptureCore.h"
#include "AudioCapture/AudioTest.hpp"

static inline void ExitOnWrongUsage()
{
	printf_s("Usage: AudioCapture.exe [-im|-pid] <process name|PID>\n");
	exit(0);
}

static PROCESSENTRY32 InterpretArgs(int& argc, char** argv)
{
	bool bResult = false;
	PROCESSENTRY32 processEntry { };

	if (_strcmpi(argv[1], "-im") == 0)
	{
		bResult = Capture::FindProcessByName(argv[2], &processEntry);
	}
	else if (_strcmpi(argv[1], "-pid") == 0)
	{
		unsigned long pid = strtoul(argv[2], nullptr, 10);
		if (pid == 0 || pid == ULONG_MAX)
		{
			ExitOnWrongUsage();
		}

		bResult = Capture::FindProcessByPID(pid, &processEntry);
	}
	else
	{
		ExitOnWrongUsage();
	}

	EXIT_ON_FALSE(bResult);

	return processEntry;
}

#define _VERIFY_CI_ERROR(exec) \
VERIFY_CI(exec, captureInst.PrintLastError())

int main(int argc, char** argv)
{
	EXIT_ON_NO_FILE_M(AUDIOCAPTUREDLL_NAME, "File %s not found!\n", AUDIOCAPTUREDLL_NAME);

	if (argc < 3)
	{
		ExitOnWrongUsage();
	}

	Capture::InitCore();

	BOOL bResult;

	// Find wanted process
	EXIT_ON_FALSE(Capture::UpdateSnapshot(TH32CS_SNAPPROCESS));
	PROCESSENTRY32 processEntry = InterpretArgs(argc, argv);
	Capture::CloseSnapshot();

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID);
	EXIT_ON_INVALID_HANDLE_M(hProcess, "Cannot open process %s!\n", processEntry.szExeFile);

	Capture::SProcessInfo process;
	process.Handle = hProcess;
	process.Pid = processEntry.th32ProcessID;
	process.ProcessName = processEntry.szExeFile;

	Capture::CaptureInstance captureInst(process);

	// Find AudioSes.dll in process
	_VERIFY_CI_ERROR(captureInst.FindAudioSessionModule());
	
#if 0
	CoInitialize(NULL);
	MyAudioSource m;
	PlayAudioStream(&m);
#endif
	// Create an appropriate pipe
	_VERIFY_CI_ERROR(captureInst.CreateBufferPipe());

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

	// Accumulated file size
	DWORD fileSize = 0;

	// Inject AudioCaptureModule to the process
	_VERIFY_CI_ERROR(captureInst.InjectModule());

	// Wait for the process to connect to the pipe
	_VERIFY_CI_ERROR(captureInst.WaitForClientConnection());

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

	// Cleanup...

	_VERIFY_CI_ERROR(captureInst.DisconnectClient());

	_VERIFY_CI_ERROR(captureInst.EjectModule());

	_VERIFY_CI_ERROR(captureInst.DeleteBufferPipe());

	return 0;
}
