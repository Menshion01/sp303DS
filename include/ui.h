#ifndef UI_H
#define UI_H

#include <citro2d.h>
#include <sys/stat.h>
#include "main.h"
#include "time.h"
#include "text.h"
#include "file.h"

#define BANK_COUNT 4
#define PADS_PER_BANK 8
#define MAX_SOUND_BUFFERS (BANK_COUNT * PADS_PER_BANK)

extern void addUI();
extern void crashPad();
extern void bpm();

extern char* soundBuffers[32];
extern int currentBank;
extern bool DJMode;

#endif