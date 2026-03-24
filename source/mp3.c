#include <mpg123.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mp3.h"
#include "playback.h"

static size_t DEFAULT_BUFF_MULT = 16;

/* Reference count for mpg123 init/exit */
static int mpg123_refcount = 0;

struct Mp3State {
  mpg123_handle* mh;
  size_t buffSize;
  uint32_t rate;
  uint8_t channels;
};

static void* initMp3(const char* file, size_t* buffSizeOut) {
  int err = 0;
  int encoding = 0;
  mpg123_handle* mh = NULL;

  if (mpg123_refcount == 0) {
    if (mpg123_init() != MPG123_OK)
      return NULL;
  }
  mpg123_refcount++;

  mh = mpg123_new(NULL, &err);
  if (mh == NULL) {
    mpg123_refcount--;
    if (mpg123_refcount == 0)
      mpg123_exit();
    return NULL;
  }

  if (mpg123_open(mh, file) != MPG123_OK ||
      mpg123_getformat(mh, (long*)&((long){0}), (int*)&((int){0}), &encoding) !=
          MPG123_OK) {
    /* the simple call above isn't helpful; instead query format properly */
    long rate = 0;
    int channels = 0;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
      mpg123_close(mh);
      mpg123_delete(mh);
      mpg123_refcount--;
      if (mpg123_refcount == 0)
        mpg123_exit();
      return NULL;
    }
  }

  /* Now get real format */
  long lr = 0;
  int ch = 0;
  if (mpg123_getformat(mh, &lr, &ch, &encoding) != MPG123_OK) {
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_refcount--;
    if (mpg123_refcount == 0)
      mpg123_exit();
    return NULL;
  }

  struct Mp3State* st = malloc(sizeof(struct Mp3State));
  if (!st) {
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_refcount--;
    if (mpg123_refcount == 0)
      mpg123_exit();
    return NULL;
  }

  st->mh = mh;
  st->rate = (uint32_t)lr;
  st->channels = (uint8_t)ch;

  /* Force format (keep it consistent) */
  mpg123_format_none(mh);
  mpg123_format(mh, st->rate, st->channels, MPG123_ENC_SIGNED_16);

  st->buffSize = mpg123_outblock(mh) * DEFAULT_BUFF_MULT;
  if (buffSizeOut)
    *buffSizeOut = st->buffSize;

  return st;
}

static uint32_t rateMp3(void* state) {
  struct Mp3State* st = (struct Mp3State*)state;
  return st->rate;
}

static uint8_t channelMp3(void* state) {
  struct Mp3State* st = (struct Mp3State*)state;
  return st->channels;
}

static uint64_t decodeMp3(void* state, void* buffer) {
  struct Mp3State* st = (struct Mp3State*)state;
  size_t done = 0;
  int err = mpg123_read(st->mh, buffer, st->buffSize, &done);
  if (err != MPG123_OK && err != MPG123_DONE) {
    return 0;
  }
  /* done is bytes, convert to samples (int16_t) */
  return done / sizeof(int16_t);
}

static void exitMp3(void* state) {
  struct Mp3State* st = (struct Mp3State*)state;
  if (!st)
    return;
  if (st->mh) {
    mpg123_close(st->mh);
    mpg123_delete(st->mh);
  }
  free(st);

  mpg123_refcount--;
  if (mpg123_refcount == 0)
    mpg123_exit();
}

static size_t getFileSamplesMp3(void* state) {
  struct Mp3State* st = (struct Mp3State*)state;
  off_t len = mpg123_length(st->mh);
  if (len == MPG123_ERR)
    return 0;
  return len * (size_t)st->channels;
}

/**
 * Set decoder parameters for MP3.
 */
void setMp3(struct decoder_fn* decoder) {
  decoder->init = &initMp3;
  decoder->rate = &rateMp3;
  decoder->channels = &channelMp3;
  decoder->buffSize = 0; /* set by init */
  decoder->decode = &decodeMp3;
  decoder->exit = &exitMp3;
  decoder->getFileSamples = &getFileSamplesMp3;
}

/* Basic isMp3 using small header sniff (kept as original) */
int isMp3(const char* path) {
  unsigned char buf[4];
  FILE* f = fopen(path, "rb");
  int ret = 1;

  if (!f)
    return 1;

  if (fread(buf, 1, 4, f) < 4)
    goto out;

  if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3') {
    ret = 0;
    goto out;
  }

  if (buf[0] == 0xFF && (buf[1] & 0xE0) == 0xE0) {
    ret = 0;
    goto out;
  }

out:
  fclose(f);
  return ret;
}
