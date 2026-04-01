# The Absolute Chaos Harvester: Modular Physical Entropy Array
**Standard: ZOSCII I(M;A) = 0 | Hardware Revision: 1.0 (2026)**

## The Philosophy: Measurement, Not Simulation
Most modern Security Modules (HSMs) rely on mathematical algorithms to "stretch" a tiny bit of internal silicon noise until it looks random. This is a gamble based on computational complexity. 

The **Cyborg Unicorn Absolute Chaos Harvester** operates on the principle of **Physical Measurement.** By harvesting raw, non-deterministic noise from disparate physical phenomena—quantum breakdown, thermal agitation, atmospheric radio noise, and satellite timing jitter—and XORing them together, we achieve Information-Theoretic Security. As long as **one** source in the array remains unpredictable, the entire stream remains secure.

---

##  Hardware Stack & Component List

### 1. Central Logic & Power
* **Microcontroller (MCU):** **STM32H7 series** or **NXP MK64FN1M0VDC12**. Samples analog inputs, manages the XOR chain, and handles the CUEP serial interface. (**~$45–$75**)
* **High-Precision ADC:** **Texas Instruments ADS1115** (16-bit, 4-Channel). Crucial for digitizing subtle analog drift without losing entropy to quantization error. (**~$15–$25**)
* **Voltage Regulation:** **TPS61041 Boost Converter** to provide 12-18V for the Avalanche source, followed by a **Low-Noise LDO** to prevent switching ripple. (**~$5–$10**)

### 2. The Entropy Source Array (XOR Chain)
The device supports 8 distinct physical vectors. These can be individually enabled or disabled via an onboard **8-Position DIP Switch**.

| Vector | Physical Source | Component | Glue Logic | Status | Est. Cost (AUD) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Quantum** | Avalanche Breakdown | **BZX79 Zener Diode** | 1x 10kΩ current-limiting resistor | **Required** | $2 |
| **Temporal** | Satellite Timing Jitter | **GPS Module (1PPS)** | None - direct UART/GPIO | Optional | $30–$55 |
| **Kinetic** | Physical Vibration/Tilt | **ADXL335 Accelerometer** | None - direct analog out | Optional | $10–$20 |
| **Acoustic** | Ambient Pressure/Noise | **CUI CMEJ-0627 Mic** | 1x 2.2kΩ bias resistor, 1x 10µF cap | Optional | $3 |
| **Thermal** | Molecular Kinetic Drift | **TMP36 Temp Sensor** | None - direct analog out | Optional | $2 |
| **Optical** | Ambient Light Flicker | **BPW34 Photodiode** | Transimpedance amp: 1x LM324, 1x 1MΩ feedback resistor, 1x 10kΩ output resistor | Optional | $1 (+$2 for op-amp) |
| **Radio** | Atmospheric RF Chaos | **Antenna + LM324 Op-Amp** | 4x 10kΩ resistors (gain stage), 2x 100nF caps (filtering) | Optional | $8 |
| **Voltage** | Analog Rail Fluctuation | **Battery/Rail Sample** | 2x 10kΩ voltage divider (if rail > 3.3V) | **Required** | $0 (Logic) |

---

### 3. Glue Logic Details

#### Quantum Source (BZX79 Avalanche Breakdown)
```
3.3V/5V ──┬── 10kΩ ──┬── BZX79 (reverse biased) ── GND
          │
          └── ADC Input
```
The zener is reverse biased. The resistor limits current. The ADC reads the avalanche noise voltage.

#### Acoustic Source (CUI CMEJ-0627 Electret Mic)
```
3.3V ── 2.2kΩ ──┬── Mic (+) ── Mic (-) ── GND
                │
                └── 10µF ── ADC Input
```
The resistor provides bias. The capacitor blocks DC while passing AC audio signal to the ADC.

#### Optical Source (BPW34 Photodiode + LM324 Transimpedance Amp)
```
                 ┌─────────────────────┐
Photodiode (+) ──┤ IN+  LM324      OUT ├── 10kΩ ── ADC Input
Photodiode (-) ──┤ IN-                 │
GND ─────────────┤ V-                  │
3.3V ────────────┤ V+                  │
                 └──────────┬──────────┘
                            │
                         1MΩ
                            │
                          GND
```
The photodiode generates current proportional to light. The op-amp converts current to voltage. Gain = 1MΩ.

#### Radio Source (Antenna + LM324 Amplifier)
```
Antenna ── 10kΩ ──┬── IN+  LM324 ── OUT ── 100nF ── ADC Input
                  │    │
GND ──────────────┼────┤ V-
                  │    │
3.3V ─────────────┼────┤ V+
                  │    │
10kΩ ── GND ──────┘    │
                       │
10kΩ ── GND ───────────┘
```
The antenna picks up RF noise. The op-amp amplifies the signal. The capacitor blocks DC.

#### Voltage Source (Rail Sampling)
```
Battery/Supply Rail (3.3V–24V) ── 10kΩ ──┬── ADC Input
                                         │
                                       10kΩ
                                         │
                                        GND
```
If the supply rail exceeds 3.3V, the voltage divider scales it down to ADC range.

---

## Unit Costs and Component Modularity
The **Absolute Chaos Harvester** is engineered for cost-scalability. A fully populated **Absolute Tier** prototype costs approximately **$160 – $220 AUD**. 

However, the board uses **"No-Fit" footprints**, allowing expensive components like the GPS (~$50) or Accelerometer (~$15) to be omitted during assembly. For bulk "Founder's Runs" (50+ units), wholesale sourcing can drop the **Core Tier** (Avalanche + Thermal + Voltage) production cost to under **$65 AUD** while maintaining I(M;A)=0 integrity. 

---

## The Cyborg Unicorn Entropy Protocol (CUEP)
Communication uses a lean, 1-byte command structure over UART/USB. Response is raw binary.

| Command (Hex) | Action | Response Format |
| :--- | :--- | :--- |
| `0x01` | **Get 1 Byte** | `[1 Byte]` (Raw XORed Entropy) |
| `0x02` | **Get 1 Word** | `[2 Bytes]` (Little-Endian) |
| `0x04` | **Get 1 Double** | `[4 Bytes]` (32-bit Integer) |
| `0x10` | **Stream 1KB** | `[1024 Bytes]` (Burst mode) |
| `0xFF` | **Continuous Stream** | `[Infinity]` (Until Reset) |
| `0x00` | **Health Check** | `[1 Byte]` (Sensor Status Bitmap) |

### Health Check Bitmap Details
The `0x00` command returns a single byte representing the physical state of the DIP switches and sensor presence:
**`[Bit 7: Radio] [Bit 6: Light] [Bit 5: Vibe] [Bit 4: Temp] [Bit 3: GPS] [Bit 2: Mic] [Bit 1: Quantum] [Bit 0: Voltage]`**

---

## Example Host Implementation (Python)
```python
import serial

def harvest_chaos(port='/dev/ttyUSB0', command=0x10, byte_count=1024):
    """
    Direct interface with the Absolute Chaos Harvester.
    0x10 triggers a 1KB burst of raw XORed physics.
    """
    with serial.Serial(port, 115200, timeout=2) as harvester:
        harvester.write(bytes([command]))
        data = harvester.read(byte_count)
        return data

# Harvest 1KB of Quantum-Proof Entropy
entropy_block = harvest_chaos()
print(f"Status: Received {len(entropy_block)} bytes of Information-Theoretic Chaos.")
```
