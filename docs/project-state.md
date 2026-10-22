# Current project state

As of **2026-10-22, Asia/Saigon**, miGarden is a documentation/design project. Product requirements and contracts exist; firmware, simulator, spatial viewer and microSched integration have not been implemented here.

## Selected design

The [product decisions](specification.md#20-product-decisions) define the current design. Their practical consequences are:

| Area | Current design |
|---|---|
| Garden domain | Dynamic `Pot`, placement/planting and assignment history. Nine pots are a survey snapshot. |
| Irrigation | One Zone A shared circuit, zone/exact-outlet target and all affected pots in preview/history. Independent per-pot actuation and Zone B are deferred. |
| Control | Web “Tưới ngay”, versioned local deterministic schedule and START/STOP at the source. Offline continuation requires trusted time/config/ledger/safety; no catch-up or double-run. Balcony button is optional. |
| Sensing | Soil/Zigbee, ZBDongle-P and sensing-only host/materials are HOLD indefinitely. Generic device/telemetry contracts remain useful; soil recommendation is conditional. |
| Spatial model | A/SVG + B/Three.js consume one canonical versioned source with provenance and uncertainty. COLMAP is an optional independent experiment, not an A+B prerequisite. |
| Repositories | miGarden owns the system; microSched owns web/API/Postgres adapter. Versioned compatibility is required across releases. |

## Available evidence and gaps

| Area | Available | Still unverified |
|---|---|---|
| Requirements | Detailed domain, command/ack, schedule and safety contracts | Executable behavior and integration |
| Site | [Anchors and preliminary route readings](site-baseline.md), with source references and uncertainty | Exact XYZ/scale/endpoints, changed placements, pressure/flow, drainage and outdoor electrical suitability |
| Hardware | P1A bench kit arrival reported by the project owner; planned Vietduino ESP32 16MB HS2250V + USB-C HS1369, breadboard/jumpers/red LED/330Ω/DIP button | Exact-unit identity, board/pins/toolchain, USB/flash/bench; water actuator/driver/power/feedback selection |
| Representation | [Shared SVG/Three.js data contract](representation.md) | Implemented scene, viewer, measured geometry and COLMAP assets |
| Skills | Three installed Markdown packages with source/license/version metadata; [usage and compatibility](skills.md) | Firmware/build/device/rendering workflows and discovery in other runtimes |
| Deployment | Planned [microSched adapter boundary](architecture.md) | App/DB integration, network/cost probes, contained-water/field and production acceptance |

The next design work is to verify site anchors/endpoints, define model-v0 and dry-LED semantics, and resolve the relevant implementation choices before building. Exact framework/board/pins/partition, time/persistence/recovery, remote/manual/schedule arbitration, transport/cost caps and numeric water policy remain open. The [operations runbooks](operations.md) describe the evidence needed to progress.
