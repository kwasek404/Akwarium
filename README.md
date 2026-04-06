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

## DS1302 RTC Reliability

The DS1302 RTC module loses time on cold boot when powered from the transformer PSU. The transients on the 5V rail during power-on/off cause a full Power-On Reset (POR) of the DS1302 chip. Multiple hardware and software defense layers have been implemented.

### Current Hardware Modifications

**Power supply** (done):
- RTC Vcc rewired from 5V to Arduino's 3.3V regulator (filters transformer transients)
- 220uF electrolytic capacitor on Vcc/GND at the module

**Signal line filtering** (done):
- 220 ohm series resistors on all three signal lines (CE, SCLK, I/O)
- 10k pull-down resistors on CE, SCLK, and I/O (forces LOW when ATmega pins float)
- 100nF bypass capacitor on CE/GND (on the module)

**Pending** (parts on order):
- 1N5819 Schottky diode in series on Vcc to block reverse current during power-off

### Pending Modification - Schottky Diode

When the 1N5819 arrives, solder it in series on the Vcc line:

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

The Schottky diode blocks reverse current when power drops, preventing the capacitor and RTC from discharging back through the falling 3.3V rail. DS1302 operates from 2.0V minimum - 3.3V minus 0.3V diode drop = 3.0V, well within spec.

### Software Defenses

1. **Boot quorum**: 7 reads with 30ms spacing, requires 3+ to agree within +-2 seconds
2. **Runtime BCD validation**: every read is validated (year/month/day/hour/min/sec ranges), bad reads fall back to lastKnownGoodTime + millis() elapsed
3. **No automatic writes**: RTC is never written to in the main loop, only during explicit user time-set
4. **EMI suppression**: relay switching events trigger 500ms RTC read suppression and LCD reinitialization
5. **Write Protect**: WP bit kept enabled, only cleared during user time-set operations

### Fallback Plan - I2C RTC

If DS1302 reliability remains insufficient after all mitigations, the I2C bus (A4/A5) is already in use for the LCD (address 0x27). A DS3231 module can be added to the same bus without rewiring - only firmware changes needed. The DS3231 has a built-in temperature-compensated oscillator (no external crystal), making it inherently more resistant to EMI.

---