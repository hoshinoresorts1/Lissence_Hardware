#pragma once

#include <Arduino.h>

namespace LissenceBlePeripheral {

constexpr const char* DeviceName = "Lissence-ESP32";
constexpr const char* ServiceUuid = "7d2f3a10-3b7a-4f9f-9b37-6b6a0f4f7c10";
constexpr const char* CharacteristicUuid = "7d2f3a11-3b7a-4f9f-9b37-6b6a0f4f7c10";

#define ENABLE_MICROPHONE 1
#define ENABLE_AUDIO_STREAMING 1

void begin();
void loop();
#if ENABLE_MICROPHONE
void sendMicLevel(int32_t rms, int32_t peak);
void sendAudioCandidate(const char* kind, int32_t rms, int32_t peak);
#endif

#if ENABLE_AUDIO_STREAMING
bool consumeAudioStreamStartRequest();
bool consumeAudioStreamStopRequest();
void setAudioStreamingActive(bool isActive);
bool sendAudioStreamPacket(uint16_t sequence, uint8_t packetIndex, uint8_t packetCount, const uint8_t* payload, uint16_t payloadSize);
#endif

}  // namespace LissenceBlePeripheral
