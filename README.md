# 한국기술교육대학교 LISSENCE 팀

청각장애인을 위한 촉각, 시각 변환 시스템

## 역할

이 저장소는 LISSENCE 프로젝트의 하드웨어 및 펌웨어 작업을 관리합니다.

- ESP32 기반 BLE Peripheral
- INMP441 I2S 마이크 입력 테스트
- DRV2605L + LRA 햅틱 모터 드라이버 제어
- BLE 통신 프로토콜 문서화
- iPhone 앱과의 BLE 연동 테스트

## 보드 종류

현재 테스트에 사용한 보드:

- ESP32-C3
- ESP32-WROOM-32E

기존 폴더명에는 `esp32-s3`가 남아 있지만, 현재 주요 테스트 보드는 ESP32-WROOM-32E입니다.

## 현재 테스트 보드

현재 기준 보드:

```text
ESP32-WROOM-32E
```

## 현재 배선

### INMP441

```text
SCK/BCLK -> GPIO26
WS/LRCK  -> GPIO25
SD       -> GPIO33
L/R      -> GND
VCC      -> 3.3V
GND      -> GND
```

### DRV2605L

```text
VIN      -> 3V3
GND      -> GND
SDA      -> GPIO21
SCL      -> GPIO22
OUT+/OUT- -> LRA motor
```

## BLE 햅틱 명령

iPhone은 BLE write로 햅틱 패턴을 요청합니다. ESP32는 BLE callback에서 바로 모터를 구동하지 않고, pending command로 저장한 뒤 main loop에서 DRV2605L effect sequence를 실행합니다.

```json
{"type":"haptic","pattern":"warning"}
```

지원 pattern:

- `siren`
- `fireAlarm`
- `carHorn`
- `speech`
- `warning`
- `test`

## 빌드

현재 PlatformIO 환경:

```text
env: esp32-wroom-32e
```

펌웨어 빌드:

```bash
cd firmware/esp32-s3/lissence-esp32-s3
/Users/administrator/.platformio/penv/bin/pio run -e esp32-wroom-32e
```

업로드:

```bash
cd firmware/esp32-s3/lissence-esp32-s3
/Users/administrator/.platformio/penv/bin/pio run -e esp32-wroom-32e -t upload
```

Serial Monitor:

```bash
/Users/administrator/.platformio/penv/bin/pio device monitor -p /dev/cu.usbserial-10 -b 115200
```

## GitHub 업로드 전 제외 대상

아래 항목은 `.gitignore`로 제외합니다.

- `.pio/`
- `build/`
- `*.bin`
- `*.elf`
- `*.map`
- `*.log`
- `.DS_Store`
- `.vscode`의 개인 설정 파일

`TeamReference_iOS`, `TeamReference_HW` 같은 참고용 폴더는 이 하드웨어 repo 밖에 두고 관리합니다.
