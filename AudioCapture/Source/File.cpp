#include "File.h"

// File

File::File()
{
}

File::~File()
{
}

bool File::Open(const char* path, IO::EFileAccess access)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);

	DWORD dwAccess;
	switch (access)
	{
		case IO::FA_Read:      dwAccess = GENERIC_READ;                  break;
		case IO::FA_Write:     dwAccess = GENERIC_WRITE;                 break;
		case IO::FA_ReadWrite: dwAccess = GENERIC_READ | GENERIC_WRITE;  break;
		default:               dwAccess = 0;
	}

	m_hFile = CreateFileA(path, dwAccess, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return m_hFile != INVALID_HANDLE_VALUE;
}

bool File::Close()
{
	return CloseHandle(m_hFile);
}

bool File::Read(BYTE* buffer, unsigned int count, unsigned int* bytesRead)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;

	return true;
}

bool File::Write(const void* buffer, unsigned int count)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD bytesWritten;
	if (!WriteFile(m_hFile, buffer, count, &bytesWritten, NULL))
		return false;

	return true;
}

// AudioFile

bool AudioFile::WriteWaveOpening(Audio::SWaveInfo* waveInfo)
{
	if (m_hFile == INVALID_HANDLE_VALUE || !waveInfo)
		return false;

	// Main Chunk
	Write(&WaveChunkID, 4);
	UINT32 chunkSize = 36 + waveInfo->DataSize;
	Write(&chunkSize, 4);
	Write((BYTE*)&WaveFormat, 4);

	// Subchunk1
	Write((BYTE*)&WaveSubChunk1ID, 4);
	Write(&WaveSubChunk1Size, 4);
	Write(&waveInfo->WaveFormat, 2);
	Write(&waveInfo->Format->Channels, 2);
	Write(&waveInfo->Format->SampleRate, 4);
	UINT32 byteRate = waveInfo->Format->SampleRate * waveInfo->Format->Channels * waveInfo->Format->BitDepth / 8;
	Write(&byteRate, 4);
	USHORT blockAlign = waveInfo->Format->Channels * waveInfo->Format->BitDepth / 8;
	Write(&blockAlign, 2);
	Write(&waveInfo->Format->BitDepth, 2);

	// Subchunk2
	Write((BYTE*)&WaveSubChunk2ID, 4);
	Write(&waveInfo->DataSize, 4);

	return true;
}

bool AudioFile::WriteWaveSize(unsigned long size)
{
	if (m_hFile == INVALID_HANDLE_VALUE || size == 0)
		return false;

	LARGE_INTEGER newPointer;
	LARGE_INTEGER sizeOffset;

	DWORD bytesWritten;

	// Write ChunkSize
	sizeOffset.QuadPart = 4;
	if (!SetFilePointerEx(m_hFile, sizeOffset, &newPointer, FILE_BEGIN))
		return false;

	DWORD chunkSize = 36 + size;
	if (!WriteFile(m_hFile, &chunkSize, 4, &bytesWritten, NULL))
		return false;

	// Write Subchunk2Size
	sizeOffset.QuadPart = 40;
	if (!SetFilePointerEx(m_hFile, sizeOffset, &newPointer, FILE_BEGIN))
		return false;

	if (!WriteFile(m_hFile, &size, 4, &bytesWritten, NULL))
		return false;

	return true;
}
