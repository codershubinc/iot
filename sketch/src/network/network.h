#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <functional>

// WebSocket event callback
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

// Connect to WiFi with scan-based best match and sequential fallback (used during boot)
bool connectWiFiWithFallback(uint32_t timeoutPerNetworkMs = 8000, 
                             std::function<void(const char* statusMsg, int progressStep)> progressCallback = nullptr);

// Discover host using mDNS (used during boot and on reconnection)
bool discoverHostMDNS(uint32_t maxWaitMs = 15000, 
                      std::function<void(const char* statusMsg, int progressStep)> progressCallback = nullptr);

// Non-blocking WiFi & host reconnection handler to be called inside loop()
void handleNetworkLoop();

// Helper to check if currently connected to WiFi
bool isWiFiConnected();

#endif
