# AGENTS.md — miGarden

Start with the assigned task, read this entry, then only the product documents needed for that task and the matching skill. No README reread or full-document sweep is required. This repository contains design documentation, technical skills and a runnable model-v0 prototype in viewer/. Firmware, control simulator and microSched integration are not implemented.

## Task routing

| Task | Read as needed |
|---|---|
| Scope or implementation status | [Project state](docs/project-state.md) |
| Domain, API, command or schedule | Relevant sections of [specification](docs/specification.md) and [architecture](docs/architecture.md) |
| Geometry, map or scene | [Representation](docs/representation.md), [model-v0](docs/model-v0.md), relevant [site readings](docs/site-baseline.md) |
| Board, firmware, build or recovery | Relevant [operations runbook](docs/operations.md), specification §§14–15 for actuation |
| Technical skill | Matching entry in [skills](docs/skills.md), then only needed references |

Every Agent may automatically select suitable available skills within the assigned task. A named invocation is optional; do not read all skills for every task or restrict selection by role. Keep one writer per file and one executor per exclusive device/serial resource.

## Product boundaries

- Zone A is one shared circuit; preview and history include all affected pots. Pots and placements are dynamic. Per-pot independent actuation and Zone B remain future extensions.
- Local deterministic schedule and source START/STOP are selected designs. Remote, schedule and local input converge at one safety arbiter. Soil/Zigbee sensing is HOLD; its conditional contracts do not make it a current requirement.
- A/SVG and B/Three.js share one canonical versioned spatial source. Do not invent coordinates, calibrated scale or RF/hydraulic performance to fill gaps.
- miGarden owns firmware/local arbiter, simulator, spatial source and integration contracts. microSched owns its human web/API/storage adapter under that repository's rules. Bind both revisions and contract versions for integration changes.
- Preserve fail-closed boot, unknown-outcome blocking, cumulative local budget, no catch-up/replay and independent manual shutoff in specification §§14–15. Numeric water policy, exact board/pins/framework and unresolved arbitration require design evidence; do not fill them under editorial changes.

## Changes and evidence

Work within the task's requested effects. Reading a skill or runbook does not itself request toolchain installation, serial access/reset/flash, provisioning, water operation or publication. Before device work, identify the exact board, build/config/boot, wiring, output state, exclusive resources and recovery path using [operations](docs/operations.md).

Keep simulator, dry LED, contained-water and field results distinct. GPIO, screenshots and received acknowledgements do not prove valve position or flow. Report changed files, observed checks and unresolved limitations; document checks do not prove runtime or field behavior. Apply the [public-content rules](docs/representation.md#public-content-and-private-sources) to data and assets.

If `.local/current-brief.md` exists on this checkout, read that local file for current work context; it points to `.local/owner-machine.json`, shared authority and the canonical task record. This is a conditional file pointer, not an executable hook. A public clone without `.local` uses the task and product documents above.
