# N01 · Commissioning bảo toàn bo đang có LAB NVS

Công cụ [commission_current.py](commission_current.py) và temporary firmware
`-Profile commissioner` đã được chuẩn bị cho **layout hiện tại**, không cần
repack/ghi lại toàn NVS. Chúng chưa được chạy trên bo bởi builder; actual
commissioning/data init và production credentials **HOLD** đến grant riêng.
[commission.py](commission.py) chỉ còn là helper offline cho ảnh first-blank,
không phải đường dùng trên bo có LAB NVS.

| Plan | Mutation được mô tả để Owner duyệt |
|---|---|
| LAB | Kiểm profile `mg-lab-v1/profile-v1` đã có đúng full digest và identity/token binding; **không ghi NVS**. Tạo `lab-v1.journal` bằng exclusive-create nếu chưa có. |
| Release | Bắt buộc namespace `mg-rel-v1` chưa tồn tại (kể cả namespace rỗng). Chỉ dùng `nvs_open/set_blob/commit` tạo `profile-v1` mới; không sửa LAB/namespace khác. Tạo riêng `release-v1.journal` bằng exclusive-create. |
| Data blank | Chỉ format LittleFS trong `spiffs 0x290000/0x160000` khi exact full-layout + toàn bộ bytes0xff + raw beforehash khớp backup + explicit approval. |
| Data LittleFS đã có | Mount read-only để inspect; không autoformat/grow. Nếu target journal chưa có, chỉ tạo file đó, so lại digest mọi file còn lại. |
| Namespace/journal đã có, file journal `.tmp`, partial hoặc state unknown | Apply trả conflict/HOLD; không overwrite, erase, retry tự động hoặc tạo genesis thay ledger. Verify cùng expected plan là read-only/idempotent. |

Commissioner **không boot Controller, không load/apply lịch, không bắt đầu
network task, không cấu hình GPIO điều khiển**. Nó giữ USB v1 PING/INFO/
app updater/reboot; marker/build riêng. Sau lần possible mutation không nhận
transaction mới; chỉ verify, PING/INFO hoặc rời image bằng reboot/upload.
Upload sau mutation phải khớp target length/hash đã verify.
Không tự chuyển sang release hoặc bắt đầu tưới.
Profile commissioner chặn SDK write/erase vào NVS và data ngay từ boot,
chỉ mở đúng vùng trong từng lệnh apply đã qua preflight. Fence này không chặn
app/otadata của USB updater. NVS cần recovery hoặc filesystem chưa nhận diện
sẽ bị từ chối thay vì tự sửa.

### Trước khi dùng: chuẩn bị và quyền

Dùng venv Python riêng với [requirements-commission.txt](requirements-commission.txt);
`pyserial==3.5` chỉ được import khi **operator** gọi lệnh dùng cổng sau grant.
Phần offline dùng `cryptography`,
`esp-idf-nvs-partition-gen==0.3.0` và parser Espressif v5.5.5 hash
`621bdbf0ac60e34ae190f63be10f0f1a4dd4d18c25ebab0cf56ff53a6f2b6c2c`.
Không cài global/provider. Firmware dùng Arduino-ESP32 3.3.12, default4M.
Layout binary phải đúng hash
`148b959cbff1c38aa8e1d5c0ba9d612c54997b945e56a63f41223eef650653a1`.
Không bootloader/partition/eFuse change.

Host tool thiết lập owner/SYSTEM/Administrators-only DACL (POSIX0700/0600)
**trước secret bytes**, kiểm ACL input/capture/bundle và mọi file sinh.
`.local` chỉ là vị trí; tên đó không chứng minh privacy. Shared hoặc reparse
file bị từ chối; không tự sửa quyền input hiện hữu. Chuẩn bị file mới trong
folder private do operator kiểm soát; tránh echo/copy vào chat. Test shared
synthetic→refusal→restore PASS là evidence của guard, không là claim về ACL
của Owner files.

Profile gồm `device_id, wifi_ssid, wifi_password, base_url, device_token, ca_pem`
và release cần `fixture_verified:true`. Identity đúng alphabet của app
`[A-Za-z0-9][A-Za-z0-9_.:-]{0,127}`, token chính xác43 base64url, origin
HTTPS DNS/IPv4 tương thích firmware (không userinfo/path/query/leading/trailing dot),
CA PEM đơn có CA-basic-constraints/validity/keyCertSign khi có KeyUsage.
LAB phải giữ đúng **bytes đã lưu**, không canonicalize rồi rewrite NVS.

Parent cung cấp file private device binding:
`{protocol,zone_id,binding_ref,device_id,token_hash,environment,app_ref}`.
Các hằng lần lượt `migarden.zone-a.v1, zone-a, zone-a-v1`; `token_hash` là
SHA256 token đúng file; `environment=qa` cho LAB, `production` cho release;
`app_ref` là40hex của revision app được parent review. Không in token/binding
values; pin digest bundle ngoài chính thư mục bundle khi review. Flag fixture
vẫn là quyết định nghiệm thu của Owner, không được tool suy ra từ USB/host test.

### Một chuỗi initialization cụ thể trên bo hiện tại

Các đường dưới là ví dụ; thay buildID bằng exact manifests của freeze cuối.
App-only upload commissioner cần quyền ghi slot app/otadata và reboot riêng.
`capture --execute-read` chỉ đọc NVS/data; cần quyền truy cập và lưu private
backup riêng. `execute` ghi NVS/data, `finalize` đổi app; cả hai cần approval
đúng phạm vi. `--dry-run` không mở COM.
Không enumerate cổng; parent giữ lease cụ thể và host USB transaction fence.

1. Operator kiểm exact SHA/manifest của temporary commissioner, updater tương
   thích và load vật lý đã cách ly; nạp **app-only** bằng retained USB v1.
   Giữ mọi NVS/data, bootloader/partition như cũ. Boot/PING/INFO đúng build.
2. Capture read-only riêng: full NVS20480 bytes và data1441792 bytes, trước
   mọi mutation. Readback từng block1024 được ACK để tránh UART overflow.
   Raw UART chứa secret NVS; dùng riêng với tool, không bật terminal/raw logging.
   Tool kiểm before/after snapshot ổn định, raw fullhash, lưu private backup
   trước khi có plan. Mã capture riêng không format/ghi NVS.
3. Offline prepare plan LAB/release từ capture pin, private profile/binding,
   target app/commissioner manifests cùng source commit, simulator pin và genesis mới.
   Existing LAB NVS được parse độc lập từ backup; release chỉ được thêm
   namespace mới, LAB profile phải match fullhash. Không sinh ảnh NVS/data để
   flash đè. Bundle chứa exact request/hash, backup, templates và runbook.
4. Owner review transaction ID, request digest, beforehash/layout và phạm vi
   data format nếu blank; copy template thành approval file riêng và chỉ sau
   grant đổi `approve:false` thành `true`. Blank không tự là approval.
5. Execute có `--execute-write` và approval đúng transaction/request. MCU
   recheck live layout/fullhash/entireFF ngay trước mutation. Tạo journal
   trước, rồi release namespace mới; fsync/commit/readback đúng fullhash,
   so logical inventory mọi key/value khác và file khác. LAB raw NVS hash
   còn nguyên; release NVS raw hash được ghi nhận mới vì API có GC.
6. Verify read-only bằng plan cũ, không mutate. Sau partial/timeout, host giữ
   `execution-attempt.json` và receipt possible-mutation; không cho execute
   lại mù. Namespace/file dở vẫn giữ để đối soát; không có cleanup/reset tự động.
7. Khi parent đã chấp nhận verify, dùng `finalize` với approval **riêng**
   cho exact target app digest. Commissioner cho release thiếu USB marker chỉ
   khi image khớp target đã verify; vẫn SHA input+full readback+image validation
   trước boot selection. LAB được xác nhận lại bằng marker/PING; release bằng
   output-only build/profile diagnostic, không đòi PONG hoặc chèn updater vào
   production. Đây là app upload có otadata/software restart, không auto-watering.
   Config/journal/runtime/tủ/nước vẫn có gate riêng.

```powershell
$py = './.local/n01/toolenv/Scripts/python.exe'
$OwnerPort = 'LEASED_PORT'
# 1. Parent upload temporary image qua retained USB helper/transaction đã review.
# 2. Read-only capture, chỉ sau read grant:
& $py code/controller/commission_current.py capture --profile lab --commissioner-manifest .local/n01/build/COMMISSIONER_ID/manifest.json --port $OwnerPort --out .local/private-commission/capture --dry-run
& $py code/controller/commission_current.py capture --profile lab --commissioner-manifest .local/n01/build/COMMISSIONER_ID/manifest.json --port $OwnerPort --out .local/private-commission/capture --execute-read

# 3. Offline; CAPTURE_SHA là digest receipt parent giữ; SIM_SHA từ host manifest.
& $py code/controller/commission_current.py prepare --profile lab --capture .local/private-commission/capture --capture-sha256 CAPTURE_SHA --private-config .local/private-commission/lab-profile.json --device-binding .local/private-commission/device-binding.json --app-manifest .local/n01/build/LAB_TARGET_ID/manifest.json --commissioner-manifest .local/n01/build/COMMISSIONER_ID/manifest.json --sim .local/n01/host-builds/FREEZE/sim.exe --sim-sha256 SIM_SHA --genesis .local/private-commission/genesis.journal --out .local/private-commission/plan --dry-run
# Bỏ --dry-run để tạo plan; không actual device effect.

# 4. Review exact file set + separately pinned manifest hash:
& $py code/controller/commission_current.py validate --bundle .local/private-commission/plan --manifest-sha256 PLAN_MANIFEST_SHA

# 5. Chỉ sau Owner grant + exact approval file riêng:
& $py code/controller/commission_current.py execute --bundle .local/private-commission/plan --manifest-sha256 PLAN_MANIFEST_SHA --port $OwnerPort --approval .local/private-commission/owner-approval.json --receipt .local/private-commission/execute-receipt --execute-write
& $py code/controller/commission_current.py verify --bundle .local/private-commission/plan --manifest-sha256 PLAN_MANIFEST_SHA --port $OwnerPort --receipt .local/private-commission/verify-receipt --execute-read
# 6. Chỉ sau approve target app riêng:
& $py code/controller/commission_current.py finalize --bundle .local/private-commission/plan --manifest-sha256 PLAN_MANIFEST_SHA --port $OwnerPort --approval .local/private-commission/finalize-approval.json --receipt .local/private-commission/finalize-receipt --execute-write
```

Genesis được tạo offline bằng simulator đúng freeze `sim genesis <new-file>`;
khóa/kiểm ACL file trước tool đọc. Có thể dùng file169bytes từ bundle synthetic
đã review nhưng vẫn kiểm fullhash và `sim verify-journal`: queue/ref/interval0,
unknownfalse, deferred0. Template/request hiện hữu không grant production.
Với release thay `--profile release`, capture theo release và target manifest
release; mọi key LAB giữ nguyên và file lab không bị thay.

### Kiểm đã làm và giới hạn

C++ transaction dùng cùng source trên host/MCU: plan/preimage/layout/approval,
existing namespace/file conflict, partial format/file/NVS failure, no-retry,
LAB zero NVS writes và logical preservation. Python current-plan synthetic
kiểm raw backup hashes, existing LAB profile, release absence, ACL, exact-file
set, frame pacing, mất reply sau attempt, verify và target upload bằng peer giả.
FD1–3 có negative→restore proof.
MCU NVS/fsync/format/power-loss/UART execution thực của commissioner và
commissioning credentials thật là **NOT_RUN/HOLD**. Một prepare thành công
không được coi là actual initialized hoặc fixture an toàn.

Rollback phải xét **toàn bộ journal**, không nhìn deferred count hiện tại.
Frame có extension cũ vẫn có thể còn trong log sau promote; old ea140ed reader
và trần20KiB có thể fail closed cả khi deferred hiện0. Compaction không tự
chứng minh rollback tương thích. Không reset/genesis để làm old reader chạy.
