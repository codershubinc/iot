#include "server.h"
#include "../core/globals.h"
#include "../config/config.h"
#include <math.h>

static int lastCpuPct = -1;
static int lastRamPct = -1;
static int lastGpuPct = -1;

void drawGaugeTrack(int x, int y, int r, String label) {
    tft.fillCircle(x, y, r, COLOR_DARK_GREY);
    tft.fillRect(x - r, y, r * 2 + 1, r + 1, ST77XX_BLACK);
    tft.fillCircle(x, y, r - 3, ST77XX_BLACK);
    tft.fillRect(x - r + 3, y, (r - 3) * 2 + 1, r - 3 + 1, ST77XX_BLACK);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
    tft.setCursor(x - 8, y + 16);
    tft.print(label);
}

void drawGaugeNeedle(int x, int y, int r, int oldPct, int newPct, uint16_t color) {
    if (oldPct >= 0) {
        float oldAngle = 3.14159 - (oldPct * 3.14159 / 100.0);
        int ox = x + (r - 2) * cos(oldAngle);
        int oy = y - (r - 2) * sin(oldAngle);
        tft.drawLine(x, y, ox, oy, ST77XX_BLACK);
        tft.drawLine(x-1, y, ox, oy, ST77XX_BLACK);
        tft.drawLine(x+1, y, ox, oy, ST77XX_BLACK);
    }

    float newAngle = 3.14159 - (newPct * 3.14159 / 100.0);
    int nx = x + (r - 2) * cos(newAngle);
    int ny = y - (r - 2) * sin(newAngle);
    tft.drawLine(x, y, nx, ny, color);
    tft.drawLine(x-1, y, nx, ny, color);
    tft.drawLine(x+1, y, nx, ny, color);
    
    tft.fillCircle(x, y, 3, ST77XX_WHITE);
    
    tft.fillRect(x - 12, y + 6, 24, 10, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    int textX = x - 8;
    if (newPct < 10) textX = x - 4;
    else if (newPct == 100) textX = x - 12;
    tft.setCursor(textX, y + 6);
    tft.print(String(newPct) + "%");
}

void drawServerScreen() {
    if (gNeedsFullRedraw) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        
        tft.setCursor(35, 8);
        tft.print("arch@swap");
        tft.drawFastHLine(10, 18, 108, COLOR_DARK_GREY);
        
        tft.setTextColor(COLOR_ACCENT, ST77XX_BLACK);
        tft.setCursor(5, 80); tft.print("Up: ");
        tft.setCursor(5, 93); tft.print("Disk:");
        tft.setCursor(5, 106); tft.print("Net: ");
        tft.setCursor(5, 119); tft.print("CPU T:");
        tft.setCursor(5, 132); tft.print("GPU W:");
        tft.setCursor(5, 145); tft.print("Fan: ");
        
        drawGaugeTrack(22, 50, 18, "CPU");
        drawGaugeTrack(64, 50, 18, "RAM");
        drawGaugeTrack(106, 50, 18, "GPU");
        
        lastCpuPct = -1;
        lastRamPct = -1;
        lastGpuPct = -1;
    }
    
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(45, 80); tft.print(gServerUptime + "       ");
    tft.setCursor(45, 93); tft.print(gServerStorage + "       ");
    tft.setCursor(45, 106); tft.print("D:" + gNetDown + " U:" + gNetUp + "      ");
    tft.setCursor(45, 119); tft.print(gServerCpuTemp + "       ");
    tft.setCursor(45, 132); tft.print(gServerGpuPower + "       ");
    tft.setCursor(45, 145); tft.print(gServerFan + "       ");

    int cpuPct = gServerCPU.toInt();
    int ramPct = gServerRAM.toInt();
    int gpuPct = gServerGPU.toInt();
    
    if (cpuPct > 100) cpuPct = 100; if (cpuPct < 0) cpuPct = 0;
    if (ramPct > 100) ramPct = 100; if (ramPct < 0) ramPct = 0;
    if (gpuPct > 100) gpuPct = 100; if (gpuPct < 0) gpuPct = 0;

    if (cpuPct != lastCpuPct) {
        drawGaugeNeedle(22, 50, 18, lastCpuPct, cpuPct, 0x07E0);
        lastCpuPct = cpuPct;
    }
    if (ramPct != lastRamPct) {
        drawGaugeNeedle(64, 50, 18, lastRamPct, ramPct, 0xFFE0);
        lastRamPct = ramPct;
    }
    if (gpuPct != lastGpuPct) {
        drawGaugeNeedle(106, 50, 18, lastGpuPct, gpuPct, 0xF800);
        lastGpuPct = gpuPct;
    }
}
