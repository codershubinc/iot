#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include <WebSocketsClient.h>

extern Adafruit_ST7735 tft;
extern WebSocketsClient webSocket;

extern const char *ssid;
extern const char *password;

extern String serverIP;
extern uint16_t serverPort;

enum DisplayMode { BOOT, MUSIC, CLOCK, STATS, SERVER, WALLPAPER };
extern DisplayMode currentMode;

extern String currentTitle;
extern String currentArtist;
extern int titleScroll;
extern int artistScroll;
extern unsigned long lastScrollTime;

extern String gTitle;
extern String gArtist;
extern String gCurrTime;
extern String gTotTime;
extern String gStatus;
extern int gProgress;

extern String gClockTime;
extern String gClockDate;
extern String gServerOS;
extern String gServerKernel;
extern String gServerUptime;
extern String gServerStorage;
extern String gServerCPU;
extern String gServerRAM;
extern String gWeather;
extern bool gNeedsFullRedraw;
extern int gBtBattery;
extern bool gBtConnected;
extern bool gNeedsBadgeRedraw;
extern bool isSleeping;

#endif
