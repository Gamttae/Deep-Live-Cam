# Serial Monitor — Flipper Zero 테스트 프로젝트

> Arduino의 시리얼 출력을 Flipper Zero 화면에서 실시간으로 보는 테스트 도구

---

## 목적

Item Finder 본 프로젝트 개발 전에  
**TX / RX / GND 3선만 연결**하여 Arduino ↔ Flipper Zero UART 통신이  
정상인지 확인하는 독립 테스트 프로젝트입니다.

```
컴퓨터 USB                         Flipper Zero 화면
    │                                      │
Arduino Mega ─── TX/RX/GND ─── Flipper Zero
    │                                      │
Arduino IDE                          Serial Monitor 앱
시리얼 모니터                        (이 프로젝트)
```

---

## 배선 (3선)

```
Arduino Mega 2560                    Flipper Zero
  TX1 (18) ──[레벨시프터 HV1→LV1]──→ RX  (GPIO 14 / PB7)
  RX1 (19) ←─[레벨시프터 LV2→HV2]──  TX  (GPIO 13 / PB6)
  GND      ─────────────────────────  GND (GPIO 15 또는 16)

레벨시프터 전원:
  Arduino 5V  → 시프터 HV
  Flipper 3.3V (GPIO 3) → 시프터 LV
```

> ⚠️ Flipper Zero GPIO는 **3.3V 전용**입니다.  
> 레벨 시프터(BSS138, TXS0108E 등) **필수**. 직결 시 Flipper 손상 위험!

### 레벨 시프터 없이 임시 테스트하는 법 (비권장)

Flipper Zero STM32WB55의 RX 핀(GPIO 14 / PB7)은 5V tolerant이므로  
단방향(Arduino TX → Flipper RX)만 연결 시 저항 분압으로 임시 사용 가능합니다:

```
Arduino TX1(18) ─── 2.2kΩ ─┬─── Flipper RX(14)
                             │
                           3.3kΩ
                             │
                            GND
```

반대 방향(Flipper TX → Arduino RX)은 **3.3V 출력이므로 직결 가능**합니다.  
단, 이 방법은 임시방편이며 장시간 사용은 피하세요.

---

## 파일 구조

```
test_serial_monitor/
├── README.md                           ← 이 파일
├── arduino/serial_bridge/
│   └── serial_bridge.ino               ← Arduino 브리지 스케치
└── flipper/
    ├── application.fam                 ← ufbt 빌드 매니페스트
    ├── serial_monitor.h                ← 타입·상수 정의
    └── serial_monitor.c                ← Flipper Zero 시리얼 모니터 앱
```

---

## 설치 방법

### 1. Arduino 스케치 업로드

1. Arduino IDE 에서 `serial_bridge.ino` 열기
2. 보드: **Arduino Mega or Mega 2560**
3. 포트 선택 후 업로드

추가 라이브러리 없음 — 표준 Arduino 함수만 사용합니다.

### 2. Flipper Zero 앱 빌드 & 설치

```bash
# ufbt 설치 (최초 1회)
pip install ufbt

# flipper/ 디렉토리에서 빌드
cd test_serial_monitor/flipper
ufbt

# Flipper Zero를 USB 연결 후 업로드 & 실행
ufbt launch
```

또는 빌드된 `.fap` 파일을 SD카드 `apps/Tools/` 에 복사 후  
Flipper에서 `앱 → Tools → Serial Monitor` 실행.

---

## 사용 방법

### 순서

1. **Flipper Zero** 에서 `Serial Monitor` 앱 실행
   - 화면: `Waiting for Arduino...` 표시
2. **Arduino** 를 컴퓨터 USB에 연결 (스케치 이미 업로드됨)
3. Arduino IDE **시리얼 모니터** 열기 (115200 baud)

   ```
   [Arduino IDE 시리얼 모니터]        [Flipper Zero 화면]
   ━━━━━━━━━━━━━━━━━━━━           ━━━━━━━━━━━━━━━━━━━━━━━━
   === Serial Bridge ===    →     === Serial Bridge ===
   [USB ] 컴퓨터 연결됨            [USB ] 컴퓨터 연결됨
   --- 입력 텍스트가              [UART] Flipper Zero 연결됨
       Flipper에 표시됨           [  3000ms] ping #1
   ```

4. Arduino IDE 시리얼 모니터 **입력창에 텍스트** 입력 → 전송
   → Flipper 화면에 실시간으로 표시됩니다.

5. 매 3초마다 자동으로 핑 메시지가 양쪽에 출력됩니다:

   ```
   [  3000ms] ping #1  free:6823
   [  6000ms] ping #2  free:6823
   ```

---

## Flipper Zero 화면 조작

```
┌──────────────────────────────┐  ← 128×64 화면
│[A] B:1234  L:56              │  ← 상태바 ([A]=자동스크롤, B=바이트, L=줄)
│─────────────────────────────│
│=== Serial Bridge ===         │
│[USB ] 컴퓨터 연결됨          │  7줄 내용 표시
│[UART] Flipper 연결됨         │
│[  3000ms] ping #1  free:6823 │
│[  6000ms] ping #2  free:6823 │
│Hello from PC!                │  ← PC에서 입력한 텍스트
│                              │
└──────────────────────────────┘
```

| 버튼 | 동작 |
|---|---|
| ↑ (위) | 위로 스크롤 (오래된 줄 보기) — 자동 스크롤 해제 |
| ↓ (아래) | 아래로 스크롤 — 맨 아래 도달 시 자동 스크롤 재활성화 |
| OK | 자동 스크롤 ON/OFF 토글 |
| ← (왼쪽) | 화면 내용 전체 지우기 |
| Back | 앱 종료 |

**상태바 표시 의미:**
- `[A]` = 자동 스크롤 활성화 (새 줄 오면 자동으로 아래로)
- `[M]` = 수동 스크롤 모드 (올라간 채로 고정)
- `B:숫자` = 수신한 총 바이트 수
- `L:숫자` = 수신한 총 줄 수

---

## 자동 핑 설정 (serial_bridge.ino)

```cpp
#define PING_INTERVAL_MS 3000   // 3초마다 핑 출력
                                // 0으로 설정하면 핑 비활성화
```

---

## 트러블슈팅

| 증상 | 원인 | 해결 |
|---|---|---|
| Flipper 화면에 아무것도 안 보임 | TX/RX 배선 뒤바뀜 | TX1(18)↔RX, RX1(19)↔TX 확인 |
| 글자 깨짐 (이상한 문자) | 보드레이트 불일치 | 양쪽 모두 115200 확인 |
| 연결 후 바로 글자 나타남 | 정상 동작 | - |
| Flipper 앱 빌드 실패 | SDK 버전 낮음 | 펌웨어 0.87+ 로 업데이트 |
| 레벨 시프터 없이 연결 | 단방향은 저항 분압으로 가능 | 위 임시 방법 참조 |

---

## 테스트 완료 후

이 테스트 프로젝트로 UART 통신이 정상임을 확인했다면,  
본 프로젝트인 **[Item Finder](../README.md)** 로 돌아가세요.

Flipper Zero Serial Monitor 앱 코드(`uart_comm.c`)와  
Arduino 측 코드(`item_finder.ino`)가 같은 UART 채널·프로토콜을 사용합니다.
