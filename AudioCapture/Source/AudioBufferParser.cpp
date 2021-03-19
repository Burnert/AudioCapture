#include "AudioBufferParser.h"

#include <cmath>
#include <cstdio>

// Works only for 32-bit samples
SVolume GetBufferVolume32(BYTE* buffer, unsigned int frames, unsigned int frameSize)
{
	BYTE* bufferPtr = buffer;
	SVolume volume { };

	for (unsigned int pos = 0; pos < frames; ++pos)
	{
		unsigned char channelSize = frameSize / 2;
		SSample sample { };

		//sample.L |= *(BYTE*)bufferPtr;
		//sample.L |= *(BYTE*)(bufferPtr + 1) << 8;
		//sample.L |= *(BYTE*)(bufferPtr + 2) << 16;
		//sample.L |= *(BYTE*)(bufferPtr + 3) << 24;
		sample.L = *(float*)bufferPtr;
		bufferPtr += channelSize;

		//sample.L |= *(BYTE*)bufferPtr;
		//sample.L |= *(BYTE*)(bufferPtr + 1) << 8;
		//sample.L |= *(BYTE*)(bufferPtr + 2) << 16;
		//sample.L |= *(BYTE*)(bufferPtr + 3) << 24;
		sample.R = *(float*)bufferPtr;
		bufferPtr += channelSize;

		//int mid = (sample.L + sample.R) / 2;
		//float sampleAmplitude = mid / 2147483648.0f;
		float sampleMid = (sample.L + sample.R) / 2.0f;
		float sampleVolume = abs(sampleMid);

		volume.Avg += sampleVolume;
		volume.Max = std::fmaxf(volume.Max, sampleVolume);

		if (pos == -1)
		// Dead code:
			printf_s("First sample volume: %.2f   ", sampleVolume);
	}
	volume.Avg /= (float)frames;

	return volume;
}

