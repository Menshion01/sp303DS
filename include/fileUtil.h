#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include "main.h"

extern playbackInfo_t currentPlayback;

extern volatile bool runThreads;
extern Thread threads[MAX_PLAYBACKS];
extern playbackInfo_t playbacks[MAX_PLAYBACKS];

extern bool selectButtonMode;
extern int fileMax;
extern int fileNum;
extern int from;
extern Thread watchdogThread;
extern Handle playbackFailEvent;
extern struct watchdogInfo watchdogInfoIn;
extern struct errInfo_t errInfo;
extern struct playbackInfo_t playbackInfo;
extern volatile int error;
extern struct dirList_t dirList;
extern u64 mill;
extern bool keyLComboPressed;
extern bool keyRComboPressed;
extern int prevPosition[MAX_DIRECTORIES];
extern int prevFrom[MAX_DIRECTORIES];
extern int oldFileNum, oldFrom;

void playbackWatchdog(void *infoIn);
int listDir(int from, int max, int select, struct dirList_t dirList);
int getDir(struct dirList_t *dirList);
int changeFile(const char *ep_file, struct playbackInfo_t *templateInfo);
bool keyPress();
int getNumberFiles(void);

#endif