# Testy akwarium
| Data | pH | GH | KH | CO2 | NO3 | PO4 | K | Ca | Mg | Fe |


# Akwarium Controller - Professional Schedule

This firmware implements an advanced, dynamic lighting schedule for professional-grade plant growth and aesthetics.

## How It Works

The controller uses a proportional, multi-phase schedule that scales automatically to the **Start** and **Stop** times you set in the menu. The day is divided into a **Morning Block**, a **Siesta** (mid-day dark period), and an **Evening Block**. The proportions of each phase within these blocks are expertly tuned for plant health and visual appeal.

The system also features **sequential ballast switching** to ensure smooth, gradual changes in light, preventing shock to fish and plants.

## Recommended Lamp Configuration

For the schedule to work as designed, the lamps should be arranged as follows:
-   **BALLAST 1 (2 tubes):** 1x Sylvania Grolux + 1x Sylvania Aquastar
-   **BALLAST 2 (2 tubes):** 1x Sylvania Grolux + 1x Sylvania Aquastar
-   **BALLAST 3 (1 tube):** 1x Sylvania Grolux

## Schedule Simulation for 09:00 - 21:00

Here is a simulation of how the lighting day will look with your chosen settings of **Start: 09:00** and **Stop: 21:00**.

-   **Total Day Duration:** 12 hours
-   **Siesta Period:** 13:26 - 17:02 (3 hours 36 minutes)

### Morning Block (09:00 - 13:26)
| Phase | Start Time | End Time | Duration | Active Ballasts | Power Profile |
|---|---|---|---|---|---|
| **Dawn** | 09:00 | 09:53 | ~53 min | Ballast 3 (1 tube) | 0% -> 50% |
| **Sunrise** | 09:53 | 10:46 | ~53 min | Ballasts 1+3 (3 tubes) | 50% -> 100% |
| **Morning** | 10:46 | 12:59 | ~2h 13m | Ballasts 1+3 (3 tubes) | HOLD 100% |
| **SiestaRamp**| 12:59 | 13:26 | ~27 min | Ballasts 1+3 (3 tubes) | 100% -> 0% |

### Evening Block (17:02 - 21:00)
| Phase | Start Time | End Time | Duration | Active Ballasts | Power Profile |
|---|---|---|---|---|---|
| **Awakening** | 17:02 | 17:38 | ~36 min | Ballasts 1+2 (4 tubes)| 0% -> 80% |
| **ZenithRamp**| 17:38 | 17:58 | ~20 min | All 3 (5 tubes) | 80% -> 100% |
| **Zenith** | 17:58 | 20:24 | ~2h 26m | All 3 (5 tubes) | HOLD 100% |
| **Dusk** | 20:24 | 21:00 | ~36 min | Ballast 1 (2 tubes) | 100% -> 0% |

*Note: All transitions between phases are smooth.*

## CO2 Dosing Recommendations

To maximize plant growth and health with this new lighting schedule, CO2 dosing should be adjusted as follows:

-   **CO2 Start:** **07:00** (2 hours before the first light appears). This allows CO2 levels in the water to reach optimal concentration by the time the plants "wake up".
-   **CO2 Stop:** **20:00** (as the Zenith period begins to end). Plants will not consume significant CO2 during the low-light dusk, and this prevents CO2 buildup overnight, which can be harmful to fish.
-   **IMPORTANT:** **Do NOT turn off CO2 during the Siesta.** The siesta is designed to allow CO2 levels to replenish in the water. When the "Awakening" phase begins, your plants will be met with a rich supply of CO2, triggering explosive growth.

---