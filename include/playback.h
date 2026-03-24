#ifndef ctrmus_playback_h
#define ctrmus_playback_h

#include <stdbool.h>
#include <stdint.h>
#include <limits.h> // for PATH_MAX

/* Channel to play music on */
#define CHANNEL 0x08

#define MAX_CHANNELS 24

extern float channelPitch[MAX_CHANNELS];

struct decoder_fn
{
	/* Initialize decoder for 'file'. Return an allocated state pointer
	   (or NULL on error). Also set *buffSizeOut to the number of samples
	   per channel to decode per buffer (i.e. buffer length in samples
	   PER CHANNEL). */
	void *(*init)(const char *file, size_t *buffSizeOut);

	/* Query properties for this state. */
	uint32_t (*rate)(void *state);
	uint8_t (*channels)(void *state);

	/* Buffer size in samples per channel (filled by init) */
	size_t buffSize;

	/* Decode into buffer. Returns number of samples (total, i.e. channels * frames). */
	uint64_t (*decode)(void *state, void *buffer);

	/* Free state. */
	void (*exit)(void *state);

	/* Optional: get total file samples (or 0 if unknown). */
	size_t (*getFileSamples)(void *state);
};

struct errInfo_t; // forward declaration

typedef struct playbackInfo_t {

	
	int channel;
	char file[PATH_MAX];
	struct errInfo_t *errInfo;
	
	size_t samples_total;
	size_t samples_played;
    int sample_rate;
	float playback_rate;
	size_t samples_per_second;

	volatile bool stop;
	volatile bool stopFlag;
} playbackInfo_t;

/**
 * Pause or play current file.
 *
 * \return True if paused.
 */
bool togglePlayback(playbackInfo_t *info);

/**
 * Stops playback. Sets stop flag inside playbackInfo.
 *
 * \param info Pointer to playbackInfo_t whose playback should stop.
 */
void stopPlayback(playbackInfo_t *info);

/**
 * Returns whether playback is ongoing.
 *
 * \param info Pointer to playbackInfo_t to query.
 * \return True if playback is ongoing (not stopped).
 */
bool isPlaying(playbackInfo_t *info);

/**
 * Plays audio file on a new thread, managing playback state.
 *
 * \param infoIn Pointer to playbackInfo_t with playback parameters.
 */
void playFile(void *infoIn);

extern void scratchDisk(int channel, float skipSeconds);

#endif
