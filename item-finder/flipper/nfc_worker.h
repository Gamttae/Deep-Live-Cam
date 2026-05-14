#pragma once

#include "item_finder.h"

#ifdef __cplusplus
extern "C" {
#endif

// NFC 폴링 스레드 시작 (UID 감지 시 "NFC:<uid>\n" 전송 후 자동 종료)
void nfc_worker_start(ItemFinderApp* app);

// NFC 폴링 스레드 중지 (이미 종료됐으면 no-op)
void nfc_worker_stop(ItemFinderApp* app);

#ifdef __cplusplus
}
#endif
