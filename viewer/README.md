# miGarden model-v0 viewer

Prototype đối chiếu vườn chậu hoa/cây cảnh ở ban công. SVG và Three.js dùng cùng `model.json`; đây là **ước lượng để cùng sửa**, chưa phải CAD đo đạc, mô phỏng thủy lực hoặc điều khiển thiết bị.

## Chạy local

Node.js22.8+ và browser có WebGL; đã kiểm bằng Node24.15.0, Three.js0.186.1.

**Windows:** bấm đúp `start-miGarden.cmd` ở gốc repo, hoặc chạy một lệnh từ PowerShell:

```powershell
.\start-miGarden.cmd
```

Launcher cài dependency theo lockfile khi thiếu, mở browser và dùng lại miGarden đang chạy. Mặc định thử4181 rồi tối đa9 cổng kế tiếp nếu bị ứng dụng khác chiếm; không kill process. Khi tạo server mới, giữ cửa sổ đó mở và Ctrl+C để dừng. Có thể truyền `--port 4300`; `--no-browser` chỉ chạy/in URL. Sau khi đã cài dependency, `node viewer/start.mjs` từ gốc repo cũng dùng được trên Windows/macOS/Linux.

Hoặc chạy thủ công:

```sh
cd viewer
npm ci --ignore-scripts
npm start
```

Mở [viewer](http://127.0.0.1:4179/?layout=space). Server chỉ bind loopback. Nếu cổng đang dùng, đặt biến môi trường `MIGARDEN_PORT`; dừng bằng Ctrl+C trong đúng terminal. Sau khi cài dependency, viewer không cần CDN.

## Sử dụng

- Xoay/zoom hoặc chọn **Toàn cảnh / Ban công / Nguồn nước / Tường phải**. SVG có mặt bằng và mặt cắt.
- So tuyến men trái, phải đi ngang, phải hạ thấp trên ống hiện hữu; đường chéo giữ tham khảo. Chiều dài/đỉnh là synthetic, chưa cộng dư thi công; dấu◇ là nơi cần thiết kế gá/đỡ.
- So giữ vòi tay đỏ một đầu hoặc thay vòi≥2 đầu: một miGarden và một sinh hoạt dùng tay. Chưa chọn linh kiện hoặc lắp thực tế.
- Sửa điểm bằng gizmo3D hoặc nhập tọa độ; thêm/bỏ/hoàn tác và ghi feedback. **Lưu cho Agent** giữ tuyến, lựa chọn vòi và ghi chú local. Đổi hình không điều khiển nước.
- Chậu là mẫu linh động,9 không là capacity; nhánh minh họa không tạo actuator riêng.

## Ảnh và feedback riêng tư

Clone public chạy model/SVG/editor **không cần ảnh gốc**. Inspector hiện ID và “Ảnh gốc giữ riêng”, không tạo thumbnail lỗi. Raw photo/video, conversations và feedback không nằm trong repo.

Optional local file `../.local/viewer-sources.json` là mảng các record như `{ "id": "R04", "path": ".local/photos/source.png" }`. Đây là placeholder, không phải ảnh được cung cấp. Chỉ đường dẫn nằm trong ignored `.local` của repo được phục vụ theo ID; `/api/sources` không trả filesystem paths.

Feedback mặc định ở `../.local/viewer-feedback/current-draft.json`, kèm immutable `snapshot-*.json`. Browser giữ nháp riêng; nháp browser chưa đồng nghĩa đã lưu project. Nạp nháp cũ là thao tác chủ động. Revision check chống ghi đè giữa các tab. Xuất dự phòng trước khi xóa browser storage hoặc đổi máy/cổng. `MIGARDEN_FEEDBACK_DIR` có thể chọn thư mục khác trong `.local`; không nhận đường dẫn từ HTTP.

Đây là server preview local, không phải dịch vụ Internet nhiều người dùng hoặc backend sản phẩm thứ hai.

## Kiểm tra

```sh
npm test
```

Tests chạy cùng process, kiểm model/route/SVG, clone không có ảnh, static allowlist, Origin/schema/revision và concurrent saves, private source aliases. Fixtures riêng được tạo/xóa trong ignored `.local`, không sửa feedback thật. WebGL/camera/drag cần kiểm trên browser thực; HTTP/tests không thay visual/field acceptance.

Xem [baseline và giới hạn](../docs/model-v0.md). Dependency duy nhất Three.js0.186.1, MIT, giữ [notice](THREE-LICENSE.txt); license này không thay license project miGarden.
