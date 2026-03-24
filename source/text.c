#include "main.h"
#include "text.h"
#include "touch.h"
#include "util.h"
#include "ui.h"

C2D_Font font[2];
C2D_TextBuf text_buf[amountOfStrings];
C2D_Text g_text[amountOfStrings];
int textX[amountOfStrings];
int textY[amountOfStrings];
float textSize[amountOfStrings];
int textIndex = 0;
int curLine = 0;
int i = 0;

/*
current text buffer usuage !!
0 - my signature i think,
0 - 32, exclusively for pad buttons
31 - (31+maxConsoleLines), exclusively for max console lines
*/

void initText() {
    font[0] = C2D_FontLoad("romfs:/liberationitalic.bcfnt");
    font[1] = C2D_FontLoad("romfs:/liberationitalic.bcfnt");
}

int createText(char *inputText, int x, int y, float size, int specificIndex) {
    int curIndex = 0;
    
    if (specificIndex) {
        curIndex = specificIndex;
        if (text_buf[curIndex] != NULL) {
            // Text at this specific index already exists
            updateText(inputText, specificIndex, 0, 0);
            return 0;
        }
    }
    else {
        curIndex = textIndex;
        textIndex++;
    }

    text_buf[curIndex] = C2D_TextBufNew(stringLength);
    C2D_TextBufClear(text_buf[curIndex]);
    C2D_TextFontParse(&g_text[curIndex], font[0], text_buf[curIndex], inputText);
    C2D_TextOptimize(&g_text[curIndex]);

    textX[curIndex] = x;
    textY[curIndex] = y;
    textSize[curIndex] = size;
    return 0;
}

void renderTextTop() {

    for (i = 0; i < (amountOfStrings); i++) {
        //if (i > 32-1 && i < 32+maxConsoleLines) continue; // Skip console lines;

        if (textY[i] < SCREEN_HEIGHT) {
            C2D_DrawText(&g_text[i], C2D_AtBaseline, textX[i], textY[i], 0, textSize[i], textSize[i]);
        }
    }
}

void renderTextBottom() {

    for (i = 0; i < (amountOfStrings); i++) {

        if (textY[i] > SCREEN_HEIGHT) {
            C2D_DrawText(&g_text[i], C2D_AtBaseline, textX[i], (textY[i] - SCREEN_HEIGHT), 0, textSize[i], textSize[i]);
        }
    }
}

void updateText(char *inputText, int localTextIndex, int shiftX, int shiftY) {

    C2D_TextBufClear(text_buf[localTextIndex]);
    C2D_TextFontParse(&g_text[localTextIndex], font[0], text_buf[localTextIndex], inputText);
    C2D_TextOptimize(&g_text[localTextIndex]);

    textX[localTextIndex] += shiftX;
    textY[localTextIndex] += shiftY;
}

int graphicalPrintf(char *inputText, ...) {
    
    if (curLine >= maxConsoleLines) {
        //curLine = 0; //If lines overflow, put cursor at the top
        return 1;
    }

    int index = curLine + 32; // This uses the text engine above so that they work together for easy implementation
    char output[stringLength]; 
    va_list args;

    va_start(args, inputText);
    vsnprintf(output, sizeof(output), inputText, args); // format inputText into a string into ouput like printf
    va_end(args);

    createText(output, 0, 10 + curLine * 18, 0.45, index);
    textSize[index] = 0.45; // Undo the clear

    curLine++;
    
    return 0;
}

void graphicalConsoleClear() {

    for (int i = 0; i < maxConsoleLines; i++ ) {
        int index = i + 32;
        textSize[index] = 0; // Do this rather than clearing buffers
        //because my 3DS does not appreciate me resseting the text so often
    }
    curLine = 0;
}