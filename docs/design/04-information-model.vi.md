# miGarden — Bước 4: Information Model Specification

**Baseline công khai · 2026-11-03 · Asia/Saigon · ý nghĩa information model 0.2.** Đọc cùng [03](03-domain-model.vi.md) và [walkthrough W00–W06](walkthrough/index.html). Phương pháp: IoT Design and Development, Chapter 4, Information Model (slide 20–21). Đây chưa phải schema database, API hoặc firmware đã triển khai.

## 1. Hệ thống cần nhớ gì?

Một lịch logic có thể thay đổi nhiều lần. Cần biết bản web muốn dùng, bản Controller cuối cùng xác nhận và nội dung dùng cho từng lần thực thi. **Lịch sử tham chiếu đúng nội dung bất biến, không đọc cấu hình hiện tại.**

| Nội dung của cùng SCHED-DEMO-A | Cấu hình | Vai trò trong W06 |
|---|---|---|
| SC-A-v1 | enabled, daily 07:00, 45 giây | Occurrence lúc 07:00 đã dùng bản này |
| SC-A-v2 | enabled, daily 08:00, 60 giây | Bản sửa sau khi phiên bắt đầu |
| SC-A-v3 | paused, daily 08:00, 60 giây | Desired và last-known applied hiện tại |

Mẫu chọn một khóa nội dung duy nhất: content_ref, ví dụ SC-A-v1. Không lưu thêm số version độc lập; tên v1/v2/v3 giúp đọc, không là thuật toán cấp ID hoặc thứ tự nhận report. Các dòng nội dung thuộc Schedule/IF01, **không tạo domain/CRUD ScheduleRevision hoặc ScheduleApplication thứ chín**.

Occurrence trỏ SC-A-v1; START lịch trỏ occurrence; Session trỏ START. Vì vậy planned duration của phiên cũ vẫn 45 giây dù applied hiện tại là v3/60 giây đã pause. Runtime phải resolve rồi chốt duration/deadline bằng timebase elapsed local lúc bắt đầu; không hỏi latest Schedule/cloud mỗi tick. STOP/lỗi/cutoff có thể kết thúc sớm; không thêm chính sách tự clamp/rút duration.

## 2. Nguồn, phép suy ra và cách lưu

| Lớp | Nguồn authoritative | Ví dụ |
|---|---|---|
| Danh mục/quan hệ | Area, Zone, Controller, Actuator | Actuator giữ ref tới Zone/Controller; chiều ngược là query |
| Nội dung bất biến | Nội dung Schedule và binding target đã resolve của START | Giữ bản cũ khi còn tham chiếu/lịch sử cần dùng |
| Ý định/thực thi | Occurrence, ControlRequest, WateringSession | Cơ hội khác yêu cầu; có yêu cầu chưa chắc có phiên |
| Report/event | Nguồn báo, subject, thời gian/chất lượng, lần web nhận đầu | Report applied khác actual apply time; last-known khác realtime |
| Projection | JOIN/resolve từ nguồn trên | Zone/source/planned duration của phiên, dashboard, card trong hình |
| Runtime local | Duration/deadline đã bind, arbiter/accounting đang vận hành | Không phải bản JSON config lặp trong mỗi dòng history |

JSON chỉ là cách chứa mẫu, không bắt mỗi object thành bảng SQL hoặc cột JSONB. Có thể giữ nội dung hỗ trợ thành dòng phụ và JOIN bằng ref. Projection tự chứa để đọc hình không phải fact được chỉnh/lưu độc lập. projections, safety_view, ingest_view, timeline và JSONPointer thuộc trình bày/expected state của bộ mẫu.

Giữ nội dung/binding được tham chiếu, ngăn sửa/xóa gây mất lịch sử, kiểm ref không mồ côi và không tái dùng khóa cho nội dung khác. Digest trong metadata fixture chỉ kiểm tính toàn vẹn mẫu, chưa chọn cơ chế DB production. Thiếu nội dung lịch sử phải ghi thiếu/unknown, không lấy bản mới thế chỗ.

Lần chạy/bỏ qua/từ chối, kết quả/lý do/thời gian khi biết là business history. Debug từng tick hay mọi duplicate upload không mặc nhiên là core data phải lưu lâu dài. Nhãn “log” không quyết định có lưu DB hay không; mục đích/retention mới quyết định.

## 3. Từ điển tối thiểu của tám concept

### 3.1 Area, Zone, Controller, Actuator

| Concept | Thông tin | Nghĩa |
|---|---|---|
| Area | area_id, name | Nhóm vị trí |
| Zone | zone_id, name, area_id | Đích tưới chung, không danh sách chậu |
| Controller | controller_id, label | Một ESP32 hiện tại, không inventory/provisioning |
| Actuator | actuator_id, zone_id, controller_id | Một mapping authoritative; ref ngược suy ra |

Capacity nhiều van theo hardware chưa xác minh. Không thêm trường hằng hardware_family/kind chỉ để lặp hướng ESP32/van. Binding của START giữ mapping tại thực thi, không JOIN mapping hiện tại để viết lại quá khứ; đây là supporting value, không domain mới. Trong mẫu START đã resolve local dùng đúng binding này. Mapping thực khác là fact cần đối soát/chặn, không tự coi intended=actual hoặc sửa binding cũ.

### 3.2 WateringSchedule và thông tin hỗ trợ

| Thông tin | Nghĩa/nơi giữ |
|---|---|
| schedule_id, zone_id | Lịch logic và target ổn định; không sửa target của lịch sử |
| desired_content_ref, desired_saved_at_web | Nội dung web muốn dùng và lúc lưu |
| last_known_applied_report_ref | Report Controller xác nhận nội dung đã áp dụng; qua report lấy content_ref/provenance/freshness |
| desired_submission khi có | Quan sát đang chờ, từ chối hoặc chưa rõ đối với desired; không suy pending chỉ vì ref khác |
| Dòng nội dung hỗ trợ | content_ref, schedule_id, config; target suy từ lịch logic |

Config gồm enabled, effective_from, effective_until (nullable), duration_seconds hữu hạn và recurrence. Nội dung applied vẫn truy xuất được trước occurrence đầu tiên. Mất report chỉ là chưa xác nhận, không chứng minh đã/chưa áp dụng.

Web đọc đúng lịch logic của Zone, desired và report applied cuối được xác nhận theo provenance/thứ tự nguồn; không MAX(version) trên Zone hoặc lấy thứ tự upload. Report nguồn cũ đến trễ không kéo confirmation lùi lại. Thiếu thứ tự/chưa đối soát được thì last-known/unknown. Số thứ tự report trong fixture chỉ minh họa, chưa chọn transport/boot/cutover protocol.

**Applied ≠ enabled ≠ readiness ≠ physical state.** Pause vẫn có thể applied thành công. application_status là projection: confirmed khi có report xác nhận desired; pending/rejected cần quan sát tương ứng; thiếu thì unknown. Enabled suy từ nội dung last-known applied; bỏ cờ active_at_controller chỉnh độc lập. So hai ref bằng nhau không đủ kết luận hiện sẵn sàng/an toàn OPEN. Mỗi Zone vẫn tối đa một lịch local enabled; nhiều nội dung không phải nhiều lịch hoạt động.

| Recurrence | Thông tin | Nghĩa |
|---|---|---|
| once | at: ngày và giờ | Một cơ hội, không lặp sau reset/version |
| daily | at_local_time: HH:mm | Hằng ngày |
| weekly | HH:mm, days_of_week ISO 1–7, không trùng/rỗng | Các thứ đã chọn |
| interval | anchor_at, interval_minutes nguyên dương | Anchor+n×chu kỳ, n≥0; không chạy bù |

Giờ Hà Nội UTC+7 cố định, không selector. Độ chia cấu hình start/anchor/interval là 1 phút; duration cấu hình là bội 15 giây. Hai độ chia này không phải sai số clock/timer cho phép. [Bốn mẫu pattern](walkthrough/pattern-examples.json) là các phương án thay thế, không đồng thời active; chưa chọn cron/UI/library.

Cửa sổ hiệu lực là [effective_from, effective_until): đầu gồm, cuối loại trừ; null là chưa đặt mốc kết thúc lịch, không cho duration vô hạn. Occurrence tạo START phải đúng pattern/cửa sổ của nội dung đã dùng; START lịch không bắt đầu khi nội dung đó hết hiệu lực. Ngoài cửa sổ có thể ghi skipped/rejected không action. Phiên bắt đầu hợp lệ giữ duration đã bind; biên cuối không tự là STOP. Cutover/late admission chi tiết vẫn là dependency.

### 3.3 ScheduledOccurrence

| Thông tin | Nghĩa |
|---|---|
| occurrence_id, content_ref | Cơ hội và nội dung bất biến dùng đánh giá; lịch/Zone suy qua ref |
| planned_for | Dự kiến, không actual start |
| evaluation_outcome, reason | pending/start_requested/skipped/rejected/unknown; lý do khi biết |
| Event occurrence_evaluated | Mốc đánh giá/chất lượng khi có |

Outcome là nguồn disposition, không có cờ processed chỉnh độc lập. Pending chưa tiêu thụ; disposition khác phải giữ trong ledger chống lặp, kể cả skipped/rejected và unresolved unknown. Unknown không cấp quyền thử lại. Ledger phải nhận diện cùng cơ hội qua đổi version/reboot/retry; ID/fixture chưa thực thi thuật toán cutover đó.

Occurrence có thể không tạo START. start_requested chỉ nói đã tạo intent; arbiter có thể reject intent và không có Session. Skipped/rejected trước intent không có START/session. Một cơ hội tối đa một START/phiên; manual không tạo occurrence giả.

### 3.4 ControlRequest

| Thông tin | Nghĩa |
|---|---|
| request_id, kind, trigger_source | START/STOP; schedule/local_button/remote, không danh tính người |
| binding_ref | Target Zone và mapping Controller/Actuator đã resolve bất biến; Session không lặp bộ ref |
| occurrence_id cho START lịch | Nguồn planned duration/content; không copy config/duration cùng fact |
| duration_seconds cho START thủ công | Intent hữu hạn đã chốt; không kế thừa lịch active ngẫu nhiên |
| valid_until | Remote START cần hữu hạn; local null không cho queue/replay sau reboot |
| status, reason | requested/received/accepted/rejected/expired/outcome_unknown; accepted chưa là start |
| confirmed_context cho remote START | Binding/duration đã preview/confirm; web confirmation không cấp quyền local OPEN |
| Event request_created/received/authorized | Mốc tạo/nhận/giao thực thi và provenance, không rải clock-quality trên Request |

Target/source/duration của Request không đổi sau khi tạo. Remote context đổi có ý nghĩa cần preview/confirm mới, không sửa intent cũ. MicroSched vẫn xác thực/phân quyền; không cần actor_ref, registry người hoặc danh tính synthetic trong core history hiện tại.

STOP không cần duration/occurrence, không sinh Session và không qua OPEN gates; remote vẫn kiểm quyền/đích/hiệu lực. STOP ưu tiên CLOSE/ngăn pending START cũ tự mở sau STOP. Chưa tới Controller không chứng minh đã dừng. Expired trước tác động khác unknown sau possible execution: hết TTL không xóa unknown hoặc chứng minh chưa chạy.

### 3.5 WateringSession

| Thông tin | Nghĩa |
|---|---|
| session_id, request_id | Phiên đã bắt đầu, đúng một START cha |
| result, end_reason | running/completed_reported/stopped_reported/fault_reported/outcome_unknown; lý do khi biết |
| reported_output_duration_seconds | Actual reported fact: có thể 17 giây dù intent 45; null khi chưa kết luận |
| Event session_started/session_ended | Mốc bắt đầu/kết thúc được báo, chất lượng/lần nhận đầu |

Zone/Controller/Actuator/source/occurrence/planned duration resolve từ START/binding/nội dung lịch bất biến. Không lưu lặp config hoặc túi execution_context. Runtime vẫn bind duration/deadline tại start. Fact thực thi không suy được từ cha (mapping thực khác/uncertainty) cần ghi nhận/đối soát rõ ở execution design, không gán bằng current config.

Toàn bộ kết quả hiện tại là **Controller-reported**; bỏ evidence_level hằng trên từng phiên. Bộ này chưa đo nước; bỏ physical_water_evidence khỏi payload. Output hoàn tất/đóng không chứng minh van, lưu lượng hoặc cây đủ nước. Chưa biết giữ unknown/null, không lấy planned làm actual. Request reject trước start không có phiên; running chưa tới terminal không tự là unknown.

## 4. Report, thời gian và các góc nhìn

Report/event dùng chung envelope mẫu: record_id, origin_ref, event_kind, subject_kind/subject_ref, time (at/meaning/quality), received_at_web và payload riêng khi cần. Một subject chính; từ Session suy START/Occurrence/Zone thay vì nhân refs. Event không Session (nhận STOP/bỏ qua cơ hội) vẫn resolve target qua Request/Occurrence. Status report giữ output/active-session cuối được báo, không phải physical realtime.

- time.meaning=occurred_at_reported: mốc sự kiện theo nguồn báo; chưa biết civil time thì at=null/quality=unknown, không điền planned/giờ nhận thay thế.
- time.meaning=reported_at: lúc nguồn phát report status/applied, **không tự là actual apply time**. Mẫu không thêm exact apply time chưa có nhu cầu/evidence.
- received_at_web: lần web nhận đầu; null nếu mới ở local. Duplicate không đổi origin time/lần nhận đầu. Ingest-view W05 kể hai lần nhận, không bắt lưu mọi upload thành core history.

Cùng identity/payload dedup; cùng identity khác payload phải conflict, không overwrite. Sync không replay action. W05 sự kiện 07:00 nhận 07:10 vẫn là thực thi reported lúc 07:00.

**Civil time và elapsed time có trách nhiệm khác nhau.** Historical timestamp quality mô tả mốc trong report, không suy từ clock hiện tại. clock_usable_for_open trong status là trạng thái Controller báo tại mốc đó; true không chứng minh mọi OPEN gate đạt.

Runtime cần clock/config/target/policy/accounting đáng tin, common local arbiter, cumulative limits xuyên nguồn, STOP priority, no replay/catch-up. Unknown sau possible execution chặn OPEN mọi nguồn/Zone trên nguồn-accounting chung chưa chứng minh độc lập. Critical storage lỗi giữa phiên cần safe stop/cutoff/degraded, không hứa luôn ghi đủ. Boot không restore OPEN/xóa accounting; van tay độc lập vẫn cần.

**Tiêu chí định tính để đánh giá clock usable:** timebase đã được khởi tạo hoặc khôi phục từ nguồn được chấp nhận; không có reset, thay đổi giờ hay bất thường chưa được đối soát; và có căn cứ đánh giá sai số tích lũy, kể cả drift/thời gian offline, còn trong tolerance của tác vụ sẽ được chọn. Thiếu căn cứ ở bất kỳ điểm cần thiết nào thì chưa được coi là usable và không OPEN; STOP/CLOSE không phải chờ các điều kiện OPEN. Đây là hợp đồng để thiết kế và kiểm tra clock policy sau, không phải chứng nhận từ boolean trong fixture hoặc cam kết clock hiện tại đã đạt.

Trước firmware/field phải chốt nguồn đồng bộ/khôi phục sau reset, tolerance, drift/offline và điều kiện vô hiệu hóa clock; timebase elapsed/cutoff, opportunity identity/cutover, boot/order binding và numeric envelope. Không có ngưỡng số mới trong04. Web timestamp/report fresh không tự cho local OPEN.

Dashboard/card là projection có góc nhìn/mốc xem: last execution từ Session/report đủ evidence; next planned từ applied khi xác định được. Thiếu freshness/clock/unknown thì ghi thiếu/chưa rõ. W04 giữ report closed cũ nhưng current assessment=unknown sau possible execution. W02 local offline không giả web đã nhận realtime. Safety-view là expected design state, không kiểm chứng arbiter.

## 5. Bảy cảnh và ý nghĩa baseline

| Cảnh | Điều cần thấy |
|---|---|
| W00 | Vẫn tám concept; content/binding/report là supporting information |
| W01 | Desired SC-A-v2/08:00/60; last-known applied SC-A-v1/07:00/45, chưa occurrence |
| W02 | Một occurrence → START → Session; 45 giây output được báo |
| W03 | START local trực tiếp; 45 planned từ Request, 17 reported; không actor/occurrence giả |
| W04 | Receipt/authorized thiếu start/terminal; không fake Session/success; unknown vẫn chặn sau expiry |
| W05 | Event-time khác receipt-time; gửi lại record không nhân phiên/replay |
| W06 | Applied SC-A-v3 paused/60; phiên vẫn resolve SC-A-v1/45, STOP/safety độc lập |

Thiết kế dùng immutable refs thay snapshots lặp; không có actor/physical/evidence hằng hoặc active/processed chỉnh độc lập; timestamp/provenance ở event; Session giữ fact riêng, ID/source/planned là derived. Recurrence, một Controller/nhiều van chưa rõ capacity, 0..1 lịch active/Zone, Zone-only/group-future và safety giữ nguyên. 01–03 đã đồng bộ cùng ý nghĩa; không nhập domain cũ.

[Fixture](walkthrough/synthetic-data.json), [views](walkthrough/views.json), [kịch bản](walkthrough/scenarios.vi.md) và bảy hình dùng cùng dữ liệu mẫu. Ref/content/history/time bindings được kiểm tra ở mức fixture; SVG/PNG là output minh họa được giữ từ cùng phiên bản nội dung. Các kiểm tra này không là scheduler, simulator, independent acceptance hoặc runtime/device/water evidence.

[08](08-operational-view-specification.vi.md#budget-d186) ghi các ứng viên hạn mức chưa chọn. [09](09-device-and-component-integration.vi.md#integration-contracts) áp dụng nghĩa dữ liệu này khi nối firmware với microSched. Schema/migration, API, transport, thuật toán scheduler/cutover và ngưỡng an toàn còn mở. Nội dung 04 là thiết kế thông tin, chưa có bằng chứng chạy dịch vụ hoặc thiết bị.
