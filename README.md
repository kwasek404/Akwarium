# Testy akwarium
| Data | pH | GH | KH | CO2 | NO3 | PO4 | K | Ca | Mg | Fe |
|---|---|---|---|---|---|---|---|---|---|---|


# Akwarium Controller - Professional Schedule

This firmware implements an advanced, dynamic lighting schedule for professional-grade plant growth and aesthetics.

## How It Works

The controller uses a proportional, multi-phase schedule that scales automatically to the **Start** and **Stop** times you set in the menu. The day is divided into a **Morning Block**, a **Siesta** (mid-day dark period), and an **Evening Block**. The proportions of each phase within these blocks are expertly tuned for plant health and visual appeal.

The system features:
- **Lumen-Compensated Ballast Switching:** To prevent jarring flashes of light, the controller pre-dims the light, switches a ballast, and then smoothly compensates the power to ensure a seamless transition.
- **Quadratic Ramps:** "Ease-in" and "ease-out" light changes for more natural and organic dawn/dusk effects.
- **Fault Detection:** Automatically detects if the 1-10V control circuit fails and safely shuts down the ballasts.

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
| Phase | Start Time | End Time | Duration | Active Tubes | Power Profile |
|---|---|---|---|---|---|
| **Dawn** | 09:00 | 10:06 | ~1h 6m | 1 (G) | 0% -> 50% (ease-in) |
| **Sunrise** | 10:06 | 11:13 | ~1h 7m | 3 (G+A, G) | 50% -> 100% (linear) |
| **Morning** | 11:13 | 12:59 | ~1h 46m | 3 (G+A, G) | HOLD 100% |
| **SiestaRamp**| 12:59 | 13:26 | ~27 min | 3 (G+A, G) | 100% -> 0% (ease-out)|

### Evening Block (17:02 - 21:00)
| Phase | Start Time | End Time | Duration | Active Tubes | Power Profile |
|---|---|---|---|---|---|
| **Awakening** | 17:02 | 17:38 | ~36 min | 4 (2x G+A) | 0% -> 80% (ease-in) |
| **ZenithRamp**| 17:38 | 18:01 | ~23 min | 5 (All) | 80% -> 100% (linear) |
| **Zenith** | 18:01 | 20:25 | ~2h 24m | 5 (All) | HOLD 100% |
| **Dusk** | 20:25 | 21:00 | ~35 min | 2 (G+A) | 100% -> 0% (ease-out)|

*Note: All transitions between phases are seamless and lumen-compensated.*

## CO2 Dosing Recommendations

To maximize plant growth and health with this new lighting schedule, CO2 dosing should be adjusted as follows:

-   **CO2 Start:** **07:00** (2 hours before the first light appears). This allows CO2 levels in the water to reach optimal concentration by the time the plants "wake up".
-   **CO2 Stop:** **20:00** (as the Zenith period begins to end). Plants will not consume significant CO2 during the low-light dusk, and this prevents CO2 buildup overnight, which can be harmful to fish.
-   **IMPORTANT:** **Do NOT turn off CO2 during the Siesta.** The siesta is designed to allow CO2 levels to replenish in the water. When the "Awakening" phase begins, your plants will be met with a rich supply of CO2, triggering explosive growth.

---
