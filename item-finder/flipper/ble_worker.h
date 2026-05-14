#pragma once

#include "item_finder.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BLE 스캔 워커
 *
 * 두 가지 모드:
 *   1. 등록 모드 (AppStateBLERegistering)
 *      - 가장 강한 신호의 BLE 기기 MAC을 찾아 "MAC:<mac>\n" 전송
 *      - app->target_mac == "" 상태
 *
 *   2. 추적 모드 (AppStateBLETracking)
 *      - app->target_mac에 지정된 MAC의 RSSI를 주기적으로
 *        "RSSI:<dBm>\n" 형식으로 전송
 *
 * BLE 스캔 API: furi_hal_bt_start_scan / furi_hal_bt_stop_scan
 * (Flipper Zero SDK 34+, 펌웨어 0.87+)
 */

// BLE 스캔 워커 스레드 시작
void ble_worker_start(ItemFinderApp* app);

// BLE 스캔 워커 스레드 중지
void ble_worker_stop(ItemFinderApp* app);

#ifdef __cplusplus
}
#endif
