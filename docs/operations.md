# Operations and recovery design

These runbooks describe prerequisites, observations and recovery for future implementation. **No runnable build, simulator or device command is supplied by this repository yet.** Exact commands must come from the selected stack and target; the stages below are design guidance, not an executable quickstart.

## Evidence and resource identity

Separate simulator, dry LED, contained water, field and release evidence. For a reproducible run, record source/build/config/policy versions, fixture and expected behavior, actual outputs/exit codes, permitted effects and resource ownership. Device results also need exact unit, boot identity, wiring and physical observation. Preserve unrelated COM sessions/processes and stop only resources identified as belonging to the run.

Record what was observed, inferred or not run. Reuse unchanged source/config evidence and repeat checks affected by a change. A timeout can leave effects behind: inspect state before retrying. Freeze cases and expected outcomes before a test batch; reconcile its consolidated results before repairs and affected retests. Builder checks and independent review are distinct evidence.

## Device preflight

Identify board/unit, observed USB/COM mapping, host OS, exclusive serial owner, exact image/build/hash/config/policy, boot ID, wiring, power/output state and recovery path. Unsupported or empty COM enumeration is a capability limit, not proof that no board exists.

Use a dry low-voltage bench; disconnect USB before rewiring. DIP/LED bench parts are not outdoor components. Seller ESD claims are not verified protection: do not power a board on an ESD bag/foam or connect a strap to an unverified earth/water pipe. Mains/pump/valve work needs its own electrical design and competent inspection.

## Simulator and build

Prerequisites are a selected board/SDK/toolchain or simulator target, versioned source/config/fixture, expected outputs, bounded workload and cleanup procedure. Bind compiler/SDK/partition, exact build command and artifact SHA to the result.

Start the target, verify executable/config/resource identity, exercise frozen command/schedule/fault cases, retain literal outputs and clean up owned resources. Fixtures cover TTL/dedup/target/boot/config/clock/unknown outcome, once-only schedule/no catch-up, manual/remote/schedule arbitration and reboot recovery. Exact arbitration remains a design decision. Conditional sensing fixtures do not reopen HOLD. A mock proves behavior under its assumptions, not board performance.

## Dry LED on a real board

Require verified unit/cable/USB identity, selected pins/resistor/button wiring and safe power, plus an explicit dry profile and bounded simulator/dev-test endpoint. Dry-profile semantics are still open; do not bypass the specification's water flags with invented numeric policy.

Exercise local button and web intent through received/accepted-or-rejected/started/terminal events tied to command/build/config/boot. Check bounded duration, local STOP, disconnect/reconnect, duplicate/stale input and reboot default-OFF. LED results prove the test output; they do not establish delivered water, valve closure or field readiness.

## Contained water and field

Follow specification §§14–15 and `G0-WATER-SAFETY`: identify source/faucet/manual shutoff; measure pump states, pressure/flow, residual flow/siphon/drain-down; verify valve/driver/power/feedback/cutoff, enclosure/wiring/drainage and blast radius. Bind every numeric policy value to measured evidence and a policy version.

Progress from dry bench to contained water, then supervised Zone A, then unattended operation only after the required network/power/fault evidence. Circuit-level volume cannot become per-pot volume. Electronic STOP supplements the independent mechanical shutoff. Unknown outcome blocks new OPEN until fresh safe state is established.

## Network and cost probes

Before request/query, require machine-readable `target_env=local|throwaway`, a parsed target/host/database and production fingerprint denylist. Missing, ambiguous, unresolvable or production-matching targets reject before I/O. Never log DSN/token/private host. The 60-second network heartbeat belongs only to this isolated lane, never production Neon; see [target guard](specification.md#52-cost-probe-target-guard).

Preserve the provisional board-network gates: 24 h receipt ≥98%, no contiguous gap >10 min, reconnect ≤5 min after AP reboot; 7 d receipt ≥99%, no unexplained outage >30 min and recovery within the buffer contract. These are acceptance baselines, not observed performance. Cost acceptance needs a selected measurement period, CU-hour/row/index growth caps and ≥7-day isolated measurement; caps are still open. The whole-microSched Neon-active goal <50%/day is unproved. Compare transport reconnect/ack/resource effects under the same workload.

## USB update, recovery and later OTA

Before flash, confirm exact target/resource ownership and safe idle state. Preserve a compatible previous image/config/partition and recovery procedure. After update, observe boot version/config and verify outputs default off/closed. Never replay OPEN from disk, queue, retained message, backup or desired state.

Device rollback must demonstrate persistence/schema compatibility and safe physical state. Git revert and artifact hashes alone cannot do that. If cumulative ledger/time cannot be restored trustworthily, remain fail-closed through the specified recovery/window/cooldown policy. OTA remains deferred until exact-board authenticity/integrity, idle, power-loss and rollback evidence exists.

## Integration and release

miGarden system image/scene and microSched adapter release independently. Bind both source refs, command/schema/config/policy/scene versions and compatibility results. Adapter migrations and required checks follow the microSched repository; deployment alone does not apply or prove a schema migration. Backup/redeploy must not replay pending OPEN.

Retain actual commands, environment, outputs and limitations with each release. See [specification acceptance](specification.md#18-test-acceptance-và-traceability) for planned tests and [content rules](representation.md#public-content-and-private-sources) for publishable artifacts.
