# Lissence BLE 프로토콜

## 개요

Lissence 하드웨어와 iPhone 앱은 BLE를 통해 짧은 상태 메시지와 햅틱 명령을 주고받습니다.

| 구분 | 역할 | 방향 | 통신 방식 |
| --- | --- | --- | --- |
| ESP32-WROOM-32E | BLE Peripheral | ESP32 -> iPhone | Notify |
| iPhone 앱 | BLE Central | iPhone -> ESP32 | Write / Write Without Response |

1차 프로토콜은 BLE 연결, notify, write 검증을 목표로 합니다. 상태/명령 메시지는 사람이 읽기 쉽고 디버깅하기 쉬운 UTF-8 JSON String을 사용합니다. PCM 오디오 스트리밍 PoC는 BLE packet 크기를 고려해 binary notify packet을 사용합니다.

## Device

| 항목 | 값 |
| --- | --- |
| BLE 이름 | `Lissence-ESP32` |
| Peripheral | ESP32-WROOM-32E |
| Central | iPhone 앱 |

## GATT

### Lissence Service

| 항목 | 값 |
| --- | --- |
| Service UUID | `7d2f3a10-3b7a-4f9f-9b37-6b6a0f4f7c10` |

### Data Characteristic

| 항목 | 값 |
| --- | --- |
| Characteristic UUID | `7d2f3a11-3b7a-4f9f-9b37-6b6a0f4f7c10` |
| Properties | `Read`, `Write`, `Write Without Response`, `Notify` |

## 메시지 인코딩

- 모든 payload는 UTF-8 JSON String입니다.
- 상태 메시지와 write command는 UTF-8 JSON String입니다.
- PCM audio stream notify만 binary packet입니다.
- 배터리 사용량, 지연시간, BLE packet 크기가 문제가 되면 이후 binary protocol을 검토합니다.
- 현재 PCM audio stream은 SoundAnalysis 연결 전 chunk reconstruction 안정성 검증용 PoC입니다.

## ESP32 -> iPhone Notify 메시지

ESP32는 iPhone이 notify를 subscribe한 뒤 상태와 이벤트를 Characteristic notify로 전달합니다.

| type | 목적 | 주요 필드 |
| --- | --- | --- |
| `test` | 연결 검증용 주기 메시지 | `seq` |
| `mic_level` | ESP32에서 측정한 마이크 입력 레벨 | `rms`, `peak` |
| `audio_candidate` | ESP32 lightweight 후보 감지 이벤트 | `kind`, `rms`, `peak` |
| `mic_event` | ESP32 마이크 기반 이벤트 알림 | `event`, `level` |
| `battery` | 배터리 잔량 알림 | `level` |
| `status` | 장치 상태 알림 | `state` |
| `error` | ESP32 처리 오류 알림 | `code`, `message` |

### Notify 예시

```json
{"type":"test","seq":1}
```

```json
{"type":"mic_level","rms":1234,"peak":8912}
```

```json
{"type":"audio_candidate","kind":"loud_sound","rms":12345,"peak":67890}
```

```json
{"type":"mic_event","event":"loud_sound","level":0.82}
```

```json
{"type":"battery","level":87}
```

```json
{"type":"status","state":"ready"}
```

```json
{"type":"error","code":"sensor_unavailable","message":"INMP441 is not ready"}
```

## ESP32 -> iPhone PCM Audio Stream Binary Notify

PCM audio stream은 continuous streaming이 아니라 iPhone의 write command로 시작되는 3초 테스트 스트리밍입니다. 목적은 raw audio 분석이 아니라 BLE packet 분할, 수신, chunk reconstruction 안정성 검증입니다.

### Audio 설정

| 항목 | 값 |
| --- | --- |
| Source | INMP441 I2S microphone |
| Sample rate | 16 kHz |
| Channel | Mono |
| PCM format | 16-bit signed little-endian |
| Chunk duration | 20 ms |
| Samples per chunk | 320 samples |
| PCM bytes per chunk | 640 bytes |

### Binary packet header

각 20ms chunk는 BLE MTU를 고려해 여러 notify packet으로 분할됩니다. 모든 정수는 little-endian입니다.

| Offset | Type | Field | 설명 |
| --- | --- | --- | --- |
| 0 | `uint8` | `magic` | audio packet 식별자, `0xA1` |
| 1 | `uint16` | `sequence` | 20ms PCM chunk sequence |
| 3 | `uint8` | `packetIndex` | chunk 안의 packet index, 0부터 시작 |
| 4 | `uint8` | `packetCount` | 해당 chunk를 구성하는 전체 packet 수 |
| 5 | `uint16` | `payloadSize` | 이 packet의 PCM payload byte 수 |
| 7 | bytes | `payload` | 16-bit signed PCM little-endian 일부 |

현재 ESP32 PoC는 MTU 강제 튜닝 없이 packet payload를 최대 160 bytes로 제한합니다. 640 byte chunk는 보통 4개 packet으로 전송됩니다. BLE notify queue 과부하를 줄이기 위해 packet 사이에 약 3ms pacing delay를 둡니다. Audio streaming 중에는 같은 characteristic을 사용하는 `test`, `mic_level` JSON notify를 잠시 중단하고, streaming 종료 후 재개합니다.

### Audio Stream 제어 command

3초 테스트 스트리밍은 iPhone write command로 시작하고, 3초가 지나면 ESP32에서 자동 종료합니다. 필요하면 중지 command로 즉시 멈춥니다.

```json
{"type":"config","audio_stream":true}
```

```json
{"type":"config","audio_stream":false}
```

## iPhone -> ESP32 Write 메시지

iPhone 앱은 같은 Data Characteristic에 write하여 ESP32로 명령을 전달합니다. Characteristic이 `Write Without Response`를 지원하면 우선 사용하고, 필요 시 `Write`를 사용합니다.

| type | 목적 | 주요 필드 |
| --- | --- | --- |
| `haptic` | 햅틱 패턴 실행 요청 | `pattern`, `intensity` |
| `stop` | 현재 동작 중지 요청 | 없음 |
| `config` | 임계값 등 설정 변경 | 설정별 key |
| `ping` | 연결 상태 확인 | `seq` |

### Haptic Pattern

| pattern | 용도 |
| --- | --- |
| `siren` | 사이렌 계열 위험 감지 햅틱 |
| `fireAlarm` | 화재 경보 계열 위험 감지 햅틱 |
| `carHorn` | 차량 경적 계열 위험 감지 햅틱 |
| `speech` | 사람 말/음성 계열 알림 햅틱 |
| `warning` | 일반 위험 경고 |
| `test` | DRV2605L/LRA 동작 확인 |
| `urgent` | 긴급 위험 경고 |
| `calm` | 낮은 강도의 안정 패턴 |
| `pulse` | 단순 pulse 패턴 |
| `music_beat` | 음악 beat 기반 패턴 |
| `music_happy` | happy mood 기반 음악 햅틱 |
| `music_sad` | sad mood 기반 음악 햅틱 |
| `music_angry` | angry mood 기반 음악 햅틱 |
| `music_relaxed` | relaxed mood 기반 음악 햅틱 |

### Write 예시

```json
{"type":"haptic","pattern":"warning"}
```

```json
{"type":"haptic","pattern":"siren"}
```

```json
{"type":"haptic","pattern":"fireAlarm"}
```

```json
{"type":"haptic","pattern":"carHorn"}
```

```json
{"type":"haptic","pattern":"test"}
```

```json
{"type":"haptic","pattern":"music_happy","intensity":0.72}
```

```json
{"type":"stop"}
```

```json
{"type":"config","mic_threshold":0.65}
```

```json
{"type":"config","audio_stream":true}
```

```json
{"type":"config","audio_stream":false}
```

```json
{"type":"ping","seq":10}
```

## DRV2605L 햅틱 제어

현재 ESP32-WROOM-32E 펌웨어는 BLE `haptic` write command를 받으면 BLE callback 안에서 직접 모터를 구동하지 않고, pending pattern으로 저장한 뒤 main `loop()`에서 DRV2605L effect sequence를 실행합니다. BLE stack이 I2C 통신이나 `delay()`로 막히지 않게 하기 위한 구조입니다.

### DRV2605L 배선

| DRV2605L | ESP32-WROOM-32E |
| --- | --- |
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| OUT+ / OUT- | LRA motor |

### DRV2605L 설정

| 항목 | 값 |
| --- | --- |
| I2C address | `0x5A` |
| Motor type | LRA |
| Library | `6` |
| Mode | Internal trigger |

## 현재 버전에서 하지 않을 것

- Continuous raw audio BLE streaming
- ESP32에서 CoreML inference 실행
- Apple Watch의 ESP32 직접 BLE 연결
- DRV2605L effect UX 최적화
- iPhone SoundAnalysis와 PCM stream 직접 연결

DRV2605L 기반 햅틱 실행은 현재 구분 가능한 기본 effect sequence 검증 단계입니다.

## 검증 결과

| 항목 | 결과 |
| --- | --- |
| ESP32 BLE advertising | 확인 |
| nRF Connect connect | 확인 |
| nRF Connect notify subscribe | 확인 |
| nRF Connect write | 확인 |
| iPhone 앱 BLE 테스트 화면 scan/connect | 확인 |
| iPhone 앱 BLE 테스트 화면 notify 수신 | 확인 |
| iPhone 앱 BLE 테스트 화면 write 전송 | 확인 |
| ESP32 Serial Monitor write 수신 | 확인 |

### 검증된 값

| 항목 | 값 |
| --- | --- |
| Device name | `Lissence-ESP32` |
| Service UUID | `7d2f3a10-3b7a-4f9f-9b37-6b6a0f4f7c10` |
| Characteristic UUID | `7d2f3a11-3b7a-4f9f-9b37-6b6a0f4f7c10` |
| Characteristic properties | `Read`, `Write`, `Write Without Response`, `Notify` |
| ESP32 -> iPhone notify | 성공 |
| iPhone -> ESP32 write | 성공 |

### 테스트 payload

ESP32 -> iPhone notify:

```json
{"type":"test","seq":1}
```

```json
{"type":"mic_level","rms":1234,"peak":8912}
```

iPhone -> ESP32 write:

```json
{"type":"haptic","pattern":"warning"}
```

## 앞으로의 단계

1. iPhone 앱 BLE 테스트 화면에서 `mic_level` 수신값 확인
2. PCM audio stream binary notify packet reconstruction 안정성 검증
3. ESP32에서 INMP441 기반 `mic_event` notify 구현
4. DRV2605L `haptic` pattern UX 조정
5. 음악모드 `currentMood`를 `haptic` write payload로 연결
6. 배터리 측정 회로 추가 후 `battery` notify 구현
