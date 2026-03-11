# Cardputer RNG

A hardware true random number generator and cryptographic key display for the M5Stack Cardputer (ESP32-S3). Generates 256-byte (2048-bit) keys from multiple physical entropy sources and displays them in four formats on the built-in 240x135 TFT screen.

## Features

- **True random key generation** from hardware entropy (no pseudo-random fallback)
- **5 transistor noise sources** on GPIO pins G3, G4, G5, G6, G13
- **IMU entropy mixing** -- accelerometer and gyroscope data continuously hashed into the entropy pool
- **4 display modes** -- OTP pairs, normalized digits, hex dump, binary
- **SHA256 key fingerprint** displayed in the status bar for visual verification
- **Menu-driven UI** built on the custom CardGFX framework
- **Serial output** of generated keys in all formats

## Hardware Requirements

- [M5Stack Cardputer](https://shop.m5stack.com/products/m5stack-cardputer-kit-w-m5stamps3) (ESP32-S3, ST7789 240x135 display, built-in keyboard)
- 5x transistor noise circuits wired to GPIO pins G3, G4, G5, G6, G13

See [`lib/TransistorNoiseSource/transistor_noise_source.png`](lib/TransistorNoiseSource/transistor_noise_source.png) for the circuit diagram (Rob Seward's transistor-based noise source design).

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VSCode extension)

### Build & Upload

```bash
# Build
pio run

# Upload to device
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

## Usage

### Navigation

1. **Boot** -- the device starts at the main menu
2. Select **Generate Key** and press **ENTER**
3. The entropy collection screen appears with a Matrix rain animation
4. **Shake the device** to accelerate entropy gathering (IMU data is mixed into the pool)
5. Once enough entropy is collected, the key is generated automatically
6. Use **SPACE** to cycle through display modes
7. Press **ENTER** to reset and generate a new key
8. Press **ESC** to return to the menu

### Key Bindings

**Main Menu:**

| Key | Action |
|-----|--------|
| UP / DOWN | Navigate menu items |
| ENTER | Select item |

**Key Display (after generation):**

| Key | Action |
|-----|--------|
| SPACE | Cycle display mode: OTP -> Digits -> Hex -> Binary |
| ENTER | Reset entropy and generate a new key |
| ESC | Return to menu |

### Display Modes

| Mode | Format | Description |
|------|--------|-------------|
| **OTP** | `38 07 42 15` | Space-separated digit pairs in range 00-49 |
| **Digits** | `3807421539` | Continuous single-digit string (0-9) |
| **Hex** | `A2 B4 C7 FF` | Raw 256-byte hex dump |
| **Binary** | `10100010...` | Full 2048-bit binary representation |

**Normalization (OTP and Digits modes):** Raw bytes with values >= 250 are discarded to maintain a uniform distribution. Remaining bytes (0-249) are mapped into the target range -- 0-49 for OTP pairs, 0-9 for single digits.

### Status Bar

When a key is generated, the status bar shows:

- **Left:** SHA256 fingerprint of the key (first 4 bytes, e.g. `A2B4C7FF`)
- **Center:** `KEY GENERATED`
- **Right:** Current display mode (`OTP`, `DIGITS`, `HEX`, `BINARY`)

### Serial Output

On each key generation, the full key is output over USB serial (115200 baud) in all four formats:

```
--- KEY (OTP) ---
38 07 42 15 ...
--- KEY (normalized digits) ---
380742153948...
--- KEY (binary) ---
1010001010110100...
--- KEY (raw hex) ---
A2B4C7FF...  (32 bytes per line)
```

## How It Works

### Entropy Sources

The device collects entropy from two independent physical sources:

1. **Transistor noise** -- Five analog noise circuits (Rob Seward design) are sampled continuously. Each source uses a bucket-based threshold calibration to produce unbiased random bits, targeting a 45-55% ones ratio.

2. **IMU data** -- Every frame, the accelerometer and gyroscope readings (6 axes total) are individually hashed with SHA256 and stirred into the entropy pool via `RNG.stir()`. Shaking the device increases the rate of fresh IMU data.

The Arduino Crypto library's `RNG` class manages the entropy pool. When `RNG.available(256)` reports sufficient entropy, 256 random bytes are extracted and processed into the four display formats.

### Key Derivation

From the 256 raw random bytes:

- **Hex** and **Binary** modes display all bytes directly
- **OTP** and **Digits** modes apply rejection sampling: bytes >= 250 are discarded, and the remaining bytes are uniformly mapped into the target range using `floor((byte / 250.0) * range)`
- A **SHA256 fingerprint** of the full key is computed and the first 4 bytes are shown in the status bar

## Project Structure

```
.
├── src/
│   ├── main.cpp               # Entry point: setup, loop, RNG state machine
│   ├── rng_scene.h            # Entropy collection + key display scene
│   └── menu_scene.h           # Main menu scene
├── lib/
│   ├── cardgfx/               # Custom UI framework
│   │   ├── src/
│   │   │   ├── cardgfx.h           # Single-include header
│   │   │   ├── cardgfx_hal.*       # Display hardware abstraction (M5GFX)
│   │   │   ├── cardgfx_canvas.*    # RGB565 sprite-backed drawing
│   │   │   ├── cardgfx_input.*     # Keyboard scanning & event routing
│   │   │   ├── cardgfx_scene.*     # Scene stack manager with transitions
│   │   │   ├── cardgfx_widget.*    # Base widget class, focus chain
│   │   │   ├── cardgfx_theme.h     # Color themes (Dark, Light, HighContrast)
│   │   │   ├── cardgfx_layout.*    # Box model positioning
│   │   │   └── widgets/            # Label, StatusBar, List, TextInput,
│   │   │                           # Grid, Modal, Bitmap
│   │   └── README.md               # Framework documentation
│   └── TransistorNoiseSource/      # Noise source library for Arduino Crypto
│       ├── TransistorNoiseSource.*  # Bucket-based threshold calibration
│       └── transistor_noise_source.png  # Circuit diagram
├── platformio.ini              # Build configuration
└── README.md
```

## CardGFX Framework

CardGFX is a lightweight sprite-based UI framework purpose-built for the Cardputer's 240x135 display:

- **Double-buffered rendering** with dirty rectangle tracking (zero flicker)
- **Scene stack** -- push/pop navigation with slide transitions
- **Widget system** -- Label, StatusBar, List, TextInput, Grid, Modal, Bitmap
- **Input manager** -- keyboard scanning, key repeat (400ms delay / 80ms rate), modifier tracking, event routing
- **Theme support** -- Dark (default), Light, and HighContrast color schemes
- **Memory-predictable** -- canvas pool with pre-allocated buffers, no dynamic allocation after init

See [`lib/cardgfx/README.md`](lib/cardgfx/README.md) for full documentation.

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| [M5Unified](https://github.com/m5stack/M5Unified) | ^0.2.10 | Unified hardware abstraction (IMU, power, display) |
| [M5Cardputer](https://github.com/m5stack/M5Cardputer) | ^1.1.1 | Cardputer keyboard and hardware drivers |
| [M5GFX](https://github.com/m5stack/M5GFX) | ^0.2.16 | Graphics library (LovyanGFX-based) |
| [Crypto](https://github.com/rweather/arduinolibs) | ^0.4.0 | SHA256, RNG, TransistorNoiseSource |

## Build Configuration

Key build flags in `platformio.ini`:

| Flag | Value | Purpose |
|------|-------|---------|
| `CARDGFX_USE_PSRAM` | 0 | Use SRAM for CardGFX buffers (no PSRAM on Cardputer) |
| `ARDUINO_LOOP_STACK_SIZE` | 16384 | Larger loop task stack for RNG and crypto operations |
| `ARDUINO_USB_CDC_ON_BOOT` | 1 | Enable USB serial output on boot |

## License

MIT
