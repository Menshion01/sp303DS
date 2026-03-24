#include "util.h"
#include "main.h"
#include "text.h"

u32 clrWhite;
u32 clrGray;
u32 clrBlack;
u32 clrPink;
u32 clrGreen;
u32 clrRed;

void initiate() {
  romfsInit();
  cfguInit();  // Allow C2D_FontLoadSystem to work
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //Sound stuff below
  ndspInit();
  ndspSetOutputMode(NDSP_OUTPUT_STEREO);

  // Create screen(s) and assign it to the global variables
  C3D_RenderTarget* initBot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  bot = initBot;
  C3D_RenderTarget* initTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  top = initTop;

  // addUI();

  svcCreateEvent(&playbackFailEvent, RESET_ONESHOT);
  errInfo.error = &error;
  errInfo.failEvent = &playbackFailEvent;

  // watchdogInfoIn.screen = &topScreenLog;
  watchdogInfoIn.errInfo = &errInfo;
  watchdogThread =
      threadCreate(playbackWatchdog, &watchdogInfoIn, 4 * 1024, 0x20, -2, true);

  playbackInfo.errInfo = &errInfo;

  chdir(DEFAULT_DIR);
  chdir("MUSIC");

  fileMax = getNumberFiles();

  /**
   * This allows for music to continue playing through the headphones whilst
   * the 3DS is closed.
   */
  aptSetSleepAllowed(false);
}

void initColours() {
  clrWhite = C2D_Color32(255, 255, 255, 255);
  clrBlack = C2D_Color32(49, 49, 49, 255);
  clrGray = C2D_Color32(69, 69, 69, 255);
  clrPink = C2D_Color32(255, 189, 189, 255);
  clrGreen = C2D_Color32(4, 151, 64, 255);
  clrRed = C2D_Color32(255, 16, 16, 255);
}

void fancyRect(int x, int y, int width, int height, u32 colour) {
  // if (y < SCREEN_HEIGHT) {
  //   // C2D_SceneBegin(top);
  //   C2D_DrawRectSolid(x + topScreenPadding, y, 0, width, height, colour);
  // } else {
  //   // C2D_SceneBegin(bot);
  //   C2D_DrawRectSolid(x, y - SCREEN_HEIGHT, 0, width, height, colour);
  // }
  C2D_DrawRectSolid(x, y, 0, width, height, colour);
}