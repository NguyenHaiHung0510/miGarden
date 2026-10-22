# Design inputs and requirement rationale

miGarden addresses a real garden with changing pots, a shared water source, intermittent connectivity and maintenance at more than one level of the house. These constraints shape the product more directly than a particular protocol or hardware catalog.

| Requirement | Behavior and rationale |
|---|---|
| MG-R01 | Web requests bounded irrigation of Zone A and previews every affected pot. A shared circuit cannot truthfully offer “skip this pot”. |
| MG-R02 | A versioned local schedule runs an occurrence once while time/config/ledger/safety remain trusted during Internet outage. Reconnect must not trigger stale or catch-up watering. |
| MG-R03 | Source START/STOP enters the same arbiter as remote and schedule input. Independent mechanical shutoff remains accessible; a balcony button is optional. |
| MG-R04 | Queued/received/accepted-or-rejected/started/terminal/unknown states carry time and reason. A lost receipt cannot be translated into physical success or safe failure. |
| MG-R05 | Adding, moving or retiring a pot preserves identity and placement/planting history. Circuit-level delivered volume does not become per-pot volume. |
| MG-R06 | Boot/power/watchdog default off/closed. Local maxima, cumulative budget, cooldown and interlock survive cloud failure; the numeric field envelope needs measured evidence. |
| MG-R07 | Transport and cadence avoid frequent database polling. Cost measurement includes the whole microSched workload, row/index growth and retention. |
| MG-R08 | SVG and Three.js use one versioned site source with correctable anchors, provenance and visible unknowns. Geometry guides test placement without certifying radio or hydraulics. |

The design separates domain, information, service, deployment and device views. A telemetry sample needs identity, raw value, units, time quality and calibration; a command needs target binding, expiry, outcome and feedback. Soil sensing and recommendation remain conditional while that lane is HOLD. DHT11 air humidity is not soil moisture.

Useful design outputs include scenario-to-verification traces, command/schedule state models, deployment/trust/resource views, a correctable A+B model-v0 and transport comparisons using the same payload/ack/retry workload. Exact implementation technology follows those constraints and evidence.

See [specification](specification.md) for detailed contracts, [site baseline](site-baseline.md) for observations and [operations](operations.md) for verification stages.
