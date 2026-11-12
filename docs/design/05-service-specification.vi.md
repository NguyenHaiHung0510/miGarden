# miGarden — Bước 05: Đặc tả dịch vụ

**Thiết kế dịch vụ.** Tám năng lực logic bao phủ quản lý lịch, chạy tưới và dừng, đồng bộ kết quả, trạng thái và lịch sử. Đọc mục 2 để hiểu các dịch vụ, mục 4–5 để thấy chúng phối hợp; mục 3 là hợp đồng chi tiết để triển khai. Các endpoint, tần suất và số giới hạn chưa chọn không phải mặc định.

## 1. Bước 05 xác định điều gì?

“Từ các quy trình đã chọn, hệ thống cần cung cấp những thao tác có hợp đồng rõ ràng nào?”

02 mô tả câu chuyện actor/trigger/luồng/nhánh lỗi.03 xác định các khái niệm.04 giải thích thông tin.05 xác định input/output, điều kiện trước/sau, trigger, tác động và phản hồi của thao tác. Theo Chapter4 slide22, các mục đó quan trọng hơn việc đặt tên URL hoặc chọn framework.

Service ở đây là năng lực logic. Một service không bắt buộc một microservice, process, server hay endpoint riêng. Các thao tác có thể nằm trong cùng firmware hoặc ứng dụng; phân bố và nhóm chức năng được xem ở06–07. Cùng một thao tác có thể được nhiều UC dùng, một UC có thể đi qua nhiều service.

## 2. Tám dịch vụ và lý do phân rã

| ID | Dịch vụ logic | Vì sao cần tách / truy vết |
|---|---|---|
| S5-SV01 | Quản lý nội dung lịch mong muốn | FR01/UC01: tạo, đọc, sửa, pause/enable; lưu desired không đợi Controller online |
| S5-SV02 | Áp dụng và xác nhận nội dung lịch tại Controller | FR01/02/UC01: saved khác applied, thất bại/chưa rõ áp dụng không thành success giả |
| S5-SV03 | Đánh giá một cơ hội lịch đến hạn | FR02/UC02: occurrence có thể bị bỏ qua trước khi sinh START; giữ no-double/no-catch-up |
| S5-SV04 | Tiếp nhận và thực thi START có giới hạn | FR02/03/06, UC02–04: ba nguồn dùng cùng cửa kiểm tra/thực thi, không ba đường mở nước khác nhau |
| S5-SV05 | Tiếp nhận STOP và yêu cầu đóng | FR03/06, UC02–04/LC01: không đòi OPEN gates hay Session đã biết; xử lý pending START cũ |
| S5-SV06 | Đồng bộ và tiếp nhận reports/history | FR04/05, UC01–05: event cũ có thể đến trễ/trùng; sync không thực thi action |
| S5-SV07 | Tra cứu trạng thái hiện biết | FR07/UC05: dashboard desired/applied/output/session/connectivity kèm freshness/unknown |
| S5-SV08 | Tra cứu lịch sử theo Zone | FR04/UC05: phân biệt dự kiến, bỏ qua/từ chối và thực thi được báo; không quản lý từng chậu |

SV01 và02 có điều kiện/kết quả khác nhau, nên không dùng một dấu “lưu thành công” cho cả hai. SV03 có thể kết thúc không có Request; SV04 còn nhận web/nút không có Occurrence. SV05 cần hoạt động khi các điều kiện START không đạt. SV06 ghi nhận thông tin, không có quyền mở nước. SV07 và08 là hai nhu cầu đọc khác nhau; có thể triển khai chung module hoặc API family sau này.

Tám service không xuất phát từ việc domain cũng có tám concept. Số lượng có thể điều chỉnh nếu hợp đồng trùng/nghĩa thiếu; không tạo domain mới để khớp service.

## 3. Hợp đồng chi tiết

### Thời điểm phục vụ (schedule của service)

Trong slide22, `schedule` là thời điểm/tần suất một service được gọi; không phải service nào cũng có một WateringSchedule hoặc cần job định kỳ. Chưa chốt tần suất polling, heartbeat hay retry số ở bước này.

| Service | Thời điểm/tần suất theo hợp đồng |
|---|---|
| SV01 | Theo thao tác tạo/đọc/sửa/pause/enable của người dùng; không có chu kỳ tự đổi lịch |
| SV02 | Khi có desired cần áp dụng/đối soát và kết nối cho phép; nhịp retry/backoff cụ thể chưa chọn |
| SV03 | Với từng occurrence đến hạn theo nội dung applied hợp lệ; không tưới bù cơ hội đã lỡ và không tạo thêm action do đánh giá lặp |
| SV04 | Khi có START từ một trong ba nguồn; phiên hợp lệ có deadline local, không để intent bận chờ đến lần sau |
| SV05 | Khi có STOP hợp lệ, ưu tiên xử lý đóng; không đợi lịch đồng bộ hoặc điều kiện OPEN |
| SV06 | Khi có report/kết nối lại hoặc giao lại record cần đối soát; chưa chọn chu kỳ upload |
| SV07 | Khi đọc/refresh trạng thái, gồm cập nhật tự động khi đang xem; chưa chọn cơ chế hoặc chu kỳ làm tươi giao diện |
| SV08 | Lấy khi mở/lọc lịch sử và tự cập nhật khi đang xem; không cần tác vụ tưới hay kết nối Controller cho từng truy vấn |

**Phân biệt hành vi và nhịp cụ thể:** 05 xác định lúc nào cần phục vụ và ý nghĩa cập nhật; giá trị số không bắt buộc phải được chọn hết ở bước này. 06–07 phân bố/nhóm chức năng và trách nhiệm giao tiếp/retry/UI. 08 chốt vận hành: điều kiện retry, nhịp/backoff, độ trễ cập nhật chấp nhận được và giới hạn tải; 09–10 cụ thể hóa timer, protocol/API và ứng dụng theo trách nhiệm. Các bước sau có thể hồi quy hợp đồng trước nếu phát hiện ràng buộc mới.

| Mục chưa chọn | Điều đã cần rõ ở05 | Quyết định còn để sau |
|---|---|---|
| SV02 retry/đối soát lịch | Giao lại/đối soát cùng nội dung đúng identity; không applied giả, version lùi, action lặp hoặc ghi đè context mới. Đây không phải chính sách retry START | Điều kiện retry được phép, nhịp/cap/backoff và xử lý lỗi/ngắt kết nối cụ thể ở08; cơ chế giao tiếp theo06–09 |
| SV06 upload/report | Sync nền độc lập việc mở web; nhận/receipt/dedup đúng record, không replay hành động | Gửi theo sự kiện hay gộp định kỳ, mức ưu tiên/độ trễ, retry và retention cụ thể ở08–09; chưa hứa một chu kỳ hay độ trễ số |
| SV07 cập nhật dashboard | Giao diện thể hiện dữ liệu mới nhận khi đang xem, kèm tuổi/nguồn và stale/unknown | Chu kỳ hay push/event, độ trễ làm tươi và hành vi khi giao diện ẩn/đóng; trách nhiệm ở07, policy ở08, triển khai microSched ở10 |
| SV08 cập nhật lịch sử đang mở | Lấy ban đầu khi mở hoặc đổi lọc; sau đó tự cập nhật kết quả liên quan bộ lọc đang xem khi có dữ liệu mới đến. Live-update này là nhu cầu của giao diện | Cơ chế polling/push, chu kỳ/độ trễ và vòng đời cập nhật; không suy cần job lịch sử liên tục khi đã đóng màn hình |

Nhịp upload từ Controller và nhịp làm tươi giao diện là hai đường khác nhau. UI refresh không làm report tại nguồn mới hơn; vẫn phải thể hiện freshness và phần chưa nhận. Live ở đây là tự cập nhật dữ liệu được nhận trong lúc xem, chưa là cam kết tức thời hoặc bằng chứng vật lý mới.

### S5-SV01 — Quản lý nội dung lịch mong muốn

- **Trigger/bên dùng:** Chủ vườn tạo, đọc, sửa hoặc pause/enable lịch qua microSched.
- **Input:** Zone/lịch logic, nội dung lịch theo04 (recurrence, hiệu lực, enabled, duration), ref nội dung được dùng khi sửa và quyền thao tác hiện có.
- **Preconditions:** Đúng quyền/target; dữ liệu và hiệu lực hợp lệ; thay đổi không ghi đè mù bản mới hơn. Controller online không là điều kiện để lưu desired.
- **Output:** Nội dung desired được lưu cùng ref/thời điểm; hoặc lỗi validation/xung đột có lý do. Đọc lịch trả nội dung mong muốn và thông tin last-known applied khi có; không đổi hai nghĩa đó thành một boolean.
- **Tác động:** Khi nội dung lịch thay đổi, ghi một bản nội dung mới với content_ref mới và cập nhật desired ref; giữ nguyên nội dung các ref cũ để lịch sử vẫn resolve đúng bản đã dùng. Đề nghị đồng bộ sang local. Không OPEN, không khẳng định đã applied, không đổi Session đang chạy.
- **Nhánh/lưu ý:** Pause cũng là nội dung cần áp dụng; nếu mất mạng sau lưu thì hiển thị chờ/chưa xác nhận. Không thêm delete/clone/import lịch nếu chưa có nhu cầu.

**Bất biến ở mức hợp đồng:** Cùng content_ref luôn phải resolve cùng nội dung; sửa lịch tạo bản nội dung mới, không sửa bản cũ đã được tham chiếu. Trong phạm vi lịch sử còn giữ, các ref cần thiết vẫn phải resolve đúng; không biến thành nghĩa vụ lưu mọi thứ vô hạn. Ví dụ SC-A-v1 vẫn là07:00/45s sau khi desired đổi sang SC-A-v2/08:00/60s. Đây là quy tắc đã có ở04 được diễn đạt rõ tại thao tác ghi. 05 chưa chọn công nghệ bảo đảm; 07 phân công persistence/validation,08 chốt storage/retention,09–10 thiết kế cơ chế cụ thể theo thành phần (giao dịch, constraints/quyền ghi hoặc phương án tương đương). Phương án sau phải chứng minh quy tắc cả khi lỗi hoặc ghi đồng thời; không thêm domain revision/application hay mặc định một DB sản phẩm.

### S5-SV02 — Áp dụng và xác nhận lịch

- **Trigger/bên dùng:** Có nội dung desired cần gửi/đối soát khi kết nối cho phép; Controller nhận hoặc báo trạng thái nội dung đang giữ.
- **Input:** Lịch logic, content_ref và nội dung tương ứng, target/context, identity/thứ tự/hiệu lực và bằng chứng nguồn hợp lệ theo contract sẽ hoàn thiện.
- **Preconditions:** Có thể xác minh đúng nguồn/target/nội dung; local lưu và chuyển cấu hình an toàn, giữ tối đa một lịch hoạt động/Zone và trạng thái cơ hội đã xử lý. Không yêu cầu phải có một phiên tưới.
- **Output:** Report xác nhận nội dung áp dụng hoặc từ chối/lỗi/chưa rõ, có provenance. Web chỉ cập nhật last-known applied theo xác nhận phù hợp; report cũ đến muộn không kéo trạng thái lùi.
- **Tác động:** Lưu nội dung local cho các occurrence tương lai, phát feedback/report. Không tự tưới bù, không restart hoặc đổi duration của Session đang chạy.
- **Nhánh/lưu ý:** Mất receipt không chứng minh đã/chưa áp dụng. Lưu local lỗi không dùng bản dở; bản cũ chỉ còn dùng nếu chứng minh hợp lệ. Cutover/boot ordering cụ thể là dependency, chưa chọn protocol ở05.

### S5-SV03 — Đánh giá occurrence đến hạn

- **Trigger/bên dùng:** Scheduler local phát hiện một cơ hội đến hạn theo nội dung đã applied và thời gian đủ tin cậy.
- **Input:** Tham chiếu nội dung lịch bất biến, planned_for, target/binding có thể resolve, ledger/cơ hội đã xử lý và trạng thái cần để đánh giá.
- **Preconditions:** Có căn cứ xác định cơ hội/nội dung. Nếu các điều kiện đó không đạt thì không mở; ghi reason/disposition khi có thể, không tạo cơ hội thay thế để thử lại.
- **Output:** Occurrence disposition (pending/start_requested/skipped/rejected/unknown theo04), reason; nếu đã sinh ý định START thì có Request để SV04 xử lý.
- **Tác động:** Giữ trạng thái tiêu thụ cơ hội/no-double; có thể gọi SV04, không tự bypass local arbiter. Lịch local không cần cloud grant từng occurrence.
- **Nhánh/lưu ý:** Bỏ qua trước khi tạo intent khác với Request đã tạo nhưng bị arbiter reject. Đổi nội dung/reconnect/reboot không tạo thêm lần tưới cùng cơ hội; không catch-up.

### S5-SV04 — START có giới hạn

- **Trigger/bên dùng:** Occurrence tạo START; nút vật lý tạo START; chủ vườn gửi remote START đã confirm hợp lệ.
- **Input:** Request identity, nguồn schedule/local_button/remote, target/binding; occurrence/content ref cho lịch hoặc duration đã chốt cho manual; remote có context confirmation và TTL/hiệu lực cần thiết.
- **Preconditions:** Intent đúng quyền/nguồn/target, chưa bị STOP làm mất hiệu lực, chưa hết hiệu lực; context phù hợp; local time/config/policy/accounting/critical storage/target đáng tin; không đang bận; giới hạn tích lũy/interlock đạt. Unresolved unknown sau possible execution chặn OPEN mọi nguồn và mọi Zone cùng nguồn/accounting chưa chứng minh độc lập, không chỉ Zone của Request trước. Điều kiện local được kiểm tại thực thi, không suy từ web saved/report boolean.
- **Nhánh remote trước tác động:** Người dùng được preview Zone/binding/duration và context, rồi explicit confirm. microSched kiểm lại quyền/version/hiệu lực và điều kiện remote OPEN, gồm fresh safe-confirmed cùng boot và không active/unknown theo UC04. Controller nhận/lưu candidate và báo received; microSched xác minh/giao quyền thực thi theo contract trước tác động. Controller vẫn kiểm context đã confirm và admission local; mismatch cần preview/confirm mới, không sửa intent cũ. Cách handoff và ngưỡng freshness chưa chọn; lịch/nút không bị buộc đi qua cloud grant này.
- **Output ngay:** Kết quả nhận/xử lý Request với reason khi từ chối/hết hiệu lực/xung đột/chưa rõ. Received, được giao quyền/authorized, accepted và started không được đồng nhất; chỉ báo mức đã có bằng chứng.
- **Phản hồi sau:** Chỉ có Session khi có bằng chứng execution-start. Có start/end/reported duration/outcome/reason khi biết; chưa rõ không được báo success hoặc tạo Session hoàn tất giả.
- **Tác động:** Ghi nhận intent/accounting bắt buộc, chốt duration/deadline local rồi mới tác động theo admission hợp lệ; giám sát và đóng khi deadline/STOP/lỗi. Các đầu ra/bằng chứng vẫn ở mức Controller-reported.
- **Feedback tại nguồn, đợt một:** Owner hoãn đèn/chỉ báo chuyên dụng. Dùng quan sát nước và tiếng bơm tại chỗ, xem report trên microSched khi cần. Chưa có báo local riêng để phân biệt chấp nhận/từ chối/đang xử lý; đây là giới hạn đã chọn cho đợt đầu, không gán thao tác bấm thành đã tưới.
- **An toàn qua failure/boot:** Mất mạng không làm mất local deadline/limits. Treo hoặc mất nguồn phải có đường fail-safe/cutoff phù hợp; boot fail-closed, không restore OPEN, không xóa cumulative accounting hoặc chạy bù. Chưa đủ căn cứ phục hồi state cần thiết thì chặn OPEN. Cơ chế watchdog/mạch/van/timebase/persistence và bằng chứng kiểm chứng để các bước sau, chưa tuyên bố thiết bị đã đạt.
- **Nhánh/lưu ý:** Cùng identity không tạo action mới do retry; identity mâu thuẫn payload phải đối soát. Bận thì reject, không queue muộn. Possible execution mất kết quả giữ unknown/chặn theo phạm vi chung nêu trên, không retry như một intent mới; hết TTL hoặc cooldown không tự xóa unknown/lỗi.
- **Điểm chưa chọn:** Numeric budget/cooldown, thứ tự arbitration chi tiết, clock/reboot recovery, cơ chế remote handoff, timer/driver/cutoff và giới hạn vật lý. Không đưa SAF-P01 candidate thành điều kiện đang active.

### S5-SV05 — STOP và yêu cầu đóng

- **Trigger/bên dùng:** STOP từ nút vật lý hoặc remote hợp lệ. Đóng do deadline/fault là trách nhiệm an toàn của đường thực thi, không giả là thao tác người dùng hoặc tạo trigger_source thứ tư.
- **Input:** Target/phạm vi điều khiển, intent identity/nguồn; remote có quyền/hiệu lực/context cần thiết. Không cần duration tưới hoặc Session đã biết.
- **Preconditions:** Resolve đúng đích; remote vẫn được xác minh. Không đòi các OPEN gates, budget còn lại hoặc cloud online cho STOP local.
- **Output:** Kết quả tiếp nhận/yêu cầu CLOSE và phản hồi liên quan khi quan sát được; nếu chưa tới Controller thì chưa xác nhận dừng, nếu chưa biết vật lý thì không báo nước đã ngừng.
- **Tác động:** Ưu tiên CLOSE và xử lý START cũ trong phạm vi điều khiển; ghi lý do dừng khi biết. Không sinh Session mới. Phải phân biệt:
  - Pending remote START chứng minh chưa được giao quyền: microSched hủy/từ chối hoặc giữ chặn, không grant intent cũ sau STOP. Chỉ báo đã hủy khi có bằng chứng đủ.
  - START đã/có thể được giao quyền hoặc có thể đang tác động: không coi yêu cầu STOP là hủy thành công; yêu cầu CLOSE/đối soát và giữ unknown nếu chưa xác minh. Không retry action để thử.
  - Khi nhận STOP local hoặc remote, Controller ngăn pending START cũ tự mở lại. STOP local lúc offline vẫn giữ ngăn/uncertainty cần thiết và đối soát sau reconnect, không chờ cloud biết STOP mới bảo vệ. Remote STOP chưa tới thiết bị không chứng minh thiết bị đã xử lý các intent đó.
- **Feedback tại nguồn, đợt một:** Hoãn chỉ báo chuyên dụng cùng START. Người tại chỗ quan sát nước/tiếng bơm; trạng thái nhận, dừng, chưa rõ hoặc lỗi ghi qua report. Nhận intent hoặc phát CLOSE chưa là bằng chứng nước đã ngừng. Không lấy âm thanh bơm làm dữ liệu tự động cho firmware.
- **Nhánh/lưu ý:** Không tự pause/xóa các occurrence tương lai chưa trở thành intent cũ. Cần ngừng lịch dùng SV01+02. STOP điện tử không thay van tay; cooldown/fault/unknown không được biến thành điều kiện cấm yêu cầu dừng.

### S5-SV06 — Đồng bộ reports và history

- **Trigger/bên dùng:** Có báo cáo cần giao, có kết nối hoặc nhận/gửi lại report. Đồng bộ nền không cần người dùng đăng nhập hoặc mở dashboard. Không chốt polling/heartbeat/cadence cụ thể.
- **Input:** Origin event/report identity, subject/ref, nội dung bất biến, time/quality; web giữ received_at riêng.
- **Preconditions:** Nguồn/subject được xác minh, payload có nghĩa và có thể ghi nhận đúng identity. Historical time chưa biết có thể được lưu đúng nhãn, không bắt phải bịa giờ hoặc thay bằng receive time.
- **Output:** Ghi nhận mới/duplicate hợp lệ/conflict hoặc lỗi chưa nhận được; bên nhận trả receipt phù hợp record đã validate/lưu/đối chiếu theo contract. Report đã nhận cập nhật các góc nhìn khi đủ căn cứ; không làm mỗi upload thành Session mới.
- **Tác động:** Cất thông tin history/status và đối soát; retry record idempotent. Không phát START/restore OPEN, không xóa unknown chỉ vì reconnect.
- **Nhánh/lưu ý:** Controller chỉ xử lý retention local theo policy sau receipt phù hợp; gửi thành công hoặc timeout không tự chứng minh web đã nhận/lưu. Điều này không cho xóa state/accounting còn phục vụ safety. Critical local intent/outcome/accounting là một phần bắt buộc của execution safety, không phải logging tùy chọn chờ mạng. Lỗi storage giữa phiên cần phản ứng an toàn đã yêu cầu; dung lượng/retention/ưu tiên queue cụ thể để vận hành sau.

### S5-SV07 — Đọc trạng thái hiện biết

- **Trigger/bên dùng:** Người dùng đọc dashboard/trạng thái hoặc giao diện đang xem được làm tươi tự động. Cơ chế/chu kỳ chưa chọn.
- **Input:** Zone hoặc Controller trong phạm vi quyền hiện có.
- **Preconditions:** Có quyền đọc và nguồn dữ liệu có thể truy xuất; Controller online không là điều kiện để xem last-known data.
- **Output:** Kết nối/last-seen, desired/last-known applied, output/Session được báo, lần thực thi gần nhất/lần dự kiến khi xác định được; thời điểm/góc nhìn/freshness/unknown. Không sinh telemetry soil/flow/battery hoặc metric chưa có yêu cầu.
- **Tác động:** Đọc/projection, không điều khiển hay tự tưới để kiểm tra.
- **Nhánh/lưu ý:** Last-known closed cũ khác current assessment unknown sau possible execution. Offline local events chưa sync không được hiển thị như web đã biết realtime.

### S5-SV08 — Đọc history theo Zone

- **Trigger/bên dùng:** Chủ vườn mở/lọc lịch sử theo Zone, khoảng ngày, nguồn hoặc kết quả; khi còn đang xem, tự cập nhật các record mới hoặc thay đổi kết quả liên quan. Cơ chế polling/push và nhịp/độ trễ chưa chọn; chưa biến service đọc thành job nền luôn chạy khi đóng màn hình.
- **Input:** Điều kiện lọc hiện có theoUC05, không pot_id hoặc user-actor tracking mới.
- **Preconditions:** Có quyền đọc và dữ liệu truy xuất được; không đòi Controller đang online.
- **Output:** Các lần tưới và disposition/request liên quan khi cần giải thích vì sao không tưới, với planned/reported/outcome/reason/time-quality và ref lịch sử. Request bị từ chối/Occurrence bỏ qua không được biến thành Session đã chạy.
- **Tác động:** Đọc/projection từ bản lịch/binding bất biến; không sửa history theo config mới.
- **Nhánh/lưu ý:** Live-update giữ ý nghĩa bộ lọc đang xem; chỉ phản ánh record đã nhận, không giả Controller online hoặc nhận đủ lịch sử. Report thiếu/đến muộn/out-of-order có thể làm history partial/unknown, cần hiển thị đúng; không suy duration từ planned khi actual chưa biết. Retention/phân trang/query shape để data/service integration sau, không hứa lưu vô hạn.

## 4. Dịch vụ phối hợp trong năm UC

| UC hiện hành | Service tham gia chính |
|---|---|
| UC01 quản lý/applied lịch | SV01 → SV02; SV06 chuyển report; SV07 cho biết trạng thái |
| UC02 lịch local | SV03 → SV04; SV06 đồng bộ, SV07/08 đọc khi có dữ liệu |
| UC03 nút vật lý | SV04 hoặc SV05; SV06/08 ghi nhận/đọc, không cần vòng cloud để tác động local |
| UC04 remote | SV04 hoặc SV05 sau confirmation/quyền/context; SV06/07/08 hiển thị report |
| UC05 dashboard/history/sync | SV06, SV07, SV08 |

Các mũi tên là hợp tác logic. [06](06-iot-level-specification.vi.md) chỉ ra nơi chạy; [07](07-functional-view-specification.vi.md) cho thấy ai xử lý từng phần và các tình huống thay thế. Một module có thể thực hiện nhiều thao tác mà vẫn giữ các hợp đồng riêng.

## 5. Ví dụ để hiểu hợp đồng, không phải chọn duration thật

Người dùng đổi lịch từ07:00/45s sang08:00/60s khi Controller offline. SV01 có thể lưu desired mới, còn SV02 chưa có xác nhận apply nên SV07 phải giữ last-known applied cũ/chờ. Nếu bản cũ còn hợp lệ, scheduler local dùng nội dung đó đánh giá occurrence quaSV03, START điSV04 và Session giữ duration đã bind. Khi kết nối lại, SV06 chuyển reports theo thời gian sự kiện; nhận trễ không biến thành “vừa tưới”.

Nếu sau đó bấm STOP ởgiây17, SV05 không phải chờ cloud hoặc quaOPEN gates. Khi có terminal report, SV08 thấy planned45/report17. Pause lịch quaSV01+02 là tác động tương lai, không phải STOP phiên đang chạy.

Các con số trên là mẫu kế thừa W01/W03/W06, không thời lượng tưới/mức an toàn được chọn.


Các hợp đồng kế thừa [01–04](README.md), theo *IoT Design and Development*, Chapter 4, slide 22. Xem [08](08-operational-view-specification.vi.md) cho quyết định vận hành và [09](09-device-and-component-integration.vi.md#integration-contracts) cho điểm nối phần cứng. Đây là thiết kế, chưa có kết quả chạy dịch vụ.
