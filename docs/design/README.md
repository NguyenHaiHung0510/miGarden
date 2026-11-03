# miGarden — baseline thiết kế 01–04

**Ngày baseline: 2026-11-03 · Asia/Saigon.** Bốn bước đầu được chấp nhận cho baseline công khai: mục đích/yêu cầu, quy trình, domain và information model. Đây là thiết kế để đọc, thảo luận và phát triển tiếp; chưa là DB/API/firmware hoặc chứng nhận vận hành thiết bị/nước.

`docs/design/` là nguồn thiết kế hiện hành duy nhất được tiếp tục chỉnh sửa. Các snapshot làm việc trước khi công bố được giữ làm lịch sử riêng, không phải bản canonical song song. [Project state](../project-state.md) phân biệt thiết kế với prototype đã chạy.

## Đọc theo thứ tự

| Bước | Nội dung | Trạng thái |
|---|---|---|
| 01 | [Purpose and Requirements](01-purpose-and-requirements.vi.md) — bài toán, FR01–07, NFR01–04, SAFE01–11 | Baseline công khai |
| 02 | [Process Specification](02-process-specification.vi.md) — năm UC, LC01–03 và nhánh lỗi | Baseline công khai |
| 03 | [Domain Model](03-domain-model.vi.md) — tám concept, quan hệ và quy tắc | Baseline công khai |
| 04 | [Information Model](04-information-model.vi.md) — dữ liệu nguồn, ref bất biến, report và thời gian | Baseline công khai; giữ ý nghĩa bản 0.2 |
| 05 | Service Specifications | Pending — chưa thiết kế |
| 06 | IoT Level Specification | Pending — chưa thiết kế |
| 07 | Functional View Specification | Pending — chưa thiết kế |
| 08 | Operational View Specification | Pending — chưa thiết kế; nơi quyết định policy vận hành |
| 09 | Device and Component Integration | Pending — chưa thiết kế/kiểm chứng phần cứng |
| 10 | Application Development | Pending — chưa triển khai ứng dụng điều khiển |

[Bảy cảnh walkthrough offline](walkthrough/index.html) có dữ liệu synthetic, thẻ nguồn, SVG và PNG. Có thể mở trực tiếp file HTML sau khi clone; không cần server, tài khoản, Internet hoặc thư mục riêng. [Hướng dẫn và nguồn hình](walkthrough/README.vi.md) giải thích cách đọc và dựng lại dữ liệu. Đây là ví dụ về thiết kế, khác [viewer model-v0](../../viewer/README.md) đang biểu diễn hiện trường.

## Phạm vi chung và truy vết thay đổi

Zone là đơn vị điều khiển, lịch và lịch sử. Area nhóm Zone, mỗi Zone thuộc một Area. Một Controller ESP32 điều khiển nhiều Actuator trong khả năng phần cứng **chưa xác minh**; phương án khái niệm là một van nhánh/Zone. Mỗi Zone có 0..1 lịch logic đang hoạt động tại local. Rollout vật lý bắt đầu ở A; mô hình cho phép B/C/D trong giới hạn một board. Độc lập điều khiển từng Zone không chứng minh độc lập nguồn nước/accounting an toàn.

| Chủ đề đã đồng bộ | 01 / 02 | 03 / 04 |
|---|---|---|
| Zone thay mô hình quản lý chậu | SAFE11, LC03: đúng đích/context, preview/history theo Zone | DM01–04, DR01/07, binding bất biến; không Pot/placement/assignment hoặc outlet/circuit registry |
| Một Controller, lịch tối thiểu | FR01/02, UC01/02 | DM03/05, DR12; desired/applied khác nhau không là hai lịch hoạt động |
| Giữ lịch sử đúng nội dung | FR04/05, LC02/03, UC05 | IF01–07; content_ref/binding_ref; Session resolve qua START, không lookup latest config |
| Ba nguồn đi chung safety | SAFE01–11; LC01 STOP/pending; START bận bị từ chối | DR03–15; all-source cumulative budget, common-source unknown block, no-double/no-catch-up/no-replay |
| Giờ và độ chia cấu hình | Hà Nội UTC+7; start/interval 1 phút, duration 15 giây | once/daily/weekly/interval; độ chia không là clock accuracy hoặc water default |
| Loại bỏ nghĩa vụ kế thừa | Không tự thêm notification lifecycle, inventory/placement CRUD | Không Source/Area/Controller inventory CRUD, revision/application management hoặc GroupRun/Step |

Chậu/hoa và chín chậu trong khảo sát vẫn là sự thật hiện trường, không biến thành entity quản lý. Soil/Zigbee tiếp tục HOLD. Tưới nhóm là mở rộng tương lai bằng điều phối đường từng Zone, chưa là domain hay chức năng đã triển khai. Chỉ báo lỗi, freshness và unknown vẫn thuộc baseline.

## Điểm mở và policy vận hành

Các safety outcomes trong 01–04 đã được giữ; thuật toán, numeric envelope, exact board/pins/framework, van/driver/power/feedback/cutoff, clock/persistence/recovery, opportunity identity/cutover, remote handoff và transport vẫn cần thiết kế/evidence. Không suy lựa chọn từ các bước còn trống hoặc từ tài liệu cũ.

### SAF-P01 — PROPOSED: giới hạn tích lũy và cooldown

**Chỉ ghi nhận candidate để quyết định ở bước 08; chưa được chọn làm numeric policy, default tưới hoặc field-tested limit. Bước 08 chưa được triển khai trong baseline này.**

| Candidate | Giá trị đề xuất để đánh giá, chưa phê duyệt |
|---|---|
| MAX_CONTINUOUS | 45 phút |
| MAX_ROLLING | 45 phút trong cửa sổ rolling 60 phút |
| Cooldown sau manual STOP | 30 giây |
| Cooldown sau kết thúc bình thường | 15 giây |

Phương án cần đánh giá accounting chung xuyên schedule/local_button/remote và không reset do reboot; START bị từ chối với `COOLDOWN_ACTIVE` và remaining khi biết, STOP/CLOSE không bị chặn. Giữ remote identity/TTL và không replay pending START cũ sau STOP; recovery fail-closed, hết cooldown không xóa fault hoặc `OUTCOME_UNKNOWN`. Software limits không phải physical cutoff. Không thêm entity Cooldown.

**OPEN:** phạm vi per-Zone hay Controller, định nghĩa continuous, cơ sở cửa sổ rolling, xử lý phiên đang chạy khi chạm trần, clock/persistence và phục hồi sau reboot. Chưa có số nào trong bảng là ngưỡng active của 01–04 hoặc giá trị đã đo an toàn.

| Bước liên quan | Trách nhiệm khi policy được lựa chọn |
|---|---|
| 08 | Nhà quyết định chính: scope, window, cooldown, ceiling, clock/recovery và bằng chứng |
| 01 | Truy vết safety outcomes, không nhồi thuật toán |
| 07 | Phân công local arbiter/accounting/cutoff |
| 09 | Tích hợp phần cứng và xác minh flow/fault thực tế |
| 02 / 04 | Cập nhật behavior/reason/state/time sau khi policy được chọn |

## Nguồn và legacy

Phương pháp tổ chức theo mười bước IoT Design and Development, Chapter 4; tài liệu khóa học chỉ là phương pháp, không tự tạo yêu cầu cảm biến hoặc hạ tầng. Không phân phối slide gốc. Nội dung xuất phát từ nhu cầu vườn thật và các lựa chọn phạm vi được chấp nhận; [provenance](../provenance.md) ghi vai trò nguồn.

[Specification](../specification.md), [architecture](../architecture.md) và [design inputs](../design-inputs.md) được giữ tại đường dẫn cũ dưới nhãn **LEGACY — reference only**. Nhãn CHỐT/MUST/gate bên trong chúng là lịch sử của bản cũ, không là hợp đồng hiện hành. Muốn dùng lại một ý phải nêu nguồn, lý do, tác động và ghi lựa chọn trong bước thiết kế phù hợp. [Operations](../operations.md) giữ nguyên tắc bring-up/evidence, phân biệt lựa chọn cũ chưa được chọn lại.

Nguồn geometry hiện hành vẫn là `viewer/model.json`; không tạo nguồn geometry editable thứ hai. Site facts, prototype, licenses và [public-content policy](../representation.md#public-content-and-private-sources) giữ vai trò hiện có. Rollback baseline tài liệu bằng revert thay đổi tương ứng; việc đó không chứng minh rollback thiết bị hay cho phép mở nước.
