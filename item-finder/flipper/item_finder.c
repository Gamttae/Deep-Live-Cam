#include "item_finder.h"
#include "uart_comm.h"
#include "nfc_worker.h"
#include "ble_worker.h"

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <notification/notification_messages.h>

#define TAG "ItemFinder"

// ── 화면 렌더링 ─────────────────────────────────────────────────
static void draw_callback(Canvas* canvas, void* ctx) {
    ItemFinderApp* app = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    canvas_draw_str(canvas, 2, 10, "Item Finder");
    canvas_set_font(canvas, FontSecondary);

    const char* state_str = "";
    switch (app->state) {
        case AppStateIdle:            state_str = "대기 중 (Arduino 연결)"; break;
        case AppStateNFCReading:      state_str = "NFC 읽는 중...";         break;
        case AppStateBLERegistering:  state_str = "BLE 기기 탐색 중...";    break;
        case AppStateBLETracking:     state_str = "BLE RSSI 보고 중";       break;
    }
    canvas_draw_str(canvas, 2, 24, state_str);

    if (app->state == AppStateBLETracking && app->target_mac[0]) {
        canvas_draw_str(canvas, 2, 36, "추적:");
        canvas_draw_str(canvas, 2, 46, app->target_mac);
    }

    canvas_draw_str(canvas, 2, 60, "Back: 앱 종료");
}

// ── 입력 처리 ───────────────────────────────────────────────────
static void input_callback(InputEvent* event, void* ctx) {
    ItemFinderApp* app = ctx;
    if (event->type == InputTypePress && event->key == InputKeyBack) {
        ItemFinderEvent evt = {.type = EventTypeStop};
        furi_message_queue_put(app->event_queue, &evt, 0);
    }
}

// ── 메인 이벤트 루프 ────────────────────────────────────────────
int32_t item_finder_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "App start");

    // ── 앱 컨텍스트 초기화
    ItemFinderApp* app = malloc(sizeof(ItemFinderApp));
    app->event_queue = furi_message_queue_alloc(16, sizeof(ItemFinderEvent));
    app->state       = AppStateIdle;
    app->nfc_thread  = NULL;
    app->ble_thread  = NULL;
    memset(app->target_mac, 0, sizeof(app->target_mac));

    // ── UART 초기화 (Arduino 통신)
    uart_comm_init(app);

    // ── GUI 설정
    Gui*      gui       = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, app);
    view_port_input_callback_set(view_port, input_callback, app);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // ── 알림 서비스 (LED 등)
    NotificationApp* notif = furi_record_open(RECORD_NOTIFICATION);

    FURI_LOG_I(TAG, "Ready, waiting for Arduino commands");

    // ── 이벤트 루프 ─────────────────────────────────────────────
    bool running = true;
    while (running) {
        // UART 수신 버퍼 파싱 (app->state 갱신 + 이벤트 큐 게시)
        uart_comm_process_rx(app);

        ItemFinderEvent event;
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, 50);

        if (status != FuriStatusOk) {
            // 타임아웃: 화면만 갱신
            view_port_update(view_port);
            continue;
        }

        switch (event.type) {

            case EventTypeStop:
                running = false;
                break;

            case EventTypeUartRx:
                // app->state는 uart_comm_process_rx()에서 이미 갱신됨.
                // 상태에 따라 워커 스레드 시작/중지.

                if (app->state == AppStateNFCReading) {
                    ble_worker_stop(app);
                    nfc_worker_start(app);
                    notification_message(notif, &sequence_blink_blue_10);

                } else if (app->state == AppStateBLERegistering) {
                    nfc_worker_stop(app);
                    ble_worker_start(app);
                    notification_message(notif, &sequence_blink_cyan_10);

                } else if (app->state == AppStateBLETracking) {
                    nfc_worker_stop(app);
                    // BLE 워커가 이미 실행 중이면 target_mac 변경만으로 충분
                    // (ble_worker_thread에서 app->target_mac을 직접 참조)
                    if (!app->ble_thread ||
                        furi_thread_get_state(app->ble_thread) != FuriThreadStateRunning) {
                        ble_worker_start(app);
                    }
                    notification_message(notif, &sequence_blink_green_10);

                } else if (app->state == AppStateIdle) {
                    // BLE_STOP 수신
                    nfc_worker_stop(app);
                    ble_worker_stop(app);
                }

                view_port_update(view_port);
                break;

            default:
                break;
        }
    }

    // ── 정리 ────────────────────────────────────────────────────
    FURI_LOG_I(TAG, "App stopping");

    nfc_worker_stop(app);
    ble_worker_stop(app);
    uart_comm_deinit(app);

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    furi_message_queue_free(app->event_queue);
    free(app);

    FURI_LOG_I(TAG, "App stopped");
    return 0;
}
