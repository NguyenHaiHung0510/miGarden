# miGarden

miGarden là bộ tài liệu thiết kế hệ thống tưới và quản lý một vườn chậu thật, dự kiến tích hợp giao diện, API và lưu trữ với microSched. Mục tiêu là biết chậu nào đang ở đâu, xem lịch sử chăm sóc và tưới chung một khu từ xa hoặc theo lịch, kể cả khi mất Internet nhưng controller vẫn có điện và trạng thái an toàn hợp lệ.

**Dự án hiện ở giai đoạn tài liệu và thiết kế.** Repository có specification, khảo sát hiện trường, thiết kế điều khiển và hướng dẫn kỹ thuật; chưa có firmware, simulator, SVG/3D viewer hoặc tích hợp microSched chạy được. Vì vậy chưa có executable quickstart. Xem [trạng thái sản phẩm](docs/project-state.md) để phân biệt thiết kế đã chọn với phần còn thiếu bằng chứng.

## Thiết kế hiện tại

- **Zone A là một mạch tưới chung.** Preview phải liệt kê mọi chậu bị tác động; chưa có điều khiển độc lập từng chậu. Chín chậu là snapshot khảo sát, inventory và lịch sử vị trí vẫn thay đổi được.
- **Remote và local cùng qua một safety arbiter.** Thiết kế có “Tưới ngay” trên web, lịch deterministic được quản lý/version trên cloud và START/STOP tại nguồn. Controller chỉ tiếp tục lịch offline khi thời gian, cấu hình, ledger và safety còn hợp lệ; không chạy bù hoặc chạy trùng.
- **Bản đồ dùng A/SVG + B/Three.js**, cùng một nguồn spatial data có version, provenance và uncertainty. Geometry chưa được dựng thành viewer.
- **Soil sensing/Zigbee đang HOLD.** Chúng không là điều kiện của irrigation MVP; thiết kế calibration/recommendation chỉ áp dụng nếu lane sensing được mở lại.
- **miGarden giữ system source; microSched giữ web/API/Postgres adapter.** Hai repository dùng versioned contracts; không mở thêm database hoặc web backend mặc định.

Fail-closed, unknown outcome, cumulative water budget và manual shutoff độc lập là các ràng buộc thiết kế. [Contract điều khiển và an toàn](docs/specification.md#14-controlled-irrigation-commandack-contract---product-mvp-field-enable-còn-gated) quy định bằng chứng cần có trước khi cho phép mở nước thật.

## Đọc theo nhu cầu

| Bạn muốn tìm hiểu | Tài liệu |
|---|---|
| Đã có gì, còn thiếu gì | [Project state](docs/project-state.md) |
| Vì sao có các yêu cầu này | [Design inputs](docs/design-inputs.md) |
| Ranh giới hệ thống và tích hợp | [Architecture](docs/architecture.md) |
| Domain, API, command/schedule và safety | [Specification](docs/specification.md) |
| Dữ kiện hiện trường và mức chắc chắn | [Site baseline](docs/site-baseline.md) |
| Nguồn chung SVG/Three.js, correction và content policy | [Representation](docs/representation.md) |
| Thiết kế kiểm chứng, bring-up và recovery | [Operations](docs/operations.md) |
| Skill kỹ thuật, version và license tương ứng | [Skills](docs/skills.md), [notices](THIRD_PARTY_NOTICES.md) |
| Nguồn khảo sát và tài liệu thiết kế | [Provenance](docs/provenance.md) |

## Đóng góp

Ở giai đoạn này, góp ý hữu ích nhất là chỉ ra requirement mâu thuẫn, failure path còn thiếu, assumption chưa có evidence hoặc cách thể hiện site dễ kiểm tra hơn. Issue/PR nên nêu section liên quan, tình huống cụ thể, hành vi mong muốn và nguồn/bằng chứng nếu có. Đề xuất số đo hoặc linh kiện cần nói rõ điều kiện áp dụng; thông số quảng cáo chưa là kết quả kiểm chứng.

Agent hỗ trợ phát triển bắt đầu từ [AGENTS.md](AGENTS.md), rồi chỉ đọc tài liệu và skill phù hợp với nhiệm vụ. Đóng góp asset tuân theo [quy tắc dữ liệu công khai](docs/representation.md#public-content-and-private-sources). MIT notices trong các skill chỉ áp dụng cho nội dung được nêu trong notices; repository chưa công bố license chung cho phần miGarden.
