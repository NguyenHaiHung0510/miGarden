# Selected representation: A/SVG + B/Three.js

Owner selected **A+B/Three.js**; this migration preserves the choice and its design inputs. Scene/model product implementation is outside MG-H1. Earlier raster comparison images were synthetic illustrations, not reconstructed house geometry; their exact files/prompts/provenance are retained privately.

| View | Role / contract |
|---|---|
| A: SVG plan + section/routes | Precise labels, anchors, route/elevation distinctions, source selection and uncertainty. Useful for correction/export; not a surveyed CAD drawing by default. |
| B: Three.js simple 3D/cutaway | Understand multi-level relations, rotate/cut/select layers and inspect anchors. Shares the same canonical versioned data as SVG; visual plausibility is not measurement. |
| C: photogrammetry/COLMAP | Owner self-experiment, not a prerequisite or Agent install/run task. Finished outputs may later be imported with scale, coordinate system, capture/reconstruction provenance and uncertainty; raw source stays local. |

Future scene source should name stable anchor/object/route IDs; explicit units, axes, origin and model version; source IDs/frame timestamp; observation date; confidence `observed`, `Owner-confirmed`, `estimated`, `inferred`, `unknown`; and reason/history for corrections. Unknown coordinates remain missing/visibly uncertain. Do not infer floor heights, wall thickness, valve SKU or calibrated scale from generic architecture conventions.

Route polyline, RF straight-line and elevation are distinct measurements. Geometry guides test placement; protocol/RF performance needs real devices/workload and hydraulic performance needs measured flow/pressure/feedback. A digital view does not certify either.

miGarden owns one canonical scene source. The microSched adapter consumes a declared compatible scene/contract version; copied render assets are releases, not a competing editable authority. A correction should update both views through that same source. Exact schema, package/build versions, CDN/local-asset choice and interaction acceptance belong in the later model-v0 packet; no package, scene source or synthetic geometry is created by this docs migration.

Finished real-site SVG/drawings/3D/COLMAP are approved public content categories. Preserve source lineage without exposing raw captures, source-media filenames, EXIF/private identifiers or transcripts in the public tree. Do not require a synthetic replacement simply because a finished model represents the actual house. This candidate contains documented readings and contracts, no newly generated measured 3D/COLMAP assets.
