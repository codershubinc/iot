#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WebSocketsClient.h>

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

#endif
