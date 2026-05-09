# Lissence Hardware Pin Map

## ESP32-C3

현재 펌웨어 프로젝트 경로에는 `esp32-s3` 이름이 남아 있지만, 실제 테스트 보드는 ESP32-C3입니다.

## INMP441 I2S Microphone

| INMP441 Pin | ESP32-C3 Pin | 설명 |
| --- | --- | --- |
| SCK | GPIO1 | I2S bit clock |
| WS | GPIO2 | I2S word select / LR clock |
| SD | GPIO3 | I2S serial data input |
| L/R | GND | Left channel 선택 |
| VCC | 3.3V | 전원 |
| GND | GND | Ground |

## 테스트 기준

- Serial Monitor 속도: `115200`
- 출력 예:

```text
Mic RMS: 1234, Peak: 8912
```

조용할 때와 소리를 냈을 때 `RMS`, `Peak` 값이 확실히 달라지는지 확인합니다.
