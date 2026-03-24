#include "dial.h"

float totalAngle;
float aditionAngle;
int lastAngle;
bool touchingDial;

int range360(int x) {
    return (x + 360) % 360;
}

float dialEncoder() {
    int angle = range360(atan2(touch.py - SCREEN_HEIGHT / 2, touch.px - SCREEN_WIDTH/2) * 180/M_PI);
    const float speed = 1;

    if (touch.px && touch.py) {

        if (!touchingDial) lastAngle = angle;

        if (angle < 0) angle += 360;
        float delta = angle - lastAngle;

        if (delta > 180)  delta -= 360;
        if (delta < -180) delta += 360;

        totalAngle += delta * speed;
        lastAngle = angle;
        touchingDial = true;
    } else touchingDial = false;
    return totalAngle;
}

float DJEncoder() {
    int angle = range360(atan2(touch.py - SCREEN_HEIGHT / 2, touch.px - SCREEN_WIDTH/2) * 180/M_PI);
    const float speed = 1;

    if (touch.px && touch.py) {

        if (!touchingDial) lastAngle = angle;

        if (angle < 0) angle += 360;
        float delta = angle - lastAngle;

        if (delta > 180)  delta -= 360;
        if (delta < -180) delta += 360;

        lastAngle = angle;
        touchingDial = true;
        return delta * speed;
    } else {
        touchingDial = false;
    }
    return 0;
}

float brokenDial() {
    int angle = range360(atan2(touch.py - SCREEN_HEIGHT / 2, touch.px - SCREEN_WIDTH/2) * 180/M_PI);
    const float speed = 0.2;

    if (touch.px && touch.py) {

        if (!touchingDial) lastAngle = angle;
        aditionAngle = (angle-lastAngle) * speed;

        lastAngle = angle;
        touchingDial = true;
    } else touchingDial = false;
    
    totalAngle += aditionAngle;
    aditionAngle = 0;
    return totalAngle;
}

// This one is like a joystick, a little circle inside moves with the finger.
// void drawDials() {
//     char output[1024];
//     dialEncoder();
//     const int radius = 100;
//     const int dialPosition[] = {BOT_SCREEN_WIDTH/2, BOT_SCREEN_HEIGHT/2};

//     snprintf(output, sizeof(output), "sum: %.1f,\ninital: %i", totalAngle, lastAngle);
//     updateText(output, 0);
//     C2D_DrawCircleSolid(dialPosition[0], dialPosition[1], 0, 100, clrWhite);
//     C2D_DrawCircleSolid(dialPosition[0], SCREEN_HEIGHT/2, 0, 80, clrGreen);
//     int fingerPosition[] = {touch.px, touch.py};
//     if (!touch.px && !touch.py) fingerPosition[0] = dialPosition[0], fingerPosition[1]= dialPosition[1]; // If not touching put circle in centre
    
//     if (fingerPosition[0] < (dialPosition[0]-radius/2)) fingerPosition[0] = dialPosition[0]-radius/2;
//     else if (fingerPosition[0] > (dialPosition[0]+radius/2)) fingerPosition[0] = dialPosition[0]+radius/2;
//     if (fingerPosition[1] < (dialPosition[1]-radius/2)) fingerPosition[1] = dialPosition[1]-radius/2;
//     else if (fingerPosition[1] > (dialPosition[1]+radius/2)) fingerPosition[1] = dialPosition[1]+radius/2;

//     C2D_DrawCircleSolid(fingerPosition[0], fingerPosition[1], 0, 100, clrWhite);
// }

void drawDials() {
    totalAngle = DJEncoder();
    char output[stringLength];
    //const int radius = 100;

    //Update the debug text
    const int dialPosition[] = {BOT_SCREEN_WIDTH/2, BOT_SCREEN_HEIGHT/2};
    snprintf(output, sizeof(output), "sum: %.1f,\ninital: %i", totalAngle, lastAngle);
    updateText(output, dialTextIndex, 0, 0);
    if (totalAngle) scratchDisk(0, totalAngle/15);
    //scratchDisk(0, totalAngle/15);
    
    //This draws the circle, later will be a picture
    C2D_DrawCircleSolid(dialPosition[0], dialPosition[1], 0, 100, clrWhite);
}