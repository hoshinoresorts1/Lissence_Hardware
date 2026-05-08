# Lissence BLE Protocol

## Overview

Lissence hardware acts as a BLE Peripheral. The iPhone app acts as the BLE Central.

This first protocol draft is for connection verification only. It does not include microphone events or haptic driver commands yet.

## Device

- BLE name: `Lissence-ESP32`
- Role: Peripheral
- Target board: ESP32-C3

## GATT

### Lissence Test Service

- Service UUID: `7d2f3a10-3b7a-4f9f-9b37-6b6a0f4f7c10`

### Data Characteristic

- Characteristic UUID: `7d2f3a11-3b7a-4f9f-9b37-6b6a0f4f7c10`
- Properties:
  - `READ`
  - `WRITE`
  - `WRITE_NR`
  - `NOTIFY`

## Message Draft

Messages are UTF-8 JSON strings.

### Peripheral to iPhone

Test notification sent every 2 seconds while connected:

```json
{"type":"test","seq":1}
```

Initial readable value:

```json
{"type":"ready"}
```

### iPhone to Peripheral

For connection testing, the iPhone may write any UTF-8 string. The ESP32 prints the received payload to Serial.

Suggested command draft:

```json
{"type":"ping"}
```

## Future Message Types

- Microphone event notification: ESP32 to iPhone
- Haptic command: iPhone to ESP32
- Device status: ESP32 to iPhone
