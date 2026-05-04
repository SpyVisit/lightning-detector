# Wiring Guide

## WCMCU-3935 to TTGO Wemos ESP32 OLED

### Overview

The AS3935 sensor communicates via I2C and shares the bus with the built-in OLED display.
Both devices use GPIO4 (SCL) and GPIO5 (SDA) — note these are **reversed** from the ESP32 standard pinout.

---

## WCMCU-3935 Pin Description (11 pins)

| Pin | Connection | Description |
|-----|-----------|-------------|
| VCC | 3.3V | Power — join with SI, A0, A1, EN_V |
| GND | GND | Ground — join with CS |
| SCL | GPIO4 | I2C clock |
| MOSI | GPIO5 | I2C data (SDA in I2C mode) |
| MISO | Not connected | SPI only |
| CS | GND | Must be pulled LOW |
| SI (S1) | 3.3V | Interface select: HIGH = I2C |
| IRQ | GPIO13 | Interrupt output |
| EN_V | 3.3V | Enable internal voltage regulator |
| A0 | 3.3V | I2C address bit 0 = 1 |
| A1 | 3.3V | I2C address bit 1 = 1 → address 0x03 |

---

## Simplified: 5 Wires from ESP32

```
ESP32 3.3V  ──────────────┬──── VCC
                          ├──── SI   (HIGH = I2C mode)
                          ├──── A0   (address bit 0)
                          ├──── A1   (address bit 1)
                          └──── EN_V (voltage regulator ON)

ESP32 GND   ──────────────┬──── GND
                          └──── CS   (must be LOW)

ESP32 GPIO5 ─────────────────── MOSI (SDA)

ESP32 GPIO4 ─────────────────── SCL

ESP32 GPIO13 ────────────────── IRQ
```

---

## I2C Bus Configuration

```
ESP32 GPIO5 (SDA) ──────────────── OLED SDA (0x3C)
                  └──────────────── AS3935 MOSI (0x03)

ESP32 GPIO4 (SCL) ──────────────── OLED SCL (0x3C)
                  └──────────────── AS3935 SCL (0x03)
```

Both devices share the same I2C bus — no conflict since addresses differ (0x3C vs 0x03).

---

## OLED Built-in Pins

| Signal | GPIO | Note |
|--------|------|------|
| SDA | GPIO5 | Non-standard for ESP32 |
| SCL | GPIO4 | Non-standard for ESP32 |
| RST | GPIO16 | Required for initialization |
| Address | 0x3C | Fixed |

---

## I2C Address Selection (AS3935)

| A1 | A0 | Address | Status |
|----|----|---------|--------|
| 0 | 0 | 0x00 | **FORBIDDEN by datasheet** |
| 0 | 1 | 0x01 | Valid |
| 1 | 0 | 0x02 | Valid |
| 1 | 1 | 0x03 | **Used in this project** |

---

## Power Analysis

| Component | Current |
|-----------|---------|
| ESP32 (active WiFi) | ~240 mA |
| OLED SSD1306 | ~20 mA |
| AS3935 (listening) | ~0.085 mA |
| **Total** | **~260 mA** |

The AS3935 draws ~1000x less current than the ESP32 — negligible load.

---

## EMI Notes

The ESP32 generates electromagnetic interference that the AS3935 may detect as noise or disturbers.

**Best practice:** Mount the WCMCU-3935 sensor at least **50 cm away** from the ESP32 using extension wires. Use twisted pair for I2C lines.

Alternatively, disable WiFi/Bluetooth in firmware:
```cpp
WiFi.mode(WIFI_OFF);
btStop();
```
