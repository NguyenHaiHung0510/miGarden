# miGarden — walkthrough domain và information model

**Baseline công khai · 2026-11-03 · SYNTHETIC.** Mở [index.html](index.html) trực tiếp qua file browser, đi W00 → W06. HTML/CSS và bảy hình tự chứa/local; không cần Internet, server, tài khoản hoặc cài dependency. Nếu trình xem trên website chỉ hiển thị source HTML, tải/clone repo rồi mở file offline. Các liên kết JSON/Markdown dùng để đọc nguồn; toàn bộ card đã có sẵn trong HTML, không fetch JSON lúc mở.

Đọc cùng [03](../03-domain-model.vi.md), [04](../04-information-model.vi.md), [fixture](synthetic-data.json), [views](views.json), [kịch bản](scenarios.vi.md) và [bốn pattern thay thế](pattern-examples.json). Các cảnh độc lập, không ghép thành một lần chạy. Card là projection có JSONPointer tới nguồn, không là fact chỉnh/lưu độc lập. Source/Zone/planned duration của Session resolve qua START và nội dung/binding bất biến; kết quả chỉ Controller-reported.

| Case | Ý chính | Ảnh | Vector |
|---|---|---|---|
| W00 | Tám concept, một Controller nhiều Actuator theo capacity chưa biết | [PNG](figures/W00.png) | [SVG](figures/W00.svg) |
| W01 | Desired v2/08:00/60 khác applied v1/07:00/45, chưa occurrence | [PNG](figures/W01.png) | [SVG](figures/W01.svg) |
| W02 | Local occurrence → START → Session; web offline chưa nhận | [PNG](figures/W02.png) | [SVG](figures/W02.svg) |
| W03 | START 45 planned, STOP 17 reported; không actor/occurrence giả | [PNG](figures/W03.png) | [SVG](figures/W03.svg) |
| W04 | Remote unknown; report closed cũ không chứng minh hiện closed | [PNG](figures/W04.png) | [SVG](figures/W04.svg) |
| W05 | Event-time khác receipt-time; duplicate không nhân phiên/replay | [PNG](figures/W05.png) | [SVG](figures/W05.svg) |
| W06 | Applied v3 paused/60; phiên cũ vẫn bind v1/45, STOP/safety độc lập | [PNG](figures/W06.png) | [SVG](figures/W06.svg) |

Mọi giờ/duration/TTL/budget-view là minh họa, không phải water policy. 15 giây là độ chia cấu hình, không buộc reported duration chia hết cho 15; unknown/null không được thay bằng planned/false/success. Applied ≠ enabled ≠ readiness ≠ physical state. Safety-view là expected state trong ví dụ, chưa thực thi arbiter.

## Nguồn và tái tạo dữ liệu

Dùng Python 3 có sẵn, standard library; từ repository root:

```sh
python -B docs/design/walkthrough/prepare_data.py
python -B docs/design/walkthrough/validate_data.py
```

Generator cập nhật đúng ba file: synthetic-data.json, views.json và scenarios.vi.md. Import hai module không ghi file; `-B` tránh bytecode. Validator đọc các file public, kiểm refs/immutability/target/disposition/time/duration/expiry/dedup/card source và pattern examples; negative controls chạy trong bộ nhớ, in summary ra stdout, không tạo raw receipts. Đây là semantic oracle giới hạn cho fixture, không là scheduler/runtime algorithm.

HTML và 7 SVG/7 PNG là **finished outputs được tuyển chọn từ walkthrough information-model 0.2**. Dữ liệu/giá trị card và nội dung hình được giữ; chỉ public text/metadata và link của bundle được làm sạch. [Asset provenance](asset-provenance.json) bind hash nguồn public và từng output. Hash văn bản chuẩn hóa xuống dòng LF để dùng được qua các cấu hình Git; PNG dùng byte gốc. Fingerprint trong footer của hình giữ dấu nguồn presentation ban đầu, còn manifest mô tả các file public hiện tại. Bốn sơ đồ đã xuất trước và ba sơ đồ SVG thường được giữ trong các composite; không chạy lại native export hoặc pipeline thiết kế hình trong lần promotion này. Không đóng gói native QA, profile browser hay toolchain riêng. Các Python command chỉ tái tạo dữ liệu, **không tái tạo HTML/SVG/PNG**; nếu sau này đổi ý nghĩa dữ liệu phải cập nhật và kiểm lại output tương ứng trước công bố.

Một số SVG giữ font JetBrains Mono nhúng và notice/OFL kèm theo trong asset. Attribution/license nhúng được bảo toàn; [third-party notices của repo](../../../THIRD_PARTY_NOTICES.md) và license skill hiện có giữ nguyên vai trò. Không có raw ảnh hiện trường, transcript hay deck trong bundle này.

## Bằng chứng và giới hạn

Promotion dùng lại các hình và semantics từ bản walkthrough đã kiểm tra trước; kiểm tra builder hiện tại tập trung vào public links, source bindings, độ tái tạo dữ liệu và HTML offline. Không tuyên bố tái chạy các gate native/pixel độc lập cũ. Validator PASS không chứng minh completeness tất cả fault hoặc tính đúng của triển khai chưa có.

Runtime scheduler/firmware/physical device/water/production: **NOT_RUN / chưa triển khai**. Independent review và chấp nhận sản phẩm là bước riêng. Software STOP, log/output hoặc screenshot không chứng minh van đóng, có flow hay cây đủ nước. Geometry hiện trường vẫn thuộc viewer/model.json, không thuộc dữ liệu mẫu này.
