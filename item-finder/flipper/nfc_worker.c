#include "nfc_worker.h"
#include "uart_comm.h"

#include <nfc/nfc.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <furi_hal_gpio.h>
#include <string.h>
#include <stdio.h>

#define TAG        "NfcWorker"
#define POLL_DELAY 200   // ms, 태그 감지 주기

typedef struct {
    ItemFinderApp*       app;
    Nfc*                 nfc;
    Iso14443_3aPoller*   poller;
    volatile bool        running;
    bool                 found;   // 한 번 발견하면 스레드 종료
} NfcWorkerCtx;

// ── 포인터를 TLS처럼 사용 (단일 인스턴스) ───────────────────────
static NfcWorkerCtx* g_ctx = NULL;

// ── ISO14443-3A 폴러 콜백 ───────────────────────────────────────
static NfcCommand nfc_poller_cb(NfcEvent event, void* ctx_ptr) {
    NfcWorkerCtx* ctx = ctx_ptr;

    if (event.type == NfcEventTypePollerReady) {
        // UID 읽기
        const Iso14443_3aData* data = iso14443_3a_poller_get_data(ctx->poller);
        if (!data) return NfcCommandContinue;

        uint8_t uid_len = 0;
        const uint8_t* uid = iso14443_3a_get_uid(data, &uid_len);
        if (!uid || uid_len == 0) return NfcCommandContinue;

        // UID → hex 문자열 "XX:XX:XX:XX"
        char uid_str[32] = {0};
        for (uint8_t i = 0; i < uid_len && i < 10; i++) {
            char part[4];
            snprintf(part, sizeof(part), i < uid_len - 1 ? "%02X:" : "%02X", uid[i]);
            strncat(uid_str, part, sizeof(uid_str) - strlen(uid_str) - 1);
        }

        // Arduino에 전송
        char msg[48];
        snprintf(msg, sizeof(msg), "NFC:%s", uid_str);
        uart_send_line(msg);
        FURI_LOG_I(TAG, "NFC found: %s", uid_str);

        ctx->found   = true;
        ctx->running = false;
        return NfcCommandStop;
    }
    return NfcCommandContinue;
}

// ── 스레드 본체 ─────────────────────────────────────────────────
static int32_t nfc_worker_thread(void* arg) {
    NfcWorkerCtx* ctx = arg;
    FURI_LOG_I(TAG, "NFC worker started");

    ctx->nfc    = nfc_alloc();
    ctx->poller = iso14443_3a_poller_alloc(ctx->nfc);

    iso14443_3a_poller_start(ctx->poller, nfc_poller_cb, ctx);

    // running 플래그가 false가 될 때까지 대기
    while (ctx->running) {
        furi_delay_ms(POLL_DELAY);
    }

    iso14443_3a_poller_stop(ctx->poller);
    iso14443_3a_poller_free(ctx->poller);
    nfc_free(ctx->nfc);

    if (!ctx->found) {
        uart_send_line("ERR:NFC timeout");
    }

    FURI_LOG_I(TAG, "NFC worker stopped");
    return 0;
}

// ────────────────────────────────────────────────────────────────
void nfc_worker_start(ItemFinderApp* app) {
    if (app->nfc_thread && furi_thread_get_state(app->nfc_thread) == FuriThreadStateRunning) {
        FURI_LOG_W(TAG, "NFC worker already running");
        return;
    }

    NfcWorkerCtx* ctx = malloc(sizeof(NfcWorkerCtx));
    ctx->app     = app;
    ctx->running = true;
    ctx->found   = false;
    ctx->nfc     = NULL;
    ctx->poller  = NULL;
    g_ctx        = ctx;

    if (app->nfc_thread) {
        furi_thread_free(app->nfc_thread);
    }
    app->nfc_thread = furi_thread_alloc_ex("NfcWorker", 2048, nfc_worker_thread, ctx);
    furi_thread_start(app->nfc_thread);
}

void nfc_worker_stop(ItemFinderApp* app) {
    if (!app->nfc_thread) return;
    if (g_ctx) g_ctx->running = false;
    furi_thread_join(app->nfc_thread);
    if (g_ctx) { free(g_ctx); g_ctx = NULL; }
}
