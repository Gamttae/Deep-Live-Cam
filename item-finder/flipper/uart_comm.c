#include "uart_comm.h"
#include <furi_hal_uart.h>
#include <string.h>
#include <stdlib.h>

#define TAG "UartComm"

// ── UART IRQ 콜백 (ISR 컨텍스트) ────────────────────────────────
static void uart_irq_cb(uint8_t* buf, size_t len, void* ctx) {
    ItemFinderApp* app = ctx;
    // 스트림 버퍼로 복사 (main loop에서 파싱)
    furi_stream_buffer_send(app->uart_rx_buf, buf, len, 0);

    // 이벤트 큐에 알림 (큐가 꽉 찼을 때 drop)
    ItemFinderEvent evt = {.type = EventTypeUartRx};
    furi_message_queue_put(app->event_queue, &evt, 0);
}

// ────────────────────────────────────────────────────────────────
void uart_comm_init(ItemFinderApp* app) {
    // 스트림 버퍼: 256바이트
    app->uart_rx_buf = furi_stream_buffer_alloc(256, 1);

    furi_hal_console_disable();  // 콘솔과 USART1 충돌 방지
    furi_hal_uart_init(ITEM_FINDER_UART_CH, ITEM_FINDER_UART_BAUD);
    furi_hal_uart_set_irq_cb(ITEM_FINDER_UART_CH, uart_irq_cb, app);

    FURI_LOG_I(TAG, "UART initialized at %u baud", ITEM_FINDER_UART_BAUD);
}

void uart_comm_deinit(ItemFinderApp* app) {
    furi_hal_uart_set_irq_cb(ITEM_FINDER_UART_CH, NULL, NULL);
    furi_hal_uart_deinit(ITEM_FINDER_UART_CH);
    furi_hal_console_enable();
    furi_stream_buffer_free(app->uart_rx_buf);
}

void uart_send_line(const char* str) {
    furi_hal_uart_tx(ITEM_FINDER_UART_CH, (const uint8_t*)str, strlen(str));
    furi_hal_uart_tx(ITEM_FINDER_UART_CH, (const uint8_t*)"\n", 1);
    FURI_LOG_D(TAG, "TX: %s", str);
}

// ── 수신 줄 파싱 → ItemFinderEvent 게시 ─────────────────────────
//
// Arduino가 보내는 명령:
//   "NFC_READ"       → AppStateNFCReading 전환, NFC 폴링 시작
//   "BLE_SCAN"       → AppStateBLERegistering 전환
//   "BLE_RSSI:<MAC>" → AppStateBLETracking 전환, MAC 저장
//   "BLE_STOP"       → AppStateIdle 전환
//
// 이 함수는 app->state를 직접 변경한다 (이벤트가 아닌 직접 처리).
// NFC/BLE 스레드 시작·중지도 여기서 수행.
void uart_comm_process_rx(ItemFinderApp* app) {
    static char line_buf[64];
    static int  line_len = 0;

    uint8_t byte;
    while (furi_stream_buffer_receive(app->uart_rx_buf, &byte, 1, 0) == 1) {
        if (byte == '\n' || byte == '\r') {
            if (line_len == 0) continue;
            line_buf[line_len] = '\0';
            FURI_LOG_D(TAG, "RX cmd: %s", line_buf);

            // ── 명령 해석 ──────────────────────────────────────
            if (strcmp(line_buf, "NFC_READ") == 0) {
                app->state = AppStateNFCReading;
                // NFC 스레드 시작 요청 → main loop에서 처리하도록 이벤트 게시
                ItemFinderEvent evt = {.type = EventTypeUartRx};
                furi_message_queue_put(app->event_queue, &evt, 0);

            } else if (strcmp(line_buf, "BLE_SCAN") == 0) {
                app->state = AppStateBLERegistering;
                memset(app->target_mac, 0, sizeof(app->target_mac));
                ItemFinderEvent evt = {.type = EventTypeUartRx};
                furi_message_queue_put(app->event_queue, &evt, 0);

            } else if (strncmp(line_buf, "BLE_RSSI:", 9) == 0) {
                app->state = AppStateBLETracking;
                strncpy(app->target_mac, line_buf + 9, 17);
                app->target_mac[17] = '\0';
                ItemFinderEvent evt = {.type = EventTypeUartRx};
                furi_message_queue_put(app->event_queue, &evt, 0);

            } else if (strcmp(line_buf, "BLE_STOP") == 0) {
                app->state = AppStateIdle;
                ItemFinderEvent evt = {.type = EventTypeStop};
                furi_message_queue_put(app->event_queue, &evt, 0);
            }

            line_len = 0;
        } else if (line_len < 62) {
            line_buf[line_len++] = (char)byte;
        }
    }
}
