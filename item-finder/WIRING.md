# Item Finder — 하드웨어 배선

---

## 현재 연결 방식 (테스트·개발 단계)

> **TX / RX / GND 3선만 연결합니다. 전원선(5V)은 연결하지 않습니다.**

```
Arduino Mega 2560                    Flipper Zero
(USB → 컴퓨터에서 전원)              (자체 배터리로 동작)

  TX1 (18) ──[레벨시프터]──────────→  RX  (GPIO 14 / PB7)
  RX1 (19) ←─[레벨시프터]──────────   TX  (GPIO 13 / PB6)
  GND      ────────────────────────   GND (GPIO 15 or 16)
  (5V 미연결)
```

**전원 공급:**
- Arduino: 컴퓨터 USB 케이블 (5V / ~500mA)
- Flipper Zero: 내장 배터리 (자체 충전)
- 레벨 시프터 저전압측(3.3V): **Flipper 3.3V 핀**(GPIO 3)에서 공급
- 레벨 시프터 고전압측(5V): **Arduino 5V 핀**에서 공급

---

## 향후 전원 공유 계획 (배터리 일체형 케이스)

나중에 케이스 안에 배터리를 공유해야 할 때 아래 선을 추가합니다:

```
공유 배터리 팩 (5V 출력)
    │
    ├── Arduino 5V 핀  (VIN 또는 5V 핀)
    └── Flipper 5V 핀  (GPIO 1)

    ※ 반드시 동일 GND 연결 유지
    ※ Flipper GPIO 1번 핀의 5V는 USB 연결 시에만 출력됨
       → 외부 5V로 Flipper에 전원 공급 시 해당 핀으로 역공급 가능
```

> ⚠️ 전원 공유 전 반드시 멀티미터로 전압 확인 후 연결하세요.

---

## 시스템 블록 다이어그램

```
[키패드 4×4] ──→ Arduino Mega 2560 ←── [LCD I2C 20×4]
                      │    │    │
              [부저]──┘    │    └──[연속회전 서보]
                           │
                    UART (3.3V 레벨 시프터)
                           │
                    Flipper Zero
                    ├── NFC 안테나 (내장)
                    └── BLE (내장 STM32WB55)
```

---

## 1. Arduino Mega 2560 핀 배치

| 모듈          | Arduino 핀        | 비고                          |
|--------------|-------------------|-------------------------------|
| LCD SDA      | 20 (SDA)          | I2C, 4.7kΩ 풀업 (모듈 내장)  |
| LCD SCL      | 21 (SCL)          | I2C                           |
| 키패드 행1-4  | 22, 23, 24, 25    | 내부 풀업 사용                |
| 키패드 열1-4  | 26, 27, 28, 29    |                               |
| 서보 신호     | 9 (PWM)           | 연속회전 서보 권장            |
| 부저          | 8                 | 수동 부저 (tone() 사용)       |
| Flipper TX1  | 18 (TX1)          | → 레벨 시프터 → Flipper RX   |
| Flipper RX1  | 19 (RX1)          | ← 레벨 시프터 ← Flipper TX   |
| GND          | GND               | Flipper GND와 공통 연결 필수  |

---

## 2. Flipper Zero GPIO 핀 배치

```
Flipper Zero 뒷면 GPIO 헤더 (왼쪽→오른쪽)

  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]
  5V   GND  3.3V PA7  PA6  PA4  PB3  PB2

  [9]  [10] [11] [12] [13] [14] [15] [16]
  PC3  PC1  PC0  GND  TX   RX   GND  GND
               (PB6) (PB7)
```

| Flipper 핀  | 연결             | 비고                     |
|-------------|-----------------|--------------------------|
| 13 (TX/PB6) | 레벨 시프터 A   | 3.3V → 5V 변환 후 Arduino RX1(19) |
| 14 (RX/PB7) | 레벨 시프터 B   | 5V → 3.3V 변환 후 Arduino TX1(18) |
| GND (16)    | Arduino GND     | 공통 GND                 |
| 3.3V (3)    | 레벨 시프터 VB  | 시프터 저전압측 전원      |

> ⚠️ **Flipper Zero GPIO는 3.3V 전용입니다.**  
> Arduino Mega는 5V 로직 → **반드시 양방향 레벨 시프터** (예: TXS0108E, BSS138 2개) 사용

---

## 3. 서보 연결

### 연속회전 서보 (권장, 360° 스캔)

| 선색  | 연결처            |
|------|-------------------|
| 빨강  | 5V (Arduino 또는 별도 전원) |
| 갈색  | GND               |
| 주황  | Arduino 9번 핀    |

> `CONTINUOUS_SERVO true` 설정 시 90°=정지, <90°=CCW, >90°=CW

### 표준 서보 대안 (0-180°만 스캔)
`CONTINUOUS_SERVO false`로 설정 → 180° 범위만 스캔

### 스테퍼 모터 대안 (28BYJ-48 + ULN2003, 최고 정밀도)
- 360° 전체 스캔, 각도 오차 없음
- `Stepper` 라이브러리 사용, 핀 30-33 연결
- 코드에서 `servoGoTo()` 함수를 스테퍼 버전으로 교체

---

## 4. BLE 태그 (물건에 부착)

물건에 부착할 BLE 비콘 옵션:

| 옵션             | 특징                                 | 가격    |
|-----------------|--------------------------------------|---------|
| ESP32-C3 미니   | 커스텀 펌웨어 가능, iBeacon 광고     | ~3,000원 |
| nRF52840 Dongle | 저전력, 배터리 오래 감   | ~8,000원 |
| 상용 BLE 비콘    | 버튼 전지형, 바로 사용 가능          | ~5,000원 |
| Tile/AirTag      | ❌ Flipper로 RSSI 스캔 어려움        | -       |

> iBeacon/Eddystone 프로토콜의 BLE 광고를 주기적으로 송신하는 태그라면 모두 사용 가능.

---

## 5. NFC 태그 (물건에 부착)

- **ISO14443-3A (MIFARE Ultralight, NTAG213 등)** 태그 사용
- Flipper Zero로 인식 가능한 모든 수동형 NFC 태그 OK
- 등록 시 태그를 Flipper Zero 뒷면 NFC 안테나(화면 아래쪽)에 3cm 이내로 태그

---

## 6. 전체 작동 흐름

```
[등록]
키패드 '1' 누름
  → Arduino가 "NFC_READ" 전송
  → Flipper가 NFC 폴링 시작
  → 태그 인식 → "NFC:<uid>" 전송
  → LCD에 UID 표시, 이름 입력 요청
  → Arduino가 "BLE_SCAN" 전송
  → Flipper가 가장 강한 BLE 기기 MAC 탐색
  → "MAC:<mac>" 전송
  → 등록 완료 (NFC UID + BLE MAC + 이름 저장)

[찾기]
키패드 '2' 누름 → 물건 선택 → '#' 확인
  → Arduino가 "BLE_RSSI:<mac>" 전송
  → Flipper가 해당 MAC의 RSSI를 주기 보고 ("RSSI:-72")
  → Arduino가 서보를 15°씩 회전하며 각 각도에서 RSSI 기록
  → 24 스텝(360°) 완료 후 최강 RSSI 각도에 서보 고정
  → LCD에 방향·거리 표시, 부저 비프 시작
  → 가까울수록 비프 빨라짐 (200ms ~ 2000ms 주기)
```

---

## 7. 빌드 방법

### Arduino
1. Arduino IDE 2.x 또는 PlatformIO
2. 라이브러리 설치: `LiquidCrystal_I2C`, `Keypad`, `Servo` (라이브러리 매니저)
3. `item-finder/arduino/item_finder/item_finder.ino` 열고 업로드

### Flipper Zero
1. [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) 설치
2. `item-finder/flipper/` 디렉토리에서:
   ```bash
   ufbt          # 빌드
   ufbt launch   # 연결된 Flipper에 업로드 & 실행
   ```
3. 필요 SDK 버전: **SDK 34+ (펌웨어 0.87+)**
