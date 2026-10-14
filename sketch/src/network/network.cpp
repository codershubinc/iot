#include "network.h"
#include "../core/globals.h"
#include "../screens/display.h"
#include "../screens/widgets.h"
#include "../core/audio_controller.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

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

      if (!error && doc["type"] == "bot")
      {
        if (doc["text"].is<const char*>()) {
          gBotText = doc["text"].as<String>();
        }
        if (doc["expression"].is<const char*>()) {
          gBotExpression = doc["expression"].as<String>();
        }
        currentMode = BOT;
        gNeedsFullRedraw = true;
        drawCurrentScreen();

        // If engine is not mp3, or if no url is streamed, speak directly on-chip via SAM
        String engine = doc["engine"] | "sam";
        if (engine == "sam" && gBotText.length() > 0) {
          speakBotText(gBotText.c_str());
        }
      }

      if (!error && doc["type"] == "play_audio")
      {
        if (doc["url"].is<const char*>()) {
          const char* audioUrl = doc["url"].as<const char*>();
          playAudioURL(audioUrl);
        }
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
        } else if (doc["action"] == "stop_audio") {
          stopAudio();
          drawCurrentScreen();
        } else if (doc["action"] == "set_volume") {
          if (doc["volume"].is<float>()) {
            setAudioVolume(doc["volume"].as<float>());
          } else if (doc["volume"].is<int>()) {
            setAudioVolume(doc["volume"].as<int>() / 100.0f);
          }
        } else if (doc["action"] == "set_screen_music") {
          currentMode = MUSIC; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_clock") {
          currentMode = CLOCK; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_stats") {
          currentMode = STATS; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_server") {
          currentMode = SERVER; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
        } else if (doc["action"] == "set_screen_bot") {
          currentMode = BOT; tft.fillScreen(TFT_BLACK); gNeedsFullRedraw = true; drawCurrentScreen();
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

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool connectWiFiWithFallback(uint32_t timeoutPerNetworkMs, 
                             std::function<void(const char* statusMsg, int progressStep)> progressCallback) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Scan available networks to find best matching SSID
  if (progressCallback) progressCallback("Scanning Wi-Fi  ", 15);
  WiFi.scanNetworks(true, false);
  uint32_t startScan = millis();
  int scanStep = 15;
  while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && millis() - startScan < 4500) {
    delay(40);
    scanStep++;
    if (scanStep > 35) scanStep = 35;
    if (progressCallback) progressCallback("Scanning Wi-Fi  ", scanStep);
  }

  int16_t numFound = WiFi.scanComplete();
  int bestIdx = -1;
  int bestRSSI = -999;
  String bestSSID = "";

  if (numFound > 0) {
    for (int i = 0; i < numFound; i++) {
      String scanned = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);
      for (size_t j = 0; j < WIFI_NETWORKS_COUNT; j++) {
        if (scanned == WIFI_NETWORKS[j].ssid) {
          if (rssi > bestRSSI) {
            bestRSSI = rssi;
            bestIdx = j;
            bestSSID = scanned;
          }
          break;
        }
      }
    }
  }
  WiFi.scanDelete();

  // Try connecting to the best matching network found during scan
  if (bestIdx >= 0) {
    Serial.printf("[WIFI] Boot found configured network: %s (%d dBm). Connecting...\n", 
                  WIFI_NETWORKS[bestIdx].ssid, bestRSSI);
    String msg = "Connect: " + bestSSID;
    if (progressCallback) progressCallback(msg.c_str(), 40);
    WiFi.begin(WIFI_NETWORKS[bestIdx].ssid, WIFI_NETWORKS[bestIdx].password);

    uint32_t startConn = millis();
    int connStep = 40;
    while (WiFi.status() != WL_CONNECTED && millis() - startConn < timeoutPerNetworkMs) {
      delay(40);
      connStep++;
      if (connStep > 70) connStep = 70;
      if (progressCallback) progressCallback(msg.c_str(), connStep);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Connected to %s, IP: %s\n", 
                    WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      ssid = WIFI_NETWORKS[bestIdx].ssid;
      password = WIFI_NETWORKS[bestIdx].password;
      if (progressCallback) progressCallback("Wi-Fi OK!       ", 70);
      return true;
    }
    Serial.println("[WIFI] Best scan match failed, trying fallbacks...");
  }

  // Fallback: try all other configured networks sequentially
  for (size_t j = 0; j < WIFI_NETWORKS_COUNT; j++) {
    if ((int)j == bestIdx) continue;

    Serial.printf("[WIFI] Trying fallback network: %s\n", WIFI_NETWORKS[j].ssid);
    String msg = "Connect: " + String(WIFI_NETWORKS[j].ssid);
    if (progressCallback) progressCallback(msg.c_str(), 50);

    WiFi.disconnect();
    delay(100);
    WiFi.begin(WIFI_NETWORKS[j].ssid, WIFI_NETWORKS[j].password);

    uint32_t startTry = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTry < timeoutPerNetworkMs) {
      delay(40);
      if (progressCallback) progressCallback(msg.c_str(), 60);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Fallback connected to %s, IP: %s\n", 
                    WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      ssid = WIFI_NETWORKS[j].ssid;
      password = WIFI_NETWORKS[j].password;
      if (progressCallback) progressCallback("Wi-Fi OK!       ", 70);
      return true;
    }
  }

  Serial.println("[WIFI] All configured networks failed to connect.");
  if (progressCallback) progressCallback("Wi-Fi Failed!   ", 70);
  return false;
}

bool discoverHostMDNS(uint32_t maxWaitMs, 
                      std::function<void(const char* statusMsg, int progressStep)> progressCallback) {
  if (progressCallback) progressCallback("Locating Host...", 75);

  MDNS.end();
  if (!MDNS.begin("quazaar-node")) {
    Serial.println("[mDNS] Error starting mDNS responder");
  }

  uint32_t startTime = millis();
  int hostStep = 75;

  while (millis() - startTime < maxWaitMs) {
    Serial.println("[mDNS] Searching for 'quazaar-iot' service...");
    int n = MDNS.queryService("quazaar-iot", "tcp");
    if (n > 0) {
      for (int i = 0; i < n; i++) {
        String ip = MDNS.IP(i).toString();
        if (MDNS.hasTxt(i, "ip")) {
          String txtIP = MDNS.txt(i, "ip");
          if (txtIP.length() > 7) ip = txtIP;
        }

        if (ip.startsWith("192.") || ip.startsWith("10.") || serverIP == "" || serverIP == "offline") {
          serverIP = ip;
          serverPort = MDNS.port(i);
        }
      }

      if (serverIP != "" && serverIP != "offline") {
        Serial.printf("[mDNS] Host found at %s:%d\n", serverIP.c_str(), serverPort);
        if (progressCallback) progressCallback("Host Found!     ", 92);
        return true;
      }
    }

    if (serverIP != "" && serverIP != "offline") break;

    delay(200);
    hostStep += 2;
    if (hostStep > 92) hostStep = 92;
    if (progressCallback) progressCallback("Locating Host...", hostStep);
  }

  if (serverIP == "" || serverIP == "offline") {
    serverIP = "offline";
    Serial.println("[mDNS] Host not found");
    if (progressCallback) progressCallback("Host Missing!   ", 92);
    return false;
  }
  return true;
}

void handleNetworkLoop() {
  enum WiFiAutoState {
    WFA_CONNECTED,
    WFA_IDLE_DISCONNECTED,
    WFA_SCANNING,
    WFA_CONNECTING
  };

  static WiFiAutoState wifiState = WFA_CONNECTED;
  static unsigned long stateTimer = 0;
  static unsigned long lastHostRetryTime = 0;
  static size_t fallbackRoundRobinIdx = 0;
  static bool wasConnected = (WiFi.status() == WL_CONNECTED);

  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

  // --- CONNECTED STATE ---
  if (currentlyConnected) {
    if (!wasConnected) {
      wasConnected = true;
      wifiState = WFA_CONNECTED;
      Serial.printf("[WIFI] Auto-connected to %s (RSSI: %d dBm), IP: %s\n", 
                    WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());

      // Update active credentials pointers
      for (size_t i = 0; i < WIFI_NETWORKS_COUNT; i++) {
        if (WiFi.SSID() == WIFI_NETWORKS[i].ssid) {
          ssid = WIFI_NETWORKS[i].ssid;
          password = WIFI_NETWORKS[i].password;
          break;
        }
      }

      // Re-initialize mDNS and OTA on new IP
      MDNS.end();
      MDNS.begin("quazaar-node");
      ArduinoOTA.begin();

      // If host server was offline or disconnected, discover host on the new network
      if (serverIP == "offline" || serverIP == "" || !webSocket.isConnected()) {
        Serial.println("[WIFI] New network: discovering host server via mDNS...");
        if (discoverHostMDNS(3000, nullptr)) {
          webSocket.disconnect();
          webSocket.begin(serverIP, serverPort, "/ws");
          webSocket.onEvent(webSocketEvent);
          webSocket.setReconnectInterval(5000);
        }
      }
    }

    // Periodic host check if WiFi is up but host server is offline (e.g. host started later)
    if ((serverIP == "offline" || !webSocket.isConnected()) && millis() - lastHostRetryTime > 15000) {
      lastHostRetryTime = millis();
      int n = MDNS.queryService("quazaar-iot", "tcp");
      if (n > 0) {
        for (int i = 0; i < n; i++) {
          String ip = MDNS.IP(i).toString();
          if (MDNS.hasTxt(i, "ip")) {
            String txtIP = MDNS.txt(i, "ip");
            if (txtIP.length() > 7) ip = txtIP;
          }
          if (ip.startsWith("192.") || ip.startsWith("10.") || serverIP == "" || serverIP == "offline") {
            serverIP = ip;
            serverPort = MDNS.port(i);
          }
        }
        if (serverIP != "" && serverIP != "offline") {
          Serial.printf("[mDNS] Host found at %s:%d! Reconnecting WebSocket...\n", 
                        serverIP.c_str(), serverPort);
          webSocket.disconnect();
          webSocket.begin(serverIP, serverPort, "/ws");
          webSocket.onEvent(webSocketEvent);
          webSocket.setReconnectInterval(5000);
        }
      }
    }
    return;
  }

  // --- DISCONNECTED STATE: Non-blocking auto-reconnect / fallback ---
  if (wasConnected) {
    wasConnected = false;
    Serial.println("[WIFI] Connection lost! Initiating auto-reconnect / fallback...");
    wifiState = WFA_IDLE_DISCONNECTED;
    stateTimer = millis();
  }

  unsigned long now = millis();

  switch (wifiState) {
    case WFA_CONNECTED:
      wifiState = WFA_IDLE_DISCONNECTED;
      stateTimer = now;
      break;

    case WFA_IDLE_DISCONNECTED:
      // Cooldown of 2.5s before starting scan to avoid radio thrashing
      if (now - stateTimer > 2500) {
        Serial.println("[WIFI] Scanning for available configured networks in background...");
        WiFi.disconnect();
        WiFi.scanNetworks(true, false); // async scan
        wifiState = WFA_SCANNING;
        stateTimer = now;
      }
      break;

    case WFA_SCANNING: {
      int16_t scanStatus = WiFi.scanComplete();
      if (scanStatus == WIFI_SCAN_RUNNING) {
        // Abort scan if it hangs longer than 6 seconds
        if (now - stateTimer > 6000) {
          Serial.println("[WIFI] Async scan timed out. Retrying...");
          WiFi.scanDelete();
          wifiState = WFA_IDLE_DISCONNECTED;
          stateTimer = now;
        }
        return; // Non-blocking: loop continues smoothly
      }

      int bestIdx = -1;
      int bestRSSI = -999;

      if (scanStatus > 0) {
        for (int i = 0; i < scanStatus; i++) {
          String s = WiFi.SSID(i);
          int32_t rssi = WiFi.RSSI(i);
          for (size_t j = 0; j < WIFI_NETWORKS_COUNT; j++) {
            if (s == WIFI_NETWORKS[j].ssid) {
              if (rssi > bestRSSI) {
                bestRSSI = rssi;
                bestIdx = j;
              }
              break;
            }
          }
        }
      }
      WiFi.scanDelete();

      if (bestIdx >= 0) {
        Serial.printf("[WIFI] Found configured network %s (%d dBm). Connecting...\n", 
                      WIFI_NETWORKS[bestIdx].ssid, bestRSSI);
        WiFi.begin(WIFI_NETWORKS[bestIdx].ssid, WIFI_NETWORKS[bestIdx].password);
        wifiState = WFA_CONNECTING;
        stateTimer = now;
      } else {
        // Fallback: round-robin attempt for hidden SSIDs or weak beacons
        size_t idx = fallbackRoundRobinIdx % WIFI_NETWORKS_COUNT;
        fallbackRoundRobinIdx++;
        Serial.printf("[WIFI] No scan match. Trying fallback network [%d]: %s\n", 
                      (int)idx, WIFI_NETWORKS[idx].ssid);
        WiFi.begin(WIFI_NETWORKS[idx].ssid, WIFI_NETWORKS[idx].password);
        wifiState = WFA_CONNECTING;
        stateTimer = now;
      }
      break;
    }

    case WFA_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        wifiState = WFA_CONNECTED;
      } else if (now - stateTimer > 8000) {
        Serial.println("[WIFI] Connection attempt timed out. Trying next fallback/scan...");
        WiFi.disconnect();
        wifiState = WFA_IDLE_DISCONNECTED;
        stateTimer = now;
      }
      break;
  }
}
