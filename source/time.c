#include "time.h"
#include <sys/time.h>
#include "main.h"
#include "text.h"
#include "ui.h"

struct timeval start_time;
struct timeval end_time;
int running = 0;

bool last_touch_state = false;

void start_stopwatch(void) {
    gettimeofday(&start_time, NULL);
    running = 1;
}

float stop_stopwatch(void) {
    if (running) {
        gettimeofday(&end_time, NULL);
        running = 0;

        double start_seconds = start_time.tv_sec + start_time.tv_usec / 1e6;
        double end_seconds = end_time.tv_sec + end_time.tv_usec / 1e6;

        return end_seconds - start_seconds;
    }
    return 0.0;
}

float get_elapsed_time(void) {
    if (running) {
        struct timeval now;
        gettimeofday(&now, NULL);
        double start_seconds = start_time.tv_sec + start_time.tv_usec / 1e6;
        double now_seconds = now.tv_sec + now.tv_usec / 1e6;
        return now_seconds - start_seconds;
    }
    else {
        double start_seconds = start_time.tv_sec + start_time.tv_usec / 1e6;
        double end_seconds = end_time.tv_sec + end_time.tv_usec / 1e6;
        return end_seconds - start_seconds;
    }
}

char* makeSexagesimal(int inputSec) {
    static char buffer[16];  // Static buffer to hold the final string
    int minutes = ((inputSec) / 60 );
    int seconds = (inputSec) % 60;

    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, abs(seconds));
    return buffer;
}
