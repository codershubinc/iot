#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS 5
#define TFT_RST 4
#define TFT_DC 2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WebSocketsClient webSocket;

const char *ssid = "null";
const char *password = "Aeiou@2357";

String serverIP = "";
uint16_t serverPort = 8081;
unsigned long lastTelemetry = 0;

// Custom Colors
#define COLOR_GREY_TRACK 0x4208
#define COLOR_PROGRESS 0x07E0
#define COLOR_ARTIST 0xBDF7
#define COLOR_TIME 0xFFE0
#define COLOR_ACCENT 0x001F

enum DisplayMode
{
  BOOT,
  MUSIC,
  CLOCK,
  STATS,
  WALLPAPER
};
DisplayMode currentMode = BOOT;

String currentTitle = "";
String currentArtist = "";
int titleScroll = 0;
int artistScroll = 0;
unsigned long lastScrollTime = 0;

// Hardware Buttons
#define BTN_PLAY_PAUSE 26
#define BTN_NEXT 27
#define BTN_PREV 25
#define BTN_FOUR 14
unsigned long lastBtnPress[4] = {0, 0, 0, 0};
const int DEBOUNCE_DELAY = 300;


// Global state for instant screen cycling
String gTitle = "";
String gArtist = "";
String gCurrTime = "00:00";
String gTotTime = "00:00";
String gStatus = "";
int gProgress = 0;
String gClockTime = "00:00";
String gClockDate = "";

void drawCurrentScreen() {
    if (currentMode == MUSIC) {
        // Reset scrolling if track changed
        if (gTitle != currentTitle)
        {
          currentTitle = gTitle;
          titleScroll = 0;
        }
        if (gArtist != currentArtist)
        {
          currentArtist = gArtist;
          artistScroll = 0;
        }

        // Draw Progress Bar
        tft.fillRect(0, 128, gProgress, 3, COLOR_PROGRESS);
        if (gProgress < 128)
          tft.fillRect(gProgress, 128, 128 - gProgress, 3, COLOR_GREY_TRACK);

        String tTitle = gTitle;
        String tArtist = gArtist;
        while (tTitle.length() < 21) tTitle += " ";
        while (tArtist.length() < 21) tArtist += " ";

        tft.setTextSize(1);
        tft.setCursor(2, 132);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(tTitle.substring(0, 21));

        tft.setCursor(2, 142);
        tft.setTextColor(COLOR_ARTIST, ST77XX_BLACK);
        tft.print(tArtist.substring(0, 21));

        tft.setCursor(2, 152);
        tft.setTextColor(COLOR_TIME, ST77XX_BLACK);
        tft.print(gCurrTime + " / " + gTotTime + "   ");

        tft.fillRect(114, 150, 12, 10, ST77XX_BLACK);
        if (gStatus == "Playing")
        {
          tft.fillRect(115, 151, 3, 8, COLOR_PROGRESS);
          tft.fillRect(121, 151, 3, 8, COLOR_PROGRESS);
        }
        else
        {
          tft.fillTriangle(115, 151, 115, 159, 123, 155, ST77XX_RED);
        }
    }
    else if (currentMode == CLOCK) {
        tft.setTextSize(3);
        tft.setTextColor(COLOR_TIME, ST77XX_BLACK);
        tft.setCursor(20, 50);
        tft.print(gClockTime);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_ARTIST, ST77XX_BLACK);
        tft.setCursor(30, 90);
        tft.print(gClockDate);
    }
    else if (currentMode == STATS) {
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(5, 10);
        tft.print("SYSTEM STATS");
        
        tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
        tft.setCursor(5, 30);
        tft.print("IP: ");
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(WiFi.localIP().toString() + "   ");

        tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
        tft.setCursor(5, 50);
        tft.print("Heap: ");
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(String(ESP.getFreeHeap() / 1024) + " KB   ");

        tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
        tft.setCursor(5, 70);
        tft.print("Uptime: ");
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(String(millis() / 1000) + " s   ");

        tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
        tft.setCursor(5, 90);
        tft.print("WiFi: ");
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(String(WiFi.RSSI()) + " dBm   ");
    }
}

void playBootAnimation()
{
  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP); 
  pinMode(BTN_PREV, INPUT_PULLUP); 
  pinMode(BTN_FOUR, INPUT_PULLUP);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(4, 4, 120, 152, COLOR_GREY_TRACK);
  tft.drawRect(6, 6, 116, 148, COLOR_ACCENT);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 25);
  tft.print("Quazaar IoT");

  tft.setTextColor(COLOR_ARTIST);
  tft.setCursor(24, 40);
  tft.print("by codershubinc");

  tft.setTextColor(COLOR_GREY_TRACK);
  tft.setCursor(32, 135);
  tft.print("v0.0.2-beta");

  int barX = 14, barY = 95, barWidth = 100, barHeight = 8;
  tft.drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, ST77XX_WHITE);

  WiFi.begin(ssid, password);
  int progress = 0;

  tft.setTextColor(COLOR_TIME);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(50);
    progress += 4;
    if (progress > barWidth)
      progress = barWidth;
    tft.fillRect(barX, barY, progress, barHeight, COLOR_PROGRESS);

    tft.fillRect(24, 75, 90, 8, ST77XX_BLACK);
    tft.setCursor(24, 75);
    tft.print("Connecting Wi-Fi");
  }

  // mDNS Discovery Phase
  tft.fillRect(24, 75, 90, 8, ST77XX_BLACK);
  tft.setCursor(24, 75);
  tft.print("Locating Server");

  if (!MDNS.begin("quazaar-node"))
  {
    Serial.println("Error starting mDNS");
  }

  while (serverIP == "")
  {
    Serial.println("Searching for mDNS service 'quazaar-iot'...");
    int n = MDNS.queryService("quazaar-iot", "tcp");
    if (n > 0)
    {
      Serial.print("Found ");
      Serial.print(n);
      Serial.println(" services!");

      for (int i = 0; i < n; i++)
      {
        String ip = MDNS.address(i).toString();

        // Extract real host IP from TXT record if provided by Go server
        if (MDNS.hasTxt(i, "ip"))
        {
          String txtIP = MDNS.txt(i, "ip");
          if (txtIP.length() > 7)
          {
            ip = txtIP;
          }
        }

        Serial.print("  [");
        Serial.print(i);
        Serial.print("] IP: ");
        Serial.print(ip);
        Serial.print(" Port: ");
        Serial.println(MDNS.port(i));

        if (ip.startsWith("192.") || ip.startsWith("10.") || serverIP == "")
        {
          serverIP = ip;
          serverPort = MDNS.port(i);
        }
      }

      Serial.print("-> Selected Server IP: ");
      Serial.println(serverIP);
    }
    else
    {
      Serial.println("No services found, retrying...");
    }
    delay(1000);
  }

  // Fallback override is now safely commented out because TXT records work flawlessly!
  // serverIP = "192.168.1.107";
  // serverPort = 8081;

  tft.fillRect(barX, barY, barWidth, barHeight, COLOR_PROGRESS);
  delay(100);
  tft.fillScreen(ST77XX_BLACK);
}

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
    tft.fillScreen(ST77XX_BLACK); // Clear the boot screen
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    break;

  case WStype_TEXT:
  {
    // If it starts with '{', it's a JSON command from the Web Dashboard
    if (payload[0] == '{')
    {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);



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
    // Otherwise, it is the Music Metadata payload
    else
    {
      if (currentMode == WALLPAPER)
      {
        break; // Ignore background music updates while wallpaper is active
      }



      int linesFound = 0;
      int lastPos = 0;
      String meta[8];

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
          if (linesFound == 8)
            break;
        }
      }

      if (linesFound == 8)
      {
        gTitle = meta[0];
        gArtist = meta[1];
        gCurrTime = meta[2];
        gTotTime = meta[3];
        gStatus = meta[4];
        gProgress = meta[5].toInt();
        gClockTime = meta[6];
        gClockDate = meta[7];

        drawCurrentScreen();
      }
    }
    break;
  }

  case WStype_BIN:
  {
    // 4096 bytes of pixel data + 1 byte index = 4097
    if (length == 4097)
    {
      uint8_t chunkIndex = payload[0];
      int y_start = chunkIndex * 16; // 16 rows of pixels per chunk

      // Draw the 16-pixel-tall strip directly to the screen
      tft.drawRGBBitmap(0, y_start, (uint16_t *)&payload[1], 128, 16);
    }
    break;
  }
  }
}

void setup()
{
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);
  tft.setTextWrap(false);

  playBootAnimation();

  // Connect to discovered server
  ArduinoOTA.setHostname("quazaar-esp32");
  ArduinoOTA.begin();

  // Connect to discovered server
  webSocket.begin(serverIP, serverPort, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop()
{
  webSocket.loop();
  ArduinoOTA.handle();

  // Read Buttons
  if (millis() - lastBtnPress[0] > DEBOUNCE_DELAY && digitalRead(BTN_PLAY_PAUSE) == LOW) {
    lastBtnPress[0] = millis();
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"play_pause\"}");
  }
  if (millis() - lastBtnPress[1] > DEBOUNCE_DELAY && digitalRead(BTN_NEXT) == LOW) {
    lastBtnPress[1] = millis();
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"next\"}");
  }
  if (millis() - lastBtnPress[2] > DEBOUNCE_DELAY && digitalRead(BTN_PREV) == LOW) {
    lastBtnPress[2] = millis();
    webSocket.sendTXT("{\"type\":\"command\",\"action\":\"prev\"}");
  }
  if (millis() - lastBtnPress[3] > DEBOUNCE_DELAY && digitalRead(BTN_FOUR) == LOW) {
    lastBtnPress[3] = millis();
    if (currentMode == MUSIC) currentMode = CLOCK;
    else if (currentMode == CLOCK) currentMode = STATS;
    else if (currentMode == STATS) {
      currentMode = MUSIC;
      webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    }
    else {
      currentMode = MUSIC;
      webSocket.sendTXT("{\"type\":\"command\",\"action\":\"refresh_art\"}");
    }
    tft.fillScreen(ST77XX_BLACK);
    drawCurrentScreen();
  }

  // Send Telemetry to Web Dashboard every 2s
  if (millis() - lastTelemetry > 2000)
  {
    if (webSocket.isConnected())
    {
      StaticJsonDocument<200> doc;
      doc["type"] = "telemetry";
      doc["uptime"] = millis() / 1000;
      doc["heap"] = ESP.getFreeHeap();

      String jsonString;
      serializeJson(doc, jsonString);
      webSocket.sendTXT(jsonString);
    }
    lastTelemetry = millis();
  }
}