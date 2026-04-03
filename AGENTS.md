# AGENTS.md - Aquarium Lighting Controller

## Project Overview

PlatformIO-based Arduino firmware for an aquarium lighting controller targeting **Arduino Nano (ATmega328)**. Controls 3 fluorescent ballasts (5 tubes total) via 1-10V analog dimming with a closed-loop voltage feedback system. Features a proportional multi-phase lighting schedule with siesta, quadratic ramps, and lumen-compensated ballast switching.

## Commands

```bash
# Build (from the PlatformIO project directory)
cd app/sterownik_oswietlenia
pio run

# Upload to device (requires /dev/ttyUSB0)
pio run --target upload

# Serial monitor (9600 baud)
pio device monitor
```

There are no tests, linters, or CI pipelines in this project.

## Code Organization

```
app/sterownik_oswietlenia/
  platformio.ini          # Build config: nanoatmega328, Arduino framework
  src/
    main.ino              # Entry point: setup() and loop()
    Constants.h           # All pin definitions, EEPROM addresses, timing constants
    Debug.h               # Compile-time debug macros (disabled by default)
    Settings.h            # EEPROM-persisted user settings (start/stop times, timezone)
    Timezones.h           # Warsaw timezone DST rules (using Timezone library)
    Schedule.h            # Lighting schedule definition (phase arrays, ballast masks)
    TimeController.h      # DS1302 RTC wrapper with quorum boot, BCD validation, EMI suppression
    DisplayController.h   # 16x2 I2C LCD wrapper with auto-backlight timeout
    InputManager.h        # 4-button input with debounce, short press, and hold-repeat
    LightingController.h  # Header for the lighting state machine
    LightingController.cpp# Core logic: scheduler, ballast transitions, voltage feedback loop
    UIManager.h           # Menu system (4 screens), inline edit with blinking cursor
  lib/
    virtuabotixRTC/       # Vendored DS1302 RTC library (not available via PlatformIO registry)
```

## Architecture and Control Flow

### Main Loop

`loop()` runs continuously: reads UTC time -> converts to local -> updates `LightingController` -> checks relay-switch EMI flag (suppresses RTC reads + reinits LCD if set) -> updates `DisplayController` (backlight timeout) -> updates `UIManager` (input + rendering).

### LightingController - Two Nested State Machines

1. **MainState** (`OFF` / `MORNING_BLOCK` / `SIESTA` / `EVENING_BLOCK` / `FAULT`): Determined each loop by comparing current time against schedule boundaries. The day is split into morning and evening blocks around a siesta (37%-67% of total day duration).

2. **TransitionState** (ballast switching sub-FSM): When the schedule demands a different ballast combination, the controller executes a multi-step transition: dim to 5% -> switch one ballast -> compensate power for new tube count -> ramp back up. Multiple ballast changes are sequenced with 1-second delays via `FINISH_TRANSITION`.

### Power Model - Critical to Understand

The schedule defines **Total System Power** as a percentage of all 5 tubes at full output. The controller translates this to **per-ballast power** for the feedback loop:

```
perTubePower = (scheduleTotalSystemPower / activeTubes) * (5.0 / 100.0) * 100.0
```

The voltage feedback loop uses a proportional controller (`VOLTAGE_KP = 1.0`) comparing the 1-10V feedback reading against the target, adjusting PWM output each loop iteration. The PWM output is **inverted** (`ANALOG_WRITE_RESOLUTION - outputVoltage`).

### Ballast Mask System

Ballasts are addressed via bitmask: `BALLAST_1 = 0x01` (2 tubes), `BALLAST_2 = 0x02` (2 tubes), `BALLAST_3 = 0x04` (1 tube). `countTubesInMask()` maps these to tube counts. Relay pins are **active LOW** (LOW = ON, HIGH = OFF).

## Hardware Interface Gotchas

- **Relays are active LOW**: `digitalWrite(pin, LOW)` turns a ballast ON. The constructor sets all relay pins HIGH (off) as the safe default.
- **PWM output is inverted**: Higher `outputVoltage` value means the analog write gets a *lower* value (`255 - outputVoltage`), which maps to higher voltage on the 1-10V circuit.
- **Transformer pin** (`SWITCH_TRANSFORMER_PIN`, pin 2) powers the 1-10V control circuit. It is activated in `begin()` after a 1-second hardware stabilization delay in `setup()`. It must be on before any ballast switching makes sense.
- **Voltage feedback** uses a voltage divider on `A2`, reading 0-10V mapped to 0-5V ADC range. The `getFeedbackVoltagePercent()` formula: `(ADC / 1023 * 5.0 * 2.0 - 1.0) * (100.0 / 9.0)` maps the 1-10V range to 0-100%.
- **EEPROM addresses 0-4** are used for settings. Adding new persistent settings must use addresses >= 5.
- **DS1302 CE pull-down**: A 10k resistor + 100nF cap soldered on CE/GND of the RTC module. Prevents spurious writes from floating GPIO during power transitions.
- **DS1302 power supply**: Fed from Arduino's 3.3V regulator (not 5V) with a 220uF electrolytic on Vcc/GND at the module. Planned addition: 1N5819 Schottky diode in series on Vcc to block reverse current during power-off, ensuring clean battery switchover. The 3.3V regulator filters transformer PSU transients that previously caused DS1302 resets on cold boot.
- **DS1302 Write Protect**: WP bit (register 0x8E) is kept enabled (WP=1) as default state. Only cleared momentarily during explicit user time-set operations. WP is battery-backed, protecting clock registers from spurious writes during power transients.
- **12V relay EMI**: The transformer relay (pin 2) generates inductive kickback on switching that can corrupt DS1302 reads and LCD HD44780 controller. Software mitigation: RTC reads are suppressed for 500ms after relay events, LCD is reinitialized after 100ms settle time.

## Schedule Modification

Schedules are defined as `SchedulePhase` arrays in `Schedule.h`. Each phase specifies: name, start/end percent within its block, ballast mask, ramp type (`RAMP_LINEAR`, `RAMP_QUAD_IN`, `RAMP_QUAD_OUT`, `HOLD`), and start/end power as **system-level percentages** (not per-tube). Phase percentages must be contiguous within a block (0.0 to 1.0). The siesta boundary is controlled by `SIESTA_START_PERCENT_OF_DAY` (0.37) and `SIESTA_END_PERCENT_OF_DAY` (0.67).

## UI Structure

4 screens navigated by BTN_RIGHT: Info (power + phase + ballast status) -> Date/Time (editable) -> Timer Start/Stop (editable) -> Timezone (toggle). BTN_SET enters edit mode on screens 1-2; BTN_RIGHT cycles edit positions; BTN_PLUS/MINUS adjust values with hold-repeat. First button press on a dark screen only wakes the backlight.

## Dependencies

Managed via `platformio.ini`:
- `marcoschwartz/LiquidCrystal_I2C` - I2C LCD driver
- `PaulStoffregen/Time` - `TimeLib.h` time manipulation
- `jchristensen/Timezone` - DST-aware timezone conversion
- `lib/virtuabotixRTC/` - vendored DS1302 RTC driver (not in PlatformIO registry)

## Conventions

- Header-only classes except `LightingController` (split into .h/.cpp due to size)
- `camelCase` for variables and methods, `PascalCase` for classes and enums, `UPPER_SNAKE_CASE` for constants
- No dynamic memory allocation (embedded target with 2KB RAM)
- Debug output via `DEBUG_PRINTLN`/`DEBUG_PRINTF` macros, disabled by default (uncomment `#define DEBUG_ENABLED` in `Debug.h`)
- LCD buffer formatting uses `snprintf` with 17-char buffers (16 columns + null terminator)
- Time is stored and processed as UTC internally; conversion to local happens at display/scheduling boundaries via `TimeController::toLocal()`

## RTC Reliability

The DS1302 RTC is vulnerable to power transients from the transformer PSU. Multiple defense layers are implemented:

1. **Hardware**: 3.3V supply (filtered by regulator), 220uF cap, CE pull-down with 10k + 100nF. Planned: Schottky diode on Vcc.
2. **Boot quorum**: `begin()` performs 7 reads with 30ms spacing, requires 3+ to agree within +-2 seconds. Rejects garbled boot reads.
3. **Runtime BCD validation**: `nowUTC()` validates every read (year 2000-2099, month 1-12, day 1-31, hour 0-23, min/sec 0-59). Bad reads fall back to lastKnownGoodTime + millis() elapsed.
4. **No automatic writes**: RTC is never written to in the main loop. Only explicit user time-set operations write to DS1302. This prevents garbled reads from being written back as "corrections".
5. **EMI suppression**: Relay switching events trigger 500ms RTC read suppression and LCD reinitialization.

## Fault Detection

If target power > 5% but feedback reads < 1% for 5 consecutive seconds, the system enters `FAULT` state and shuts down all ballasts. The fault is latching - it persists and requires a device reset to clear.
