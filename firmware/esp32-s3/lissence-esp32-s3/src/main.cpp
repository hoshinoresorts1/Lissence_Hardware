#include <Arduino.h>
#include "LissenceBlePeripheral.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("Lissence ESP32-C3 BLE peripheral boot");

  LissenceBlePeripheral::begin();
}

void loop() {
  LissenceBlePeripheral::loop();
}
