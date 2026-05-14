#include "ble_worker.h"
#include "uart_comm.h"

#include <furi_hal_bt.h>
#include <furi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TAG              "BleWorker"
#define RSSI_REPORT_MS   350    // RSSI 보고 주기 (ms)
#define REG_SCAN_SEC     10     // 등록 모드 스캔 최대 대기 시간 (초)
#define RSSI_MIN_VALID   -100   // 유효 RSSI 하한 (dBm)
#define IF_PREFIX        "IF-"  // 안드로이드 비콘 이름 접두사

// ── 내부 컨텍스트 ───────────────────────────────────────────────
typedef struct {
    ItemFinderApp* app;
    volatile bool  running;

    // 등록 모드용: 가장 강한 IF- 비콘 (또는 fallback MAC)
    char    best_id[32];    // "IF-리모컨" 또는 "AA:BB:CC:DD:EE:FF"
    int8_t  best_rssi;
    bool    found_if_beacon;

    FuriMutex* result_mutex;

    // 추적 모드용
    int8_t  target_rssi;
} BleWorkerCtx;

static BleWorkerCtx* g_ctx = NULL;

// ── MAC 바이트 배열 → "AA:BB:CC:DD:EE:FF" 변환 ─────────────────
static void mac_to_str(const uint8_t* addr, char* out) {
    snprintf(
        out, 18,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

// ── BLE 광고 데이터에서 디바이스 이름 파싱 ─────────────────────
// LTV (Length-Type-Value) 구조:
//   [len][type][value...]
//   type 0x09 = Complete Local Name
//   type 0x08 = Shortened Local Name
//
// 안드로이드 비콘은 스캔 응답에 이름을 포함하므로,
// Flipper의 furi_hal_bt 스캐너가 광고+스캔응답을 합쳐 전달한다고 가정.
//
static bool parse_device_name(const uint8_t* data, uint8_t len, char* out, size_t out_size) {
    if (!data || len == 0) return false;

    size_t i = 0;
    while (i < len) {
        uint8_t field_len = data[i];
        if (field_len == 0) break;
        if (i + field_len >= len) break;

        uint8_t field_type = data[i + 1];
        if (field_type == 0x09 || field_type == 0x08) {
            size_t name_len = field_len - 1;
            if (name_len >= out_size) name_len = out_size - 1;
            memcpy(out, &data[i + 2], name_len);
            out[name_len] = '\0';
            return true;
        }
        i += field_len + 1;
    }
    return false;
}

// ── BLE 스캔 콜백 (BLE 스택 컨텍스트) ───────────────────────────
static void ble_scan_cb(BleGapScanResult* result, void* ctx_ptr) {
    BleWorkerCtx* ctx = ctx_ptr;
    if (!ctx->running || !result) return;

    char    mac[18];
    char    name[32] = {0};
    int8_t  rssi    = result->rssi;
    bool    has_name = false;

    mac_to_str(result->addr, mac);
    has_name = parse_device_name(result->data, result->len, name, sizeof(name));
    bool is_if_beacon = has_name && strncmp(name, IF_PREFIX, 3) == 0;

    furi_mutex_acquire(ctx->result_mutex, FuriWaitForever);

    if (ctx->app->state == AppStateBLERegistering) {
        // ── 등록 모드 ─────────────────────────────────────────
        // 우선순위 1: IF- 안드로이드 비콘 (이름 기반)
        // 우선순위 2: 다른 BLE 기기 (MAC 기반, IF- 비콘이 없을 때만)
        if (is_if_beacon) {
            if (!ctx->found_if_beacon || rssi > ctx->best_rssi) {
                ctx->best_rssi = rssi;
                ctx->found_if_beacon = true;
                strncpy(ctx->best_id, name, 31);
                ctx->best_id[31] = '\0';
            }
        } else if (!ctx->found_if_beacon && rssi > ctx->best_rssi) {
            ctx->best_rssi = rssi;
            strncpy(ctx->best_id, mac, 31);
        }

    } else if (ctx->app->state == AppStateBLETracking) {
        // ── 추적 모드 ─────────────────────────────────────────
        // target_id가 "IF-"로 시작하면 이름 매칭, 아니면 MAC 매칭
        bool match = false;
        if (strncmp(ctx->app->target_id, IF_PREFIX, 3) == 0) {
            match = has_name && strcmp(name, ctx->app->target_id) == 0;
        } else {
            match = strcmp(mac, ctx->app->target_id) == 0;
        }
        if (match) {
            ctx->target_rssi = rssi;
        }
    }

    furi_mutex_release(ctx->result_mutex);
}

// ── 스레드 본체 ─────────────────────────────────────────────────
static int32_t ble_worker_thread(void* arg) {
    BleWorkerCtx* ctx = arg;
    FURI_LOG_I(TAG, "BLE worker started, mode=%d", ctx->app->state);

    furi_hal_bt_stop_advertising();
    furi_delay_ms(100);
    furi_hal_bt_start_scan(ble_scan_cb, ctx);

    uint32_t start_tick = furi_get_tick();

    while (ctx->running) {
        furi_delay_ms(RSSI_REPORT_MS);

        furi_mutex_acquire(ctx->result_mutex, FuriWaitForever);

        if (ctx->app->state == AppStateBLERegistering) {
            if (ctx->best_rssi > RSSI_MIN_VALID && ctx->best_id[0] != '\0') {
                char msg[64];
                snprintf(msg, sizeof(msg), "MAC:%s", ctx->best_id);
                uart_send_line(msg);
                FURI_LOG_I(TAG, "Registered: %s (%d dBm)%s",
                           ctx->best_id, ctx->best_rssi,
                           ctx->found_if_beacon ? " [IF beacon]" : "");
                ctx->running = false;
            } else {
                uint32_t elapsed_ms =
                    (furi_get_tick() - start_tick) * 1000 / furi_kernel_get_tick_frequency();
                if (elapsed_ms > (uint32_t)(REG_SCAN_SEC * 1000)) {
                    uart_send_line("ERR:BLE no device found");
                    ctx->running = false;
                }
            }

        } else if (ctx->app->state == AppStateBLETracking) {
            if (ctx->target_rssi > RSSI_MIN_VALID) {
                char msg[16];
                snprintf(msg, sizeof(msg), "RSSI:%d", (int)ctx->target_rssi);
                uart_send_line(msg);
            }
        }

        furi_mutex_release(ctx->result_mutex);
    }

    furi_hal_bt_stop_scan();
    furi_hal_bt_start_advertising();

    FURI_LOG_I(TAG, "BLE worker stopped");
    return 0;
}

// ────────────────────────────────────────────────────────────────
void ble_worker_start(ItemFinderApp* app) {
    if (app->ble_thread &&
        furi_thread_get_state(app->ble_thread) == FuriThreadStateRunning) {
        FURI_LOG_W(TAG, "BLE worker already running");
        return;
    }

    BleWorkerCtx* ctx = malloc(sizeof(BleWorkerCtx));
    ctx->app             = app;
    ctx->running         = true;
    ctx->best_rssi       = RSSI_MIN_VALID;
    ctx->target_rssi     = RSSI_MIN_VALID;
    ctx->found_if_beacon = false;
    ctx->result_mutex    = furi_mutex_alloc(FuriMutexTypeNormal);
    memset(ctx->best_id, 0, sizeof(ctx->best_id));
    g_ctx                = ctx;

    if (app->ble_thread) furi_thread_free(app->ble_thread);
    app->ble_thread = furi_thread_alloc_ex("BleWorker", 2048, ble_worker_thread, ctx);
    furi_thread_start(app->ble_thread);
}

void ble_worker_stop(ItemFinderApp* app) {
    if (!app->ble_thread) return;
    if (g_ctx) {
        g_ctx->running = false;
        furi_thread_join(app->ble_thread);
        furi_mutex_free(g_ctx->result_mutex);
        free(g_ctx);
        g_ctx = NULL;
    }
}
