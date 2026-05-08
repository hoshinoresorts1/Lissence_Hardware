#include "LissenceBlePeripheral.h"

#include <NimBLEDevice.h>

namespace {

NimBLEServer* server = nullptr;
NimBLECharacteristic* dataCharacteristic = nullptr;

bool isConnected = false;
uint32_t lastNotifyMillis = 0;
uint32_t notifySequence = 0;

class ServerCallbacks final : public NimBLEServerCallbacks {
 public:
  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
    isConnected = true;

    Serial.print("[BLE] iPhone connected: ");
    Serial.println(connInfo.getAddress().toString().c_str());
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
    isConnected = false;

    Serial.print("[BLE] iPhone disconnected. reason=");
    Serial.println(reason);

    NimBLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising restarted");
  }
};

class DataCharacteristicCallbacks final : public NimBLECharacteristicCallbacks {
 public:
  void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo& connInfo) override {
    std::string value = characteristic->getValue();

    Serial.print("[BLE] Write received: ");
    if (value.empty()) {
      Serial.println("(empty)");
      return;
    }

    Serial.write(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    Serial.println();
  }
};

ServerCallbacks serverCallbacks;
DataCharacteristicCallbacks dataCallbacks;

void sendTestNotification() {
  if (!isConnected || dataCharacteristic == nullptr) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastNotifyMillis < 2000) {
    return;
  }

  lastNotifyMillis = now;
  notifySequence += 1;

  String payload = String("{\"type\":\"test\",\"seq\":") + notifySequence + "}";
  dataCharacteristic->setValue(payload.c_str());
  dataCharacteristic->notify();

  Serial.print("[BLE] Notify sent: ");
  Serial.println(payload);
}

}  // namespace

namespace LissenceBlePeripheral {

void begin() {
  NimBLEDevice::init(DeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks);

  NimBLEService* service = server->createService(ServiceUuid);
  dataCharacteristic = service->createCharacteristic(
      CharacteristicUuid,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
          NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);

  dataCharacteristic->setCallbacks(&dataCallbacks);
  dataCharacteristic->setValue("{\"type\":\"ready\"}");

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setName(DeviceName);
  advertising->addServiceUUID(ServiceUuid);
  advertising->enableScanResponse(true);
  advertising->start();

  Serial.print("[BLE] Advertising as ");
  Serial.println(DeviceName);
  Serial.print("[BLE] Service UUID: ");
  Serial.println(ServiceUuid);
  Serial.print("[BLE] Characteristic UUID: ");
  Serial.println(CharacteristicUuid);
}

void loop() {
  sendTestNotification();
  delay(10);
}

}  // namespace LissenceBlePeripheral
