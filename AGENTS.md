# AGENTS.md — miGarden

Start with the assigned task, read this entry, then only the product documents needed for that task and the matching skill. No README reread or full-document sweep is required. This repository contains design documentation, technical skills and a runnable model-v0 prototype in viewer/. Firmware, control simulator and microSched integration are not implemented.

## Task routing

| Task | Read as needed |
|---|---|
| Scope or implementation status | [Project state](docs/project-state.md) |
| Current requirements, process, domain, information, services or schedule | [Design index](docs/design/README.md), then the relevant step 01–09; application implementation is pending |
| Installation, selected components or wiring | [Step 09 installation graph](docs/design/09-device-and-component-integration.vi.md#installation-graph) and [component catalogue](docs/design/components-zone-a.vi.md); distinguish selected design from installed/tested hardware |
| Geometry, map or scene | [Representation](docs/representation.md), [model-v0](docs/model-v0.md), relevant [site readings](docs/site-baseline.md) |
| Board, firmware, build or recovery | [Operations runbook](docs/operations.md), [08 operational policy](docs/design/08-operational-view-specification.vi.md) and [09 hardware](docs/design/09-device-and-component-integration.vi.md); numeric limits and field verification remain open |
| Technical skill | Matching entry in [skills](docs/skills.md), then only needed references |

Every Agent may automatically select suitable available skills within the assigned task. A named invocation is optional; do not read all skills for every task or restrict selection by role. Keep one writer per file and one executor per exclusive device/serial resource.

## Technical documents and Owner review

The Owner reads miGarden technical documents directly to understand and review them. Author and revise the document itself for that purpose; explanations in chat or a separate presentation do not substitute for a readable design document.

- At important sections, explain the problem, proposed decision, reason, consequences and open choices in plain Vietnamese. Define technical terms at first use and connect them to concrete miGarden behavior. Keep precise contracts, formulas and traceability available below or in linked appendices without making the Owner decode them to understand the decision.
- Include diagrams or another suitable human-readable form where needed to understand important content: component/interface maps for responsibilities, sequence diagrams for cooperation, state diagrams for transitions, power-path diagrams for electrical inputs/rails, and tables or annotated images for comparisons/physical details. Choose by the information; do not require a diagram for every paragraph. Follow the Owner-selected visual format and retain editable sources plus visible exports when producing diagrams.
- For behavioral designs, keep a navigable catalogue of all defined in-scope use-case flows and alternatives. Use concrete walkthroughs and diagrams for important differences, linking shared flows rather than drawing every combination. Show preconditions, outcomes, failure/recovery behavior, what the user sees and what remains unverified. A representative example must not be presented as coverage of every case or interface.
- PM/T1 owns technical completeness, consistency and verification. Call out the consequential needs, behavior, trade-offs and reserved decisions for Owner judgment; do not require the Owner to approve each field or implementation detail. Keep detailed material accessible for deeper review when requested.
- Before presenting a document, check that its important decisions and scenarios can be understood from the file without the chat, that diagrams agree with the text and display correctly, and that open decisions and evidence limits are explicit. Use chat to point to the relevant sections, summarize changes and capture feedback. Preserve scoped design approval, implementation grants and runtime/device acceptance as separate statuses.

## Product boundaries

- Zone is the control/schedule/history unit; Area groups Zones, each Zone one Area. Preview/history are Zone-based, without managed Pot/placement/assignment or affected-pot lists. Rollout starts at A; B/C/D are possible within the unverified capacity of exactly one ESP32 Controller and multiple Actuators, conceptually one branch valve per Zone. Each Zone has 0..1 active local logical schedule. Site pots remain representation facts.
- Local deterministic schedule and source START/STOP are selected designs. Remote, schedule and local input converge at one safety arbiter. Soil/Zigbee sensing is HOLD; its conditional contracts do not make it a current requirement.
- A/SVG and B/Three.js share one canonical versioned spatial source. Do not invent coordinates, calibrated scale or RF/hydraulic performance to fill gaps.
- miGarden owns firmware/local arbiter, simulator, spatial source and integration contracts. microSched owns its human web/API/storage adapter under that repository's rules. Bind both revisions and contract versions for integration changes.
- Preserve the safety outcomes in design 01/02/04: fail-closed boot, all-source cumulative local budget, STOP priority and no old pending START after STOP, no-double/no-catch-up/no-replay, critical-storage safe stop/degraded, and independent manual shutoff. Unknown after possible execution blocks every source and all Zones on a common source/accounting boundary unless independence is proven. Implementation details and numerical policy remain open under Step 08; SAF-P01 and 165 min/3 h are unselected candidates. Step 09 specifies first-stage hardware; it does not establish field safety or runtime acceptance.

`docs/design/` is the only editable current product-design source. Earlier working snapshots are history. Specification, architecture and design-inputs at their old paths are LEGACY/reference-only; their internal CHỐT/MUST/API/thresholds do not automatically govern current work. Application development and end-to-end acceptance remain pending. No implicit inventory/Area/Source/Controller CRUD, revision/application management, notification lifecycle or group domain is selected. Viewer geometry remains `viewer/model.json`.

## Changes and evidence

Work within the task's requested effects. Reading a skill or runbook does not itself request toolchain installation, serial access/reset/flash, provisioning, water operation or publication. Before device work, identify the exact board, build/config/boot, wiring, output state, exclusive resources and recovery path using [operations](docs/operations.md).

Keep simulator, dry LED, contained-water and field results distinct. GPIO, screenshots and received acknowledgements do not prove valve position or flow. Report changed files, observed checks and unresolved limitations; document checks do not prove runtime or field behavior. Apply the [public-content rules](docs/representation.md#public-content-and-private-sources) to data and assets.

If `.local/current-brief.md` exists on this checkout, read that local file for current work context; it points to `.local/owner-machine.json`, shared authority and the canonical task record. This is a conditional file pointer, not an executable hook. A public clone without `.local` uses the task and product documents above.
