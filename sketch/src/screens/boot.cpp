#include "boot.h"
#include "../core/globals.h"
#include "../config/config.h"
#include "../network/network.h"
#include <WiFi.h>
#include <ESPmDNS.h>

void playBootAnimation() {
  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_FOUR, INPUT_PULLUP);

  tft.fillScreen(TFT_BLACK);
  
  tft.fillRoundRect(8, 20, 112, 120, 12, COLOR_CARD_BG);
  tft.drawRoundRect(8, 20, 112, 120, 12, COLOR_DARK_GREY);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(22, 35);
  tft.print("Quazaar");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(55, 55);
  tft.print("OS v2.0");

  tft.setTextColor(COLOR_DARK_GREY);
  tft.setCursor(35, 122);
  tft.print("IoT Node");

  int barX = 18, barY = 100, barWidth = 92, barHeight = 6;
  tft.fillRoundRect(barX, barY, barWidth, barHeight, 3, COLOR_DARK_GREY);
  tft.setTextColor(COLOR_ARTIST, COLOR_CARD_BG);

  auto updateBootUI = [&](const char* statusMsg, int progress) {
    if (progress > barWidth) progress = barWidth;
    tft.fillRoundRect(barX, barY, progress, barHeight, 3, COLOR_PROGRESS);
    tft.setCursor(20, 85);
    char buf[18];
    snprintf(buf, sizeof(buf), "%-16.16s", statusMsg);
    tft.print(buf);
  };

  bool wifiOk = connectWiFiWithFallback(8000, updateBootUI);
  if (!wifiOk) {
    serverIP = "offline";
    updateBootUI("Wi-Fi Failed!   ", barWidth / 2);
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    return;
  }

  bool hostOk = discoverHostMDNS(15000, updateBootUI);
  if (!hostOk) {
    serverIP = "offline";
    updateBootUI("Host Missing!   ", barWidth * 3 / 4);
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    return;
  }

  updateBootUI("Ready!          ", barWidth);
  delay(300);
  tft.fillScreen(TFT_BLACK);
}
