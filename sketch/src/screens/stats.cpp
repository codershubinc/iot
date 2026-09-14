#include "stats.h"
#include "../core/globals.h"
#include "../config/config.h"
#include <WiFi.h>

void drawStatsScreen() {
    if (gNeedsFullRedraw) {
        tft.fillRoundRect(4, 20, 120, 115, 8, COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
        tft.setCursor(10, 28);
        tft.print("ESP32 DEVICE");
        tft.drawFastHLine(10, 38, 100, COLOR_DARK_GREY);
        
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(10, 48); tft.print("IP: ");
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(10, 68); tft.print("RAM: ");
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(10, 88); tft.print("WiFi: ");
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(10, 108); tft.print("Temp:");
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
    tft.setCursor(34, 48); tft.print(WiFi.localIP().toString() + "       ");
    tft.setCursor(40, 68); tft.print(String(ESP.getFreeHeap() / 1024) + " KB    ");
    tft.setCursor(46, 88); tft.print(String(WiFi.RSSI()) + " dBm    ");
    tft.setCursor(46, 108); tft.print(String(temperatureRead(), 1) + " C    ");
}
