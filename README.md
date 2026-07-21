<img width="4000" height="3000" alt="20260721_072716" src="https://github.com/user-attachments/assets/5232318a-e837-4125-860d-9aa150d65a64" />

# SeismoTrak — DIY Earthquake Detector

SeismoTrak is an open-source earthquake detector built around an Arduino Nano Every. It uses two VL53L0X time-of-flight laser distance sensors in an X-Y configuration to track the motion of a suspended lead pendulum weight. A custom Z-Score algorithm continuously monitors both axes for statistically significant movement, triggering a bright alarm LED and loud piezo siren when ground motion is detected.

---

## Table of Contents

- [How It Works](#how-it-works)
- [Hardware](#hardware)
- [Wiring](#wiring)
- [Software Design](#software-design)
- [Building and Flashing](#building-and-flashing)
- [Configuration](#configuration)
- [Operating the Device](#operating-the-device)
- [3D Printed Parts](#3d-printed-parts)
- [Project Structure](#project-structure)
- [License](#license)

---

## How It Works

A heavy lead fishing weight (~227 g / 8 oz) is suspended on a fine nylon thread inside a 3D-printed housing. The weight is painted white so that the two VL53L0X laser sensors — mounted at 90° to each other — can reliably measure the distance to it. When the ground shakes, the weight swings and the distances change. The microcontroller samples both distances at ~5 Hz and runs a Z-Score algorithm to distinguish genuine seismic movement from sensor noise.

```
        ┌──────────────────────────────┐
        │      Suspended Weight        │
        │                              │
        │  Y-Sensor ──► ●  ◄── X-Sensor│
        │                              │
        └──────────────────────────────┘
```

---

## Hardware

| Component | Quantity | Notes |
|---|---|---|
| Arduino Nano Every | 1 | ATmega4809, 3.3 V / 5 V I/O |
| VL53L0X Time-of-Flight Distance Sensor | 2 | Adafruit breakout or equivalent |
| Alarm Pilot Lamp (red) | 1 | Custom, driven from digital pin |
| Piezo Buzzer | 1 | Active buzzer, 5 V |
| Alarm Mute Pushbutton | 1 | Normally open, pulled up internally |
| 3 mm LED (activity indicator) | 1 | Any colour |
| Sleep / Program Pushbutton | 1 | Halts sketch to allow re-flashing |
| Barrel Jack (5.5 mm / 2.1 mm) | 1 | Power input |
| 12 V 2.5 A Wall Adaptor | 1 | |
| 8 oz Lead Fishing Weight | 1 | Painted white with acrylic paint |
| M1.7 × 8 mm Screw | 1 | Pendulum suspension point |
| Black Nylon Upholstery Thread | — | Pendulum string |
| Custom 3D Printed Parts | — | Detector housing + controller enclosure |

---

## Wiring

| Arduino Pin | Connection |
|---|---|
| D2 | Activity LED (anode via resistor) |
| D3 | Alarm LED (anode via resistor) |
| D4 | Alarm Mute Button (INPUT_PULLUP) |
| D5 | Alarm Piezo Siren (+) |
| D6 | VL53L0X #1 XSHUT (Y-axis sensor) |
| D7 | VL53L0X #2 XSHUT (X-axis sensor) |
| A0 | Sleep / Program Button (INPUT_PULLUP) |
| SDA / SCL | VL53L0X #1 and #2 (shared I²C bus, 400 kHz) |

> **Note:** Both VL53L0X sensors share the I²C bus. The firmware sequences their XSHUT pins at startup to assign each a unique address (0x30 and 0x31).

---

## Software Design

The firmware is written in C++ for the Arduino framework and built with PlatformIO.

### Main Loop

Each iteration of `loop()` performs three tasks in order:

1. **Sleep check** — If the sleep button (A0) is held, the CPU halts, allowing PlatformIO/Arduino IDE to re-flash the board without a manual reset.
2. **Alarm mute check** — If the mute button (D4) is pressed while an alarm is active, the siren is silenced immediately. The alarm LED remains on until the event fully clears.
3. **Sensor read & event detection** — Both sensors are read, the Z-Score algorithm is updated, and alarm state is managed.

### Signal Processing

#### `WindowedMean`
An O(1) sliding-window mean implemented as a running accumulator. Pre-seeded with a configurable starting value so it is immediately usable without a warmup period.

#### `ZScore`
A robust, noise-tolerant motion detector. Each axis runs an independent `ZScore` instance backed by three `WindowedMean` filters:

| Filter | Window | Purpose |
|---|---|---|
| Event mean | 10 samples | Short-term mean of raw distance |
| Event MAD | 10 samples | Mean absolute deviation from event mean |
| Baseline MAD | 300 samples | Long-term background noise level |

The **event score** is computed as `event_MAD / baseline_MAD`. When the event score exceeds the configurable `EVENT_THRESHOLD`, the event is declared active and baseline updates are frozen — preventing the background model from adapting to the earthquake itself. The event clears automatically when the score drops back below the threshold.

### Alarm State Machine

| State | Condition | LED | Siren |
|---|---|---|---|
| **Idle** | No event on either axis | Off | Off |
| **Active** | Event on X or Y axis | On | On |
| **Suppressed** | Mute button pressed during active event | On | Off |
| **Reset** | 30 s elapsed with no event | Off | Off |

A new event after mute-suppression will not re-trigger the siren until the full 30-second reset cycle completes.

### Activity Heartbeat

The 3 mm activity LED flashes briefly every 10th sensor sample to confirm the system is alive and sampling. During an active seismic event it stays on continuously.

---

## Building and Flashing

This project uses [PlatformIO](https://platformio.org/).

### Prerequisites

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide), **or** the PlatformIO Core CLI.

### Clone and Build

```bash
git clone https://github.com/jhogsett/SeismoTrak.git
cd SeismoTrak
pio run
```

### Flash

Connect the Arduino Nano Every via USB, then:

```bash
pio run --target upload
```

Or use the PlatformIO toolbar in VS Code (the → Upload button).

### Dependencies

Declared in `platformio.ini` and fetched automatically:

- [`adafruit/Adafruit_VL53L0X`](https://github.com/adafruit/Adafruit_VL53L0X) v1.2.5+

---

## Configuration

Key constants in `src/SeismoTrak6.ino`:

| Constant | Default | Description |
|---|---|---|
| `EVENT_WINDOW_SIZE` | 10 | Short-term averaging window (samples) |
| `BASELINE_WINDOW_SIZE` | 300 | Long-term noise baseline window (samples) |
| `PRIMED_VALUE` | 45 | Initial seed value for the event mean filter (mm) |
| `NOISE_FLOOR` | 1.0 | Minimum baseline MAD to prevent divide-by-zero (mm) |
| `EVENT_THRESHOLD` | 1.5 | Z-Score ratio that triggers an event |
| `ALARM_RESET_MS` | 30000 | Milliseconds of quiet before alarm auto-clears |
| `TIME_BUDGET` | 100000 | VL53L0X measurement timing budget (µs) |

---

## Operating the Device

1. **Power on** — The alarm LED and siren fire briefly as a self-test, then go silent.
2. **Normal operation** — The activity LED flashes once every ~2 seconds. Both sensors are sampling continuously.
3. **Earthquake detected** — The red alarm LED illuminates and the siren sounds.
4. **Mute the siren** — Press the mute button. The siren stops; the LED stays on.
5. **Auto-clear** — 30 seconds after motion ceases the alarm resets fully and the system returns to normal monitoring.
6. **Re-flashing** — Hold the sleep button before connecting USB. The sketch halts, allowing PlatformIO to upload new firmware without a manual reset race.

---

## 3D Printed Parts

| Part | Source |
|---|---|
| Detector Housing | https://www.tinkercad.com/things/25R8DhcATeh-earthquake-detector-final |
| Controller Enclosure | https://www.tinkercad.com/things/kzI76yMjvhY-seismotrak-controller-final |

---

## Project Structure

```
SeismoTrak/
├── platformio.ini          # PlatformIO build configuration
├── src/
│   ├── main.cpp            # Main firmware (setup, loop, alarm logic)
│   └── zscore.h            # ZScore motion-detection class
├── include/
│   └── windowed_mean.h     # WindowedMean sliding-window filter class
├── doc/
│   └── Project_Description.md
└── README.md
```

---

## License

This project is released under the [MIT License](LICENSE).
