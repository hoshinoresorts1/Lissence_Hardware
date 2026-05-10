#pragma once

#include <Arduino.h>

namespace HapticMotorController {

/// DRV2605L 햅틱 모터 드라이버를 초기화합니다.
void begin();

/// BLE callback에서 받은 햅틱 pattern을 loop 처리용으로 보류합니다.
void enqueuePattern(const String& pattern);

/// 보류된 햅틱 command가 있으면 DRV2605L effect sequence로 실행합니다.
void loop();

}  // namespace HapticMotorController
