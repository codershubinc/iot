#include "display.h"
#include "../core/globals.h"
#include "music.h"
#include "clock.h"
#include "stats.h"
#include "server.h"
#include "bot.h"

void drawCurrentScreen() {
    if (isSleeping) return;

    if (currentMode == MUSIC) drawMusicScreen();
    else if (currentMode == CLOCK) drawClockScreen();
    else if (currentMode == STATS) drawStatsScreen();
    else if (currentMode == SERVER) drawServerScreen();
    else if (currentMode == BOT) drawBotScreen();

    gNeedsFullRedraw = false;
}
