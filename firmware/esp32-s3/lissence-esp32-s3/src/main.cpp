#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("Lissence ESP32-C3 boot success");
}

void loop() {
  Serial.println("ESP32-C3 running...");
  delay(1000);
}