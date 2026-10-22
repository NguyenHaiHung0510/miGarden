# Operations and recovery

Prepared runbooks, 2026-10-22. These steps describe how a later authorized batch can run; **no app/DB/toolchain/build/flash/device/network probe was run by this migration lane**. Exact executable commands are deliberately unset until the actual stack, target and effects are approved. Do not turn placeholders or skill helpers into an execution grant.

## Read and resume

Open the actual project root, read AGENTS → state → relevant spec/site/architecture. On the Owner machine, read `.local/continuity.md` and `.local/owner-machine.json` before changing decisions. If ledger state says SNAPSHOT, send decisions to the current canonical writer; never append authority to a snapshot. Preserve prior IDs, grants and counters.

Classify the next task: inquiry/design, docs-only change, simulator/build, dry LED, contained water, field, release/recovery. Match permission and evidence to that plane. Unavailable capability holds that lane while independent approved work continues. Do not ask routine reapproval for the already granted MG-H1 scope.

## MG-H1 staging → actual root → cutover (PM-owned)

MG-H1 đã thực hiện quy trình này và public initial release trên `develop`; canonical ledger đã chuyển một lần, source cũ giữ redirect. Đọc current local locator/continuity và receipt trước khi dùng phần bên dưới như lịch sử hoặc rollback, không tự chạy lại một batch đã hoàn thành.

1. Assemble main and skill lanes without overwriting either write set. Freeze owned file list, source/target SHA-256 and public candidate; review `.local/migration-manifest.json` and lane receipts.
2. Verify `.gitignore` against representative private/media/secret paths and allowed finished geometry. Check actual staged Git tree/history; ignore rules cannot remove files already tracked. No raw media/private ledger/transcript/credential may enter public history.
3. Copy the reviewed candidate, including ignored private context locally, into the actual root. Preserve source bytes; do not move/delete originals. Resolve owner-machine locators only in `.local/`.
4. Run independent review and fresh-context task from the **actual root**: identify grant/canonical source, selected A+B, HOLD sensing, pending implementation and correct next action. Separate CLI root/discovery, metadata selection and actual skill workflow. Reading SKILL.md in an inherited old CWD does not prove automatic discovery.
5. Parent synchronizes current Owner ledger last, records cutover once, updates private locator to the new canonical ledger and leaves a historical pointer at the old source. Source snapshot stays immutable. Never leave both active. Keep this migration's snapshot warning until that step.
6. Parent applies release checks and publishes only reviewed exact candidate. Verify remote/ref/SHA/visibility. Root-copy/commit/push/remote/canonical cutover have their own receipts and are not claimed by this writer.

Rollback before cutover: restore/revert only owned candidate files using the manifest; current canonical ledger/source originals stay intact. After cutover, preserve decisions and append a superseding pointer if changing authority location. After publication, local revert cannot promise withdrawal of public bytes; do not force-push/delete history or source by default.

## Device/resource preflight (NOT_RUN)

Record target board/unit and ownership, observed USB/COM identity, host OS, exclusive serial owner, exact image/build/hash/config/policy versions, boot ID, wiring diagram, power/output state and permitted actions. COM enumeration unsupported/empty in a helper is a capability limitation, not proof that a board is absent. Do not scan/probe merely because a script offers it.

Use a dry low-voltage table; disconnect USB before rewiring. R1 DIP/LED parts are bench materials, not outdoor components. Seller ESD claims are not verified protection; do not power a board on an ESD bag/foam or connect a strap to an unverified earth/water pipe. No mains/pump/valve work follows from an R1 grant. Preserve unrelated COM sessions/processes/browser profiles; stop only resources owned by the task, by exact identity.

## Simulator/build plane (NOT_RUN)

Prerequisites: approved implementation packet; board/SDK/toolchain choice or explicit simulator target; source/config/fixture version; expected outputs, bounded workload and cleanup. No native toolchain is selected/installed by MG-H1.

Lifecycle: launch the declared target → doctor exact executable/tool/config/resource identity → drive frozen command/schedule/fault cases → save literal outputs/exit codes and manifest → cleanup owned resources. Capture build command, compiler/SDK/partition, artifact SHA, source revision and config. A mock proves contract behavior under assumptions, not provider/board/physical quality.

Fixtures cover TTL/dedup/target/boot/config/clock/unknown outcome, local schedule once-only/no catch-up, manual/remote/schedule arbitration and reboot recovery. Optional sensing tests remain conditional; do not reopen the HOLD lane.

## Dry LED on a real board (NOT_RUN)

Prerequisites: verified unit/cable/USB identity, approved pins/resistor/button wiring and safe power state, bounded simulator/dev-test endpoint and explicit dry-profile semantics. The current spec's water flag cannot be bypassed with made-up numeric policy. Reconcile a dry-specific profile in the later implementation packet before enabling any command route.

Drive local button and web → command/received/accepted-or-rejected/started/terminal receipts; tie serial/UI events to command/build/config/boot. Exercise bounded output duration, local STOP, disconnect/reconnect, duplicate/stale input and reboot default-OFF. LED evidence proves the electrical/software test output only. Do not report “watered”, “valve closed” or field readiness from it.

## Contained water / field (NOT_RUN, separately authorized)

Follow spec §§14–15 and `G0-WATER-SAFETY`: exact source/faucet/manual shutoff; pump states, pressure/flow and residual/siphon/drain-down; measured valve/driver/power/feedback/cutoff; enclosure/wiring/drainage/blast radius; Owner-approved numeric envelope bound to receipt and policy version.

Progress dry bench → contained water → supervised one-circuit Zone A → unattended only after required network/power/fault evidence. Apply existing accepted thresholds; no hidden defaults or common-path safety waiver. Circuit-level volume cannot become per-pot volume. Source electronic STOP supplements the independent mechanical shutoff; unknown outcome blocks new OPEN until fresh safe state.

## Network/cost probes (NOT_RUN)

Before any request/query, require machine-readable `target_env=local|throwaway`, parsed target/host/database and a production fingerprint denylist. Missing/ambiguous/unresolvable or production match rejects before I/O. Never log DSN/token/private host. The 60-second network heartbeat belongs only to this isolated lane, never production Neon.

Preserve spec provisional board-network gates: 24 h receipt ≥98%, no contiguous gap >10 min, reconnect ≤5 min after AP reboot; 7 d receipt ≥99%, no unexplained outage >30 min and recovery within buffer contract. These are acceptance baselines, not observed performance. Cost gate requires approved measurement period plus CU-hour/row/index growth caps and ≥7-day isolated measurement; all caps still OPEN. Whole microSched Neon-active goal <50%/day remains unproved. Protocol selection must compare reconnect/ack/resource effects under the same workload.

## USB update/recovery and later OTA (NOT_RUN)

Before flash, exact target/resource ownership and safe-idle state must be confirmed; preserve previous compatible image/config/partition and a recovery procedure. Record observed boot identity/version/config after update and verify outputs default off/closed. No replay of OPEN from disk, queue, retained message, backup or desired state.

Firmware rollback must demonstrate compatible persistence/schema and safe physical state. A Git revert or artifact hash does not prove device rollback. If cumulative ledger/time cannot be restored trustworthily, remain fail-closed through the approved recovery/window/cooldown policy. OTA remains deferred until exact-board authenticity/integrity, idle, power-loss and rollback evidence; a firmware registry does not enable a deployment route.

## Release and accounting

System source/image/scene and microSched adapter release independently; bind both refs and contract/schema/scene versions with compatibility tests. Adapter DB changes follow microSched migration/release policy, not this docs migration. A backup/redeploy never replays pending OPEN. No product release, credentials/admin changes, purchase or automation follows from a runbook.

Record actual commands/calls/tokens/USD/time when available and preserve unknowns. This writer's USD/token accounting is UNKNOWN; no new paid service/API calls were made for migration. Parent owns consolidated runtime/cost and required independent evidence. Timeout is not proof of no effects: inspect before retry and retain receipts.
