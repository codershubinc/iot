#include "music.h"
#include "../core/globals.h"
#include "../config/config.h"
#include "widgets.h"

void drawMusicScreen() {
    if (gTitle != currentTitle) { currentTitle = gTitle; titleScroll = 0; }
    if (gArtist != currentArtist) { currentArtist = gArtist; artistScroll = 0; }

    if (gNeedsFullRedraw) {
        tft.fillRoundRect(5, 131, 118, 4, 2, COLOR_DARK_GREY);
    }

    int progWidth = (gProgress * 118) / 128;
    if (progWidth > 0) {
        tft.fillRoundRect(5, 131, progWidth, 4, 2, COLOR_PROGRESS);
    }

    String tTitle = gTitle;
    String tArtist = gArtist;
    while (tTitle.length() < 17) tTitle += " ";
    while (tArtist.length() < 17) tArtist += " ";

    tft.setTextSize(1);
    tft.setCursor(5, 140);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(tTitle.substring(0, 16));

    tft.setCursor(5, 150);
    tft.setTextColor(COLOR_ARTIST, ST77XX_BLACK);
    tft.print(tArtist.substring(0, 16));

    drawBluetoothBadge();
    drawPlayPauseIcon();
}
