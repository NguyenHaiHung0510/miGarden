# Project skills — miGarden

Ngày review: **2026-10-22, Asia/Saigon**. Ba skill dưới đã được cài thành file trong `.agents/skills/`, qua bundled Codex skill-installer với immutable `--ref` và project `--dest`, sau exact-source/license review. Bản thích nghi **mg-h1.1** là project-local installed pilot. Chưa nâng trạng thái thành proven runtime workflow.

Mọi Agent được tự chọn/invoke skill phù hợp với task và assigned scope; không yêu cầu người dùng gọi tên, không gate theo PM/T1. Mỗi package có `agents/openai.yaml` đặt `allow_implicit_invocation: true`. Việc chọn skill không cấp quyền build/install toolchain, flash/reset/erase, provisioning, publish hay điều khiển vật lý. Task/project contract có ưu tiên cao hơn skill.

| Skill / một nhiệm vụ chính | Immutable source và license | Phiên bản, adaptations và compatibility |
|---|---|---|
| [esp32-development](../.agents/skills/esp32-development/SKILL.md): board/pin/power, bring-up planning, boot/firmware diagnosis và build-to-device evidence | [magnus919/agent-skills, `92c327e525da69c9ad3812fd223a1770e977c2e3`](https://github.com/magnus919/agent-skills/tree/92c327e525da69c9ad3812fd223a1770e977c2e3/esp32-development), [MIT](../.agents/skills/esp32-development/LICENSE) | `mg-h1.1`; upstream không công bố semantic version của skill trong entry đã đọc. Thu hẹp cross-framework entry; giữ 3 references + 3 worksheets, thêm Windows preflight. Không cài Python helper, firmware/config templates, eval suite hay framework. Windows COM mapping cần evidence thật; helper upstream không enumerate COM và danh sách rỗng không chứng minh thiếu board. |
| [threejs-fundamentals](../.agents/skills/threejs-fundamentals/SKILL.md): scene/camera/renderer lifecycle và transforms | [CloudAI-X/threejs-skills, `0e181866f095b5f06d005133876e926515390d95`](https://github.com/CloudAI-X/threejs-skills/tree/0e181866f095b5f06d005133876e926515390d95/skills/threejs-fundamentals), [MIT](../.agents/skills/threejs-fundamentals/LICENSE) | `mg-h1.1`; reference API target r186, import-map examples `three@0.186.0`, README upstream tự báo kiểm trên `0.186.1`. Giữ technical body trong reference, entry thêm shared SVG/3D units, coordinates và provenance. Không tự pin hay cài package sản phẩm. |
| [threejs-interaction](../.agents/skills/threejs-interaction/SKILL.md): picking/hover/camera/drag/transform trong scene | [CloudAI-X/threejs-skills, cùng SHA](https://github.com/CloudAI-X/threejs-skills/tree/0e181866f095b5f06d005133876e926515390d95/skills/threejs-interaction), [MIT](../.agents/skills/threejs-interaction/LICENSE) | `mg-h1.1`; cùng API target. Entry thêm canvas-relative coordinates, stable IDs, view/edit/command distinction, cleanup và keyboard evidence. Bỏ danh sách See Also cuối trỏ toàn suite chưa cài; không thêm dependency React Three Fiber. |

Mỗi package có `PROVENANCE.json` ghi source path/SHA/date, SHA256 và Git blob của source files/license, disposition giữ/sửa/bỏ, local modification notice và hash payload đã cài. Provenance không tự hash chính nó; private final manifest có hash đầy đủ cả file đó. [Third-party notices](../THIRD_PARTY_NOTICES.md) giữ attribution/license. Không bulk-update đè adaptation; update cần review đúng diff và giữ license.

## Dependencies và quyền thực thi

Đọc các package này chỉ cần Markdown/YAML support. Không package nào chứa executable helper, plugin manifest, MCP server, hook, credential requirement hoặc provider dependency. Các JavaScript example là reference, không được chạy khi nạp skill.

Build/device work tương lai cần board/SDK/toolchain/resources được chọn và cấp quyền riêng theo task; package/board/config hiện chưa được nghiệm thu bằng việc cài skill. Three.js implementation cần package version/lockfile, browser và renderer/GPU phù hợp của sản phẩm. CDN import maps chỉ là phương án upstream; ưu tiên dependency/assets đã được project chọn, không tự tải nguồn riêng hay gửi raw ảnh/video lên dịch vụ ngoài.

Skill instructions dùng đường dẫn tương đối, không dựa trên `.claude/`, POSIX shell, absolute path cá nhân hay host riêng. Thao tác build/serial/physical trong reference là planning material; opening serial/querying chip/test fixtures vẫn có thể reset/flash. Không có auto-board detector hoặc firmware runner được cài cùng.

## Ví dụ trigger để review

Các ví dụ sau là **static trigger review**, không phải model-selection eval hoặc observed automatic invocation.

| Skill | Nên tự chọn khi task phù hợp | Không nên tự chọn |
|---|---|---|
| ESP32 | “Lập bring-up plan từ schematic ESP32 và chỉ rõ facts còn thiếu”; “Phân tích boot log/brownout của board ESP32” | “Sửa CSS card”; “Chọn camera trong Three.js”. Yêu cầu flash có thể cần kiến thức ESP32, nhưng skill không cấp quyền thực thi flash. |
| Fundamentals | “Map cùng spatial fixture sang SVG và Three.js, nêu units/axes”; “Sửa camera aspect khi Three.js canvas resize” | “Tô màu SVG thuần”; “Tái dựng nhà bằng COLMAP”. |
| Interaction | “Pick chậu trong Three.js canvas nhúng, đồng bộ object ID”; “Sửa OrbitControls và drag gizmo tranh gesture” | “Validate form đăng nhập”; “Gửi OPEN xuống van”. Lệnh vật lý phải đi theo contract của task, không qua view handler chỉ vì pick được mesh. |

## Reuse và defer

Reuse các skill user/system đã có nếu runtime cung cấp: `owner-pstack`, `owner-security-review`, `maintain-codebase`, UI/UX và browser QA, `skill-installer`, `skill-creator`. Không copy chúng vào project và không thêm router thay thế.

[Trail of Bits sharp-edges](https://github.com/trailofbits/skills/tree/main/plugins/sharp-edges/skills/sharp-edges) giữ **external reference-only** cho một security review được yêu cầu qua `owner-security-review` khi skill đó khả dụng. Đây là locator mutable, không phải source pin đã admission. Không copy nội dung CC-BY-SA, không cài skill/agent/plugin hoặc runner; nếu cần dùng sau này phải đọc đúng nguồn/license/revision và áp actual review scope. Routine firmware work không tự mở broad security audit.

Jeffallan embedded-systems, ezrover ESP32, magnus919 digital-twin và reusable migrate-harness skill tiếp tục **deferred**. Không cài full suite. Skill admission này không thay framework/architecture lựa chọn của dự án.

## Evidence và giới hạn discovery

Static validation, byte/hash integrity, local links và JavaScript syntax được ghi trong private skills receipts. Đây là builder checks; independent acceptance thuộc integration review. Cài file thành công khác với catalog discovery, automatic selection và runtime execution.

Với Codex, project-local skill root là `.agents/skills/` tại project mà Agent đang chạy; các runtime khác phải hỗ trợ hoặc được trỏ tới cùng Markdown packages. Phiên independent Codex CLI **Luna/xhigh** mới tại root miGarden ngày 2026-10-22 đã có đủ ba entry trong injected skill catalog và tự chọn/đọc cả ba khi xử lý yêu cầu lên kế hoạch ESP32 + thiết kế viewer 3D, user prompt không gọi tên skill. PM kiểm scoped metadata và tool trace; đây là **observed discovery/selection cho một representative task**, không phải always-trigger guarantee hoặc runtime implementation proof. [Báo cáo MG-H1](reports/2026-oct-22-mg-h1.md) ghi phân lớp evidence.

Three.js rendering/interaction, firmware/device/field execution và cross-runtime discovery vẫn **NOT_RUN/UNVERIFIED**. Không helper hay code example nào được thực thi khi nạp skill. Source/hash receipts riêng giữ giá trị cho payload không đổi; `.gitattributes` cố định LF để license/provenance không lệch do Windows autocrlf.

Review lại sau unexpected selection, conflict với project contract, upstream change, SDK/Three.js major change hoặc lần dùng thật cho thấy thiếu guidance. Rollback bằng restore/remove đúng ba owned skill directories và catalog/notices tương ứng; không sửa global skills/config hoặc đụng task khác. PM thực hiện root/Git/release integration theo batch chung.
