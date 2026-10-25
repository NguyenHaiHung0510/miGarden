# Model-v0 — balcony garden prototype

Baseline **2026-10-25, Asia/Saigon**, model-v0-draft-06. [Canonical field geometry, names and readings](site-model.md) · [Viewer quickstart](../viewer/README.md). The Owner approved the finished model and detailed readings as the current canonical field representation for publication. Raw captures stay private. This is not a complete survey or irrigation controller.

## Shared source

viewer/model.json supplies SVG and Three.js with metre units, stable IDs,14 tape readings, source references and explicit inferred/estimated dimensions. Main balcony/room lie immediately below the rooftop. Window is left of the main door. Mini Zone B: outside width3.13m, depth1.50m excluding the divider but including the outer wall, clear left gap1.00m. Its geometry does not enable irrigation.

The owner's tường thấp phải and tall neighboring wall are separate. The main route uses the right perimeter. Existing pipe diameter10cm is not a clearance allowance. Source→top mini edge is8.90m aggregate; down to fence top2.60m; inward/down≈0.10m each; fence4.00m. Total chain≈15.70m excludes pot branches. Buffer is separate and editable. Main source option replaces the tap with two outlets, manual miGarden isolation preceding a future electronic valve.

Missing absolute heights, wall thicknesses, equipment dimensions and illustrative new-pipe size remain estimated. Historical7.79m,7.25m+0.77m and3.01m/1.01m readings retain unresolved endpoints and do not override the selected route. Old W is retained as the main-door ID for note compatibility. See [historical baseline](site-baseline.md).

## Viewer behavior

- Flow-ordered quick focus includes separate source/tank/pump/wall/fence objects.
- Orbit/pan plus **Bay**: active-canvas WASD movement, Q/E down/up, ←/→ yaw through360°, ↑/↓ look up/down, Shift fast, right-drag look, Esc to orbit. Blur stops movement; editing points switches to orbit.
- Highlight is optional. Turning off **Số đo / ước lượng / nhãn** gives a clear view without text overlays, dimensions or support markers.
- Native3D fullscreen retains quick navigation; an explicit embedded-window expansion fallback is used if fullscreen is unavailable.
- Only the main route is edited. Old routes remain view-only overlays with original draft05 coordinates retained.
- Feedback is versioned/revision guarded. Old draft03–05 coordinates stay separate; explicit recovery imports note content only.

Nine pots are dynamic examples, not fixed capacity. Zone A stays one shared circuit with manual branch shutoffs. Per-outlet telemetry/actuation and Zone B irrigation remain deferred.

## Evidence and next work

Four Node groups pass for geometry/SVG, guarded feedback, private aliases and edits arriving during a delayed save. Independent Luna/xhigh review and affected retest resolved SVG clear-view and save-status defects. Independent Gemini3.8Flash/high exercised the main UI journeys and a focused retest. Continuous right-drag/Shift speed were not exercised; full-cycle yaw and translation are not established merely by changed screenshot bytes. Ordinary render/focus, clear view, source/pot options, edit/save and fullscreen entry/exit have qualified browser evidence. Source/browser checks do not prove field behavior. Three.js0.186.1 is pinned locally; public clones work without private images. Raw photos, frames, transcripts, ledgers, personal paths and feedback are excluded from Git; N01–N10/FIELD-20261025 are public-safe refs.

The local preview/save adapter is not a microSched backend. Firmware, control simulator and microSched integration remain unimplemented. Next: Owner visual corrections, bounded simulator/dry-LED slice, source/flow and practical mounting evidence before contained-water work. See [project state](project-state.md) and [operations](operations.md).
