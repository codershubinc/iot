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
    tft.fillScreen(TFT_BLACK);
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    break;

  case WStype_TEXT:
  {
    if (payload[0] == '{')
    {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error && doc["type"] == "server_stats")
      {
                gServerUptime = doc["uptime"].as<String>();
        gServerStorage = doc["storage"].as<String>();
        gServerCPU = doc["cpu"].as<String>();
        gServerRAM = doc["ram"].as<String>();
        gNetDown = doc["net_down"].as<String>();
        gNetUp = doc["net_up"].as<String>();
        gServerGPU = doc["gpu"].as<String>();
        gServerGpuPower = doc["gpu_power"].as<String>();
        gServerCpuTemp = doc["cpu_temp"].as<String>();
        gServerFan = doc["fan"].as<String>();
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
        else if (doc["action"] == "clear") {
          tft.fillScreen(TFT_BLACK);
          currentMode = MUSIC;
        } else if (doc["action"] == "set_screen_music") {
          currentMode = MUSIC; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_clock") {
          currentMode = CLOCK; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_stats") {
          currentMode = STATS; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_server") {
          currentMode = SERVER; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_clock_0") {
          currentClockStyle = 0; if(currentMode==CLOCK){tft.fillScreen(TFT_BLACK); gNeedsFullRedraw=true; drawCurrentScreen();}
        } else if (doc["action"] == "set_clock_1") {
          currentClockStyle = 1; if(currentMode==CLOCK){tft.fillScreen(TFT_BLACK); gNeedsFullRedraw=true; drawCurrentScreen();}
        } else if (doc["action"] == "set_clock_2") {
          currentClockStyle = 2; if(currentMode==CLOCK){tft.fillScreen(TFT_BLACK); gNeedsFullRedraw=true; drawCurrentScreen();}
        }
        else if (doc["action"] == "wallpaper") {
          tft.fillScreen(TFT_BLACK);
          currentMode = WALLPAPER;
        } else if (doc["action"] == "upload_screensaver") {
          isUploadingScreensaver = true;
          for (int i = 0; i < 8; i++) {
            if (dynamicScreensaverChunks[i] == nullptr) {
              dynamicScreensaverChunks[i] = (uint8_t*)malloc(4096);
            }
          }
        } else if (doc["action"] == "test_sleep") {
          isSleeping = true;
          tft.fillScreen(TFT_BLACK);
          bool hasAllChunks = true;
          for (int i=0; i<8; i++) if (dynamicScreensaverChunks[i] == nullptr) hasAllChunks = false;
          
          if (hasAllChunks) {
            for (int i=0; i<8; i++) {
               tft.pushImage(0, i * 16, 128, 16, (uint16_t*)dynamicScreensaverChunks[i]);
            }
          } else {
            tft.pushImage(0, 0, 128, 128, screensaver_img);
          }
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
    if (isUploadingScreensaver) {
      if (length == 4097) {
        uint8_t chunkIndex = payload[0];
        if (chunkIndex < 8 && dynamicScreensaverChunks[chunkIndex] != nullptr) {
          memcpy(dynamicScreensaverChunks[chunkIndex], &payload[1], 4096);
        }
        if (chunkIndex == 7) {
          isUploadingScreensaver = false; // Done!
        }
      }
      break;
    }

    if (currentMode != MUSIC && currentMode != WALLPAPER)
      break; // Do not draw artwork over other screens!
    
    if (length == 4097)
    {
      uint8_t chunkIndex = payload[0];
      int y_start = chunkIndex * 16;
      
      // Fix LoadStoreAlignmentException by copying to an aligned buffer first!
      uint16_t alignedBuffer[2048];
      memcpy(alignedBuffer, &payload[1], 4096);

      tft.pushImage(0, y_start, 128, 16, alignedBuffer);

      if (chunkIndex == 0)
        maskCornersTop(12, TFT_BLACK);
      if (chunkIndex == 7)
      {
        maskCornersBottom(12, TFT_BLACK);
        gNeedsBadgeRedraw = true;
        drawCurrentScreen();
      }
    }
    break;
  }
  }
}
