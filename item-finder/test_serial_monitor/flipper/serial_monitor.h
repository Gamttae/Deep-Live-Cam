#pragma once

#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── UART 설정 ────────────────────────────────────────────────────
#define SM_UART_CH   FuriHalUartIdUSART1
#define SM_BAUD_RATE 115200

// ── 화면 레이아웃 (128×64) ───────────────────────────────────────
// FontSecondary: 6×8px  → 21자/줄, 8줄
// 상단 1줄 = 상태바, 나머지 7줄 = 내용
#define SM_COLS         21   // 한 줄 최대 문자 수
#define SM_VISIBLE      7    // 내용 표시 줄 수 (상태바 제외)
#define SM_MAX_LINES    80   // 저장할 최대 줄 수 (순환 버퍼)
#define SM_LINE_BUF     (SM_COLS + 1)

// ── 이벤트 타입 ──────────────────────────────────────────────────
typedef enum {
    SmEventUartRx,   // UART 수신 데이터 있음
    SmEventInput,    // 버튼 입력
    SmEventTick,     // 화면 갱신 타이머
} SmEventType;

typedef struct {
    SmEventType  type;
    InputEvent   input;  // SmEventInput일 때만 유효
} SmEvent;

// ── 앱 컨텍스트 ──────────────────────────────────────────────────
typedef struct {
    // 이벤트 큐
    FuriMessageQueue* queue;

    // UART 수신
    FuriStreamBuffer* rx_buf;

    // 줄 버퍼 (순환)
    char  lines[SM_MAX_LINES][SM_LINE_BUF];
    int   write_idx;    // 다음에 쓸 인덱스 (0~SM_MAX_LINES-1)
    int   total;        // 지금까지 저장된 총 줄 수

    // 현재 조립 중인 줄
    char  cur_line[SM_LINE_BUF];
    int   cur_pos;

    // 스크롤
    int   scroll_up;    // 아래서 몇 줄 위로 올렸는지 (0 = 맨 아래)
    bool  auto_scroll;  // true면 새 줄 올 때 자동 스크롤

    // 통계
    uint32_t rx_bytes;
    uint32_t rx_lines;
} SmApp;

// ── 진입점 ───────────────────────────────────────────────────────
int32_t serial_monitor_app(void* p);

#ifdef __cplusplus
}
#endif
