# Site baseline and evidence limits

Public derivative of the approved survey/media inputs, reconciled 2026-10-22. Actual house geometry/drawings/3D/COLMAP may be public; this document carries source readings without raw captures or private account identifiers. Exact source filenames/bytes/hashes, frames and contact sheets are in the ignored local manifest. The original survey remains immutable privately.

## Source inventory and approval

23 source images (A01–A11, B01–B12) + V01 video, **42,747,201 bytes** in the recorded inventory; A11 is an earlier Owner annotation derivative retained as a source input. Source use was approved; that does not calibrate measurements or determine every XYZ/endpoint. Directory dates are evidence grouping dates, not verified capture dates.

V01 metadata recorded 720×1280, 30 fps, 1,981 decoded frames, about 66.03 s. Review v2 sampled **24 PNG frames**, 0.000–66.000 s, gap ≤2.90 s, and prepared 11 source-image groups + 6 video groups = **17 sheets**. Review v1's 3 sheets remain privately for provenance. Audio and continuous motion were **NOT_REVIEWED** in the source review; migration does not add review. A contact sheet helps locate evidence; open the native source for small text. Grouping creates no additional photogrammetry overlap.

## Anchors and preliminary route readings

| Anchor | Meaning |
|---|---|
| G | Farthest historical balcony pot |
| C | Window-side corner / balcony route collection |
| W | Window pass-through into room |
| P | Candidate room power socket |
| AP | Room Wi-Fi access point |
| R | Transition from corner toward upper level |
| U | Upper-level rim point |
| Q | Transition toward pipe/water-source route |
| X | Intermediate point near pump/faucet cluster |
| F | Intended upper-level water source/faucet area |

| Segment | Recorded reading | Provenance / confidence |
|---|---:|---|
| G → C | 3.85–3.89 m | A01/02/03/05/07/08 have differing labels; preserve range. |
| C → W | 1.13 m | A01/03/05/06; photo label, uncalibrated. |
| W → P | 2.04–2.05 m | A06 differs from A01/03/05; preserve both. |
| P → AP | 1.36 m | A03/06; preliminary route label. |
| C → R | 3.01 m | A01/03/04; route, not certified vertical height. |
| R → U | 1.01 m | A02/03/04. |
| U → Q | 0.41–0.44 m | A02/03; endpoint/calibration uncertainty. |
| Q → X | 7.25 m | A03; endpoint assignment inferred. |
| X → F | 0.77 m | A03; endpoint assignment inferred. |

The old Q→F ≈8 m description fits 7.25+0.77=8.02 m, but the endpoint assignment remains an inference. G→F polyline total 16.30–16.37 m is not a purchasing/cutting length or RF straight-line distance. Include real routing/bends/slack only after measurement; no XYZ, floor height or wall thickness is solved by these sums. B03/B04 may show repeated measurement; the fitting/ruler/pump photos do not establish nominal threads or real pressure/flow.

## Physical and operating context

Owner-described historical context: nine pots, dynamic inventory; tiled floor with a drain; pots have bottom holes and drain quickly in ordinary manual watering, often roughly every two days. Those observations do not prove safe drainage during an unbounded leak. Main balcony is Zone A; mini balcony/Zone B remains future scope. Source is upstairs; room is closed from balcony/roof, and Owner prefers avoiding new through-room cables/drilling.

The target faucet is described as below-left of the large green rotary valve in A03; do not conflate faucet, valve, pump and tank. Source-area candidate mains supply near the pump was Owner-identified in A11. Exact socket/load/earthing/rain cover and safe sharing remain unverified. Balcony has no identified outside-room supply. Source area can receive wind-driven rain/mist; source node is not an indoor dry enclosure by assumption.

There is a pressure-boosting pump that may start on water demand; partially opening the tap may not trigger it. Pump-off or power loss does not imply zero pressure. Exact pump/controller/threshold, static/dynamic pressure and effects of other taps remain unknown. Manual water shutoff location, flow/leak cutoff, fittings/pipe ratings, actuator/driver and outdoor electrical design remain open.

## Network context: estimates, not acceptance

| Observation / estimate | What it supports / does not support |
|---|---|
| Historical phone tests: source 0.34 Mbps down / 1.58 up; room/balcony 11.1 down / 21.8 up | Connectivity on a phone at those moments; neither ESP32 delivery/reconnect nor board SLA. Ping samples are not a location ranking. |
| AP → source controller estimated 5–8 m, diagonal concrete total estimated 25–60 cm | Plan a real-board workload/fault test; no RF proof from geometry. |
| Room PC → AP estimated 0–2 m | Candidate supervised-bench placement, not permanent-host choice. |
| Optional balcony soil radio → room estimated 3–6 m through ~10 cm wall | Historical optional sensing context only; HOLD remains in force. |

Field ESP32 is expected at the source using LAN/Wi-Fi; USB-C is for supervised dry bench/programming/debug, not a room-to-roof runtime cable. Required real-board network gates are preserved in the spec and operations, all **NOT_RUN** here.

Next useful measurements: Owner identifies exact faucet/anchors and changed placements; dimensions/scale/elevation where A+B needs them; safe manual flow measurements and pump states; drainage/route/weather/electricity details appropriate to the next approved water/hardware batch. Do not repeat source-use approval or invent endpoints to make a complete model.
