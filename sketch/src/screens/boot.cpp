#include "boot.h"
#include "../core/globals.h"
#include "../config/config.h"
#include <WiFi.h>
#include <ESPmDNS.h>

void playBootAnimation() {
  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_FOUR, INPUT_PULLUP);

  tft.fillScreen(ST77XX_BLACK);
  
  tft.fillRoundRect(8, 20, 112, 120, 12, COLOR_CARD_BG);
  tft.drawRoundRect(8, 20, 112, 120, 12, COLOR_DARK_GREY);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
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

  WiFi.begin(ssid, password);
  int progress = 0;
  tft.setTextColor(COLOR_ARTIST, COLOR_CARD_BG);

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    progress += 4;
    if (progress > barWidth) progress = barWidth;
    
    tft.fillRoundRect(barX, barY, progress, barHeight, 3, COLOR_PROGRESS);
    tft.setCursor(20, 85);
    tft.print("Wi-Fi Connect...");
  }

  tft.setCursor(20, 85);
  tft.print("Locating Host...");

  if (!MDNS.begin("quazaar-node")) {
    Serial.println("Error starting mDNS");
  }

  while (serverIP == "") {
    Serial.println("Searching for mDNS service 'quazaar-iot'...");
    int n = MDNS.queryService("quazaar-iot", "tcp");
    if (n > 0) {
      for (int i = 0; i < n; i++) {
        String ip = MDNS.IP(i).toString();
        if (MDNS.hasTxt(i, "ip")) {
          String txtIP = MDNS.txt(i, "ip");
          if (txtIP.length() > 7) ip = txtIP;
        }

        if (ip.startsWith("192.") || ip.startsWith("10.") || serverIP == "") {
          serverIP = ip;
          serverPort = MDNS.port(i);
        }
      }
    } else {
      Serial.println("No services found, retrying...");
    }
    delay(1000);
  }

  tft.fillRoundRect(barX, barY, barWidth, barHeight, 3, COLOR_PROGRESS);
  delay(100);
  tft.fillScreen(ST77XX_BLACK);
}
