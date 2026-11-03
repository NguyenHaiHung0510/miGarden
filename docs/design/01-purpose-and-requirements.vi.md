# miGarden — Bước 1: Purpose and Requirements Specification

**Baseline công khai · 2026-11-03 · Asia/Saigon.** Đọc [chỉ mục và phạm vi chung](README.md). Đây là yêu cầu thiết kế, chưa phải kết quả thực nghiệm, SLA hoặc quyền mở nước thật.

## 1. Bài toán và mục đích

Người dùng trồng hoa/cây cảnh trong chậu tại ban công, thường vắng nhà vì học tập hoặc công việc. Việc tưới hiện còn phụ thuộc chủ vườn hoặc người hỗ trợ. Mong muốn là giảm thao tác trực tiếp và tích hợp quản lý trong website cá nhân microSched. Mô tả cách tưới hiện tại không đủ để suy ra lịch/lượng nước phù hợp cho cây.

**S1-P01 / Vấn đề:** duy trì việc tưới khi người chăm vườn không thường xuyên có mặt.

**S1-G01 / Mục đích:** hỗ trợ tưới có kiểm soát và tự chủ local khi điều kiện cho phép, giảm phụ thuộc người tưới trực tiếp, đồng thời cho biết thiết bị báo đã thực hiện gì.

Không cam kết cây luôn khỏe/đủ nước, hoạt động khi mất điện/nước hoặc có thể bỏ mặc vô hạn. Zone là đơn vị điều khiển/lịch/lịch sử; Area nhóm Zone. Ban đầu triển khai A, mô hình cho phép mở B/C/D trong capacity thực tế chưa xác minh của một ESP32. Không có nghiệp vụ quản lý chậu/placement hoặc danh sách chậu bị tác động.

## 2. Yêu cầu chức năng

| ID | Yêu cầu | Ví dụ chấp nhận cần kiểm tra |
|---|---|---|
| S1-FR01 | Tạo/xem/sửa/tạm dừng/bật lại lịch từng Zone qua microSched; có ngày hiệu lực, recurrence, thời lượng hữu hạn và trạng thái. Phân biệt desired đã lưu với last-known applied tại Controller; tối đa một lịch logic hoạt động local/Zone | Web sửa khi offline vẫn hiện chờ/chưa rõ; báo applied phải resolve đúng nội dung. Pause đã applied chỉ ngăn occurrence tương lai, không đổi phiên đang chạy |
| S1-FR02 | Thực thi lịch đã áp dụng hợp lệ tại local khi chỉ mất Internet, còn điện/nguồn nước phù hợp và time/config/policy/accounting/safety đáng tin | Ngắt mạng sau applied; một cơ hội tối đa một lần, không chạy bù; từ chối/bỏ qua nếu điều kiện không đạt |
| S1-FR03 | START/STOP vật lý tại nguồn không chờ cloud, cùng local arbiter/giới hạn như lịch và remote | START hữu hạn; STOP yêu cầu CLOSE không cần OPEN gates. Gesture, nút và manual duration chưa chọn; không thay van tay độc lập |
| S1-FR04 | Lịch sử cho biết nguồn, Zone/context thực dùng, planned và thời điểm/thời lượng output do Controller báo khi biết, outcome/reason | Planned 45 giây nhưng STOP sau 17 giây phải giữ 17 reported; thiếu start/terminal không tạo Session thành công giả |
| S1-FR05 | Giữ critical events/outcomes local khi offline, đồng bộ lịch sử sau reconnect; thể hiện last-sync/freshness/unknown | Cùng record gửi lại không nhân phiên/replay; cùng identity khác payload phải conflict. Storage hỏng thể hiện degraded/evidence gap, không hứa luôn ghi đủ |
| S1-FR06 | Tưới/dừng từ web theo quyền/đích/hiệu lực và preview/confirm đúng context; hiển thị các mức nhận/thực thi/kết quả | Đã gửi/received/authorized không là đã tưới. Remote context đổi có ý nghĩa phải confirm lại; timeout sau possible execution giữ unknown |
| S1-FR07 | Dashboard cho biết device/kết nối, desired/applied, phiên đang được báo, lần gần nhất và lần kế tiếp khi xác định được | Dữ liệu cũ/partial/unknown hiện rõ; next planned không là action thật. Không chế soil/flow hoặc metric chưa có |

## 3. Yêu cầu chất lượng

| ID | Yêu cầu | Giới hạn thiết kế |
|---|---|---|
| S1-NFR01 | Tin cậy: không lặp một cơ hội do retry/reconnect/reboot/đổi nội dung; phân biệt từ chối, hoàn tất được báo và unknown | Chưa có SLA/uptime hay tỷ lệ thành công thực nghiệm |
| S1-NFR02 | Tự chủ local có điều kiện: lịch/nút không cần cloud grant cho từng lần | Không hứa chạy khi mất điện/nước; thời gian tự chủ và dung lượng lịch sử chưa chốt |
| S1-NFR03 | An toàn: không mở vô hạn, không restore OPEN/chạy bù; mọi nguồn tuân thủ cumulative local limits, STOP và interlock | Numeric limits, clock/recovery, hardware feedback/cutoff phải được thiết kế và kiểm chứng |
| S1-NFR04 | Trung thực dữ liệu: planned/saved/applied/reported/physical khác nhau; giữ nguồn, chất lượng thời gian và freshness | Applied không bằng enabled/readiness; GPIO/thời lượng output không chứng minh van đóng hoặc nước tới cây |

## 4. Safety outcomes

Các hàng dưới là tổ chức traceability, không là exhaustive FMEA hoặc các ca đã chạy thành công.

| ID | Tình huống | Kết quả bắt buộc | Thiết kế/evidence còn cần |
|---|---|---|---|
| S1-SAFE01 | Mất Internet khi tưới | Local limits/deadline và CLOSE vẫn có hiệu lực, không chờ cloud để kết thúc an toàn | Timer/accounting, ngắt mạng, feedback/cutoff; không buộc dừng ngay chỉ vì mất mạng nếu phiên còn hợp lệ |
| S1-SAFE02 | microSched down | Không mất bảo vệ local; lịch đã applied/nút còn dùng khi local hợp lệ | Outage, hiệu lực cấu hình và lưu bền; remote có thể không khả dụng |
| S1-SAFE03 | Controller treo | Không cho nước mở vô hạn, đưa đầu ra/cấp nước về an toàn trong giới hạn sẽ chốt | Watchdog/output/fail-safe cùng driver/van; reset CPU không là bằng chứng ngắt nước |
| S1-SAFE04 | Mất nguồn/reboot | Mất nguồn an toàn, boot fail-closed; không restore OPEN hoặc replay cơ hội lỡ | Mạch/van/cutoff, drain-down và power-loss/recovery tests |
| S1-SAFE05 | Clock/config/policy/ledger không đáng tin | Không OPEN; reboot không reset budget; STOP/CLOSE không đòi OPEN gates | Timebase/source/tolerance/recovery, validity/persistence; boolean clock usable chưa là proof |
| S1-SAFE06 | Intent trễ/trùng/sai đích/sai quyền/hết hạn | Chỉ xét intent hợp lệ, đúng context; không lặp/replay do retry/reconnect | Identity/TTL/dedup và handoff cụ thể chưa chọn |
| S1-SAFE07 | Xung đột nguồn, START bận, vượt tích lũy | Chung local arbiter và all-source cumulative budget; STOP ưu tiên CLOSE và xử lý pending START cũ; START bận bị từ chối, không queue muộn | Priority còn lại, scope accounting, số giới hạn/cooldown và algorithm còn mở |
| S1-SAFE08 | Possible execution nhưng thiếu kết quả | Giữ unknown, chặn OPEN mọi nguồn; nguồn/accounting chung chưa chứng minh độc lập thì chặn mọi Zone. Không retry action; recovery/xác minh an toàn mới được xét lại | Fresh safe evidence, fault/recovery; TTL/cooldown hết không xóa unknown |
| S1-SAFE09 | Van/driver/áp/nguồn nước bất thường, tắc/rò/siphon hoặc thiếu feedback | Fail-safe/cutoff và feedback phù hợp; van khóa tay độc lập. CLOSE/timer/GPIO không chứng minh nước ngừng/đủ | Hardware/failure model, pressure/flow/residual/cutoff và numeric envelope trước enable |
| S1-SAFE10 | Critical storage/ledger đầy/hỏng, sync thiếu/trùng/cũ | Không START thiếu kiểm soát; lỗi giữa phiên cần safe stop/cutoff/degraded. Telemetry không lấn critical outcomes; sync idempotent, UI trung thực | Capacity/retention/recovery; có thể mất evidence, không hứa luôn lưu đủ |
| S1-SAFE11 | Sai Zone/mapping hoặc context đổi giữa intent và thực thi | Resolve đúng Zone/Controller/Actuator và nội dung hiệu lực; remote context đổi cần preview/confirm mới; lịch sử giữ context thực dùng, không lookup cấu hình hiện tại | Binding/cutover và xử lý đổi context khi đang chạy; không cần managed Pot/outlet/circuit entity |

## 5. Quy ước, ranh giới và điểm mở

- Giờ Hà Nội **UTC+7 cố định**, không timezone selector. Một lần cần ngày+giờ; daily/weekly cần ngày hiệu lực và quy tắc; interval cần anchor ngày+giờ và chu kỳ. Hiệu lực và điều kiện kết thúc rõ.
- Start/anchor/interval có độ chia cấu hình **1 phút**; duration là bội **15 giây**. Đây không là clock accuracy, min/max/default nước hoặc cho phép tưới mỗi phút bất chấp safety. Actual watering duration cần khảo sát/evidence.
- microSched chịu web/quyền/API/storage adapter; miGarden chịu local execution/safety và system contracts. Không chọn lại auth/provider/transport/schema ở bước này. Dữ liệu vườn là dữ liệu cá nhân. Chủ vườn là người quản lý chính; quyền người khác không tự mở rộng.
- Thu record theo sự kiện: applied, occurrence disposition, request, start/end/outcome/reason và status có freshness. Cadence/retention/latency/cost/tolerance còn mở. Soil/Zigbee HOLD; không thêm sensor vì ví dụ khóa học.
- Không có nghĩa vụ kế thừa notification lifecycle/Web Push, inventory cây/chậu/placement hoặc Area/Source/Controller CRUD. Status/error/freshness giữ nguyên. Tưới nhóm và multi-Controller không thuộc core hiện tại.

[SAF-P01](README.md#saf-p01--proposed-giới-hạn-tích-lũy-và-cooldown) ghi candidate cho bước 08; chưa phê duyệt numeric policy. Bước 07 phân công trách nhiệm, bước 09 xác minh hardware/flow/fault; 02/04 cập nhật khi policy được lựa chọn.

## 6. Kết quả và cách đọc tiếp

Có mục đích, bảy FR, bốn NFR và mười một safety outcomes để dẫn sang [năm UC ở 02](02-process-specification.vi.md), [domain ở 03](03-domain-model.vi.md), [thông tin ở 04](04-information-model.vi.md). Giữ nguyên các ID để truy vết. Baseline tài liệu không chứng minh completeness mọi sự cố hoặc runtime/device/water acceptance; các điểm mở phải được giải quyết trước implementation tương ứng và enable nước.

Phương pháp: IoT Design and Development, Chapter 4, bước Purpose and Requirements (slide 14–15). Nguồn phương pháp không tự sinh yêu cầu sản phẩm; [provenance](../provenance.md) giải thích nguồn và giới hạn.
