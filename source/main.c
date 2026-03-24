/**
 * ctrmus - 3DS Music Player
 * Copyright (C) 2016 Mahyar Koshkouei
 *
 * This program comes with ABSOLUTELY NO WARRANTY and is free software. You are
 * welcome to redistribute it under certain conditions; for details see the
 * LICENSE file.
 */

#include "main.h"

C3D_RenderTarget* bot = NULL;  // Defined at launch
C3D_RenderTarget* top = NULL;
u32 kDown, kHeld, kUp;

int main() {
  initiate();
  initColours();
  initText();

  addUI();

  getDir(&dirList);
  listDir(from, MAX_LIST, 0, dirList);

  while (aptMainLoop()) {
	updateTouch();  // hidScanInput() is in this
  
  C2D_DrawCircleSolid(0, 0, 0, 0, 0); //Don't ask questions.
	render();

	kDown = hidKeysDown();
	kHeld = hidKeysHeld();
	kUp = hidKeysUp();

	if (kDown)
	keyPress();  // Button Pressed

	if (kDown & KEY_START)
	goto out;  // Exit on start press

	gspWaitForVBlank();
  }

out:
  graphicalPrintf("Exiting...");
  runThreads = false;
  svcSignalEvent(playbackFailEvent);

  // Stop all active playback threads
  for (int i = 0; i < MAX_PLAYBACKS; i++) {
	if (threads[i]) {
		stopPlayback(&playbacks[i]);
		threadJoin(threads[i], U64_MAX);
		threadFree(threads[i]);
		threads[i] = NULL;
	}
  }

  C2D_FontFree(font[0]);
  C2D_FontFree(font[1]);

  // Clean up resources
  ndspExit();
  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}