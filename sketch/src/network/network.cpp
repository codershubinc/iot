#include "network.h"
#include "../core/globals.h"
#include "../screens/display.h"
#include "../screens/widgets.h"
#include <ArduinoJson.h>

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WS] Disconnected!");
    break;

  case WStype_CONNECTED:
    Serial.println("[WS] Connected!");
    currentMode = MUSIC;
    gNeedsFullRedraw = true;
    tft.fillScreen(ST77XX_BLACK);
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    break;

  case WStype_TEXT:
  {
    if (payload[0] == '{')
    {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error && doc["type"] == "server_stats")
      {
        gServerOS = doc["os"].as<String>();
        gServerKernel = doc["kernel"].as<String>();
        gServerUptime = doc["uptime"].as<String>();
        gServerStorage = doc["storage"].as<String>();
        gServerCPU = doc["cpu"].as<String>();
        gServerRAM = doc["ram"].as<String>();
        gNetDown = doc["net_down"].as<String>();
        gNetUp = doc["net_up"].as<String>();
        if (currentMode == SERVER)
          drawCurrentScreen();
      }

      if (!error && doc["type"] == "bluetooth")
      {
        int newBatt = doc["battery"].as<int>();
        bool newConn = doc["connected"].as<bool>();
        if (newBatt != gBtBattery || newConn != gBtConnected)
        {
          gNeedsBadgeRedraw = true;
        }
        gBtBattery = newBatt;
        gBtConnected = newConn;
        if (currentMode == MUSIC)
          drawCurrentScreen();
      }

      if (!error && doc["type"] == "weather")
      {
        gWeather = doc["temp"].as<String>();
        if (currentMode == CLOCK)
          drawCurrentScreen();
      }

      if (!error && doc["type"] == "command")
      {
        if (doc["action"] == "reboot")
        {
          ESP.restart();
        }
        else if (doc["action"] == "clear")
        {
          tft.fillScreen(ST77XX_BLACK);
          currentMode = MUSIC;
        }
        else if (doc["action"] == "wallpaper")
        {
          tft.fillScreen(ST77XX_BLACK);
          currentMode = WALLPAPER;
        }
      }
    }
    else
    {
      if (currentMode == WALLPAPER)
      {
        break;
      }

      int linesFound = 0;
      int lastPos = 0;
      String meta[9];

      for (size_t i = 0; i < length; i++)
      {
        if (payload[i] == '\n')
        {
          for (size_t j = lastPos; j < i; j++)
          {
            meta[linesFound] += (char)payload[j];
          }
          linesFound++;
          lastPos = i + 1;
          if (linesFound == 9)
            break;
        }
      }

      if (linesFound == 9)
      {
        gTitle = meta[0];
        gArtist = meta[1];
        gCurrTime = meta[2];
        gTotTime = meta[3];
        gStatus = meta[4];
        gProgress = meta[5].toInt();
        gClockTime = meta[6];
        gThemeColor = meta[8].toInt();
        gClockDate = meta[7];

        drawCurrentScreen();
      }
    }
    break;
  }

  case WStype_BIN:
  {
    if (currentMode != MUSIC)
      break; // Do not draw artwork over other screens!
    if (length == 4097)
    {
      uint8_t chunkIndex = payload[0];
      int y_start = chunkIndex * 16;

      tft.drawRGBBitmap(0, y_start, (uint16_t *)&payload[1], 128, 16);

      if (chunkIndex == 0)
        maskCornersTop(12, ST77XX_BLACK);
      if (chunkIndex == 7)
      {
        maskCornersBottom(12, ST77XX_BLACK);
        gNeedsBadgeRedraw = true;
        drawCurrentScreen();
      }
    }
    break;
  }
  }
}
