# miGarden model-v0 viewer

Prototype đối chiếu vườn chậu hoa/cây cảnh ở ban công. SVG và Three.js dùng cùng `model.json`; baseline 07-r2 có **số đo thước dây và phần còn ước lượng**, chưa phải CAD đo đạc, mô phỏng thủy lực hoặc điều khiển thiết bị.

## Chạy local

Node.js22.8+ và browser có WebGL; đã kiểm bằng Node24.15.0, Three.js0.186.1.

**Windows:** bấm đúp `start-miGarden.cmd` ở gốc repo, hoặc chạy một lệnh từ PowerShell:

```powershell
.\start-miGarden.cmd
```

Launcher cài dependency theo lockfile khi thiếu, mở browser và dùng lại miGarden đang chạy cùng build. Nếu cổng có server bản cũ, launcher chọn cổng kế tiếp để tránh API lưu cũ. Mặc định thử4181 rồi tối đa9 cổng kế tiếp nếu bị ứng dụng khác chiếm; không kill process. Khi tạo server mới, giữ cửa sổ đó mở và Ctrl+C để dừng. Có thể truyền `--port 4300`; `--no-browser` chỉ chạy/in URL. Sau khi đã cài dependency, `node viewer/start.mjs` từ gốc repo cũng dùng được trên Windows/macOS/Linux.

Hoặc chạy thủ công:

```sh
cd viewer
npm ci --ignore-scripts
npm start
```

Mở [viewer](http://127.0.0.1:4179/?layout=space). Server chỉ bind loopback. Nếu cổng đang dùng, đặt biến môi trường `MIGARDEN_PORT`; dừng bằng Ctrl+C trong đúng terminal. Sau khi cài dependency, viewer không cần CDN.

## Sử dụng

- **Xoay**: kéo để xoay, chuột phải kéo để dịch, cuộn zoom. **Bay**: bấm vào hình, WASD di chuyển, Q/E xuống/lên, ←/→ quay hướng nhìn360°, ↑/↓ ngẩng/cúi, giữ chuột phải kéo để nhìn, Shift nhanh, Esc về Xoay. Tab ra ngoài vùng hình sẽ ngừng phím bay.
- **Đi nhanh đến** theo luồng nước; chọn bồn/bơm/tường/hàng rào riêng. **Nổi bật điểm chọn** bật/tắt độc lập.
- Tắt **Số đo / ước lượng / nhãn** để xem sạch không bị chữ che hình. **Toàn màn hình** giữ điều khiển; nếu browser không cấp quyền thì báo rõ đang mở rộng trong cửa sổ.
- Đợt một: **vòi đỏ + DNV5 → dây mềm 1,2 m → van NC 12 V ở tường phải → uPVC theo PA2 → đầu chờ bịt dưới rào cũ**. Mốc S → đỉnh rào 11,90 m; tim uPVC dựng hình ≈12,56 m. Dư cắt chỉnh được, không phải số mua/cắt. PA1 và PA2 bản 07 là các lớp so sánh; editor chỉ sửa uPVC đợt một.
- Bật/tắt **Sau Y / phần nới** để xem hai hướng mềm cùng Zone A và viền nới ước lượng. Chậu chỉ là mẫu không gian, không sinh đường ống/béc. Viewer không điều khiển bơm hoặc van.
- Sửa điểm/gizmo hoặc tọa độ mét; thêm/bỏ/hoàn tác/về tuyến chuẩn và ghi feedback. **Lưu cho Agent** giữ nháp local, không ghi đè model.

## Ảnh và feedback riêng tư

Clone public chạy model/SVG/editor **không cần ảnh gốc**. Inspector hiện ID và “Ảnh gốc giữ riêng”, không tạo thumbnail lỗi. Raw photo/video, conversations và feedback không nằm trong repo.

Optional local file `../.local/viewer-sources.json` là mảng các record như `{ "id": "R04", "path": ".local/photos/source.png" }`. Đây là placeholder, không phải ảnh được cung cấp. Chỉ đường dẫn nằm trong ignored `.local` của repo được phục vụ theo ID; `/api/sources` không trả filesystem paths.

Feedback mặc định đợt một ở `../.local/viewer-feedback/model-v0-draft-08/current-draft.json`, kèm các `snapshot-*.json`. Bản lưu trước đó ở thư mục cha giữ nguyên. Browser dùng cache riêng cho revision mới; chỉ nhập **nội dung ghi chú cũ** nếu bạn bấm nạp, không áp tọa độ 07 hoặc cũ hơn. Kiểm revision chống ghi đè giữa các tab. Xuất dự phòng trước khi xóa browser storage hoặc đổi máy/cổng. `MIGARDEN_FEEDBACK_DIR` chọn thư mục gốc khác trong `.local`; nháp mới vẫn ghi trong thư mục con theo model.

Đây là server preview local, không phải dịch vụ Internet nhiều người dùng hoặc backend sản phẩm thứ hai.

## Xuất lại ảnh README

Mở **/hero** trên chính cổng viewer (ví dụ [localhost:4179/hero](http://127.0.0.1:4179/hero)), rồi bấm **Tải PNG 1920 × 1080**. [Nguồn xuất](../docs/assets/migarden-zone-a-hero.html) lưu cả vị trí, đích, span và kích thước của hai camera. Trang xuất đọc model chuẩn, không đọc nháp, ảnh riêng hoặc API feedback. Kết quả có thể khác nhẹ theo font và GPU; hướng camera và nội dung tuyến giữ nguyên với cùng revision.

## Kiểm tra

```sh
npm test
```

Tests chạy cùng process, kiểm model/route/SVG, clone không có ảnh, static allowlist, Origin/schema/revision và concurrent saves, private source aliases. Fixtures riêng được tạo/xóa trong ignored `.local`, không sửa feedback thật. WebGL/camera/drag cần kiểm trên browser thực; HTTP/tests không thay visual/field acceptance.

Xem [baseline và giới hạn](../docs/model-v0.md). Dependency duy nhất Three.js0.186.1, MIT, giữ [notice](THREE-LICENSE.txt); license này không thay license project miGarden.
