#pragma once

#include <Arduino.h>

namespace LissenceBlePeripheral {

constexpr const char* DeviceName = "Lissence-ESP32";
constexpr const char* ServiceUuid = "7d2f3a10-3b7a-4f9f-9b37-6b6a0f4f7c10";
constexpr const char* CharacteristicUuid = "7d2f3a11-3b7a-4f9f-9b37-6b6a0f4f7c10";

void begin();
void loop();
void sendMicLevel(int32_t rms, int32_t peak);

}  // namespace LissenceBlePeripheral
