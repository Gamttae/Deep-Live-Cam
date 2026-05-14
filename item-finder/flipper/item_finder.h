#pragma once

#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── UART 채널 (Flipper GPIO 핀 13=TX, 14=RX → USART1) ───────────
#define ITEM_FINDER_UART_CH   FuriHalUartIdUSART1
#define ITEM_FINDER_UART_BAUD 115200

// ── 앱 내부 이벤트 타입 ──────────────────────────────────────────
typedef enum {
    EventTypeUartRx,    // UART 데이터 수신
    EventTypeNfcFound,  // NFC 태그 감지
    EventTypeBleResult, // BLE 스캔 결과
    EventTypeTick,      // 주기 타이머
    EventTypeStop,      // 앱 종료 요청
} ItemFinderEventType;

// ── UART 수신 이벤트 데이터 ──────────────────────────────────────
typedef struct {
    uint8_t data[64];
    size_t  len;
} UartRxData;

// ── BLE 스캔 결과 ─────────────────────────────────────────────────
typedef struct {
    char    mac[18];   // "AA:BB:CC:DD:EE:FF\0"
    int8_t  rssi;
    bool    is_target; // 추적 대상 MAC과 일치 여부
} BleResult;

// ── 통합 이벤트 ──────────────────────────────────────────────────
typedef struct {
    ItemFinderEventType type;
    union {
        UartRxData uart;
        char       nfc_uid[32]; // "04:A3:2B:C1\0"
        BleResult  ble;
    };
} ItemFinderEvent;

// ── 앱 동작 상태 ─────────────────────────────────────────────────
typedef enum {
    AppStateIdle,          // 명령 대기
    AppStateNFCReading,    // NFC 폴링 중
    AppStateBLERegistering,// BLE 스캔 (등록용, 처음 발견 MAC 반환)
    AppStateBLETracking,   // BLE RSSI 지속 보고 중
} AppState;

// ── 앱 컨텍스트 ──────────────────────────────────────────────────
//
// target_id: 추적 중인 BLE 식별자
//   · MAC 형식 ("AA:BB:CC:DD:EE:FF") → MAC 매칭 (고정 MAC BLE 비콘용)
//   · "IF-..." 로 시작하면 → 광고 디바이스 이름 매칭 (안드로이드 비콘용)
//
typedef struct {
    FuriMessageQueue* event_queue;
    FuriStreamBuffer* uart_rx_buf;

    AppState state;
    char     target_id[32]; // 32자: MAC 또는 IF-<한글이름> (UTF-8)

    FuriThread* nfc_thread;
    FuriThread* ble_thread;
} ItemFinderApp;

// ── 전방 선언 ─────────────────────────────────────────────────────
int32_t item_finder_app(void* p);

#ifdef __cplusplus
}
#endif
