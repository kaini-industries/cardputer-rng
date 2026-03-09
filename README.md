# Cardputer RNG

Hardware random number generator and key display for the M5Stack Cardputer.

## Features

- **True random key generation** -- 256-byte (2048-bit) keys from hardware entropy
- **5 transistor noise sources** on GPIO pins G3, G4, G5, G6, G13
- **IMU entropy mixing** -- accelerometer and gyroscope data hashed into the entropy pool
- **4 display modes** -- OTP pairs, normalized digits, hex dump, binary
- **Matrix rain animation** during entropy collection
- **SHA256 key fingerprint** shown in the status bar
- **Serial output** of generated keys for external logging

## Hardware Requirements

- [M5Stack Cardputer](https://docs.m5stack.com/en/core/CoreS3) (ESP32-S3, ST7789 240x135 display)
- 5x transistor noise circuits wired to GPIO pins G3, G4, G5, G6, G13

See [`lib/TransistorNoiseSource/transistor_noise_source.png`](lib/TransistorNoiseSource/transistor_noise_source.png) for the circuit diagram.

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VSCode extension)

### Build & Upload

```bash
# Build
pio run

# Upload to device
pio run --target upload

# Open serial monitor
pio device monitor
```

## Usage

1. **Boot** -- the device starts collecting entropy immediately, showing a Matrix rain animation
2. **Shake the device** to accelerate entropy gathering (IMU data is mixed into the pool)
3. Once enough entropy is collected, the key is generated automatically
4. **SPACE** -- cycle through display modes: OTP, Digits, Hex, Binary
5. **ENTER** -- reset and generate a new key

### Display Modes

| Mode | Description |
|------|-------------|
| OTP | Normalized digits grouped in pairs (e.g. `38 71 05 92`) |
| Digits | Continuous normalized digit string (0-9) |
| Hex | Raw 256-byte hex dump |
| Binary | 8-bit binary representation of each byte |

### Serial Output

On key generation, the full key is printed over USB serial in four formats: OTP (digit pairs), normalized digits, binary, and raw hex.

## Project Structure

```
.
├── src/
│   └── main.cpp                    # Application: RNG scene, entropy collection, display
├── lib/
│   ├── cardgfx/                    # Custom UI framework (see below)
│   │   ├── src/
│   │   │   ├── cardgfx.h           # Single-include header
│   │   │   ├── cardgfx_hal.*       # Display hardware abstraction
│   │   │   ├── cardgfx_canvas.*    # RGB565 sprite-backed drawing
│   │   │   ├── cardgfx_input.*     # Keyboard scanning & events
│   │   │   ├── cardgfx_scene.*     # Scene stack manager
│   │   │   ├── cardgfx_widget.*    # Base widget, focus chain
│   │   │   ├── cardgfx_theme.h     # Color/spacing themes
│   │   │   └── widgets/            # Label, TextInput, List, Grid, Modal, StatusBar, Bitmap
│   │   └── README.md               # Framework documentation
│   └── TransistorNoiseSource/      # Hardware entropy source for Arduino Crypto library
├── platformio.ini                  # Build configuration
└── README.md
```

## CardGFX Framework

CardGFX is a lightweight sprite-based UI framework built for the Cardputer's 240x135 display:

- **Double-buffered rendering** with dirty rectangle tracking (zero flicker)
- **Widget system** -- Label, TextInput, List, Grid, Modal, StatusBar, Bitmap
- **Scene stack** with push/pop semantics
- **Input manager** with key repeat, modifier tracking, and event routing
- **Theme support** -- Dark, Light, HighContrast

See [`lib/cardgfx/README.md`](lib/cardgfx/README.md) for full documentation.

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| [M5Unified](https://github.com/m5stack/M5Unified) | ^0.2.10 | Unified hardware abstraction (IMU, power, display) |
| [M5Cardputer](https://github.com/m5stack/M5Cardputer) | ^1.1.1 | Cardputer keyboard and hardware drivers |
| [M5GFX](https://github.com/m5stack/M5GFX) | ^0.2.16 | Graphics library (LovyanGFX-based) |
| [Crypto](https://github.com/rweather/arduinolibs) | ^0.4.0 | SHA256, RNG, NoiseSource |

## Build Configuration

Key build flags in `platformio.ini`:

| Flag | Value | Purpose |
|------|-------|---------|
| `CARDGFX_USE_PSRAM` | 0 | Use SRAM for CardGFX buffers |
| `ARDUINO_LOOP_STACK_SIZE` | 16384 | Larger loop task stack for RNG operations |
| `ARDUINO_USB_CDC_ON_BOOT` | 1 | Enable USB serial on boot (also set by board definition) |

## License

MIT
