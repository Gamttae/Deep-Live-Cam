/*
 * Item Finder — Arduino Mega 2560
 *
 * 역할: LCD/키패드/서보/부저 제어, Flipper Zero와 UART 통신
 *
 * ── 하드웨어 연결 ─────────────────────────────────────────────────
 *  LCD I2C (20×4)   SDA → 20,  SCL → 21
 *  키패드 4×4       행 → 22-25,  열 → 26-29
 *  연속회전 서보     신호 → 9    (또는 스테퍼 → 30-33)
 *  부저(수동)        → 8
 *  Flipper Zero UART  TX1(18) → Flipper RX(pin 14), RX1(19) → Flipper TX(pin 13)
 *                   ※ 반드시 3.3V↔5V 레벨 시프터 사용!
 *  공통 GND 연결 필수
 *
 * ── UART 프로토콜 ────────────────────────────────────────────────
 *  Arduino → Flipper (명령)
 *    "NFC_READ\n"           NFC 태그 읽기 시작
 *    "BLE_SCAN\n"           가장 가까운 BLE 기기 MAC 반환 (등록용)
 *    "BLE_RSSI:<MAC>\n"     해당 MAC의 RSSI 지속 보고 시작
 *    "BLE_STOP\n"           BLE 스캔 중지
 *
 *  Flipper → Arduino (응답)
 *    "NFC:<uid>\n"          NFC UID (예: "04:A3:2B:C1")
 *    "MAC:<mac>\n"          등록된 BLE MAC (예: "AA:BB:CC:DD:EE:FF")
 *    "RSSI:<dBm>\n"         RSSI 값 (예: "RSSI:-72")
 *    "ERR:<msg>\n"          에러
 *
 * ── 서보 모드 ────────────────────────────────────────────────────
 *  CONTINUOUS_SERVO true  → 연속회전 서보 (90=정지, 0=CCW, 180=CW)
 *  CONTINUOUS_SERVO false → 표준 서보 0-180° (180° 범위만 스캔)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// ── 설정 ────────────────────────────────────────────────────────
#define CONTINUOUS_SERVO  true   // 연속회전 서보 여부
#define SCAN_STEPS        24     // 스캔 분할 수 (360° / SCAN_STEPS)
#define STEP_DELAY_MS     450    // 각 스텝 대기 (서보 이동 + RSSI 안정화)
#define SERVO_PIN         9
#define BUZZER_PIN        8
#define MAX_OBJECTS       8

// ── LCD ─────────────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ── 키패드 ──────────────────────────────────────────────────────
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {26, 27, 28, 29};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ── 서보 ────────────────────────────────────────────────────────
Servo scanServo;

// ── 물건 레지스트리 ──────────────────────────────────────────────
struct TrackedObject {
    char name[16];
    char nfcUID[32];   // NFC UID hex 문자열
    char bleMAC[32];   // BLE 식별자: "AA:BB:CC:DD:EE:FF" 또는 "IF-<이름>" (안드로이드 비콘)
};
TrackedObject objects[MAX_OBJECTS];
int objectCount = 0;
int selectedIdx  = 0;

// ── 스캔 상태 ────────────────────────────────────────────────────
struct ScanPoint { int angle; int rssi; };
ScanPoint scanBuf[SCAN_STEPS];
int  scanIdx    = 0;
int  bestAngle  = 0;
int  bestRSSI   = -999;
int  curRSSI    = -999;

// ── 비프 ────────────────────────────────────────────────────────
unsigned long lastBeepMs  = 0;
int           beepPeriodMs = 1500;

// ── 상태 머신 ────────────────────────────────────────────────────
enum AppState {
    ST_MENU,
    ST_REG_NFC,    // NFC 읽기 대기
    ST_REG_NAME,   // 이름 입력
    ST_REG_BLE,    // BLE MAC 등록 대기
    ST_SELECT,     // 물건 선택
    ST_SWEEP,      // 360° 스캔 중
    ST_POINT       // 방향 고정, 비프 중
};
AppState state = ST_MENU;

// ── 입력 버퍼 ────────────────────────────────────────────────────
char inputBuf[16];
int  inputLen = 0;

// ── Flipper UART 수신 버퍼 ───────────────────────────────────────
char rxBuf[64];
int  rxLen = 0;

// ── 스윕 타이밍 ──────────────────────────────────────────────────
unsigned long sweepTimer  = 0;
bool          waitingRSSI = false;
bool          sweepDone   = false;

// ═══════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);   // Flipper Zero UART

    lcd.init();
    lcd.backlight();

    scanServo.attach(SERVO_PIN);
    servoStop();

    pinMode(BUZZER_PIN, OUTPUT);

    showMenu();
}

void loop() {
    char key = keypad.getKey();
    handleFlipperRx();

    switch (state) {
        case ST_MENU:    onMenu(key);    break;
        case ST_REG_NFC: /* Flipper 응답 대기 */ break;
        case ST_REG_NAME: onNameInput(key); break;
        case ST_REG_BLE: /* Flipper 응답 대기 */ break;
        case ST_SELECT:  onSelect(key);  break;
        case ST_SWEEP:   onSweep();      break;
        case ST_POINT:   onPoint(key);   break;
    }
}

// ── 메뉴 ────────────────────────────────────────────────────────
void showMenu() {
    state = ST_MENU;
    lcd.clear();
    lcdRow(0, "=== Item Finder ===");
    lcdRow(1, "1: 물건 등록");
    lcdRow(2, "2: 물건 찾기");
    char buf[21];
    snprintf(buf, sizeof(buf), "등록된 물건: %d개", objectCount);
    lcdRow(3, buf);
}

void onMenu(char key) {
    if      (key == '1') startRegNFC();
    else if (key == '2') startSelect();
}

// ── 등록: NFC ────────────────────────────────────────────────────
void startRegNFC() {
    if (objectCount >= MAX_OBJECTS) {
        lcdMsg("메모리 가득!", "", "", "");
        delay(1500); showMenu(); return;
    }
    state = ST_REG_NFC;
    lcdMsg("단계 1/3: NFC 등록", "물건에 붙인 NFC 태그를", "Flipper에 태그하세요", "");
    Serial1.println("NFC_READ");
}

void onNFCReceived(const char* uid) {
    strncpy(objects[objectCount].nfcUID, uid, 31);
    inputLen = 0; inputBuf[0] = '\0';
    state = ST_REG_NAME;

    lcd.clear();
    lcdRow(0, "단계 2/3: 이름 입력");
    lcdRow(1, uid);
    lcdRow(2, "이름 입력 후 # 확인");
    lcd.setCursor(0, 3); lcd.print("_");
}

void onNameInput(char key) {
    if (key == NO_KEY) return;
    if (key == '#') {
        if (inputLen == 0) return;
        inputBuf[inputLen] = '\0';
        strncpy(objects[objectCount].name, inputBuf, 15);
        startRegBLE();
    } else if (key == '*') {
        if (inputLen > 0) { inputLen--; inputBuf[inputLen] = '\0'; refreshNameRow(); }
    } else if (inputLen < 13) {
        inputBuf[inputLen++] = key;
        inputBuf[inputLen]   = '\0';
        refreshNameRow();
    }
}

void refreshNameRow() {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print(inputBuf);
    lcd.print("_");
}

void startRegBLE() {
    state = ST_REG_BLE;
    lcdMsg("단계 3/3: BLE 등록", "BLE 태그를 Flipper", "가까이 가져오세요", "");
    Serial1.println("BLE_SCAN");
}

void onBLEMACReceived(const char* mac) {
    strncpy(objects[objectCount].bleMAC, mac, 31);
    objects[objectCount].bleMAC[31] = '\0';
    objectCount++;

    char buf[21];
    lcd.clear();
    lcdRow(0, "등록 완료!");
    lcdRow(1, objects[objectCount - 1].name);
    snprintf(buf, sizeof(buf), "%s", mac);
    lcdRow(2, buf);
    lcdRow(3, "아무 키 → 메뉴");
    delay(2500);
    showMenu();
}

// ── 물건 선택 ────────────────────────────────────────────────────
void startSelect() {
    if (objectCount == 0) {
        lcdMsg("등록된 물건 없음", "먼저 등록하세요", "", "");
        delay(1500); showMenu(); return;
    }
    state = ST_SELECT;
    selectedIdx = 0;
    showSelectPage();
}

void showSelectPage() {
    lcd.clear();
    lcdRow(0, "2/8=이동  #=선택  *=뒤");
    for (int i = 0; i < 3 && i < objectCount; i++) {
        int idx = (selectedIdx / 3) * 3 + i;
        if (idx >= objectCount) break;
        lcd.setCursor(0, 1 + i);
        lcd.print(idx == selectedIdx ? '>' : ' ');
        lcd.print(objects[idx].name);
    }
}

void onSelect(char key) {
    if      (key == '2') { selectedIdx = (selectedIdx + 1) % objectCount; showSelectPage(); }
    else if (key == '8') { selectedIdx = (selectedIdx - 1 + objectCount) % objectCount; showSelectPage(); }
    else if (key == '#') startSweep();
    else if (key == '*') showMenu();
}

// ── 360° 스윕 스캔 ───────────────────────────────────────────────
void startSweep() {
    state       = ST_SWEEP;
    scanIdx     = 0;
    bestRSSI    = -999;
    bestAngle   = 0;
    curRSSI     = -999;
    sweepDone   = false;
    waitingRSSI = false;

    lcd.clear();
    lcdRow(0, "스캔 중...");
    char buf[21];
    snprintf(buf, sizeof(buf), "물건: %s", objects[selectedIdx].name);
    lcdRow(1, buf);

    // 서보 기준 위치 복귀
    servoGoHome();
    delay(700);

    // Flipper에게 추적할 MAC 전달
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "BLE_RSSI:%s", objects[selectedIdx].bleMAC);
    Serial1.println(cmd);
}

void onSweep() {
    if (sweepDone) return;

    if (!waitingRSSI) {
        if (scanIdx < SCAN_STEPS) {
            // 다음 각도로 이동
            int targetDeg = (scanIdx * 360) / SCAN_STEPS;   // 0~345°
            servoGoTo(targetDeg);
            sweepTimer  = millis();
            waitingRSSI = true;

            char buf[21];
            snprintf(buf, sizeof(buf), "각도: %3d°", targetDeg);
            lcdRow(2, buf);
        } else {
            // 스윕 완료
            sweepDone = true;
            Serial1.println("BLE_STOP");
            finishSweep();
        }
    } else {
        // RSSI 응답 타임아웃 처리
        if (millis() - sweepTimer > (unsigned long)(STEP_DELAY_MS + 300)) {
            // 타임아웃: 현재 RSSI 또는 최솟값으로 기록
            recordPoint(scanIdx, curRSSI != -999 ? curRSSI : -100);
            scanIdx++;
            waitingRSSI = false;
        }
    }
}

void onRSSIReceived(int rssi) {
    if (!waitingRSSI) return;
    curRSSI = rssi;
    recordPoint(scanIdx, rssi);
    if (rssi > bestRSSI) { bestRSSI = rssi; bestAngle = (scanIdx * 360) / SCAN_STEPS; }
    scanIdx++;
    waitingRSSI = false;

    char buf[21];
    snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi);
    lcdRow(3, buf);
}

void recordPoint(int idx, int rssi) {
    if (idx < SCAN_STEPS) { scanBuf[idx].angle = (idx * 360) / SCAN_STEPS; scanBuf[idx].rssi = rssi; }
}

void finishSweep() {
    servoGoTo(bestAngle);   // 최강 신호 방향으로 고정

    int dist       = rssiToDistance(bestRSSI);
    beepPeriodMs   = rssiToBeepPeriod(bestRSSI);
    lastBeepMs     = 0;
    state          = ST_POINT;

    lcd.clear();
    char buf[21];
    snprintf(buf, sizeof(buf), "%s", objects[selectedIdx].name);
    lcdRow(0, buf);
    snprintf(buf, sizeof(buf), "방향: %d°", bestAngle);
    lcdRow(1, buf);
    snprintf(buf, sizeof(buf), "거리: ~%dm  %ddBm", dist, bestRSSI);
    lcdRow(2, buf);
    lcdRow(3, "* 종료");
}

// ── 방향 고정 모드 ───────────────────────────────────────────────
void onPoint(char key) {
    // 거리 비프 (RSSI 갱신 시 주기 업데이트)
    if (millis() - lastBeepMs > (unsigned long)beepPeriodMs) {
        lastBeepMs = millis();
        tone(BUZZER_PIN, 1200, 80);
    }
    if (key == '*') { Serial1.println("BLE_STOP"); servoStop(); showMenu(); }
}

// ── Flipper UART 파서 ────────────────────────────────────────────
void handleFlipperRx() {
    while (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n') {
            rxBuf[rxLen] = '\0';
            parseFlipperLine(rxBuf);
            rxLen = 0;
        } else if (rxLen < 62) {
            rxBuf[rxLen++] = c;
        }
    }
}

void parseFlipperLine(const char* line) {
    if      (strncmp(line, "NFC:", 4)  == 0) onNFCReceived(line + 4);
    else if (strncmp(line, "MAC:", 4)  == 0) onBLEMACReceived(line + 4);
    else if (strncmp(line, "RSSI:", 5) == 0) onRSSIReceived(atoi(line + 5));
    else if (strncmp(line, "ERR:", 4)  == 0) {
        lcd.clear();
        lcdRow(0, "Flipper 오류:");
        lcdRow(1, line + 4);
        lcdRow(3, "아무 키 → 메뉴");
        state = ST_MENU;
    }
    // 수신 내용을 시리얼 모니터로도 출력 (디버그)
    Serial.print("[FLIP] "); Serial.println(line);
}

// ── 서보 제어 (연속회전 or 표준) ─────────────────────────────────
/*
 * 연속회전 서보: 90=정지, <90=CCW, >90=CW (속도 비례)
 * 표준 서보: 0~180° 직접 제어
 *
 * 연속회전 서보로 360° 추적 시 시간 기반으로 각도를 추정하는
 * 단순 방식을 사용. 정밀도가 필요하면 스테퍼 모터(28BYJ-48 +
 * ULN2003)로 교체 권장.
 */
static int currentDeg = 0;

void servoGoHome() {
#if CONTINUOUS_SERVO
    currentDeg = 0;
    // 이미 0에 있다고 가정 (전원 인가 시 기준점 필요 시 인코더 추가)
    servoStop();
#else
    scanServo.write(0);
    currentDeg = 0;
#endif
}

void servoGoTo(int targetDeg) {
    targetDeg = ((targetDeg % 360) + 360) % 360;

#if CONTINUOUS_SERVO
    // 현재 각도에서 목표까지 최단 경로 계산
    int delta = targetDeg - currentDeg;
    if (delta > 180)  delta -= 360;
    if (delta < -180) delta += 360;

    // 회전 방향 및 시간 계산 (서보별 조정 필요: ms/degree)
    const int MS_PER_DEG = 6;
    if (delta > 0) {
        scanServo.write(100);                         // CW (느린 속도)
    } else if (delta < 0) {
        scanServo.write(80);                          // CCW
        delta = -delta;
    }
    delay((unsigned long)delta * MS_PER_DEG);
    servoStop();
    currentDeg = targetDeg;
#else
    // 표준 서보: 360° 표현을 0-180°로 매핑 (절반 범위)
    int deg180 = map(targetDeg, 0, 359, 0, 180);
    scanServo.write(deg180);
    currentDeg = targetDeg;
#endif
}

void servoStop() {
#if CONTINUOUS_SERVO
    scanServo.write(90);   // 연속회전 서보 정지
#endif
    // 표준 서보는 write() 후 자동 홀딩
}

// ── 변환 함수 ────────────────────────────────────────────────────
int rssiToDistance(int rssi) {
    // 자유공간 경로손실 모델: d = 10^((A - RSSI) / (10*n))
    // A=-59dBm (1m 기준), n=2
    if (rssi >= -59) return 0;
    float d = pow(10.0f, (-59.0f - (float)rssi) / 20.0f);
    return (int)constrain(d, 0, 99);
}

int rssiToBeepPeriod(int rssi) {
    // -40dBm(가까움)→200ms,  -95dBm(멂)→2000ms
    return (int)constrain(map(rssi, -95, -40, 2000, 200), 200, 2000);
}

// ── LCD 헬퍼 ────────────────────────────────────────────────────
void lcdRow(int row, const char* text) {
    lcd.setCursor(0, row);
    lcd.print("                    ");
    lcd.setCursor(0, row);
    lcd.print(text);
}

void lcdMsg(const char* r0, const char* r1, const char* r2, const char* r3) {
    lcd.clear();
    if (r0[0]) lcdRow(0, r0);
    if (r1[0]) lcdRow(1, r1);
    if (r2[0]) lcdRow(2, r2);
    if (r3[0]) lcdRow(3, r3);
}
