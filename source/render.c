#include <citro2d.h>

// #include "main.h"
// #include "time.h"
#include "ui.h"
#include "render.h"
#include "touch.h"
#include "util.h"
#include "text.h"
#include "dial.h"

void render() {
    // Initalise rendering
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    //top screen
    C2D_SceneBegin(top);
    C2D_TargetClear(top, clrGreen);
    //consoleRenderText();
    renderTextTop();
    
    // bottom screen
    C2D_SceneBegin(bot); // Render on bottom screen
    C2D_TargetClear(bot, clrGreen);

    crashPad();
    bpm();
    renderTextBottom();
    if (DJMode) drawDials();

    C3D_FrameEnd(0);
}