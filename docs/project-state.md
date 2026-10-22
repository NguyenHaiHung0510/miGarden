# Current project state

Snapshot dẫn xuất ngày **2026-10-22, Asia/Saigon**, cập nhật sau integration/fresh-root review MG-H1. Đây là product/handoff summary, không phải authority ledger hoặc grant mới. Provenance ở [source map](provenance.md); Owner machine giữ ledger/private context qua locator ngoài public tree.

## Những điều đã chốt

| Chủ đề | Current decision / implication |
|---|---|
| Product | Vườn thật, có đường nâng cấp; rubric không tự quyết protocol, sensor hay phần cứng. Course đang cần requirements/design; code là batch riêng. |
| Domain | `Pot` + placement/planting/assignment history động. Chín chậu là snapshot khảo sát, không là cardinality hoặc số valve cần mua. |
| Actuation | Một **Zone A shared circuit**; lệnh target zone/exact outlet, preview/receipt toàn bộ affected pots. Không hứa bỏ tưới một pot khi circuit chưa tách vật lý. Zone B/mini balcony deferred. |
| Control | Web “Tưới ngay”; cloud quản lý/version lịch. Controller thực thi local schedule khi Internet mất nhưng còn điện, trusted time/config/ledger và safety hợp lệ; không catch-up/double-run. |
| Local input | START/STOP tại nguồn/controller tầng thượng là field baseline. Nút ban công optional; DIP chỉ dry bench. Manual water shutoff vẫn độc lập. |
| Sensing | Soil/Zigbee, ZBDongle-P, vật tư/host dành riêng sensing **HOLD vô thời hạn**. Không tự mở lại sau R1 hoặc vì rubric; permanent edge host chưa chọn. |
| Hardware | P1A đã về, Owner report; chưa đụng/bench. Board dự kiến Vietduino ESP32 16MB HS2250V + USB-C HS1369, breadboard/jumpers/red LED/330Ω/DIP button. Không phải exact-unit verification. Water/field driver/valve/power/feedback chưa chọn. |
| Spatial | Dataset 23 ảnh + 1 video được duyệt sử dụng; 24 frames + 17 nhóm sheet v2. **A/SVG + B/Three.js** đã chọn, chung một source model có version/provenance/uncertainty; scene build chưa được cấp trong MG-H1. |
| C/COLMAP | Owner tự thử, không là dependency A+B; nhập finished output khi Owner cung cấp kèm scale/provenance. |
| Repository | MG-H1 tạo system root miGarden; microSched giữ web/API/DB adapter. Supersede điều cũ bắt mọi firmware/docs cùng repo microSched; giữ modular monolith/same-origin/Neon adapter boundaries. |
| Publication | Finished geometry/drawings/3D/COLMAP của site thật được phép public; raw capture/video và private transcript/ledger/secrets local. |
| Skills | **3/3 project skills đã cài tại root miGarden**: ESP32 development, Three.js fundamentals và interaction; [usage/status](skills.md), [notices](../THIRD_PARTY_NOTICES.md). **Mọi Agent** tự chọn khi phù hợp, không PM/T1-only hoặc explicit-only; đã quan sát một representative fresh-context task tự chọn đủ ba. `migrate-harness` optional/deferred, không chặn batch. |

## Grant hiện tại và trạng thái delivery

MG-H1 đã hoàn tất setup/selective migration/handoff, project skills và public repository qua PM. Canonical ledger mới nằm trong phần local của miGarden; nguồn cur_docs giữ lịch sử và redirect, không còn là nơi append quyết định mới. PM giữ receipts của root integration, effective pins, independent review, runtime proof, public GitHub release và cutover. Grant MG-H1 đã hoàn thành không tự cấp quyền cho chat tiếp theo làm product, flash, DB, network probe hoặc water.

| Plane | Evidence hiện có / còn thiếu |
|---|---|
| Documentation/data | Specs/research + immutable private snapshots; curated public derivative, local media/hash manifest. Writer checks không là independent acceptance. |
| Product code inventory | Cached surveyed microSched `develop` SHA `ddba8d367d575c91f8e355bb9a21fa408d6fea0c` chưa thấy committed miGarden implementation. Đây là dated source observation, không live branch/production claim. |
| Target-root/skills | 3/3 installed; fresh CLI đúng root/pin, catalog discovery và selection đủ ba được PM quan sát. Planning/handoff là scope đã chứng minh; renderer/toolchain/device workflow runtime vẫn **NOT_RUN**. |
| App/DB/firmware/device | Build/simulator/USB/flash/LED/button/clock/RF/water/production **NOT_RUN** trong batch này. |
| Release/cutover | Public repo `NguyenHaiHung0510/miGarden`, default `develop`, đã push và kiểm exact remote SHA/tree. Canonical cutover đã hoàn tất một lần sau sync, giữ source history. [Review report](reports/2026-oct-22-mg-h1.md) giữ kết quả theo từng stage; local locator/receipts giữ current canonical state. |

## Việc tiếp theo

Sau MG-H1, tiếp tục requirements/design: Owner verify anchors/endpoint/unknowns, khóa model-v0 outcome và dry-LED semantics thành packet riêng trước implementation. New Agent đọc local locator/continuity để lấy release/canonical state hiện hành; không tái thực thi grant migration đã hoàn thành.

Các quyết định còn mở: exact XYZ/scale/endpoints; transport và workload/cost caps; dry-profile separation với water flags; priority remote/manual/schedule, clock/persistence/recovery; exact toolchain/board identity/partition; water BOM, hydraulic/electrical evidence và numeric envelope; course-sensor disposition. Không tự điền số hoặc đổi HOLD thành requirement.
