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
