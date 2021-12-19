#pragma once

#include <Windows.h>

namespace IO
{
	enum EFileAccess
	{
		FA_Read,
		FA_Write,
		FA_ReadWrite
	};
}

namespace Audio
{
	struct AudioFormat
	{
		unsigned short Channels;
		unsigned short BitDepth;
		unsigned long SampleRate;
	};

	struct WaveInfo
	{
		AudioFormat* Format;
		unsigned short WaveFormat;
		unsigned long DataSize;
	};
}

class File
{
public:
	File();
	virtual ~File();

	bool Open(const char* path, IO::EFileAccess access);
	bool Close();
	bool Read(BYTE* buffer, unsigned int count, unsigned int* bytesRead);
	bool Write(const void* buffer, unsigned int count);

protected:
	HANDLE m_hFile = INVALID_HANDLE_VALUE;
};

class AudioFile : public File
{
public:
	bool WriteWaveOpening(Audio::WaveInfo* waveInfo);
	/* Use it only after the whole file has been written! (moves the file pointer) */
	bool WriteWaveSize(unsigned long size);

private:
	static constexpr BYTE WaveChunkID[]       = { 0x52, 0x49, 0x46, 0x46 };
	static constexpr BYTE WaveFormat[]        = { 0x57, 0x41, 0x56, 0x45 };
	static constexpr BYTE WaveSubChunk1ID[]   = { 0x66, 0x6d, 0x74, 0x20 };
	static constexpr BYTE WaveSubChunk2ID[]   = { 0x64, 0x61, 0x74, 0x61 };
	static constexpr DWORD WaveSubChunk1Size  = 16;
};