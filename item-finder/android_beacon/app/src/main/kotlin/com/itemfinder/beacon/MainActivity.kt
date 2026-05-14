package com.itemfinder.beacon

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.itemfinder.beacon.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val mainHandler = Handler(Looper.getMainLooper())

    private val prefs by lazy { getSharedPreferences(PREFS, MODE_PRIVATE) }

    // ── BeaconService.statusListener 등록용 콜백 ──────────────
    private val statusCallback = object : BeaconService.StatusListener {
        override fun onStatusChanged(running: Boolean, name: String, errMsg: String?) {
            mainHandler.post { updateUi(running, name, errMsg) }
        }
    }

    // ── 권한 요청 런처 ───────────────────────────────────────
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { results ->
        val denied = results.filterValues { !it }.keys
        if (denied.isEmpty()) {
            doStartService()
        } else {
            binding.statusText.text = getString(R.string.error_permission_denied, denied.joinToString())
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.nameInput.setText(prefs.getString(KEY_NAME, "리모컨"))

        binding.startBtn.setOnClickListener { onStartClicked() }
        binding.stopBtn.setOnClickListener { onStopClicked() }

        BeaconService.statusListener = statusCallback
        showInitialStatus()
    }

    override fun onResume() {
        super.onResume()
        // 서비스가 이미 실행 중이면 현재 상태 즉시 반영
        BeaconService.lastStatus?.let { (running, name, err) ->
            updateUi(running, name, err)
        }
    }

    override fun onDestroy() {
        if (BeaconService.statusListener === statusCallback) {
            BeaconService.statusListener = null
        }
        super.onDestroy()
    }

    // ─────────────────────────────────────────────────────────
    private fun showInitialStatus() {
        val bm = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter: BluetoothAdapter? = bm.adapter

        when {
            adapter == null -> {
                binding.statusText.text = getString(R.string.error_no_bluetooth)
                binding.startBtn.isEnabled = false
            }
            !adapter.isEnabled -> {
                binding.statusText.text = getString(R.string.error_bluetooth_off)
            }
            !adapter.isMultipleAdvertisementSupported -> {
                binding.statusText.text = getString(R.string.error_no_advertise_support)
                binding.startBtn.isEnabled = false
            }
            else -> {
                binding.statusText.text = getString(R.string.status_idle)
            }
        }
    }

    private fun onStartClicked() {
        val name = binding.nameInput.text.toString().trim()
        if (name.isEmpty()) {
            binding.statusText.text = getString(R.string.error_empty_name)
            return
        }
        prefs.edit().putString(KEY_NAME, name).apply()

        val perms = requiredPermissions()
        val notGranted = perms.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        if (notGranted.isEmpty()) {
            doStartService()
        } else {
            permissionLauncher.launch(notGranted.toTypedArray())
        }
    }

    private fun onStopClicked() {
        startService(Intent(this, BeaconService::class.java).apply {
            action = BeaconService.ACTION_STOP
        })
    }

    private fun doStartService() {
        val name = binding.nameInput.text.toString().trim()
        val intent = Intent(this, BeaconService::class.java).apply {
            action = BeaconService.ACTION_START
            putExtra(BeaconService.EXTRA_NAME, name)
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent)
        } else {
            startService(intent)
        }
    }

    private fun updateUi(running: Boolean, name: String, errMsg: String?) {
        binding.startBtn.isEnabled = !running
        binding.stopBtn.isEnabled = running
        binding.nameInput.isEnabled = !running

        binding.statusText.text = when {
            errMsg != null -> getString(R.string.status_error, errMsg)
            running        -> getString(R.string.status_running, "IF-$name")
            else           -> getString(R.string.status_idle)
        }
    }

    // ── 권한 목록 (API 버전별) ───────────────────────────────
    private fun requiredPermissions(): List<String> {
        val list = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            list += Manifest.permission.BLUETOOTH_ADVERTISE
            list += Manifest.permission.BLUETOOTH_CONNECT
        } else {
            list += Manifest.permission.ACCESS_FINE_LOCATION
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            list += Manifest.permission.POST_NOTIFICATIONS
        }
        return list
    }

    companion object {
        private const val PREFS    = "beacon_prefs"
        private const val KEY_NAME = "device_name"
    }
}
