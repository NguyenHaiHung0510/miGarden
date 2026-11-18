# N01 · Commissioning package

**Bo hiện tại có LAB NVS:** dùng [current-layout runbook](CURRENT-COMMISSIONING.vi.md),
[commission_current.py](commission_current.py) và temporary commissioner image.
Đường này đã được hiện thực hóa: thêm release namespace qua NVS API khi toàn
namespace đích chưa có; hoặc LAB chỉ tạo journal, giữ raw NVS nguyên vẹn.
Không cần full-NVS repack. Full backup/preimage/hash/layout, ACL và exact
Owner approval là các bước cụ thể; actual data initialization vẫn HOLD.

Commissioner không chạy network/Controller hoặc GPIO outputs. Normal LAB và
release loại injection bằng build macro; PING/INFO/USB updater của commissioner
cho phép parent tự quay lại image đã pin sau verify. Không tự switch release,
không tự apply lịch/tưới. Read-only verify khi retry không ghi state; namespace/
file tồn tại hoặc dở bị apply từ chối, không overwrite/recovery bypass.

| Đầu ra | Mức đã kiểm |
|---|---|
| Current commissioner portable transaction | Synthetic conflict, beforehash/layout/approval, partial format/file/NVS failure, LAB no-NVS-write, preserving others. |
| Current host tool | Private captured backup + exact profile/device binding, manifest pins, required file set, paced frames, explicit execute/verify/finalize và no blind retry. |
| Legacy first-blank offline helper `commission.py` | NVS generator/parser + LittleFS pack/unpack, ACL/validators, negative/restore checks; **không dùng để đè bo hiện hữu**. |
| Actual commissioner/ACL of Owner inputs/device mutations | NOT_RUN bởi builder; operator cần grant, lease, exact before/after evidence. |

### Privacy và input contract

Dùng file/stdin private, không token/password trong argv hoặc echo. Windows
owner/SYSTEM/Administrators-only ACL (hoặc POSIX0700/0600) được tạo và kiểm
trước secret bytes; input và generated files bị shared/reparse đều fail.
Tên `.local` không là privacy proof. Chỉ bảo vệ output mới; không tự thay
permission input Owner đang có.

Device binding private khóa `protocol,zone_id,binding_ref,device_id,token_hash,
environment,app_ref`. Identity alphabet và token43-base64url đúng app v1;
origin HTTPS được so với firmware; một CA certificate có CA-basic-constraints,
validity và keyCertSign nếu extension KeyUsage có mặt. Release đòi
`fixture_verified:true` do Owner đã nghiệm thu, không do build/USB tự suy.
[Chi tiết và command sequence](CURRENT-COMMISSIONING.vi.md#trước-khi-dùng-chuẩn-bị-và-quyền).

### Helper first-blank (reference riêng)

Nếu **cả NVS và data đều proven blank** trên thiết bị mới, `commission.py prepare`
vẫn có thể tạo ảnh offline để review; actual write chưa được tool cấp.
Cần `--device-binding` mới bên cạnh profile, captures, app manifest và simulator.
`validate` bắt buộc `--manifest-sha256` parent giữ riêng, exact expected file
set và unpack **actual data image hiện tại** để đối chiếu duy nhất genesis.
Không đủ khi chỉ so saved unpacked copy hoặc size ảnh. Generator 0.3.0 và parser
Espressif v5.5.5 được pin hash; package/dependency isolated dưới `.local`.
Không thử helper này với current LAB NVS, không full-flash/copy NVS để đổi profile.

### Rollback và compatibility

Khả năng rollback cần xét **toàn bộ persisted journal**, không suy từ
`deferredStops.size()==0` hoặc queue đã drained. Mọi frame cũ vẫn được reader
scan; extended frame còn trong log sau promotion có thể khiến ea140ed fail
closed. New frame cap24KiB so với old20KiB cũng khác. Compaction không tự chứng
minh compatibility. Old-reader rollback trên dữ liệu thực là UNVERIFIED;
giữ fail closed và dùng exact copied-log test trước quyết định riêng.
Không format/xóa sổ/restore genesis để làm rollback “pass”.

### Host / CI

[Test C++](tests/commissioner_tests.cpp) dùng cùng transaction source của MCU.
[CLI tests](tests/test_current_commission.py) và [privacy/shape/actual-image tests](tests/test_commission.py)
chạy synthetic riêng, không serial. Portable [ci-host.sh](ci-host.sh) và
[workflow](../../.github/workflows/controller-host.yml) compile C++17 thực,
kiểm wire14 vectors với official ArduinoJson commit pin và jsonschema4.25.1.
CI không dùng private path/hardware/GPIO; Windows ACL/NVS/LittleFS integration
ở local receipts, không được coi là đã chạy GitHub hay device.
