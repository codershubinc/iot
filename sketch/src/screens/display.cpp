#include "display.h"
#include "../core/globals.h"
#include "music.h"
#include "clock.h"
#include "stats.h"
#include "server.h"

void drawCurrentScreen() {
    if (isSleeping) return;

    if (currentMode == MUSIC) drawMusicScreen();
    else if (currentMode == CLOCK) drawClockScreen();
    else if (currentMode == STATS) drawStatsScreen();
    else if (currentMode == SERVER) drawServerScreen();

    gNeedsFullRedraw = false;
}
