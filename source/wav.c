#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>

#include "wav.h"
#include "playback.h"

struct WavState
{
	drwav wav;
	size_t buffSize;
};

static const size_t DEFAULT_BUFF_SIZE = 16 * 1024;

/* API implemented per new decoder_fn */
static void *initWav(const char *file, size_t *buffSizeOut)
{
	struct WavState *st = malloc(sizeof(struct WavState));
	if (!st)
		return NULL;
	memset(st, 0, sizeof(*st));

	if (!drwav_init_file(&st->wav, file, NULL))
	{
		free(st);
		return NULL;
	}
	st->buffSize = DEFAULT_BUFF_SIZE;
	if (buffSizeOut)
		*buffSizeOut = st->buffSize;
	return st;
}

static uint32_t rateWav(void *state)
{
	struct WavState *st = (struct WavState *)state;
	return st->wav.sampleRate;
}

static uint8_t channelWav(void *state)
{
	struct WavState *st = (struct WavState *)state;
	return st->wav.channels;
}

static uint64_t readWav(void *state, void *buffer)
{
	struct WavState *st = (struct WavState *)state;
	size_t bytesPerFrame = st->wav.channels * sizeof(int16_t);
	size_t buffSizeFrames = st->buffSize / bytesPerFrame;
	uint64_t samplesRead = drwav_read_pcm_frames_s16(&st->wav, buffSizeFrames, buffer);
	samplesRead *= (uint64_t)st->wav.channels;
	return samplesRead;
}

static void exitWav(void *state)
{
	struct WavState *st = (struct WavState *)state;
	if (!st)
		return;
	drwav_uninit(&st->wav);
	free(st);
}

static size_t getFileSamplesWav(void *state)
{
	struct WavState *st = (struct WavState *)state;
	return st->wav.totalPCMFrameCount * (size_t)st->wav.channels;
}

/**
 * Set decoder parameters for WAV.
 */
void setWav(struct decoder_fn *decoder)
{
	decoder->init = &initWav;
	decoder->rate = &rateWav;
	decoder->channels = &channelWav;
	decoder->buffSize = DEFAULT_BUFF_SIZE;
	decoder->decode = &readWav;
	decoder->exit = &exitWav;
	decoder->getFileSamples = &getFileSamplesWav;
}
