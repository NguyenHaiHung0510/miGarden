# N01 · Bằng chứng firmware theo catalogue

Các tên ca dưới đây là ca trong [core_tests.cpp](tests/core_tests.cpp) hoặc
[verify-wire.py](tests/verify-wire.py), chạy cùng lõi firmware và adapter
wire nhưng dùng clock/journal/output giả. Cột sau chỉ phần cần kiểm tiếp.
**Mỗi hàng chỉ là mapping phạm vi**; PASS host ở một hàng không có nghĩa
hàng đó đã đạt đầy đủ ở app/thiết bị/hiện trường.

Chạy bằng `test-host.ps1` và `python -B code/controller/tests/verify-wire.py`.
Build script ghi manifest, log, ELF/MAP và byte/hash app-only dưới
`.local/n01/build`; staging riêng giữ kết quả test và genesis. Parent giữ
quyền fixture/COM và independent acceptance. Mọi cột về app, ESP32 N01 thật,
driver/nước/field ghi dưới đây đều **NOT_RUN trong lane builder**. Không lấy
kết quả USB bootstrap seed làm kết quả của candidate N01.

## Đủ 54 dòng Step 07

Delta theo independent review ea140ed (không thay evidence cũ):

| Finding | Ca mới / disposition | Bằng chứng cần ghép |
|---|---|---|
| F1 | `accountingAcrossReboot`, `uptimeAgesCarriedBudgetWithoutDowntimeCredit`: forward/backward jump, reboot không refill, 15 đã dùng vẫn còn105, powered uptime đủ3h | Reset/time/storage thật riêng; downtime không được suy từ SNTP. |
| F2 | `actualDelayedDeadlineIsAccounted`: delay7ms giữ elapsed thật qua reboot; PM chấp nhận small transient, không giảm duration/cap bằng headroom mới | Deadline/task lease latency/tải vật lý UNVERIFIED; không hứa zero-ms. |
| F3 | `stopReceiptSurvivesFullQueueAckAndRetry`, `deferredStopRecoversAfterReboot`, `deferredStopJournalFailureIsNotProof` | STOP close/dedup tách receipt, lưu16 pending receipt; full-queue+ACK, retry, reboot, lỗi ghi. Không physical proof. |
| F4 | `configRefBoundariesSurviveCompactAndBoot`: 255/256/257, stale ref, reboot+compact giữ trần | CONFIG_CAPACITY là giới hạn v1 explicit; không reset/GC protocol. |
| Capacity | `maximumLedgerAndDeferredReceiptsFit`: tối đa interval/ref/deferred cùng frame | Kiểm serialized bounds, không endurance/RAM peak/timing benchmark bo. |
| Commissioning | [CLI tests](tests/test_commission.py): 8 ca offline pack/parse, no-overwrite/nonblank/fixture/secret-redaction/tamper FAIL→restore PASS | Actual NVS/data write HOLD; SDK mount trên bo chưa kiểm. |

[review_regressions.py](tests/review_regressions.py) đối chiếu binary ea140ed đã
giữ nguyên với simulator mới: hai guard F1/F3 phải FAIL trên bản cũ rồi PASS bản
sửa. Chạy full host cũ + ca mới một lượt, sau đó QA độc lập delta theo freeze mới.
Không biến kết quả cũ thành kết quả candidate mới.

Nguồn: [catalogue gốc](../../docs/design/07-functional-view-specification.vi.md#catalogue).

| ID | Ca host có phần liên quan | Giới hạn và lane còn lại |
|---|---|---|
| S2-UC01 | configAndBoot, applyCutover | Report đúng ref; UI saved/pending/applied cần full app. |
| UC01-A01 | configInvalidConflictAndStale, wireVectors | Bundle sai bị từ chối local. Form/API validation cần app. |
| UC01-A02 | applyCutover, schedulePatternsAndCutover | Chỉ bản đã applied chi phối local; lưu/pause offline của web cần app. |
| UC01-A03 | configInvalidConflictAndStale | Từ chối ref local đã dùng hoặc cùng ref khác nghĩa; CAS409/late reports cần DB. |
| UC01-A04 | remoteHandoffAndStableReports, boundedQueueCompactionAndAck | ACK mất vẫn giữ report cùng ID; applied local không phụ thuộc ACK. |
| UC01-A05 | applyCutover | Idle-only apply, giữ deadline/ref phiên đang chạy. |
| UC01-A06 | persistBeforeOutput, corruptionBeforeAdmissionAndMissingStore | Lỗi ghi/chain hỏng fail closed; fault LittleFS trên bo chưa chạy. |
| UC01-A07 | schedulePatternsAndCutover, missedAndRejectedNotReplayed | Giữ dấu phút xuyên ref/pause/reboot. |
| UC01-A08 | configAndBoot | Applied với clock unknown vẫn not ready. |
| S2-UC02 | schedulePatternsAndCutover, allSourcesOneBudget | Kỳ local tạo output logic và deadline, van/nước chưa chạy. |
| UC02-A01 | schedulePatternsAndCutover, missedAndRejectedNotReplayed | Không double sau ref/reboot. |
| UC02-A02 | defaultsAndDeadline, missedAndRejectedNotReplayed | Holdover/no network ở core; các outage thật chạy riêng NET01/02. |
| UC02-A03 | configAndBoot, allSourcesOneBudget, corruptionBeforeAdmissionAndMissingStore | Không START khi không đủ gate, không chạy bù. |
| UC02-A04 | sourceBusyIsNotFifo, applyCutover | Busy từ chối, không hàng chờ; STOP ngang quyền. |
| UC02-A05 | applyCutover, simulator v1 roundtrip | STOP tại 17 s, old refs; nước thực chưa kiểm. |
| UC02-A06 | uncertainClosure, unknownAcrossSourcesAndAck | Mô phỏng output untrusted; van/driver/cutoff, flow chưa chạy. |
| UC02-A07 | unknownAcrossSourcesAndAck, failureAfterOutputAndTerminal | Boot sau possible execution fail closed; reset/power thật riêng. |
| UC02-A08 | persistBeforeOutput, damageDuringSession, boundedQueueCompactionAndAck | Lỗi ghi và đầy queue; storage thật chưa fault inject. |
| UC02-A09 | remoteContextAndReboot, applyCutover | Binding cố định, giữ context thực; remap vật lý không thuộc fixture. |
| UC02-A10 | clockChangesDoNotMoveDeadlineOrRefill | Wall jump không đổi deadline hoặc invent end. |
| S2-UC03 | defaultsAndDeadline, stopWithoutReadinessAndDedup | Logic nút cả online/offline; IO33/32 và tủ chưa test. |
| UC03-A01 | defaultsAndDeadline | Core không phụ thuộc mạng; tách AP/WAN/service ở bench. |
| UC03-A02 | configAndBoot, allSourcesOneBudget | Gate chặn START; STOP vẫn tới đường CLOSE. |
| UC03-A03 | sourceBusyIsNotFifo, stopBetweenReservationAndOutput | Cạnh tới khi bận/sau STOP không chạy muộn; bounce điện chưa đo. |
| UC03-A04 | defaultsAndDeadline, applyCutover | Source local_button, không gắn người bấm Owner. |
| UC03-A05 | unknownAcrossSourcesAndAck | Boot/unknown logic; mất điện/treo/cutoff/khóa tay thật chưa chạy. |
| UC03-A06 | uncertainClosure, unknownAcrossSourcesAndAck | Shared boundary chặn cả ba nguồn. |
| UC03-A07 | failureAfterOutputAndTerminal, damageDuringSession | Lỗi critical lúc đang chạy đóng và giữ uncertainty. |
| UC03-A08 | applyCutover, configInvalidConflictAndStale | Không swap context đã bind của phiên. |
| S2-UC04 | remoteHandoffAndStableReports, simulator v1 roundtrip | Hai giai đoạn và STOP v1; Owner preview/confirm/HTTPS thật cần app/bench. |
| UC04-A01 | remoteHandoffAndStableReports, configAndBoot | Device chỉ xử lý grant hợp lệ; UI quyền/freshness≤45 s cần backend. |
| UC04-A02 | expiryAndMismatchedGrant, finalOutputTimeFence, wallRollbackCannotRenewHandoff | TTL/handoff local hết hạn; offline confirm UX cần app. |
| UC04-A03 | remoteHandoffAndStableReports, expiryAndMismatchedGrant | Cùng ID không nhân tác động, nội dung grant khác bị từ chối. |
| UC04-A04 | unknownAcrossSourcesAndAck, failureAfterOutputAndTerminal | Local unknown chặn OPEN/ACK; web timeout sau possible grant cần app. |
| UC04-A05 | sourceBusyIsNotFifo, stopBeatsGrantAndLocal | STOP trước candidate/grant và invalidation epoch. |
| UC04-A06 | stopBeatsGrantAndLocal, unknownAcrossSourcesAndAck | Local invalidation; server cancel ungranted và reconciliation cần DB. |
| UC04-A07 | failureAfterOutputAndTerminal, uncertainClosure | Safe logic, thực nghiệm reset/van/điện chưa có. |
| UC04-A08 | remoteContextAndReboot, expiryAndMismatchedGrant | Sai boot/ref/duration không OPEN cũ; preview mới do app. |
| UC04-A09 | stopWithoutReadinessAndDedup, simulator v1 roundtrip | Chỉ có receipt khi local xử lý; UI chưa xác nhận nếu mất mạng cần app. |
| S2-UC05 | remoteHandoffAndStableReports, simulator v1 roundtrip | ACK/readback source/ref; UI history/freshness cần full app. |
| UC05-A01 | boundedQueueCompactionAndAck | Offline giữ report; UI đọc history stale cần app. |
| UC05-A02 | defaultsAndDeadline, syncCadence | Local deadline/retry tách biệt; microSched outage thật chưa kiểm. |
| UC05-A03 | clockChangesDoNotMoveDeadlineOrRefill, timeCodec | Giờ thiếu/nội suy deadline đúng dữ liệu; report đến muộn/received_at do app. |
| UC05-A04 | ackOnlySentAndPriorityReceipt, wireVectors | ACK lạ không xóa; 409 cùng ID/mâu thuẫn phải kiểm DB. |
| UC05-A05 | remoteHandoffAndStableReports, boundedQueueCompactionAndAck | Gửi lại cùng nội dung; không action replay; search DB dedup riêng. |
| UC05-A06 | boundedQueueCompactionAndAck, compactFailureKeepsOriginal, damageDuringSession | Full/compact fail/corrupt giữ gate; wear/power-loss LittleFS chưa chạy. |
| UC05-A07 | uncertainClosure, simulator v1 roundtrip | Không tạo physical feedback/volume; field evidence chưa có. |
| UC05-A08 | applyCutover, remoteContextAndReboot | Ref của history cũ không đổi theo bundle mới; web projection riêng. |
| UC05-A09 | configAndBoot, simulator v1 roundtrip | Snapshot có null và blocked_reason; loading/forecast/error UI cần app. |
| LC01 | stopBeatsGrantAndLocal, stopBetweenReservationAndOutput | STOP thắng, fences candidate/grant cũ; app hủy phía server cần QA. |
| LC02 | schedulePatternsAndCutover, missedAndRejectedNotReplayed | No-double/catch-up qua ref/reboot/clock. |
| LC03 | applyCutover, remoteContextAndReboot | Context fixed bound ở admission, report không resolve lại theo latest. |
| S2-AT-NET01 | defaultsAndDeadline, syncCadence | Chỉ xấp xỉ mất link ở host; tắt AP thật trước/trong phiên NOT_RUN. |
| S2-AT-NET02 | defaultsAndDeadline, syncCadence | Chỉ xấp xỉ exchange fail ở host; AP còn/WAN mất thật NOT_RUN. |

## Yêu cầu Step 01

Nguồn: [FR/NFR/SAFE](../../docs/design/01-purpose-and-requirements.vi.md#2-yêu-cầu-chức-năng).

| ID | Ca host có phần liên quan | Gate cần ghép tiếp |
|---|---|---|
| FR01 | configAndBoot, configInvalidConflictAndStale, applyCutover | Desired/CAS/UI trong app. |
| FR02 | schedulePatternsAndCutover, missedAndRejectedNotReplayed | Hai mất kết nối thật tách riêng. |
| FR03 | defaultsAndDeadline, stopBeatsGrantAndLocal | Nút/driver thật. |
| FR04 | applyCutover, simulator v1 roundtrip | Giữ 17s output; projection/auto-refresh do app. |
| FR05 | remoteHandoffAndStableReports, boundedQueueCompactionAndAck | ACK giữ sổ; server conflict409/roundtrip thật. |
| FR06 | remoteHandoffAndStableReports, expiryAndMismatchedGrant | Owner authorization/confirm và HTTPS device thật. |
| FR07 | configAndBoot, simulator v1 roundtrip | Forecast, stale/partial UI. |
| NFR01 | schedulePatternsAndCutover, remoteContextAndReboot | SLA, reliability thiết bị chưa đo. |
| NFR02 | defaultsAndDeadline, allSourcesOneBudget | Không suy capacity offline của bo từ host. |
| NFR03 | exactBudgetOracle, stopBetweenReservationAndOutput, persistBeforeOutput | Latency/cutoff thật. |
| NFR04 | applyCutover, uncertainClosure, simulator v1 roundtrip | Output report khác nước. |
| SAFE01 | defaultsAndDeadline, syncCadence | AP mất/WAN mất tách riêng. |
| SAFE02 | defaultsAndDeadline, syncCadence | Outage dịch vụ thật. |
| SAFE03 | maintenanceGate, defaultsAndDeadline | Treo control/device và hardware cutoff. |
| SAFE04 | unknownAcrossSourcesAndAck, failureAfterOutputAndTerminal | Reset/power loss thật. |
| SAFE05 | configAndBoot, clockChangesDoNotMoveDeadlineOrRefill, corruptionBeforeAdmissionAndMissingStore | SNTP/RTC/brownout thật. |
| SAFE06 | remoteHandoffAndStableReports, expiryAndMismatchedGrant, finalOutputTimeFence, wallRollbackCannotRenewHandoff | Backend authorization/freshness. |
| SAFE07 | stopBeatsGrantAndLocal, allSourcesOneBudget, exactBudgetOracle | Button timing và deadline dưới tải. |
| SAFE08 | unknownAcrossSourcesAndAck, uncertainClosure | Chỉ có Zone A triển khai; shared boundary giữ unknown. Recovery vật lý riêng. |
| SAFE09 | uncertainClosure | Chỉ mô phỏng evidence thất bại; van, flow, khóa tay và cutoff NOT_RUN. |
| SAFE10 | damageDuringSession, compactFailureKeepsOriginal, boundedQueueCompactionAndAck | LittleFS thật/giao dịch DB. |
| SAFE11 | configInvalidConflictAndStale, remoteContextAndReboot, applyCutover | Wiring đối chiếu tại tủ. |

Với mỗi lần chạy trên bo, gắn: exact build ID/digest/boot, slot, profile,
nguồn clock/network (AP, WAN, service), cấu hình applied, journal và connector.
Với tủ/nước cần riêng voltage S, thời gian STOP đến OFF, mất A còn B, đường
khóa tay độc lập và quan sát residual flow. ACK/LED/telemetry không chứng minh
van đã đóng. Nếu không có bằng chứng recovery cho unknown, vẫn khóa OPEN.
