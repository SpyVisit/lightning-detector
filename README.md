# ⚡ Lightning Detector

**AS3935 Franklin Lightning Sensor + TTGO Wemos ESP32 OLED**

A lightning detector based on the AS3935 sensor (WCMCU-3935 module) and ESP32 microcontroller with built-in OLED display. Detects lightning activity, estimates distance to storm front, and displays real-time data on a 128x64 OLED screen.

![Lightning Detector](https://img.shields.io/badge/version-2.4-blue) ![Platform](https://img.shields.io/badge/platform-ESP32-green) ![Sensor](https://img.shields.io/badge/sensor-AS3935-orange) ![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## 📋 Features

- ⚡ Lightning detection up to **40 km** radius
- 📏 Distance estimation in **15 steps** (1–40 km)
- 🔋 Ultra-low sensor power consumption (~80 µA in listening mode)
- 🖥️ Real-time OLED display with adaptive info line
- 📊 Distinguishes between **Lightning**, **Disturber**, and **Noise**
- 🔢 Disturber counter per session
- 📶 Noise level indicator (0–7 scale)
- ⏱️ Auto-reset to Quiet: 10 sec after lightning, 5 sec after disturber
- 🔇 Optional WiFi/BT disable to reduce EMI interference
- 🔌 No external AS3935 library — direct register access via Wire

---

## 🛒 Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | TTGO Wemos ESP32 OLED (ESP-WROOM-32) |
| Lightning Sensor | WCMCU-3935 (AS3935, analog to CJMCU-3935) |
| Display | Built-in OLED 0.96" 128x64, SSD1306, I2C |
| Power | 5V USB or 3.7V LiPo |

---

## 🔌 Wiring

Only **5 wires** from ESP32 to sensor:

| Wire | ESP32 | WCMCU-3935 | Color |
|------|-------|------------|-------|
| Power | 3.3V | VCC + SI + A0 + A1 + EN_V | Red |
| Ground | GND | GND + CS | Black |
| I2C Data | GPIO5 | MOSI | Blue |
| I2C Clock | GPIO4 | SCL | Green |
| Interrupt | GPIO13 | IRQ | Yellow |

> ⚠️ **Important:** Tie SI, A0, A1, EN_V together to 3.3V. Tie CS together with GND.

### Pin Notes for TTGO Wemos ESP32 OLED

| Signal | GPIO | Note |
|--------|------|------|
| OLED SDA | GPIO5 | Reversed from ESP32 standard! |
| OLED SCL | GPIO4 | Reversed from ESP32 standard! |
| OLED RST | GPIO16 | Required — OLED won't start without it |
| AS3935 IRQ | GPIO13 | Interrupt input |

### I2C Addresses

| Device | Address |
|--------|---------|
| OLED SSD1306 | 0x3C |
| AS3935 | 0x03 (A0=1, A1=1) |

---

## 🖥️ OLED Display Layout

```
┌──────────────────────────┐
│   Lightning Detector     │  ← header
├──────────────────────────┤
│ Dist:  27 km             │  ← distance (size 2)
├──────────────────────────┤
│ Energy:   48321          │  ← adaptive info line
│ Status: Lightning!       │  ← status
└──────────────────────────┘
```

### Adaptive Info Line

| Status | Info Line Shows |
|--------|----------------|
| Quiet | `Energy: 0` |
| Lightning! | `Energy: 48321` |
| Noise! | `Noise lvl: 4/7` |
| Disturber | `Disturb: 7x` |

---

## 📦 Libraries Required

Install via Arduino IDE Library Manager:

- **Adafruit SSD1306** — OLED display driver
- **Adafruit GFX Library** — installed automatically with SSD1306

> AS3935 uses **direct Wire register access** — no external library needed.

---

## ⚙️ Arduino IDE Setup

1. Add ESP32 board URL in **File → Preferences**:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```

2. Install **esp32 by Espressif Systems** via Board Manager

3. Select board settings:

   | Parameter | Value |
   |-----------|-------|
   | Board | ESP32 Dev Module |
   | Upload Speed | 921600 |
   | CPU Frequency | 240MHz |
   | Flash Mode | DIO |
   | Flash Size | 4MB |

4. If upload fails — hold **BOOT**, press **RST**, release **BOOT**, then upload

---

## 🚀 Serial Monitor Output

After upload (115200 baud):

```
Lightning Detector 2.4 starting...
AS3935 initialized OK
REG0x00: 0x24   // Indoor mode, active
REG0x01: 0x22   // NF_LEV=2, WDTH=2
REG0x02: 0x82   // SREJ=2, MIN_LIGH=1
REG0x08: 0x00   // DISP_SRCO=0
Ready. Waiting for lightning...
```

---

## 📡 EMI Interference Notes

The AS3935 is sensitive to electromagnetic interference. The ESP32 itself generates noise that the sensor may detect as Disturber or Noise events.

**Solutions (in order of effectiveness):**

1. **Physically separate** the sensor from ESP32 by 50–100 cm (most effective)
2. **Disable WiFi/BT** — uncomment in `setup()`:
   ```cpp
   WiFi.mode(WIFI_OFF);
   btStop();
   ```
3. **Shield ESP32** in a metal enclosure
4. Use **twisted pair** for I2C wires

---

## 📁 File Structure

```
lightning-detector/
├── README.md
├── CHANGELOG.md
├── WIRING.md
└── lightning_detector_v24/
    └── lightning_detector_v24.ino
```

---

## 📝 Version History

| Version | Changes |
|---------|---------|
| 1.0 | OLED interface with random test data |
| 2.0 | Real AS3935 sensor, direct Wire register access |
| 2.1 | Quiet timeout (10 sec) |
| 2.2 | Two separate timers: lightning (10s) and disturber (5s) |
| 2.3 | Fixed init sequence based on PWFusion library analysis |
| 2.4 | Adaptive info line, disturber counter, WiFi/BT disable option |

---

## 📄 License

MIT License — feel free to use, modify and share.

---

## 🙏 References

- [AS3935 Datasheet v1.04 — ams AG](https://ams.com)
- [PWFusion AS3935 Library](https://github.com/PlayingWithFusion/PWFusion_AS3935_I2C)
- [Embedded Adventures AS3935 Library](https://github.com/raivisr/AS3935-Arduino-Library)
- [RandomNerdTutorials — ESP8266 pinout reference](https://randomnerdtutorials.com)
