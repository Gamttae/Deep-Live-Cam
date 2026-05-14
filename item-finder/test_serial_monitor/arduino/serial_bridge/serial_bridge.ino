/*
 * Serial Bridge — Arduino Mega 2560 테스트 스케치
 *
 * 역할:
 *   - Serial  (USB,  핀 0/1)  : 컴퓨터 Arduino IDE 시리얼 모니터
 *   - Serial1 (UART, 핀 18/19): Flipper Zero 시리얼 모니터 앱
 *
 * 두 채널을 양방향으로 연결해서:
 *   → 컴퓨터에서 입력한 글자가 Flipper 화면에 보임
 *   → Flipper 버튼 입력이 컴퓨터 시리얼 모니터에 보임
 *   → Arduino 자체 디버그 출력도 양쪽 모두에 표시됨
 *
 * 배선 (TX/RX/GND 3선):
 *   Arduino TX1(18) → 레벨시프터 → Flipper RX (GPIO 14)
 *   Arduino RX1(19) ← 레벨시프터 ← Flipper TX (GPIO 13)
 *   Arduino GND     ────────────── Flipper GND (GPIO 15/16)
 *
 * ※ Flipper 3.3V (GPIO 3)  → 레벨시프터 LV 전원
 * ※ Arduino  5V            → 레벨시프터 HV 전원
 */

#define BAUD_RATE 115200

// ── 자동 핑 설정 ─────────────────────────────────────────────────
#define PING_INTERVAL_MS 3000   // 자동 상태 출력 주기 (ms), 0이면 비활성화
static unsigned long lastPingMs = 0;
static unsigned long pingCount  = 0;

// ── 브리지 버퍼 ──────────────────────────────────────────────────
#define BRIDGE_BUF 64
static char bridgeBuf[BRIDGE_BUF];

void setup() {
    Serial.begin(BAUD_RATE);    // USB ↔ 컴퓨터
    Serial1.begin(BAUD_RATE);   // UART ↔ Flipper Zero

    // 양쪽에 시작 메시지 출력
    const char* header =
        "============================\r\n"
        "  Arduino Serial Bridge\r\n"
        "  Baud: 115200  3-wire UART\r\n"
        "============================\r\n";

    Serial.print(header);
    Serial1.print(header);

    Serial.println("[USB ] 컴퓨터 시리얼 모니터 연결됨");
    Serial1.println("[UART] Flipper Zero 연결됨");

    Serial.println("--- 입력한 텍스트가 Flipper 화면에 표시됩니다 ---");
    Serial1.println("--- Arduino Serial Bridge ready ---");
}

void loop() {
    // ── USB(컴퓨터) → Serial1(Flipper) 브리지 ─────────────────
    while (Serial.available()) {
        int c = Serial.read();
        Serial1.write((char)c);
    }

    // ── Serial1(Flipper) → USB(컴퓨터) 브리지 ─────────────────
    while (Serial1.available()) {
        int c = Serial1.read();
        Serial.write((char)c);
    }

    // ── 자동 핑 (상태 확인용 주기 출력) ───────────────────────
#if PING_INTERVAL_MS > 0
    if (millis() - lastPingMs >= PING_INTERVAL_MS) {
        lastPingMs = millis();
        pingCount++;

        char buf[48];
        snprintf(buf, sizeof(buf),
                 "[%7lums] ping #%lu  free:%d\r\n",
                 millis(), pingCount, freeMemory());

        Serial.print(buf);
        Serial1.print(buf);
    }
#endif
}

// ── 여유 메모리 측정 (디버그용) ──────────────────────────────────
extern unsigned int __heap_start;
extern void*        __brkval;

int freeMemory() {
    int free_memory;
    if ((int)__brkval == 0) {
        free_memory = ((int)&free_memory) - ((int)&__heap_start);
    } else {
        free_memory = ((int)&free_memory) - ((int)__brkval);
    }
    return free_memory;
}
