# N01 · Gói commissioning offline để Owner review

Công cụ [commission.py](commission.py) **chỉ đọc file và tạo bundle riêng trên máy**.
Không mở serial, không tìm bo, không chạy esptool, không đổi app/boot/NVS/filesystem
thật. Tạo bundle không cấp quyền dùng nó trên thiết bị. Production credentials và
data initialization vẫn **HOLD** đến khi Owner duyệt phạm vi cụ thể.

Trường hợp được công cụ xử lý là commissioning lần đầu: **toàn NVS 0x5000 byte
và data 0x160000 byte đều trống 0xff**, được parent đọc/capture dưới quyền riêng.
Đầu vào nonblank bị từ chối, kể cả có vẻ là LAB profile hoặc một filesystem rỗng.
Nhờ vậy công cụ không diễn giải lại, repack hoặc đè một namespace/journal chưa biết.
Máy hiện tại đã có LAB NVS không thuộc đường ghi ảnh này.

| Trường hợp | Cách đi tiếp |
|---|---|
| NVS và data blank, exact layout đã xác minh | Chuẩn bị ảnh NVS profile riêng và ảnh LittleFS genesis; Owner review trước ghi. |
| LittleFS đã có LAB journal hoặc dữ liệu khác | Giữ nguyên toàn bộ filesystem. Muốn thêm release journal phải có giao dịch tạo file mới với điều kiện exclusive/nonexistence và căn cứ profile chưa từng vận hành. |
| NVS đã có LAB profile hoặc dữ liệu khác | Giữ nguyên NVS. Cần writer cấp namespace `mg-rel-v1` và key `profile-v1` sau kiểm toàn namespace đích chưa tồn tại; không được dùng ảnh NVS mới để thay vùng cũ. |
| Journal tồn tại/hỏng/unknown hoặc không đủ căn cứ lần commissioning | Không ghi genesis và không clear state. Phục hồi/đối soát riêng; giữ STORAGE_FAULT/OUTCOME_UNKNOWN. |

Hai dòng giữa là **phụ thuộc vận hành còn mở**, không có lệnh writer đó trong
release/USB v1 hiện tại. Gói này hoàn chỉnh cho **first-blank commissioning**;
không giả vờ có đường bảo toàn NVS đã dùng bằng cách tạo một ảnh mới. Phương án
một commissioner firmware riêng để chèn namespace/file vào storage hiện hữu cần
gói grant/contract/review riêng trước khi triển khai hoặc nạp; không tự thêm protocol.

### Chuẩn bị tool trên máy

PowerShell, Python sẵn có. Dùng venv riêng; không đổi Python/package toàn máy.
[NVS generator 0.3.0 chính thức](https://pypi.org/project/esp-idf-nvs-partition-gen/0.3.0/)
tạo blob format v2; parser độc lập từ
[ESP-IDF v5.5.5](https://github.com/espressif/esp-idf/blob/v5.5.5/components/nvs_flash/nvs_partition_tool/nvs_parser.py)
kiểm namespace, key, header/entry/blob CRC và dựng lại đúng bytes profile.
CLI bắt buộc hash parser
`621bdbf0ac60e34ae190f63be10f0f1a4dd4d18c25ebab0cf56ff53a6f2b6c2c`.
Không dùng dump tool để in giá trị private.

```powershell
python -m venv .local/n01/toolenv
& ./.local/n01/toolenv/Scripts/python.exe -m pip install esp-idf-nvs-partition-gen==0.3.0
New-Item -ItemType Directory -Force .local/n01/deps/nvs-tool-v5.5.5
Invoke-WebRequest https://raw.githubusercontent.com/espressif/esp-idf/v5.5.5/components/nvs_flash/nvs_partition_tool/nvs_parser.py -OutFile .local/n01/deps/nvs-tool-v5.5.5/nvs_parser.py
Get-FileHash .local/n01/deps/nvs-tool-v5.5.5/nvs_parser.py -Algorithm SHA256
& ./.local/n01/toolenv/Scripts/python.exe code/controller/commission.py --help
```

`mklittlefs` dùng bản đã cài cùng SDK, block4096/page256. Simulator vừa build
bằng source candidate tạo genesis đúng binary format (không report/config/seed),
độc lập release profile. Build host mới tạo thư mục unique; đọc `HOST_OUTPUT`
để lấy đường dẫn `--sim`. Không ghi đè output ea140ed do QA đang giữ.

### Profile và lệnh cụ thể

Parent chuẩn bị file UTF-8 private, không BOM, chứa:
`device_id`, `wifi_ssid`, `wifi_password`, `base_url` (origin HTTPS),
`device_token`, `ca_pem`, và **`fixture_verified:true`** cho release.
CA chỉ là public certificate, không đưa private key vào profile. Flag fixture là
lời xác nhận của operator theo nghiệm thu riêng; công cụ không chứng minh thay.
Đưa giá trị qua file dưới `.local`/`provisioning-private` hoặc `--private-config -`
(stdin), không argv, không echo.

Ví dụ đường dẫn, dùng actual candidate manifest và output host đã freeze:
```powershell
$py = './.local/n01/toolenv/Scripts/python.exe'
$args = @(
  'code/controller/commission.py', 'prepare', '--profile', 'release',
  '--private-config', '.local/owner-provisioning-private/release.json',
  '--nvs-before', '.local/owner-provisioning-private/nvs-blank-readback.bin',
  '--data-before', '.local/owner-provisioning-private/data-blank-readback.bin',
  '--app-manifest', '.local/n01/build/RELEASE_BUILD_ID/manifest.json',
  '--sim', '.local/n01/host-builds/HOST_OUTPUT/sim.exe',
  '--out', '.local/owner-provisioning-private/release-bundle'
)
& $py @args --dry-run
& $py @args
& $py code/controller/commission.py validate --bundle .local/owner-provisioning-private/release-bundle
```

`--dry-run` đọc/kiểm đầu vào, in kế hoạch đã lọc và không tạo file. Prepare chỉ
tạo thư mục mới, không có `--force` hoặc overwrite. Exit0 = thành công,
2 = input/format không hợp lệ, 3 = boundary hold (nonblank/existing/fixture),
4 = tool/I/O/integrity failure. Stdout là JSON status; stderr là mã lỗi, không
in exception/private content hay output của generator.

Bundle chứa profile private, CSV tham chiếu file (không raw secret trong command),
`profile-nvs.bin` 0x5000 byte có namespace `mg-rel-v1`/key `profile-v1`,
`data-genesis.bin` 0x160000 byte có duy nhất `release-v1.journal`, manifest
hash/preimage và `OWNER-APPLY.txt`. NVS/file này chứa secret; giữ riêng, không
commit/upload prompt/đưa screenshot. Firmware app không compile credential.
Công cụ pack rồi unpack LittleFS đối chiếu genesis, parse NVS dựng lại profile
và kiểm không chứa token trong app BIN đã pin. `validate` kiểm lại bundle.

### Phạm vi ghi để Owner duyệt sau

`OWNER-APPLY.txt` có command **tham khảo, không tự chạy** cho ROM-loader session
do parent sở hữu, sau khi grant và preflight xác minh blank lại ngay trước ghi:

```text
<OWNER_PYTHON> -m esptool --chip esp32 --port <LEASED_PORT> --baud 115200 --before no-reset --after no-reset write-flash --flash-mode keep --flash-freq keep --flash-size keep 0x9000 profile-nvs.bin 0x290000 data-genesis.bin
```

Lệnh này có erase/program các sector thuộc hai vùng; không phải “ghi file không
format”. Không bootloader/partition table/eFuse/erase-all. Nó **không đi qua**
USB updater v1 (chỉ nhận app). Phải có đường vào ROM/recovery được xác minh và
được Owner cấp quyền; không hứa script tự reset/start bo. Sau transfer, đọc lại
hai vùng privately đối chiếu digest rồi normal boot app release đã pin bằng
phương thức được duyệt; kiểm journal/clock/config/readiness/fixture riêng.
Nếu timeout sau possible write, inspect trước retry. Nếu vùng không còn blank,
dừng: gói ảnh không tự tiếp tục lên vùng đã ghi dở hoặc đã có dữ liệu.

Actual device provisioning/format/write, NVS SDK mount từ blob này trên bo,
power-loss và hardware enable đều **NOT_RUN** trong builder. Throwaway host
đã kiểm NVS/filesystem pack-unpack và các refusal; chúng không thay acceptance
thiết bị. Mọi app update bình thường chỉ thay app, giữ nguyên NVS/journal,
không tự kèm data-genesis.

### Giới hạn journal và cập nhật

Candidate mới đọc ledger ea140ed mà không reset state/ref/accounting.
Khi cần trì hoãn receipt STOP do queue đầy, một extension v1 tùy chọn giữ tối đa
16 receipt đã xử lý trong state (chưa cấp event ID); ACK mở chỗ thì promote thành
report cùng boot/time/outcome ban đầu. Old reader ea140ed không hiểu extension
này và sẽ fail closed. Trước rollback firmware cần kiểm journal compatibility;
không “sửa” bằng restore genesis.

Không có bypass unknown/reset-budget/ref-GC trong CLI. Ref đã applied giữ trần
256 qua reboot/compact: ở biên cần báo CONFIG_CAPACITY và planned intervention;
STOP độc lập vẫn dùng được, không reset ledger để nhận thêm config.
