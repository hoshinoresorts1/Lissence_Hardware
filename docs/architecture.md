# Lissence Hardware Architecture

## Overview

The hardware project is separated from the iOS app so ESP32 firmware, BLE protocol work, and hardware experiments can evolve independently.

## Current BLE Test Architecture

```text
iPhone BLE Central
        |
        | scan/connect/write/subscribe notify
        v
ESP32-C3 BLE Peripheral
        |
        | Serial debug output
        v
USB Serial Monitor
```

## ESP32-C3 Responsibilities

- Advertise as `Lissence-ESP32`
- Expose one custom GATT service
- Expose one data characteristic with read, write, write without response, and notify
- Print connection and write events to Serial
- Send a test notification every 2 seconds while connected

## BLE Communication Direction

| Direction | Transport | Purpose |
| --- | --- | --- |
| ESP32-C3 -> iPhone | BLE Notify | test, mic_event, battery, status, error |
| iPhone -> ESP32-C3 | BLE Write | haptic, stop, config, ping |

## Not Included Yet

- INMP441 microphone sampling
- Audio event detection
- DRV2605L haptic driver control
- LRA waveform selection
- Battery or power management

## Planned Extension Points

- Add microphone event notifications beside the current test notification payload.
- Add iPhone-to-ESP32 haptic command parsing on the writable characteristic.
- Split hardware modules into dedicated firmware components:
  - BLE transport
  - microphone input
  - haptic driver
  - power/battery state
- Connect iPhone music mood output to BLE haptic command payloads.
