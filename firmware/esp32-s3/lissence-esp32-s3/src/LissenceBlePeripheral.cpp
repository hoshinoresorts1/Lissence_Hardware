#include "LissenceBlePeripheral.h"

#include <NimBLEDevice.h>

namespace {

NimBLEServer* server = nullptr;
NimBLECharacteristic* dataCharacteristic = nullptr;

bool isConnected = false;
#if ENABLE_AUDIO_STREAMING
bool audioStreamStartRequested = false;
bool audioStreamStopRequested = false;
#endif
uint32_t lastNotifyMillis = 0;
uint32_t notifySequence = 0;

#if ENABLE_AUDIO_STREAMING
constexpr size_t AudioPacketHeaderSize = 6;
constexpr size_t MaxAudioPacketPayloadSize = 160;
#endif

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

#if ENABLE_AUDIO_STREAMING
    String payload(value.c_str());
    if (payload.indexOf("\"type\":\"config\"") >= 0 &&
        payload.indexOf("\"audio_stream\":true") >= 0) {
      audioStreamStartRequested = true;
      audioStreamStopRequested = false;
      Serial.println("[BLE] Audio stream start requested");
    } else if (payload.indexOf("\"type\":\"config\"") >= 0 &&
               payload.indexOf("\"audio_stream\":false") >= 0) {
      audioStreamStopRequested = true;
      audioStreamStartRequested = false;
      Serial.println("[BLE] Audio stream stop requested");
    }
#endif
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
#if ENABLE_AUDIO_STREAMING
  NimBLEDevice::setMTU(185);
#endif
  Serial.println("[BLE] init");
  NimBLEDevice::init(DeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  server = NimBLEDevice::createServer();
  Serial.println("[BLE] server created");
  server->setCallbacks(&serverCallbacks);

  NimBLEService* service = server->createService(ServiceUuid);
  Serial.println("[BLE] service created");
  dataCharacteristic = service->createCharacteristic(
      CharacteristicUuid,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
          NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
  Serial.println("[BLE] characteristic created");

  dataCharacteristic->setCallbacks(&dataCallbacks);
  dataCharacteristic->setValue("{\"type\":\"ready\"}");

  Serial.println("[BLE] service started");

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setName(DeviceName);
  advertising->addServiceUUID(ServiceUuid);
  advertising->enableScanResponse(true);
  advertising->start();
  Serial.println("[BLE] advertising started");

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

#if ENABLE_MICROPHONE
void sendMicLevel(int32_t rms, int32_t peak) {
  if (!isConnected || dataCharacteristic == nullptr) {
    return;
  }

  String payload = String("{\"type\":\"mic_level\",\"rms\":") + rms +
                   ",\"peak\":" + peak + "}";
  dataCharacteristic->setValue(payload.c_str());
  dataCharacteristic->notify();

  Serial.print("[BLE] Notify sent: ");
  Serial.println(payload);
}
#endif

#if ENABLE_AUDIO_STREAMING
bool consumeAudioStreamStartRequest() {
  if (!audioStreamStartRequested) {
    return false;
  }

  audioStreamStartRequested = false;
  return true;
}

bool consumeAudioStreamStopRequest() {
  if (!audioStreamStopRequested) {
    return false;
  }

  audioStreamStopRequested = false;
  return true;
}

bool sendAudioStreamPacket(
    uint16_t sequence,
    uint8_t packetIndex,
    uint8_t packetCount,
    const uint8_t* payload,
    uint16_t payloadSize) {
  if (!isConnected || dataCharacteristic == nullptr) {
    return false;
  }

  if (payloadSize > MaxAudioPacketPayloadSize) {
    Serial.println("[BLE] Audio packet payload too large, skip notify");
    return false;
  }

  uint8_t packet[AudioPacketHeaderSize + MaxAudioPacketPayloadSize] = {};
  packet[0] = static_cast<uint8_t>(sequence & 0xFF);
  packet[1] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
  packet[2] = packetIndex;
  packet[3] = packetCount;
  packet[4] = static_cast<uint8_t>(payloadSize & 0xFF);
  packet[5] = static_cast<uint8_t>((payloadSize >> 8) & 0xFF);
  memcpy(packet + AudioPacketHeaderSize, payload, payloadSize);

  dataCharacteristic->setValue(packet, AudioPacketHeaderSize + payloadSize);
  const bool didNotify = dataCharacteristic->notify();
  if (!didNotify) {
    Serial.println("[BLE] Audio notify failed or queue unavailable");
  }

  return didNotify;
}
#endif

}  // namespace LissenceBlePeripheral
