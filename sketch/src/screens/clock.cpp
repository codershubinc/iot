#include "clock.h"
#include "../core/globals.h"
#include "../config/config.h"

void drawClockScreen() {
    String hhmm = gClockTime;
    String ss = "";
    if (gClockTime.length() >= 8) {
        hhmm = gClockTime.substring(0, 5);
        ss = gClockTime.substring(6, 8);
    }

    if (currentClockStyle == 0) {
        // STYLE 0: Classic Card
        if (gNeedsFullRedraw) {
            tft.fillRoundRect(4, 25, 120, 105, 10, COLOR_CARD_BG);
            tft.drawFastHLine(14, 85, 100, COLOR_DARK_GREY);
        }

        tft.setTextSize(3);
        tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
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
            tft.setTextColor(gThemeColor, COLOR_CARD_BG);
            int wx = 64 - (gWeather.length() * 3);
            tft.setCursor(wx, 110);
            tft.print(gWeather + "   ");
        }
    } 
    else if (currentClockStyle == 1) {
        // STYLE 1: Huge Minimal
        if (gNeedsFullRedraw) {
            tft.fillScreen(TFT_BLACK);
        }
        tft.setTextSize(4);
        tft.setTextColor(gThemeColor, TFT_BLACK);
        tft.setCursor(6, 45);
        tft.print(hhmm);
        
        tft.setTextSize(1);
        tft.setTextColor(COLOR_DARK_GREY, TFT_BLACK);
        tft.setCursor(64 - (gClockDate.length() * 3), 90);
        tft.print(gClockDate);
    }
    else if (currentClockStyle == 2) {
        // STYLE 2: Tech Vertical
        if (gNeedsFullRedraw) {
            tft.fillScreen(TFT_BLACK);
            tft.drawRect(5, 5, 118, 148, COLOR_DARK_GREY);
        }
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        
        if (hhmm.length() >= 5) {
            tft.setCursor(45, 20); tft.print(hhmm.substring(0, 2));
            tft.setCursor(45, 45); tft.print(hhmm.substring(3, 5));
        }
        tft.setTextColor(COLOR_ACCENT, TFT_BLACK);
        tft.setCursor(45, 70); tft.print(ss);
        
        tft.setTextSize(1);
        tft.setTextColor(gThemeColor, TFT_BLACK);
        tft.setCursor(15, 110); tft.print(gClockDate + "   ");
        tft.setCursor(15, 125); tft.print(gWeather + "   ");
    }
}
