#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// --- TFT WIRING ---
#define TFT_CS     5
#define TFT_RST    4  
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- NETWORK CONFIGURATION ---
const char* ssid = "null";
const char* password = "Aeiou@2357";
const char* serverUrl = "http://192.168.1.106:8080/nowplaying"; 

unsigned long previousMillis = 0;
const long interval = 1000; // Fetch data every 1 second

// Custom Colors
#define COLOR_GREY_TRACK 0x4208
#define COLOR_PROGRESS   0x07E0 // Neon Green
#define COLOR_ARTIST     0xBDF7 // Light Grey
#define COLOR_TIME       0xFFE0 // Yellow
#define COLOR_ACCENT     0x001F // Deep Blue

// --- ANIMATED BOOT SEQUENCE ---
void playBootAnimation() {
  tft.fillScreen(ST77XX_BLACK);

  // Decorative border frame
  tft.drawRect(4, 4, 120, 152, COLOR_GREY_TRACK);
  tft.drawRect(6, 6, 116, 148, COLOR_ACCENT);

  // Title Text Render
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(32, 25);
  tft.print("qqxion-iot");

  tft.setTextColor(COLOR_ARTIST);
  tft.setCursor(24, 40);
  tft.print("by codershubinc");

  tft.setTextColor(COLOR_TIME);
  tft.setCursor(24, 75);
  tft.print("Initializing...");

  // Loading Bar Animation
  int barX = 14;
  int barY = 95;
  int barWidth = 100;
  int barHeight = 8;

  tft.drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, ST77XX_WHITE);

  WiFi.begin(ssid, password);
  
  int progress = 0;
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(150);
    attempts++;
    if (attempts > 100) ESP.restart(); // Restart if no connection after 15s

    progress += 4;
    if (progress > barWidth) progress = barWidth;

    tft.fillRect(barX, barY, progress, barHeight, COLOR_PROGRESS);

    if (progress > 60 && progress < 80) {
      tft.fillRect(24, 75, 90, 8, ST77XX_BLACK);
      tft.setCursor(24, 75);
      tft.print("Linking Server.");
    } else if (progress >= 80) {
      tft.fillRect(24, 75, 90, 8, ST77XX_BLACK);
      tft.setCursor(24, 75);
      tft.print("Launching UI...");
    }

    if (progress >= barWidth && WiFi.status() != WL_CONNECTED) {
      progress = barWidth - 10; 
    }
  }

  tft.fillRect(barX, barY, barWidth, barHeight, COLOR_PROGRESS);
  delay(400);
  tft.fillScreen(ST77XX_BLACK);
}

void fetchAndDisplayPlayer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(serverUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    WiFiClient *stream = http.getStreamPtr();

    String title    = stream->readStringUntil('\n');
    String artist   = stream->readStringUntil('\n');
    String currTime = stream->readStringUntil('\n');
    String totTime  = stream->readStringUntil('\n');
    String status   = stream->readStringUntil('\n');
    String progStr  = stream->readStringUntil('\n');

    title.trim();
    artist.trim();
    currTime.trim();
    totTime.trim();
    status.trim();
    int progress = progStr.toInt(); // 0 to 128 pixels

    // --- 1. THE PROGRESS BAR ---
    tft.fillRect(0, 128, progress, 3, COLOR_PROGRESS);
    if (progress < 128) {
      tft.fillRect(progress, 128, 128 - progress, 3, COLOR_GREY_TRACK);
    }

    // --- 2. TYPOGRAPHY PADDING ---
    while(title.length() < 21) title += " ";
    while(artist.length() < 21) artist += " ";

    tft.setTextSize(1);

    // Song Title (Y: 132) - Safely inside view
    tft.setCursor(2, 132);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
    tft.print(title.substring(0, 21));

    // Artist (Y: 142) - Safely inside view
    tft.setCursor(2, 142);
    tft.setTextColor(COLOR_ARTIST, ST77XX_BLACK); 
    tft.print(artist.substring(0, 21));

    // Timing Line (Y: 152) - Pulled up to prevent bottom clipping
    tft.setCursor(2, 152);
    tft.setTextColor(COLOR_TIME, ST77XX_BLACK); 
    tft.print(currTime);
    tft.print(" / ");
    tft.print(totTime);

    // --- 3. ICONOGRAPHY (Play/Pause) ---
    tft.fillRect(114, 150, 12, 10, ST77XX_BLACK);
    if (status == "Playing") {
      tft.fillRect(115, 151, 3, 8, COLOR_PROGRESS);
      tft.fillRect(121, 151, 3, 8, COLOR_PROGRESS);
    } else {
      tft.fillTriangle(115, 151, 115, 159, 123, 155, ST77XX_RED); 
    }

    // --- 4. RENDER ALBUM ART ---
    const int imageSize = 128 * 128 * 2;
    uint8_t* imageBuffer = (uint8_t*) malloc(imageSize);

    if (imageBuffer != NULL) {
      size_t bytesRead = 0;
      while (http.connected() && bytesRead < imageSize) {
        size_t available = stream->available();
        if (available) {
          size_t spaceLeft = imageSize - bytesRead;
          size_t toRead = (available < spaceLeft) ? available : spaceLeft;
          int c = stream->readBytes(&imageBuffer[bytesRead], toRead);
          bytesRead += c;
        }
      }

      tft.drawRGBBitmap(0, 0, (uint16_t*)imageBuffer, 128, 128);
      free(imageBuffer);
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);
  tft.setTextWrap(false);

  playBootAnimation();
  fetchAndDisplayPlayer();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    fetchAndDisplayPlayer();
  }
}