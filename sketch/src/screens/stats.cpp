#include "stats.h"
#include "../core/globals.h"
#include "../config/config.h"
#include <WiFi.h>

void drawStatsScreen() {
    if (gNeedsFullRedraw) {
        tft.fillRoundRect(4, 12, 120, 124, 8, COLOR_CARD_BG);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
        tft.setCursor(10, 20);
        tft.print("ESP32 DEVICE");
        tft.drawFastHLine(10, 30, 100, COLOR_DARK_GREY);
        
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(10, 38); tft.print("IP: ");
        tft.setCursor(10, 56); tft.print("RAM: ");
        tft.setCursor(10, 74); tft.print("Flash:");
        tft.setCursor(10, 92); tft.print("WiFi: ");
        tft.setCursor(10, 110); tft.print("Temp:");
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);

    uint32_t flashTotal = ESP.getFlashChipSize();
    uint32_t flashUsed = ESP.getSketchSize();
    uint32_t flashFree = (flashTotal > flashUsed) ? (flashTotal - flashUsed) : 0;
    String storageStr = String(flashFree / (1024.0 * 1024.0), 1) + "/" + String(flashTotal / (1024.0 * 1024.0), 1) + "MB";

    if (WiFi.status() == WL_CONNECTED) {
        tft.setCursor(34, 38); tft.print(WiFi.localIP().toString() + "       ");
        tft.setCursor(40, 56); tft.print(String(ESP.getFreeHeap() / 1024) + " KB    ");
        tft.setCursor(48, 74); tft.print(storageStr + "    ");
        tft.setCursor(46, 92); tft.print(String(WiFi.RSSI()) + " dBm    ");
        tft.setCursor(46, 110); tft.print(String(temperatureRead(), 1) + " C    ");
    } else {
        tft.setCursor(34, 38); tft.print("No Network    ");
        tft.setCursor(40, 56); tft.print(String(ESP.getFreeHeap() / 1024) + " KB    ");
        tft.setCursor(48, 74); tft.print(storageStr + "    ");
        tft.setCursor(46, 92); tft.print("Offline       ");
        tft.setCursor(46, 110); tft.print(String(temperatureRead(), 1) + " C    ");
    }
}
