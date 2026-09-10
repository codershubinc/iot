#include "clock.h"
#include "../core/globals.h"
#include "../config/config.h"

void drawClockScreen() {
    if (gNeedsFullRedraw) {
        tft.fillRoundRect(4, 25, 120, 105, 10, COLOR_CARD_BG);
        tft.drawFastHLine(14, 85, 100, COLOR_DARK_GREY);
    }

    String hhmm = gClockTime;
    String ss = "";
    if (gClockTime.length() >= 8) {
        hhmm = gClockTime.substring(0, 5);
        ss = gClockTime.substring(6, 8);
    }

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE, COLOR_CARD_BG);
    tft.setCursor(8, 48);
    tft.print(hhmm);

    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
    tft.setCursor(98, 55);
    tft.print(ss);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_ARTIST, COLOR_CARD_BG);
    int dateX = 64 - (gClockDate.length() * 3);
    if (dateX < 10) dateX = 10;
    tft.setCursor(dateX, 95);
    tft.print(gClockDate + "   ");

    if (gWeather != "") {
        tft.setTextColor(COLOR_PROGRESS, COLOR_CARD_BG);
        int wx = 64 - (gWeather.length() * 3);
        tft.setCursor(wx, 110);
        tft.print(gWeather + "   ");
    }
}
