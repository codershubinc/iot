#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WebSocketsClient.h>

extern TFT_eSPI tft;
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
extern String gServerUptime;
extern String gServerStorage;
extern String gServerCPU;
extern String gServerRAM;
extern String gNetDown;
extern String gNetUp;
extern String gServerGPU;
extern String gServerGpuPower;
extern String gServerCpuTemp;
extern String gServerFan;
extern String gWeather;
extern bool gNeedsFullRedraw;
extern int gBtBattery;
extern uint16_t gThemeColor;
extern bool gBtConnected;
extern bool gNeedsBadgeRedraw;
extern bool isSleeping;
extern bool isUploadingScreensaver;
extern uint16_t* dynamicScreensaver;

#endif
extern int currentClockStyle;
