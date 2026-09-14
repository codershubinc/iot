#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <OneButton.h>

#include "config/config.h"
#include "core/globals.h"
#include "screens/display.h"
#include "screens/boot.h"
#include "network/network.h"
#include "screens/screensaver_img.h"

unsigned long lastTelemetry = 0;

OneButton btnPlayPause(BTN_PLAY_PAUSE, true);
OneButton btnNext(BTN_NEXT, true);
OneButton btnPrev(BTN_PREV, true);
OneButton btnFour(BTN_FOUR, true);

unsigned long lastVolTick = 0;

unsigned long lastPlayingTime = 0;

bool wakeUpIfNeeded() {
  lastPlayingTime = millis(); // Reset idle timer on any button press!
  if (isSleeping) {
    isSleeping = false;
    tft.fillScreen(TFT_BLACK);
    gNeedsFullRedraw = true;
    drawCurrentScreen();
    return true;
  }
  return false;
}


void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setSwapBytes(true);
  tft.invertDisplay(false);
  tft.setTextWrap(false);

  playBootAnimation();

  ArduinoOTA.setHostname("quazaar-esp32");
  ArduinoOTA.begin();

  if (serverIP != "offline") {
    webSocket.begin(serverIP, serverPort, "/ws");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
  } else {
    currentMode = CLOCK;
    gNeedsFullRedraw = true;
    drawCurrentScreen();
  }

  // --- BTN 1 (Volume Decrease) ---
  btnPlayPause.attachClick([]() {
    if (wakeUpIfNeeded()) return;
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"vol_down\"}");
  });

  // --- BTN 2 (Play/Pause, Next, Prev) ---
  btnNext.attachClick([]() {
    if (wakeUpIfNeeded()) return;
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"play_pause\"}");
  });
  btnNext.attachDoubleClick([]() {
    if (wakeUpIfNeeded()) return;
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"next\"}");
  });
  btnNext.attachMultiClick([]() {
    if (wakeUpIfNeeded()) return;
    if (btnNext.getNumberClicks() == 3) {
      webSocket.sendTXT("{\"type\":\"command\",\"action\":\"prev\"}");
    }
  });

  // --- BTN 3 (Volume Increase) ---
  btnPrev.attachClick([]() {
    if (wakeUpIfNeeded()) return;
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"vol_up\"}");
  });

  // --- BTN 4 (Cycle Screens) ---
  btnFour.attachClick([]() {
    if (wakeUpIfNeeded()) return;

    if (currentMode == MUSIC) currentMode = CLOCK;
    else if (currentMode == CLOCK) currentMode = STATS;
    else if (currentMode == STATS) currentMode = SERVER;
    else if (currentMode == SERVER) {
      currentMode = MUSIC;
      webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    }
    else {
      currentMode = MUSIC;
      webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    }
    
    tft.fillScreen(TFT_BLACK);
    gNeedsFullRedraw = true;
    drawCurrentScreen();
  });
  btnFour.attachLongPressStart([]() {
    ESP.restart();
  });
}

void loop()
{
  webSocket.loop();
  ArduinoOTA.handle();


  if (gStatus == "Playing") {
    lastPlayingTime = millis();
    if (isSleeping) {
      isSleeping = false;
      tft.fillScreen(TFT_BLACK);
      gNeedsFullRedraw = true;
      drawCurrentScreen();
    }
  }

  // Screensaver after 5 minutes of inactivity (300,000 ms)
  if (!isSleeping && millis() - lastPlayingTime > 300000) {
    isSleeping = true;
    tft.fillScreen(TFT_BLACK);
    if (dynamicScreensaver != nullptr) {
      tft.pushImage(0, 0, 128, 128, dynamicScreensaver);
    } else {
      tft.pushImage(0, 0, 128, 128, screensaver_img);
    }
  }

  // Process Buttons
  btnPlayPause.tick();
  btnNext.tick();
  btnPrev.tick();
  btnFour.tick();

  // Send Telemetry to Web Dashboard every 2s
  if (millis() - lastTelemetry > 2000)
  {
    if (webSocket.isConnected())
    {
      JsonDocument doc;
      doc["type"] = "telemetry";
      doc["uptime"] = millis() / 1000;
      doc["heap"] = ESP.getFreeHeap();
      doc["screen"] = currentMode == MUSIC ? "Music" : currentMode == CLOCK ? "Clock" : currentMode == STATS ? "ESP Stats" : "Host Server";
      doc["clock_style"] = currentClockStyle;

      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.sendTXT(jsonString);
    }
    lastTelemetry = millis();
  }
}
