#include "widgets.h"
#include "../core/globals.h"
#include "../config/config.h"

void maskCornersTop(int r, uint16_t color) {
  for (int i = 0; i < r; i++) {
    for (int j = 0; j < r; j++) {
      if ((r - i) * (r - i) + (r - j) * (r - j) > r * r) {
        tft.drawPixel(i, j, color);
        tft.drawPixel(127 - i, j, color);
      }
    }
  }
}

void maskCornersBottom(int r, uint16_t color) {
  for (int i = 0; i < r; i++) {
    for (int j = 0; j < r; j++) {
      if ((r - i) * (r - i) + (r - j) * (r - j) > r * r) {
        tft.drawPixel(i, 127 - j, color);
        tft.drawPixel(127 - i, 127 - j, color);
      }
    }
  }
}

void drawBluetoothBadge() {
    if (gBtConnected && gBtBattery >= 0) {
        if (gNeedsBadgeRedraw || gNeedsFullRedraw) {
            tft.fillRoundRect(4, 4, 46, 16, 4, COLOR_CARD_BG);
            
            tft.fillCircle(11, 10, 2, TFT_WHITE); 
            tft.fillRoundRect(10, 10, 3, 6, 1, TFT_WHITE);
            
            tft.fillCircle(17, 10, 2, TFT_WHITE);
            tft.fillRoundRect(16, 10, 3, 6, 1, TFT_WHITE);
            
            gNeedsBadgeRedraw = false;
        }
        
        tft.setTextSize(1);
        uint16_t battColor = gThemeColor;
        if (gBtBattery <= 20) battColor = TFT_RED;
        else if (gBtBattery <= 50) battColor = COLOR_TIME;
        
        tft.setTextColor(battColor, COLOR_CARD_BG);
        tft.setCursor(24, 8);
        tft.print(String(gBtBattery) + "% ");
    }
}

void drawPlayPauseIcon() {
    static String lastStatus = "";
    if (gNeedsFullRedraw || gStatus != lastStatus) {
        lastStatus = gStatus;
        
        tft.fillCircle(115, 147, 9, COLOR_DARK_GREY);
        
        if (gStatus == "Playing") {
            tft.fillRect(112, 144, 2, 7, gThemeColor);
            tft.fillRect(116, 144, 2, 7, gThemeColor);
        } else {
            tft.fillTriangle(113, 143, 113, 151, 119, 147, TFT_WHITE);
        }
    }
}
