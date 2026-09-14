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

  WiFi.begin(ssid, password);
  int progress = 0;
  tft.setTextColor(COLOR_ARTIST, COLOR_CARD_BG);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 500) { // 10 seconds max
    delay(20);
    wifiAttempts++;
    progress += 4;
    if (progress > barWidth) progress = barWidth;
    
    tft.fillRoundRect(barX, barY, progress, barHeight, 3, COLOR_PROGRESS);
    tft.setCursor(20, 85);
    tft.print("Wi-Fi Connect...");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    serverIP = "offline";
    tft.setCursor(20, 85);
    tft.print("Wi-Fi Failed!   ");
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    return;
  }

  tft.setCursor(20, 85);
  tft.print("Locating Host...");

  if (!MDNS.begin("quazaar-node")) {
    Serial.println("Error starting mDNS");
  }

  int mdnsAttempts = 0;
  while (serverIP == "" && mdnsAttempts < 75) { // 15 seconds max
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
    
    if (serverIP != "") break; // Skip the delay if we found it!
    
    mdnsAttempts++;
    delay(200); // reduced from 1000 to check much faster!
  }
  
  if (serverIP == "") {
    serverIP = "offline";
    tft.setCursor(20, 85);
    tft.print("Host Missing!   ");
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    return;
  }

  tft.fillRoundRect(barX, barY, barWidth, barHeight, 3, COLOR_PROGRESS);
  tft.fillScreen(TFT_BLACK);
}
