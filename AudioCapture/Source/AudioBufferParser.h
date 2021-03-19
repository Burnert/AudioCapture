#pragma once

#define NOMINMAX
#include <Windows.h>

struct SSample
{
	float L;
	float R;
};

struct SVolume
{
	float Avg;
	float Max;
};

// Works only for 32-bit samples
SVolume GetBufferVolume32(BYTE* buffer, unsigned int frames, unsigned int frameSize);
