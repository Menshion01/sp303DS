#include <3ds.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "all.h"
#include "error.h"
#include "file.h"
#include "mp3.h"
#include "playback.h"
#include "text.h"
#include "wav.h"

#define MAX_CHANNELS 24
float channelPitch[MAX_CHANNELS] = { [0 ... MAX_CHANNELS-1] = 1};

static bool channelInUse[MAX_CHANNELS] = {false};
playbackInfo_t* activeInfo[MAX_CHANNELS] = {0};

// -----------------------------------------------------------------------------
// Channel management
// -----------------------------------------------------------------------------
static int getFreeChannel(void) {
  for (int ch = 0; ch < MAX_CHANNELS; ch++) {
    if (!channelInUse[ch]) {
      channelInUse[ch] = true;
      return ch;
    }
  }
  // No free channels reset them
  graphicalPrintf("[WARN] All NDSP channels busy");
  svcSleepThread(50 * 1000000);  // 50 ms
  ndspChnWaveBufClear(MAX_CHANNELS - 1);
  ndspChnReset(MAX_CHANNELS - 1);
  channelInUse[MAX_CHANNELS - 1] = true;
  return MAX_CHANNELS - 1;
}

static void releaseChannel(int ch) {
  if (ch < 0 || ch >= MAX_CHANNELS)
    return;
  // Wait until NDSP is idle
  while (ndspChnIsPlaying(ch))
    svcSleepThread(10 * 1000);  // wait 10 ms
  ndspChnWaveBufClear(ch);
  ndspChnReset(ch);
  channelInUse[ch] = false;
}

// -----------------------------------------------------------------------------
// Playback control helpers
// -----------------------------------------------------------------------------
bool togglePlayback(playbackInfo_t* info) {
  if (!info)
    return false;
  int ch = info->channel;
  if (ch < 0 || ch >= MAX_CHANNELS)
    return false;
  bool paused = ndspChnIsPaused(ch);
  ndspChnSetPaused(ch, !paused);
  return !paused;
}

bool isPlaying(playbackInfo_t* info) {
  return (info && !info->stopFlag);
}

void stopPlayback(playbackInfo_t* info) {
  if (info)
    info->stopFlag = true;
}

// -----------------------------------------------------------------------------
// Main playback thread
// -----------------------------------------------------------------------------
void playFile(void* arg) {
  playbackInfo_t* info = (playbackInfo_t*)arg;
  struct decoder_fn decoder = {0};

  int16_t* buffer1 = NULL;
  int16_t* buffer2 = NULL;
  ndspWaveBuf waveBuf[2];
  bool lastbuf = false;
  bool stopNow = false;
  void* decoderState = NULL;

  // Acquire a free NDSP channel (reuse one if none free)
  int channel = getFreeChannel();
  if (channel < 0) {
    graphicalPrintf("[WARN] No free NDSP channels, reusing channel %d",
                    channel);
    ndspChnWaveBufClear(channel);
    ndspChnReset(channel);
    releaseChannel(channel);
    channelInUse[channel] = true;
  }
  info->channel = channel;

  activeInfo[channel] = info;

  /* Choose decoder by file type */
  switch (getFileType(info->file)) {
    case FILE_TYPE_WAV:
      setWav(&decoder);
      break;
    case FILE_TYPE_MP3:
      setMp3(&decoder);
      break;
    default:
      graphicalPrintf("[ERROR] Unsupported file type: %s", info->file);
      errno = EINVAL;
      goto cleanup;
  }

  /* Initialize decoder (state) */
  decoderState = decoder.init(info->file, &decoder.buffSize);
  if (decoderState == NULL) {
    graphicalPrintf("[ERROR] Decoder init failed: %s", info->file);
    errno = DECODER_INIT_FAIL;
    goto cleanup;
  }

  int channels = decoder.channels(decoderState);
  if (channels < 1 || channels > 2) {
    graphicalPrintf("[ERROR] Unsupported channel count: %d", channels);
    errno = UNSUPPORTED_CHANNELS;
    goto cleanup;
  }

  if (decoder.getFileSamples)
    info->samples_total = decoder.getFileSamples(decoderState);
  else
    info->samples_total = 0;

  info->samples_per_second = decoder.rate(decoderState);
  info->samples_played = 0;
  info->stopFlag = false;

  /* Debug */
  graphicalPrintf("Playing (ch %d): %s", channel, info->file);
  graphicalPrintf("File Rate: %lu Hz, Channels: %d, Buffer frames: %zu",
                  decoder.rate(decoderState), channels, decoder.buffSize);

  /* Configure NDSP channel to the file's sample rate - no software resample */
  ndspChnReset(channel);
  ndspChnWaveBufClear(channel);
  ndspChnSetInterp(channel, NDSP_INTERP_POLYPHASE);
  ndspChnSetRate(channel, ((float)decoder.rate(decoderState) * channelPitch[channel]));
  ndspChnSetFormat(channel, channels == 2 ? NDSP_FORMAT_STEREO_PCM16
                                          : NDSP_FORMAT_MONO_PCM16);

  /* Allocate linear buffers: decoder.buffSize is samples PER CHANNEL * frames,
     but decoders return total samples = frames * channels (original used that)
   */
  size_t totalSamplesPerBuf = decoder.buffSize * (size_t)channels;
  buffer1 = linearAlloc(totalSamplesPerBuf * sizeof(int16_t));
  buffer2 = linearAlloc(totalSamplesPerBuf * sizeof(int16_t));
  if (!buffer1 || !buffer2) {
    graphicalPrintf("[ERROR] Buffer allocation failed");
    errno = ENOMEM;
    goto cleanup;
  }

  /* Setup wave buffers */
  memset(waveBuf, 0, sizeof(waveBuf));
  waveBuf[0].data_vaddr = buffer1;
  waveBuf[1].data_vaddr = buffer2;

  /* Prime the buffers (decode into them) */
  size_t read0 = decoder.decode(decoderState, (void*)buffer1);
  if (read0 > 0) {
    // read0 is total samples (channels * frames)
    waveBuf[0].nsamples = (u32)(read0 / channels);
    ndspChnWaveBufAdd(channel, &waveBuf[0]);
  } else {
    waveBuf[0].nsamples = 0;
    lastbuf = true;
  }

  size_t read1 = decoder.decode(decoderState, (void*)buffer2);
  if (read1 > 0) {
    waveBuf[1].nsamples = (u32)(read1 / channels);
    ndspChnWaveBufAdd(channel, &waveBuf[1]);
  } else {
    waveBuf[1].nsamples = 0;
    lastbuf = true;
  }

  /* Wait until channel is actually playing (as original did) */
  while (!ndspChnIsPlaying(channel) && !info->stopFlag && !stopNow)
    svcSleepThread(10 * 1000);

  while (!info->stopFlag && !stopNow) {
    svcSleepThread(100 * 1000);

    /* When both buffers marked done after lastbuf, exit loop */
    if (lastbuf && waveBuf[0].status == NDSP_WBUF_DONE &&
        waveBuf[1].status == NDSP_WBUF_DONE)
      break;

    if (ndspChnIsPaused(channel) || lastbuf)
      continue;

    /* Refill whichever buffer is done */
    if (waveBuf[0].status == NDSP_WBUF_DONE) {
      size_t read = decoder.decode(decoderState, (void*)buffer1);
      /* accumulate played samples exactly like original */
      info->samples_played += waveBuf[0].nsamples * (size_t)channels;

      if (read == 0) {
        lastbuf = true;
      } else {
        if (read < totalSamplesPerBuf)
          waveBuf[0].nsamples = (u32)(read / channels);
        /* Flush and queue */
        DSP_FlushDataCache(buffer1, totalSamplesPerBuf * sizeof(int16_t));
        ndspChnWaveBufAdd(channel, &waveBuf[0]);
      }
    }

    if (waveBuf[1].status == NDSP_WBUF_DONE) {
      size_t read = decoder.decode(decoderState, (void*)buffer2);
      info->samples_played += waveBuf[1].nsamples * (size_t)channels;

      if (read == 0) {
        lastbuf = true;
      } else {
        if (read < totalSamplesPerBuf)
          waveBuf[1].nsamples = (u32)(read / channels);
        DSP_FlushDataCache(buffer2, totalSamplesPerBuf * sizeof(int16_t));
        ndspChnWaveBufAdd(channel, &waveBuf[1]);
      }
    }
  }

  info->samples_played += waveBuf[0].nsamples * (size_t)channels;
  info->samples_played += waveBuf[1].nsamples * (size_t)channels;

cleanup:

  activeInfo[channel] = NULL;

  ndspChnWaveBufClear(channel);
  ndspChnReset(channel);
  releaseChannel(channel);

  if (decoder.exit && decoderState)
    decoder.exit(decoderState);

  if (buffer1)
    linearFree(buffer1);
  if (buffer2)
    linearFree(buffer2);

  /* Signal watchdog */
  if (info && info->errInfo) {
    if (info->errInfo->error)
      *(info->errInfo->error) = stopNow ? -1 : errno;
    if (info->errInfo->failEvent)
      svcSignalEvent(*(info->errInfo->failEvent));
  }
  
  activeInfo[channel] = NULL;
  threadExit(0);
}


//Ogga Booga DJ... WoOOoo!
void scratchDisk(int channel, float skipSeconds) {
  
  if (channel < 0 || channel >= MAX_CHANNELS) return;
  
  playbackInfo_t* info = activeInfo[channel];
  if (!info) return;
  
  graphicalPrintf("skipSeconds=%.3f, sample_rate=%u, skipSamples=%lld", 
               skipSeconds, info->samples_per_second, skipSeconds);
               
    // Convert seconds to samples
    long long skipSamples = (long long)(skipSeconds * info->samples_per_second);
    long long newPos = (long long)info->samples_played + skipSamples;

    // Clamp inside valid sample range
    if (newPos < 0) newPos = 0;
    if (info->samples_total > 0 && newPos > info->samples_total)
        newPos = info->samples_total;

    info->samples_played = (size_t)newPos;

    // Reset NDSP to pick up new position
    ndspChnWaveBufClear(channel);
    //ndspChnReset(channel);

    graphicalPrintf("Skipped %.2f seconds (newPos=%lld)", skipSeconds, newPos);

    // Tiny delay to avoid NDSP glitches
    svcSleepThread(1000);
}