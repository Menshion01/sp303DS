#ifndef TIME_H
#define TIME_H

#include <time.h>
#include <stdbool.h>

extern void start_stopwatch(void);
extern float stop_stopwatch(void);
extern float get_elapsed_time(void);

extern char* makeSexagesimal(int inputSec);
extern void stopwatchFunction(void);

extern bool last_touch_state;
#endif
