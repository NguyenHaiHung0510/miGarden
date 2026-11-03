# Current project state

As of **2026-11-03, Asia/Saigon**, miGarden has a runnable [model-v0 SVG/Three.js prototype](model-v0.md) and a public [design baseline 01–04](design/README.md). Firmware, control simulator and microSched integration have not been implemented here. The viewer edits a representation and saves local feedback; it does not actuate devices. The static [walkthrough](design/walkthrough/index.html) explains synthetic data and opens offline.

## Selected design and implementation boundary

| Area | Current baseline |
|---|---|
| Domain | Zone is the control/schedule/history unit; Area groups Zones, each Zone one Area. No managed Pot/placement/assignment or affected-pot lists. Site pots remain physical representation facts. |
| Controller / expansion | Exactly one existing ESP32 Controller → multiple Actuators within unverified hardware capacity; conceptually one branch valve/Zone. Rollout A first; model allows B/C/D. No Controller/Source/Area inventory CRUD or group domain; orchestration is future work. |
| Schedule / history | 0..1 active local logical schedule/Zone. Fixed Hanoi UTC+7, once/daily/weekly/interval; configuration resolution 1 minute and duration increment 15 seconds. Desired differs from last-known applied; historical immutable content/binding refs differ from current config. |
| Control / safety | Local schedule, source START/STOP and remote converge at one arbiter; cumulative limits across sources, STOP priority/pending invalidation, no-double/no-catch-up/no-replay, unknown block, fail-closed boot, critical-storage safe stop/degraded and independent manual shutoff. Numeric policy is open; SAF-P01 is proposed for Step 08 only. |
| Sensing / scope | Soil/Zigbee remains HOLD. Current error/freshness views retained; no inherited notification lifecycle or plant inventory requirement. |
| Spatial model | A/SVG + B/Three.js consume one canonical versioned `viewer/model.json` with provenance/uncertainty. COLMAP remains optional; not measured output here. |
| Ownership | miGarden owns system/local arbiter, simulator, spatial source and integration contracts; microSched owns human web/API/storage adapter. Compatibility must bind both revisions/contract versions. |

## Available evidence and gaps

| Area | Available evidence | Limits / remaining work |
|---|---|---|
| Design | 01 requirements, 02 five UC, 03 eight concepts, 04 information semantics; curated W00–W06 examples | Baseline for publication, not executable behavior/field acceptance. Steps 05–10 pending; legacy API/schema/transport/numbers are not automatically selected. |
| Site | [Approved readings/names](site-model.md), [historical sources/uncertainty](site-baseline.md) | Absolute XYZ, mounting, pressure/flow, drainage, outdoor electrical and RF/hydraulic behavior remain unverified. PA2 follows the mini and the two-outlet source. |
| Hardware | Bench-kit arrival previously Owner-reported: planned Vietduino ESP32 16MB HS2250V + USB-C HS1369, breadboard/jumpers/red LED/330Ω/DIP button | This is historical procurement context, not exact-unit verification or a selected pin/toolchain/actuator design. No USB/flash/bench/water acceptance here. |
| Representation | [Shared model](representation.md), [viewer/editor](../viewer/README.md); previously recorded six passing Node groups and qualified browser review | Historical prototype receipts are not rerun by this docs promotion. Continuous drag/Shift/full flight-motion proof and surveyed geometry remain qualified/open. |
| Walkthrough | Static HTML, seven SVG/seven PNG, synthetic data and source-bound views; portable data generator/semantic validator | Reuses unchanged prior behavior/figure evidence. Builder checks do not recertify native exports, independent review, firmware, scheduler, hardware or water. |
| Skills / licenses | [Three technical packages](skills.md), provenance/version/license metadata and [notices](../THIRD_PARTY_NOTICES.md) | Package availability does not prove build/device workflows or grant device operations. |
| Deployment | System/adapter ownership boundary is retained | No integrated app/DB/device deployment, production, contained-water or field acceptance. |

Next design work must resolve service/functional/operational/device/application contracts, including clock/elapsed time/persistence/recovery, opportunity identity/cutover, remote handoff and measured safety envelope. [Operations](operations.md) retains technical evidence principles while quarantining old network/Neon/rate thresholds. No field readiness follows from this baseline or synthetic fixture PASS.

The only editable current design home is `docs/design/`. [Specification](specification.md), [architecture](architecture.md) and [design inputs](design-inputs.md) remain at existing paths as LEGACY/reference-only. They cannot fill steps 05–10 or revive omitted scope without an explicit new design choice. Spatial facts and viewer runtime remain separate, unchanged sources.
