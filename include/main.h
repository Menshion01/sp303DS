/**
 * ctrmus - 3DS Music Player
 * Copyright (C) 2016 Mahyar Koshkouei
 *
 * This program comes with ABSOLUTELY NO WARRANTY and is free software. You are
 * welcome to redistribute it under certain conditions; for details see the
 * LICENSE file.
 */

#include <3ds.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <citro2d.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "all.h"
#include "error.h"
#include "file.h"
#include "playback.h"

#include "touch.h"
#include "util.h"
#include "text.h"
#include "ui.h"
#include "render.h"
#include "time.h"
#include "fileUtil.h"

#ifndef ctrmus_main_h
#define ctrmus_main_h

/* Default folder */
#define DEFAULT_DIR		"sdmc:/"

/* Maximum number of lines that can be displayed on bottom screen */
#define	MAX_LIST		16
/* Arbitrary cap for number of stored parent positions in folder to avoid
 * unbounded memory consumption. If directories are added exceeding this,
 * dequeues path closest to root to make space.
 */
#define MAX_DIRECTORIES 20
#define MAX_PLAYBACKS 16

extern C3D_RenderTarget *bot;
extern C3D_RenderTarget *top;
extern u32 kDown, kHeld, kUp;

struct watchdogInfo {
	PrintConsole*		screen;
	struct errInfo_t*	errInfo;
};

struct dirList_t
{
	char**	files;
	int		fileNum;

	char**	directories;
	int		dirNum;

	char*	currentDir;
};



#endif
