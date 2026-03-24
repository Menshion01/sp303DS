#ifndef UTIL_H
#define UTIL_H

#include <3ds.h>
#include <citro2d.h>

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#define BOT_SCREEN_WIDTH 320
#define BOT_SCREEN_HEIGHT 240

// Declare colors
extern u32 clrWhite;
extern u32 clrGray;
extern u32 clrBlack;
extern u32 clrPink;
extern u32 clrGreen;
extern u32 clrRed;

void initiate();
void initColours();
void fancyRect(int x, int y, int width, int height, u32 colour);

#endif