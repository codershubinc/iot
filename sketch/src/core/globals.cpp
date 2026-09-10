#include "globals.h"
#include "../config/config.h"

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WebSocketsClient webSocket;

const char *ssid = "null";
const char *password = "Aeiou@2357";

String serverIP = "";
uint16_t serverPort = 8081;

DisplayMode currentMode = BOOT;

String currentTitle = "";
String currentArtist = "";
int titleScroll = 0;
int artistScroll = 0;
unsigned long lastScrollTime = 0;

String gTitle = "";
String gArtist = "";
String gCurrTime = "00:00";
String gTotTime = "00:00";
String gStatus = "";
int gProgress = 0;

String gClockTime = "00:00";
String gClockDate = "";
String gServerOS = "Loading...";
String gServerKernel = "Loading...";
String gServerUptime = "Loading...";
String gServerStorage = "Loading...";
String gServerCPU = "Loading...";
String gServerRAM = "Loading...";
String gWeather = "";
bool gNeedsFullRedraw = true;
int gBtBattery = -1;
bool gBtConnected = false;
bool gNeedsBadgeRedraw = true;
bool isSleeping = false;
