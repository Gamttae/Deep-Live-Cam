# Item Finder — 치매·건망증 보조 물건 찾기 기기

> **방향 + 거리 + 인지 보조**를 카메라 없이 풀어낸 저가 치매 보조 도구.  
> 상용 트래커(소리만)와 카메라 기반 위치 시스템 사이의 빈틈을 채웁니다.

---

## 목차

1. [프로젝트 개요](#1-프로젝트-개요)
2. [차별점](#2-차별점)
3. [하드웨어 목록](#3-하드웨어-목록)
4. [시스템 구조](#4-시스템-구조)
5. [배선](#5-배선)
6. [소프트웨어 구조](#6-소프트웨어-구조)
7. [빌드 및 설치](#7-빌드-및-설치)
8. [사용 방법](#8-사용-방법)
9. [UART 통신 프로토콜](#9-uart-통신-프로토콜)
10. [트러블슈팅](#10-트러블슈팅)

---

## 1. 프로젝트 개요

치매·건망증 환자가 리모컨, 안경, 지갑, 약통 같은 물건을 잃었을 때  
**방향(모터가 가리킴) + 거리(비프 주기)** 두 가지 단서를 동시에 제공하는 탁상형 보조 기기입니다.

### 작동 원리

```
[등록 단계]
물건에 NFC 태그 + BLE 비콘 부착
→ 기기에서 키패드로 물건 이름 입력
→ NFC UID + BLE MAC 저장

[찾기 단계]
키패드로 물건 선택
→ Flipper Zero가 서보 위에서 360° 회전하며 BLE RSSI 측정
→ 신호 가장 강한 방향으로 서보 고정 (물건 방향 지시)
→ LCD에 방향·거리 표시
→ 부저 비프: 가까울수록 빨라짐 (2000ms → 200ms)
```

---

## 2. 차별점

| 비교 대상 | 문제점 | Item Finder |
|---|---|---|
| Tile / AirTag | 소리만 남 → 환자가 소리 따라가다 혼란 | 방향(모터) + 거리(비프) 동시 제공 |
| 카메라 기반 시스템 | 노인 프라이버시 거부감 높음 | 카메라 없음 |
| 스마트폰 앱 | 노인이 사용하기 어려운 UI | 큰 키패드 + LCD 단순 인터페이스 |

---

## 3. 하드웨어 목록

### 메인 표시기 (하나의 케이스에 모두 내장)

| 부품 | 모델 예시 | 역할 |
|---|---|---|
| 메인 컨트롤러 | Arduino Mega 2560 | 전체 제어, LCD·서보·부저·키패드 구동 |
| NFC + BLE 처리기 | Flipper Zero | NFC 태그 읽기, BLE 신호 스캔 및 RSSI 보고 |
| 화면 | LCD I2C 20×4 | 메뉴·물건 이름·방향·거리 표시 |
| 입력 | 4×4 키패드 (큰 캡 권장) | 물건 선택, 이름 입력 |
| 방향 지시 모터 | 연속회전 서보 (MG996R 등) | Flipper Zero를 얹어 360° 회전 |
| 거리 알림 | 수동 부저 (passive buzzer) | 비프 주기로 거리 표현 |
| 전압 변환 | 양방향 레벨 시프터 | Flipper 3.3V ↔ Arduino 5V 변환 |
| 전원 | 5V 2A USB 어댑터 or 배터리팩 | |

### 물건에 부착하는 태그 (개당 1세트)

| 부품 | 역할 |
|---|---|
| NFC 스티커 태그 (NTAG213/215) | 등록 시 물건 식별 |
| BLE 비콘 (ESP32-C3 / 상용 비콘) | 실시간 위치 신호 송출 |

---

## 4. 시스템 구조

```
┌──────────────────────────────────────────────────┐
│                  메인 표시기 케이스                │
│                                                  │
│  [키패드 4×4]                                    │
│       │                                          │
│       ▼                                          │
│  [Arduino Mega 2560]──I2C──[LCD 20×4]            │
│       │         │                                │
│       │         └──PWM──[연속회전 서보]           │
│       │                      │                  │
│       │               (Flipper Zero 얹힘)        │
│       │                      │                  │
│   Serial1 (UART)             │ NFC 안테나 (내장) │
│  + 레벨시프터                 │ BLE 안테나 (내장) │
│       │                      │                  │
│  [Flipper Zero]──────────────┘                   │
│       │                                          │
│  [수동 부저]                                      │
└──────────────────────────────────────────────────┘
                           ↕ BLE (무선)
              [물건에 부착된 BLE 비콘 + NFC 태그]
```

---

## 5. 배선

### Arduino Mega 2560 핀 연결

| 모듈 | Arduino 핀 | 비고 |
|---|---|---|
| LCD SDA | 20 | I2C (모듈 내 풀업 내장) |
| LCD SCL | 21 | I2C |
| 키패드 행1~4 | 22, 23, 24, 25 | |
| 키패드 열1~4 | 26, 27, 28, 29 | |
| 서보 신호선 | 9 (PWM) | |
| 부저 | 8 | |
| Flipper TX → Arduino RX1 | 19 | 레벨 시프터 경유 |
| Flipper RX ← Arduino TX1 | 18 | 레벨 시프터 경유 |
| 공통 GND | GND | Flipper GND와 반드시 연결 |

### Flipper Zero GPIO 핀 (뒷면 헤더)

```
핀 번호:  [1]  [2]  [3]  [4] ... [13] [14] [15] [16]
기능:     5V  GND  3.3V ...      TX   RX   GND  GND
                                (PB6)(PB7)
```

| Flipper 핀 | 연결 대상 | 설명 |
|---|---|---|
| 13 (TX / PB6) | 레벨 시프터 → Arduino RX1(19) | Flipper→Arduino 송신 |
| 14 (RX / PB7) | 레벨 시프터 ← Arduino TX1(18) | Arduino→Flipper 수신 |
| GND | Arduino GND | 공통 기준 전위 |

> ⚠️ **Flipper Zero GPIO는 3.3V 전용입니다.**  
> Arduino Mega는 5V 로직이므로 **양방향 레벨 시프터** (TXS0108E, BSS138 등) 없이 직결하면 Flipper가 손상됩니다.

### 레벨 시프터 연결 예시 (BSS138 2채널)

```
Arduino 5V  ──── HV      LV ──── Flipper 3.3V
Arduino GND ──── GND    GND ──── Flipper GND
Arduino TX1(18) ─ HV1   LV1 ──── Flipper RX(14)
Arduino RX1(19) ─ HV2   LV2 ──── Flipper TX(13)
```

### 서보 연결

```
서보 빨간선 ──── 5V (외부 전원 권장, 전류 충분히)
서보 갈색선 ──── GND
서보 주황선 ──── Arduino 9번 핀
```

> Flipper Zero를 서보 상단 브래킷에 고정합니다.  
> 서보가 회전하면 Flipper Zero도 함께 돌며 BLE 안테나가 각 방향을 가리킵니다.

---

## 6. 소프트웨어 구조

```
item-finder/
├── README.md                       ← 이 파일
├── MANUAL_KO.md                    ← 상세 사용 설명서 (한국어)
├── WIRING.md                       ← 배선 빠른 참조
│
├── arduino/item_finder/
│   └── item_finder.ino             ← Arduino Mega 메인 스케치
│
└── flipper/
    ├── application.fam             ← ufbt 빌드 매니페스트
    ├── item_finder.h               ← 공용 타입·상수 정의
    ├── item_finder.c               ← 앱 진입점·이벤트 루프·GUI
    ├── uart_comm.h / uart_comm.c   ← UART 통신 레이어
    ├── nfc_worker.h / nfc_worker.c ← NFC 폴링 워커 스레드
    └── ble_worker.h / ble_worker.c ← BLE GAP 스캔 워커 스레드
```

### 모듈별 역할

| 파일 | 역할 |
|---|---|
| `item_finder.ino` | 상태 머신 (등록/찾기), LCD·키패드·서보·부저 제어, Flipper UART 파서 |
| `item_finder.c` | Flipper 앱 이벤트 루프, NFC/BLE 워커 시작·중지, GUI 렌더링 |
| `uart_comm.c` | IRQ 기반 UART 수신, 명령 파싱, Arduino로 응답 전송 |
| `nfc_worker.c` | ISO14443-3A 폴링, UID 감지 후 `NFC:<uid>` 전송 |
| `ble_worker.c` | BLE GAP 스캔, 등록 모드(MAC 탐색) / 추적 모드(RSSI 보고) |

---

## 7. 빌드 및 설치

### Arduino 업로드

**필요 라이브러리** (Arduino 라이브러리 매니저에서 설치):
- `LiquidCrystal_I2C` (johnrickman 버전)
- `Keypad` (Mark Stanley, Alexander Brevig)
- `Servo` (Arduino 내장)

```
1. Arduino IDE 2.x 실행
2. [스케치] → [라이브러리 포함하기] → [라이브러리 관리] 에서 위 라이브러리 설치
3. item-finder/arduino/item_finder/item_finder.ino 열기
4. 보드: "Arduino Mega or Mega 2560" 선택
5. 포트 선택 후 업로드
```

**서보 모드 설정** (`item_finder.ino` 상단):
```cpp
#define CONTINUOUS_SERVO  true   // 연속회전 서보 → true (기본값, 360° 스캔)
                                 // 표준 서보(0-180°) → false
```

### Flipper Zero 앱 빌드

**요구 사항**: Flipper Zero 펌웨어 **0.87 이상** (SDK 34+)

```bash
# ufbt 설치 (Python 3.8+)
pip install ufbt

# 빌드
cd item-finder/flipper
ufbt

# Flipper Zero를 USB로 연결 후 업로드 & 실행
ufbt launch
```

빌드된 `.fap` 파일은 Flipper의 SD카드 `apps/Tools/` 폴더에도 수동 복사 가능합니다.

---

## 8. 사용 방법

자세한 사용법은 **[MANUAL_KO.md](./MANUAL_KO.md)** 를 참조하세요.

**요약**:
1. Flipper Zero를 서보 브래킷에 장착
2. Flipper Zero에서 `앱 → Tools → Item Finder` 실행
3. Arduino에 전원 인가 → LCD 메인 화면 표시
4. `1` 누름 → 물건 등록 (NFC 태그 → 이름 입력 → BLE 등록)
5. `2` 누름 → 물건 선택 → `#` → 360° 스캔 → 방향·거리 표시

---

## 9. UART 통신 프로토콜

Arduino ↔ Flipper Zero 간 115200 baud, `\n` 종단 텍스트 프로토콜

| 방향 | 명령 | 의미 |
|---|---|---|
| Arduino → Flipper | `NFC_READ` | NFC 폴링 시작 |
| Arduino → Flipper | `BLE_SCAN` | 가장 강한 BLE 기기 MAC 탐색 (등록용) |
| Arduino → Flipper | `BLE_RSSI:AA:BB:CC:DD:EE:FF` | 해당 MAC RSSI 주기 보고 시작 |
| Arduino → Flipper | `BLE_STOP` | 스캔 중지 |
| Flipper → Arduino | `NFC:04:A3:2B:C1` | NFC UID 감지 |
| Flipper → Arduino | `MAC:AA:BB:CC:DD:EE:FF` | 등록된 BLE MAC |
| Flipper → Arduino | `RSSI:-72` | 현재 RSSI 값 (dBm) |
| Flipper → Arduino | `ERR:메시지` | 오류 |

---

## 10. 트러블슈팅

| 증상 | 원인 | 해결 방법 |
|---|---|---|
| LCD 아무것도 안 보임 | I2C 주소 불일치 | 주소 `0x27` → `0x3F` 로 변경 시도 |
| Flipper에서 응답 없음 | UART 배선 TX/RX 뒤바뀜 | TX1↔RX1 선 바꿔 연결 |
| NFC 인식 안 됨 | 태그가 ISO14443-3A 비호환 | NTAG213/215 태그 사용 확인 |
| BLE 기기 못 찾음 | 비콘 광고 주기가 너무 김 | 비콘 광고 주기 1초 이하로 설정 |
| 서보가 안 돌거나 오작동 | 전원 부족 | 서보에 별도 5V 2A 이상 공급 |
| 방향 정밀도 낮음 | 연속회전 서보 속도 보정 필요 | `MS_PER_DEG` 값 조정 (기본 6ms/°) |
| Flipper 앱 빌드 오류 | SDK 버전 낮음 | 펌웨어 0.87+ 업데이트 후 재빌드 |

---

## 라이선스

MIT License — 자유롭게 수정·배포 가능합니다.
