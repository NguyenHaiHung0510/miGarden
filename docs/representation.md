# Selected representation: A/SVG + B/Three.js

The selected design combines SVG and Three.js views of one versioned spatial source. A runnable [model-v0 prototype](model-v0.md) is implemented in [viewer/](../viewer/README.md), alongside the design contract and preliminary readings. The [2026-10-25 field baseline](site-model.md) adds approved tape readings and names; unmeasured geometry remains estimated, not surveyed.

| View | Role and contract |
|---|---|
| A: SVG plan + section/routes | Labels, anchors, route/elevation distinctions, installation stages and uncertainty. Useful for correction/export; not a surveyed CAD drawing by default. |
| B: Three.js simple 3D/cutaway | Multi-level relations, rotation/cutaway/layer selection and anchor inspection. Shares the same canonical data as SVG; visual plausibility is not measurement. |
| C: photogrammetry/COLMAP | Optional independent experiment, not a prerequisite for A+B. Import finished output with scale, coordinate system, reconstruction provenance and uncertainty. |

This document governs spatial representation and its public-content boundary. The product design lives in [design 01–09](design/README.md); pots shown in the viewer are spatial examples, not managed inventory. SVG and Three.js use `viewer/model.json`; the synthetic walkthrough illustrates information semantics separately.

## Shared source and corrections

The future scene source needs stable anchor/object/route IDs; explicit units, axes, origin and model version; source IDs/frame timestamps; observation date; confidence `observed`, `Owner-confirmed`, `estimated`, `inferred`, `unknown`; and reason/history for corrections. Unknown coordinates stay missing or visibly uncertain. Do not infer floor heights, wall thickness, valve SKU or calibrated scale from generic architecture conventions.

Route polyline, RF straight-line and elevation are different measurements. Geometry guides test placement; RF needs real devices/workload and hydraulics need measured flow/pressure/feedback. See [site baseline](site-baseline.md).

miGarden owns one canonical scene source. The microSched adapter consumes a compatible scene/contract version; copied render assets are releases, not another editable authority. A correction updates both views through the same source. The prototype pins Three.js locally and records its model/schema version; production packaging and automated live interaction acceptance remain open. Earlier raster comparisons were synthetic illustrations, not reconstructed site geometry.

### Model đợt một Zone A

Revision lắp đặt 12/11 giữ nguyên hình học baseline 07-r2. Hai view lấy tuyến PA2 uPVC, cụm DNV5/dây/van và đầu chờ từ cùng model. Đợt một dùng nét liền; sau Y và phần nới dùng nét đứt. Hai nhánh chỉ biểu diễn hướng trong Zone A. Các preset 07 là đối chiếu; nháp cũ giữ tọa độ riêng.

[Ảnh nhà 3D](assets/migarden-zone-a-hero.png) dùng cùng renderer và model, với hai camera trực giao (toàn nhà và cận nguồn). [Nguồn tái tạo](assets/migarden-zone-a-hero.html) xuất PNG local tại đường dẫn `/hero` của viewer, không tải ảnh gốc. Màu và bề dày tuyến trên ảnh được nhấn để đọc ở kích thước README.

## Public content and private sources

Finished drawings, SVG, 3D models and COLMAP outputs of the actual site may be public. They do not need synthetic replacements. Keep source lineage, scale and uncertainty useful to readers, while excluding raw captures and private identifiers from published artifacts.

Raw photos/video, extracted frames/contact sheets, private transcripts/ledgers/receipts, credentials and personal machine paths stay in ignored local storage. Preserve original source bytes. Before publishing a finished asset, review EXIF/embedded metadata, visible secrets and third-party content; use public-safe source IDs rather than private filenames or paths. No measured 3D/COLMAP reconstruction is present in this repository yet.
