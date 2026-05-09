# Lissence BLE 프로토콜

## 개요

Lissence 하드웨어와 iPhone 앱은 BLE를 통해 짧은 상태 메시지와 햅틱 명령을 주고받습니다.

| 구분 | 역할 | 방향 | 통신 방식 |
| --- | --- | --- | --- |
| ESP32-C3 | BLE Peripheral | ESP32 -> iPhone | Notify |
| iPhone 앱 | BLE Central | iPhone -> ESP32 | Write / Write Without Response |

1차 프로토콜은 BLE 연결, notify, write 검증을 목표로 합니다. 메시지는 사람이 읽기 쉽고 디버깅하기 쉬운 UTF-8 JSON String을 사용합니다.

## Device

| 항목 | 값 |
| --- | --- |
| BLE 이름 | `Lissence-ESP32` |
| Peripheral | ESP32-C3 |
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
- 1차 구현에서는 사람이 읽기 쉬운 JSON을 사용합니다.
- 배터리 사용량, 지연시간, BLE packet 크기가 문제가 되면 이후 binary protocol을 검토합니다.
- 현재 단계에서는 raw audio를 BLE로 streaming하지 않습니다.

## ESP32 -> iPhone Notify 메시지

ESP32는 iPhone이 notify를 subscribe한 뒤 상태와 이벤트를 Characteristic notify로 전달합니다.

| type | 목적 | 주요 필드 |
| --- | --- | --- |
| `test` | 연결 검증용 주기 메시지 | `seq` |
| `mic_level` | ESP32에서 측정한 마이크 입력 레벨 | `rms`, `peak` |
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
| `warning` | 일반 위험 경고 |
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
{"type":"haptic","pattern":"music_happy","intensity":0.72}
```

```json
{"type":"stop"}
```

```json
{"type":"config","mic_threshold":0.65}
```

```json
{"type":"ping","seq":10}
```

## 현재 버전에서 하지 않을 것

- Raw audio BLE streaming
- ESP32에서 CoreML inference 실행
- Apple Watch의 ESP32 직접 BLE 연결
- DRV2605L 제어 구현

DRV2605L 기반 햅틱 실행은 하드웨어 도착 후 다음 단계에서 구현합니다.

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
2. ESP32에서 INMP441 기반 `mic_event` notify 구현
3. DRV2605L 도착 후 `haptic` command 실행
4. 음악모드 `currentMood`를 `haptic` write payload로 연결
5. 배터리 측정 회로 추가 후 `battery` notify 구현
