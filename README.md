# Testy akwarium
| Data | pH | GH | KH | CO2 | NO3 | PO4 | K | Ca | Mg | Fe |
|---|---|---|---|---|---|---|---|---|---|---|


# Akwarium Controller - "Prestige Plus" Firmware

This firmware implements an advanced, dynamic lighting schedule for professional-grade plant growth and aesthetics.

## How It Works

The controller uses a proportional, multi-phase schedule that scales automatically to the **Start** and **Stop** times you set in the menu. The day is divided into a **Morning Block**, a **Siesta** (mid-day dark period), and an **Evening Block**. The proportions of each phase within these blocks are expertly tuned for plant health and visual appeal.

The system features:
- **Lumen-Compensated Ballast Switching:** To prevent jarring flashes of light, the controller pre-dims the light, switches a ballast, and then smoothly compensates the power to ensure a seamless transition.
- **Quadratic Ramps:** "Ease-in" and "ease-out" light changes for more natural and organic dawn/dusk effects.
- **Fault Detection:** Automatically detects if the 1-10V control circuit fails and safely shuts down the ballasts.
- **System Power Logic:** The schedule defines the desired **Total System Power** (as a % of all 5 tubes). The controller intelligently calculates the required per-ballast power to achieve this target, with a safety clamp at 100%.

## Recommended Lamp Configuration

For the schedule to work as designed, the lamps should be arranged as follows:
-   **BALLAST 1 (2 tubes):** 1x Sylvania Grolux + 1x Sylvania Aquastar (This pair is used for the long, aesthetic dusk)
-   **BALLAST 2 (2 tubes):** 1x Sylvania Grolux + 1x Sylvania Aquastar
-   **BALLAST 3 (1 tube):** 1x Sylvania Grolux (This is the first light on in the morning)

## Schedule Simulation for 09:00 - 21:00

Here is a simulation of how the lighting day will look with settings of **Start: 09:00** and **Stop: 21:00**.

-   **Total Day Duration:** 12 hours
-   **Siesta Period:** 13:26 - 17:02 (3 hours 36 minutes)

### Morning Block (09:00 - 13:26)
| Phase | Start Time | End Time | Duration | Active Tubes | Power (System%) | Power (Per-Tube%) |
|---|---|---|---|---|---|---|
| **Dawn** | 09:00 | 10:06 | ~1h 6m | 1 (G) | 0% -> 10% (ease-in) | 0% -> 50% |
| **Sunrise** | 10:06 | 11:13 | ~1h 7m | 3 (G+A, G) | 10% -> 60% (linear) | 16.7% -> 100% |
| **Morning** | 11:13 | 12:59 | ~1h 46m | 3 (G+A, G) | HOLD 60% | 100% |
| **SiestaRamp**| 12:59 | 13:26 | ~27 min | 3 (G+A, G) | 60% -> 0% (ease-out)| 100% -> 0% |

### Evening Block (17:02 - 21:00)
| Phase | Start Time | End Time | Duration | Active Tubes | Power (System%) | Power (Per-Tube%) |
|---|---|---|---|---|---|---|
| **Awakening** | 17:02 | 17:38 | ~36 min | 4 (2x G+A) | 0% -> 64% (ease-in) | 0% -> 80% |
| **ZenithRamp**| 17:38 | 18:01 | ~23 min | 5 (All) | 64% -> 100% (linear) | 80% -> 100% |
| **Zenith** | 18:01 | 20:25 | ~2h 24m | 5 (All) | HOLD 100% | 100% |
| **ZenithDrop**| 20:25 | 20:38 | ~13 min | 5 (All) | 100% -> 90% (linear) | 100% -> 90% |
| **Dusk** | 20:38 | 21:00 | ~22 min | 2 (G+A) | 40% -> 0% (ease-out)| 100% -> 0% |

*Note: The `Dusk` phase starts from a system power of 40% (2 tubes at 100% is 40% of the 5-tube max), not 90% from the previous phase. The transition logic handles this smoothly.*

## CO2 Dosing Recommendations

To maximize plant growth and health with this new lighting schedule, CO2 dosing should be adjusted as follows:

-   **CO2 Start:** **07:00** (2 hours before the first light appears). This allows CO2 levels in the water to reach optimal concentration by the time the plants "wake up".
-   **CO2 Stop:** **20:00** (as the Zenith period begins to end). Plants will not consume significant CO2 during the low-light dusk, and this prevents CO2 buildup overnight, which can be harmful to fish.
-   **IMPORTANT:** **Do NOT turn off CO2 during the Siesta.** The siesta is designed to allow CO2 levels to replenish in the water. When the "Awakening" phase begins, your plants will be met with a rich supply of CO2, triggering explosive growth.

---

## DS1302 RTC Power Fix

The DS1302 RTC module loses time on cold boot when powered from the transformer PSU. The transients on the 5V rail during power-on/off corrupt the chip's internal registers. The fix is to power the RTC from the Arduino's 3.3V regulator with a Schottky diode and bulk capacitor.

### Required Parts

- **1N5819** Schottky diode (or equivalent: SS14, SB140, BAT42 - any Schottky rated >= 20V, >= 0.5A)
- **220uF** electrolytic capacitor (any voltage rating >= 6.3V, physically 10-16V are common)

### Wiring Diagram

Disconnect the RTC module Vcc wire from the 5V rail and reconnect to Arduino's 3.3V pin:

```
                        1N5819
                   (stripe toward RTC)

Arduino 3.3V ------|>|------+------ Vcc DS1302 module
                            |
                          [220uF]
                          + |  -
                            |
Arduino GND ----------------+------ GND DS1302 module
```

- Cut the existing Vcc wire going from 5V to the RTC module
- Solder the 1N5819 diode in series: anode to 3.3V, cathode (stripe) toward RTC Vcc
- Solder the 220uF capacitor between the diode output and GND (long leg + to diode output)
- GND remains shared

### How It Works

1. **3.3V regulator** filters transformer PSU transients (additional regulation stage)
2. **Schottky diode** blocks reverse current when power drops - prevents the capacitor and RTC from discharging back through the falling 3.3V rail
3. **220uF capacitor** holds voltage steady during the switchover to battery backup
4. DS1302 operates from 2.0V minimum - 3.3V minus 0.3V diode drop = 3.0V, well within spec

---