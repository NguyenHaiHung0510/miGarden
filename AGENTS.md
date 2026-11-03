# AGENTS.md — miGarden

Start with the assigned task, read this entry, then only the product documents needed for that task and the matching skill. No README reread or full-document sweep is required. This repository contains design documentation, technical skills and a runnable model-v0 prototype in viewer/. Firmware, control simulator and microSched integration are not implemented.

## Task routing

| Task | Read as needed |
|---|---|
| Scope or implementation status | [Project state](docs/project-state.md) |
| Current requirements, process, domain, information or schedule | [Design index](docs/design/README.md), then relevant 01–04; API/service/operational/device/application steps 05–10 are pending |
| Geometry, map or scene | [Representation](docs/representation.md), [model-v0](docs/model-v0.md), relevant [site readings](docs/site-baseline.md) |
| Board, firmware, build or recovery | Relevant [operations runbook](docs/operations.md); current safety outcomes in design 01/02/04, hardware and numeric policy still open |
| Technical skill | Matching entry in [skills](docs/skills.md), then only needed references |

Every Agent may automatically select suitable available skills within the assigned task. A named invocation is optional; do not read all skills for every task or restrict selection by role. Keep one writer per file and one executor per exclusive device/serial resource.

## Product boundaries

- Zone is the control/schedule/history unit; Area groups Zones, each Zone one Area. Preview/history are Zone-based, without managed Pot/placement/assignment or affected-pot lists. Rollout starts at A; B/C/D are possible within the unverified capacity of exactly one ESP32 Controller and multiple Actuators, conceptually one branch valve per Zone. Each Zone has 0..1 active local logical schedule. Site pots remain representation facts.
- Local deterministic schedule and source START/STOP are selected designs. Remote, schedule and local input converge at one safety arbiter. Soil/Zigbee sensing is HOLD; its conditional contracts do not make it a current requirement.
- A/SVG and B/Three.js share one canonical versioned spatial source. Do not invent coordinates, calibrated scale or RF/hydraulic performance to fill gaps.
- miGarden owns firmware/local arbiter, simulator, spatial source and integration contracts. microSched owns its human web/API/storage adapter under that repository's rules. Bind both revisions and contract versions for integration changes.
- Preserve the safety outcomes in design 01/02/04: fail-closed boot, all-source cumulative local budget, STOP priority and no old pending START after STOP, no-double/no-catch-up/no-replay, critical-storage safe stop/degraded, and independent manual shutoff. Unknown after possible execution blocks every source and all Zones on a common source/accounting boundary unless independence is proven. Exact arbitration/clock/recovery/hardware and numeric policy remain open; SAF-P01 is a proposed Step 08 candidate, never an approved limit/default.

`docs/design/` is the only editable current product-design source. Earlier working snapshots are history. Specification, architecture and design-inputs at their old paths are LEGACY/reference-only; their internal CHỐT/MUST/API/thresholds do not automatically govern current work. Steps 05–10 remain pending. No implicit inventory/Area/Source/Controller CRUD, revision/application management, notification lifecycle or group domain is selected. Viewer geometry remains `viewer/model.json`.

## Changes and evidence

Work within the task's requested effects. Reading a skill or runbook does not itself request toolchain installation, serial access/reset/flash, provisioning, water operation or publication. Before device work, identify the exact board, build/config/boot, wiring, output state, exclusive resources and recovery path using [operations](docs/operations.md).

Keep simulator, dry LED, contained-water and field results distinct. GPIO, screenshots and received acknowledgements do not prove valve position or flow. Report changed files, observed checks and unresolved limitations; document checks do not prove runtime or field behavior. Apply the [public-content rules](docs/representation.md#public-content-and-private-sources) to data and assets.

If `.local/current-brief.md` exists on this checkout, read that local file for current work context; it points to `.local/owner-machine.json`, shared authority and the canonical task record. This is a conditional file pointer, not an executable hook. A public clone without `.local` uses the task and product documents above.
