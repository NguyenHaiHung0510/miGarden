# Harness applicability

Public project workflow, 2026-10-22. [State](project-state.md) và [spec](specification.md) mô tả product; bảng dưới áp dụng 26-row migration assessment vào domain IoT. Đây không phải bản fork personal authority. Shared Owner authority có **một nguồn**, locatable từ `.local/owner-machine.json`; canonical decisions có một ledger theo cutover state. Personal policy, original grants, exact research/accounting và máy cá nhân ở `.local/`.

Public checkout dùng được để đọc product, lập proposal và làm task có grant của chính nó. Trên máy Owner, đọc shared authority và ledger theo locator khi task cần. Không suy quyền từ một summary hay từ việc file/skill có mặt.

## M01–M26 disposition

`I` = inherit/rebind shared authority; `R` = retain portable workflow; `A` = adapt theo domain; `B` = build domain contract/runbook; `L` = leave behind. “Documented” bên dưới chỉ là artifact applicability, không claim executable activation. Source assessment/provenance được giữ privately; đây là project disposition, không tạo policy authority mới.

| ID | Disposition | Áp dụng tại project / proof còn cần |
|---|---|---|
| M01 | I — authority/elevation | AGENTS + private locator trỏ một shared source; separate research, docs, build, flash, water và publication grants. Actual-root authority discovery do PM kiểm. |
| M02 | R — Owner work habits | Vietnamese, concrete behavior, evidence/counterargument/viable alternative; depth/medium theo nhu cầu. Đánh giá ở task thật, không compulsory menu. |
| M03 | R — intake/PM | MG-H1 appointment theo private continuity; không biến PM thành default project mode cho mọi chat. Coupled artifacts một lead; PM giữ acceptance/cutover. |
| M04 | R — roles/pins | Role/model source giữ shared, không registry cạnh tranh. MG-H1 exact requested/effective pins có PM receipts riêng; không worker tự suy từ requested. Không recursion. |
| M05 | R — grants/ledger | Canonical locator + SNAPSHOT label trước cutover; PM đồng bộ cuối. Stable IDs/history/budgets/counters không reset khi đổi root. Public state chỉ derived. |
| M06 | R — continuity | Private continuity + source manifest/snapshots; resume từ current source, không từ memory hay transcript copy public. Không generated-memory edits. |
| M07 | A — domain brief | State/spec/site: shared Zone A, dynamic pots, local schedule/source button, soil HOLD; stale repository/arrival claims reconciled. |
| M08 | R — skill governance | Giữ shared governance; 3/3 installed/frozen theo lane riêng, [usage](skills.md)/[notices](../THIRD_PARTY_NOTICES.md), source/license/adaptation receipts private. Không duplicate registry; actual-root discovery/selection NOT_RUN. |
| M09 | A — invocation/runners | Mọi Agent chọn fitting skills; docs/read-only khác build/flash/reset/provision/water runner. Trigger không cấp quyền side effects, metadata không là sandbox. |
| M10 | R — tools/providers | Reinventory actual capability ở root mới; không copy auth/profile. Tool visibility và effective data eligibility không suy từ cur_docs. Main lane runtime NOT_RUN. |
| M11 | A — resources | Operations bind COM/serial/board/build/wiring/output + owner, cùng ports/PIDs/containers. Một executor trên resource độc quyền; preserve unrelated resources. |
| M12 | A — QA lifecycle | Freeze → autonomous independent batch → consolidated handback → T1 reconcile/grouped fixes → affected retest. Simulator/LED/contained-water/field tách evidence. |
| M13 | A — evidence | Build/config/policy/boot/command/ack/physical feedback identity; receipts/counts/cost unknowns. Screenshot không chứng minh valve. Durable report path/date contract bên dưới. |
| M14 | A — useful delivery | Geometry thiếu chi tiết có nhãn, stale UI hoặc manual recovery có thể là contained limitation. Không miễn physical invariants, explicit gates hay repeated common-path friction. |
| M15 | B — fail-safe | Spec §§14–15 + operations: off/closed boot, unknown outcome block, cumulative ledger, no catch-up. Executable firmware/fault proof NOT_RUN. |
| M16 | B — threat brief | Device/command/config/update/replay/target/physical harm tại architecture + spec. Requested security assessment mới đi workflow phù hợp; không scanner/provision/key-change grant. |
| M17 | B — verification workflow | Operations mô tả launch/doctor/drive/evidence/cleanup theo plane. `verify-migarden` chưa tạo; chỉ promote sau observed useful lifecycle, không gọi runbook là tested skill. |
| M18 | A — resources/accounting | Outcome/dependency, finite resources, measured calls/tokens/USD/time; unset caps OPEN. Không estimate bằng 0, không reset quota/counters khi resume. |
| M19 | A — spatial/UI | A/SVG+B/Three.js chung scene version; provenance/uncertainty/units. Representation doc giữ selected design; model không chứng minh RF/flow. Scene build ngoài MG-H1. |
| M20 | B — build/toolchain | Operations bind exact board/SDK/tool versions/build/config; chưa chọn/cài/chạy toolchain theo skill presence. Runtime NOT_RUN. |
| M21 | A — release/CI | System và microSched adapter release riêng, versioned compatibility receipt. Giữ checks tại repo thực; không bê web suite sang docs/firmware. PM owns MG-H1 publication. |
| M22 | A — recovery | USB previous-image/config compatibility + fail-closed; OTA deferred. Git revert không là device restore proof; device recovery NOT_RUN. |
| M23 | A — schedule/automation | Event-driven coordination, finite retries, inspect effects trước retry. Local watering schedule không phải Codex automation; không copy cron/monitor. |
| M24 | R — Windows | UTF-8, exact CWD, cached tool identity, owned process only; Docker do Owner start khi cần. Không sửa auth/proxy hoặc kill-by-name để unblock. |
| M25 | R — evolution | Lesson → runbook → skill/control khi recurring benefit có evidence. NO CHANGE hợp lệ; migrate-harness optional future, không framework prerequisite. |
| M26 | L — baggage | Mimi tasks, old leases/allowlists/model routes, app auth/DB paths, unrelated slide library/global configs để lại nguồn. Adapter làm trong microSched vẫn đọc rules ở đó. |

## Practical execution and evidence

Before a batch, bind scope, source/build/config/fixture/oracle, permitted effects, capability and owned resources. Use bounded autonomous cases and one consolidated handback when independent execution is useful/required. T1 reconciles claims; builder self-check is not independent review. Reuse unchanged freeze evidence and retest affected/unproved contracts; do not multiply every matrix by every evidence plane.

Use `OBSERVED`, `INFERRED`, `UNVERIFIED`, `NOT_RUN` accurately. A safe refusal can be an explicit limitation; recurring refusal on an ordinary journey remains friction. Do not replace missing physical evidence with an inference. Preserve required gates and explicit Owner stops.

Durable live QA reports, when such a batch is actually run: dated folder `YYYY-mmm-DD`, start/end+timezone, short abstract and compact matrix, every screenshot embedded/indexed, source/receipts and verified links, attempt/worker-claim/T1-accepted/partial/fail/blocked/not-run counts with denominators and percentages, calls/tokens/USD/time with honest unknowns. Current migration checks have no live device/screenshots and do not pretend to be that batch.

Promotion ladder: approved source → files staged/installed → actual-root discovery → fitting selection → observed lifecycle → independent acceptance. Record each step separately. Consult [operations](operations.md) for prerequisites/rollback; runtime activation/device evidence remain explicit until receipts exist.
