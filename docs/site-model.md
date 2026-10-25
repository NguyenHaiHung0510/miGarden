# Site model — 2026-10-25

Owner-approved **model-v0-draft-06**. The canonical editable source for geometry, names, measurements and navigation order is [viewer/model.json](../viewer/model.json). This document explains that source; SVG and Three.js consume it together. Earlier [photo-label readings](site-baseline.md) remain historical.

## Coordinate frame and confidence

One scene unit is **one metre**, +Y up. Left/right are fixed as seen standing outside the main balcony facing the main door. X runs left to right; Z from source toward the outer edge. The main-balcony floor is display Y=0. Camera rotation does not rename sides. Absolute floor heights, wall thicknesses and equipment sizes remain estimated; this is not a complete surveyed reconstruction. Draft edits/feedback do not overwrite approved readings. Legacy W is the corrected main door; window-left identifies the actual window.

## Controlled names

### 1 · Cụm nguồn

| Stable ID | Tên chuẩn |
|---|---|
| source-assembly | Cụm nguồn |
| F | Vòi nguồn miGarden · van tay |
| household-outlet | Đầu vòi sinh hoạt |
| future-valve | Vị trí van điện miGarden tương lai |
| tank | Bồn nước |
| pump | Bơm hiện hữu |
| existing-blue-valves | Van tay xanh hiện hữu |

### 2 · Sang phải và men tường

| Stable ID | Tên chuẩn |
|---|---|
| existing-floor-pipe | Ống hiện hữu gần vòi |
| right-ledge | Tường thấp phải |
| existing-ledge-pipe | Ống hiện hữu trên tường thấp phải |
| route | Tuyến chính |

### 3 · Ban công mini

| Stable ID | Tên chuẩn |
|---|---|
| terrace-divider | Tường ngăn sân thượng–mini |
| zoneB | Ban công mini · Zone B |
| mini-low-walls | Tường thấp ban công mini |
| mini-edge | Mép ngoài ban công mini |

### 4 · Xuống hàng rào / chậu

| Stable ID | Tên chuẩn |
|---|---|
| C | Điểm chuyển vào hàng rào |
| fence | Hàng rào sắt ban công chính |
| rail | Ống phân phối trên hàng rào |
| fence-base | Chân tường dưới hàng rào |
| balcony | Ban công chính · Zone A |

### 5 · Không gian tham chiếu

| Stable ID | Tên chuẩn |
|---|---|
| terrace | Sân thượng |
| W | Cửa chính |
| window-left | Cửa sổ trái |
| left-wall | Tường trái |
| right-wall | Tường hàng xóm phải |
| AP | Bộ phát Wi-Fi / AP |

Tường thấp phải is the owner's low rooftop wall, formerly the owner's ledge/parapet. Tường hàng xóm phải is the separate tall wall continuing down to the main balcony. Hàng rào sắt is the lower metal structure; ống phân phối is the pipe supported by it. The term lan can alone is avoided because it previously mixed these objects.

## Tape readings and endpoints

Reported by the Owner on2026-10-25, source FIELD-20261025. N01–N10 identify five source angles, two main-balcony wall views and three mini/transition views. Raw captures and private discussion remain excluded from Git.

| ID | Reading | Endpoint IDs | Meaning / inclusion |
|---|---:|---|---|
| M01 | 0.53m | F → right-access | Vuông góc từ nguồn sang phải. |
| M02 | 0.26m | right-access → right-wall-top | Đến mặt trên tường thấp phải, chưa gồm ống sẵn. |
| M03 | 8.90m | F → mini-edge | Tổng theo các đoạn của tuyến; gồm các đoạn con và đoạn hạ40cm ở mini. |
| M04 | 0.10m | existing-ledge-pipe-bottom → existing-ledge-pipe-top | Đường kính ống hiện hữu, không phải clearance. |
| M05 | 0.40m | mini-edge → main-route-crest | Chênh cao đã tính phần Ø10cm ống sẵn. |
| M06 | 1.50m | after-terrace-divider → mini-outer-face | Không gồm tường ngăn sân thượng–mini; có gồm tường thấp mép ngoài. |
| M07 | 3.13m | mini-left-outer-face → mini-right-outer-face | Gồm bề dày hai tường thấp bên. |
| M08 | 1.00m | left-wall-inner-face → mini-left-outer-face | Khe lọt lòng; không gồm bề dày hai tường. |
| M09 | 2.60m | mini-edge → fence-top | Mặt trên mép mini → đỉnh thanh ngang hàng rào. |
| M10 | ≈0.10m | fence-top → rail-inward | Đi vào phía cửa chính. |
| M11 | ≈0.10m | rail-inward → C | Đi xuống để vào hàng rào. |
| M12 | 4.00m | rail-start → rail-end | Chiều dài hàng rào sắt, chưa tính nhánh chậu. |
| M13 | 3.00m | AP → window-plane | Vuông góc từ AP ra mặt cửa sổ. |
| M14 | 1.70m | room-floor → AP | AP sát tường trái; cao so sàn phòng. |

M03 is an **aggregate along the route**, not a longitudinal length. Its subsegments are not added again. M04 is the existing pipe's10cm **diameter**, not an additional10cm clearance. M06 excludes the terrace–mini divider and includes the outer low-wall thickness. M07 includes both side walls; M08 is clear width. M09 starts at the **top** of the outer mini edge and ends at the fence crossbar top.

The longitudinal run≈7.60m is **derived**, not independently measured:8.90−0.53−0.37−0.40. The0.37m rise consists of0.26m to wall top,0.10m existing pipe and an illustrative0.01m new-pipe radius. The aggregate includes the0.40m transition to the top-edge anchor. This endpoint interpretation is explicit so later visual corrections can refine it without discarding the original readings.

Display terrace floor3.53m, mini floor3.55m, mini-edge top3.75m, fence top1.15m and route crest4.15m are **estimated absolute elevations** constrained by reported relative dimensions. Assumed new-pipe diameter2cm is not selected hardware. Mini wall thickness≈10cm and divider≈12cm are estimates and are not double counted in outside dimensions. No extra10cm clearance is added.

## Selected route and source

The main route goes right from source, over the existing pipe on the owner's low wall, along the right perimeter past the mini, down to the main fence, inward≈10cm, down≈10cm and along the4m fence. Only this route is developed/edited. Old right-horizontal, right-low, left and diagonal options remain view-only comparisons, reprojected from draft05 with original coordinates retained. They are not new measured installation alternatives.

Main source option: **replacement two-outlet tap**, ordinary-use outlet plus miGarden branch. The miGarden manual valve provides hard upstream isolation before a future electronic valve. The old single-outlet tap remains a reference. Source detail only needs to identify and avoid the overlapping existing assembly; it does not certify each old pipe's function, nominal thread, flow direction or pump state.

Baseline chain≈**15.70m**:8.90+2.60+0.10+0.10+4.00, excluding individual pot branches and unmeasured mounting detours. Purchase **buffer** is separate and editable; default example3m gives18.70m before pot branches/sales-roll sizing. The3m is an Agent proposal, not an Owner-selected purchase quantity. Longer pipe can be cut; the viewer performs no purchase.

Supports remain schematic; no neighbor-wall drilling/fixing permission is implied. Zone A stays one shared circuit with dynamic pots and illustrative manual branch shutoffs. Zone B irrigation is deferred. Geometry does not prove pressure, flow, RF delivery, mounting strength or device operation.
