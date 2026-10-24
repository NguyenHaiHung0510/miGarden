# Model-v0 — balcony garden prototype

Source baseline **2026-10-24, Asia/Saigon**, `model-v0-draft-05`. [Runnable viewer and quickstart](../viewer/README.md). This is a visual collaboration prototype, not a surveyed reconstruction or irrigation controller.

## Represented site

- Zone A/room immediately below the rooftop source. Facing the room from the balcony, the window is left of the main door.
- Mini balcony Zone B is above the door area, approximately3/4 rooftop width and reaching the right wall. Its geometry does not enable Zone B irrigation.
- The neighboring right wall extends from rooftop to balcony. The owner's ledge/parapet and existing pipes are separate objects. Mounting to the neighbor wall remains unselected.
- The main balcony fence has an approximately10cm wall base. Distribution pipe follows the fence;◇ markers identify proposed support locations, not engineered fixings.
- Tank/source axis is perpendicular to the right wall. Existing tap has a red manual handle, approximately25cm above rooftop floor. Positions and missing sizes remain estimated.
- Right routes approach horizontally or lower above the floor pipe, rise before the owner's ledge, then pass above its pipe and Zone B to Zone A. Left perimeter and floor diagonal remain comparisons; the central floor should stay usable.

## Shared source and uncertainty

`viewer/model.json` holds version, axes, object/source IDs, statuses and estimated conversion factors for both views. Physical XYZ remain unknown. Image labels retain provisional endpoints:7.79m and7.25m+0.77m are not one confirmed span;3.01m/1.01m endpoints are unresolved. Earlier survey anchor names in [site baseline](site-baseline.md) are historical; viewer object `W` is retained for compatibility but displays the corrected main door, while `window-left` identifies the window.

| Route | Estimated total | Crest above rooftop floor |
|---|---:|---:|
| Left perimeter |21.25m|0.51m|
| Right horizontal |18.23m|0.86m|
| Right low approach |18.29m|0.86m|

Totals include balcony distribution, exclude installation slack and do not prove pressure/flow. Editing points recalculates the current route; preset cards describe their baseline geometry.

Source options retain the single-outlet tap or propose a replacement with≥2 outlets: miGarden plus ordinary manual use. A downstream splitter after the old tap remains another fitting option. SKU/thread/pipe ID/pump/mounting and numeric water policy are not selected by the viewer. Ordinary use is outside the controlled circuit.

Pots are dynamic,9 samples are not fixed slots. Current product direction is one shared Zone A circuit with manual branch shutoffs. Per-outlet telemetry/actuation feasibility is not settled by the illustration.

## Publication and evidence

The finished actual-site geometry is public with uncertainty. Raw captures, frames, private transcripts, personal machine paths and feedback stay excluded. A/B/V source IDs identify observations; R01–R04 are supplemental parapet/left-route/right-wall/source-detail references without private filenames.

The local server is a preview/save adapter, not an implemented microSched integration. miGarden owns the canonical spatial source; future adapters must bind versions.

Three Node test groups pass on Node24.15.0 / Three.js0.186.1: source/route/SVG integrity; operation without private photos and guarded/conflict-safe feedback; private source alias boundaries. Published files are reviewed for private paths. No CI pipeline or Internet deployment is claimed.

Basic live WebGL rendering and3D focus mode were observed in the browser for this public package. Extended camera/drag/save-UI matrices are **NOT_RUN**; owner requested proportional live checks and early visual review. Earlier owner iteration and these checks do not prove surveyed geometry, physical-device behavior or hydraulics. Firmware/control simulator/microSched integration remain unimplemented.

Next: owner corrections and route/source choice; a bounded simulator/dry-LED slice; source/pipe/flow measurements and mounting design before contained-water tests. See [project state](project-state.md) and [operations](operations.md).
