#include "HapticMotorController.h"

#include <Wire.h>
#include "Adafruit_DRV2605.h"

namespace {

constexpr uint8_t Drv2605Address = 0x5A;
constexpr int I2CSdaPin = 21;
constexpr int I2CSclPin = 22;

Adafruit_DRV2605 drv;
bool isReady = false;
bool haveCommand = false;
String pendingPattern;

bool isDrv2605Present() {
  Wire.beginTransmission(Drv2605Address);
  return Wire.endTransmission() == 0;
}

void playEffectSequence(const char* pattern, const uint8_t* effects, uint8_t count, uint16_t waitMs) {
  if (!isReady) {
    Serial.print("[HAPTIC] skip pattern, DRV2605L not ready: ");
    Serial.println(pattern);
    return;
  }

  Serial.print("[HAPTIC] play pattern=");
  Serial.print(pattern);
  Serial.print(" effects=");
  for (uint8_t i = 0; i < count; ++i) {
    if (i > 0) {
      Serial.print(",");
    }
    Serial.print(effects[i]);
    drv.setWaveform(i, effects[i]);
  }

  drv.setWaveform(count, 0);
  Serial.print(" waitMs=");
  Serial.println(waitMs);
  drv.go();
  delay(waitMs);
}

void playSiren() {
  const uint8_t effects[] = {84, 82, 84, 82};
  playEffectSequence("siren", effects, 4, 2600);
}

void playFireAlarm() {
  const uint8_t effects[] = {16, 14, 16};
  playEffectSequence("fireAlarm", effects, 3, 3200);
}

void playCarHorn() {
  const uint8_t effects[] = {70, 70};
  playEffectSequence("carHorn", effects, 2, 1300);
}

void playSpeech() {
  const uint8_t effects[] = {13};
  playEffectSequence("speech", effects, 1, 1000);
}

void playWarning() {
  const uint8_t effects[] = {14, 15};
  playEffectSequence("warning", effects, 2, 2100);
}

void playTest() {
  const uint8_t effects[] = {14};
  playEffectSequence("test", effects, 1, 1100);
}

void runPattern(const String& pattern) {
  if (pattern == "siren") {
    playSiren();
  } else if (pattern == "fireAlarm") {
    playFireAlarm();
  } else if (pattern == "carHorn") {
    playCarHorn();
  } else if (pattern == "speech") {
    playSpeech();
  } else if (pattern == "warning") {
    playWarning();
  } else if (pattern == "test") {
    playTest();
  } else {
    Serial.print("[HAPTIC] unknown pattern: ");
    Serial.println(pattern);
  }
}

}  // namespace

namespace HapticMotorController {

void begin() {
  Serial.println("[DRV2605L] init");
  Wire.begin(I2CSdaPin, I2CSclPin);

  Serial.print("[DRV2605L] I2C SDA GPIO");
  Serial.print(I2CSdaPin);
  Serial.print(", SCL GPIO");
  Serial.println(I2CSclPin);

  if (!isDrv2605Present()) {
    Serial.println("[DRV2605L] address 0x5A not found");
    isReady = false;
    return;
  }

  if (!drv.begin()) {
    Serial.println("[DRV2605L] begin failed");
    isReady = false;
    return;
  }

  drv.useLRA();
  drv.selectLibrary(6);
  drv.setMode(DRV2605_MODE_INTTRIG);
  isReady = true;

  Serial.println("[DRV2605L] ready: LRA, library 6, internal trigger");
}

void enqueuePattern(const String& pattern) {
  if (pattern.length() == 0) {
    return;
  }

  pendingPattern = pattern;
  haveCommand = true;

  Serial.print("[HAPTIC] queued pattern: ");
  Serial.println(pattern);
}

void loop() {
  if (!haveCommand) {
    return;
  }

  const String pattern = pendingPattern;
  pendingPattern = "";
  haveCommand = false;
  runPattern(pattern);
}

}  // namespace HapticMotorController
