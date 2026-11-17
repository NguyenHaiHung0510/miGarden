"""Render the Step 10 review drawings. No product or device dependencies."""

from pathlib import Path
from sys import argv
from xml.sax.saxutils import escape


HERE = Path(__file__).resolve().parent
W = 1240
INK = "#172a40"
MUTED = "#415873"
LINE = "#43637f"
TEAL = "#176050"
AMBER = "#735018"


class Diagram:
    def __init__(self, height, title, subtitle,
                 rubric="MIGARDEN  /  BƯỚC 10  /  KIẾN TRÚC ĐỀ XUẤT"):
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{height}" '
            f'viewBox="0 0 {W} {height}" role="img" aria-labelledby="title desc">',
            f'<title id="title">{escape(title)}</title><desc id="desc">{escape(subtitle)}</desc>',
            '<defs><marker id="arrow" markerWidth="11" markerHeight="11" refX="9" refY="5.5" '
            'orient="auto" markerUnits="userSpaceOnUse"><path d="M0 0 L10 5.5 L0 11 Z" '
            f'fill="{LINE}"/></marker></defs>',
        ]
        self.rect(0, 0, W, height, "#f3f7fb", "none", 0)
        self.rect(0, 0, W, 116, "#172e49", "none", 0)
        self.text(42, 29, rubric, 17, "#a8dfcf", True)
        self.text(42, 72, title, 34, "#ffffff", True)
        self.text(42, 100, subtitle, 17, "#d2e2f2")

    def rect(self, x, y, w, h, fill, stroke, rad=14):
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" '
                          f'rx="{rad}" fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>')

    def text(self, x, y, s, size, fill=INK, bold=False, mono=False, anchor="start"):
        family = "Consolas,monospace" if mono else "Segoe UI,Arial,sans-serif"
        weight = 700 if bold else 400
        self.parts.append(f'<text x="{x}" y="{y}" fill="{fill}" text-anchor="{anchor}" '
                          f'font-family="{family}" font-weight="{weight}" font-size="{size}">'
                          f'{escape(s)}</text>')

    def card(self, x, y, w, h, title, rows, fill="#ffffff", stroke="#bbcbd9", color=INK):
        self.rect(x, y, w, h, fill, stroke)
        self.text(x + 20, y + 32, title, 22, color, True)
        for i, (line, is_code) in enumerate(rows):
            self.text(x + 20, y + 60 + i * 23, line, 17, MUTED, mono=is_code)

    def arrow(self, points, dashed=False):
        coords = " ".join(f"{x},{y}" for x, y in points)
        dash = ' stroke-dasharray="6 6"' if dashed else ""
        self.parts.append(f'<polyline points="{coords}" fill="none" stroke="{LINE}" '
                          f'stroke-width="2.2" stroke-linejoin="round"{dash} '
                          'marker-end="url(#arrow)"/>')

    def save(self, name):
        (HERE / name).write_text("\n".join(self.parts + ["</svg>", ""]), encoding="utf-8")


def app():
    d = Diagram(925, "microSched: màn hình gọi tới cơ sở dữ liệu ra sao",
                "Một ứng dụng FastAPI và một SPA hiện có; hai cửa API với danh tính riêng.")
    d.card(55, 139, 522, 127, "A · Người dùng trong browser  /  G01", [
        ("App / menu → MiGardenScreen", True),
        ("Hook TanStack Query → apiRequest()", True),
        ("Thao tác lịch, remote, trạng thái và lịch sử.", False),
    ])
    d.card(663, 139, 522, 127, "B · Thiết bị kết nối ra ngoài  /  G07", [
        ("DeviceSync → HTTPS/JSON (đề xuất)", True),
        ("ESP32 chủ động lấy lịch, lệnh và gửi report.", False),
        ("Browser không nối thẳng tới ESP32.", False),
    ], "#e9f6f2", "#a4cfc4", TEAL)
    d.rect(30, 298, 1180, 566, "#e9f0f7", "#bed0e0")
    d.text(620, 324, "FASTAPI MICROSCHED  /  CHUNG APP VÀ DATABASE", 16, MUTED, True, anchor="middle")
    d.arrow([(316, 267), (316, 337)])
    d.arrow([(924, 267), (924, 337)])
    d.card(55, 339, 522, 112, "Router người dùng  /  I01", [
        ("web/routers/migarden_human.py", True),
        ("Session hiện có, kiểm quyền và context ở server.", False),
    ])
    d.card(663, 339, 522, 112, "Router thiết bị  /  I02, I03, I04", [
        ("web/routers/migarden_device.py", True),
        ("Xác thực thiết bị, ràng buộc đúng Zone/Controller.", False),
    ], "#e9f6f2", "#a4cfc4", TEAL)
    d.arrow([(316, 452), (316, 480)])
    d.arrow([(924, 452), (924, 480)])
    d.card(55, 482, 522, 145, "Nghiệp vụ  /  domain/migarden/", [
        ("schedule.py → lưu desired / content ref", True),
        ("commands.py → preview, confirm, grant, STOP", True),
        ("queries.py → dựng status và history để đọc", True),
    ])
    d.card(663, 482, 522, 145, "Giao tiếp thiết bị  /  domain/migarden/", [
        ("exchange.py → giao lịch / lệnh đúng thiết bị", True),
        ("reports.py → kiểm, lưu, đối soát và trả receipt", True),
        ("Tra policy của schedule/commands; ingest không OPEN.", False),
    ], "#e9f6f2", "#a4cfc4", TEAL)
    d.arrow([(662, 545), (579, 545)], True)
    d.arrow([(316, 628), (316, 663)])
    d.arrow([(924, 628), (924, 663)])
    d.card(146, 665, 948, 101, "storage.py · repository và giao dịch", [
        ("SQLModel/AsyncSession: nội dung bất biến, desired, command, report và ràng buộc duy nhất.", False),
        ("Giữ đúng bản đã dùng để lịch sử không đổi khi sửa lịch mới.", False),
    ])
    d.arrow([(620, 767), (620, 796)])
    d.rect(245, 798, 750, 48, "#dce8f3", "#a9bdcf", 12)
    d.text(620, 829, "PostgreSQL hiện có  ·  schema tiến hóa qua Alembic", 20, INK, True, anchor="middle")
    d.text(54, 899, "Các tên file là đề xuất. Cửa thiết bị không dùng cookie người dùng và không tự tạo lệnh tưới.",
           18, MUTED)
    d.save("01-component-call-map.svg")


def firmware():
    d = Diagram(936, "ESP32: ba nguồn START, một nơi quyết định",
                "Một firmware trên một bo; quản lý lịch, an toàn local và giao tiếp là các module trong cùng image.")
    d.card(40, 149, 361, 129, "Nút START / STOP  ·  I07", [
        ("platform/esp32/buttons", True),
        ("Đọc IO33 / IO32 và lọc dội.", False),
        ("STOP đi đường ưu tiên.", False),
    ])
    d.card(439, 149, 362, 129, "Lịch đã áp dụng  ·  G04", [
        ("core/schedule", True),
        ("Xét cơ hội theo giờ đủ tin cậy.", False),
        ("Bỏ qua kỳ lỡ hoặc đã xử lý.", False),
    ])
    d.card(839, 149, 361, 129, "Lệnh từ microSched  ·  G07", [
        ("app/device_sync → esp32/http", True),
        ("Nhận candidate, grant và STOP.", False),
        ("Gửi report qua luồng mạng riêng.", False),
    ], "#e9f6f2", "#a4cfc4", TEAL)
    d.arrow([(220, 279), (220, 319), (507, 319), (507, 344)])
    d.arrow([(620, 279), (620, 344)])
    d.arrow([(1020, 279), (1020, 319), (733, 319), (733, 344)])
    d.text(635, 311, "I05", 15, MUTED, True)
    d.card(376, 346, 488, 172, "core/arbiter  ·  G05", [
        ("Kiểm đúng đích, hiệu lực và giới hạn chung.", False),
        ("START: phải xác nhận ghi bền trước OPEN.", False),
        ("STOP thắng, vô hiệu START cũ; không chờ mạng.", False),
        ("Firmware khởi tạo OFF và giữ deadline local.", False),
    ], "#edf3fa", "#9ab4ce")
    d.card(40, 367, 294, 136, "Clock  ·  I06", [
        ("Giờ lịch và chất lượng.", False),
        ("Thời gian trôi cho deadline.", False),
        ("Thiếu căn cứ: không START.", False),
    ])
    d.card(905, 367, 295, 136, "SafetyJournal  ·  I06", [
        ("platform/esp32/nvs", True),
        ("Bản lịch, sổ an toàn, phục hồi.", False),
        ("Ghi lỗi: safe stop / degraded.", False),
    ])
    d.arrow([(375, 440), (335, 440)])
    d.arrow([(865, 440), (904, 440)])
    d.arrow([(620, 519), (620, 558)])
    d.card(376, 560, 488, 100, "OutputPort  ·  I07", [
        ("Vận hành: platform/esp32/gpio", True),
        ("Mô phỏng: ghi lệnh trong RAM trên PC.", False),
    ])
    d.arrow([(620, 661), (620, 698)])
    d.card(376, 700, 488, 104, "IO27 → S của MKE-M06 → cuộn van", [
        ("Nguồn B cấp riêng. Firmware chỉ phát lệnh.", False),
        ("Đo driver để xác nhận đầu ra điện thực tế.", False),
    ], "#fff5e2", "#d7bc82", AMBER)
    d.rect(40, 831, 1160, 75, "#e9f6f2", "#a4cfc4", 13)
    d.text(64, 862, "Report quay về microSched qua DeviceSync (I04); không có đường tắt từ mạng tới GPIO.",
           18, TEAL, True)
    d.text(64, 889, "Không có phản hồi xác nhận vị trí van ở cấu hình hiện tại. Điện mặc định OFF phải kiểm riêng.",
           17, TEAL)
    d.save("02-firmware-call-map.svg")


def fail_off():
    d = Diagram(915, "Bo tắt nhưng driver còn điện: tải có tắt không?",
                "Đọc cùng sơ đồ nguồn 09. Đây là hình giải thích, không phải ảnh đồ thật hay bản khoan.",
                "MIGARDEN  /  CHÚ THÍCH HÌNH 09  /  CHƯA ĐO TRÊN TỦ THẬT")
    red, red_bg, red_line = "#893a29", "#fff0eb", "#e4b4a8"
    green_bg, green_line = "#e9f6f2", "#a4cfc4"
    amber_bg, amber_line = "#fff5e2", "#d7bc82"
    xs = (46, 445, 844)
    width = 350

    d.text(46, 150, "1   NHÌN HÌNH 09: A NUÔI ESP32, B NUÔI TẢI", 17, MUTED, True)
    d.card(xs[0], 169, width, 128, "ESP32 · nguồn A", [
        ("A cắm vào jack của bo.", False),
        ("IO27 gửi lệnh tới S.", True),
        ("GND bo chung mốc 0 V với B.", False),
    ])
    d.card(xs[1], 169, width, 128, "Driver MKE-M06 · nguồn B", [
        ("IN+ / IN−: nguồn B.", True),
        ("S: chân nhận lệnh từ IO27.", False),
        ("Hai cọc OUT cấp cho tải.", False),
    ], green_bg, green_line, TEAL)
    d.card(xs[2], 169, width, 128, "Tải thử · chưa nối van", [
        ("OUT+ / OUT− đi tới tải thử.", True),
        ("Thợ quan sát tín hiệu S và tải.", False),
        ("Đo điện ở đây, chưa kết luận về nước.", False),
    ])
    d.arrow([(397, 226), (441, 226)])
    d.arrow([(796, 226), (840, 226)])
    d.text(46, 327, "Đối chiếu nhãn hai đầu dây trên đồ thật. Không nối tắt OUT− về GND.", 17, MUTED)

    d.text(46, 367, "2   SO BA TRẠNG THÁI, TRONG KHI B VẪN CÓ ĐIỆN", 17, MUTED, True)
    d.card(xs[0], 386, width, 130, "LOW · bo chủ động giữ thấp", [
        ("S ở gần 0 V so với IN−.", False),
        ("Driver phải ngắt tải.", False),
        ("Đây là lệnh tắt bình thường.", False),
    ], green_bg, green_line, TEAL)
    d.card(xs[1], 386, width, 130, "HIGH · lệnh bật có chủ đích", [
        ("S nhận mức cao từ bo.", False),
        ("Tải thử phải bật khi ra lệnh.", False),
        ("Đối chứng để biết phép đo thấy ON.", False),
    ])
    d.card(xs[2], 386, width, 130, "Bo reset hoặc mất nguồn", [
        ("IO27 có thể không giữ mức nào.", False),
        ("Cần mạch giữ S thấp khi bo thả.", False),
        ("Đây là phần phải đo trên tủ thật.", False),
    ], amber_bg, amber_line, AMBER)
    d.rect(46, 540, 1148, 68, "#e9f0f7", "#a9bdcf", 12)
    d.text(65, 568, "‘Thả’ nghĩa là chân không chủ động giữ thấp hay cao. Ta cần một mạch mặc định kéo về LOW.",
           18, INK, True)
    d.text(65, 592, "Điện trở kéo xuống là một cách nếu lỗi do S bị thả; nó không thắng được tín hiệu HIGH bị kẹt.",
           17, MUTED)

    d.text(46, 650, "3   NHÌN KẾT QUẢ RỒI QUYẾT ĐỊNH", 17, MUTED, True)
    d.card(xs[0], 669, width, 151, "Bật được, tự tắt đúng", [
        ("Có ON đối chứng; không cấp tải", False),
        ("ngoài ý muốn khi boot / reset / mất A.", False),
        ("Giữ sơ đồ trong phạm vi đã đo.", False),
        ("Kiểm lại khi tủ hoàn chỉnh.", False),
    ], green_bg, green_line, TEAL)
    d.card(xs[1], 669, width, 151, "Lóe, tự bật hoặc còn bật", [
        ("Tìm nguyên nhân ở S, driver hay dây.", False),
        ("Sửa và thử lại cả bật lẫn tắt.", False),
        ("Chưa nối van để vận hành.", False),
    ], red_bg, red_line, red)
    d.card(xs[2], 669, width, 151, "Phép đo chưa tới", [
        ("Thiếu ca bật hoặc chưa đo được", False),
        ("xung ngắn: chưa kết luận đạt.", False),
        ("Ghi rõ thiếu gì, giữ van cách ly.", False),
    ], amber_bg, amber_line, AMBER)
    d.text(46, 858, "Ca mất A: USB không được vô tình nuôi bo. LED thường chỉ minh họa, có thể bỏ sót xung ngắn.",
           17, MUTED)
    d.text(46, 883, "Ca bo treo đang phát HIGH, driver chập và van kẹt cần kiểm riêng.", 17, MUTED)
    d.save("03-default-off-and-probe.svg")


if __name__ == "__main__":
    if argv[1:] == ["off"]:
        fail_off()
    else:
        app()
        firmware()
        fail_off()
