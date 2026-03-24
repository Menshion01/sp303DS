#include "fileUtil.h"

/**
 * Allows the playback thread to return any error messages that it may
 * encounter.
 *
 * \param	infoIn	Struct containing addresses of the event, the error
 * code, and an optional error string.
 */

playbackInfo_t currentPlayback = {0};

volatile bool runThreads = true;
Thread threads[MAX_PLAYBACKS] = {0};
playbackInfo_t playbacks[MAX_PLAYBACKS] = {0};

bool selectButtonMode = false;
int fileMax;
int fileNum = 0;
int from = 0;
Thread watchdogThread;
Handle playbackFailEvent;
struct watchdogInfo watchdogInfoIn;
struct errInfo_t errInfo;
struct playbackInfo_t playbackInfo = {0};
volatile int error = 0;
struct dirList_t dirList = {0};
u64 mill = 0;

/* ignore key release of L/R if L+R or L+down was pressed */
bool keyLComboPressed = false;
bool keyRComboPressed = false;
/* position of parent folder in parent directory */
int prevPosition[MAX_DIRECTORIES] = {0};
int prevFrom[MAX_DIRECTORIES] = {0};
int oldFileNum, oldFrom;

void playbackWatchdog(void* infoIn) {
  struct watchdogInfo* info = infoIn;

  while (runThreads) {
    svcWaitSynchronization(*info->errInfo->failEvent, U64_MAX);
    svcClearEvent(*info->errInfo->failEvent);

    if (*info->errInfo->error > 0) {
      //continue;;
      graphicalPrintf("Error %d: %s\n", *info->errInfo->error,
                      ctrmus_strerror(*info->errInfo->error));
    } else if (*info->errInfo->error == -1) {
      //continue;;

      graphicalPrintf("Stopped");
    }
  }

  return;
}

/**
 * Stop the currently playing file (if there is one) and play another file.
 *
 * \param	ep_file			File to play.
 * \param	playbackInfo	Information that the playback thread requires to
 *							play file.
 */
int changeFile(const char* ep_file, struct playbackInfo_t* templateInfo) {
  s32 prio;
  int slot = -1;

  // // Only play actual files, not directories
  // if (fileNum <= dirList.dirNum) {
  //   graphicalPrintf("[INFO] Not a playable file.");
  //   return 0;
  // }

  // Find a free playback slot
  for (int i = 0; i < MAX_PLAYBACKS; i++) {
    if (!threads[i]) {
      slot = i;
      break;
    }
  }

  static int lastUsedSlot = 0;
  if (slot < 0) {
    slot = lastUsedSlot % MAX_PLAYBACKS;
    lastUsedSlot = (lastUsedSlot + 1) % MAX_PLAYBACKS;

    graphicalPrintf("[WARN] Reusing slot %d\n", slot);

    if (threads[slot]) {
      stopPlayback(&playbacks[slot]);
      threadJoin(threads[slot], U64_MAX);
      threadFree(threads[slot]);
      threads[slot] = NULL;
    }
  }

  if (!ep_file)
    return 0;

  enum file_types ft = getFileType(ep_file);
  if (ft == FILE_TYPE_ERROR) {
    *templateInfo->errInfo->error = errno;
    svcSignalEvent(*templateInfo->errInfo->failEvent);
    return -1;
  }

  // Setup new playback info
  playbackInfo_t* info = &playbacks[slot];
  memset(info, 0, sizeof(*info));
  info->errInfo = templateInfo->errInfo;

  if (strlen(ep_file) >= sizeof(info->file)) {
    graphicalPrintf("[ERROR] File path too long");
    return -1;
  }
  strncpy(info->file, ep_file, sizeof(info->file) - 1);
  info->file[sizeof(info->file) - 1] = '\0';

  svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

  threads[slot] = threadCreate(playFile, info, 32 * 1024, prio - 1, -2, false);

  if (!threads[slot]) {
    graphicalPrintf("[ERROR] Failed to create playback thread");
    errno = ENOMEM;
    return -1;
  }

  graphicalPrintf("[INFO] Started playback %d: %s\n", slot, info->file);
  return 0;
}

static int cmpstringp(const void* p1, const void* p2) {
  /* Arguments to this function are "pointers to
     pointers to char", but strcmp(3) arguments are "pointers
     to char", hence the following cast plus dereference */

  return strcasecmp(*(char* const*)p1, *(char* const*)p2);
}

// Store the list of files and folders in current director to an array.
int getDir(struct dirList_t* dirList) {
  DIR* dp;
  struct dirent* ep;
  int fileNum = 0;
  int dirNum = 0;
  char* wd = getcwd(NULL, 0);

  if (wd == NULL)
    goto out;

  /* Clear strings */
  for (int i = 0; i < dirList->dirNum; i++)
    free(dirList->directories[i]);

  for (int i = 0; i < dirList->fileNum; i++)
    free(dirList->files[i]);

  free(dirList->currentDir);

  if ((dirList->currentDir = strdup(wd)) == NULL)
    graphicalPrintf("Failure");

  if ((dp = opendir(wd)) == NULL)
    goto out;

  while ((ep = readdir(dp)) != NULL) {
    //Skip Hidden Files
    if (ep->d_name[0] == '.')
      continue;

    if (ep->d_type == DT_DIR) {
      /* Add more space for another pointer to a dirent struct */
      dirList->directories =
          realloc(dirList->directories, (dirNum + 1) * sizeof(char*));

      if ((dirList->directories[dirNum] = strdup(ep->d_name)) == NULL)
        graphicalPrintf("Failure");

      dirNum++;
      continue;
    }

    /* Add more space for another pointer to a dirent struct */
    dirList->files = realloc(dirList->files, (fileNum + 1) * sizeof(char*));

    if ((dirList->files[fileNum] = strdup(ep->d_name)) == NULL)
      graphicalPrintf("Failure");

    fileNum++;
  }

  qsort(&dirList->files[0], fileNum, sizeof(char*), cmpstringp);
  qsort(&dirList->directories[0], dirNum, sizeof(char*), cmpstringp);

  dirList->dirNum = dirNum;
  dirList->fileNum = fileNum;

  if (closedir(dp) != 0)
    goto out;

out:
  free(wd);
  return fileNum + dirNum;
}

/**
 * List current directory.
 *
 * \param	from	First entry in directory to list.
 * \param	max		Maximum number of entries to list. Must be > 0.
 * \param	select	File to show as selected. Must be > 0.
 * \return			Number of entries listed or negative on error.
 */
// is suppost to be static lol
int listDir(int from, int max, int select, struct dirList_t dirList) {
  int fileNum = 0;
  int listed = 0;

  graphicalPrintf("Dir: %.33s\n", dirList.currentDir);

  if (from == 0) {
    graphicalPrintf("%c../\n", select == 0 ? '>' : ' ');
    listed++;
    max--;
  }

  while (dirList.fileNum + dirList.dirNum > fileNum) {
    fileNum++;

    if (fileNum <= from)
      //continue;;

    listed++;

    if (dirList.dirNum >= fileNum) {
      graphicalPrintf("%c%.37s\n", select == fileNum ? '>' : ' ',
                      dirList.directories[fileNum - 1]);
    }

    /* fileNum must be referring to a file instead of a directory. */
    if (dirList.dirNum < fileNum) {
      graphicalPrintf("%c%.37s\n", select == fileNum ? '>' : ' ',
                      dirList.files[fileNum - dirList.dirNum - 1]);
    }

    if (fileNum == max + from)
      break;
  }

  return listed;
}

/**
 * Get number of files in current working folder
 *
 * \return	Number of files in current working folder, -1 on failure with
 *			errno set.
 */
int getNumberFiles(void) {
  DIR* dp;
  struct dirent* ep;
  int ret = 0;

  if ((dp = opendir(".")) == NULL)
    goto err;

  while ((ep = readdir(dp)) != NULL)
    ret++;

  closedir(dp);

out:
  return ret;

err:
  ret = -1;
  goto out;
}


bool keyPress() {
  // if (buttonPressed & KEY_DDOWN) {
  //     padDown = true;
  //     return 1;
  // }
  // if (buttonPressed & KEY_DUP) {
  //     padDown = false;
  //     return 1;
  // }

  if (kDown)
    mill = osGetTime();

  if (kHeld & KEY_L) {
    /* Pause/Play */
    if (kDown & (KEY_R | KEY_UP)) {
      if (isPlaying(&currentPlayback) == false) {
      }
      // continue;

      if (togglePlayback(&playbackInfo) == true)
        graphicalPrintf("Paused");
      else
        graphicalPrintf("Playing");

      keyLComboPressed = true;
      // distinguish between L+R and L+Up
      if (KEY_R & kDown) {
        keyRComboPressed = true;
      }
      // continue;
    }
  }

  if ((kDown & KEY_UP || ((kHeld & KEY_UP) && (osGetTime() - mill > 500))) &&
      fileNum > 0) {
    graphicalConsoleClear();
    fileNum--;

    // one line taken up by cwd, other by ../
    if (fileMax - fileNum > MAX_LIST - 2 && from != 0)
      from--;

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // graphicalPrintf("Unable to list directory.");
  }

  if ((kDown & KEY_DOWN ||
       ((kHeld & KEY_DOWN) && (osGetTime() - mill > 500))) &&
      fileNum < fileMax) {
    graphicalConsoleClear();
    fileNum++;

    if (fileNum >= MAX_LIST && fileMax - fileNum >= 0 &&
        from < fileMax - MAX_LIST)
      from++;

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // graphicalPrintf("Unable to list directory.");
  }

  if ((kDown & KEY_LEFT ||
       ((kHeld & KEY_LEFT) && (osGetTime() - mill > 500))) &&
      fileNum > 0) {
    graphicalConsoleClear();
    int skip = MAX_LIST / 2;

    if (fileNum < skip)
      skip = fileNum;

    fileNum -= skip;

    // one line taken up by cwd, other by ../
    if (fileMax - fileNum > MAX_LIST - 2 && from != 0) {
      from -= skip;
      if (from < 0)
        from = 0;
    }

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // graphicalPrintf("Unable to list directory.");
  }

  if ((kDown & KEY_RIGHT ||
       ((kHeld & KEY_RIGHT) && (osGetTime() - mill > 500))) &&
      fileNum < fileMax) {
    graphicalConsoleClear();
    int skip = fileMax - fileNum;

    if (skip > MAX_LIST / 2)
      skip = MAX_LIST / 2;

    fileNum += skip;

    if (fileNum >= MAX_LIST && fileMax - fileNum >= 0 &&
        from < fileMax - MAX_LIST) {
      from += skip;
      if (from > fileMax - MAX_LIST)
        from = fileMax - MAX_LIST;
    }

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // graphicalPrintf("Unable to list directory.");
  }

  /*
   * Pressing B goes up a folder, as well as pressing A or R when ".."
   * is selected.
   */
  if ((kDown & KEY_B) || ((kDown & KEY_A) && (from == 0 && fileNum == 0))) {
    chdir("..");
    graphicalConsoleClear();
    fileMax = getDir(&dirList);

    fileNum = prevPosition[0];
    from = prevFrom[0];
    for (int i = 0; i < MAX_DIRECTORIES - 1; i++) {
      prevPosition[i] = prevPosition[i + 1];
      prevFrom[i] = prevFrom[i + 1];
    }
    /* default to first entry */
    prevPosition[MAX_DIRECTORIES - 1] = 0;
    prevFrom[MAX_DIRECTORIES - 1] = 0;

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // graphicalPrintf("Unable to list directory.");

    // continue;
  }

  if (kDown & KEY_A) {
    if (dirList.dirNum >= fileNum) {
      chdir(dirList.directories[fileNum - 1]);
      graphicalConsoleClear();
      fileMax = getDir(&dirList);

      oldFileNum = fileNum;
      oldFrom = from;
      fileNum = 0;
      from = 0;

      if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
        // graphicalPrintf("Unable to list directory.");
      } else {
        /* save old position in folder */
        for (int i = MAX_DIRECTORIES - 1; i > 0; i--) {
          prevPosition[i] = prevPosition[i - 1];
          prevFrom[i] = prevFrom[i - 1];
        }
        prevPosition[0] = oldFileNum;
        prevFrom[0] = oldFrom;
      }
      // continue;
    }

    if (dirList.dirNum < fileNum) {
      graphicalConsoleClear(); //This one happens when A is presssed

      //Select a button on bottom screen
      selectButtonMode = true;
      graphicalPrintf("Pick a Pad to assign to.");

      error = 0;
      // continue;
    }
  }

  // ignore R release if key combo with R used
  bool keyRActivation = false;
  if (kUp & KEY_R) {
    if (!keyRComboPressed) {
      keyRActivation = true;
    }
    keyRComboPressed = false;
  }
  bool goToNextFile = (kDown & KEY_ZR) || keyRActivation;
  if (goToNextFile && fileNum < fileMax) {

    // This stops all sounds now frfr
    for (int i = 0; i < MAX_PLAYBACKS; i++) {
      stopPlayback(&playbacks[i]);
    }

    fileNum += 1;
    if (fileNum >= MAX_LIST && fileMax - fileNum >= 0 &&
        from < fileMax - MAX_LIST)
      from++;


    graphicalConsoleClear();

    changeFile(dirList.files[fileNum - dirList.dirNum - 1], &playbackInfo);
    error = 0;

    if (listDir(from, MAX_LIST, fileNum, dirList) < 0) {
    }
    // err_print("Unable to list directory.");
    // continue;
  }
  // ignore L release if key combo with L used
  bool keyLActivation = false;
  if (kUp & KEY_L) {
    if (!keyLComboPressed) {
      keyLActivation = true;
    }
    keyLComboPressed = false;
  }
  bool stopAllSounds = (kDown & KEY_ZL) || keyLActivation;
  // don't go to ../
  if (stopAllSounds) {
    //This stops all sounds now frfr
    for (int i = 0; i < MAX_PLAYBACKS; i++) {
      stopPlayback(&playbacks[i]);
    }
  }

  return 0;
}