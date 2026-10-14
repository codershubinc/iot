#include "bot.h"
#include "../core/globals.h"
#include "../config/config.h"

static unsigned long lastMouthBlink = 0;
static bool mouthOpen = false;

void drawBotScreen() {
    // If full redraw requested, clear and draw background cards
    if (gNeedsFullRedraw) {
        tft.fillScreen(TFT_BLACK);
        
        // Face Card container (120x85)
        tft.fillRoundRect(4, 8, 120, 85, 10, COLOR_CARD_BG);
        tft.drawRoundRect(4, 8, 120, 85, 10, COLOR_DARK_GREY);

        // Subtitle / Text Card container (120x55)
        tft.fillRoundRect(4, 98, 120, 56, 8, COLOR_CARD_BG);
        tft.drawFastHLine(10, 114, 108, COLOR_DARK_GREY);

        tft.setTextSize(1);
        tft.setTextColor(COLOR_ACCENT, COLOR_CARD_BG);
        tft.setCursor(12, 103);
        tft.print("BOT RESPONSE");
    }

    // --- Face Animation (Eyes & Mouth) ---
    // Left Eye center: (42, 42), Right Eye center: (86, 42)
    uint16_t eyeColor = gThemeColor;
    if (gBotExpression == "angry") {
        eyeColor = TFT_RED;
    } else if (gBotExpression == "surprised") {
        eyeColor = COLOR_TIME; // Yellow
    }

    // Static / Re-drawn Eyes
    tft.fillRoundRect(30, 28, 24, 26, 6, eyeColor);
    tft.fillRoundRect(74, 28, 24, 26, 6, eyeColor);
    
    // Eye pupils/glints
    tft.fillCircle(38, 36, 3, TFT_BLACK);
    tft.fillCircle(82, 36, 3, TFT_BLACK);

    // Mouth animation (animate while speaking)
    if (gBotSpeaking) {
        if (millis() - lastMouthBlink > 180) {
            lastMouthBlink = millis();
            mouthOpen = !mouthOpen;
        }
    } else {
        mouthOpen = false;
    }

    // Clear mouth area
    tft.fillRect(44, 62, 40, 18, COLOR_CARD_BG);

    if (gBotSpeaking && mouthOpen) {
        // Open talking mouth
        tft.fillRoundRect(50, 64, 28, 12, 5, TFT_WHITE);
        tft.fillRoundRect(54, 67, 20, 6, 2, TFT_BLACK);
    } else if (gBotExpression == "surprised") {
        tft.fillCircle(64, 70, 6, TFT_WHITE);
    } else {
        // Friendly smile / resting mouth
        tft.fillRoundRect(52, 67, 24, 5, 2, TFT_WHITE);
    }

    // --- Bot Speech Text Subtitle ---
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);

    // Print text with basic 2-line wrap
    String line1 = gBotText;
    String line2 = "";
    if (gBotText.length() > 18) {
        int splitIdx = gBotText.lastIndexOf(' ', 18);
        if (splitIdx == -1) splitIdx = 18;
        line1 = gBotText.substring(0, splitIdx);
        line2 = gBotText.substring(splitIdx + 1);
        if (line2.length() > 18) {
            line2 = line2.substring(0, 15) + "...";
        }
    }

    // Clear previous text line area
    tft.fillRect(10, 120, 108, 30, COLOR_CARD_BG);
    tft.setCursor(12, 122);
    tft.print(line1);
    if (line2.length() > 0) {
        tft.setCursor(12, 134);
        tft.print(line2);
    }
}
