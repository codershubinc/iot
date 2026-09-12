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
        tft.fillRoundRect(5, 131, progWidth, 4, 2, gThemeColor);
    }

    String tTot = gTotTime;
    String tCurr = gCurrTime;
    if (tTot == "" || tTot == "00:00") tTot = "-:-";
    if (tCurr == "" || tCurr == "00:00") {
        if (tTot == "-:-") tCurr = "-:-";
    }

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    
    String leftStr = tCurr;
    while (leftStr.length() < 5) leftStr += " ";
    tft.setCursor(5, 120);
    tft.print(leftStr);

    String rightStr = tTot;
    while (rightStr.length() < 5) rightStr = " " + rightStr;
    tft.setCursor(123 - (rightStr.length() * 6), 120);
    tft.print(rightStr);

    String tTitle = gTitle;
    String tArtist = gArtist;
    while (tTitle.length() < 17) tTitle += " ";
    while (tArtist.length() < 17) tArtist += " ";

    tft.setTextSize(1);
    tft.setCursor(5, 140);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.print(tTitle.substring(0, 16));

    tft.setCursor(5, 150);
    tft.setTextColor(gThemeColor, ST77XX_BLACK);
    tft.print(tArtist.substring(0, 16));

    drawBluetoothBadge();
    drawPlayPauseIcon();
}
