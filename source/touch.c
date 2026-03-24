#include "main.h"
#include "touch.h"

touchPosition touch;

void updateTouch() {
    hidScanInput();
    hidTouchRead(&touch);
}

// Point to rectangle collusion script
bool collusion(float x, float y, float width, float height, float ox, float oy) {
    u32 kDown = hidKeysDown();
    if(kDown & KEY_TOUCH) {
        // If specified as 0, default to mouse position
        if (!x) x = touch.px;
        if (!y) y = touch.py;
    
        if (ox <= x && x <= ox+width) {
    
            if (oy <= y && y <= oy + height) {
                return true;
            }
        }
    }
    return false;
}