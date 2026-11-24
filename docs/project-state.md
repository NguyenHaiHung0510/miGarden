# Current project state

As of **2026-11-24, Asia/Saigon**, Zone A has an installed IoT setup integrated with microSched. It supports watering from the web, the applied schedule and the two local buttons. The remaining installation work is to run the water line along the balcony railing to the pots. The [miGarden v1.0.0](releases/v1.0.0.vi.md) record links the firmware source, the deployed app at cutover, controller CI and the first authenticated health sample. The [SVG/Three.js viewer](model-v0.md), [design steps 01–10](design/README.md) and [host simulator](../code/controller/README.vi.md) document the intended wiring and behavior.

## Selected design and implementation boundary

| Area | Current baseline |
|---|---|
| Domain | Zone is the control/schedule/history unit; Area groups Zones, each Zone one Area. No managed Pot/placement/assignment or affected-pot lists. Site pots remain physical representation facts. |
| Controller / expansion | Exactly one existing ESP32 Controller → multiple Actuators within unverified hardware capacity; conceptually one branch valve/Zone. Rollout A first; model allows B/C/D. No Controller/Source/Area inventory CRUD or group domain; orchestration is future work. |
| Schedule / history | 0..1 active local logical schedule/Zone. Fixed Hanoi UTC+7, once/daily/weekly/interval; configuration resolution 1 minute and duration increment 15 seconds. Desired differs from last-known applied; historical immutable content/binding refs differ from current config. |
| Control / safety | Local schedule, source START/STOP and remote converge at one arbiter; cumulative limits across sources, STOP priority/pending invalidation, no-double/no-catch-up/no-replay, unknown block, fail-closed boot, critical-storage safe stop/degraded and independent manual shutoff. The N01 candidate uses the [initial Zone A defaults](design/08-operational-view-specification.vi.md#budget-d186); field verification remains open. |
| First-stage installation | One ESP32, two separate 12 V/2 A adapters, MKE-M06, one VAKS normally closed 12 V valve and two normally open buttons. DNV5 supplies a manual wash outlet and Zone A; two ports capped. One flexible hose leads to the valve, then rigid uPVC reaches a capped stub below the old railing. Y, emitters, filter and dedicated indicator are deferred. [Graph and parts](design/09-device-and-component-integration.vi.md). |
| Sensing / scope | Soil/Zigbee remains HOLD. Current error/freshness views retained; no inherited notification lifecycle or plant inventory requirement. |
| Spatial model | A/SVG + B/Three.js consume one canonical versioned `viewer/model.json` with provenance/uncertainty. COLMAP remains optional; not measured output here. |
| Ownership | miGarden owns system/local arbiter, simulator, spatial source and integration contracts; microSched owns human web/API/storage adapter. Compatibility must bind both revisions/contract versions. |

## Available evidence and gaps

| Area | Available evidence | Limits / remaining work |
|---|---|---|
| Design | 01–04 requirements, processes, domain and information; 05–08 services, responsibilities and operations; 09 installation; 10 development and integration | Phase direction, architecture, initial policy and UI C selected. N01 pins its own wire contract and requires end-to-end verification. |
| Controller | [C++17 core, Arduino-ESP32 profiles and host tests](../code/controller/README.vi.md); [54-row QA mapping](../code/controller/QA.vi.md) | LAB has virtual output only. Bare-board TLS, actual storage, buttons, driver, power-loss and water acceptance remain distinct from host and compile evidence. |
| Commissioning | [Current-layout preserving commissioner/CLI](../code/controller/CURRENT-COMMISSIONING.vi.md), separate temporary USB image and offline validation | LAB journal-only plan preserves raw NVS; release adds only a new namespace. Actual data init/production credentials remain HOLD; existing journals/unknown are never overwritten. Parent-reported4b LAB TLS/crossstack evidence is distinct from new commissioner/device acceptance. |
| Site | [Readings and names](site-model.md), [historical sources and uncertainty](site-baseline.md) | PA2 follows the mini balcony. Exact mount points, railing changes, pressure/flow, drainage, outdoor electrical and RF/hydraulic performance still need site checks. |
| Hardware | S01 reported 16 MiB physical flash. The [first-stage installation](design/09-device-and-component-integration.vi.md) has the ESP32, valve and local buttons. The cutover records the first boot and health sample. | The original S01 diagnostic was an earlier board-only check. The final water path and flow at each pot remain to be checked after the pipe is installed. |
| Representation | [Shared model](representation.md) and [viewer](../viewer/README.md) show the planned source, valve, route and pre-Y endpoint. | The model records measured anchors and estimates. Rendering does not verify mounting, water or network performance. |
| Walkthrough | Static HTML, seven SVG/seven PNG, synthetic data and source-bound views; portable data generator/semantic validator | Reuses unchanged prior behavior/figure evidence. Builder checks do not recertify native exports, independent review, firmware, scheduler, hardware or water. |
| Skills / licenses | [Three technical packages](skills.md), provenance/version/license metadata and [notices](../THIRD_PARTY_NOTICES.md) | Package availability does not prove build/device workflows or grant device operations. |
| Deployment | Zone A app/DB/controller cutover. Wire v1 includes negotiated quiet transport and optional ESP32 health. See [miGarden v1.0.0](releases/v1.0.0.vi.md). | The installed system demos and dry boot/health receipt cover the recorded tasks. Confirm supply and watering at each pot after the final pipe is installed. |

Next, complete the balcony line and record the contained-water and field checks. [Operations](operations.md) lists the evidence for each stage. The viewer and synthetic walkthrough remain available for explaining the system.

The only editable current design home is `docs/design/`. [Specification](specification.md), [architecture](architecture.md) and [design inputs](design-inputs.md) are legacy references. The spatial source is `viewer/model.json`; it is used by both SVG and Three.js.
