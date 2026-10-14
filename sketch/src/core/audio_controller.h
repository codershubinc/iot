#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include <Arduino.h>

void setupAudio();
void loopAudio();
void speakBotText(const char *phrase);
void playAudioURL(const char *url);
void stopAudio();
bool isAudioPlaying();
void setAudioVolume(float gain); // 0.0 to 1.0
float getAudioVolume();

#endif
