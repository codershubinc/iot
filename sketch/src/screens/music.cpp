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
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    static String lastCurr = "";
    static String lastTot = "";
    static String lastTitle = "";
    static String lastArtist = "";
    static uint16_t lastThemeColor = 0;

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (tCurr != lastCurr || tTot != lastTot || gNeedsFullRedraw || gNeedsBadgeRedraw) {
        // Draw elegant pill backgrounds for the time text so it doesn't look like a glitchy black bar
        tft.fillRoundRect(2, 118, (tCurr.length() * 6) + 6, 11, 3, TFT_BLACK);
        tft.fillRoundRect(120 - (tTot.length() * 6), 118, (tTot.length() * 6) + 6, 11, 3, TFT_BLACK);
        
        tft.setCursor(5, 120);
        tft.print(tCurr);
        tft.setCursor(123 - (tTot.length() * 6), 120);
        tft.print(tTot);
        lastCurr = tCurr;
        lastTot = tTot;
    }

    // Pad strings to ensure old characters are wiped if title gets shorter
    String tTitle = gTitle.substring(0, 16);
    String tArtist = gArtist.substring(0, 16);
    while(tTitle.length() < 16) tTitle += " ";
    while(tArtist.length() < 16) tArtist += " ";

    if (tTitle != lastTitle || tArtist != lastArtist || gThemeColor != lastThemeColor || gNeedsFullRedraw || gNeedsBadgeRedraw) {
        tft.setCursor(5, 140);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.print(tTitle);
        
        tft.setCursor(5, 150);
        tft.setTextColor(gThemeColor, TFT_BLACK);
        tft.print(tArtist);
        
        lastTitle = tTitle;
        lastArtist = tArtist;
        lastThemeColor = gThemeColor;
    }

    drawBluetoothBadge();
    drawPlayPauseIcon();
}
