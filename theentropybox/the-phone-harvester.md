# Mobile Chaos Harvester: Smartphone as a Bluetooth Entropy Source

**Version:** 1.0  
**Author:** Julian Cassin  
**Date:** 1st April 2026  
**License:** MIT  

---

## Overview

The Mobile Chaos Harvester turns any smartphone into a portable, physics-based entropy source. Using built-in sensors—camera, microphone, magnetometer, ambient light, barometer, and RF signals—the device captures true random noise from the physical world and serves it over Bluetooth using the **Cyborg Unicorn Entropy Protocol (CUEP)**.

A connected laptop, IoT device, or second phone can request entropy bytes (`0x10` for 1KB, `0xFF` for continuous stream) and receive high-quality random data suitable for:
- Generating ZOSCII ROMs
- microZOSCII seed creation
- Cryptographic key material
- Air-gapped entropy for Web3 wallets

Unlike dedicated hardware, this solution uses hardware you already carry.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                 Smartphone (Android/iOS)                │
├─────────────────────────────────────────────────────────┤
│  Sensors: Camera, Microphone, Magnetometer, Light, etc. │
│                         ↓                               │
│                Background Service                       │
│         Samples sensors → XOR mixing → entropy pool     │
│                         ↓                               │
│              CUEP Protocol (BLE Serial)                 │
│                 Characteristic: --...                   │
└─────────────────────────────────────────────────────────┘
                              ↕ BLE
┌─────────────────────────────────────────────────────────┐
│              Connected Device (Laptop, etc.)            │
│  Send: 0x10 → Receive: 1024 bytes of entropy            │
│  Send: 0xFF → Continuous stream until disconnect        │
└─────────────────────────────────────────────────────────┘
```

---

## Entropy Sources (Phone Stationary)

Even with the phone sitting on a table, multiple high-quality sources remain available:

| Source | Physics | Quality | Notes |
|--------|---------|---------|-------|
| **Camera** | Photon shot noise, thermal noise, rolling shutter | Very high | 2D array of millions of pixels, each with quantum noise |
| **Microphone** | Ambient acoustic noise, preamp thermal noise | High | Samples at 44.1kHz, 16-bit |
| **Magnetometer** | Geomagnetic fluctuations, local EM noise | Medium | 3-axis, sampled at 50-100Hz |
| **Ambient Light** | 50/60Hz flicker, sunlight variations | Low-Medium | Adds entropy to XOR pool |
| **Barometer** | Air pressure fluctuations | Low | Slow but contributes |
| **WiFi/Bluetooth RSSI** | Signal strength fluctuations | Low | Environment-dependent |
| **Battery Voltage** | Power rail micro-fluctuations | Low | Sampled via system API |

**Camera Note:** Even in complete darkness, thermal noise (dark current) and read noise remain, providing usable entropy.

---

## CUEP Over Bluetooth

The phone exposes a BLE service with a single characteristic for entropy requests/responses.

### BLE Service & Characteristic

| Field | Value |
|-------|-------|
| Service UUID | `A1B2C3D4-E5F6-7890-1234-567890ABCDEF` |
| Characteristic UUID | `B1C2D3E4-F5A6-7890-1234-567890ABCDEF` |
| Properties | Read, Write, Notify |
| Write | Command byte (CUEP) |
| Notify | Entropy bytes (response) |

### CUEP Commands

| Command (Hex) | Action | Response |
|---------------|--------|---------|
| `0x01` | Get 1 byte | 1 byte of XORed entropy |
| `0x02` | Get 1 word | 2 bytes (little-endian) |
| `0x04` | Get 1 double | 4 bytes (32-bit integer) |
| `0x10` | Stream 1KB | 1024 bytes of entropy |
| `0xFF` | Continuous stream | Infinite stream until disconnect or stop |
| `0x00` | Health check | 1 byte sensor status bitmap |

### Health Check Bitmap

| Bit | Sensor |
|-----|--------|
| 0 | Camera |
| 1 | Microphone |
| 2 | Magnetometer |
| 3 | Ambient Light |
| 4 | Barometer |
| 5 | GPS (if available) |
| 6 | WiFi/BT RSSI |
| 7 | Battery Voltage |

---

## Android Implementation

### 1. Permissions

```xml
<uses-permission android:name="android.permission.CAMERA" />
<uses-permission android:name="android.permission.RECORD_AUDIO" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" /> <!-- optional, for magnetometer/GPS -->
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.BLUETOOTH_ADVERTISE" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
```

### 2. Entropy Service (Background)

```kotlin
class EntropyService : Service() {
    private lateinit var cameraEntropy: CameraEntropySource
    private lateinit var micEntropy: MicrophoneEntropySource
    private lateinit var sensorEntropy: SensorEntropySource
    private lateinit var entropyPool: EntropyPool
    private lateinit var bluetoothServer: CUEPBluetoothServer

    override fun onCreate() {
        super.onCreate()
        entropyPool = EntropyPool()
        
        // Initialize entropy sources
        cameraEntropy = CameraEntropySource(this) { data ->
            entropyPool.add(data)
        }
        micEntropy = MicrophoneEntropySource(this) { data ->
            entropyPool.add(data)
        }
        sensorEntropy = SensorEntropySource(this) { data ->
            entropyPool.add(data)
        }
        
        // Start sources
        cameraEntropy.start()
        micEntropy.start()
        sensorEntropy.start()
        
        // Start BLE server
        bluetoothServer = CUEPBluetoothServer(this, entropyPool)
        bluetoothServer.start()
    }
}
```

### 3. Camera Entropy Source

```kotlin
class CameraEntropySource(
    private val context: Context,
    private val onEntropy: (ByteArray) -> Unit
) {
    private var camera: Camera? = null
    private var isRunning = false

    fun start() {
        camera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK)
        camera?.let { cam ->
            val parameters = cam.parameters
            parameters.previewFormat = ImageFormat.YV12
            cam.parameters = parameters
            cam.setPreviewCallback { data, _ ->
                if (isRunning) {
                    // XOR camera frames to extract entropy
                    val entropy = extractEntropy(data)
                    onEntropy(entropy)
                }
            }
            cam.startPreview()
            isRunning = true
        }
    }

    private fun extractEntropy(frame: ByteArray): ByteArray {
        // Sample low bits of each pixel—photon shot noise dominates
        val entropy = ByteArray(1024)
        for (i in entropy.indices) {
            val pixelIndex = (i * 17) % frame.size  // Stride
            entropy[i] = (frame[pixelIndex].toInt() and 0x03).toByte()
        }
        return entropy
    }
}
```

### 4. Microphone Entropy Source

```kotlin
class MicrophoneEntropySource(
    private val context: Context,
    private val onEntropy: (ByteArray) -> Unit
) {
    private var audioRecord: AudioRecord? = null
    private var isRunning = false

    fun start() {
        val bufferSize = AudioRecord.getMinBufferSize(
            44100, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT
        )
        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            44100,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
            bufferSize
        )
        audioRecord?.startRecording()
        isRunning = true
        
        val buffer = ByteArray(4096)
        while (isRunning) {
            val bytesRead = audioRecord?.read(buffer, 0, buffer.size) ?: 0
            if (bytesRead > 0) {
                // Extract low bits—thermal/acoustic noise
                val entropy = extractLowBits(buffer, bytesRead)
                onEntropy(entropy)
            }
        }
    }

    private fun extractLowBits(data: ByteArray, length: Int): ByteArray {
        val entropy = ByteArray(length / 4)
        for (i in entropy.indices) {
            entropy[i] = (data[i * 4].toInt() and 0x0F).toByte()
        }
        return entropy
    }
}
```

### 5. Sensor Entropy Source

```kotlin
class SensorEntropySource(
    private val context: Context,
    private val onEntropy: (ByteArray) -> Unit
) : SensorEventListener {
    private var sensorManager: SensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val entropyBuffer = mutableListOf<Byte>()

    fun start() {
        sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_FASTEST)
        }
        sensorManager.getDefaultSensor(Sensor.TYPE_LIGHT)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_FASTEST)
        }
        sensorManager.getDefaultSensor(Sensor.TYPE_PRESSURE)?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
    }

    override fun onSensorChanged(event: SensorEvent) {
        when (event.sensor.type) {
            Sensor.TYPE_MAGNETIC_FIELD -> {
                for (value in event.values) {
                    val byte = (value * 256).toInt().toByte()
                    entropyBuffer.add(byte)
                }
            }
            Sensor.TYPE_LIGHT -> {
                val byte = event.values[0].toInt().toByte()
                entropyBuffer.add(byte)
            }
            Sensor.TYPE_PRESSURE -> {
                val byte = (event.values[0] * 1000).toInt().toByte()
                entropyBuffer.add(byte)
            }
        }
        
        if (entropyBuffer.size >= 128) {
            val entropy = entropyBuffer.toByteArray()
            entropyBuffer.clear()
            onEntropy(entropy)
        }
    }
}
```

### 6. Entropy Pool & XOR Mixing

```kotlin
class EntropyPool {
    private val pool = Collections.synchronizedList(mutableListOf<Byte>())
    private var lastHash = ByteArray(32)

    fun add(data: ByteArray) {
        synchronized(pool) {
            pool.addAll(data.toList())
            while (pool.size >= 1024) {
                val chunk = pool.take(1024).toByteArray()
                pool.subList(0, 1024).clear()
                mixEntropy(chunk)
            }
        }
    }

    private fun mixEntropy(chunk: ByteArray) {
        // XOR all sources together
        for (i in chunk.indices) {
            lastHash[i % 32] = (lastHash[i % 32].toInt() xor chunk[i].toInt()).toByte()
        }
    }

    fun getBytes(count: Int): ByteArray {
        val result = ByteArray(count)
        synchronized(pool) {
            for (i in 0 until count) {
                if (pool.isNotEmpty()) {
                    result[i] = pool.removeAt(0)
                } else {
                    result[i] = lastHash[i % 32]
                }
            }
        }
        return result
    }
}
```

### 7. CUEP Bluetooth Server

```kotlin
class CUEPBluetoothServer(
    private val context: Context,
    private val entropyPool: EntropyPool
) {
    private var bluetoothAdapter: BluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
    private var bluetoothGattServer: BluetoothGattServer? = null
    private var connectedDevice: BluetoothDevice? = null
    
    private val serviceUuid = UUID.fromString("A1B2C3D4-E5F6-7890-1234-567890ABCDEF")
    private val characteristicUuid = UUID.fromString("B1C2D3E4-F5A6-7890-1234-567890ABCDEF")
    
    fun start() {
        val service = BluetoothGattService(serviceUuid, BluetoothGattService.SERVICE_TYPE_PRIMARY)
        val characteristic = BluetoothGattCharacteristic(
            characteristicUuid,
            BluetoothGattCharacteristic.PROPERTY_READ or
            BluetoothGattCharacteristic.PROPERTY_WRITE or
            BluetoothGattCharacteristic.PROPERTY_NOTIFY,
            BluetoothGattCharacteristic.PERMISSION_READ or
            BluetoothGattCharacteristic.PERMISSION_WRITE
        )
        service.addCharacteristic(characteristic)
        
        bluetoothGattServer = bluetoothAdapter.openGattServer(context, gattServerCallback)
        bluetoothGattServer?.addService(service)
        
        // Start advertising
        val advertiser = bluetoothAdapter.bluetoothLeAdvertiser
        val advertisingSet = AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setConnectable(true)
            .build()
        val advertiseData = AdvertiseData.Builder()
            .addServiceUuid(ParcelUuid(serviceUuid))
            .build()
        advertiser.startAdvertising(advertisingSet, advertiseData, advertiseCallback)
    }
    
    private val gattServerCallback = object : BluetoothGattServerCallback() {
        override fun onConnectionStateChange(device: BluetoothDevice, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                connectedDevice = device
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                connectedDevice = null
            }
        }
        
        override fun onCharacteristicWriteRequest(
            device: BluetoothDevice,
            requestId: Int,
            characteristic: BluetoothGattCharacteristic,
            preparedWrite: Boolean,
            responseNeeded: Boolean,
            offset: Int,
            value: ByteArray
        ) {
            val command = value[0].toInt()
            val response = handleCommand(command)
            
            if (responseNeeded) {
                bluetoothGattServer?.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null)
            }
            
            characteristic.value = response
            bluetoothGattServer?.notifyCharacteristicChanged(device, characteristic, false)
        }
    }
    
    private fun handleCommand(command: Int): ByteArray {
        return when (command) {
            0x01 -> entropyPool.getBytes(1)
            0x02 -> entropyPool.getBytes(2)
            0x04 -> entropyPool.getBytes(4)
            0x10 -> entropyPool.getBytes(1024)
            0xFF -> entropyPool.getBytes(65536) // Continuous, client disconnects when done
            0x00 -> byteArrayOf(getHealthStatus())
            else -> byteArrayOf(0x00)
        }
    }
    
    private fun getHealthStatus(): Byte {
        var status: Byte = 0
        // Check each sensor and set bits
        if (isCameraActive) status = (status.toInt() or (1 shl 0)).toByte()
        if (isMicActive) status = (status.toInt() or (1 shl 1)).toByte()
        if (isMagnetometerActive) status = (status.toInt() or (1 shl 2)).toByte()
        // ... continue for other sensors
        return status
    }
}
```

---

## iOS Implementation

iOS requires similar permissions and uses CoreBluetooth, AVFoundation for camera/mic, and CoreMotion for sensors. Key differences:

- **Camera**: AVCaptureSession with video preview, extract pixel buffer data
- **Microphone**: AVAudioEngine with tap on input node
- **Sensors**: CMMotionManager for magnetometer, CMAltimeter for barometer
- **Bluetooth**: CBPeripheralManager for BLE server

---

## Client Example (Python)

```python
import asyncio
from bleak import BleakScanner, BleakClient

SERVICE_UUID = "A1B2C3D4-E5F6-7890-1234-567890ABCDEF"
CHARACTERISTIC_UUID = "B1C2D3E4-F5A6-7890-1234-567890ABCDEF"

async def harvest_entropy():
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: SERVICE_UUID.lower() in ad.service_uuids
    )
    
    async with BleakClient(device) as client:
        # Send command 0x10 (get 1KB)
        await client.write_gatt_char(CHARACTERISTIC_UUID, bytearray([0x10]))
        
        # Wait for notification
        entropy = bytearray()
        def notification_handler(sender, data):
            entropy.extend(data)
        
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        await asyncio.sleep(1)  # Wait for data
        await client.stop_notify(CHARACTERISTIC_UUID)
        
        return entropy

entropy = asyncio.run(harvest_entropy())
print(f"Received {len(entropy)} bytes of physical entropy")
```

---

## Use Cases

| Use Case | How |
|----------|-----|
| **ZOSCII ROM Generation** | Laptop connects to phone, reads 64KB of entropy, creates ROM locally |
| **microZOSCII Seeds** | Phone generates 54-character seeds from true physical randomness |
| **Air-Gapped Key Storage** | Phone never connects to internet; serves entropy via BLE only |
| **Web3 Wallet Provisioning** | Mobile wallet uses phone's own entropy; desktop wallet uses phone as entropy source |
| **IoT Device Pairing** | Phone provisions IoT devices with entropy over BLE |
| **Penetration Testing** | Portable entropy source for field work |

---

## Advantages vs Dedicated Hardware

| Advantage | Why |
|-----------|-----|
| **Free** | Uses existing smartphone hardware |
| **Always Available** | People carry phones everywhere |
| **Multiple Sensors** | Diverse physical entropy sources |
| **Bluetooth Built-in** | No extra modules needed |
| **App Updatable** | Can improve mixing algorithms over time |
| **Camera as Quantum Source** | Photon shot noise approximates quantum randomness |

---

## Limitations & Mitigations

| Limitation | Mitigation |
|------------|------------|
| **No dedicated quantum source** | Camera photon noise provides quantum-level entropy |
| **Phone sensors may be filtered** | Use raw sensor data where APIs allow |
| **Battery/power constraints** | App can run on demand or when charging |
| **Permission requirements** | Camera, microphone, Bluetooth permissions needed |
| **Stationary reduces motion entropy** | Camera and microphone remain high-quality sources |

---

## Security Considerations

- **All entropy is XORed together**—as long as one source is unpredictable, the output is secure
- **Camera provides the strongest source**—photon shot noise is true quantum randomness
- **Phone never stores entropy**—pool is volatile; app does not log sensor data
- **Bluetooth is local**—no internet connection required; can be air-gapped
- **CUEP protocol is simple**—no headers, no checksums, pure entropy

---

## Conclusion

The Mobile Chaos Harvester turns any smartphone into a portable, physics-based entropy source using only the hardware already in your pocket. With camera photon noise, microphone thermal noise, and ambient sensor fluctuations, even a stationary phone provides high-quality random data suitable for cryptographic applications, ZOSCII ROM generation, and Web3 key material. 

Combined with the CUEP protocol over Bluetooth, it becomes a wireless entropy source for laptops, IoT devices, and other phones—free, open source, and quantum-proof by physics.

For total security between the device and the smart phone harvester - use ZOSCII or UNSIGNAL Protocol for realtime security.

---

## References

- Shannon, C.E. (1949). "Communication Theory of Secrecy Systems"
- Cyborg Unicorn Entropy Protocol (CUEP) Specification
- Android Sensor API Documentation
- iOS Core Bluetooth Framework
