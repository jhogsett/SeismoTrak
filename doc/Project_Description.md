# SeismoTrak Earthquake Detector

## Overview

- Uses two VL52L0X time-of-flight distance detectors mounted in an X-Y configuration
- Measures the distance to an 8 ounce lead weight suspended 8 inches
- Weight is a standard fishing line weight with an eye screw, painted with acrylic white paint
- Suspended using a black Nylon upholstery thread with a M*1.7 8mm screw at the top
- An Arduino Nano Every samples the distance five times per second
- A sophisticated Z-Score algorithm detects movement, filtering out detector noise
- When ground movement is detected, a bright red pilot lamp warns along with a loud piezo alarm
- A mute pushbutton is provided to quiet the alarm
- 30 seconds after the movement is no longer detected the alarm quiets automatically

## Hardware

- Arduino Nano Every
- VL53L0X Time-of-Flight Distance Detector (2)
- Alarm Pilot Lamp (custom)
- Alarm Piezo Buzzer 
- Alarm Mute Pushbutton
- 3mm LED Activity Indicator
- Sleep Pushbutton (quiets CPU for programming)
- Barrel jack power connector
- 12V 2.5A Wall Adaptor
- 8 ounce lead fishing weight (painted with Acrylic white paint)
- Custom 3D printed detector housing and controller enclosure

## Software

The firmware is written in C++ for the Arduino Nano Every using the PlatformIO build system.

### Architecture

The main loop runs continuously and performs three operations on each iteration:
1. **Sleep check** — polls a dedicated sleep pin (A0). If the pin is pulled low, the CPU halts in a tight loop, allowing the Arduino IDE/PlatformIO to re-flash the board without interference from the running sketch.
2. **Alarm reset check** — polls the mute pushbutton (pin 4). When pressed during an active alarm, it silences the siren immediately while leaving the alarm LED on until the event clears.
3. **Dual sensor read & event detection** — reads both distance sensors, runs Z-Score analysis, and manages alarm state.

### Sensor Initialization

Both VL53L0X sensors share the I²C bus. Because they have identical default addresses, the `setID()` routine uses their XSHUT pins (6 and 7) to bring each sensor out of reset individually and assign them unique I²C addresses (0x30 for the Y-axis sensor, 0x31 for the X-axis sensor). I²C is run at 400 kHz (fast mode). Each sensor's measurement timing budget is set to 100 ms, giving a combined sample rate of approximately 5 Hz.

### Signal Processing

#### `WindowedMean` (include/windowed_mean.h)
A lightweight O(1) exponential-style sliding-window mean. It is implemented as a running accumulator: on each sample the previous mean is subtracted from the accumulator and the new sample is added, then the accumulator is divided by the fixed window length. It is pre-seeded with a `primed_value` so that the filter is immediately usable without a warmup period.

#### `ZScore` (src/zscore.h)
Detects statistically significant deviations from a slowly-varying baseline. Three `WindowedMean` instances are used per axis:
- **Event mean** (window = 10 samples) — tracks the short-term mean of the raw distance measurements.
- **Event MAD** (window = 10 samples) — tracks the mean absolute deviation (MAD) of each sample from the event mean; represents recent signal variability.
- **Baseline MAD** (window = 300 samples) — tracks the long-term background MAD, representing the detector's ambient noise level.

On each sample the algorithm:
1. Computes the deviation of the current sample from the event mean.
2. Updates the event MAD with that deviation.
3. Computes an **event score** = `event_MAD / baseline_MAD` (baseline is clamped to a minimum `NOISE_FLOOR` of 1.0 mm to avoid division by near-zero).
4. If the event score exceeds the `EVENT_THRESHOLD` (1.5), the event is declared **active** and the baseline MAD is frozen — preventing the background model from adapting to the earthquake itself.
5. Once the event score drops back below the threshold, the event is cleared automatically and baseline adaptation resumes.

A one-shot `_event_triggered` flag is also maintained; reading it via `get_event_triggered()` auto-resets it, enabling pulse-style event detection.

### Alarm State Machine

| State | Condition | Outputs |
|---|---|---|
| Idle | Neither axis event active | All outputs off |
| Active | Either axis event active | Alarm LED on, siren on |
| Suppressed | Mute button pressed while active | Alarm LED on, siren off |
| Reset | 30 s (ALARM_RESET_MS) elapsed since last event | All outputs off, suppression cleared |

The alarm can only be suppressed while an event is in progress; it clears fully only after the seismic activity ceases for 30 seconds. If a new event begins after the button-mute, the siren does not reactivate until the cycle resets.

### Activity Indicator

The 3 mm activity LED (pin 2) pulses briefly on every 10th sensor read (controlled by `ACTIVITY_LED_FILTER = 10`) to provide a visible heartbeat confirming the system is sampling. During an active seismic event the same LED is held on continuously.

### Build System

PlatformIO (`platformio.ini`) is used for dependency management, compilation, and flashing. The Adafruit VL53L0X library is used for sensor communication.

## Links

- 3D Model Source (Detector) https://www.tinkercad.com/things/25R8DhcATeh-earthquake-detector-final
- 3D Model Source (Controller) https://www.tinkercad.com/things/kzI76yMjvhY-seismotrak-controller-final
