# Changelog

All notable changes to this project will be documented in this file.

---

## [2.4] — 2025-05-04

### Added
- Adaptive middle info line on OLED:
  - `Energy: XXXXX` when Quiet or Lightning
  - `Noise lvl: X/7` when Noise detected
  - `Disturb: Xx` when Disturber detected
- Disturber counter per session (not reset on Quiet)
- Optional WiFi/BT disable to reduce EMI interference (commented out by default)
- `#include <WiFi.h>` added for WiFi control

### Changed
- Disturber counter persists across Quiet resets (session counter)

---

## [2.3] — 2025-05-04

### Fixed
- Initialization sequence corrected based on PWFusion library analysis:
  - Added DISP_SRCO=1 → delay(2) → DISP_SRCO=0 step for proper SRCO calibration
  - Fixed CL_STAT clear sequence: high → low → **high** (was missing final high)
  - Changed IRQ read delay from 2ms to 10ms (more reliable)
  - Changed all Wire.endTransmission() to Wire.endTransmission(false)
- Spurious IRQ (intSrc==0) now logged as "Spurious IRQ ignored"

---

## [2.2] — 2025-05-03

### Added
- Two independent timers:
  - `lightningTimer` — 10 sec, resets only on Lightning events
  - `disturberTimer` — 5 sec, resets on Disturber and Noise events
- Timer logic: Lightning status holds 10 sec even if disturbers continue

### Fixed
- Status no longer stuck in Disturber/Noise indefinitely
- Timer only resets on the relevant event type

---

## [2.1] — 2025-05-03

### Added
- Auto-reset to Quiet after 10 seconds of no events
- Fixed zero distance display showing blank instead of `---`

### Fixed
- `lastEventTime` timer now properly resets on each event

---

## [2.0] — 2025-05-03

### Added
- Real AS3935 sensor integration
- Direct Wire register access (no external library)
- Complete AS3935 initialization sequence
- IRQ interrupt handler with IRAM_ATTR
- Lightning, Disturber, Noise event handling
- Distance and energy readout from registers
- Serial Monitor output for all events

### Changed
- Replaced random test data with real sensor data

---

## [1.0] — 2025-05-02

### Added
- Initial OLED interface with random test data
- OLED initialization with manual RST on GPIO16
- Discovered actual OLED pins: SDA=GPIO5, SCL=GPIO4 (reversed from ESP32 standard)
- I2C Scanner utility
- Screen layout: header, distance (size 2), energy, status
- Serial Monitor output
