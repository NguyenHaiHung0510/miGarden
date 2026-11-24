#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <vector>

namespace mg {
using Ms = int64_t;
inline constexpr Ms none = -1;
inline constexpr Ms second = 1000, minute = 60 * second, day = 24 * 60 * minute;
inline constexpr Ms minUtc = 0, maxUtc = 253402300799999LL; // 9999-12-31T23:59:59.999Z
inline constexpr int windowSeconds = 10800;
inline constexpr int ttlSeconds = 45;
inline constexpr size_t backlogMax = 512, batchMax = 16, remoteBatchMax = 16;
inline constexpr size_t intervalMax = 768, configHistoryMax = 256;
inline constexpr size_t deferredStopMax = 16;
inline constexpr size_t idMax = 128;
inline constexpr char protocol[] = "migarden.zone-a.v1";
inline constexpr char zone[] = "zone-a";
inline constexpr char binding[] = "zone-a-v1";
inline constexpr char exchangePath[] = "/api/migarden/device/exchange";

template<size_t N> struct Text {
    char data[N + 1]{};
    bool set(std::string_view s) {
        if (s.size() > N || s.find('\0') != s.npos) return false;
        if (!s.empty()) std::memcpy(data, s.data(), s.size());
        data[s.size()] = 0;
        return true;
    }
    size_t size() const { return std::strlen(data); }
    bool empty() const { return !data[0]; }
    std::string_view view() const { return {data, size()}; }
    const char* c_str() const { return data; }
    bool operator==(const Text& r) const { return view() == r.view(); }
    bool operator!=(const Text& r) const { return !(*this == r); }
};
using Id = Text<idMax>;
using BootId = Text<64>;
using EventId = Text<96>;
inline bool visible(std::string_view s) {
    if (s.empty()) return false;
    for (unsigned char c : s) if (c < 33 || c > 126) return false;
    return true;
}
enum class Quality : uint8_t { unknown, synced, holdover };
enum class Source : uint8_t { none, schedule, local_button, remote };
enum class Kind : uint8_t {
    config_applied, config_rejected, request_received, request_rejected,
    session_started, session_ended, fault, recovered, stop_processed, occurrence_decided
};
enum class Reason : uint8_t {
    none, busy, stop_held, stopped, remote_stop, time_unknown, clock_jump, config_missing,
    invalid_config, config_conflict, stale_config, config_capacity, wrong_target, context_changed,
    stale_boot, stale_epoch, invalid_duration, expired, invalid_ttl, no_candidate, intent_conflict,
    rolling_limit, interval_capacity, report_capacity, storage_fault, outcome_unknown, output_fault,
    reboot_interrupted, interlock, missed_boundary, deadline, remote_capacity, maintenance, unsafe_output,
    reboot_interrupted_recovered
};
enum class Outcome : uint8_t {
    none, completed, stopped, fault, unknown, close_requested, already_off, unconfirmed,
    started, skipped, rejected
};
enum class Recurrence : uint8_t { once, daily, weekly, interval };

struct Sample {
    Ms mono = none;
    Ms wall = none;
    Quality quality = Quality::unknown;
    bool usable() const { return mono >= 0 && wall >= minUtc && wall <= maxUtc && quality != Quality::unknown; }
};
struct Policy {
    int32_t manual = 900, session = 1800, window = windowSeconds, cumulative = 7200;
    bool operator==(const Policy& r) const {
        return manual == r.manual && session == r.session && window == r.window && cumulative == r.cumulative;
    }
};
struct Schedule {
    bool enabled = false;
    int32_t duration = 900;
    Ms from = 0, until = none;
    Recurrence type = Recurrence::daily;
    uint16_t localMinute = 7 * 60;
    uint8_t weekdays = 0; // bit 0 = Monday
    Ms at = none;          // once timestamp or interval anchor
    uint32_t intervalMinutes = 0;
    bool operator==(const Schedule& r) const {
        return enabled == r.enabled && duration == r.duration && from == r.from && until == r.until &&
            type == r.type && localMinute == r.localMinute && weekdays == r.weekdays &&
            at == r.at && intervalMinutes == r.intervalMinutes;
    }
};
struct Config {
    Id ref;
    Policy policy;
    Schedule schedule;
    bool operator==(const Config& r) const { return ref == r.ref && policy == r.policy && schedule == r.schedule; }
};
struct Intent {
    Id request, config;
    BootId boot;
    uint32_t epoch = 0;
    Ms confirmed = none, expires = none;
    int32_t duration = 0;
    bool target = false;   // exact Zone A + zone-a-v1 on the wire
    bool operator==(const Intent& r) const {
        return request == r.request && config == r.config && boot == r.boot && epoch == r.epoch &&
            confirmed == r.confirmed && expires == r.expires && duration == r.duration && target == r.target;
    }
};
struct Grant { Intent start; Id id; };
struct Stop {
    Id request;
    BootId boot;
    Ms confirmed = none;
    bool target = false;
};
struct Range { Ms start = 0, end = 0; };
struct Session {
    Id id, request, config;
    Source source = Source::none;
    int32_t duration = 0;
    Ms occurrence = none, started = none, accountedAt = none;
    // armed is a durable reservation, not evidence of OPEN; running has evidence.
    enum class Phase : uint8_t { idle, armed, running, unresolved } phase = Phase::idle;
};
struct Event {
    BootId boot;
    uint64_t sequence = 0;
    Kind kind = Kind::fault;
    Ms occurred = none;
    Quality quality = Quality::unknown;
    Id config, request, session;
    Source source = Source::none;
    Reason reason = Reason::none;
    Outcome outcome = Outcome::none;
    Ms started = none, ended = none, occurrence = none;
    int32_t plannedSeconds = 0;
    Ms outputMs = none;
};
struct Pending {
    Intent intent;
    uint64_t receivedSequence = 0;
};
enum class RecoveryCause : uint8_t { unspecified, reboot_running, reboot_armed, other };
struct Ledger {
    Config applied;
    Session session;
    Pending pending;
    Ms scheduleFloor = none;  // logical Zone A slot, independent of config_ref
    Ms accountingFloor = none; // safety timeline; reboot credits uptime only, never wall downtime
    bool unknown = false;    // shared source boundary, not a per-START flag
    uint64_t sequence = 0;   // persisted independently of ACK/compaction
    std::vector<Range> history;
    std::vector<uint64_t> configRefs; // bounded, retained for stale ref refusal
    std::vector<Event> deferredStops; // processed remote STOP, not enqueued; sequence assigned at promotion
    RecoveryCause recoveryCause=RecoveryCause::unspecified;
    Ms recoveryDebt=0,recoveryDebtAt=none; // full planned reservation, never measured output
};
inline Ms recoveryCharge(const Ledger& l,Ms at) {
    return l.recoveryDebt>0&&(at<l.recoveryDebtAt||at-l.recoveryDebtAt<windowSeconds*second)?l.recoveryDebt:0;
}
struct Snapshot {
    Sample time;
    uint32_t epoch = 0;
    Id applied;
    bool on = false, ready = false;
    Reason blocked = Reason::none;
    Session active;
    uint32_t elapsedSeconds = 0;
};
struct Exchange {
    bool hasConfig = false, invalidConfig = false;
    bool quietHealth = false, waiting = false;
    Config config;
    Ms serverTime = none; // transport provenance only; never sets controller clock
    std::vector<Intent> candidates;
    std::vector<Grant> grants;
    std::vector<Stop> stops;
    std::vector<EventId> acked;
    int32_t retryAfter = -1;
};
inline bool durationValid(int32_t s) { return s > 0 && s % 15 == 0; }
bool valid(const Config&);
bool due(const Schedule&, Ms slot);
Ms slotOf(Ms wall);
Ms rollingUsage(const std::vector<Range>&, Ms end);
Ms rollingPeak(const std::vector<Range>&, Range proposed);
uint64_t idFingerprint(std::string_view);
EventId eventId(const Event&);
Id occurrenceId(Ms slot);
const char* label(Quality);
const char* label(Source);
const char* label(Kind);
const char* label(Reason);
const char* label(Outcome);
bool parseUtc(std::string_view, Ms&);
void formatUtc(Ms, char (&)[32]);
}
