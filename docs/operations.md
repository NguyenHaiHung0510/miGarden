# Operations and recovery design

These runbooks describe prerequisites, observations and recovery for future implementation. **No runnable control firmware build, control simulator or device command is supplied by this repository yet.** The spatial viewer and static information walkthrough are separate runnable artifacts. Exact commands must come from the selected stack and target; the stages below are design guidance, not an executable quickstart.

## Current design versus historical operational choices

Current safety outcomes are in [01](design/01-purpose-and-requirements.vi.md), [02](design/02-process-specification.vi.md) and [04](design/04-information-model.vi.md). [Design index](design/README.md) is the current baseline; steps 05–10, including operational/device design, are pending. This runbook retains evidence/resource/recovery principles; it does not select board/pins/framework, API/transport, network cadence, numeric policy or a field-enable protocol.

Every source uses the same local arbiter and cumulative accounting; STOP prioritizes CLOSE and prevents old pending START from opening later. Boot never restores OPEN or resets budget. Unknown after possible execution blocks all sources and all Zones on common source/accounting until independence and safe recovery are proven. Critical-storage failure during execution requires safe stop/cutoff/degraded handling, with honest evidence gaps. Independent mechanical shutoff and hardware feedback/cutoff remain necessary.

SAF-P01 at the index records **PROPOSED candidates only**, for Step 08 decision. No candidate value is an accepted maximum/default or measured safe limit; software limits cannot certify physical shutoff. Technical device work still requires its own exact task authorization, target identity and safe recovery path.

## Evidence and resource identity

Separate simulator, dry LED, contained water, field and release evidence. For a reproducible run, record source/build/config/policy versions, fixture and expected behavior, actual outputs/exit codes, permitted effects and resource ownership. Device results also need exact unit, boot identity, wiring and physical observation. Preserve unrelated COM sessions/processes and stop only resources identified as belonging to the run.

Record what was observed, inferred or not run. Reuse unchanged source/config evidence and repeat checks affected by a change. A timeout can leave effects behind: inspect state before retrying. Freeze cases and expected outcomes before a test batch; reconcile its consolidated results before repairs and affected retests. Builder checks and independent review are distinct evidence.

## Device preflight

Identify board/unit, observed USB/COM mapping, host OS, exclusive serial owner, exact image/build/hash/config/policy, boot ID, wiring, power/output state and recovery path. Unsupported or empty COM enumeration is a capability limit, not proof that no board exists.

Use a dry low-voltage bench; disconnect USB before rewiring. DIP/LED bench parts are not outdoor components. Seller ESD claims are not verified protection: do not power a board on an ESD bag/foam or connect a strap to an unverified earth/water pipe. Mains/pump/valve work needs its own electrical design and competent inspection.

## Simulator and build

Prerequisites are a selected board/SDK/toolchain or simulator target, versioned source/config/fixture, expected outputs, bounded workload and cleanup procedure. Bind compiler/SDK/partition, exact build command and artifact SHA to the result.

Start the target, verify executable/config/resource identity, exercise frozen command/schedule/fault cases, retain literal outputs and clean up owned resources. Fixtures cover TTL/dedup/target/boot/config/clock/unknown outcome, once-only schedule/no catch-up, manual/remote/schedule arbitration and reboot recovery. STOP priority/pending invalidation and busy START rejection are selected outcomes; remaining arbitration/order and implementation remain design decisions. Conditional sensing fixtures do not reopen HOLD. A mock proves behavior under its assumptions, not board performance.

## Dry LED on a real board

Require verified unit/cable/USB identity, selected pins/resistor/button wiring and safe power, plus an explicit dry profile and bounded simulator/dev-test endpoint. Dry-profile semantics and numeric water policy remain open; do not invent limits, reuse old flags as current permission, or bypass the task's permitted effects.

Exercise local button and web intent through received/accepted-or-rejected/started/terminal events tied to command/build/config/boot. Check bounded duration, local STOP, disconnect/reconnect, duplicate/stale input and reboot default-OFF. LED results prove the test output; they do not establish delivered water, valve closure or field readiness.

## Contained water and field

Use the current safety outcomes as requirements for a future explicitly authorized hardware validation plan: identify source/faucet/manual shutoff; measure pump states, pressure/flow, residual flow/siphon/drain-down; verify valve/driver/power/feedback/cutoff, enclosure/wiring/drainage and blast radius. Bind every numeric policy value to measured evidence and a policy version.

Progress from dry bench to contained water, then supervised Zone A, then unattended operation only after the required network/power/fault evidence. Shared-source/Zone volume cannot become per-pot volume; site pots do not create a managed-pot domain. Electronic STOP supplements the independent mechanical shutoff. Unknown outcome blocks new OPEN until fresh safe state is established.

## Network and cost probes — unselected legacy references

**REFERENCE ONLY.** The old [specification target guard](specification.md#52-cost-probe-target-guard) and network/cost gates describe a historical experimental lane. Its 60-second heartbeat, 24 h receipt ≥98%/gap ≤10 min/reconnect ≤5 min, 7 d receipt ≥99%/outage ≤30 min, ≥7-day cost period and Neon-active goal <50%/day are **not current acceptance requirements or observed performance**. Old transport, Neon topology, cadence and numeric gates have not been selected again by 01–04. Keep these numbers only as clearly labeled historical context, not defaults to execute.

A future authorized probe must identify an isolated local/throwaway target, permitted requests/costs, workload, measurement period and cleanup before I/O. Ambiguous or production targets must not be used through a test assumption. Keep DSNs/tokens/private hosts out of logs. Choose transport and cost criteria in their appropriate future design step using comparable reconnect/ack/resource evidence; this document does not launch a probe or authorize infrastructure/provider changes.

## USB update, recovery and later OTA

Before flash, confirm exact target/resource ownership and safe idle state. Preserve a compatible previous image/config/partition and recovery procedure. After update, observe boot version/config and verify outputs default off/closed. Never replay OPEN from disk, queue, retained message, backup or desired state.

Device rollback must demonstrate persistence/schema compatibility and safe physical state. Git revert and artifact hashes alone cannot do that. If cumulative ledger/time cannot be restored trustworthily, remain fail-closed until a trustworthy safe recovery can be established under the policy still to be designed. OTA remains deferred until exact-board authenticity/integrity, idle, power-loss and rollback evidence exists.

## Integration and release

miGarden system image/scene and microSched adapter release independently. Bind both source refs, command/schema/config/policy/scene versions and compatibility results. Adapter migrations and required checks follow the microSched repository; deployment alone does not apply or prove a schema migration. Backup/redeploy must not replay pending OPEN.

Retain actual commands, environment, outputs and limitations with each release. Use current 01–04 traceability to plan affected checks; the old [specification acceptance list](specification.md#18-test-acceptance-và-traceability) is historical reference, not the new release oracle. Follow [content rules](representation.md#public-content-and-private-sources) for publishable artifacts. Documentation/fixture checks do not grant field acceptance.
