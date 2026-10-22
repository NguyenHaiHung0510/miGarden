# miGarden

miGarden là hệ thống chăm sóc vườn thật, tích hợp giao diện, API và lưu trữ với microSched. MVP hướng tới **tưới chung Zone A từ xa và theo lịch deterministic đã duyệt**, có local START/STOP tại nguồn. MG-H1 cung cấp tài liệu, harness và project skills để tiếp tục requirements/design; firmware, scene, simulator và water deployment chưa được triển khai trong batch này.

## Bắt đầu từ một chat mới

1. Đọc [AGENTS.md](AGENTS.md) và [trạng thái hiện hành](docs/project-state.md): quyết định nào đã chốt, việc nào còn mở, evidence nào chưa chạy.
2. Requirements/architecture: [specification](docs/specification.md), [architecture](docs/architecture.md), [course/design inputs](docs/design-inputs.md).
3. Hiện trường/A+B: [site baseline](docs/site-baseline.md), [representation contract](docs/representation.md). Các số đo có provenance và uncertainty; ảnh gốc giữ local.
4. Làm việc/kiểm chứng: [harness applicability và M01–M26](docs/harness.md), [operations/runbooks](docs/operations.md), [provenance](docs/provenance.md).
5. Skills đã cài trong project: [usage và trạng thái](docs/skills.md), [licenses/notices](THIRD_PARTY_NOTICES.md). Một phiên mới tại root miGarden đã discover và tự chọn cả ba từ yêu cầu thông thường, không cần gọi tên.

Trên máy Owner, đọc thêm `.local/continuity.md` và `.local/owner-machine.json` để tìm đúng canonical ledger/shared authority, source snapshots, media và phần việc đang bàn giao. Không cần mở chat PM cũ để biết current product state. Private locator không có trong public clone; task mới phải mang grant của chính nó.

## Ranh giới và dữ liệu

System source thuộc miGarden; human web/API/Postgres adapter tiếp tục thuộc microSched. Hai bên dùng versioned contracts và compatibility receipt theo exact revision; không có database hay web service thứ hai được mở mặc định. Khảo sát product microSched dùng `develop`, `main` dùng release tags.

Raw ảnh/video, frames/contact sheets, transcripts/ledger/receipts và đường dẫn cá nhân ở `.local/`, không commit. Finished drawings/3D/COLMAP của site thật được phép public; không cần thay bằng synthetic chỉ vì là nhà thật. Chưa có finished measured geometry/COLMAP trong candidate này. Không đưa cả slide library hoặc kho học kỳ sang project.

## Trạng thái kiểm chứng

Document/link/hash/private-separation checks và independent review đã thực hiện; fresh-root Codex đã chứng minh discovery/selection ở một task handoff. [Báo cáo MG-H1](docs/reports/2026-oct-22-mg-h1.md) phân biệt worker claims, PM reconciliation và giới hạn; release/canonical cutover có receipt riêng trên máy Owner. Device/toolchain/app/DB/flash/field tests **NOT_RUN**. Có tài liệu/skill không chứng minh hardware workflow đã hoạt động. Xem [operations](docs/operations.md) để tiếp tục đúng plane.
