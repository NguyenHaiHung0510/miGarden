# Selected representation: A/SVG + B/Three.js

The selected design combines SVG and Three.js views of one versioned spatial source. A runnable [model-v0 prototype](model-v0.md) is implemented in [viewer/](../viewer/README.md), alongside the design contract and preliminary readings. The [2026-10-25 field baseline](site-model.md) adds approved tape readings and names; unmeasured geometry remains estimated, not surveyed.

| View | Role and contract |
|---|---|
| A: SVG plan + section/routes | Labels, anchors, route/elevation distinctions, source selection and uncertainty. Useful for correction/export; not a surveyed CAD drawing by default. |
| B: Three.js simple 3D/cutaway | Multi-level relations, rotation/cutaway/layer selection and anchor inspection. Shares the same canonical data as SVG; visual plausibility is not measurement. |
| C: photogrammetry/COLMAP | Optional independent experiment, not a prerequisite for A+B. Import finished output with scale, coordinate system, reconstruction provenance and uncertainty. |

## Shared source and corrections

The future scene source needs stable anchor/object/route IDs; explicit units, axes, origin and model version; source IDs/frame timestamps; observation date; confidence `observed`, `Owner-confirmed`, `estimated`, `inferred`, `unknown`; and reason/history for corrections. Unknown coordinates stay missing or visibly uncertain. Do not infer floor heights, wall thickness, valve SKU or calibrated scale from generic architecture conventions.

Route polyline, RF straight-line and elevation are different measurements. Geometry guides test placement; RF needs real devices/workload and hydraulics need measured flow/pressure/feedback. See [site baseline](site-baseline.md).

miGarden owns one canonical scene source. The microSched adapter consumes a compatible scene/contract version; copied render assets are releases, not another editable authority. A correction updates both views through the same source. The prototype pins Three.js locally and records its model/schema version; production packaging and automated live interaction acceptance remain open. Earlier raster comparisons were synthetic illustrations, not reconstructed site geometry.

## Public content and private sources

Finished drawings, SVG, 3D models and COLMAP outputs of the actual site may be public. They do not need synthetic replacements. Keep source lineage, scale and uncertainty useful to readers, while excluding raw captures and private identifiers from published artifacts.

Raw photos/video, extracted frames/contact sheets, private transcripts/ledgers/receipts, credentials and personal machine paths stay in ignored local storage. Preserve original source bytes. Before publishing a finished asset, review EXIF/embedded metadata, visible secrets and third-party content; use public-safe source IDs rather than private filenames or paths. No measured 3D/COLMAP reconstruction is present in this repository yet.
