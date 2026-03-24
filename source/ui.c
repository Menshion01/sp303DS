#include "ui.h"
#include "text.h"
#include "touch.h"
#include "util.h"
#include "dial.h"

bool DJMode = false; //Currently turns dial on or off
//bool padDown = false;
char* stringNum[] = {"1",      "2",    "3",       "4",      "5",     "6",
                     "7",      "8",    "cancel",  "remain", "lo-fi",
                     "gate",   "loop", "reverse", "HOLD", "EXT",   "del",   "rec",
                     "sample", "mark", "A",       "B",      "C",     "D"};
char* soundBuffers[32]; // 8 x 4 buffers
int currentBank = 0; // starting from 0, soundbuffer locations

// addUI only runs once in Main
void addUI() {
  createText("", 10, 10, 1.0, dialTextIndex); //Create Dial Debug Text
}

char* modify(int bufferIndex) {

  // Get directory of currently playing sound
  char soundDir[PATH_MAX];
  static char location[PATH_MAX];
  strcpy(soundDir, soundBuffers[bufferIndex]);
  *strrchr(soundDir, '/') = '\0';

  // Create a 'modified' folder
  char modifiedPath[PATH_MAX];
  snprintf(modifiedPath, sizeof(modifiedPath), "%s/modified", soundDir);
  mkdir(modifiedPath, 0777);

  copyFileToDir(soundBuffers[bufferIndex], modifiedPath);
  
  snprintf(location, sizeof(location), "%s/%s", soundDir, strrchr(soundBuffers[bufferIndex], '/') + 1);

  graphicalPrintf("Modified at %s", location);

  return location;
}

int padPress(int i, int j) {
  int x = i * BOT_SCREEN_WIDTH / 4;
  int y = 80 + j * 80;
  int index = (i + (4 * j)) + currentBank * 8;
  const char* relPath = dirList.files[fileNum - dirList.dirNum - 1];
  fancyRect(x, y, 80, 80, clrGreen);
  float pitchScale = 1;

  // [DEBUG] set first buffer to a sound
  soundBuffers[0] = "sdmc:/3ds/3DSoundboard/radio.mp3";

  // [DEBUG] Force pitch adjustment very interesting indeed
  //channelPitch[index - currentBank * 8] = powf(2.0f, pitchScale / 12.0f);

  // Prevent out-of-range index
  if ((index < 0 || index >= MAX_SOUND_BUFFERS) || (currentBank >= BANK_COUNT) || (!relPath)) {
    graphicalPrintf("Files Coocked %d, %s", index, relPath);
    return -1;
  }

  if (selectButtonMode) {
    char absPath[PATH_MAX];
    snprintf(absPath, sizeof(absPath), "%s/%s", dirList.currentDir, relPath);

    if (soundBuffers[index]) {
      free(soundBuffers[index]);
      soundBuffers[index] = NULL;
    }
    soundBuffers[index] = strdup(absPath);
    selectButtonMode = false;

    graphicalPrintf("Set buffer[%d]: %s", index, soundBuffers[index]);
    
    //If the file has been modified duplicate save this version in the modified folder
    if (pitchScale != 1) {
      modify(index);
    }
    
    return 0;
  }

  //If button was pressed to play it, play it
  if (soundBuffers[index] && soundBuffers[index][0] != '\0') {
    graphicalPrintf("Playing: %s\n", soundBuffers[index]);

    if (!soundBuffers[index] || strlen(soundBuffers[index]) >= PATH_MAX) {
      graphicalPrintf("Invalid path pointer or overflow\n");
      return -1;
    }
    changeFile(soundBuffers[index], &playbackInfo);
    return 0;
  }
  return -1;
}

void bankPress(int i, int j) {
  int x = i * BOT_SCREEN_WIDTH / 8;
  int y = j * 40;
  //int index = (9 + i + j * 8);

  //Bank button switching
  if (j == 1 && i > 3) {
    currentBank = i - 4; //Ensure counting from 0
    fancyRect(x, y, 40, 40, clrGreen);  // Draw Green Square
  }
  
  // cancel to stop all sounds
  if (j==0 && i==0) {
    //  This stops all sounds now frfr
    for (int i = 0; i < MAX_PLAYBACKS; i++) {
      stopPlayback(&playbacks[i]);
    }

    fancyRect(x, y, 40, 40, clrGreen);  // Draw Green Square
  }
  // Enable DJMode when pressing ext
  if (i==7 && j==0) {
    DJMode = !DJMode;
    updateText("", dialTextIndex, 0, 0); //Clear the debug text

    fancyRect(x, y, 40, 40, clrGreen);  // Draw Green Square
  }
}

void crashPad() {
  int i = 0;
  int j = 0;

  // make pad buttons
  for (j = 0; j < 2; j++) {
    for (i = 0; i < 4; i++) {
      int x = i * BOT_SCREEN_WIDTH / 4;
      int y = 80 + j * 80; //padDown should be here
      u32 tmpColour = (selectButtonMode ? clrRed // red when its asking for it
        : ((i + j) % 2) ? clrWhite : clrPink);  // Otherwise, makes an alternating, checkerboard colour pattern
      int index = (i + 1 + (4 * j));

      fancyRect(x, y, 80, 80, tmpColour);
      createText(stringNum[index - 1], x + 31, y + 20 + SCREEN_HEIGHT, 1.0, index);

      if (!DJMode && collusion(0, 0, 80, 80, x, y))
        padPress(i, j);
    }
  }

  // make button buttons
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 2; j++) {
      int x = i * BOT_SCREEN_WIDTH / 8;
      int y = j * 40;
      u32 tmpColour = ((i + j) % 2) ? clrWhite : clrPink;
      int index = (9 + i + j * 8);

      fancyRect(x, y, 40, 40, tmpColour);
      createText(stringNum[index - 1], x + 5, y + 25 + SCREEN_HEIGHT, 0.45, index);

      if (collusion(0, 0, 40, 40, x, y))
        bankPress(i, j);  // Check for presses
    }
  }

  // Draw highlighted bank button
  //  Draw highlighted bank button
  fancyRect((currentBank + 4) * BOT_SCREEN_WIDTH / 8, 40, 40, 40, clrRed); // Draw Green Square
}

void bpm() {
  // fancyRect(15, 40, 100, 100, clrWhite);
  // createText("120", 20, 50, 2.0, 31);
}
