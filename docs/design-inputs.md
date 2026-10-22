# Requirements and course design inputs

Current analysis inputs, not an implementation grant. Product-first authority remains: rubric informs an explicit disposition and upgrade path, not silent compulsory procurement. Course output focuses on requirements/design; a later authorized dry slice can settle ambiguity without claiming water readiness.

| Requirement | Concrete behavior / evidence to design |
|---|---|
| MG-R01 | Web asks to irrigate Zone A, previews every affected pot and bounded duration; truthful receipt, not success inferred from send. |
| MG-R02 | Versioned local schedule runs each occurrence once when time/config/ledger/safety remain trusted during Internet outage; no stale/catch-up/reboot replay. |
| MG-R03 | Local START/STOP at source enters the same arbiter; independent mechanical shutoff remains accessible and tested before field. Balcony button optional. |
| MG-R04 | Queued/received/accepted-or-rejected/started/terminal/unknown states include timestamp/reason. Unknown outcome requires inspection/safe confirmation before another OPEN. |
| MG-R05 | Pot additions/moves/retirements and planting/assignment history do not rename device identity or rewrite telemetry. Shared circuit does not imply independent per-pot control. |
| MG-R06 | Boot/power/watchdog safe off/closed; local maxima, cumulative budget, cooldown and interlock independent of cloud. Numeric field envelope OPEN. |
| MG-R07 | Transport/cadence avoid short DB polling; measure Neon-active across microSched, including resource/cost and history retention. |
| MG-R08 | SVG and Three.js resolve one versioned site source; Owner can identify and correct anchors/routes, see provenance and unknowns. Geometry does not become RF/hydraulic acceptance. |

Design views follow the course's requirements → process → domain → information → service → deployment → functional → operational → device/integration sequence. Site captures are one input; stakeholder scenarios, outages, garden changes, maintenance and truthful state reporting supply requirements that geometry cannot.

The final-course assignment includes a sensor requirement that remains a **disposition gap** while soil sensing is HOLD. Trace each criterion as `FOLLOW`, `ADAPT/EXTENSION`, `NEGOTIATE` or `OMIT WITH RATIONALE`, with Owner agreement/lecturer clarification where needed. Do not buy a soil probe or map DHT11 air humidity to soil moisture merely to fill the rubric. Chapter 1–3 concepts retained: device identity/lifecycle, raw+metadata/time/calibration, upstream/downstream flow, buffering, lossy connectivity, protocol choice and safety. Chapter 4 adds the design process. Slide files and private classroom notes stay in the original course workspace.

Proposed later design output: scenario/requirement-source-verification trace; domain/information and command/schedule state models; deployment/trust/resource views; A+B model-v0 scope and correction flow; transport comparison with common payload/ack/retry workload; dry profile separated from water enable. No fixed schedule, SKU or toolchain is inferred from a diagram.

See [specification](specification.md) for preserved detailed contracts and gates, [site baseline](site-baseline.md) for observations/uncertainty and [operations](operations.md) for verification planes. The original snapshots are retained privately with hashes; this page is a public synthesis, not new authority.
