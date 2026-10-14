#include "audio_controller.h"
#include "globals.h"
#include "../screens/display.h"
#include <ESP8266SAM.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceBuffer.h>

static AudioOutputI2S *audioOut = nullptr;
static AudioGeneratorMP3 *mp3Gen = nullptr;
static AudioFileSourceHTTPStream *httpSource = nullptr;
static AudioFileSourceBuffer *bufferedSource = nullptr;

static float currentVolumeGain = 0.15f; // Good default: 15%

void setupAudio() {
    if (!audioOut) {
        // 0 = I2S port 0, INTERNAL_DAC routes audio to GPIO 25 (L) & GPIO 26 (R)
        audioOut = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
        audioOut->SetOutputModeMono(true);
        audioOut->SetGain(currentVolumeGain);
        Serial.println("[AUDIO] I2S Internal DAC ready on GPIO 25 & 26.");
    }
}

void setAudioVolume(float gain) {
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    currentVolumeGain = gain;
    if (audioOut) {
        audioOut->SetGain(currentVolumeGain);
    }
    Serial.printf("[AUDIO] Volume set to: %.2f (%.0f%%)\n", currentVolumeGain, currentVolumeGain * 100);
}

float getAudioVolume() {
    return currentVolumeGain;
}

void stopAudio() {
    if (mp3Gen && mp3Gen->isRunning()) {
        mp3Gen->stop();
    }
    if (mp3Gen) { delete mp3Gen; mp3Gen = nullptr; }
    if (bufferedSource) { delete bufferedSource; bufferedSource = nullptr; }
    if (httpSource) { delete httpSource; httpSource = nullptr; }

    gBotSpeaking = false;
}

bool isAudioPlaying() {
    return (mp3Gen && mp3Gen->isRunning());
}

void playAudioURL(const char *url) {
    if (!url || strlen(url) == 0) return;
    setupAudio();
    stopAudio();

    Serial.printf("[AUDIO STREAM] Connecting to: %s\n", url);
    httpSource = new AudioFileSourceHTTPStream(url);
    if (!httpSource) {
        Serial.println("[AUDIO STREAM] Failed to allocate HTTP source!");
        return;
    }

    // 8KB buffer for smooth streaming over WiFi
    bufferedSource = new AudioFileSourceBuffer(httpSource, 8192);
    if (!bufferedSource) {
        Serial.println("[AUDIO STREAM] Failed to allocate buffer!");
        stopAudio();
        return;
    }

    mp3Gen = new AudioGeneratorMP3();
    if (!mp3Gen) {
        Serial.println("[AUDIO STREAM] Failed to allocate MP3 generator!");
        stopAudio();
        return;
    }

    if (mp3Gen->begin(bufferedSource, audioOut)) {
        Serial.println("[AUDIO STREAM] MP3 stream started!");
        gBotSpeaking = true;
    } else {
        Serial.println("[AUDIO STREAM] MP3 begin() failed!");
        stopAudio();
    }
}

void loopAudio() {
    if (mp3Gen && mp3Gen->isRunning()) {
        if (!mp3Gen->loop()) {
            Serial.println("[AUDIO STREAM] Stream finished.");
            stopAudio();
            drawCurrentScreen();
        }
    }
}

void speakBotText(const char *phrase) {
    if (!phrase || strlen(phrase) == 0) return;
    setupAudio();
    stopAudio();

    Serial.printf("[BOT AUDIO] Speaking: %s\n", phrase);
    gBotSpeaking = true;
    drawCurrentScreen();

    ESP8266SAM *sam = new ESP8266SAM;
    sam->SetVoice(ESP8266SAM::SAMVoice::VOICE_ELF);
    sam->SetSpeed(72);
    sam->SetPitch(64);
    sam->SetThroat(128);
    sam->SetMouth(128);
    sam->Say(audioOut, phrase);
    delete sam;

    gBotSpeaking = false;
    drawCurrentScreen();
    Serial.println("[BOT AUDIO] Finished speaking.");
}
