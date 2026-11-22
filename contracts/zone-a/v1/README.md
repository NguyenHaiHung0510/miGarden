# miGarden Zone A device wire v1

`POST /api/migarden/device/exchange` uses `Authorization: Bearer <device token>`
over HTTPS with server CA and hostname verification. Wire version
`migarden.zone-a.v1`; one target, `zone-a` / `zone-a-v1`. The schema and
synthetic vectors include clarification A: STOP processing and occurrence
disposition. This schema checks shape; the arbiter and server must also check
semantics, authentication and provenance.

- A changed configuration has a new immutable `config_ref`. The three
  limits are positive multiples of 15 seconds, in this order:
  manual ≤ session ≤ cumulative ≤ 10800. The schedule duration is ≤ session.
  The effective range is start-inclusive, end-exclusive. Once/interval anchors
  have minute resolution, daily/weekly use UTC+7 and ISO weekdays 1–7.
  Disabled schedules still deliver policy. Desired and applied are distinct.
- A candidate does not authorize output. Only a matching immutable grant
  following a durable receipt can reach final local admission. The expiry is
  confirmed time + 45 seconds, never renewed; STOP invalidates the previous
  epoch. A valid authenticated STOP uses its own target/boot fence and has
  no dependency on OPEN readiness. `stop_processed` reports local processing,
  including without an active session. It says nothing about physical water.
- `session_started` is output execution evidence. A session's context is
  immutable; a disposition or request receipt is not a session.
  `occurrence_decided` also describes missed/denied scheduled opportunities.
  Unknown time and elapsed evidence remain null where appropriate. The server
  generates receive time separately and orders source sequence across reports
  without allowing old provenance to regress the current snapshot.
- ACK only after durable server validation/deduplication. Replay identical
  records; conflict on same ID with different content. Device accepts ACKs
  only for records included in that exchange and retains accounting/unknown
  independent of ACK. Fields not understood can be ignored by the firmware
  adapter; the schema describes the exact v1 producer shapes.
- A response has bounded lists (up to 16 of each kind); a request includes at
  most 16 reports. Controller may wake sync on a critical record, use a
  15-second idle / 5-second active poll, and respect the retry delay when
  provided. Server time is not a source of trusted controller clock.
# Reboot recovery and foreground hint

The existing `recovered` event uses exactly
`payload.reason: "REBOOT_INTERRUPTED_RECOVERED"` for a verified reboot
interruption settled by a fresh STOP. Its `session_id`, `request_id`,
`config_ref` and `trigger_source` match the original interrupted reservation.
The event boot/sequence/time are those of recovery. The previous unknown
`session_ended` remains unchanged, with null measured duration. An armed
reservation is recorded as unknown with null `started_at` and `ended_at`;
there is no started report or assertion of OPEN. The marker has no measured
duration. An unacknowledged marker can arrive in a later boot's exchange.
App correlation must not
release unrelated fences, amend the old outcome, or confuse the STOP request
ID with the original START request ID. The current schema and endpoints remain
unchanged. Deploy a correlated recipient before enabling this sender.

On a successful otherwise idle exchange, exactly `retry_after_seconds: 2`
can be a soft poll hint while the app's foreground presence lease is fresh.
It applies to one response only. No hint with desired work, candidates,
grants, STOP, reports, ACKs or in-flight work. Device progress and events
can wake immediately; a `Retry-After` HTTP header and error backoff remain
hard waits. Without the hint the base is 15 s idle / 5 s active. First page
open can still wait the remainder of the previous idle interval; no
physical latency guarantee follows.

## Negotiated quiet and optional health (N03/N04)

The v1 request/response and persistent journal remain the rollback contract.
New firmware offers `X-MiGarden-Extension: quiet-health-v1` with an ordinary v1
body first. Only a committed authenticated 200 response containing
`extension: quiet-health-v1` enables the optional `health` root. An old server
ignores the offer. If rollback reaches an old strict server while health is
enabled, its 422 makes the sender retry the unchanged control/reports without
health and use legacy cadence. No report is acknowledged by that rejection.

For an offered request the new server can return HTTP 202 with exactly
`{"protocol":"migarden.wait.v1","retry_after_seconds":5}`. This is only a
soft transport wait: no server time, identity, last-seen proof, config, command,
grant or ACK. The controller does not receive this object as an exchange input.
It keeps its local clock, scheduler, journal and safety gates. Local events,
backlog and context changes interrupt only soft waiting; HTTP Retry-After and
transient/error backoff stay hard. Ordinary negotiated idle transport is 5 s;
full foreground/active exchange uses a 2 s hint. Legacy remains 15 s/5 s.

The server gates before opening a DB session. A successful durable exchange
sets the six-hour reconciliation deadline. Authenticated human activity keeps
a five-minute window; actual watering and a newly committed terminal report
keep a ten-minute tail. Repeated OFF snapshots or health never renew it.
Cold hydrate is single flight. Failed authenticated exchanges retry at 5, 15,
30, then up to 60 seconds. The six-hour deadline applies to normal idle
reconciliation. A routing hint that has yet to recognize the credential receives
a transient response until the next bounded database lookup. Every permitted
full exchange checks credentials/revocation in the database and only publishes
ACKs after the outer commit. Process state schedules work and grants no rights.
Multiple server processes each have a bounded gate; this is not a shared lease.

Optional health contains bounded build/boot, uptime, reset code, free/min heap,
filtered connectivity status, pending reports and evidence gaps, with source
clock quality and server receive time. It rides existing eligible exchanges,
creates no health wake/timer and never controls admission or accounting.
The app's additive 0023 table allows one durable row per server UTC hour/Zone;
health validation/savepoint failure leaves the main report contract intact.
A lost outer commit still cannot ACK. At idle the ordinary six-hour exchange
carries a sample, so gaps between samples cannot establish a crash or outage.

[Extended schema](quiet-health.schema.json) and [compatibility vectors](quiet-health.vectors.json)
supplement the unchanged legacy schema/vectors. The app carries the same test vectors
under `backend/tests/fixtures/migarden/`. Apply additive schema, release a
backward-compatible app, then coordinate the separately authorized app-only
firmware flash. App rollback keeps 0023/data; production migration downgrade
is not a rollback step. Host/build proof does not establish device or flow.
