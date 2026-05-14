#pragma once

#include "item_finder.h"

#ifdef __cplusplus
extern "C" {
#endif

// ── UART 초기화 / 해제 ───────────────────────────────────────────
void uart_comm_init(ItemFinderApp* app);
void uart_comm_deinit(ItemFinderApp* app);

// ── Arduino로 줄 전송 ("...\n" 자동 추가) ──────────────────────
void uart_send_line(const char* str);

// ── 수신 버퍼에서 한 줄 파싱 후 이벤트 큐 게시 ─────────────────
// (main loop 혹은 IRQ 컨텍스트에서 호출)
void uart_comm_process_rx(ItemFinderApp* app);

#ifdef __cplusplus
}
#endif
