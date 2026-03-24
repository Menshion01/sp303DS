#ifndef TEXT_H
#define TEXT_H

#include <citro2D.h>
#include <stdarg.h>

// extern float size;
// extern C2D_TextBuf g_staticBuf;
extern C2D_Font font[2];

#define amountOfStrings 256 // Set the amount of strings here
#define stringLength    64 // How long are the Text Strings? (Include the null-terminator as one extra)
#define maxConsoleLines 13 // Max console lines before needing to rewrite
#define topScreenPadding (SCREEN_WIDTH - BOT_SCREEN_WIDTH) / 2

extern C2D_TextBuf text_buf[amountOfStrings];
extern C2D_Text g_text[amountOfStrings];
extern int textIndex;
extern int textX[amountOfStrings];
extern int textY[amountOfStrings];
extern float textSize[amountOfStrings];
extern int textIndex;
extern int curLine;

void initText();

int createText(char *inputText, int x, int y, float size, int specificIndex);
void updateText(char *inputText, int localTextIndex, int shiftX, int shiftY);
int graphicalPrintf(char *inputText, ...);
void renderTextBottom();
void renderTextTop();

void graphicalConsoleClear();

#endif