#include "serial_monitor.h"

#include <gui/gui.h>
#include <gui/view_port.h>
#include <furi_hal_uart.h>
#include <string.h>
#include <stdio.h>

#define TAG "SerialMonitor"

// ─────────────────────────────────────────────────────────────────
// UART IRQ (ISR 컨텍스트)
// ─────────────────────────────────────────────────────────────────
static void uart_irq_cb(uint8_t* buf, size_t len, void* ctx) {
    SmApp* app = ctx;
    furi_stream_buffer_send(app->rx_buf, buf, len, 0);

    SmEvent evt = {.type = SmEventUartRx};
    furi_message_queue_put(app->queue, &evt, 0);
}

// ─────────────────────────────────────────────────────────────────
// 줄 버퍼 관리
// ─────────────────────────────────────────────────────────────────

// 현재 줄을 버퍼에 저장하고 초기화
static void commit_line(SmApp* app) {
    if (app->cur_pos == 0) return;  // 빈 줄이면 저장하지 않음
    app->cur_line[app->cur_pos] = '\0';

    strncpy(app->lines[app->write_idx], app->cur_line, SM_COLS);
    app->lines[app->write_idx][SM_COLS] = '\0';
    app->write_idx = (app->write_idx + 1) % SM_MAX_LINES;
    app->total++;
    app->rx_lines++;

    app->cur_pos = 0;
    app->cur_line[0] = '\0';

    if (app->auto_scroll) app->scroll_up = 0;
}

// 수신 바이트 처리: 줄 조립 → commit
static void process_rx_bytes(SmApp* app) {
    uint8_t byte;
    while (furi_stream_buffer_receive(app->rx_buf, &byte, 1, 0) == 1) {
        app->rx_bytes++;

        if (byte == '\n') {
            commit_line(app);
        } else if (byte == '\r') {
            // 무시 (CRLF 처리)
        } else {
            // 표시 불가 제어문자 → '.'로 치환
            if (byte < 0x20 || byte > 0x7E) byte = '.';

            if (app->cur_pos < SM_COLS - 1) {
                app->cur_line[app->cur_pos++] = (char)byte;
            } else {
                // 줄 넘침 → 강제 줄바꿈
                commit_line(app);
                app->cur_line[app->cur_pos++] = (char)byte;
            }
        }
    }
}

// idx번째 줄의 포인터 반환 (total <= SM_MAX_LINES이면 단순 인덱스,
// 이후에는 순환 버퍼 인덱스 계산)
static const char* get_line(SmApp* app, int idx) {
    if (idx < 0 || idx >= app->total) return "";
    int ring_idx;
    if (app->total <= SM_MAX_LINES) {
        ring_idx = idx;
    } else {
        ring_idx = (app->write_idx + idx) % SM_MAX_LINES;
    }
    return app->lines[ring_idx];
}

// ─────────────────────────────────────────────────────────────────
// GUI 렌더링
// ─────────────────────────────────────────────────────────────────
static void draw_cb(Canvas* canvas, void* ctx) {
    SmApp* app = ctx;
    canvas_clear(canvas);

    // ── 상태바 (줄 0, y=8) ───────────────────────────────────
    canvas_set_font(canvas, FontSecondary);
    char status[SM_LINE_BUF + 4];
    snprintf(status, sizeof(status),
             "%s B:%lu L:%lu",
             app->auto_scroll ? "[A]" : "[M]",
             (unsigned long)app->rx_bytes,
             (unsigned long)app->rx_lines);
    canvas_draw_str(canvas, 0, 8, status);

    // 상태바 아래 구분선
    canvas_draw_line(canvas, 0, 9, 127, 9);

    // ── 내용 줄 (y=18 ~ y=64, 7줄) ───────────────────────────
    // 표시할 첫 번째 줄 인덱스
    int first = app->total - SM_VISIBLE - app->scroll_up;
    if (first < 0) first = 0;

    for (int row = 0; row < SM_VISIBLE; row++) {
        int line_idx = first + row;
        if (line_idx >= app->total) break;

        const char* text = get_line(app, line_idx);
        int y = 19 + row * 8;   // 8px 간격
        canvas_draw_str(canvas, 0, y, text);
    }

    // ── 스크롤 인디케이터 ─────────────────────────────────────
    if (app->scroll_up > 0 && app->total > SM_VISIBLE) {
        // 오른쪽 가장자리에 작은 화살표
        canvas_draw_str(canvas, 122, 19, "^");
    }
    if (app->total > SM_VISIBLE + app->scroll_up) {
        canvas_draw_str(canvas, 122, 63, "v");
    }
}

// ─────────────────────────────────────────────────────────────────
// 입력 콜백
// ─────────────────────────────────────────────────────────────────
static void input_cb(InputEvent* event, void* ctx) {
    SmApp* app = ctx;
    SmEvent evt = {.type = SmEventInput, .input = *event};
    furi_message_queue_put(app->queue, &evt, 0);
}

// ─────────────────────────────────────────────────────────────────
// 버튼 처리
// ─────────────────────────────────────────────────────────────────
static bool handle_input(SmApp* app, InputEvent* e) {
    if (e->type != InputTypePress && e->type != InputTypeRepeat) return true;

    switch (e->key) {
        case InputKeyUp:
            // 위로 스크롤 (오래된 줄 보기)
            if (app->total > SM_VISIBLE) {
                int max_scroll = app->total - SM_VISIBLE;
                if (app->scroll_up < max_scroll) {
                    app->scroll_up++;
                    app->auto_scroll = false;
                }
            }
            break;

        case InputKeyDown:
            // 아래로 스크롤
            if (app->scroll_up > 0) {
                app->scroll_up--;
                if (app->scroll_up == 0) app->auto_scroll = true;
            }
            break;

        case InputKeyOk:
            // 자동 스크롤 토글
            app->auto_scroll = !app->auto_scroll;
            if (app->auto_scroll) app->scroll_up = 0;
            break;

        case InputKeyLeft:
            // 화면 지우기
            memset(app->lines, 0, sizeof(app->lines));
            app->write_idx  = 0;
            app->total      = 0;
            app->rx_bytes   = 0;
            app->rx_lines   = 0;
            app->scroll_up  = 0;
            app->auto_scroll= true;
            app->cur_pos    = 0;
            break;

        case InputKeyBack:
            return false;   // 앱 종료

        default:
            break;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────
// 앱 진입점
// ─────────────────────────────────────────────────────────────────
int32_t serial_monitor_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Start");

    // ── 컨텍스트 초기화 ─────────────────────────────────────
    SmApp* app = malloc(sizeof(SmApp));
    memset(app, 0, sizeof(SmApp));
    app->queue       = furi_message_queue_alloc(32, sizeof(SmEvent));
    app->rx_buf      = furi_stream_buffer_alloc(512, 1);
    app->auto_scroll = true;

    // ── UART 초기화 ─────────────────────────────────────────
    furi_hal_console_disable();
    furi_hal_uart_init(SM_UART_CH, SM_BAUD_RATE);
    furi_hal_uart_set_irq_cb(SM_UART_CH, uart_irq_cb, app);
    FURI_LOG_I(TAG, "UART ready at %u baud", SM_BAUD_RATE);

    // ── GUI 초기화 ───────────────────────────────────────────
    Gui*      gui  = furi_record_open(RECORD_GUI);
    ViewPort* vp   = view_port_alloc();
    view_port_draw_callback_set(vp, draw_cb,   app);
    view_port_input_callback_set(vp, input_cb, app);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    // ── 환영 메시지 (Flipper 화면에 표시) ───────────────────
    strncpy(app->lines[0], "=== Serial Monitor ===", SM_COLS);
    strncpy(app->lines[1], "Baud: 115200 / 3-wire", SM_COLS);
    strncpy(app->lines[2], "Up/Dn=scroll  OK=auto", SM_COLS);
    strncpy(app->lines[3], "Left=clear  Back=exit", SM_COLS);
    strncpy(app->lines[4], "Waiting for Arduino...", SM_COLS);
    app->write_idx = 5;
    app->total     = 5;
    app->rx_lines  = 5;

    view_port_update(vp);

    // ── 메인 이벤트 루프 ─────────────────────────────────────
    bool running = true;
    while (running) {
        SmEvent evt;
        FuriStatus st = furi_message_queue_get(app->queue, &evt, 100);

        if (st == FuriStatusOk) {
            switch (evt.type) {
                case SmEventUartRx:
                    process_rx_bytes(app);
                    break;

                case SmEventInput:
                    running = handle_input(app, &evt.input);
                    break;

                case SmEventTick:
                    break;
            }
        } else {
            // 타임아웃마다 미처리 RX 바이트 재처리 (큐 miss 보완)
            process_rx_bytes(app);
        }

        view_port_update(vp);
    }

    // ── 정리 ─────────────────────────────────────────────────
    furi_hal_uart_set_irq_cb(SM_UART_CH, NULL, NULL);
    furi_hal_uart_deinit(SM_UART_CH);
    furi_hal_console_enable();

    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_record_close(RECORD_GUI);

    furi_stream_buffer_free(app->rx_buf);
    furi_message_queue_free(app->queue);
    free(app);

    FURI_LOG_I(TAG, "Exit");
    return 0;
}
