#include "ble_worker.h"
#include "uart_comm.h"

#include <furi_hal_bt.h>
#include <furi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TAG              "BleWorker"
#define RSSI_REPORT_MS   350    // RSSI 보고 주기 (ms) — 스윕 STEP_DELAY_MS보다 짧아야 함
#define REG_SCAN_SEC     10     // 등록 모드 스캔 최대 대기 시간 (초)
#define RSSI_MIN_VALID   -100   // 유효 RSSI 하한 (dBm)

// ── 내부 컨텍스트 ───────────────────────────────────────────────
typedef struct {
    ItemFinderApp* app;
    volatile bool  running;

    // 등록 모드용: 가장 강한 기기 추적
    char    best_mac[18];
    int8_t  best_rssi;
    FuriMutex* result_mutex;

    // 추적 모드용: target_mac의 최근 RSSI
    int8_t  target_rssi;
} BleWorkerCtx;

static BleWorkerCtx* g_ctx = NULL;

// ── MAC 바이트 배열 → "AA:BB:CC:DD:EE:FF" 변환 ─────────────────
static void mac_to_str(const uint8_t* addr, char* out) {
    // Flipper Zero BLE 주소 바이트 순서: addr[5]..addr[0] (little-endian)
    snprintf(
        out, 18,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

// ── BLE 스캔 콜백 (ISR 또는 BLE 스택 컨텍스트) ──────────────────
//
// SDK 34+: furi_hal_bt_start_scan(callback, ctx)
// 콜백 인자: BleGapScanResult* result
//   result->addr  uint8_t[6] (LE byte order)
//   result->rssi  int8_t
//
static void ble_scan_cb(BleGapScanResult* result, void* ctx_ptr) {
    BleWorkerCtx* ctx = ctx_ptr;
    if (!ctx->running || !result) return;

    char mac[18];
    mac_to_str(result->addr, mac);
    int8_t rssi = result->rssi;

    furi_mutex_acquire(ctx->result_mutex, FuriWaitForever);

    if (ctx->app->state == AppStateBLERegistering) {
        // 등록 모드: 가장 강한 신호 기기 업데이트
        if (rssi > ctx->best_rssi) {
            ctx->best_rssi = rssi;
            strncpy(ctx->best_mac, mac, 17);
        }

    } else if (ctx->app->state == AppStateBLETracking) {
        // 추적 모드: 대상 MAC과 일치하면 RSSI 저장
        if (strncmp(mac, ctx->app->target_mac, 17) == 0) {
            ctx->target_rssi = rssi;
        }
    }

    furi_mutex_release(ctx->result_mutex);
}

// ── 스레드 본체 ─────────────────────────────────────────────────
static int32_t ble_worker_thread(void* arg) {
    BleWorkerCtx* ctx = arg;
    FURI_LOG_I(TAG, "BLE worker started, mode=%d", ctx->app->state);

    // 광고 중이면 중지 후 스캔 모드 진입
    furi_hal_bt_stop_advertising();
    furi_delay_ms(100);
    furi_hal_bt_start_scan(ble_scan_cb, ctx);

    uint32_t start_tick = furi_get_tick();

    while (ctx->running) {
        furi_delay_ms(RSSI_REPORT_MS);

        furi_mutex_acquire(ctx->result_mutex, FuriWaitForever);

        if (ctx->app->state == AppStateBLERegistering) {
            // 등록 모드: 유효 결과가 있으면 전송 후 종료
            if (ctx->best_rssi > RSSI_MIN_VALID && ctx->best_mac[0] != '\0') {
                char msg[32];
                snprintf(msg, sizeof(msg), "MAC:%s", ctx->best_mac);
                uart_send_line(msg);
                FURI_LOG_I(TAG, "Registered: %s (%d dBm)", ctx->best_mac, ctx->best_rssi);
                ctx->running = false;
            } else {
                // 타임아웃 체크
                uint32_t elapsed_ms = (furi_get_tick() - start_tick) * 1000 / furi_kernel_get_tick_frequency();
                if (elapsed_ms > (uint32_t)(REG_SCAN_SEC * 1000)) {
                    uart_send_line("ERR:BLE no device found");
                    ctx->running = false;
                }
            }

        } else if (ctx->app->state == AppStateBLETracking) {
            // 추적 모드: 현재 RSSI 주기 보고
            if (ctx->target_rssi > RSSI_MIN_VALID) {
                char msg[16];
                snprintf(msg, sizeof(msg), "RSSI:%d", (int)ctx->target_rssi);
                uart_send_line(msg);
            }
        }

        furi_mutex_release(ctx->result_mutex);
    }

    furi_hal_bt_stop_scan();
    // 광고 재시작 (Flipper BLE 키보드 등 기능 복원)
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
    ctx->app          = app;
    ctx->running      = true;
    ctx->best_rssi    = RSSI_MIN_VALID;
    ctx->target_rssi  = RSSI_MIN_VALID;
    ctx->result_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    memset(ctx->best_mac, 0, sizeof(ctx->best_mac));
    g_ctx             = ctx;

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
