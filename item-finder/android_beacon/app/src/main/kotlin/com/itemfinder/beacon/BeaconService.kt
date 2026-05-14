package com.itemfinder.beacon

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.AdvertiseCallback
import android.bluetooth.le.AdvertiseData
import android.bluetooth.le.AdvertiseSettings
import android.bluetooth.le.BluetoothLeAdvertiser
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.os.ParcelUuid
import android.util.Log
import androidx.core.app.NotificationCompat
import java.util.UUID

/**
 * BLE 비콘 광고 포그라운드 서비스
 *
 * - 광고 페이로드:
 *     · 서비스 UUID: SERVICE_UUID (Item Finder 식별자)
 *     · 디바이스 이름: "IF-<사용자입력>" (스캔 응답에 포함)
 * - Flipper Zero ble_worker는 디바이스 이름 접두사 "IF-" 로 비콘을 식별
 */
class BeaconService : Service() {

    private var advertiser: BluetoothLeAdvertiser? = null
    private var deviceName: String = ""

    private val advertiseCallback = object : AdvertiseCallback() {
        override fun onStartSuccess(settingsInEffect: AdvertiseSettings?) {
            Log.i(TAG, "Advertising started")
            broadcastStatus(running = true)
        }

        override fun onStartFailure(errorCode: Int) {
            Log.e(TAG, "Advertising failed: $errorCode")
            val msg = when (errorCode) {
                ADVERTISE_FAILED_ALREADY_STARTED      -> "이미 광고 중"
                ADVERTISE_FAILED_DATA_TOO_LARGE       -> "광고 데이터가 너무 큼"
                ADVERTISE_FAILED_FEATURE_UNSUPPORTED  -> "기기가 BLE 광고를 지원하지 않음"
                ADVERTISE_FAILED_INTERNAL_ERROR       -> "블루투스 내부 오류"
                ADVERTISE_FAILED_TOO_MANY_ADVERTISERS -> "동시 광고 한도 초과"
                else                                  -> "알 수 없는 오류 ($errorCode)"
            }
            broadcastStatus(running = false, errMsg = msg)
            stopSelf()
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                val name = intent.getStringExtra(EXTRA_NAME).orEmpty().ifBlank { "device" }
                deviceName = name
                startForegroundWithNotification(name)
                startAdvertising(name)
            }
            ACTION_STOP -> {
                stopAdvertising()
                stopForeground(STOP_FOREGROUND_REMOVE)
                stopSelf()
            }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        stopAdvertising()
        broadcastStatus(running = false)
        super.onDestroy()
    }

    // ─────────────────────────────────────────────────────────
    @SuppressLint("MissingPermission")
    private fun startAdvertising(name: String) {
        val bm = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter: BluetoothAdapter? = bm.adapter

        if (adapter == null || !adapter.isEnabled) {
            broadcastStatus(running = false, errMsg = "블루투스가 꺼져 있음")
            stopSelf()
            return
        }
        if (!adapter.isMultipleAdvertisementSupported) {
            broadcastStatus(running = false, errMsg = "BLE 광고 미지원 기기")
            stopSelf()
            return
        }

        // 디바이스 이름을 "IF-<name>" 으로 설정 → 광고에 포함됨
        val fullName = "IF-$name".take(20)   // BT 이름 길이 제한
        try {
            adapter.name = fullName
        } catch (e: SecurityException) {
            Log.w(TAG, "Cannot set bluetooth name: ${e.message}")
        }

        advertiser = adapter.bluetoothLeAdvertiser
        if (advertiser == null) {
            broadcastStatus(running = false, errMsg = "광고기 사용 불가")
            stopSelf()
            return
        }

        val settings = AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
            .setConnectable(false)
            .setTimeout(0)   // 0 = 무제한
            .build()

        // 광고 패킷 (31바이트 한계) — 서비스 UUID만
        val advData = AdvertiseData.Builder()
            .setIncludeTxPowerLevel(true)
            .addServiceUuid(ParcelUuid(SERVICE_UUID))
            .build()

        // 스캔 응답 — 디바이스 이름 (긴 한글 이름 수용)
        val scanResponse = AdvertiseData.Builder()
            .setIncludeDeviceName(true)
            .build()

        advertiser?.startAdvertising(settings, advData, scanResponse, advertiseCallback)
    }

    @SuppressLint("MissingPermission")
    private fun stopAdvertising() {
        try {
            advertiser?.stopAdvertising(advertiseCallback)
        } catch (e: Exception) {
            Log.w(TAG, "stopAdvertising: ${e.message}")
        }
        advertiser = null
    }

    // ── 포그라운드 알림 ───────────────────────────────────────
    private fun startForegroundWithNotification(name: String) {
        val pi = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val notif: Notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .setContentTitle("Item Finder 비콘")
            .setContentText("'IF-$name' 으로 광고 중")
            .setOngoing(true)
            .setContentIntent(pi)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(
                NOTIF_ID,
                notif,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE
            )
        } else {
            startForeground(NOTIF_ID, notif)
        }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val nm = getSystemService(NotificationManager::class.java)
            val ch = NotificationChannel(
                CHANNEL_ID,
                "Item Finder 비콘",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "BLE 비콘 광고 상태"
                setShowBadge(false)
            }
            nm.createNotificationChannel(ch)
        }
    }

    // ── 상태 알림 (Activity 콜백) ─────────────────────────────
    private fun broadcastStatus(running: Boolean, errMsg: String? = null) {
        lastStatus = Triple(running, deviceName, errMsg)
        statusListener?.onStatusChanged(running, deviceName, errMsg)
    }

    // ─────────────────────────────────────────────────────────
    interface StatusListener {
        fun onStatusChanged(running: Boolean, name: String, errMsg: String?)
    }

    companion object {
        private const val TAG        = "BeaconService"
        private const val CHANNEL_ID = "beacon_channel"
        private const val NOTIF_ID   = 1

        // Item Finder 전용 서비스 UUID (고정)
        val SERVICE_UUID: UUID = UUID.fromString("12fe0001-cb0b-4703-bfc1-3940df000001")

        const val ACTION_START = "com.itemfinder.beacon.START"
        const val ACTION_STOP  = "com.itemfinder.beacon.STOP"
        const val EXTRA_NAME   = "name"

        // Activity ↔ Service 상태 공유 (간단한 콜백 방식)
        var statusListener: StatusListener? = null
        var lastStatus: Triple<Boolean, String, String?>? = null
    }
}
