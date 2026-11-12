# Current project state

As of **2026-11-12, Asia/Saigon**, miGarden has a runnable [SVG/Three.js site viewer](model-v0.md) and [design steps 01–09](design/README.md) in the working checkout. Steps 01–08 have design approval within their documented scope; [09](design/09-device-and-component-integration.vi.md) is ready for review. Watering firmware, a control simulator and microSched integration are still pending. The viewer edits spatial data and saves local feedback. The static [walkthrough](design/walkthrough/index.html) uses synthetic data and opens offline.

## Selected design and implementation boundary

| Area | Current baseline |
|---|---|
| Domain | Zone is the control/schedule/history unit; Area groups Zones, each Zone one Area. No managed Pot/placement/assignment or affected-pot lists. Site pots remain physical representation facts. |
| Controller / expansion | Exactly one existing ESP32 Controller → multiple Actuators within unverified hardware capacity; conceptually one branch valve/Zone. Rollout A first; model allows B/C/D. No Controller/Source/Area inventory CRUD or group domain; orchestration is future work. |
| Schedule / history | 0..1 active local logical schedule/Zone. Fixed Hanoi UTC+7, once/daily/weekly/interval; configuration resolution 1 minute and duration increment 15 seconds. Desired differs from last-known applied; historical immutable content/binding refs differ from current config. |
| Control / safety | Local schedule, source START/STOP and remote converge at one arbiter; cumulative limits across sources, STOP priority/pending invalidation, no-double/no-catch-up/no-replay, unknown block, fail-closed boot, critical-storage safe stop/degraded and independent manual shutoff. [Numerical limits](design/08-operational-view-specification.vi.md#budget-d186) are open. |
| First-stage installation | One ESP32, two separate 12 V/2 A adapters, MKE-M06, one VAKS normally closed 12 V valve and two normally open buttons. DNV5 supplies a manual wash outlet and Zone A; two ports capped. One flexible hose leads to the valve, then rigid uPVC reaches a capped stub below the old railing. Y, emitters, filter and dedicated indicator are deferred. [Graph and parts](design/09-device-and-component-integration.vi.md). |
| Sensing / scope | Soil/Zigbee remains HOLD. Current error/freshness views retained; no inherited notification lifecycle or plant inventory requirement. |
| Spatial model | A/SVG + B/Three.js consume one canonical versioned `viewer/model.json` with provenance/uncertainty. COLMAP remains optional; not measured output here. |
| Ownership | miGarden owns system/local arbiter, simulator, spatial source and integration contracts; microSched owns human web/API/storage adapter. Compatibility must bind both revisions/contract versions. |

## Available evidence and gaps

| Area | Available evidence | Limits / remaining work |
|---|---|---|
| Design | 01–04 requirements, processes, domain and information; 05–08 services, responsibilities and operations; 09 installation and interfaces | Design evidence only. Step 10, application contracts, remaining numerical choices and end-to-end verification are pending. |
| Site | [Readings and names](site-model.md), [historical sources and uncertainty](site-baseline.md) | PA2 follows the mini balcony. Exact mount points, railing changes, pressure/flow, drainage, outdoor electrical and RF/hydraulic performance still need site checks. |
| Hardware | Diagnostic S01 read the Vietduino ESP32 and its 16 MiB physical flash over USB. The [first-stage components](design/components-zone-a.vi.md) and proposed wiring are documented. | S01 does not prove a watering firmware build or the 12 V supply/driver/valve behavior. No installed-system or water acceptance. |
| Representation | [Shared model](representation.md) and [viewer](../viewer/README.md) show the planned source, valve, route and pre-Y endpoint. | The model records measured anchors and estimates. Rendering does not verify mounting, water or network performance. |
| Walkthrough | Static HTML, seven SVG/seven PNG, synthetic data and source-bound views; portable data generator/semantic validator | Reuses unchanged prior behavior/figure evidence. Builder checks do not recertify native exports, independent review, firmware, scheduler, hardware or water. |
| Skills / licenses | [Three technical packages](skills.md), provenance/version/license metadata and [notices](../THIRD_PARTY_NOTICES.md) | Package availability does not prove build/device workflows or grant device operations. |
| Deployment | System/adapter ownership boundary is retained | No integrated app/DB/device deployment, production, contained-water or field acceptance. |

Next, resolve the remaining implementation contracts for clock, persistence, command/report transport and numerical safety bounds; build and verify the device and application slices. [Operations](operations.md) describes the evidence expected for each stage. Neither the representation nor the synthetic walkthrough establishes field readiness.

The only editable current design home is `docs/design/`. [Specification](specification.md), [architecture](architecture.md) and [design inputs](design-inputs.md) are legacy references. The spatial source is `viewer/model.json`; it is used by both SVG and Three.js.
