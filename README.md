# miGarden

miGarden thiết kế tưới có kiểm soát cho vườn hoa/cây cảnh ở ban công, dự kiến tích hợp quản lý vào microSched. Zone là đơn vị tưới, lịch và lịch sử; thiết bị hướng tới chạy lịch hợp lệ và START/STOP local khi mất Internet nhưng vẫn có điện và điều kiện an toàn đáng tin.

**[Baseline thiết kế 01–04](docs/design/README.md) là nguồn thiết kế hiện hành.** Có bảy [ví dụ synthetic mở offline](docs/design/walkthrough/index.html) để đọc domain/information model; bước 05–10 còn pending. Đã có prototype SVG/3D model-v0 chạy local. Chưa triển khai firmware, simulator điều khiển hoặc tích hợp microSched; [project state](docs/project-state.md) phân biệt rõ các mức này.

## Xem và sửa mô hình

Với Node.js22.8+ và browser có WebGL:

**Windows:** bấm đúp **`start-miGarden.cmd`** ở gốc repo hoặc chạy `./start-miGarden.cmd`. Launcher tự chuẩn bị dependency khi thiếu và mở viewer; dùng lại miGarden đang chạy hoặc chọn cổng trống, không cần nhớ cổng. Giữ cửa sổ server mở; Ctrl+C để dừng.

Hoặc chạy thủ công:

```sh
cd viewer
npm ci --ignore-scripts
npm start
```

Mở [viewer local](http://127.0.0.1:4179/?layout=space). Xoay/bay tự do, focus theo luồng nước, bật/tắt nhãn, toàn màn hình, so tuyến và lưu feedback. Clone public không cần ảnh gốc; raw captures và feedback không nằm trong repo. [Hướng dẫn](viewer/README.md) · [baseline và giới hạn](docs/model-v0.md). Chạy `npm test` trong `viewer/` để kiểm dữ liệu và local server; viewer không điều khiển nước thật.

## Thiết kế hiện tại

- **Area nhóm Zone; điều khiển/lịch/history theo Zone.** Ban đầu triển khai A. Mô hình cho phép B/C/D trong khả năng thực tế chưa xác minh của một ESP32 nhiều Actuator, phương án một van nhánh/Zone, tối đa một lịch logic đang hoạt động local/Zone. Không quản lý chậu/placement hoặc danh sách chậu bị tác động. Chín chậu trong khảo sát là hiện trường của prototype.
- **Remote, local schedule và source START/STOP cùng qua local safety arbiter.** Giữ all-source cumulative budget, STOP ưu tiên/ngăn pending START cũ, no-double/no-catch-up/no-replay, unknown block, fail-closed boot và van khóa tay độc lập. Numeric water policy/hardware chưa được chọn hoặc kiểm chứng; candidate [SAF-P01](docs/design/README.md#saf-p01--proposed-giới-hạn-tích-lũy-và-cooldown) chỉ để quyết định ở bước 08.
- **A/SVG + B/Three.js dùng chung `viewer/model.json`.** [Site baseline 2026-10-25 bản 07](docs/site-model.md) giữ số đo, tên chuẩn và phần còn ước lượng; geometry không chứng minh RF/hydraulics.
- **Soil/Zigbee HOLD.** Không tự kế thừa notification lifecycle, inventory CRUD hoặc tưới nhóm vào baseline; nhóm chỉ là mở rộng tương lai.
- **miGarden giữ system source; microSched giữ human web/API/storage adapter.** Contract/revision của hai bên phải được gắn khi tích hợp; chưa chọn API/schema/transport bằng việc công bố thiết kế này.

## Đọc theo nhu cầu

| Bạn muốn tìm hiểu | Tài liệu |
|---|---|
| Mục đích, yêu cầu, năm UC, domain và dữ liệu | [Design 01–04](docs/design/README.md) |
| Ví dụ desired/applied, lịch sử bất biến, unknown/time | [Walkthrough offline](docs/design/walkthrough/index.html), [hướng dẫn](docs/design/walkthrough/README.vi.md) |
| Đã có gì, còn thiếu gì | [Project state](docs/project-state.md) |
| Mô hình thực địa, tên gọi và số đo | [Site model](docs/site-model.md), [site baseline](docs/site-baseline.md) |
| Nguồn chung SVG/Three.js và content policy | [Representation](docs/representation.md) |
| Bring-up, evidence và recovery tương lai | [Operations](docs/operations.md) |
| Skill kỹ thuật và license | [Skills](docs/skills.md), [notices](THIRD_PARTY_NOTICES.md) |
| Nguồn thiết kế và vai trò lịch sử | [Provenance](docs/provenance.md) |
| Thiết kế cũ, chỉ reference-only | [Specification](docs/specification.md), [architecture](docs/architecture.md), [design inputs](docs/design-inputs.md) |

## Đóng góp

Ở giai đoạn này, góp ý hữu ích nhất là chỉ ra requirement mâu thuẫn, failure path còn thiếu, assumption chưa có evidence hoặc cách thể hiện site dễ kiểm tra hơn. Issue/PR nên nêu section liên quan, tình huống cụ thể, hành vi mong muốn và nguồn/bằng chứng nếu có. Đề xuất số đo hoặc linh kiện cần nói rõ điều kiện áp dụng; thông số quảng cáo chưa là kết quả kiểm chứng.

Agent hỗ trợ phát triển bắt đầu từ [AGENTS.md](AGENTS.md), rồi chỉ đọc tài liệu và skill phù hợp với nhiệm vụ. Đóng góp asset tuân theo [quy tắc dữ liệu công khai](docs/representation.md#public-content-and-private-sources). MIT notices trong các skill chỉ áp dụng cho nội dung được nêu trong notices; repository chưa công bố license chung cho phần miGarden.
