#include "server.h"
#include "../core/globals.h"
#include "../config/config.h"

void drawServerScreen() {
    if (gNeedsFullRedraw) {
        tft.fillRoundRect(4, 20, 120, 130, 8, 0x4810);
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, 0x4810);
        tft.setCursor(10, 28);
        tft.print("HOST SERVER");
        tft.drawFastHLine(10, 38, 100, COLOR_DARK_GREY);
        
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 48); tft.print("OS: ");
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 64); tft.print("Ker: ");
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 80); tft.print("C/R: ");
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 96); tft.print("Up: ");
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 112); tft.print("Disk:");
        tft.setTextColor(COLOR_ACCENT, 0x4810);
        tft.setCursor(10, 128); tft.print("Net: ");
    }
    
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE, 0x4810);
    tft.setCursor(34, 48); tft.print(gServerOS + "       ");
    tft.setCursor(40, 64); tft.print(gServerKernel.substring(0, 11) + "       ");
    tft.setCursor(40, 80); tft.print(gServerCPU + " " + gServerRAM + "    ");
    tft.setCursor(34, 96); tft.print(gServerUptime + "       ");
    tft.setCursor(40, 112); tft.print(gServerStorage + "       ");
    tft.setCursor(40, 128); tft.print("D:" + gNetDown + " U:" + gNetUp + "      ");
}
