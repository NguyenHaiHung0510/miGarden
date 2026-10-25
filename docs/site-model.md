# Site model — 2026-10-25

Owner-approved **model-v0-draft-07**. The canonical editable source for geometry, names, measurements and navigation order is [viewer/model.json](../viewer/model.json). This document explains that source; SVG and Three.js consume it together. Earlier [photo-label readings](site-baseline.md) remain historical.

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
| S | S · Bắt đầu đoạn thẳng 890cm |

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

Tường thấp phải is the Owner low rooftop wall, separate from the tall neighbor wall and lower metal fence. S is the reference after the source access/rise/sideways seating.

## Tape readings and endpoints

Reported by the Owner on2026-10-25, source FIELD-20261025. N01–N10 identify five source angles, two main-balcony wall views and three mini/transition views. Raw captures and private discussion remain excluded from Git.

| ID | Reading | Endpoint IDs | Meaning / inclusion |
|---|---:|---|---|
| M01 | 0.53m | F → right-access | Vuông góc từ nguồn sang phải. |
| M02 | 0.26m | right-access → right-wall-top | Đến mặt trên tường thấp phải, chưa gồm ống sẵn. |
| M03 | 8.90m | S → main-route-crest | Một đoạn thẳng8,90m sau khi đặt ống lên tường, không tính từ vòi hoặc gồm hạ40cm. PA2 tách phần này7,40m và1,50m. |
| M04 | 0.10m | existing-ledge-pipe-bottom → existing-ledge-pipe-top | Đường kính ống hiện hữu, không phải clearance. |
| M05 | 0.40m | mini-edge → main-route-crest | Chênh cao tham chiếu40cm từ mép mini lên tuyến cao, đã tính phầnốngØ10cm; không phải đầu mốc260cm. |
| M06 | 1.50m | after-terrace-divider → mini-outer-face | Không gồm tường ngăn sân thượng–mini; có gồm tường thấp mép ngoài. |
| M07 | 3.13m | mini-left-outer-face → mini-right-outer-face | Gồm bề dày hai tường thấp bên. |
| M08 | 1.00m | left-wall-inner-face → mini-left-outer-face | Khe lọt lòng; không gồm bề dày hai tường. |
| M09 | 2.60m | mini-edge → fence-top | Mặt trên mép mini → đỉnh thanh ngang hàng rào. |
| M10 | ≈0.10m | fence-top → rail-inward | Đi vào phía cửa chính. |
| M11 | ≈0.10m | rail-inward → C | Đi xuống để vào hàng rào. |
| M12 | 4.00m | rail-start → rail-end | Chiều dài hàng rào sắt, chưa tính nhánh chậu. |
| M13 | 3.00m | AP → window-plane | Vuông góc từ AP ra mặt cửa sổ. |
| M14 | 1.70m | room-floor → AP | AP sát tường trái; cao so sàn phòng. |
| M15 | ≈0.10m | source-above-wall → S | Rẽ ngang về phía phải≈10cm để nằm trên mặt tường trước S; riêng vớiØ10cm và haioffset hàng rào. |

**M03 corrected:**8.90m is one straight reference span from S to the high outer-mini bend. It is not a source-to-edge aggregate and includes none of the source approach/rise/sideways seating. Source→S nominal chain≈0.99m is0.53+0.26+0.10+≈0.10m. The10cm pipe allowance is the existing diameter, separate from the new≈10cm sideways step and purchase buffer.

PA2 reference from S:7.40m forward,0.40m down,1.50m along mini,2.60m down. PA1 reference:8.90m forward,3.00m down. Both are11.90m S→fence. M09 always starts at the **top of the outer mini edge**, not the high route40cm above it. M06 excludes the divider and includes the outer wall; M07 includes both side walls; M08 is clear gap.

Reference points bind these measurements independently of the displayed tube centerline. The illustrated tube uses radius1cm and a5mm geometric gap, both **Agent estimates**, not selected materials or support specifications. The source rise stays outside the wall face; it turns sideways above the wall. At the first mini bend the centerline is6.5cm past the reference entry to clear the existing pipe end, then1.5cm outside the outer edge before dropping. The displayed sideways seating is11.5cm around the approximate10cm reference, keeping one continuous X alignment through the fence. These offsets preserve the original reference spans and produce a displayed centerline≈17.12m versus nominal17.09m. The rendered lower run is shorter than the1.50m reference by the corner clearances; it is not a second tape measurement.

Estimated absolute elevations: terrace floor3.53m, mini floor3.54m, mini-edge top3.74m, fence top1.14m. The right low-wall thickness≈20cm, mini side-wall≈10cm and divider≈12cm remain unmeasured. Renderer and collision checks consume the same solid wall bounds. Tube radius is included in conservative wall/existing-pipe clearance checks; these checks do not certify site mounting or bending radii.

## Selected route and source

**PA2 is selected:** source→approach face→rise outside the wall→sideways≈10cm onto the wall→S; then follow the mini contour and descend to the main fence. **PA1 stays view-only:** high straight route followed by3m drop, with no existing support bridge for the≈1.50m suspended span. Proposed clips/support markers do not depict installed supports. Older left/diagonal/right presets stay historical comparisons and are not redesigned.

N11–N13 reference the private correction images; FIELD-20261025-C2 records corrected endpoints/choice. The earlier source→mini aggregate interpretation and15.70m total are superseded, retained in the draft06 archive only.

Main source option: **replacement two-outlet tap**, ordinary-use outlet plus miGarden branch. The miGarden manual valve provides hard upstream isolation before a future electronic valve. The old single-outlet tap remains a reference. Source detail only needs to identify and avoid the overlapping existing assembly; it does not certify each old pipe's function, nominal thread, flow direction or pump state.

Nominal chain≈**17.09m**:0.99 source entry+11.90 S→fence+≈0.10 inward+≈0.10 down+4.00 fence. The displayed centerline≈17.12m includes illustrative geometric offsets. Both exclude pot branches, installation allowances and sales-roll sizing. Purchase **buffer** remains separate and editable; default3m is an Agent example, not an Owner purchase quantity. The UI currently calculates centerline+buffer≈20.12m before branches. No purchase is performed.

Supports remain schematic; no neighbor-wall drilling/fixing permission is implied. Zone A stays one shared circuit with dynamic pots and illustrative manual branch shutoffs. Zone B irrigation is deferred. Geometry does not prove pressure, flow, RF delivery, mounting strength or device operation.
