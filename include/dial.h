#ifndef DIAL_H
#define DIAL_H

#include <citro2d.h>
#include "main.h"
#include "text.h"
#include "playback.h"

// extern float totalAngle;
// extern float aditionAngle;
// extern int lastAngle;
// extern bool touchingDial;

#define dialTextIndex 155

extern int range360(int x);
extern float dialEncoder();
extern float otherEncoder();
extern void drawDials();

extern playbackInfo_t* activeInfo[MAX_CHANNELS];

#endif